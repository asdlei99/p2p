// PHZ
// 2019-11-11

#ifndef FFMPEG_DEMUXER_H
#define FFMPEG_DEMUXER_H

#include <string>
#include <mutex>
#include <memory>

extern "C" {
#include "libavformat/avformat.h"
}

namespace ffmpeg {

typedef std::shared_ptr<AVPacket> AVPacketPtr;

class Demuxer
{
public:
	Demuxer &operator=(const Demuxer &) = delete;
	Demuxer(const Demuxer &) = delete;
	Demuxer();
	virtual ~Demuxer();

	virtual bool Open(std::string url);
	virtual void Close();

	virtual int Read(AVPacketPtr& pkt);

	AVStream* GetVideoStream();
	AVStream* GetAudioStream();

private:
	std::mutex  mutex_;
	std::string url_;

	AVFormatContext* format_context_;
	AVStream* video_stream_;
	AVStream* audio_stream_;
	uint32_t  video_index_;
	uint32_t  audio_index_;
};

}

#endif

