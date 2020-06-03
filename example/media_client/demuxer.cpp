#include "Demuxer.h"
#include "log.h"

using namespace ffmpeg;

static double r2d(AVRational r)
{
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

Demuxer::Demuxer()
	: format_context_(nullptr)
	, video_stream_(nullptr)
	, audio_stream_(nullptr)
	, video_index_(-1)
	, audio_index_(-1)
{

}

Demuxer::~Demuxer()
{

}

bool Demuxer::Open(std::string url)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (format_context_ != nullptr) {
		LOG("demuxer was opened.");
		return false;
	}

	AVDictionary* options = nullptr;
	
	//av_dict_set(&options, "buffer_size", "1024000", 0);
	//av_dict_set(&options, "max_delay", "500000", 0);
	//av_dict_set(&options, "stimeout", "20000000", 0);  
	av_dict_set(&options, "rtsp_transport", "tcp", 0);

	int ret = avformat_open_input(&format_context_, url.c_str(), 0, &options);
	if (ret != 0) {
		AV_LOG(ret, "open %s failed.", url.c_str());
		return false;
	}

	avformat_find_stream_info(format_context_, 0);
	av_dump_format(format_context_, 0, url.c_str(), 0);

	video_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (video_index_ >= 0) {
		video_stream_ = format_context_->streams[video_index_];
	}

	//audio_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	//if (audio_index_ >= 0) {
	//	audio_stream_ = format_context_->streams[audio_index_];
	//}

	url_ = url;
	return true;
}

void Demuxer::Close()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (format_context_ != NULL) {
		avformat_close_input(&format_context_);
		format_context_ = NULL;
	}

	video_stream_ = NULL;
	audio_stream_ = NULL;
	video_index_ = -1;
	audio_index_ = -1;
}

int Demuxer::Read(AVPacketPtr& pkt)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (!format_context_)  {
		return -1;
	}
	
	pkt.reset(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
	int ret = av_read_frame(format_context_, pkt.get());
	if (ret != 0) {
		return -1;
	}

	pkt->pts = (int64_t)(pkt->pts*(1000 * (r2d(format_context_->streams[pkt->stream_index]->time_base))));
	pkt->dts = (int64_t)(pkt->dts*(1000 * (r2d(format_context_->streams[pkt->stream_index]->time_base))));
	return 0;
}

AVStream* Demuxer::GetVideoStream() 
{
	std::lock_guard<std::mutex> locker(mutex_);
	return video_stream_;
}

AVStream* Demuxer::GetAudioStream()
{
	std::lock_guard<std::mutex> locker(mutex_);
	return audio_stream_;
}