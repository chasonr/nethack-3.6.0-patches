// sdl2window.cpp

extern "C" {
#include "hack.h"
#include "unicode.h"
}
#include "sdl2.h"
#include "sdl2interface.h"
#include "sdl2font.h"
#include "sdl2window.h"

namespace NH_SDL2
{

//////////////////////////////////////////////////////////////////////////////
//                        Base class for all windows                        //
//////////////////////////////////////////////////////////////////////////////

SDL2Window::SDL2Window(SDL2Interface *interface) :
    m_interface(interface),
    m_xmin(0),
    m_xmax(0),
    m_ymin(0),
    m_ymax(0),
    m_visible(true),
    m_font(NULL),
    m_line_height(0)
{
    m_interface->createNotify(this);
}
    
SDL2Window::~SDL2Window(void)
{
    m_interface->closeNotify(this);
    delete m_font;
}

void SDL2Window::resize(int x1, int y1, int x2, int y2)
{
    if (x1 < x2) {
        m_xmin = x1;
        m_xmax = x2;
    } else {
        m_xmin = x2;
        m_xmax = x1;
    }
    if (y1 < y2) {
        m_ymin = y1;
        m_ymax = y2;
    } else {
        m_ymin = y2;
        m_ymax = y1;
    }
    m_interface->resizeNotify(this);
}

void SDL2Window::redraw(void)
{
    // place holder
}

void SDL2Window::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        m_interface->resizeNotify(this);
    }
}

// Draw a hollow rectangle in the given color
void SDL2Window::drawBox(int x1, int y1, int x2, int y2, SDL_Color color)
{
    SDL_Rect rect;

    // Top
    rect.x = x1;
    rect.y = y1;
    rect.w = x2 - x1 + 1;
    rect.h = 1;
    interface()->fill(this, rect, color);

    // Bottom
    rect.y = y2;
    interface()->fill(this, rect, color);

    // Left
    rect.x = x1;
    rect.y = y1;
    rect.w = 1;
    rect.h = y2 - y1 + 1;
    interface()->fill(this, rect, color);

    // Right
    rect.x = x2;
    interface()->fill(this, rect, color);
}

void SDL2Window::startMenu(void)
{
    // Place holder
}

void
SDL2Window::addMenu(int, const anything*, char, char, int, const char *, bool)
{
    // Place holder
}

void SDL2Window::endMenu(const char *)
{
    // Place holder
}

int SDL2Window::selectMenu(int, menu_item **)
{
    // Place holder
    return 0;
}

void SDL2Window::putStr(int, const char *)
{
    // Place holder
}

void SDL2Window::putMixed(int attr, const char *str)
{
    str_context ctx = str_open_context("SDL2Window::putMixed");

    // Unless overridden, this will just convert any glyphs that are present
    // The converted string will never be longer than the input
    char *out = str_alloc(strlen(str));

    const char *p = str;
    char *q = out;
    while (*p != '\0') {
        // Get literal part of string
        const char *end1 = index(p, '\\');
        if (end1 == NULL) {
            strcpy(q, p);
            q += strlen(q);
            break;
        }
        memcpy(q, p, (size_t) (end1 - p));
        q += (size_t) (end1 - p);
        p = end1;
        // *p is a backslash
        if (strlen(p) >= 10 && p[1] == 'G') {
            char rndchk_str[5];
            char *end2;
            memcpy(rndchk_str, p + 2, 4);
            rndchk_str[4] = '\0';
            int rndchk = strtol(rndchk_str, &end2, 16);
            if (rndchk == context.rndencode && *end2 == '\0') {
                char gv_str[5];
                memcpy(gv_str, p + 6, 4);
                gv_str[4] = '\0';
                int gv = strtol(gv_str, &end2, 16);
                if (*end2 == '\0') {
                    nhsym ch;
                    int oc;
                    unsigned os;
                    utf32_t ch32[2];
                    mapglyph(gv, &ch, &oc, &os, 0, 0);
                    ch32[0] = chrConvert(ch);
                    ch32[1] = 0;
                    strcpy(q, uni_32to8(ch32));
                    q += strlen(q);
                    p += 10;
                    continue;
                }
            }
        }
        if (p[1] == '\\') {
            ++p;
        }
        *(q++) = '\\';
        ++p;
    }
    *q = '\0';
    putStr(attr, out);
    str_close_context(ctx);
}

void SDL2Window::clear(void)
{
    // Place holder
}

void SDL2Window::printGlyph(xchar, xchar, int, int)
{
    // Place holder
}

void SDL2Window::setCursor(int, int)
{
    // Place holder
}

void SDL2Window::setFont(
        const char *nameOption, int sizeOption,
        const char *defaultName, int defaultSize)
{
    const char *name;
    int size;

    name = (nameOption != NULL && nameOption[0] != '\0')
            ? nameOption : defaultName;
    size = (sizeOption != 0) ? sizeOption : defaultSize;

    if (m_font) {
        delete m_font;
    }

    m_font = new SDL2Font(name, size);
    m_line_height = (m_font->textSize("X")).h;
}

// Render text at given coordinates without stretching
// Return rectangle in which the text was rendered
SDL_Rect SDL2Window::render(
        const char *str,
        int x, int y,
        SDL_Color fg, SDL_Color bg)
{
    SDL_Surface *text = m_font->render(str, fg, bg);
    SDL_Rect rect = { x, y, text->w, text->h };
    interface()->blit(this, rect, text);
    SDL_FreeSurface(text);
    return rect;
}

SDL_Rect SDL2Window::render(
        const char *str,
        int x, int y,
        SDL_Color fg)
{
    static const SDL_Color clear = { 0, 0, 0, 0 };

    return render(str, x, y, fg, clear);
}

// Convert attribute flags to colors
// Underline is implemented as bold on a gray background
// Blink is not implemented
SDL_Color SDL2Window::textFG(int attr)
{
    static const SDL_Color black = {   0,   0,   0, 255 };
    static const SDL_Color gray  = { 128, 128, 128, 255 };
    static const SDL_Color bgray = { 176, 176, 176, 255 };
    static const SDL_Color white = { 255, 255, 255, 255 };

    switch (attr) {
    default:
    case ATR_NONE:
        return bgray;

    case ATR_BOLD:
    case ATR_ULINE:
        return white;

    case ATR_DIM:
        return gray;

    case ATR_INVERSE:
        return black;
    }
}

SDL_Color SDL2Window::textBG(int attr)
{
    static const SDL_Color clear = {   0,   0,   0,   0 };
    static const SDL_Color gray  = { 128, 128, 128, 255 };
    static const SDL_Color white = { 255, 255, 255, 255 };

    switch (attr) {
    default:
    case ATR_NONE:
    case ATR_BOLD:
    case ATR_DIM:
        return clear;

    case ATR_ULINE:
        return gray;

    case ATR_INVERSE:
        return white;
    }
}

SDL_Color SDL2Window::textColor(uint32_t /*color*/)
{
    // TODO: status and menu colors not implemented
    return textFG(ATR_NONE);
}

}
