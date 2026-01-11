/*
 * file_loaders.c - Data file format loaders
 * 
 * DOS file I/O using Open Watcom C library (fcntl.h)
 */

#include "file_loaders.h"
#include "globals.h"
#include "level_data.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    /* Legacy minimal loader retained for compatibility. The new runtime
     * SHP loader provides frame storage and accessors (see load_level_shp_files).
     * This function will attempt to read a small header if present, otherwise
     * return success if the file exists. */
    int file_handle;
    shp_file_t* shp = (shp_file_t*)buffer;
    int bytes_read;

    if (!filename || !shp) {
        return -1;
    }

    file_handle = _open(filename, O_RDONLY | O_BINARY);
    if (file_handle == -1) {
        return -1;
    }

    /* Try to read up to 3 bytes into header structure; if file shorter it's an error */
    bytes_read = _read(file_handle, shp, 3);
    if (bytes_read < 0) {
        _close(file_handle);
        return -1;
    }

    _close(file_handle);
    return 0;
}

/* Runtime cache for up to 4 SHP files referenced by a level */
static shp_runtime_t loaded_shps[4] = {0};

void free_loaded_shp_files(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (loaded_shps[i].frames) {
            free(loaded_shps[i].frames);
            loaded_shps[i].frames = NULL;
            loaded_shps[i].num_frames = 0;
            loaded_shps[i].frame_size = 0;
        }
    }
}

const uint8_t* shp_get_frame(uint8_t shp_index, uint8_t frame_index)
{
    if (shp_index >= 4) return NULL;
    if (loaded_shps[shp_index].frames == NULL) return NULL;
    if (frame_index >= loaded_shps[shp_index].num_frames) return NULL;
    /* Cast to unsigned long to avoid overflow: frame_index (0-255) * frame_size (80-320) = 0-81,600 */
    return (const uint8_t*)(loaded_shps[shp_index].frames + ((unsigned long)frame_index * loaded_shps[shp_index].frame_size));
}

uint16_t shp_get_frame_size(uint8_t shp_index)
{
    if (shp_index >= 4) return 0;
    return loaded_shps[shp_index].frame_size;
}

int load_level_shp_files(const level_t* level)
{
    int i;
    int files_loaded = 0;

    if (!level) return -1;

    /* Free any existing SHP data first */
    free_loaded_shp_files();

    for (i = 0; i < 4; i++) {
        const shp_t* s = &level->shp[i];
        int fh;
        long file_len;
        uint16_t frame_size;
        uint8_t *buf;
        long bytes_read_total;
        int r;

        if (s->num_distinct_frames == SHP_UNUSED) {
            /* leave entry empty */
            continue;
        }

        /* Open file and determine size */
        fh = _open(s->filename, O_RDONLY | O_BINARY);
        if (fh == -1) {
            /* Failed to open - skip */
            continue;
        }

        file_len = _lseek(fh, 0, SEEK_END);
        if (file_len <= 0) {
            _close(fh);
            continue;
        }

        /* Compute expected frame size */
        if (s->num_distinct_frames == 0) {
            _close(fh);
            continue;
        }

        if (file_len % s->num_distinct_frames != 0) {
            /* Unexpected size; skip loading */
            fprintf(stderr, "WARNING: SHP file '%s' size %ld not divisible by %u frames\n", 
                    s->filename, file_len, s->num_distinct_frames);
            _close(fh);
            continue;
        }

        /* Validate that frame size won't overflow uint16_t */
        if ((file_len / s->num_distinct_frames) > 65535L) {
            fprintf(stderr, "WARNING: SHP file '%s' frame size exceeds 65535 bytes\n", s->filename);
            _close(fh);
            continue;
        }

        frame_size = (uint16_t)(file_len / s->num_distinct_frames);

        /* Accept only known frame sizes: 80 (16x8), 160 (16x16), 320 (16x32) */
        if (frame_size != 80 && frame_size != 160 && frame_size != 320) {
            fprintf(stderr, "WARNING: SHP file '%s' has unsupported frame size %u bytes\n", 
                    s->filename, frame_size);
            _close(fh);
            continue;
        }

        /* Allocate buffer and read whole file */
        buf = (uint8_t *)malloc(file_len);
        if (!buf) {
            fprintf(stderr, "ERROR: Failed to allocate %ld bytes for SHP file '%s'\n", 
                    file_len, s->filename);
            _close(fh);
            continue;
        }

        _lseek(fh, 0, SEEK_SET);
        bytes_read_total = 0;
        while (bytes_read_total < file_len) {
            r = _read(fh, buf + bytes_read_total, file_len - bytes_read_total);
            if (r <= 0) break;
            bytes_read_total += r;
        }
        _close(fh);

        if (bytes_read_total != file_len) {
            fprintf(stderr, "ERROR: Failed to read SHP file '%s' (got %ld of %ld bytes)\n", 
                    s->filename, bytes_read_total, file_len);
            free(buf);
            continue;
        }

        /* Store into runtime cache */
        loaded_shps[i].num_frames = s->num_distinct_frames;
        loaded_shps[i].frame_size = frame_size;
        loaded_shps[i].frames = buf;
        files_loaded++;
    }

    /* Return the number of successfully loaded files (0 = all failed, 1-4 = success) */
    return files_loaded;
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
