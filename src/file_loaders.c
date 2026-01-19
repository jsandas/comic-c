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
    
    /* Read tile data - need to loop in case _read doesn't return all bytes at once */
    {
        uint8_t *buf_ptr = pt->tiles;
        unsigned bytes_remaining = MAP_WIDTH_TILES * MAP_HEIGHT_TILES;
        unsigned total_bytes_read = 0;
        
        while (bytes_remaining > 0) {
            bytes_read = _read(file_handle, buf_ptr, bytes_remaining);
            if (bytes_read == 0) {
                /* End of file reached before all data was read */
                break;
            }
            if (bytes_read == -1) {
                /* Read error */
                break;
            }
            total_bytes_read += bytes_read;
            buf_ptr += bytes_read;
            bytes_remaining -= bytes_read;
        }
        
        if (total_bytes_read != MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
            _close(file_handle);
            return -1;
        }
    }
    
    _close(file_handle);
    return 0;
}

/*
 * Load TT2 file (tileset graphics)
 * 
 * TODO: Implement TT2 loader
 * TT2 format contains tile graphics data (variable size, not all levels use 128 tiles)
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
     * This function attempts to read a 3-byte header from the file.
     * 
     * Returns: 0 if file exists and header (3 bytes) read successfully
     *          -1 if file not found, read error, or file is shorter than 3 bytes
     */
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

    /* Try to read 3-byte header; file must be at least 3 bytes long */
    bytes_read = _read(file_handle, shp, 3);
    _close(file_handle);
    
    /* Reject if read failed, returned 0 bytes, or returned fewer than 3 bytes.
     * This ensures the shp_file_t buffer is fully initialized. */
    if (bytes_read != 3) {
        return -1;
    }

    return 0;
}

/* Runtime cache for up to 4 SHP files referenced by a level */
static shp_runtime_t loaded_shps[4] = {0};

/* Open a file with a simple case-insensitive fallback (uppercasing). */
static int open_file_case_insensitive(const char *filename)
{
    int fh;
    char upper_name[14];
    uint16_t i;

    if (filename == NULL) {
        return -1;
    }

    fh = _open(filename, O_RDONLY | O_BINARY);
    if (fh != -1) {
        return fh;
    }

    /* Try uppercase version (8.3 names are typically uppercase in assets). */
    for (i = 0; i < sizeof(upper_name) - 1; i++) {
        char c = filename[i];
        if (c == '\0') {
            upper_name[i] = '\0';
            break;
        }
        if (c >= 'a' && c <= 'z') {
            upper_name[i] = (char)(c - ('a' - 'A'));
        } else {
            upper_name[i] = c;
        }
    }
    upper_name[sizeof(upper_name) - 1] = '\0';

    return _open(upper_name, O_RDONLY | O_BINARY);
}

void free_loaded_shp_files(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (loaded_shps[i].frames) {
            free(loaded_shps[i].frames);
            loaded_shps[i].frames = NULL;
            loaded_shps[i].num_frames = 0;
            loaded_shps[i].frame_size = 0;
            loaded_shps[i].horizontal = 0;
            loaded_shps[i].animation = 0;
            loaded_shps[i].num_distinct = 0;
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

uint8_t shp_get_horizontal(uint8_t shp_index)
{
    if (shp_index >= 4) return 0;
    return loaded_shps[shp_index].horizontal;
}

uint8_t shp_get_animation(uint8_t shp_index)
{
    if (shp_index >= 4) return 0;
    return loaded_shps[shp_index].animation;
}

uint8_t shp_get_num_distinct_frames(uint8_t shp_index)
{
    if (shp_index >= 4) return 0;
    return loaded_shps[shp_index].num_distinct;
}

uint8_t shp_get_animation_length(uint8_t shp_index)
{
    uint8_t distinct = shp_get_num_distinct_frames(shp_index);

    if (distinct <= 1) {
        return distinct;
    }

    if (shp_get_animation(shp_index) == ENEMY_ANIMATION_ALTERNATE) {
        return (uint8_t)((distinct * 2) - 2);
    }

    return distinct;
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
        uint16_t frames_in_file;
        uint8_t *buf;
        long bytes_read_total;
        int r;

        if (s->num_distinct_frames == SHP_UNUSED) {
            /* leave entry empty */
            continue;
        }

        /* Open file and determine size */
        fh = open_file_case_insensitive(s->filename);
        if (fh == -1) {
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

        if (s->horizontal == ENEMY_HORIZONTAL_SEPARATE) {
            frames_in_file = (uint16_t)(s->num_distinct_frames * 2);
        } else {
            frames_in_file = s->num_distinct_frames;
        }

        if (frames_in_file == 0 || (file_len % frames_in_file) != 0) {
            /* Unexpected size; skip loading */
            _close(fh);
            continue;
        }

        /* Validate that frame size won't overflow uint16_t */
        if ((file_len / frames_in_file) > 65535L) {
            _close(fh);
            continue;
        }

        frame_size = (uint16_t)(file_len / frames_in_file);

        /* Accept only known frame sizes: 80 (16x8), 160 (16x16), 320 (16x32) */
        if (frame_size != 80 && frame_size != 160 && frame_size != 320) {
            _close(fh);
            continue;
        }

        /* Allocate buffer and read whole file */
        buf = (uint8_t *)malloc(file_len);
        if (!buf) {
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
            free(buf);
            continue;
        }

        /* Store into runtime cache */
        loaded_shps[i].num_frames = frames_in_file;
        loaded_shps[i].frame_size = frame_size;
        loaded_shps[i].frames = buf;
        loaded_shps[i].horizontal = s->horizontal;
        loaded_shps[i].animation = s->animation;
        loaded_shps[i].num_distinct = s->num_distinct_frames;
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
