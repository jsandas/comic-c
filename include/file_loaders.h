/*
 * file_loaders.h - Data file format loaders
 * 
 * Functions for loading game data files (.PT, .TT2, .SHP, .EGA)
 */

#ifndef FILE_LOADERS_H
#define FILE_LOADERS_H

#include "globals.h"

/* PT file structure - tile map (128x10 tiles) */
typedef struct {
    uint16_t width;   /* Always 128 */
    uint16_t height;  /* Always 10 */
    uint8_t tiles[MAP_WIDTH_TILES * MAP_HEIGHT_TILES];  /* 1280 bytes */
} pt_file_t;

/* TT2 file structure - tileset graphics */
typedef struct {
    uint16_t num_tiles;
    /* Followed by EGA-encoded tile data */
} tt2_file_t;

/* SHP file structure - sprite graphics */
typedef struct {
    uint8_t horizontal;     /* ENEMY_HORIZONTAL_DUPLICATED or ENEMY_HORIZONTAL_SEPARATE */
    uint8_t animation;      /* ENEMY_ANIMATION_LOOP or ENEMY_ANIMATION_ALTERNATE */
    uint8_t num_frames;     /* Number of animation frames */
    /* Followed by frame data */
} shp_file_t;

/* EGA file structure - fullscreen graphics (320x200, 16-color) */
typedef struct {
    uint16_t plane_size;    /* Always 8000 (bytes per bitplane) */
    /* Followed by RLE-compressed data for 4 bitplanes */
} ega_file_t;

/* File loading functions */
/* These will be converted from assembly to C incrementally */

/* Load PT file (tile map) - CONVERSION TARGET #1 */
/* Returns 0 on success, non-zero on error */
int load_pt_file(const char* filename, pt_file_t* pt);

/* Load TT2 file (tileset graphics) - CONVERSION TARGET #2 */
/* Returns 0 on success, non-zero on error */
int load_tt2_file(const char* filename, void* buffer);

/* Load SHP file (sprite graphics) - CONVERSION TARGET #3 */
/* Returns 0 on success, non-zero on error */
int load_shp_file(const char* filename, void* buffer);

/* Load EGA file (fullscreen graphics with RLE) - CONVERSION TARGET #4 */
/* Returns 0 on success, non-zero on error */
int load_ega_file(const char* filename, void* video_buffer);

/* Assembly versions (to be replaced) */
extern void load_fullscreen_graphic(void);  /* Assembly: load .EGA file */

#endif /* FILE_LOADERS_H */
