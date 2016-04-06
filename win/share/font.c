/* Maintain a data structure describing a monospaced bitmap font */

#include "hack.h"
#include "integer.h"
#include "unicode.h"
#include "font.h"

static uint32 FDECL(read_u32, (const unsigned char *));
static void FDECL(add_unicode_index, (
            struct BitmapFont *font,
            utf32_t ch,
            unsigned index));

struct BitmapFont *
load_font(filename)
const char *filename;
{
    str_context ctx = str_open_context("load_font");

    FILE *fp;
    struct BitmapFont *font = NULL;
    unsigned char header[32];
    size_t size;
    uint32 magic;
    uint32 version;
    uint32 headersize;
    uint32 flags;
    uint32 length;
    uint32 charsize;
    uint32 height;
    uint32 width;
    uint32 bwidth, memsize;
    uint32 i;

    fp = fopen(filename, "rb");
    if (fp == NULL) goto error;

    /* Read the PSF header */
    size = fread(header, 1, sizeof(header), fp);
    if (size != sizeof(header)) goto error;

    /* Convert from little endian order */
    magic      = read_u32(header +  0);
    if (magic != 0x864AB572) goto error;
    version    = read_u32(header +  4);
    if (version != 0) goto error;
    headersize = read_u32(header +  8);
    if (headersize < sizeof(header)) goto error;
    flags      = read_u32(header + 12);
    length     = read_u32(header + 16);
    charsize   = read_u32(header + 20);
    height     = read_u32(header + 24);
    width      = read_u32(header + 28);

    /* Check that the declared character size can hold the declared width
       and height */
    bwidth = (width + 7) / 8;
    memsize = bwidth * height;
    if (memsize > charsize) goto error;

    /* Allocate a font structure */
    font = (struct BitmapFont *) alloc(sizeof(struct BitmapFont));
    memset(font, 0, sizeof(struct BitmapFont));

    /* Dimensions of the font */
    font->width = width;
    font->height = height;
    font->num_glyphs = length;

    /* The glyph array */
    font->glyphs = (unsigned char **) alloc(length * sizeof(unsigned char *));
    memset(font->glyphs, 0, length * sizeof(unsigned char *));

    /* Read the glyphs */
    fseek(fp, headersize, SEEK_SET);
    for (i = 0; i < length; ++i) {
        font->glyphs[i] = (unsigned char *) alloc(memsize);
        size = fread(font->glyphs[i], 1, memsize, fp);
        if (size != memsize) goto error;
        fseek(fp, charsize - memsize, SEEK_CUR);
    }

    if (flags & 0x01) {
        /* Read the Unicode table */
        char buf[128], buf2[128+1];
        unsigned bufsize, strsize;
        char *p;
        utf32_t *codepoints;

        bufsize = 0;
        i = 0;
        while (i < length) {
            unsigned j;

            size = fread(buf + bufsize, 1, sizeof(buf) - bufsize, fp);
            if (ferror(fp)) goto error;
            bufsize += size;
            if (bufsize == 0) goto error; /* unexpected EOF */

            p = memchr(buf, 0xFF, bufsize);
            if (p != NULL) { /* end marker found */
                strsize = p - buf;
                memcpy(buf2, buf, strsize);
                buf2[strsize] = '\0';
                bufsize -= strsize + 1;
                memmove(buf, buf + strsize + 1, bufsize);
            } else { /* partial string */
                strsize = bufsize - 1;
                /* Roll back to character boundary in case of partial character */
                while (strsize != 0 && (buf[strsize] & 0xC0) == 0x80)
                    --strsize;
                if (strsize == 0) /* avoid infinite loop */
                    strsize = (bufsize < 4) ? bufsize : 4;
                memcpy(buf2, buf, strsize);
                buf2[strsize] = '\0';
                bufsize -= strsize;
                memmove(buf, buf + strsize, bufsize);
            }
            codepoints = uni_8to32(buf2);
            for (j = 0; codepoints[j] != 0; ++j) {
                add_unicode_index(font, j, i);
            }
            if (p != NULL)
                ++i;
        }
    } else {
        /* Fake a Unicode table, assuming that ASCII glyphs are in the
           expected places */
        for (i = 0x20; i <= 0x7E; ++i) {
            add_unicode_index(font, i, i);
        }
    }

    fclose(fp);
    str_close_context(ctx);
    return font;

error:
    if (fp != NULL) fclose(fp);
    free_font(font);
    str_close_context(ctx);
    return NULL;
}

void
free_font(font)
struct BitmapFont *font;
{
    unsigned i, j;

    if (font == NULL) return;

    if (font->glyphs != NULL) {
        for (i = 0; i < font->num_glyphs; ++i)
            free(font->glyphs[i]);
        free(font->glyphs);
    }

    for (i = 0; i < SIZE(font->unicode); ++i) {
        if (font->unicode[i] == NULL) continue;
        for (j = 0; j < 256; ++j)
            free(font->unicode[i][j]);
        free(font->unicode[i]);
    }

    free(font);
}

const unsigned char *
get_font_glyph(font, ch, unicode)
struct BitmapFont *font;
utf32_t ch;
boolean unicode;
{
    unsigned index;

    if (unicode) {
        index = 0;
        if (ch <= 0x10FFFF && (ch & 0xFFFFD800) != 0xD800) {
            unsigned i, j, k;

            i = (unsigned) (ch >> 16);
            j = (unsigned) ((ch >> 8) & 0xFF);
            k = (unsigned) (ch & 0xFF);
            if (font->unicode[i] != NULL
            &&  font->unicode[i][j] != NULL) {
                index = font->unicode[i][j][k];
            }
        }
    } else {
        index = ch;
    }

    if (index >= font->num_glyphs)
        index = 0;

    return font->glyphs[index];
}

static void
add_unicode_index(font, ch, index)
struct BitmapFont *font;
utf32_t ch;
unsigned index;
{
    unsigned i, j, k;

    i = (unsigned) (ch >> 16);
    j = (unsigned) ((ch >> 8) & 0xFF);
    k = (unsigned) (ch & 0xFF);

    if (font->unicode[i] == NULL) {
        /* Create the second level node */
        font->unicode[i] = (unsigned **) alloc(256 * sizeof(unsigned *));
        memset(font->unicode[i], 0, 256 * sizeof(unsigned *));
    }
    if (font->unicode[i][j] == NULL) {
        /* Create the third level node */
        font->unicode[i][j] = (unsigned *) alloc(256 * sizeof(unsigned));
        memset(font->unicode[i][j], 0, 256 * sizeof(unsigned));
    }
    font->unicode[i][j][k] = index;
}

static uint32
read_u32(buf)
const unsigned char *buf;
{
    return ((uint32) buf[0] <<  0)
         | ((uint32) buf[1] <<  8)
         | ((uint32) buf[2] << 16)
         | ((uint32) buf[3] << 24);
}
