#ifndef TILESET_H
#define TILESET_H

struct Pixel {
    unsigned char r, g, b, a;
};

/* To let the tileset interface split the tiles into separate image, use
 * these functions */

struct TileImage {
    /* Image data */
    unsigned width, height;
    struct Pixel *pixels; /* for direct color */
    unsigned char *indexes; /* for paletted images */
};

boolean FDECL(read_tiles, (const char *filename, BOOLEAN_P true_color));
const struct Pixel *NDECL(get_palette);
void NDECL(free_tiles);
const struct TileImage *FDECL(get_tile, (unsigned tile_index));

/* To read the tileset as a single image, use these functions */

struct TileSetImage {
    /* Image data */
    unsigned width, height;
    struct Pixel *pixels; /* for direct color */
    unsigned char *indexes; /* for paletted images */
    struct Pixel palette[256];
    
    /* Image description from the file */
    char *image_desc;

    /* Tile dimensions */
    unsigned tile_width, tile_height;
};

boolean FDECL(read_tile_image, (struct TileSetImage *image,
            const char *filename, BOOLEAN_P true_color));
void FDECL(free_tile_image, (struct TileSetImage *image));

/* For resizing tiles */
struct TileImage *FDECL(stretch_tile, (const struct TileImage *,
                                       unsigned, unsigned));
void FDECL(free_tile, (struct TileImage *));

/* Used internally by the tile set code */
boolean FDECL(read_bmp_tiles, (const char *filename, struct TileSetImage *image));
boolean FDECL(read_gif_tiles, (const char *filename, struct TileSetImage *image));
boolean FDECL(read_png_tiles, (const char *filename, struct TileSetImage *image));
boolean FDECL(do_read_png_tiles, (FILE *fp, struct TileSetImage *image));
boolean FDECL(read_xpm_tiles, (const char *filename, struct TileSetImage *image));

#endif
