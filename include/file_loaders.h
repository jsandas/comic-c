/*
 * file_loaders.h - Data file format loaders
 * 
 * Functions for loading game data files (.PT, .TT2, .SHP, .EGA)
 */

#ifndef FILE_LOADERS_H
#define FILE_LOADERS_H

#include "globals.h"
#include "level_data.h"

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

/* Runtime SHP cache - holds decoded frames loaded at level load time */
typedef struct {
    uint8_t num_frames;        /* Number of frames loaded */
    uint16_t frame_size;       /* Bytes per frame */
    uint8_t *frames;           /* Contiguous frame data (num_frames * frame_size) */
} shp_runtime_t;

/* Load all SHP files referenced by a level into runtime cache (4 entries) */
int load_level_shp_files(const level_t* level);

/* Free all loaded SHP runtime data */
void free_loaded_shp_files(void);

/* Get pointer to a specific frame's data, or NULL if not available */
const uint8_t* shp_get_frame(uint8_t shp_index, uint8_t frame_index);

/* Get the frame size (bytes) for a loaded SHP, or 0 if not loaded */
uint16_t shp_get_frame_size(uint8_t shp_index);

/* Load EGA file (fullscreen graphics with RLE) - CONVERSION TARGET #4 */
/* Returns 0 on success, non-zero on error */
int load_ega_file(const char* filename, void* video_buffer);

/* load_fullscreen_graphic is defined in graphics.h */

#endif /* FILE_LOADERS_H */
