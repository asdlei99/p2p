#include "MediaServer.h"
#include "message.hpp"
#include "log.h"

#define TEST_TOKEN "12345"

using namespace xop;

MediaServer::MediaServer()
	: event_cb_(nullptr)
{

}

MediaServer::~MediaServer()
{
	Stop();
}

void MediaServer::SetEventCallback(EventCallback* event_cb)
{
	std::lock_guard<std::mutex> locker(mutex_);
	event_cb_ = event_cb;
}

bool MediaServer::Start(const char* ip, uint16_t port)
{
	std::lock_guard<std::mutex> locker(mutex_);

	static std::once_flag flag;
	std::call_once(flag, [] {
		enet_initialize();
	});

	if (is_started_) {
		return false;
	}

	if (!event_server_.Start(ip, port, kMaxConnectios)) {
		LOG(" Event server start failed.");
		return false;
	}

	is_started_ = true;
	event_thread_.reset(new std::thread([this] {
		EventLoop();
	}));
	
	io_service_work_.reset(new asio::io_service::work(io_service_));
	/*io_service_thread_.reset(new std::thread([this] {
		io_service_.run();
	}));*/
	return true;
}

void MediaServer::Stop()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (is_started_) {
		is_started_ = false;
		event_thread_->join();
		event_thread_ = nullptr;
		event_server_.Stop();
		media_sessions_.clear();

		io_service_work_.reset();
		io_service_.stop();
		//io_service_thread_->join();

		while (!frame_queue_.empty()) {
			frame_queue_.pop();
		}
	}
}

void MediaServer::EventLoop()
{
	uint32_t msec = 1;
	uint32_t cid = 0;
	uint32_t max_message_len = 1500;
	std::shared_ptr<uint8_t> message(new uint8_t[max_message_len]);

	while (is_started_)  
	{
		/* 处理信令 */
		int msg_len = event_server_.Recv(&cid, message.get(), max_message_len, msec);
		if (msg_len > 0) {
			OnMessage(cid, (char*)message.get(), msg_len);
		}
		else if (msg_len < 0) {			
			media_sessions_.erase(cid);
		}

		/* 处理媒体数据 */
		queue_mutex_.lock();
		while (!frame_queue_.empty()) {
			auto byte_array = frame_queue_.front();
			for (auto iter : media_sessions_) {
				auto session = iter.second;
				if (session->IsPlaying()) {
					uint8_t type = (uint8_t)byte_array->ReadUint16BE();
					uint32_t timestamp = byte_array->ReadUint32BE();					
					uint32_t size = byte_array->ReadUint32BE();
					if (size > 0) {
						session->SendFrame(byte_array->Data()+10, size, type, timestamp);
					}					
				}
			}
			frame_queue_.pop();
		}
		queue_mutex_.unlock();
	}
}

void MediaServer::OnMessage(uint32_t cid, const char* message, uint32_t len)
{
	int msg_type = message[0];
	ByteArray byte_array((char*)message, len);

	uint32_t status_code = 0;

	switch(msg_type)
	{
	case MSG_ACTIVE:
		status_code = OnActive(cid, byte_array);
		break;
	case MSG_SETUP:
		status_code = OnSetup(cid, byte_array);
		break;
	case MSG_PLAY:
		status_code = OnPlay(cid, byte_array);
	default:
		break;
	}

	if (status_code != 0) {
		event_server_.Close(cid);
		media_sessions_.erase(cid);
	}
}

uint32_t MediaServer::OnActive(uint32_t cid, ByteArray& message)
{
	ActiveMsg active_msg;
	if (active_msg.Decode(message) != message.Size()) {
		return 1;
	}

	uint32_t status_code = 0;

	if (event_cb_) {
		if (!event_cb_->Connect(active_msg.GetToken())) {
			status_code = 1; /* token error */
		}
	}

	ByteArray byte_array;
	ActiveAckMsg ack_msg(status_code);
	ack_msg.SetUid(active_msg.GetUid());
	ack_msg.SetCSeq(active_msg.GetCSeq());
	int size = ack_msg.Encode(byte_array);
	if (size > 0) {
		event_server_.Send(cid, byte_array.Data(), size);
	}

	return status_code;
}

uint32_t MediaServer::OnSetup(uint32_t cid, ByteArray& message)
{
	SetupMsg setup_msg;
	if (setup_msg.Decode(message) != message.Size()) {
		return 1;
	}

	uint32_t status_code = 0;
	auto session = std::make_shared<MediaSession>(io_service_);

	if (!session->Open()) {
		status_code = 500;
	}
	else {
		media_sessions_.emplace(cid, session);
	}

	ByteArray byte_array;
	SetupAckMsg ack_msg;
	ack_msg.SetUid(setup_msg.GetUid());
	ack_msg.SetCSeq(setup_msg.GetCSeq());
	int size = ack_msg.Encode(byte_array);
	if (size > 0) {
		event_server_.Send(cid, byte_array.Data(), size);
	}

	return status_code;
}

uint32_t MediaServer::OnPlay(uint32_t cid, ByteArray& message)
{
	PlayMsg play_msg;
	if (play_msg.Decode(message) != message.Size()) {
		return 1;
	}

	auto iter = media_sessions_.find(cid);
	if (iter != media_sessions_.end()) {
		auto session = iter->second;
		session->StartPlay();
	}
	else {
		return 1;
	}

	uint32_t status_code = 0;
	if (event_cb_) {
		event_cb_->StartPlay();
	}

	ByteArray byte_array;
	PlayAckMsg ack_msg;
	ack_msg.SetUid(play_msg.GetUid());
	ack_msg.SetCSeq(play_msg.GetCSeq());
	int size = ack_msg.Encode(byte_array);
	if (size > 0) {
		event_server_.Send(cid, byte_array.Data(), size);
	}

	return status_code;
}

int MediaServer::SendFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp)
{
	auto byte_array = std::make_shared<ByteArray>();
	byte_array->WriteUint16BE(type);
	byte_array->WriteUint32BE(timestamp);
	byte_array->WriteUint32BE(size);
	byte_array->Write((char*)data, size);
	byte_array->Seek(0);

	if (is_started_) {
		std::lock_guard<std::mutex> locker(queue_mutex_);
		if (frame_queue_.size() >= kMaxFrameLength) {
			return -1;
		}

		frame_queue_.push(byte_array);
	}

	return 0;
}