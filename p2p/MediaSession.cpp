#include "MediaSession.h"

MediaSession::MediaSession(asio::io_service& io_service)
	: io_service_(io_service)
	, rtp_sink_(new RtpSink(io_service))
	, is_playing_(false)
{

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
