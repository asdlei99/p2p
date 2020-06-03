#ifndef FFMPEG_RENDERER_H
#define FFMPEG_RENDERER_H

#include "SDL.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#include <string>
#include <cstdint>

namespace ffmpeg {
	
class Renderer
{
public:
	Renderer();
	virtual ~Renderer();

	virtual bool Init(SDL_Window* window, int width, int height);
	virtual void Destroy();
	virtual int Resize();
	virtual int Render(AVFrame* frame);

protected:
	SDL_Window* window_ ;
	SDL_Renderer* sdl_renderer_;
	SDL_Texture*  sdl_texture_;
	int sdl_flags_ = 0;
	int sdl_format_ = 0;
	std::string renderer_name_;

	uint32_t video_width_;
	uint32_t video_height_;
};

}

#endif