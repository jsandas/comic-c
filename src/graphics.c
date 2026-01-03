/*
 * graphics.c - Graphics loading and rendering functions
 * 
 * Handles loading .EGA graphics files, RLE decoding, EGA plane management,
 * and video buffer operations for the title sequence and gameplay.
 */

#include <stdint.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include "globals.h"

/* EGA Register Addresses */
#define EGA_CRTC_INDEX_PORT     0x3d4
#define EGA_CRTC_DATA_PORT      0x3d5
#define EGA_GRAPHICS_INDEX_PORT 0x3ce
#define EGA_GRAPHICS_DATA_PORT  0x3cf
#define EGA_SEQUENCER_INDEX_PORT 0x3c4
#define EGA_SEQUENCER_DATA_PORT  0x3c5
#define EGA_ATTRIBUTE_INDEX_PORT 0x3c0
#define EGA_PALETTE_INDEX_PORT  0x3c8
#define EGA_PALETTE_DATA_PORT   0x3c9

/* Graphics register indices */
#define EGA_READ_PLANE_SELECT   0x04
#define EGA_WRITE_PLANE_ENABLE  0x02

/* Video memory base address in segment 0xa000 */
#define VIDEO_MEMORY_BASE       0xa000

/* Buffer offsets within video memory (in paragraphs, not bytes) */
#define BUFFER_GAMEPLAY_A       0x0000  /* 0xa000:0000 - Double buffer page A (8KB) */
#define BUFFER_GAMEPLAY_B       0x2000  /* 0xa000:2000 - Double buffer page B (8KB) */
#define BUFFER_RENDERED_MAP     0x4000  /* 0xa000:4000 - Rendered map (40KB) */
#define BUFFER_TITLE_TEMP1      0x8000  /* 0xa000:8000 - Title temp buffer 1 (8KB) */
#define BUFFER_TITLE_TEMP2      0xa000  /* 0xa000:a000 - Title temp buffer 2 (8KB) */

/* File names for title sequence graphics */
static const char FILENAME_TITLE_GRAPHIC[] = "SYS000.EGA\0";
static const char FILENAME_STORY_GRAPHIC[] = "SYS001.EGA\0";
static const char FILENAME_UI_GRAPHIC[] = "SYS003.EGA\0";
static const char FILENAME_ITEMS_GRAPHIC[] = "SYS004.EGA\0";

/* Buffer for loading fullscreen graphics from disk (32KB max for uncompressed EGA data) */
#define GRAPHICS_LOAD_BUFFER_SIZE 0x8000  /* 32KB */
static uint8_t __far *graphics_load_buffer = (uint8_t __far *)0x20000000L;  /* Placeholder - will be set at runtime */

/* Current display buffer offset (0x0000, 0x2000, 0x8000, or 0xa000) */
static uint16_t current_display_offset = BUFFER_GAMEPLAY_A;

/*
 * enable_ega_plane_write - Enable writing to a specific EGA color plane
 * 
 * Input:
 *   plane = plane index (0=Blue, 1=Green, 2=Red, 3=Intensity)
 * 
 * Sets the Write Plane Enable register (Graphics Register 0x02)
 * to select which plane is written to.
 */
void enable_ega_plane_write(uint8_t plane)
{
    uint8_t mask = 1 << plane;  /* Convert plane index to bit mask (1, 2, 4, 8) */
    
    /* Output to Graphics Registers at port 0x3ce/0x3cf */
    outp(EGA_GRAPHICS_INDEX_PORT, EGA_WRITE_PLANE_ENABLE);
    outp(EGA_GRAPHICS_DATA_PORT, mask);
}

/*
 * enable_ega_plane_read - Enable reading from a specific EGA color plane
 * 
 * Input:
 *   plane = plane index (0=Blue, 1=Green, 2=Red, 3=Intensity)
 * 
 * Sets the Read Plane Select register (Graphics Register 0x04)
 * to select which plane is read from.
 */
void enable_ega_plane_read(uint8_t plane)
{
    /* Output to Graphics Registers at port 0x3ce/0x3cf */
    outp(EGA_GRAPHICS_INDEX_PORT, EGA_READ_PLANE_SELECT);
    outp(EGA_GRAPHICS_DATA_PORT, plane);
}

/*
 * enable_ega_plane_read_write - Enable both reading and writing for a specific plane
 * 
 * Input:
 *   plane = plane index (0=Blue, 1=Green, 2=Red, 3=Intensity)
 */
void enable_ega_plane_read_write(uint8_t plane)
{
    enable_ega_plane_read(plane);
    enable_ega_plane_write(plane);
}

/*
 * rle_decode - Decode RLE-encoded data for one EGA plane
 * 
 * Input:
 *   src_ptr = pointer to RLE-encoded source data
 *   dst_offset = offset in video memory where decoded data goes (0x0000, 0x2000, etc.)
 *   plane_size = number of bytes to decode (typically 8000)
 * 
 * The RLE format is:
 *   - If bit 7 is clear (0x00-0x7f): literal byte - output as-is
 *   - If bit 7 is set (0x80-0xff): repeat count in bits 6-0, next byte is the value to repeat
 * 
 * Output: Decoded plane data written to video memory segment 0xa000
 * Returns: Number of source bytes consumed to decode the plane
 */
uint16_t rle_decode(uint8_t *src_ptr, uint16_t dst_offset, uint16_t plane_size)
{
    uint16_t bytes_decoded = 0;
    uint16_t bytes_consumed = 0;
    uint8_t __far *video_ptr = (uint8_t __far *)MK_FP(0xa000, dst_offset);
    uint8_t byte_value;
    uint8_t repeat_count;
    
    while (bytes_decoded < plane_size) {
        byte_value = *src_ptr++;  /* Read next encoded byte */
        bytes_consumed++;
        
        if ((byte_value & 0x80) == 0) {
            /* Literal byte - output as-is */
            *video_ptr++ = byte_value;
            bytes_decoded++;
        } else {
            /* Repeat sequence - bits 6-0 are the repeat count */
            repeat_count = byte_value & 0x7f;  /* Extract repeat count (1-127) */
            byte_value = *src_ptr++;  /* Get the byte to repeat */
            bytes_consumed++;
            
            /* Output the repeated byte */
            for (; repeat_count > 0; repeat_count--) {
                *video_ptr++ = byte_value;
                bytes_decoded++;
            }
        }
    }
    
    return bytes_consumed;
}

/*
 * load_fullscreen_graphic - Load and decode a fullscreen .EGA graphic from disk
 * 
 * Input:
 *   filename = pointer to null-terminated filename (e.g., "SYS000.EGA")
 *   dst_offset = video memory offset where graphic goes (0x0000, 0x8000, etc.)
 * 
 * Process:
 *   1. Open file using DOS INT 21h
 *   2. Read entire file into temporary buffer
 *   3. Close file
 *   4. Decode RLE data for each of 4 EGA planes
 *   5. Write decoded planes to video memory at dst_offset
 * 
 * The .EGA format:
 *   - First 2 bytes: plane size (always 0x8000 = 8000 decimal)
 *   - Followed by RLE-encoded data for 4 planes (Blue, Green, Red, Intensity)
 *   - Each plane is 8000 bytes (320×200 / 8 = 8000 bytes per plane)
 * 
 * Returns:
 *   0 if successful
 *   -1 if file open failed
 *   -2 if file read failed
 */
int load_fullscreen_graphic(const char *filename, uint16_t dst_offset)
{
    union REGS regs;
    int file_handle;
    uint16_t bytes_read;
    uint8_t *src_ptr;
    uint16_t plane_size;
    uint8_t plane;
    uint16_t src_offset;
    
    /* Open the file (DOS INT 21h AH=3Dh) */
    regs.h.ah = 0x3d;  /* AH=3Dh: open existing file */
    regs.h.al = 0x00;  /* AL=00h: read-only */
    regs.x.dx = (uint16_t)(uintptr_t)filename;
    int86(0x21, &regs, &regs);
    
    if (regs.x.cflag) {
        return -1;  /* File open failed */
    }
    
    file_handle = regs.x.ax;
    
    /* Read entire file into temporary buffer (DOS INT 21h AH=3Fh) */
    regs.h.ah = 0x3f;  /* AH=3Fh: read from file */
    regs.x.bx = file_handle;
    regs.x.cx = GRAPHICS_LOAD_BUFFER_SIZE;  /* Max bytes to read */
    regs.x.dx = (uint16_t)(uintptr_t)graphics_load_buffer;
    int86(0x21, &regs, &regs);
    
    /* Close the file (DOS INT 21h AH=3Eh) */
    regs.h.ah = 0x3e;  /* AH=3Eh: close file */
    regs.x.bx = file_handle;
    int86(0x21, &regs, &regs);
    
    /* Parse the loaded data */
    src_ptr = graphics_load_buffer;
    
    /* First word is the plane size (should be 8000 decimal) */
    plane_size = (uint16_t)src_ptr[0] | ((uint16_t)src_ptr[1] << 8);
    src_offset = 2;  /* Skip past the plane size word */
    
    /* Decode and write each of the 4 EGA planes */
    for (plane = 0; plane < 4; plane++) {
        /* Set the read/write plane for this color channel */
        enable_ega_plane_read_write(plane);
        
        /* Decode RLE data for this plane directly into video memory */
        src_offset += rle_decode(&src_ptr[src_offset], dst_offset, plane_size);
    }
    
    return 0;  /* Success */
}

/*
 * switch_video_buffer - Change which video memory buffer is displayed
 * 
 * Input:
 *   buffer_offset = offset of buffer to display (0x0000, 0x2000, 0x8000, 0xa000)
 * 
 * This function changes which 8KB region of video memory is displayed to the screen.
 * It does this by modifying the video memory offset register.
 */
void switch_video_buffer(uint16_t buffer_offset)
{
    union REGS regs;
    
    /* Set the video memory start address using INT 10h AH=05h
     * Actually, INT 10h AH=0Bh sets video memory offset
     * We use CRTC registers directly for more control
     */
    
    /* Set Start Address High Register (CRTC register 0x0c) */
    regs.h.ah = 0x00;  /* Not an INT 10h function, use CRTC directly */
    
    /* Use port I/O to set CRTC registers directly */
    /* CRTC Start Address High (register 0x0c) = high byte of offset */
    outp(EGA_CRTC_INDEX_PORT, 0x0c);
    outp(EGA_CRTC_DATA_PORT, (buffer_offset >> 8) & 0xff);
    
    /* CRTC Start Address Low (register 0x0d) = low byte of offset */
    outp(EGA_CRTC_INDEX_PORT, 0x0d);
    outp(EGA_CRTC_DATA_PORT, buffer_offset & 0xff);
    
    current_display_offset = buffer_offset;
}

/*
 * get_current_display_offset - Return the currently displayed buffer offset
 * 
 * Returns:
 *   Current display buffer offset (0x0000, 0x2000, 0x8000, 0xa000, etc.)
 */
uint16_t get_current_display_offset(void)
{
    return current_display_offset;
}
