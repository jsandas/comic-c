/*
 * graphics.h - Graphics loading and rendering declarations
 * 
 * Functions for loading .EGA graphics files, managing EGA color planes,
 * and controlling video memory buffers.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

/* Video buffer offsets */
#define GRAPHICS_BUFFER_GAMEPLAY_A  0x0000  /* 0xa000:0000 - Double buffer page A (8KB) */
#define GRAPHICS_BUFFER_GAMEPLAY_B  0x2000  /* 0xa000:2000 - Double buffer page B (8KB) */
#define GRAPHICS_BUFFER_RENDERED_MAP 0x4000 /* 0xa000:4000 - Rendered map (40KB) */
#define GRAPHICS_BUFFER_TITLE_TEMP1 0x8000  /* 0xa000:8000 - Title temp buffer 1 (8KB) */
#define GRAPHICS_BUFFER_TITLE_TEMP2 0xa000  /* 0xa000:a000 - Title temp buffer 2 (8KB) */

/* EGA plane indices */
#define EGA_PLANE_BLUE       0
#define EGA_PLANE_GREEN      1
#define EGA_PLANE_RED        2
#define EGA_PLANE_INTENSITY  3

/* Graphics file loaders */
int load_fullscreen_graphic(const char *filename, uint16_t dst_offset);

/* EGA plane management */
void enable_ega_plane_write(uint8_t plane);
void enable_ega_plane_read(uint8_t plane);
void enable_ega_plane_read_write(uint8_t plane);

/* RLE decoding */
uint16_t rle_decode(uint8_t *src_ptr, uint16_t src_size, uint16_t dst_offset, uint16_t plane_size);

/* Video buffer operations */
void switch_video_buffer(uint16_t buffer_offset);
uint16_t get_current_display_offset(void);

#endif /* GRAPHICS_H */
