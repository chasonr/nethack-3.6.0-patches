/* xpmtiles.c -- read a tile map in XPM format */

#include "config.h"
#include "tileset.h"

#ifdef HAVE_XPM
#include <X11/xpm.h>

static boolean FDECL(xpm_color, (struct Pixel *, const XpmColor *));

boolean
read_xpm_tiles(filename, image)
const char *filename;
struct TileSetImage *image;
{
    XpmImage xpm_image;
    XpmInfo xpm_info;
    struct Pixel *palette = NULL; /* custodial */
    unsigned i, j;
    size_t num_pixels, k;
    int rc;

    image->width = 0;
    image->height = 0;
    image->pixels = NULL;       /* custodial, returned */
    image->indexes = NULL;      /* custodial, returned */
    image->image_desc = NULL;   /* custodial, returned */
    image->tile_width = 0;
    image->tile_height = 0;

    memset(&xpm_image, 0, sizeof(xpm_image));
    memset(&xpm_info, 0, sizeof(xpm_info));
    xpm_info.valuemask = XpmReturnExtensions;
    rc = XpmReadFileToXpmImage(filename, &xpm_image, &xpm_info);
    if (rc != 0)
        return FALSE; /* nothing allocated */

    image->width = xpm_image.width;
    image->height = xpm_image.height;
    num_pixels = (size_t) xpm_image.width * (size_t) xpm_image.height;

    /* Convert the color table to our form */
    palette = (struct Pixel *) alloc(xpm_image.ncolors * sizeof(palette[0]));
    for (i = 0; i < xpm_image.ncolors; ++i) {
        rc = xpm_color(&palette[i], &xpm_image.colorTable[i]);
        if (!rc) goto error;
    }

    /* If no more than 256 colors, indicate paletted image */
    if (xpm_image.ncolors <= 256) {
        memcpy(image->palette, palette,
               xpm_image.ncolors * sizeof(palette[0]));
        image->indexes = (unsigned char *) alloc(num_pixels);
        for (k = 0; k < num_pixels; ++k) {
            image->indexes[k] = (unsigned char) xpm_image.data[k];
        }
    }

    /* Return image in RGBA form */
    image->pixels = (struct Pixel *) alloc(num_pixels * sizeof(struct Pixel));
    for (k = 0; k < num_pixels; ++k) {
        image->pixels[k] = palette[xpm_image.data[k]];
    }

    /* Look for a NETHACK3 extension */
    for (i = 0; i < xpm_info.nextensions; ++i) {
        if (strcmp(xpm_info.extensions[i].name, "NETHACK3") == 0) {
            unsigned size = 0;
            char *p;
            for (j = 0; j < xpm_info.extensions[i].nlines; ++j) {
                unsigned s = strlen(xpm_info.extensions[i].lines[j]) + 1;
                size += s;
                /* Watch out for overflow */
                if (size < s) {
                    /* We won't return the extension if there's overflow */
                    size = 0;
                    break;
                }
            }
            if (size != 0) {
                /* Join the extension lines into a single block  */
                image->image_desc = (char *) alloc(size);
                p = image->image_desc;
                for (j = 0; j < xpm_info.extensions[i].nlines; ++j) {
                    strcpy(p, xpm_info.extensions[i].lines[j]);
                    p += strlen(p);
                    *(p++) = '\n';
                }
                *p = '\0';
                break;
            }
        }
    }

    XpmFreeXpmImage(&xpm_image);
    XpmFreeXpmInfo(&xpm_info);
    free(palette);
    return TRUE;

error:
    XpmFreeXpmImage(&xpm_image);
    XpmFreeXpmInfo(&xpm_info);
    free(palette);
    free(image->pixels);
    image->pixels = NULL;
    free(image->indexes);
    image->indexes = NULL;
    free(image->image_desc);
    image->image_desc = NULL;
    return FALSE;
}

static boolean
xpm_color(rgb, color)
struct Pixel *rgb;
const XpmColor *color;
{
    if (color->c_color != NULL) {
        if (strncmp(color->c_color, "None", 4) == 0) {
            /* Transparent pixel */
            rgb->r = 0;
            rgb->g = 0;
            rgb->b = 0;
            rgb->a = 0;
        } else if (color->c_color[0] == '#') {
            /* RGB code */
            size_t len = strlen(color->c_color);
            unsigned r, g, b;
            if (len == 4) {
                if (sscanf(color->c_color + 1, "%1x%1x%1x", &r, &g, &b) != 3)
                    return FALSE;
                r *= 0x11;
                g *= 0x11;
                b *= 0x11;
            } else if (len == 7) {
                if (sscanf(color->c_color + 1, "%2x%2x%2x", &r, &g, &b) != 3)
                    return FALSE;
            } else if (len == 12) {
                if (sscanf(color->c_color + 1, "%4x%4x%4x", &r, &g, &b) != 3)
                    return FALSE;
                r >>= 8;
                g >>= 8;
                b >>= 8;
            } else {
                return FALSE;
            }
            rgb->r = r;
            rgb->g = g;
            rgb->b = b;
            rgb->a = 255;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    return TRUE;
}

#else

boolean
read_xpm_tiles(filename, image)
const char *filename;
struct TileSetImage *image;
{
    return FALSE;
}
#endif
