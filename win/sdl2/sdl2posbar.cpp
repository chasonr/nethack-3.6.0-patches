// sdl2posbar.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2posbar.h"
#include "sdl2font.h"
#include "sdl2interface.h"
#include "sdl2map.h"

namespace NH_SDL2
{

#ifdef POSITIONBAR
SDL2PositionBar::SDL2PositionBar(SDL2Interface *interface, SDL2MapWindow *map_window) :
    SDL2Window(interface),
    m_text_w(0), m_text_h(0),
    m_data(NULL),
    m_map_window(map_window)
{
    // Map window font
    setFont(iflags.wc_font_map, iflags.wc_fontsiz_map,
            SDL2Font::defaultMonoFont(), 20);

    SDL_Rect rect = font()->textSize("M");
    m_text_w = rect.w;
    m_text_h = rect.h;
}

SDL2PositionBar::~SDL2PositionBar(void)
{
    delete[] m_data;
}

void SDL2PositionBar::redraw(void)
{
    static const SDL_Color black = {   0,   0,   0, 255 };
    static const SDL_Color brown = {  96,  32,   0, 255 };
    static const SDL_Color white = { 255, 255, 255, 255 };

    if (m_data == NULL) { return; }

    StringContext ctx("SDL2PositionBar::redraw");

    // Position of window with respect to map
    int xpos = -m_map_window->xPos();
    int xsize = m_map_window->mapWidth();
    int xleft, xright;
    SDL_Rect rect;
    if (xpos < 0) {
        // Screen is wider than map
        xleft = 0;
        xright = width();
    } else {
        // Map is at least as wide as screen
        xleft = xpos * width() / xsize;
        xright = (xpos + m_map_window->width()) * width() / xsize;
    }
    rect.y = 0;
    rect.h = height();
    if (xleft > 0) {
        rect.x = 0;
        rect.w = xleft;
        interface()->fill(this, rect, black);
    }
    rect.x = xleft;
    rect.w = xright - xleft;
    interface()->fill(this, rect, brown);
    if (xright < width()) {
        rect.x = xright;
        rect.w = width() - xright;
        interface()->fill(this, rect, black);
    }

    for (unsigned i = 0; m_data[i + 0] != '\0' && m_data[i + 1] != '\0'; i += 2) {
        utf32_t glyph[2];
        glyph[0] = static_cast<unsigned char>(m_data[i + 0]);
        glyph[1] = 0;
        int pos = static_cast<unsigned char>(m_data[i + 1]);
        if (pos < 1 || pos >= COLNO) { continue; }
        int x = pos * width() / COLNO;
        render(uni_32to8(glyph), x, 0, white);
    }
}

int SDL2PositionBar::heightHint(void)
{
    return lineHeight();
}

void SDL2PositionBar::updatePositionBar(const char *data)
{
    delete[] m_data;
    m_data = new char[strlen(data) + 1];
    strcpy(m_data, data);
}
#endif // POSITIONBAR

}
