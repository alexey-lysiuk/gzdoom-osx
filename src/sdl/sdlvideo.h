
#ifndef SRC_SDL_SDLVIDEO_H_INCLUDED
#define SRC_SDL_SDLVIDEO_H_INCLUDED


#include "hardware.h"
#include "v_video.h"

class SDLVideo : public IVideo
{
 public:
	SDLVideo (int parm);
	~SDLVideo ();

	EDisplayType GetDisplayType () { return DISPLAY_Both; }
	void SetWindowedScale (float scale);

	DFrameBuffer *CreateFrameBuffer (int width, int height, bool fs, DFrameBuffer *old);

	void StartModeIterator (int bits, bool fs);
	bool NextMode (int *width, int *height, bool *letterbox);

private:
	int IteratorMode;
	int IteratorBits;
	bool IteratorFS;
};


#endif // SRC_SDL_SDLVIDEO_H_INCLUDED
