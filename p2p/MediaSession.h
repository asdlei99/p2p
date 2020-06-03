#ifndef MEDIA_SESSION_H
#define MEDIA_SESSION_H

#include "RtpSink.h"
#include "option.hpp"

class MediaSession : public Option
{
public:
	MediaSession(asio::io_service& io_service);
	virtual ~MediaSession();

	bool Open();
	void Close();

	void StartPlay();
	void StopPlay();

	int SendFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp);

	int Poll(char* msg, uint32_t max_msg_size);
	int Process(const char* msg, uint32_t msg_size);

	bool IsPlaying() const
	{ return is_playing_; }

	uint16_t GetRtpPort()  const
	{ return rtp_sink_->GetRtpPort(); }

	uint16_t GetRtcpPort() const
	{ return rtp_sink_->GetRtcpPort(); }

private:
	asio::io_service& io_service_;
	std::shared_ptr<RtpSink> rtp_sink_;

	int64_t  last_tick_ts_;
	uint32_t rtt_;
	bool is_playing_;
};

#endif