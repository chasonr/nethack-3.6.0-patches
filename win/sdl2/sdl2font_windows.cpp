//  sdl2font_windows.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include "sdl2font.h"

struct FontImpl
{
    HFONT hfont;
    HDC memory_dc;
};

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

// Text rendering
// If no background is given, background is transparent
SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground)
{
    return render(ch, foreground, (SDL_Color){ 0, 0, 0, 0 });
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground)
{
    return render(text, foreground, (SDL_Color){ 0, 0, 0, 0 });
}

SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground, SDL_Color background)
{
    wchar_t utf16[2];
    std::size_t size;

    size = unicode::put_next_cp(utf16, ch);
    return render(std::wstring(utf16, size), foreground, background);
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground, SDL_Color background)
{
    return render(unicode::convert<wchar_t>(text), foreground, background);
}

SDL_Surface *SDL2Font::render(const std::wstring& text, SDL_Color foreground, SDL_Color background)
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
    if (!GetTextExtentPoint32W(memory_dc, text.c_str(), text.size(), &size)) goto error;
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
    if (!TextOutW(memory_dc, 0, 0, text.c_str(), text.size())) goto error;

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
    wchar_t utf16[2];
    std::size_t size;

    size = unicode::put_next_cp(utf16, ch);
    return textSize(std::wstring(utf16, size));
}

SDL_Rect SDL2Font::textSize(const std::string& text)
{
    return textSize(unicode::convert<wchar_t>(text));
}

SDL_Rect SDL2Font::textSize(const std::wstring& text)
{
    FontImpl *font = reinterpret_cast<FontImpl *>(m_impl);
    SIZE size;

    // Get the size of the text
    GetTextExtentPoint32W(font->memory_dc, text.c_str(), text.size(), &size);

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
