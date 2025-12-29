/*
 * file_loaders.c - Data file format loaders
 * 
 * DOS file I/O using INT 21h or Open Watcom C library
 */

#include "file_loaders.h"
#include "globals.h"

/* DOS file I/O helper - using Open Watcom C library for simplicity */
#include <stdio.h>
#include <string.h>

/* STUB: C standard library functions (will be replaced with actual I/O) */
#pragma aux fopen_ "*"
FILE *fopen_(const char *name, const char *mode) { return (FILE *)0; }

#pragma aux fread_ "*"
size_t fread_(void *ptr, size_t size, size_t n, FILE *f) { return 0; }

#pragma aux fclose_ "*"
int fclose_(FILE *fp) { return 0; }

/*
 * Load PT file (tile map) - 128x10 tiles
 * 
 * PT file format:
 *   Bytes 0-1: Width (uint16_t, always 128)
 *   Bytes 2-3: Height (uint16_t, always 10)
 *   Bytes 4-1283: Tile IDs (1280 bytes = 128 * 10)
 * 
 * Returns: 0 on success, -1 on error
 */
int load_pt_file(const char* filename, pt_file_t* pt)
{
    FILE* fp;
    size_t bytes_read;
    
    if (!filename || !pt) {
        return -1;
    }
    
    /* Open file in binary mode */
    fp = fopen(filename, "rb");
    if (!fp) {
        return -1;
    }
    
    /* Read width and height */
    bytes_read = fread(&pt->width, sizeof(uint16_t), 1, fp);
    if (bytes_read != 1) {
        fclose(fp);
        return -1;
    }
    
    bytes_read = fread(&pt->height, sizeof(uint16_t), 1, fp);
    if (bytes_read != 1) {
        fclose(fp);
        return -1;
    }
    
    /* Validate dimensions */
    if (pt->width != MAP_WIDTH_TILES || pt->height != MAP_HEIGHT_TILES) {
        fclose(fp);
        return -1;
    }
    
    /* Read tile data */
    bytes_read = fread(pt->tiles, 1, MAP_WIDTH_TILES * MAP_HEIGHT_TILES, fp);
    if (bytes_read != MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

/*
 * Load TT2 file (tileset graphics)
 * 
 * TODO: Implement TT2 loader
 * TT2 format is more complex with EGA-encoded tile graphics
 */
int load_tt2_file(const char* filename, void* buffer)
{
    /* Placeholder - to be implemented */
    (void)filename;
    (void)buffer;
    return -1;
}

/*
 * Load SHP file (sprite graphics)
 * 
 * TODO: Implement SHP loader
 * SHP format contains sprite metadata and frame data
 */
int load_shp_file(const char* filename, void* buffer)
{
    /* Placeholder - to be implemented */
    (void)filename;
    (void)buffer;
    return -1;
}

/*
 * Load EGA file (fullscreen graphics with RLE compression)
 * 
 * TODO: Implement EGA loader with RLE decompression
 * EGA format uses run-length encoding across 4 bitplanes
 */
int load_ega_file(const char* filename, void* video_buffer)
{
    /* Placeholder - to be implemented */
    (void)filename;
    (void)video_buffer;
    return -1;
}
