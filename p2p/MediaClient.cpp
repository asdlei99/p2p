#include "MediaClient.h"
#include "message.hpp"
#include "log.h"

#define TEST_TOKEN "12345"

using namespace xop;

MediaClient::MediaClient()
	: event_cb_(nullptr)
{
	is_active_ = false;
	is_setup_  = false;
	is_play_   = false;
	is_start_play_ = false;
}

MediaClient::~MediaClient()
{

}

void MediaClient::SetEventCallback(EventCallback* event_cb)
{
	std::lock_guard<std::mutex> locker(mutex_);
	event_cb_ = event_cb;
}

bool MediaClient::Connect(const char* ip, uint16_t port, uint32_t timeout_msec)
{
	std::lock_guard<std::mutex> locker(mutex_);

	static std::once_flag flag;
	std::call_once(flag, []{
		enet_initialize();
	});

	if (is_started_) {
		Stop();
	}

	if (!event_client_.Connect(ip, port, timeout_msec)) {
		LOG("Event client connect failed.");
		return false;
	}

	is_started_ = true;

	SendActive();
	PollEvent(true, timeout_msec / 2);

	if (!is_active_) {
		event_client_.Close();
		is_started_ = false;
		return false;
	}
	
	if (!Start()) {
		Stop();
		return false;
	}

	SendSetup();
	return true;
}

void MediaClient::Close()
{
	std::lock_guard<std::mutex> locker(mutex_);
	Stop();	
}

bool MediaClient::Start()
{
	if (is_started_) {
		io_service_work_.reset(new asio::io_service::work(io_service_));
		io_service_thread_.reset(new std::thread([this] {
			io_service_.run();
		}));
		rtp_source_.reset(new RtpSource(io_service_));
		if (!rtp_source_->Open()) {
			LOG("Rtp source open failed.");
			rtp_source_.reset();
			return false;
		}

		rtp_source_->SetFrameCallback([this] (std::shared_ptr<uint8_t> data,
			size_t size, uint8_t type, uint32_t timestamp) {
			OnFrame(data.get(), size, type, timestamp);
			return true;
		});

		if (event_thread_ == nullptr) {
			event_thread_.reset(new std::thread([this] {
				PollEvent();
			}));
		}
	}

	return true;
}

void MediaClient::Stop()
{
	if (is_started_) {
		is_started_ = false;
		if (event_thread_ != nullptr) {
			event_thread_->join();
			event_thread_ = nullptr;
		}

		event_client_.Close();

		io_service_work_.reset();
		io_service_.stop();
		io_service_thread_->join();

		is_active_ = false;
		is_setup_ = false;
		is_play_ = false;
		is_start_play_ = false;

		while (!frame_queue_.empty()) {
			frame_queue_.pop();
		}
	}
}

bool MediaClient::IsConnected()
{
	std::lock_guard<std::mutex> locker(mutex_);
	return is_active_;
}

void MediaClient::PollEvent(bool run_once, uint32_t timeout_msec)
{
	uint32_t msec = timeout_msec;
	uint32_t cid = 0;
	uint32_t max_message_len = 1500;
	std::shared_ptr<uint8_t> message(new uint8_t[max_message_len]);

	while (is_started_ )
	{
		if (is_play_) {
			PlayStream();
			msec = 1;
		}

		int msg_len = event_client_.Recv(message.get(), max_message_len, msec);
		if (msg_len > 0) {
			if (!OnMessage((char*)message.get(), msg_len)) {
				is_active_ = false;
				event_client_.Close();
				break;
			}
		}
		else if(msg_len < 0) {
			is_active_ = false;
		}

		if (!event_client_.IsConnected()) {
			is_active_ = false;
		}

		if (run_once) {
			break;
		}

		if (!is_start_play_ && !is_play_) {
			StartPlay();
		}
	}
}

void MediaClient::StartPlay()
{
	if (!is_start_play_ && !is_play_) {
		if (rtp_source_->IsAlive()) {
			SendPlay();
			is_start_play_ = true;
		}
		else {
			rtp_source_->KeepAlive();
		}
	}
}

void MediaClient::PlayStream()
{
	if (is_play_) {
		int need_idr = 0;
		queue_mutex_.lock();
		while (!frame_queue_.empty()) {
			auto byte_array = frame_queue_.front();
			uint8_t type = (uint8_t)byte_array->ReadUint16BE();
			uint32_t timestamp = byte_array->ReadUint32BE();
			uint32_t size = byte_array->ReadUint32BE();
			if (event_cb_) {
				need_idr = event_cb_->OnFrame(byte_array->Data() + 10, size, type, timestamp);
			}
			frame_queue_.pop();
		}
		queue_mutex_.unlock();

		if (need_idr < 0) {
			// request idr
		}
	}
}

void MediaClient::OnFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp)
{
	auto byte_array = std::make_shared<ByteArray>();
	byte_array->WriteUint16BE(type);
	byte_array->WriteUint32BE(timestamp);
	byte_array->WriteUint32BE(size);
	byte_array->Write((char*)data, size);
	byte_array->Seek(0);

	std::lock_guard<std::mutex> locker(queue_mutex_);
	frame_queue_.push(byte_array);
}


bool MediaClient::OnMessage(const char* message, uint32_t len)
{
	int msg_type = message[0];
	ByteArray byte_array(message, len);

	uint32_t status_code = 0;

	switch (msg_type)
	{
	case MSG_ACTIVE_ACK:
	{
		ActiveAckMsg active_ack_msg;
		active_ack_msg.Decode(byte_array);
		status_code = active_ack_msg.GetStatusCode();
		if (!status_code) {
			is_active_ = true;
		}
	}
	break;
	case MSG_SETUP_ACK:
	{
		SetupAckMsg setup_ack_msg;
		setup_ack_msg.Decode(byte_array);
		status_code = setup_ack_msg.GetStatusCode();
		if (!status_code) {
			is_setup_ = true;
			rtp_source_->SetPeerAddress(event_client_.GetPeerAddress(),
				setup_ack_msg.GetRtpPort(), setup_ack_msg.GetRtcpPort());
		}
	}
	break;
	case MSG_PLAY_ACK:
	{
		PlayAckMsg play_ack_msg;
		play_ack_msg.Decode(byte_array);
		status_code = play_ack_msg.GetStatusCode();
		if (!status_code) {
			is_play_ = true;
		}
	}
	break;
	default:
		break;
	}

	return (status_code == 0);
}

void MediaClient::SendActive()
{
	std::string token = TEST_TOKEN;
	ByteArray byte_array;

	ActiveMsg msg(token.c_str(), (uint32_t)token.size() + 1);
	int size = msg.Encode(byte_array);
	if (size > 0) {
		event_client_.Send(byte_array.Data(), size);
	}
}

void MediaClient::SendSetup()
{
	uint16_t rtp_port = 0;
	uint16_t rtcp_port = 0;
	if (rtp_source_) {
		rtp_port = rtp_source_->GetRtpPort();
		rtcp_port = rtp_source_->GetRtcpPort();
	}

	if (rtp_port > 0 && rtcp_port > 0) {
		ByteArray byte_array;
		SetupMsg msg(rtp_port, rtcp_port);
		int size = msg.Encode(byte_array);
		if (size > 0) {
			event_client_.Send(byte_array.Data(), size);
		}
	}
}

void MediaClient::SendPlay()
{
	ByteArray byte_array;
	PlayMsg msg;
	int size = msg.Encode(byte_array);
	if (size > 0) {
		event_client_.Send(byte_array.Data(), size);
	}
}

