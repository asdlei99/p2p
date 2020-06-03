// PHZ
// 2019-11-11

#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include <string>
#include <mutex>
#include <memory>
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
#include "libavutil/pixdesc.h"
}

namespace ffmpeg {

typedef std::shared_ptr<AVFrame> AVFramePtr;

class Decoder
{
public:
	typedef std::shared_ptr<AVPacket> AVPacketPtr;

	Decoder &operator=(const Decoder &) = delete;
	Decoder(const Decoder &) = delete;
	Decoder();
	virtual ~Decoder();

	virtual bool Open(AVStream* stream);
	virtual void Close();

	virtual int Send(AVPacketPtr& packet);
	virtual int Recv(AVFramePtr& frame);

	AVCodecContext* GetAVCodecContext();

private:
	std::mutex mutex_;
	AVCodecContext* codec_context_;
};

}

#endif
