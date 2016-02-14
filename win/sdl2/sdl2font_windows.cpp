//  sdl2font_windows.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
extern "C" {
#include "hack.h"
}
#include "unicode.h"
#include "sdl2font.h"

struct FontImpl
{
    HFONT hfont;
    HDC memory_dc;
};

namespace {

wchar_t *
singleChar(utf32_t ch, wchar_t str16[3])
{
    if (ch > 0x10FFFF || (ch & 0xFFFFF800) == 0xD800) {
        ch = 0xFFFD;
    }

    if (ch < 0x10000) {
        str16 = (wchar_t *) str_mem_alloc(2 * sizeof(wchar_t));
        str16[0] = (wchar_t) ch;
        str16[1] = 0;
    } else {
        str16 = (wchar_t *) str_mem_alloc(3 * sizeof(wchar_t));
        str16[0] = 0xD7C0 + (ch >> 10);
        str16[1] = 0xDC00 + (ch & 0x3FF);
        str16[2] = 0;
    }
    return str16;
}


/* UTF-8 to UTF-16 conversion; but don't depend on wchar_t being the same type
   as utf16_t */
wchar_t *
win32String(const char *str)
{
    str_context ctx = str_open_context("win32String");

    utf16_t *str16;
    wchar_t *wstr16;
    size_t size16, i;

    str16 = uni_8to16(str);
    size16 = uni_length16(str16);
    wstr16 = (wchar_t *) str_mem_alloc((size16 + 1) * sizeof(wstr16[0]));
    for (i = 0; str16[i] != 0; ++i) {
        wstr16[i] = str16[i];
    }
    wstr16[i] = 0;
    str_export(ctx, wstr16);
    str_close_context(ctx);
    return wstr16;
}

}

static SDL_Surface *renderImpl(
        void *m_impl,
        const wchar_t *text,
        SDL_Color foreground,
        SDL_Color background);
static SDL_Rect textSizeImpl(void *m_impl, const wchar_t *text);

SDL2Font::SDL2Font(const char *name, int ptsize) : m_impl(NULL)
{
    FontImpl *font = new FontImpl;
    font->hfont = NULL;
    font->memory_dc = NULL;
    m_impl = font;

    int height;

    font->memory_dc = CreateCompatibleDC(NULL);
    if (font->memory_dc == NULL) {
        throw std::bad_alloc();
    }
    //RLC The constant 72 is specified in Microsoft's documentation for
    // CreateFont; but 97 comes closer to the behavior of the original
    // SDL_ttf library
    height = -MulDiv(ptsize, GetDeviceCaps(font->memory_dc, LOGPIXELSY), 97);

    font->hfont = CreateFontA(
            height,
            0,
            0,
            0,
            FW_DONTCARE,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            name);
    if (font->hfont == NULL) {
        throw std::bad_alloc();
    }

    SelectObject(font->memory_dc, font->hfont);
}

SDL2Font::~SDL2Font(void)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);

    if (font->hfont != NULL) {
        DeleteObject(font->hfont);
    }
    if (font->memory_dc != NULL) {
        DeleteDC(font->memory_dc);
    }

    delete font;
}

// Font metrics
int SDL2Font::fontAscent(void)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    TEXTMETRIC metrics;

    GetTextMetrics(font->memory_dc, &metrics);
    return metrics.tmAscent;
}

int SDL2Font::fontDescent(void)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    TEXTMETRIC metrics;

    GetTextMetrics(font->memory_dc, &metrics);
    return metrics.tmDescent;
}

int SDL2Font::fontLineSkip(void)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    TEXTMETRIC metrics;

    GetTextMetrics(font->memory_dc, &metrics);
    return metrics.tmHeight + metrics.tmExternalLeading;
}

int SDL2Font::fontHeight(void)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    TEXTMETRIC metrics;

    GetTextMetrics(font->memory_dc, &metrics);
    return metrics.tmHeight;
}

static const SDL_Color transparent = { 0, 0, 0, 0 };

// Text rendering
// If no background is given, background is transparent
SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground)
{
    return render(ch, foreground, transparent);
}

SDL_Surface *SDL2Font::render(const char *text, SDL_Color foreground)
{
    return render(text, foreground, transparent);
}

SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground, SDL_Color background)
{
    wchar_t wch[3];

    return renderImpl(m_impl, singleChar(ch, wch), foreground, background);
}

SDL_Surface *SDL2Font::render(const char *text, SDL_Color foreground, SDL_Color background)
{
    str_context ctx = str_open_context("SDL2Font::render");
    SDL_Surface *surface;

    surface = renderImpl(m_impl, win32String(text), foreground, background);
    str_close_context(ctx);
    return surface;
}

static SDL_Surface *renderImpl(
        void *m_impl,
        const wchar_t *text,
        SDL_Color foreground,
        SDL_Color background)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    HFONT hfont = font->hfont;
    HDC memory_dc = NULL; // custodial
    HBITMAP hbitmap = NULL; // custodial
    SIZE size;
    BITMAPINFO bi;
    void *bits;
    SDL_Surface *surface = NULL; // custodial, returned
    int x, y;
    uint32_t *row1, *row2;

    // We'll render to this device context
    memory_dc = CreateCompatibleDC(NULL);
    if (memory_dc == NULL) goto error;
    if (!SelectObject(memory_dc, (HGDIOBJ)hfont)) goto error;

    // We'll do alpha blending ourselves by using these colors
    if (SetTextColor(memory_dc, RGB(255, 255, 255)) == CLR_INVALID)
        goto error;
    if (SetBkColor(memory_dc, RGB(0, 0, 0)) == CLR_INVALID)
        goto error;
    if (!SetBkMode(memory_dc, OPAQUE)) goto error;

    // Get the size of the bitmap
    if (!GetTextExtentPoint32W(memory_dc, text, wcslen(text), &size)) goto error;
    // Bitmap creation fails if the size is zero
    if (size.cx == 0) { size.cx = 1; }
    if (size.cy == 0) { size.cy = 1; }

    // We'll use this bitmap
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi);
    bi.bmiHeader.biWidth = size.cx;
    bi.bmiHeader.biHeight = size.cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    hbitmap = CreateDIBSection(memory_dc, &bi, 0, &bits, NULL, 0);
    if (hbitmap == NULL) goto error;
    if (!SelectObject(memory_dc, (HGDIOBJ)hbitmap)) goto error;

    // Render the text
    if (!TextOutW(memory_dc, 0, 0, text, wcslen(text))) goto error;

    // Get the bitmap
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi);
    bi.bmiHeader.biWidth = size.cx;
    bi.bmiHeader.biHeight = size.cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    surface = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            size.cx, size.cy, 32,
            0x000000FF,  // red
            0x0000FF00,  // green
            0x00FF0000,  // blue
            0xFF000000); // alpha
    if (surface == NULL) goto error;
    if (!GetDIBits(memory_dc, hbitmap, 0, size.cy, surface->pixels, &bi,
            DIB_RGB_COLORS))
        goto error;
    for (y = 0; y < size.cy; ++y)
    {
        row1 = (uint32_t *)bits + size.cx*y;
        row2 = (uint32_t *)surface->pixels + size.cx*(size.cy-1-y);
        for (x = 0; x < size.cx; ++x)
        {
            uint8_t r, g, b, a;
            unsigned alpha = row1[x] & 0xFF;
            // The most common cases are alpha == 0 and alpha == 255
            if (alpha == 0) {
                r = background.r;
                g = background.g;
                b = background.b;
                a = background.a;
            } else if (alpha == 255 && foreground.a == 255) {
                r = foreground.r;
                g = foreground.g;
                b = foreground.b;
                a = foreground.a;
            } else {
                // srcA, dstA and outA are fixed point quantities
                // such that 0xFF00 represents an alpha of 1
                uint_fast32_t srcA = foreground.a * 256 * alpha / 255;
                uint_fast32_t dstA = background.a * 256;
                dstA = dstA * (0xFF00 - srcA) / 0xFF00;
                uint_fast32_t outA = srcA + dstA;
                if (outA == 0) {
                    r = 0;
                    g = 0;
                    b = 0;
                } else {
                    r = (foreground.r*srcA + background.r*dstA) / outA;
                    g = (foreground.g*srcA + background.g*dstA) / outA;
                    b = (foreground.b*srcA + background.b*dstA) / outA;
                }
                a = outA / 256;
            }
            row2[x] = (static_cast<uint32_t>(r) <<  0)
                    | (static_cast<uint32_t>(g) <<  8)
                    | (static_cast<uint32_t>(b) << 16)
                    | (static_cast<uint32_t>(a) << 24);
        }
    }

    DeleteDC(memory_dc);
    DeleteObject(hbitmap);
    return surface;

error:
    DeleteDC(memory_dc);
    DeleteObject(hbitmap);
    if (surface) SDL_FreeSurface(surface);
    return NULL;
}

// Text extent
SDL_Rect SDL2Font::textSize(utf32_t ch)
{
    wchar_t utf16[3];

    return textSizeImpl(m_impl, singleChar(ch, utf16));
}

SDL_Rect SDL2Font::textSize(const char *text)
{
    str_context ctx = str_open_context("SDL2Font::render");
    SDL_Rect rect;

    rect = textSizeImpl(m_impl, win32String(text));
    str_close_context(ctx);
    return rect;
}

static SDL_Rect textSizeImpl(void *m_impl, const wchar_t *text)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    SIZE size;

    // Get the size of the text
    GetTextExtentPoint32W(font->memory_dc, text, wcslen(text), &size);

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = size.cx;
    rect.h = size.cy;

    return rect;
}

// Default font names
const char *SDL2Font::defaultMonoFont(void)
{
    return "Courier New";
}

const char *SDL2Font::defaultSerifFont(void)
{
    return "Times New Roman";
}

const char *SDL2Font::defaultSansFont(void)
{
    return "Arial";
}
