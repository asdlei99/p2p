#include "Parser.h"

using namespace ffmpeg;

Parser::Parser()
	: avio_context_(nullptr)
	, format_context_(nullptr)
	, avio_buffer_(nullptr)
	, data_(nullptr)
	, size_(0)
{
	
}

Parser::~Parser()
{
	Destroy();
}

bool Parser::Init()
{
	format_context_ = avformat_alloc_context();
	if (format_context_ == nullptr) {
		printf("[Parser] avformat_alloc_context() failed.\n");
		return false;
	}

	avio_buffer_ = (uint8_t *)av_malloc(AVIO_BUFFER_SIZE);
	if (avio_buffer_ == nullptr) {
		printf("[Parser] av_malloc() failed.\n");
		return false;
	}

	avio_context_ = avio_alloc_context(avio_buffer_, AVIO_BUFFER_SIZE, 0, nullptr, &Read, nullptr, nullptr);
	if (avio_context_ == nullptr) {
		printf("[Parser] avio_alloc_context() failed.\n");
		return false;
	}

	return true;
}

void Parser::Destroy()
{
	if (format_context_ != nullptr) {
		avformat_close_input(&format_context_);
		avformat_free_context(format_context_);
		format_context_ = nullptr;
	}

	if (avio_buffer_ != nullptr) {
		av_freep(&avio_buffer_);
		avio_buffer_ = nullptr;
	}

	if (avio_context_) {		
		avio_context_free(&avio_context_);
		avio_context_ = nullptr;
	}
}

bool Parser::Parse(uint8_t *data, uint32_t size)
{
	data_ = data;
	size_ = size;

	Destroy();
	if (!Init()) {
		return false;
	}

	format_context_->pb = avio_context_;
	avio_context_->opaque = this;
	if (avformat_open_input(&format_context_, nullptr, nullptr, nullptr) < 0) {
		printf("[Parser] avformat_open_input() failed.");
		return false;
	}

	if (avformat_find_stream_info(format_context_, nullptr) < 0) {
		printf("[Parser] avformat_find_stream_info() failed.");
		return false;
	}

	av_dump_format(format_context_, 0, "video stream info", 0);
	return true;
}

int Parser::Read(void *opaque, uint8_t *buf, int buf_size)
{
	Parser *parser = (Parser*)opaque;

	if (parser->size_ > 0) {
		int size = ((int)parser->size_ <= buf_size) ? parser->size_ : buf_size;
		memcpy(buf, parser->data_, size);
		parser->size_ = 0;
		return size;
	}

	return -1;
}

AVFormatContext* Parser::GetAVFormatContext()
{
	return format_context_;
}