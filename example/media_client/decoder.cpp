#include "decoder.h"
#include "log.h"

using namespace ffmpeg;

Decoder::Decoder()
	: codec_context_(nullptr)
{

}

Decoder::~Decoder()
{

}

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
		NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
	const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

int test_hw_decoder(AVCodecContext* codec_context, AVCodec* codec)
{
	static const char* name = "cuda"; // cuda dxva2 qsv d3d11va

	AVHWDeviceType type = av_hwdevice_find_type_by_name(name);
	if (type == AV_HWDEVICE_TYPE_NONE) {
		fprintf(stderr, "Device type %s is not supported.\n", name);
		fprintf(stderr, "Available device types:");
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
			fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
		fprintf(stderr, "\n");
	}

	for (int i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
		if (!config) {
			fprintf(stderr, "Decoder %s does not support device type %s.\n",
				codec->name, av_hwdevice_get_type_name(type));
			return -1;
		}
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			config->device_type == type) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}

	codec_context->get_format = get_hw_format;

	if (hw_decoder_init(codec_context, type) < 0) {
		return -1;
	}

	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		return -1;
	}

	return 0;
}

bool Decoder::Open(AVStream* stream)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (codec_context_ != nullptr) {
		LOG("codec was opened.");
		return false;
	}

	//av_log_set_level(AV_LOG_DEBUG);

	//AVCodec* codec = avcodec_find_decoder_by_name("hevc_cuvid");//
	AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

	if (!codec) {
		LOG("decoder(%d) not found.", (int)stream->codecpar->codec_id);
		return false;
	}

	codec_context_ = avcodec_alloc_context3(codec);
	if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
		LOG("avcodec_parameters_to_context() failed.");
		return false;
	}

	//return test_hw_decoder(codec_context_, codec) == 0;

	//codec_context_->thread_count = 1;
	//codec_context_->flags |= AV_CODEC_FLAG_LOW_DELAY;      // low delay decoding
	//codec_context_->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT; // allow display of corrupt frames and frames missing references
	//codec_context_->flags2 |= AV_CODEC_FLAG2_SHOW_ALL;

	int ret = avcodec_open2(codec_context_, 0, 0);
	if (ret != 0) {
		AV_LOG(ret, "open decoder(%d) failed.", (int)stream->codecpar->codec_id);
		avcodec_free_context(&codec_context_);
		codec_context_ = nullptr;
		return false;
	}


	return true;
}

void Decoder::Close()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (codec_context_ != nullptr) {
		avcodec_close(codec_context_);
		avcodec_free_context(&codec_context_);
		codec_context_ = nullptr;
	}
}

int Decoder::Send(AVPacketPtr& packet)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (codec_context_ == NULL) {
		return -1;
	}

	int ret = avcodec_send_packet(codec_context_, packet.get());
	if (ret != 0) {
		AV_LOG(ret, "send packet failed.");
		return -1;
	}

	return 0;
}

int Decoder::Recv(AVFramePtr& frame)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (codec_context_ == NULL) {
		return -1;
	}

	frame.reset(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });
	int ret = avcodec_receive_frame(codec_context_, frame.get());
	if (ret != 0) {
		//AV_LOG(ret, "receive frame failed.");
		frame.reset();
		return -1;
	}

	return 0;
}

AVCodecContext* Decoder::GetAVCodecContext()
{
	std::lock_guard<std::mutex> locker(mutex_);
	return codec_context_;
}