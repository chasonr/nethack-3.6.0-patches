// sdl2map.h

#ifndef SDL2MAP_H
#define SDL2MAP_H

#include <list>
#include "sdl2.h"
#include "unicode.h"
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2MapWindow : public SDL2Window
{
public:
    SDL2MapWindow(SDL2Interface *interface);
    ~SDL2MapWindow(void);

    virtual void resize(int x1, int y1, int x2, int y2);
    virtual void redraw(void);

    int heightHint(void);

    void clipAround(int x, int y);
    void clipAround(void) { clipAround(m_cursor_x, m_cursor_y); }
    virtual void clear(void);
    virtual void printGlyph(xchar x, xchar y, int glyph);
    virtual void setCursor(int x, int y);

    void toggleTileMode(void);

    // For the position bar
    int mapWidth(void);
    int xPos(void);

    void nextZoomMode(void);

    bool mapMouse(int x_in, int y_in, int *x_out, int *y_out);

    void shieldEffect(int x, int y);

    static const SDL_Color colors[];

private:
    struct Glyph
    {
        // Text display
        utf32_t text_glyph;
        SDL_Color text_fg;
        SDL_Color text_bg;

        // Tile display
        unsigned tile_glyph;

        // Per-tile timed effects. Currently only supports shieldeff.
        enum {
            effect_none,
            effect_shield
        } effect;
        unsigned timer;
    };

    Glyph m_map[ROWNO][COLNO];
    bool m_tile_mode;
    int m_text_w, m_text_h;
    int m_tile_w, m_tile_h;
    int m_scroll_x, m_scroll_y;
    int m_cursor_x, m_cursor_y;

    SDL_Surface *m_tilemap;
    int m_tilemap_w, m_tilemap_h;

    SDL_Surface *m_map_image;

    void mapDraw(unsigned x, unsigned y, bool cursor);
    bool wallDraw(utf32_t ch, const SDL_Rect *rect, SDL_Color color);

    enum ZoomMode {
        SDL2_ZOOMMODE_NORMAL,
        SDL2_ZOOMMODE_FULLSCREEN,
        SDL2_ZOOMMODE_HORIZONTAL,
        SDL2_ZOOMMODE_VERTICAL
    };
    ZoomMode m_zoom_mode;
    void setupMap(void);
};

}

#endif
