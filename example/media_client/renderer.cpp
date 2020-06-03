#include "Renderer.h"
#include "log.h"
#include <algorithm>

using namespace ffmpeg;

Renderer::Renderer()
	: window_(nullptr)
	, sdl_renderer_(nullptr)
	, sdl_texture_(nullptr)
{

}

Renderer::~Renderer()
{
	Destroy();
}

bool Renderer::Init(SDL_Window* window, int width, int height)
{
	if (sdl_renderer_ != NULL) {
		return false;
	}

	window_ = window;
	video_width_ = width;
	video_height_ = height;
	sdl_flags_ = SDL_RENDERER_SOFTWARE;

	int index = -1;
	int num_drivers = SDL_GetNumRenderDrivers();
	
	for (int i = 0; i < num_drivers; i++) {
		SDL_RendererInfo renderer_info;
		if (SDL_GetRenderDriverInfo(i, &renderer_info) < 0) {
			continue;
		}

#ifdef _WIN32
		if (strcmp(renderer_info.name, "direct3d") == 0) {
			renderer_name_ = "direct3d";
			index = i;
			if (renderer_info.flags & SDL_RENDERER_ACCELERATED) {
				sdl_flags_ = SDL_RENDERER_ACCELERATED;
			}
		}
#else
		if (strstr(renderer_info.name, "opengl") != NULL) {
			renderer_name_ = "opengl";
			index = i;
		}
#endif	
	}

	sdl_renderer_ = SDL_CreateRenderer(window, index, sdl_flags_);
	if (sdl_renderer_ == nullptr) {
		LOG("create renderer failed.");
		return false;
	}

	SDL_SetRenderDrawColor(sdl_renderer_, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(sdl_renderer_);
	SDL_RenderPresent(sdl_renderer_);
	return true;
}

void Renderer::Destroy()
{
	if (sdl_texture_ != nullptr) {
		SDL_DestroyTexture(sdl_texture_);
		sdl_texture_ = nullptr;
	}

	if (sdl_renderer_ != NULL) {
		SDL_DestroyRenderer(sdl_renderer_);
		sdl_renderer_ = nullptr;
	}
}

int Renderer::Resize()
{
	if (sdl_texture_ != nullptr) {
		//SDL_SetRenderDrawColor(sdl_renderer_, 0, 0, 0, SDL_ALPHA_OPAQUE);
		//SDL_RenderClear(sdl_renderer_);
		//SDL_RenderPresent(sdl_renderer_);

		if (sdl_texture_ != nullptr) {
			SDL_DestroyTexture(sdl_texture_);
			sdl_texture_ = nullptr;
		}
	}

	return 0;
}

#include "libyuv.h"
#include "Timestamp.h"

int Renderer::Render(AVFrame* frame)
{
	//return 0;

	static Timestamp tp, tp2;
	static int fps = 0;
	if (tp.elapsed() >= 1000) {
		printf("renderer fps: %d\n", fps);
		fps = 0;
		tp.reset();
	}

	fps++;

	tp2.reset();

	if (sdl_renderer_ == nullptr) { 
		return -1;
	}

	if (sdl_texture_ == nullptr) {
		switch (frame->format)
		{
		case AV_PIX_FMT_YUV420P:
			sdl_format_ = SDL_PIXELFORMAT_YV12;
			break;
		case AV_PIX_FMT_NV12:
			sdl_format_ = SDL_PIXELFORMAT_NV12;
			break;
		case AV_PIX_FMT_NV21:
			sdl_format_ = SDL_PIXELFORMAT_NV21;
			break;
		case AV_PIX_FMT_BGRA:
			sdl_format_ = SDL_PIXELFORMAT_BGRA32;
			break;
		default:
			LOG("format(%d) unsupported.", (int)frame->format);
			return -1;
		}

		sdl_texture_ = SDL_CreateTexture(sdl_renderer_, sdl_format_, SDL_TEXTUREACCESS_STREAMING, video_width_, video_height_);
		if (sdl_texture_ == nullptr) {
			LOG("create texture failed.");
			return -1;
		}
	}

	if (sdl_texture_ == nullptr) {
		return -1;
	}

	if (frame->format == AV_PIX_FMT_YUV420P) {
		SDL_UpdateYUVTexture(sdl_texture_, nullptr,
			frame->data[0],
			frame->linesize[0],
			frame->data[1],
			frame->linesize[1],
			frame->data[2],
			frame->linesize[2]);
	}
	else if (frame->format == AV_PIX_FMT_BGRA) {
		char* pixels = NULL;
		int pitch;
		int ret = SDL_LockTexture(sdl_texture_, nullptr, (void**)&pixels, &pitch);
		if (ret < 0) {
			LOG("lock texture failed.");
			return -1;
		}
		memcpy(pixels, frame->data[0], frame->width * frame->height * 4);
		SDL_UnlockTexture(sdl_texture_);
	}
	else {
		char* pixels = NULL;
		int pitch;
		int ret = SDL_LockTexture(sdl_texture_, nullptr, (void**)&pixels, &pitch);
		if (ret < 0) {
			LOG("lock texture failed.");
			return -1;
		}
		memcpy(pixels, frame->data[0], frame->linesize[0] * frame->height);
		memcpy(pixels + (frame->linesize[0] * frame->height), frame->data[1], frame->linesize[1] * frame->height / 2);
		SDL_UnlockTexture(sdl_texture_);
	}

	SDL_RenderClear(sdl_renderer_);

	SDL_Rect rect = { 0, 0, 0, 0 };
	SDL_GetWindowSize(window_, &rect.w, &rect.h);
	SDL_RenderCopy(sdl_renderer_, sdl_texture_, nullptr, &rect);
	SDL_RenderPresent(sdl_renderer_);	
	return 0;
}

