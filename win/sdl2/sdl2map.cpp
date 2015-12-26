// sdl2map.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2.h"
#include "sdl2map.h"
#include "sdl2interface.h"
#include "sdl2font.h"

extern short glyph2tile[];

namespace NH_SDL2
{

SDL2MapWindow::SDL2MapWindow(SDL2Interface *interface) :
    SDL2Window(interface),
    m_tile_mode(iflags.wc_tiled_map),
    m_text_w(0), m_text_h(0),
    m_tile_w(0), m_tile_h(0),
    m_scroll_x(0), m_scroll_y(0),
    m_cursor_x(0), m_cursor_y(0),
    m_tilemap(NULL),
    m_tilemap_w(iflags.wc_tile_width), m_tilemap_h(iflags.wc_tile_height),
    m_map_image(NULL),
    m_zoom_mode(SDL2_ZOOMMODE_NORMAL)
{
    // Map window font
    setFont(iflags.wc_font_map, iflags.wc_fontsiz_map,
            SDL2Font::defaultMonoFont(), 20);

    SDL_Rect rect = font()->textSize("M");
    m_text_w = rect.w;
    m_text_h = rect.h;

    // Tile map file
    const char *tilemap = iflags.wc_tile_file;
    if (tilemap == NULL || tilemap[0] == '\0') {
        tilemap = "nhtiles.bmp";
    }
    SDL_Surface *tilemap_img = IMG_Load(tilemap);
    if (tilemap_img != NULL) {
        // We need to convert the tile map if it uses a palette;
        // it doesn't hurt to convert the tile map anyway
        m_tilemap = SDL_ConvertSurface(tilemap_img, interface->pixelFormat(), 0);
        SDL_FreeSurface(tilemap_img);
    }
    if (m_tilemap == NULL) {
        // Failed to load the tile map; fall back to text mode
        m_tile_mode = false;
    } else {
        // Tile size
        if (m_tilemap_w == 0) { m_tilemap_w = m_tilemap->w / 40; }
        if (m_tilemap_h == 0) { m_tilemap_h = m_tilemap_w; }
    }

    // Cells are initially unstretched
    m_tile_w = m_tile_mode ? m_tilemap_w : m_text_w;
    m_tile_h = m_tile_mode ? m_tilemap_h : m_text_h;

    m_map_image = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            m_tile_w * COLNO,
            m_tile_h * ROWNO,
            32,
            0x000000FF,  // red
            0x0000FF00,  // green
            0x00FF0000,  // blue
            0xFF000000); // alpha

    clear();
}

SDL2MapWindow::~SDL2MapWindow(void)
{
    if (m_tilemap) { SDL_FreeSurface(m_tilemap); }
    if (m_map_image) { SDL_FreeSurface(m_map_image); }
}

void SDL2MapWindow::resize(int x1, int y1, int x2, int y2)
{
    SDL2Window::resize(x1, y1, x2, y2);
    setupMap();
}

void SDL2MapWindow::redraw(void)
{
    static const SDL_Color background = {  96,  32,   0, 255 };
    SDL_Rect srect, drect;

    // Draw the map image
    if (m_scroll_x < 0) {
        srect.x = -m_scroll_x;
        drect.x = 0;
        srect.w = m_map_image->w + m_scroll_x;
    } else {
        srect.x = 0;
        drect.x = m_scroll_x;
        srect.w = m_map_image->w;
    }
    if (srect.w > width()) {
        srect.w = width();
    }
    drect.w = srect.w;
    if (m_scroll_y < 0) {
        srect.y = -m_scroll_y;
        drect.y = 0;
        srect.h = m_map_image->h + m_scroll_y;
    } else {
        srect.y = 0;
        drect.y = m_scroll_y;
        srect.h = m_map_image->h;
    }
    if (srect.h > height()) {
        srect.h = height();
    }
    drect.h = srect.h;
    interface()->blit(this, drect, m_map_image, srect);

    // Erase above the map image
    drect.x = 0;
    drect.y = 0;
    drect.w = width();
    drect.h = m_scroll_y;
    if (drect.h > 0) {
        interface()->fill(this, drect, background);
    }

    // Erase below the map image
    drect.y = m_scroll_y + m_map_image->h;
    drect.h = height() - drect.y;
    if (drect.h > 0) {
        interface()->fill(this, drect, background);
    }

    // Erase left of the map image
    drect.x = 0;
    drect.y = m_scroll_y;
    drect.w = m_scroll_x;
    drect.h = m_map_image->h;
    if (drect.w > 0) {
        interface()->fill(this, drect, background);
    }

    // Erase right of the map image
    drect.x = m_scroll_x + m_map_image->w;
    drect.w = width() - drect.x;
    if (drect.w > 0) {
        interface()->fill(this, drect, background);
    }
}

void SDL2MapWindow::clear(void)
{
    int background_glyph = cmap_to_glyph(S_room);

    for (unsigned y = 0; y < ROWNO; ++y) {
        for (unsigned x = 0; x < COLNO; ++x) {
            m_map[y][x].text_glyph = ' ';
            m_map[y][x].text_fg.r = 176;
            m_map[y][x].text_fg.g = 176;
            m_map[y][x].text_fg.b = 176;
            m_map[y][x].text_fg.a = 255;
            m_map[y][x].text_bg.r =   0;
            m_map[y][x].text_bg.g =   0;
            m_map[y][x].text_bg.b =   0;
            m_map[y][x].text_bg.a = 255;
            m_map[y][x].tile_glyph = background_glyph;
        }
    }

    // This is faster than calling mapDraw on each cell
    mapDraw(0, 0, false);
    SDL_Rect rect1, rect2;
    rect1.x = 0;
    rect1.y = 0;
    rect1.w = m_tile_w;
    rect1.h = m_tile_h;
    rect2.w = rect1.w;
    rect2.h = rect1.h;
    for (unsigned y = 0; y < ROWNO; ++y) {
        rect2.y = y * rect2.h;
        for (unsigned x = 0; x < COLNO; ++x) {
            rect2.x = x * rect2.w;
            SDL_BlitSurface(m_map_image, &rect1, m_map_image, &rect2);
        }
    }

    if (isVisible()) { redraw(); }
}

void SDL2MapWindow::printGlyph(xchar x, xchar y, int glyph)
{
    static const SDL_Color black = { 0, 0, 0, 255 };
    static const SDL_Color colors[] =
    {
        {  96,  96,  96, 255 }, // "black" is actually dark gray (e.g., black dragon)
        { 176,   0,   0, 255 }, // red
        {   0, 191,   0, 255 }, // green
        { 127, 127,   0, 255 }, // brown
        {   0,   0, 176, 255 }, // blue
        { 176,   0, 176, 255 }, // magenta
        {   0, 176, 176, 255 }, // cyan
        { 176, 176, 176, 255 }, // gray
        { 255, 255, 255, 255 }, // no color (Rogue level)
        { 255, 127,   0, 255 }, // orange
        { 127, 255, 127, 255 }, // bright green
        { 255, 255,   0, 255 }, // yellow
        { 127, 127, 255, 255 }, // bright blue
        { 255, 127, 255, 255 }, // bright magenta
        { 127, 255, 255, 255 }, // bright cyan
        { 255, 255, 255, 255 }  // white
    };

    if (x < 0 || COLNO <= x || y < 0 || ROWNO <= y) { return; }

    nhsym ochar;
    int ocolor;
    unsigned ospecial;

    mapglyph(glyph, &ochar, &ocolor, &ospecial, x, y);

    if (ocolor < 0 || ocolor > SIZE(colors)) { ocolor = 0; }
    m_map[y][x].text_glyph = ochar;
    m_map[y][x].text_fg = colors[ocolor];
    m_map[y][x].text_bg = black;
    m_map[y][x].tile_glyph = glyph2tile[glyph];

    mapDraw(x, y, true);
}

void SDL2MapWindow::setCursor(int x, int y)
{
    mapDraw(m_cursor_x, m_cursor_y, false);

    if (0 <= x && x < COLNO) {
        m_cursor_x = x;
    }
    if (0 <= y && y < ROWNO) {
        m_cursor_y = y;
    }

    mapDraw(m_cursor_x, m_cursor_y, true);
}

void SDL2MapWindow::clipAround(int x, int y)
{
    if (m_map_image->w <= width()) {
        // The map fits horizontally within the window
        m_scroll_x = (width() - m_map_image->w) / 2;
        // m_scroll_x is at least zero
    } else {
        int cell_w = m_tile_w;

        // Initially place x at the center of the window
        int cell_x = x * cell_w;                // x in map
        int seen_x = (width() - cell_w) / 2;    // x in window
        m_scroll_x = seen_x - cell_x;

        // Adjust so that maximum width is visible
        if (m_scroll_x > 0) { m_scroll_x = 0; }
        int min_x = width() - m_map_image->w;
        if (m_scroll_x < min_x) { m_scroll_x = min_x; }
        // m_scroll_x is at most zero
    }

    if (m_map_image->h <= height()) {
        // The map fits vertically within the window
        m_scroll_y = (height() - m_map_image->h) / 2;
        // m_scroll_y is at least zero
    } else {
        int cell_h = m_tile_h;

        // Initially place y at the center of the window
        int cell_y = y * cell_h;                // y in map
        int seen_y = (height() - cell_h) / 2;   // y in window
        m_scroll_y = seen_y - cell_y;

        // Adjust so that maximum height is visible
        if (m_scroll_y > 0) { m_scroll_y = 0; }
        int min_y = height() - m_map_image->h;
        if (m_scroll_y < min_y) { m_scroll_y = min_y; }
        // m_scroll_y is at most zero
    }
}

void SDL2MapWindow::toggleTileMode(void)
{
    // Disable tile mode if no tile map
    if (m_tilemap == NULL) {
        interface()->setMessage("No tile set available");
        return;
    }

    m_tile_mode = !m_tile_mode;
    interface()->setMessage(
            m_tile_mode ? "Display Mode: Tiles"
                        : "Display Mode: ASCII");

    // Map image size may have changed
    setupMap();
}

void SDL2MapWindow::nextZoomMode(void)
{
    switch (m_zoom_mode) {
    default:
    case SDL2_ZOOMMODE_VERTICAL:
        m_zoom_mode = SDL2_ZOOMMODE_NORMAL;
        interface()->setMessage("Zoom Mode: Tileset 1:1");
        break;

    case SDL2_ZOOMMODE_NORMAL:
        m_zoom_mode = SDL2_ZOOMMODE_FULLSCREEN;
        interface()->setMessage("Zoom Mode: Scale to screen");
        break;

    case SDL2_ZOOMMODE_FULLSCREEN:
        m_zoom_mode = SDL2_ZOOMMODE_HORIZONTAL;
        interface()->setMessage("Zoom Mode: Fit horizontally");
        break;

    case SDL2_ZOOMMODE_HORIZONTAL:
        m_zoom_mode = SDL2_ZOOMMODE_VERTICAL;
        interface()->setMessage("Zoom Mode: Fit vertically");
        break;
    }

    setupMap();
}

void SDL2MapWindow::setupMap(void)
{
    int cell_w = m_tile_mode ? m_tilemap_w : m_text_w;
    int cell_h = m_tile_mode ? m_tilemap_h : m_text_h;

    switch (m_zoom_mode) {
    default:
    case SDL2_ZOOMMODE_NORMAL:
        m_tile_w = cell_w;
        m_tile_h = cell_h;
        break;

    case SDL2_ZOOMMODE_FULLSCREEN:
        m_tile_w = width() / COLNO;
        m_tile_h = height() / ROWNO;
        break;

    case SDL2_ZOOMMODE_HORIZONTAL:
        m_tile_w = width() / COLNO;
        m_tile_h = m_tile_w * cell_h / cell_w;
        break;

    case SDL2_ZOOMMODE_VERTICAL:
        m_tile_h = height() / ROWNO;
        m_tile_w = m_tile_h * cell_w / cell_h;
        break;
    }

    int width = m_tile_w * COLNO;
    int height = m_tile_h * ROWNO;
    if (width != m_map_image->w || height != m_map_image->h) {
        SDL_FreeSurface(m_map_image);
        m_map_image = SDL_CreateRGBSurface(
                SDL_SWSURFACE,
                m_tile_w * COLNO,
                m_tile_h * ROWNO,
                32,
                0x000000FF,  // red
                0x0000FF00,  // green
                0x00FF0000,  // blue
                0xFF000000); // alpha
    }

    // Redraw the entire map
    for (unsigned y = 0; y < ROWNO; ++y) {
        for (unsigned x = 0; x < COLNO; ++x) {
            mapDraw(x, y, true);
        }
    }

    clipAround(m_cursor_x, m_cursor_y);
}

void SDL2MapWindow::mapDraw(unsigned x, unsigned y, bool cursor)
{
    SDL_Rect tile, cell;
    cell.x = x * m_tile_w;
    cell.y = y * m_tile_h;
    cell.w = m_tile_w;
    cell.h = m_tile_h;
    if (m_tile_mode) {
        int tile_cols = m_tilemap->w / m_tilemap_w;
        unsigned tx = m_map[y][x].tile_glyph % tile_cols;
        unsigned ty = m_map[y][x].tile_glyph / tile_cols;
        tile.x = tx * m_tilemap_w;
        tile.y = ty * m_tilemap_h;
        tile.w = m_tilemap_w;
        tile.h = m_tilemap_h;
        Uint32 bg = SDL_MapRGBA(m_map_image->format, 0, 0, 0, 255);
        SDL_FillRect(m_map_image, &cell, bg);
        SDL_BlitScaled(m_tilemap, &tile, m_map_image, &cell);
    } else {
        Uint32 bg = SDL_MapRGBA(m_map_image->format,
                m_map[y][x].text_bg.r,
                m_map[y][x].text_bg.g,
                m_map[y][x].text_bg.b,
                m_map[y][x].text_bg.a);
        SDL_FillRect(m_map_image, &cell, bg);
        if (!wallDraw(m_map[y][x].text_glyph, &cell,
                m_map[y][x].text_fg)) {
            SDL_Surface *text = font()->render(
                    m_map[y][x].text_glyph,
                    m_map[y][x].text_fg);
            tile.x = 0;
            tile.y = 0;
            tile.w = text->w;
            tile.h = text->h;
            SDL_BlitScaled(text, &tile, m_map_image, &cell);
            SDL_FreeSurface(text);
        }
    }

    if (cursor && x == m_cursor_x && y == m_cursor_y) {
        SDL_Rect line;
        // TODO: This is the cursor. Make this the same color as hit points.
        SDL_Color rgb = { 255, 255, 255, 255 };
        Uint32 color = SDL_MapRGBA(m_map_image->format, rgb.r, rgb.g, rgb.b, rgb.a);

        // top
        line.x = cell.x;
        line.y = cell.y;
        line.w = cell.w;
        line.h = 1;
        SDL_FillRect(m_map_image, &line, color);
        // bottom
        line.y = cell.y + cell.h - 1;
        SDL_FillRect(m_map_image, &line, color);
        // left
        line.x = cell.x;
        line.y = cell.y;
        line.w = 1;
        line.h = cell.h;
        SDL_FillRect(m_map_image, &line, color);
        // right
        line.x = cell.x + cell.w - 1;
        SDL_FillRect(m_map_image, &line, color);
    }
}

bool SDL2MapWindow::wallDraw(char32_t ch, const SDL_Rect *rect, SDL_Color color)
{
    enum
    {
        w_left      = 0x01,
        w_right     = 0x02,
        w_up        = 0x04,
        w_down      = 0x08,
        w_sq_top    = 0x10,
        w_sq_bottom = 0x20,
        w_sq_left   = 0x40,
        w_sq_right  = 0x80
    };
    unsigned walls;
    SDL_Rect linerect;
    unsigned linewidth;
    Uint32 c = SDL_MapRGBA(m_map_image->format,
            color.r, color.g, color.b, color.a);

    linewidth = ((rect->w < rect->h) ? rect->w : rect->h)/8;
    if (linewidth == 0) linewidth = 1;

    // Single walls
    walls = 0;
    switch (ch)
    {
    case 0x2500: // BOX DRAWINGS LIGHT HORIZONTAL
        walls = w_left | w_right;
        break;

    case 0x2502: // BOX DRAWINGS LIGHT VERTICAL
        walls = w_up | w_down;
        break;

    case 0x250C: // BOX DRAWINGS LIGHT DOWN AND RIGHT
        walls = w_down | w_right;
        break;

    case 0x2510: // BOX DRAWINGS LIGHT DOWN AND LEFT
        walls = w_down | w_left;
        break;

    case 0x2514: // BOX DRAWINGS LIGHT UP AND RIGHT
        walls = w_up | w_right;
        break;

    case 0x2518: // BOX DRAWINGS LIGHT UP AND LEFT
        walls = w_up | w_left;
        break;

    case 0x251C: // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
        walls = w_up | w_down | w_right;
        break;

    case 0x2524: // BOX DRAWINGS LIGHT VERTICAL AND LEFT
        walls = w_up | w_down | w_left;
        break;

    case 0x252C: // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
        walls = w_down | w_left | w_right;
        break;

    case 0x2534: // BOX DRAWINGS LIGHT UP AND HORIZONTAL
        walls = w_up | w_left | w_right;
        break;

    case 0x253C: // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
        walls = w_up | w_down | w_left | w_right;
        break;
    }

    if (walls != 0)
    {
        linerect.x = rect->x + (rect->w - linewidth)/2;
        linerect.w = linewidth;
        switch (walls & (w_up | w_down))
        {
        case w_up:
            linerect.y = rect->y;
            linerect.h = rect->h/2;
            SDL_FillRect(m_map_image, &linerect, c);
            break;

        case w_down:
            linerect.y = rect->y + rect->h/2;
            linerect.h = rect->h - rect->h/2;
            SDL_FillRect(m_map_image, &linerect, c);
            break;

        case w_up | w_down:
            linerect.y = rect->y;
            linerect.h = rect->h;
            SDL_FillRect(m_map_image, &linerect, c);
            break;
        }

        linerect.y = rect->y + (rect->h - linewidth)/2;
        linerect.h = linewidth;
        switch (walls & (w_left | w_right))
        {
        case w_left:
            linerect.x = rect->x;
            linerect.w = rect->w/2;
            SDL_FillRect(m_map_image, &linerect, c);
            break;

        case w_right:
            linerect.x = rect->x + rect->w/2;
            linerect.w = rect->w - rect->w/2;
            SDL_FillRect(m_map_image, &linerect, c);
            break;

        case w_left | w_right:
            linerect.x = rect->x;
            linerect.w = rect->w;
            SDL_FillRect(m_map_image, &linerect, c);
            break;
        }

        return true;
    }

    // Double walls
    walls = 0;
    switch (ch)
    {
    case 0x2550: // BOX DRAWINGS DOUBLE HORIZONTAL
        walls = w_left | w_right | w_sq_top | w_sq_bottom;
        break;

    case 0x2551: // BOX DRAWINGS DOUBLE VERTICAL
        walls = w_up | w_down | w_sq_left | w_sq_right;
        break;

    case 0x2554: // BOX DRAWINGS DOUBLE DOWN AND RIGHT
        walls = w_down | w_right | w_sq_top | w_sq_left;
        break;

    case 0x2557: // BOX DRAWINGS DOUBLE DOWN AND LEFT
        walls = w_down | w_left | w_sq_top | w_sq_right;
        break;

    case 0x255A: // BOX DRAWINGS DOUBLE UP AND RIGHT
        walls = w_up | w_right | w_sq_bottom | w_sq_left;
        break;

    case 0x255D: // BOX DRAWINGS DOUBLE UP AND LEFT
        walls = w_up | w_left | w_sq_bottom | w_sq_right;
        break;

    case 0x2560: // BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
        walls = w_up | w_down | w_right | w_sq_left;
        break;

    case 0x2563: // BOX DRAWINGS DOUBLE VERTICAL AND LEFT
        walls = w_up | w_down | w_left | w_sq_right;
        break;

    case 0x2566: // BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
        walls = w_down | w_left | w_right | w_sq_top;
        break;

    case 0x2569: // BOX DRAWINGS DOUBLE UP AND HORIZONTAL
        walls = w_up | w_left | w_right | w_sq_bottom;
        break;

    case 0x256C: // BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
        walls = w_up | w_down | w_left | w_right;
        break;
    }
    if (walls != 0)
    {
        if (walls & w_up)
        {
            linerect.x = rect->x + rect->w/2 - linewidth*2;
            linerect.w = linewidth;
            linerect.y = rect->y;
            linerect.h = rect->h/2 - linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
            linerect.x += linewidth*3;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_down)
        {
            linerect.x = rect->x + rect->w/2 - linewidth*2;
            linerect.w = linewidth;
            linerect.y = rect->y + rect->h/2 + linewidth;
            linerect.h = rect->h - rect->h/2 - linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
            linerect.x += linewidth*3;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_left)
        {
            linerect.x = rect->x;
            linerect.w = rect->w/2 - linewidth;
            linerect.y = rect->y + rect->h/2 - linewidth*2;
            linerect.h = linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
            linerect.y += linewidth*3;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_right)
        {
            linerect.x = rect->x + rect->w/2 + linewidth;
            linerect.w = rect->w - rect->w/2 - linewidth;
            linerect.y = rect->y + rect->h/2 - linewidth*2;
            linerect.h = linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
            linerect.y += linewidth*3;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_sq_top)
        {
            linerect.x = rect->x + rect->w/2 - linewidth*2;
            linerect.w = linewidth*4;
            linerect.y = rect->y + rect->h/2 - linewidth*2;
            linerect.h = linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_sq_bottom)
        {
            linerect.x = rect->x + rect->w/2 - linewidth*2;
            linerect.w = linewidth*4;
            linerect.y = rect->y + rect->h/2 + linewidth;
            linerect.h = linewidth;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_sq_left)
        {
            linerect.x = rect->x + rect->w/2 - linewidth*2;
            linerect.w = linewidth;
            linerect.y = rect->y + rect->h/2 - linewidth*2;
            linerect.h = linewidth*4;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        if (walls & w_sq_right)
        {
            linerect.x = rect->x + rect->w/2 + linewidth;
            linerect.w = linewidth;
            linerect.y = rect->y + rect->h/2 - linewidth*2;
            linerect.h = linewidth*4;
            SDL_FillRect(m_map_image, &linerect, c);
        }
        return true;
    }

    // Solid blocks
    if (0x2591 <= ch && ch <= 0x2593)
    {
        unsigned shade = ch - 0x2590;
        SDL_Color color2;
        color2.r = color.r*shade/4;
        color2.g = color.g*shade/4;
        color2.b = color.b*shade/4;
        color2.a = color.a;
        linerect = *rect;
        Uint32 c2 = SDL_MapRGBA(m_map_image->format,
                color2.r, color2.g, color2.b, color2.a);
        SDL_FillRect(m_map_image, &linerect, c2);
        return true;
    }

    return false;
}

int SDL2MapWindow::heightHint(void)
{
    return (m_tile_mode ? m_tilemap_h : m_text_h) * ROWNO;
}

int SDL2MapWindow::mapWidth(void)
{
    return m_tile_w * COLNO;
}

int SDL2MapWindow::xPos(void)
{
    return m_scroll_x;
}


bool SDL2MapWindow::mapMouse(int x_in, int y_in, int *x_out, int *y_out)
{
    x_in -= m_scroll_x;
    y_in -= m_scroll_y;
    if (x_in < 0 || y_in < 0) { return false; }
    *x_out = x_in / m_tile_w;
    *y_out = y_in / m_tile_h;
    return (*x_out < COLNO && *y_out < ROWNO);
}

}
