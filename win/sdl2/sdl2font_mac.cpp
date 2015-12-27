// sdl2font_mac.cpp

#ifdef __APPLE__
extern "C" {
#include "hack.h"
}
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <string>
#include "unicode.h"

#include "sdl2font.h"

namespace {

template <class Reference>
class RefWrapper
{
public:
    RefWrapper(void) : m_ref(NULL) {}
    RefWrapper(Reference ref) : m_ref(ref) { }
    ~RefWrapper(void) { if (m_ref) CFRelease(m_ref); }

    Reference get(void) const { return m_ref; }
    void set(Reference ref) {
        if (m_ref) CFRelease(m_ref);
        m_ref = ref;
    }

private:
    Reference m_ref;
};

// RGB color space
RefWrapper<CGColorSpaceRef> rgb_colorspace(CGColorSpaceCreateDeviceRGB());

// Transparent background
SDL_Color transparent = { 0, 0, 0, 0 };

struct FontData
{
    RefWrapper<CTFontRef> font;
    RefWrapper<CFDictionaryRef> attributes;
};

std::basic_string<UniChar>
singleChar(utf32_t ch)
{
    if (ch > 0x10FFFF || (ch & 0xFFFFF800) == 0xD800) {
        ch = 0xFFFD;
    }

    if (ch < 0x10000) {
        return std::basic_string<UniChar>(1, (UniChar) ch);
    } else {
        UniChar str16[2];

        str16[0] = 0xD7C0 + (ch >> 10);
        str16[1] = 0xDC00 + (ch & 0x3FF);
        return std::basic_string<UniChar>(str16, 2);
    }
}

std::basic_string<UniChar>
appleString(const std::string& str)
{
    StringContext ctx("appleString");

    utf16_t *str16;
    UniChar *astr16;
    std::size_t size16, i;
    std::basic_string<UniChar> out;

    str16 = uni_8to16(str.c_str());
    size16 = uni_length16(str16);
    astr16 = (UniChar *) str_mem_alloc((size16 + 1) * sizeof(astr16[0]));
    for (i = 0; str16[i] != 0; ++i) {
        astr16[i] = str16[i];
    }
    return std::basic_string<UniChar>(astr16, i);
}

}

SDL2Font::SDL2Font(const char *name, int ptsize)
{
    FontData *mfont = new FontData;

    RefWrapper<CFStringRef> nameref(
            CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8));
    mfont->font.set(CTFontCreateWithName(nameref.get(), ptsize, NULL));

    CFStringRef keys[] = { kCTFontAttributeName };
    CFTypeRef values[] = { mfont->font.get() };
    mfont->attributes.set(
            CFDictionaryCreate(NULL,
                    (const void **)keys, (const void **)values,
                    sizeof(keys)/sizeof(keys[0]),
                    &kCFTypeDictionaryKeyCallBacks,
                    &kCFTypeDictionaryValueCallBacks));

    m_impl = reinterpret_cast<void *>(mfont);
}

SDL2Font::~SDL2Font(void)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    delete mfont;
}

// Font metrics
int SDL2Font::fontAscent(void)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return static_cast<int>(CTFontGetAscent(mfont->font.get()));
}

int SDL2Font::fontDescent(void)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return static_cast<int>(CTFontGetDescent(mfont->font.get()));
}

int SDL2Font::fontLineSkip(void)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return static_cast<int>(
            CTFontGetAscent(mfont->font.get())
          + CTFontGetDescent(mfont->font.get())
    );
}

int SDL2Font::fontHeight(void)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return static_cast<int>(
            CTFontGetAscent(mfont->font.get())
          + CTFontGetDescent(mfont->font.get())
          + CTFontGetLeading(mfont->font.get())
    );
}

// Text rendering
// If no background is given, background is transparent
SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground)
{
    return render(ch, foreground, transparent);
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground)
{
    return render(text, foreground, transparent);
}

static SDL_Surface *renderImpl(const std::basic_string<UniChar>& text,
        SDL_Color foreground, SDL_Color background,
        FontData *mfont)
{
    RefWrapper<CFStringRef> string(
            CFStringCreateWithCharacters(NULL, text.c_str(), text.size()));

    RefWrapper<CFAttributedStringRef> attrstring(
            CFAttributedStringCreate(NULL, string.get(), mfont->attributes.get()));

    // CoreText line with complex rendering
    RefWrapper<CTLineRef> line(
            CTLineCreateWithAttributedString(attrstring.get()));

    // Size of text
    CGFloat width, ascent, descent, leading;
    width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);
    int w = static_cast<int>(width);
    int h = static_cast<int>(ascent + descent + leading);
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    // SDL expects the color components not to be premultiplied by the alpha.
    // CoreGraphics does not support this usage, so we have to fake it: the
    // text is drawn in black and the returned pixels have only alpha, and we
    // fill in the colors later.

    // SDL surface to receive the bitmap
    SDL_Surface *surface = SDL_CreateRGBSurface(
                SDL_SWSURFACE,
                w, h, 32,
                0x000000FF,  // red
                0x0000FF00,  // green
                0x00FF0000,  // blue
                0xFF000000); // alpha
    if (surface == NULL) return NULL;

    // CoreGraphics bitmap context
    RefWrapper<CGContextRef> context(
        CGBitmapContextCreate(surface->pixels, w, h, 8,
                surface->pitch, rgb_colorspace.get(),
                kCGImageAlphaPremultipliedLast));
    CGContextSetTextDrawingMode(context.get(), kCGTextFillStrokeClip);

    CGContextSetTextPosition(context.get(), 0.0, descent + leading/2.0);
    CTLineDraw(line.get(), context.get());

    for (int y = 0; y < surface->h; ++y)
    {
        uint32_t *row = reinterpret_cast<uint32_t *>(
                reinterpret_cast<uint8_t *>(surface->pixels) + surface->pitch*y);
        for (int x = 0; x < surface->w; ++x)
        {
            unsigned alpha = row[x] >> 24;
            uint8_t r, g, b, a;
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
            row[x] = (static_cast<uint32_t>(r) <<  0)
                   | (static_cast<uint32_t>(g) <<  8)
                   | (static_cast<uint32_t>(b) << 16)
                   | (static_cast<uint32_t>(a) << 24);
        }
    }

    return surface;
}

SDL_Surface *SDL2Font::render(utf32_t ch, SDL_Color foreground, SDL_Color background)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return renderImpl(singleChar(ch), foreground, background, mfont);
}

SDL_Surface *SDL2Font::render(const std::string& text, SDL_Color foreground, SDL_Color background)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return renderImpl(appleString(text), foreground, background, mfont);
}

// Text extent
SDL_Rect textSizeImpl(const std::basic_string<UniChar>& text, FontData *mfont)
{
    RefWrapper<CFStringRef> string(
            CFStringCreateWithCharacters(NULL, text.c_str(), text.size()));

    RefWrapper<CFAttributedStringRef> attrstring(
            CFAttributedStringCreate(NULL, string.get(), mfont->attributes.get()));

    RefWrapper<CTLineRef> line(
            CTLineCreateWithAttributedString(attrstring.get()));

    CGFloat width, ascent, descent, leading;
    width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);
    
    SDL_Rect size;
    size.x = 0;
    size.y = 0;
    size.w = width;
    size.h = ascent + descent + leading;

    return size;
}

SDL_Rect SDL2Font::textSize(utf32_t ch)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return textSizeImpl(singleChar(ch), mfont);
}

SDL_Rect SDL2Font::textSize(const std::string& text)
{
    FontData *mfont = reinterpret_cast<FontData *>(m_impl);

    return textSizeImpl(appleString(text), mfont);
}

// Default font names
const char *SDL2Font::defaultMonoFont(void)
{
    return "CourierNewPSMT";
}

const char *SDL2Font::defaultSerifFont(void)
{
    return "TimesNewRomanPSMT";
}

const char *SDL2Font::defaultSansFont(void)
{
    return "ArialMT";
}
#endif
