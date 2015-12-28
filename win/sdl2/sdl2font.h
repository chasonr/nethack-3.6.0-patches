// SDL2Font.h

#ifndef SDL2FONT_H
#define SDL2FONT_H

#include "sdl2.h"

class SDL2Font
{
public:
    SDL2Font(const char *name, int ptsize);
    ~SDL2Font(void);

    // Font metrics
    int fontAscent(void);
    int fontDescent(void);
    int fontLineSkip(void);
    int fontHeight(void);

    // Text rendering
    // If no background is given, background is transparent
    SDL_Surface *render(utf32_t ch, SDL_Color foreground);
    SDL_Surface *render(const char *text, SDL_Color foreground);
    SDL_Surface *render(utf32_t ch, SDL_Color foreground, SDL_Color background);
    SDL_Surface *render(const char *text, SDL_Color foreground, SDL_Color background);

    // Text extent
    SDL_Rect textSize(utf32_t ch);
    SDL_Rect textSize(const char *text);

    // Default font names
    static const char *defaultMonoFont(void);
    static const char *defaultSerifFont(void);
    static const char *defaultSansFont(void);

private:
    void *m_impl;
};

#endif
