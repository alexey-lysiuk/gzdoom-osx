
#ifdef NO_SDL

#include "cocoa/hardware.cpp"
#include "cocoa/i_main.cpp"
#include "cocoa/i_system.cpp"

#else // !NO_SDL

#include "sdl/hardware.cpp"
#include "sdl/i_input.cpp"
#include "sdl/i_joystick.cpp"
#include "sdl/i_main.cpp"
#include "sdl/i_system.cpp"
#include "sdl/sdlvideo.cpp"

#endif // NO_SDL

#include "sdl/i_cd.cpp"
#include "sdl/i_movie.cpp"
#include "sdl/st_start.cpp"
