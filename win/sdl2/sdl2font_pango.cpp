// sdl2font_pango.cpp

#ifndef __APPLE__
#include <pango/pangoft2.h>
#include <cstring>
#include <stdint.h>
#include "sdl2font.h"

// Classes and functions used internally here:
namespace
{

struct SDL_TTF_FontImpl
{
    PangoFontDescription *desc;
    PangoFontMap *map;
    PangoContext *ctx;
    PangoLayout *layout;
};

// C++ wrapper for the FT_Bitmap type, to equip it with a constructor and a
// destructor
class SDL_FT_Bitmap : public ::FT_Bitmap
{
public:
    SDL_FT_Bitmap(int width, int height)
    {
        // Per the pango-view example
        this->width = width;
        this->pitch = (this->width + 3) & ~3;
        this->rows = height;
        this->buffer = new unsigned char[this->pitch * this->rows];
        this->num_grays = 255;
        this->pixel_mode = ft_pixel_mode_grays;
        std::memset(this->buffer, 0x00, this->pitch * this->rows);
        this->palette_mode = 0;
        this->palette = NULL;
    }
    ~SDL_FT_Bitmap(void)
    {
        delete[] this->buffer;
    }
};

};

static PangoFontMetrics *TTF_GetMetrics(const SDL_TTF_FontImpl *fdata);

SDL2Font::SDL2Font(const char *name, int ptsize) :
    m_impl(NULL)
{
    SDL_TTF_FontImpl *fdata = new SDL_TTF_FontImpl;

    fdata->desc = pango_font_description_new();
    fdata->map = pango_ft2_font_map_new();
    fdata->ctx = pango_font_map_create_context(fdata->map);
    fdata->layout = pango_layout_new(fdata->ctx);

    pango_font_description_set_family(fdata->desc, name);
    pango_font_description_set_size(fdata->desc, ptsize*PANGO_SCALE);
    pango_layout_set_font_description(fdata->layout, fdata->desc);

    m_impl = reinterpret_cast<void *>(fdata);
}

SDL2Font::~SDL2Font(void)
{
    SDL_TTF_FontImpl *fdata = reinterpret_cast<SDL_TTF_FontImpl *>(m_impl);

    g_object_unref(fdata->layout);
    g_object_unref(fdata->ctx);
    g_object_unref(fdata->map);
    pango_font_description_free(fdata->desc);
    delete fdata;
}

// Font metrics
int SDL2Font::fontAscent(void)
{
    const SDL_TTF_FontImpl *fdata = reinterpret_cast<const SDL_TTF_FontImpl *>(m_impl);

    PangoFontMetrics *metrics = TTF_GetMetrics(fdata);
    int ascent = pango_font_metrics_get_ascent(metrics);
    pango_font_metrics_unref(metrics);
    return ascent / PANGO_SCALE;
}

int SDL2Font::fontDescent(void)
{
    const SDL_TTF_FontImpl *fdata = reinterpret_cast<const SDL_TTF_FontImpl *>(m_impl);

    PangoFontMetrics *metrics = TTF_GetMetrics(fdata);
    int descent = pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    return descent / PANGO_SCALE;
}

int SDL2Font::fontLineSkip(void)
{
    const SDL_TTF_FontImpl *fdata = reinterpret_cast<const SDL_TTF_FontImpl *>(m_impl);

    PangoFontMetrics *metrics = TTF_GetMetrics(fdata);
    int skip = pango_font_metrics_get_ascent(metrics)
             + pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    return skip / PANGO_SCALE;
}

int SDL2Font::fontHeight(void)
{
    const SDL_TTF_FontImpl *fdata = reinterpret_cast<const SDL_TTF_FontImpl *>(m_impl);

    PangoFontMetrics *metrics = TTF_GetMetrics(fdata);
    int height = pango_font_metrics_get_ascent(metrics)
               + pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    return height / PANGO_SCALE;
}

// Text rendering
// If no background is given, background is transparent
SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground)
{
    char utf8[4];
    std::size_t size;

    size = unicode::put_next_cp(utf8, ch);
    return render(std::string(utf8, size), foreground);
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground)
{
    return render(text, foreground, (SDL_Color){ 0, 0, 0, 0 });
}

SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground, SDL_Color background)
{
    char utf8[4];
    std::size_t size;

    size = unicode::put_next_cp(utf8, ch);
    return render(std::string(utf8, size), foreground, background);
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground, SDL_Color background)
{
    SDL_TTF_FontImpl *fdata = reinterpret_cast<SDL_TTF_FontImpl *>(m_impl);

    pango_layout_set_text(fdata->layout, text.c_str(), text.size());

    int width, height;
    pango_layout_get_pixel_size(fdata->layout, &width, &height);
    SDL_FT_Bitmap *bitmap = new SDL_FT_Bitmap(width, height);

    pango_ft2_render_layout(bitmap, fdata->layout, 0, 0);

    SDL_Surface *surface = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            bitmap->width, bitmap->rows, 32,
            0x000000FF,  // red
            0x0000FF00,  // green
            0x00FF0000,  // blue
            0xFF000000); // alpha
    if (surface != NULL)
    {
        unsigned char *row1;
        uint32_t *row2;

        for (int y = 0; y < bitmap->rows; ++y)
        {
            row1 = bitmap->buffer + bitmap->pitch * y;
            row2 = (uint32_t *)((uint8_t *)surface->pixels + surface->pitch*y);
            for (int x = 0; x < bitmap->width; ++x)
            {
                uint8_t r, g, b, a;
                uint8_t alpha = row1[x];
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
    }

    delete bitmap;
    return surface;
}

// Text extent
SDL_Rect SDL2Font::textSize(utf32_t ch)
{
    char utf8[4];
    std::size_t size;

    size = unicode::put_next_cp(utf8, ch);
    return textSize(std::string(utf8, size));
}

SDL_Rect SDL2Font::textSize(const std::string& text)
{
    SDL_TTF_FontImpl *fdata = reinterpret_cast<SDL_TTF_FontImpl *>(m_impl);

    pango_layout_set_text(fdata->layout, text.c_str(), text.size());

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    pango_layout_get_pixel_size(fdata->layout, &rect.w, &rect.h);

    return rect;
}

// Default font names
const char *SDL2Font::defaultMonoFont(void)
{
    return "DejaVu Sans Mono";
}

const char *SDL2Font::defaultSerifFont(void)
{
    return "DejaVu Serif";
}

const char *SDL2Font::defaultSansFont(void)
{
    return "DejaVu Sans";
}

// Retrieve metrics for a font
static PangoFontMetrics *TTF_GetMetrics(const SDL_TTF_FontImpl *fdata)
{
    PangoFont *pfont = pango_font_map_load_font(fdata->map, fdata->ctx, fdata->desc);

    PangoFontMetrics *metrics = pango_font_get_metrics(pfont, NULL);

    g_object_unref(pfont);
    return metrics;
}
#endif
