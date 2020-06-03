#ifndef FFMPEG_PARSER_H
#define FFMPEG_PARSER_H

#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

namespace ffmpeg {

class Parser
{
public:
	Parser();
	Parser &operator=(const Parser &) = delete;
	Parser(const Parser &) = delete;
	virtual ~Parser();

	bool Parse(uint8_t *data, uint32_t size);

	AVFormatContext* GetAVFormatContext();

private:
	bool Init();
	void Destroy();

	static int Read(void *opaque, uint8_t *buf, int buf_size);

	uint8_t* data_;
	uint32_t size_;
	uint8_t* avio_buffer_;
	AVIOContext* avio_context_;
	AVFormatContext* format_context_;

	const int AVIO_BUFFER_SIZE = 1024000;
};

}


#endif