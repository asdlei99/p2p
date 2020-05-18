#include "MediaSession.h"
#include "message.hpp"
#include <chrono>

using namespace std::chrono;
using namespace xop;

static inline int64_t GetTimestamp()
{
	auto time_point = time_point_cast<milliseconds>(high_resolution_clock::now());
	return (int64_t)time_point.time_since_epoch().count();
}

MediaSession::MediaSession(asio::io_service& io_service)
	: io_service_(io_service)
	, rtp_sink_(new RtpSink(io_service))
	, is_playing_(false)
{
	last_tick_ts_ = GetTimestamp();
	rtt_ = 5;
}

MediaSession::~MediaSession()
{
	Close();
}

bool MediaSession::Open()
{
	return rtp_sink_->Open();
}

void MediaSession::Close()
{
	rtp_sink_->Close();
}

void MediaSession::StartPlay()
{
	is_playing_ = true;
}

void MediaSession::StopPlay()
{
	is_playing_ = false;
}

int MediaSession::SendFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp)
{
	//printf(" - timestamp: %u, type: %u, size: %u \n", timestamp, type, size);
	std::shared_ptr<uint8_t> data_ptr(new uint8_t[size]);
	memcpy(data_ptr.get(), data, size);
	rtp_sink_->SendFrame(data_ptr, size, type, timestamp);
	return 0;
}

int MediaSession::Poll(char* msg, uint32_t max_msg_size)
{
	int64_t now_ts = GetTimestamp();
	ByteArray byte_array;

	if (last_tick_ts_ + 250 < now_ts) {
		last_tick_ts_ = now_ts;
		PingMsg ping_msg;
		ping_msg.SetTimestamp((uint32_t)now_ts);
		ping_msg.Encode(byte_array);
	}

	uint32_t msg_size = (uint32_t)byte_array.Size();
	if (msg_size > 0 && max_msg_size >= msg_size) {
		memcpy(msg, byte_array.Data(), msg_size);
	}

	return msg_size;
}

int MediaSession::Process(const char* msg, uint32_t msg_size)
{
	int msg_type = msg[0];
	ByteArray byte_array((char*)msg, msg_size);
	if (msg_type == MSG_PONG) {
		PongMsg pong_msg;
		pong_msg.Decode(byte_array);
		uint32_t now_ts = (uint32_t)GetTimestamp();
		uint32_t rtt = now_ts - pong_msg.GetTimestamp();
		//printf("rtt: %u\n", rtt);
		if (rtt < 5) {
			rtt = 5;
		}

		/* rtt Æ½»¬ */
		rtt_ = (7 * rtt_ + rtt) / 8;
	}

	return 0;
}