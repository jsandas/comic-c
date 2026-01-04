/*
 * file_loaders.c - Data file format loaders
 * 
 * DOS file I/O using Open Watcom C library (fcntl.h)
 */

#include "file_loaders.h"
#include "globals.h"
#include <fcntl.h>
#include <io.h>
#include <string.h>

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
    int file_handle;
    unsigned bytes_read;
    
    if (!filename || !pt) {
        return -1;
    }
    
    /* Open file in binary read-only mode */
    file_handle = _open(filename, O_RDONLY | O_BINARY);
    if (file_handle == -1) {
        return -1;
    }
    
    /* Read width */
    bytes_read = _read(file_handle, &pt->width, sizeof(uint16_t));
    if (bytes_read != sizeof(uint16_t)) {
        _close(file_handle);
        return -1;
    }
    
    /* Read height */
    bytes_read = _read(file_handle, &pt->height, sizeof(uint16_t));
    if (bytes_read != sizeof(uint16_t)) {
        _close(file_handle);
        return -1;
    }
    
    /* Validate dimensions */
    if (pt->width != MAP_WIDTH_TILES || pt->height != MAP_HEIGHT_TILES) {
        _close(file_handle);
        return -1;
    }
    
    /* Read tile data */
    bytes_read = _read(file_handle, pt->tiles, MAP_WIDTH_TILES * MAP_HEIGHT_TILES);
    if (bytes_read != MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
        _close(file_handle);
        return -1;
    }
    
    _close(file_handle);
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
