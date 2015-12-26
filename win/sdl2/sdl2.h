// sdl2.h -- to include the SDL headers, accounting for different
// install locations

#ifndef WIN_SDL2_SDL2_H
#define WIN_SDL2_SDL2_H

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#elif defined(_WIN32)
#include <SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#endif
