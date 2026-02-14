/*
 * graphics.c - Graphics loading and rendering functions
 * 
 * Handles loading .EGA graphics files, RLE decoding, EGA plane management,
 * and video buffer operations for the title sequence and gameplay.
 */

#include <stdint.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include "globals.h"
#include "graphics.h"
#include "sprite_data.h"
#include "timing.h"

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

/* Palette register indices for title sequence fade effects */
#define PALETTE_REG_BACKGROUND  2   /* Background color register */
#define PALETTE_REG_ITEMS       10  /* Items/inventory color register */
#define PALETTE_REG_TITLE       12  /* Title text color register */

/* Palette color values for fade effects */
#define PALETTE_COLOR_DARK_GRAY   0x18  /* Dark gray for fade start */

/* Video memory base address in segment 0xa000 */
#define VIDEO_MEMORY_BASE       0xa000

/* Screen dimensions (both SCREEN_WIDTH and SCREEN_HEIGHT are defined in globals.h) */

/* Buffer for loading fullscreen graphics from disk (32KB max for uncompressed EGA data) */
#define GRAPHICS_LOAD_BUFFER_SIZE 0x8000  /* 32KB */
static uint8_t graphics_load_buffer[GRAPHICS_LOAD_BUFFER_SIZE];  /* Statically allocated buffer in data segment */

/* Current display buffer offset (0x0000, 0x2000, 0x8000, or 0xa000) */
static uint16_t current_display_offset = GRAPHICS_BUFFER_GAMEPLAY_A;

/* Forward declaration for static helper function */
static void set_palette_register(uint8_t index, uint8_t color);

/* Inventory item flags (defined in game_main.c) */
extern uint8_t comic_has_door_key;
extern uint8_t comic_has_teleport_wand;
extern uint8_t comic_has_corkscrew;
extern uint8_t comic_has_lantern;
extern uint8_t comic_has_gems;         /* 1 if Gems collected, 0 otherwise */
extern uint8_t comic_has_crown;        /* 1 if Crown collected, 0 otherwise */
extern uint8_t comic_has_gold;         /* 1 if Gold collected, 0 otherwise */
extern uint8_t comic_firepower;        /* Number of active fireball slots (controls Blastola Cola inventory display) */
extern uint8_t comic_num_treasures;    /* Number of treasures collected (0-3, used for win/victory logic) */
extern uint8_t comic_jump_power;       /* Jump power level (controls Boots display at >4) */

/* Score data (defined in game_main.c) */
extern uint8_t score_bytes[3];  /* 3-byte score in base-100 representation */

/*
 * init_ega_graphics - Initialize EGA graphics controller for pixel writing
 * 
 * Sets the Graphics Controller to write mode 0 (CPU data replicated to all bits),
 * which works with the Map Mask to write individual planes.
 */
void init_ega_graphics(void)
{
    /* Set Graphics Controller Write Mode to 0 */
    outp(EGA_GRAPHICS_INDEX_PORT, 0x05);  /* GC Register 5: Graphics Mode */
    outp(EGA_GRAPHICS_DATA_PORT, 0x00);   /* Bits 0-1: Write Mode 0, Bit 3: Read Mode 0 */
}

/*
 * enable_ega_plane_write - Enable writing to a specific EGA color plane
 * 
 * Input:
 *   plane = plane index (0=Blue, 1=Green, 2=Red, 3=Intensity)
 * 
 * Sets the Sequencer Map Mask register (SC Register 0x02) to select which
 * plane is written to. Uses ports 0x3c4 (index) and 0x3c5 (data).
 */
void enable_ega_plane_write(uint8_t plane)
{
    uint8_t mask;
    
    /* Compute plane mask: set bit N for plane N (0-3) */
    if (plane < 4) {
        mask = 1 << plane;  /* bit shift: 1 << 0 = 0x01, 1 << 1 = 0x02, etc. */
    } else {
        return;  /* Invalid plane */
    }
    
    /* Output to Sequencer Registers at port 0x3c4/0x3c5 */
    outp(EGA_SEQUENCER_INDEX_PORT, 0x02);  /* SC Map Mask register */
    outp(EGA_SEQUENCER_DATA_PORT, mask);
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
 *   src_size = total number of bytes available in source buffer
 *   dst_offset = offset in video memory where decoded data goes (0x0000, 0x2000, etc.)
 *   plane_size = number of bytes to decode (typically 8000)
 * 
 * The RLE format is:
 *   - If control byte < 0x80 (0-127): Literal copy mode
 *     * Control byte value = number of literal bytes to follow
 *     * Followed by that many literal bytes to copy as-is
 *   - If control byte >= 0x80 (128-255): Repeat mode
 *     * Repeat count = (control byte - 127), giving range 1-128
 *     * Followed by 1 byte that is repeated that many times
 * 
 * Output: Decoded plane data written to video memory segment 0xa000
 * Returns: Number of source bytes consumed to decode the plane
 * 
 * Safety: Validates source buffer bounds to prevent reading beyond truncated files.
 * If source data ends prematurely, stops decoding and returns bytes consumed.
 */
uint16_t rle_decode(uint8_t *src_ptr, uint16_t src_size, uint16_t dst_offset, uint16_t plane_size)
{
    uint16_t bytes_decoded = 0;
    uint16_t bytes_consumed = 0;
    uint16_t remaining_space;
    uint8_t __far *video_ptr = (uint8_t __far *)MK_FP(0xa000, dst_offset);
    uint8_t control_byte;
    uint8_t byte_value;
    uint8_t repeat_count;
    uint8_t literal_count;
    uint8_t i;
    
    while (bytes_decoded < plane_size) {
        /* Validate we have at least one byte available for the control byte */
        if (bytes_consumed >= src_size) {
            break;  /* Source buffer exhausted, stop decoding */
        }
        
        control_byte = *src_ptr++;  /* Read next control byte */
        bytes_consumed++;
        
        if (control_byte < 0x80) {
            /* Literal copy mode: control byte is count of bytes to copy */
            literal_count = control_byte;
            
            /* Validate remaining destination space */
            remaining_space = plane_size - bytes_decoded;
            if (literal_count > remaining_space) {
                literal_count = remaining_space;  /* Clamp to available space */
            }
            
            /* Copy literal bytes */
            for (i = 0; i < literal_count; i++) {
                /* Validate we have source data available */
                if (bytes_consumed >= src_size) {
                    break;  /* Source exhausted, stop decoding */
                }
                
                byte_value = *src_ptr++;  /* Read literal byte */
                bytes_consumed++;
                *video_ptr++ = byte_value;
                bytes_decoded++;
            }
        } else {
            /* Repeat mode: control byte minus 128 is the repeat count */
            repeat_count = control_byte - 128;  /* Repeat count (0-127) */
            
            /* Validate we have at least one byte available for the repeat value */
            if (bytes_consumed >= src_size) {
                break;  /* Source buffer exhausted, stop decoding */
            }
            
            byte_value = *src_ptr++;  /* Get the byte to repeat */
            bytes_consumed++;
            
            /* A repeat count of 0 is a no-op, but we still consumed the value byte
             * to maintain stream synchronization. This shouldn't occur in actual
             * .EGA files as it wastes 2 bytes (0x80 control + value byte). */
            if (repeat_count == 0) {
                continue;
            }
            
            /* Validate that repeat won't exceed plane boundary */
            remaining_space = plane_size - bytes_decoded;
            if (repeat_count > remaining_space) {
                repeat_count = remaining_space;  /* Clamp to available space */
            }
            
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
 *   - First 2 bytes: plane size (always 8000 = 0x1F40 hexadecimal)
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
    struct SREGS sregs;
    int file_handle;
    uint16_t bytes_read;
    uint8_t *src_ptr;
    uint16_t plane_size;
    uint8_t plane;
    uint16_t src_offset;
    uint16_t remaining_src_bytes;
    
    /* Open the file (DOS INT 21h AH=3Dh) */
    /* Must set DS:DX to point to filename string for large memory model */
    regs.h.ah = 0x3d;  /* AH=3Dh: open existing file */
    regs.h.al = 0x00;  /* AL=00h: read-only */
    regs.x.dx = FP_OFF(filename);  /* DX = offset of filename */
    sregs.ds = FP_SEG(filename);   /* DS = segment of filename */
    int86x(0x21, &regs, &regs, &sregs);
    
    if (regs.x.cflag) {
        fprintf(stderr, "ERROR: Failed to open file '%s'\n", filename);
        return -1;  /* File open failed */
    }
    
    file_handle = regs.x.ax;
    
    /* Read entire file into temporary buffer (DOS INT 21h AH=3Fh) */
    /* Must set DS:DX to point to buffer for large memory model */
    regs.h.ah = 0x3f;  /* AH=3Fh: read from file */
    regs.x.bx = file_handle;
    regs.x.cx = GRAPHICS_LOAD_BUFFER_SIZE;  /* Max bytes to read */
    regs.x.dx = FP_OFF(graphics_load_buffer);  /* DX = offset of buffer */
    sregs.ds = FP_SEG(graphics_load_buffer);   /* DS = segment of buffer */
    int86x(0x21, &regs, &regs, &sregs);

    /* Check for read error (carry flag set) */
    if (regs.x.cflag) {
        fprintf(stderr, "ERROR: Failed to read file '%s'\n", filename);
        /* Close the file before returning */
        regs.h.ah = 0x3e;  /* AH=3Eh: close file */
        regs.x.bx = file_handle;
        int86(0x21, &regs, &regs);
        return -2;  /* File read failed (per documentation) */
    }

    /* Save number of bytes actually read (AX) */
    bytes_read = regs.x.ax;
    
    /* Close the file (DOS INT 21h AH=3Eh) */
    regs.h.ah = 0x3e;  /* AH=3Eh: close file */
    regs.x.bx = file_handle;
    int86(0x21, &regs, &regs);
    
    /* Validate file has at least the 2-byte header */
    if (bytes_read < 2) {
        fprintf(stderr, "ERROR: File '%s' too small (%u bytes) - invalid format\n", filename, bytes_read);
        return -2;  /* File too small - invalid format */
    }
    
    /* Parse the loaded data */
    src_ptr = graphics_load_buffer;
    
    /* First word is the plane size (must be 8000 decimal = 0x1F40 hex) */
    plane_size = (uint16_t)src_ptr[0] | ((uint16_t)src_ptr[1] << 8);
    
    /* Validate plane size matches EGA fullscreen format specification */
    if (plane_size != 8000) {
        fprintf(stderr, "ERROR: Invalid plane size in '%s' (%u bytes, expected 8000)\n", filename, plane_size);
        return -2;  /* Invalid plane size - corrupted or wrong format */
    }
    
    src_offset = 2;  /* Skip past the plane size word */
    
    /* Clear the destination buffer to black (zeros) before decoding */
    /* This is important to ensure old video data doesn't show through */
    {
        uint8_t plane_clear;
        uint16_t i;
        uint8_t __far *video_ptr;
        
        /* Clear all 4 planes */
        for (plane_clear = 0; plane_clear < 4; plane_clear++) {
            enable_ega_plane_write(plane_clear);
            video_ptr = (uint8_t __far *)MK_FP(0xa000, dst_offset);
            
            /* Clear 8000 bytes (one full plane) with zeros */
            for (i = 0; i < 8000; i++) {
                *video_ptr++ = 0x00;
            }
        }
    }
    
    /* Decode and write each of the 4 EGA planes */
    /* Standard BGRI order: Blue(0), Green(1), Red(2), Intensity(3) */
    for (plane = 0; plane < 4; plane++) {
        /* Set the write plane for this color channel */
        enable_ega_plane_write(plane);
        
        /* Decode RLE data for this plane directly into video memory */
        /* Calculate remaining bytes in source buffer for this plane */
        if (bytes_read <= src_offset) {
            /* File truncated before all planes could be decoded */
            fprintf(stderr, "ERROR: File '%s' truncated at plane %u - insufficient data\n", filename, plane);
            return -2;  /* File read failed - insufficient data */
        }
        remaining_src_bytes = bytes_read - src_offset;
        src_offset += rle_decode(&src_ptr[src_offset], remaining_src_bytes, dst_offset, plane_size);
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
void load_ega_palette_from_file(const uint8_t *palette16)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        set_palette_register(i, palette16[i]);
    }
}


void switch_video_buffer(uint16_t buffer_offset)
{
    /* Set the video memory start address by writing to CRTC registers.
     * The display start address register controls which 8KB buffer is visible.
     * High byte (register 0x0c) and low byte (register 0x0d) form a 16-bit offset
     * into video memory (0xa000 segment) that points to the first byte displayed.
     */
    
    /* Write to CRTC Start Address High Register (0x0c) */
    outp(EGA_CRTC_INDEX_PORT, 0x0c);
    outp(EGA_CRTC_DATA_PORT, (buffer_offset >> 8) & 0xff);
    
    /* Write to CRTC Start Address Low Register (0x0d) */
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

/*
 * init_default_palette - Initialize EGA palette registers to default EGA values
 * 
 * Initializes all 16 palette registers (0-15) with their standard EGA colors.
 * Uses INT 10h AH=10h AL=00h to set individual palette registers.
 * Each palette register maps to a BIOS color index (0-63), which in turn maps
 * to actual RGB values in the DAC (Digital-to-Analog Converter).
 * 
 * Palette register initialization (BIOS color indices):
 * 
 * Low-intensity colors (indices 0-7) map directly to themselves:
 *   0: Black (index 0)           8: Dark Gray (index 0x38)
 *   1: Blue (index 1)            9: Light Blue (index 0x39)
 *   2: Green (index 2)           10: Light Green (index 0x3a)
 *   3: Cyan (index 3)            11: Light Cyan (index 0x3b)
 *   4: Red (index 4)             12: Light Red (index 0x3c)
 *   5: Magenta (index 5)         13: Light Magenta (index 0x3d)
 *   6: Brown (index 6)           14: Yellow (index 0x3e)
 *   7: Light Gray (index 7)      15: White (index 0x3f)
 * 
 * The actual RGB values are determined by the BIOS default DAC palette:
 *   Low-intensity (0x00-0x07): RGB with 0-2 intensity per channel
 *   High-intensity (0x38-0x3f): RGB with 2-3 intensity per channel
 */
void init_default_palette(void)
{
    union REGS regs;
    uint8_t palette[16] = {
        0x00,  /* 0: Black */
        0x01,  /* 1: Blue */
        0x02,  /* 2: Green */
        0x03,  /* 3: Cyan */
        0x04,  /* 4: Red */
        0x05,  /* 5: Magenta */
        0x06,  /* 6: Brown */
        0x07,  /* 7: Light Gray */
        0x38,  /* 8: Dark Gray */
        0x39,  /* 9: Light Blue */
        0x3a,  /* 10: Light Green */
        0x3b,  /* 11: Light Cyan */
        0x3c,  /* 12: Light Red */
        0x3d,  /* 13: Light Magenta */
        0x3e,  /* 14: Yellow */
        0x3f   /* 15: White */
    };
    uint8_t i;
    
    /* Set each palette register individually using INT 10h AH=10h AL=00h */
    for (i = 0; i < 16; i++) {
        regs.h.ah = 0x10;   /* AH=10h: Palette functions */
        regs.h.al = 0x00;   /* AL=00h: Set individual palette register */
        regs.h.bl = i;      /* BL = color index to set */
        regs.h.bh = palette[i]; /* BH = palette register value */
        int86(0x10, &regs, &regs);
    }
}

/*
 * set_palette_register - Set a single EGA palette register to a specific color
 * 
 * Input:
 *   index = palette register index (0-15)
 *   color = color value (0x00-0x3f)
 * 
 * Uses BIOS INT 10h AH=10h AL=00h to set an individual palette register.
 */
static void set_palette_register(uint8_t index, uint8_t color)
{
    union REGS regs;
    
    regs.h.ah = 0x10;  /* AH=10h: Palette functions */
    regs.h.al = 0x00;  /* AL=00h: Set individual palette register */
    regs.h.bl = index; /* BL = palette register index */
    regs.h.bh = color; /* BH = color value */
    int86(0x10, &regs, &regs);
}

/*
 * palette_darken - Set specific palette entries to dark gray for fade effect
 * 
 * Sets palette entries 2, 10, and 12 to dark gray (0x18) to prepare for
 * fade-in animation. This is called before displaying a new title screen.
 * 
 * Uses BIOS INT 10h AH=10h AL=00h to set individual palette registers.
 * BL = palette register index, BH = color value
 */
void palette_darken(void)
{
    set_palette_register(PALETTE_REG_BACKGROUND, PALETTE_COLOR_DARK_GRAY);
    set_palette_register(PALETTE_REG_ITEMS, PALETTE_COLOR_DARK_GRAY);
    set_palette_register(PALETTE_REG_TITLE, PALETTE_COLOR_DARK_GRAY);
}

/*
 * palette_fade_in - Fade in from dark to final colors in 5 steps
 * 
 * Performs a 5-step fade animation for palette registers 2, 10, and 12.
 * Uses 6-bit color values (0x00-0x3f) to smoothly transition from dark gray
 * to the final display colors. Based on the original assembly implementation.
 * 
 * This creates a smooth fade-in effect for title sequence screens.
 * Uses wait_n_ticks() from timing.h for timing between steps.
 */
void palette_fade_in(void)
{
    /* Step 1: Set all three to dark gray to start the fade from black */
    set_palette_register(PALETTE_REG_BACKGROUND, 0x18);  /* dark gray */
    set_palette_register(PALETTE_REG_ITEMS, 0x18);       /* dark gray */
    set_palette_register(PALETTE_REG_TITLE, 0x18);       /* dark gray */
    wait_n_ticks(1);
    
    /* Step 2: Set all three to light gray (6-bit color value 0x07) */
    set_palette_register(PALETTE_REG_BACKGROUND, 0x07);  /* light gray */
    set_palette_register(PALETTE_REG_ITEMS, 0x07);       /* light gray */
    set_palette_register(PALETTE_REG_TITLE, 0x07);       /* light gray */
    wait_n_ticks(1);
    
    /* Step 3: Set registers ITEMS and TITLE to white (0x1f), keep BACKGROUND at gray */
    set_palette_register(PALETTE_REG_BACKGROUND, 0x07);  /* stay gray */
    set_palette_register(PALETTE_REG_ITEMS, 0x1f);       /* white */
    set_palette_register(PALETTE_REG_TITLE, 0x1f);       /* white */
    wait_n_ticks(1);
    
    /* Step 4: Set BACKGROUND to green, ITEMS to bright green, keep TITLE at white */
    set_palette_register(PALETTE_REG_BACKGROUND, 0x02);  /* green (6-bit: 000010) */
    set_palette_register(PALETTE_REG_ITEMS, 0x1a);       /* bright green (6-bit: 011010) */
    set_palette_register(PALETTE_REG_TITLE, 0x1f);       /* stay white */
    wait_n_ticks(1);
    
    /* Step 5: Set TITLE to bright red (6-bit: 011100) */
    set_palette_register(PALETTE_REG_TITLE, 0x1c);       /* bright red */
    wait_n_ticks(1);
}

/*
 * copy_ega_plane - Copy video memory from one buffer to another
 * 
 * Copies data within EGA video memory from source to destination offset.
 * Handles all 4 EGA planes independently. Used primarily to duplicate the
 * UI buffer from GRAPHICS_BUFFER_GAMEPLAY_A to GRAPHICS_BUFFER_GAMEPLAY_B.
 * 
 * Parameters:
 *   src_offset - Source offset within video segment 0xa000
 *   dst_offset - Destination offset within video segment 0xa000
 *   num_bytes  - Number of bytes to copy per plane
 * 
 * Note: Must iterate through planes because EGA planar memory doesn't
 *       allow simultaneous access to all planes.
 */
void copy_ega_plane(uint16_t src_offset, uint16_t dst_offset, uint16_t num_bytes)
{
    uint8_t plane;
    uint16_t i;
    uint8_t __far *src_ptr;
    uint8_t __far *dst_ptr;
    
    /* Copy each of the 4 EGA planes independently */
    for (plane = 0; plane < 4; plane++) {
        /* Select plane for both reading and writing */
        enable_ega_plane_read_write(plane);
        
        /* Set up far pointers to video memory using MK_FP */
        src_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, src_offset);
        dst_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, dst_offset);
        
        /* Copy bytes for this plane */
        for (i = 0; i < num_bytes; i++) {
            dst_ptr[i] = src_ptr[i];
        }
    }
}

/* copy_plane_bytes - Copy a small number of bytes within a single EGA plane
 * This is a helper that avoids relying on far-pointer casting inside other
 * modules. It assumes the caller has already selected the appropriate plane
 * for reading/writing. Offsets are relative to segment 0xa000. */
void copy_plane_bytes(uint16_t src_offset, uint16_t dst_offset, uint16_t num_bytes)
{
    uint8_t __far *src_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, src_offset);
    uint8_t __far *dst_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, dst_offset);
    uint16_t i;

    for (i = 0; i < num_bytes; i++) {
        dst_ptr[i] = src_ptr[i];
    }
}

/*
 * blit_sprite_16x16_masked - Blit a 16x16 masked EGA sprite to video memory
 * 
 * Blits a 16x16 sprite with mask to both gameplay buffers (0x0000 and 0x2000)
 * at the specified pixel coordinate. The sprite is in EGA planar format.
 * Works with both embedded sprites and runtime-loaded .SHP sprites.
 * 
 * Input:
 *   pixel_x = X coordinate in pixels (0-319)
 *   pixel_y = Y coordinate in pixels (0-199)
 *   sprite_data = pointer to 160-byte sprite data
 *                 (32 bytes per plane × 4 planes + 32 bytes mask)
 * 
 * Sprite format (160 bytes):
 *   Bytes 0-31:     Blue plane (2 bytes/row × 16 rows)
 *   Bytes 32-63:    Green plane
 *   Bytes 64-95:    Red plane
 *   Bytes 96-127:   Intensity plane
 *   Bytes 128-159:  Mask (bit-packed: bit=1 transparent, bit=0 opaque)
 */
void blit_sprite_16x16_masked(uint16_t pixel_x, uint16_t pixel_y, const uint8_t *sprite_data)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t col;
    uint8_t sprite_byte;
    uint8_t mask_byte;
    uint8_t current_a;
    uint8_t current_b;
    uint16_t video_offset;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    const uint8_t *plane_data;
    const uint8_t *mask_data;
    uint16_t plane_offset;
    
    /* Calculate video memory offset for top-left corner of sprite */
    base_offset = (pixel_y * 320 + pixel_x) / 8;
    
    /* Mask data starts at byte 128 of the sprite */
    mask_data = sprite_data + 128;
    
    /* Blit each of the 4 color planes */
    for (plane = 0; plane < 4; plane++) {
        /* Enable BOTH reading and writing to this plane (needed for masked blitting) */
        enable_ega_plane_read_write(plane);
        
        /* Plane data: Blue=0, Green=32, Red=64, Intensity=96 bytes offset */
        plane_offset = plane * 32;
        plane_data = sprite_data + plane_offset;
        
        /* Copy this plane to both buffers, 2 bytes per row (16 pixels wide) */
        for (row = 0; row < 16; row++) {
            /* Calculate video offset for this row */
            video_offset = base_offset + (row * 40);
            
            /* Get pointers to both video buffers for this row */
            video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + video_offset);
            video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + video_offset);
            
            /* Copy 2 bytes per row (16 pixels in EGA format = 2 bytes) */
            for (col = 0; col < 2; col++) {
                sprite_byte = plane_data[row * 2 + col];
                mask_byte = mask_data[row * 2 + col];
                
                /* Read current video memory */
                current_a = video_ptr_a[col];
                current_b = video_ptr_b[col];
                
                /* Apply masking: bit=1 keeps background, bit=0 writes sprite */
                video_ptr_a[col] = (current_a & mask_byte) | (sprite_byte & ~mask_byte);
                video_ptr_b[col] = (current_b & mask_byte) | (sprite_byte & ~mask_byte);
            }
        }
    }
}


/*
 * blit_sprite_16x16_unmasked - Blit a 16x16 unmasked EGA sprite to video memory
 * 
 * Blits a 16x16 sprite without mask to both gameplay buffers (0x0000 and 0x2000)
 * at the specified pixel coordinate. Completely overwrites the destination area.
 * The sprite is in EGA planar format (128 bytes: 32 bytes per plane × 4 planes).
 * 
 * Input:
 *   pixel_x = X coordinate in pixels (0-319)
 *   pixel_y = Y coordinate in pixels (0-199)
 *   sprite_data = pointer to 128-byte sprite data (32 bytes per plane × 4 planes)
 * 
 * Sprite format (128 bytes):
 *   Bytes 0-31:     Blue plane (2 bytes/row × 16 rows)
 *   Bytes 32-63:    Green plane
 *   Bytes 64-95:    Red plane
 *   Bytes 96-127:   Intensity plane
 */
void blit_sprite_16x16_unmasked(uint16_t pixel_x, uint16_t pixel_y, const uint8_t *sprite_data)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t col;
    uint16_t video_offset;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    const uint8_t *plane_data;
    uint16_t plane_offset;
    
    /* Calculate video memory offset for top-left corner of sprite */
    base_offset = (pixel_y * 320 + pixel_x) / 8;
    
    /* Blit each of the 4 color planes */
    for (plane = 0; plane < 4; plane++) {
        /* Enable writing to this plane only (no need to read for unmasked) */
        enable_ega_plane_write(plane);
        
        /* Plane data: Blue=0, Green=32, Red=64, Intensity=96 bytes offset */
        plane_offset = plane * 32;
        plane_data = sprite_data + plane_offset;
        
        /* Copy this plane to both buffers, 2 bytes per row (16 pixels wide) */
        for (row = 0; row < 16; row++) {
            /* Calculate video offset for this row */
            video_offset = base_offset + (row * 40);
            
            /* Get pointers to both video buffers for this row */
            video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + video_offset);
            video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + video_offset);
            
            /* Copy 2 bytes per row (16 pixels in EGA format = 2 bytes) - direct overwrite */
            for (col = 0; col < 2; col++) {
                video_ptr_a[col] = plane_data[row * 2 + col];
                video_ptr_b[col] = plane_data[row * 2 + col];
            }
        }
    }
}


/*
 * blit_sprite_16x32_masked - Blit a 16x32 masked EGA sprite to video memory
 *
 * Sprite format (320 bytes):
 *   Bytes 0-63:     Blue plane (2 bytes/row × 32 rows = 64 bytes)
 *   Bytes 64-127:   Green plane
 *   Bytes 128-191:  Red plane
 *   Bytes 192-255:  Intensity plane
 *   Bytes 256-319:  Mask (2 bytes/row × 32 rows = 64 bytes)
 */
void blit_sprite_16x32_masked(uint16_t pixel_x, uint16_t pixel_y, const uint8_t *sprite_data)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t rows_to_draw;
    const uint8_t *plane_base;
    const uint8_t *mask_base;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    uint16_t screen_row_bytes;
    uint8_t b0;
    uint8_t b1;
    uint8_t m0;
    uint8_t m1;

    /* Validate sprite doesn't exceed screen bounds
     * Screen is 320×200; sprite is 16×32
     * Sprite extends from (pixel_x, pixel_y) to (pixel_x+15, pixel_y+31) */
    
    /* Check horizontal bounds: pixel_x must fit within screen and sprite can't overflow */
    if (pixel_x >= SCREEN_WIDTH || pixel_x + 16 > SCREEN_WIDTH) {
        return;  /* Sprite entirely or partially off-screen horizontally */
    }
    
    /* Check vertical bounds: pixel_y must be within screen */
    if (pixel_y >= SCREEN_HEIGHT) {
        return;  /* Sprite entirely off-screen vertically (below bottom) */
    }
    
    /* Calculate how many rows we can safely draw
     * If sprite extends past screen bottom, limit rows to fit within screen */
    if (pixel_y + 32 > SCREEN_HEIGHT) {
        rows_to_draw = SCREEN_HEIGHT - pixel_y;  /* Clamp to remaining screen height */
    } else {
        rows_to_draw = 32;  /* Full sprite fits */
    }

    base_offset = (pixel_y * 320 + pixel_x) / 8;
    mask_base = sprite_data + 256;
    screen_row_bytes = SCREEN_WIDTH / 8; /* 40 */

    /* For each plane, copy rows to both buffers */
    for (plane = 0; plane < 4; plane++) {
        plane_base = sprite_data + plane * 64; /* 64 bytes per plane */

        /* Start at top row for both buffers */
        video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + base_offset);
        video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + base_offset);

        /* Enable plane for both reading and writing */
        enable_ega_plane_read_write(plane);

        for (row = 0; row < rows_to_draw; row++) {
            /* Two bytes per row */
            b0 = plane_base[row * 2 + 0];
            b1 = plane_base[row * 2 + 1];
            m0 = mask_base[row * 2 + 0];
            m1 = mask_base[row * 2 + 1];

            video_ptr_a[0] = (video_ptr_a[0] & m0) | (b0 & ~m0);
            video_ptr_a[1] = (video_ptr_a[1] & m1) | (b1 & ~m1);

            video_ptr_b[0] = (video_ptr_b[0] & m0) | (b0 & ~m0);
            video_ptr_b[1] = (video_ptr_b[1] & m1) | (b1 & ~m1);

            /* Advance to next screen row (40 bytes) */
            video_ptr_a += screen_row_bytes;
            video_ptr_b += screen_row_bytes;
        }
    }
}

/*
 * blit_sprite_16x32_masked_rows - Blit only the top N rows of a 16x32 masked EGA sprite
 *
 * Sprite format (320 bytes):
 *   Bytes 0-63:     Blue plane (2 bytes/row × 32 rows = 64 bytes)
 *   Bytes 64-127:   Green plane
 *   Bytes 128-191:  Red plane
 *   Bytes 192-255:  Intensity plane
 *   Bytes 256-319:  Mask (2 bytes/row × 32 rows = 64 bytes)
 *
 * Input:
 *   pixel_x, pixel_y: top-left position in pixels
 *   sprite_data: pointer to 320-byte sprite data (masked)
 *   rows: number of rows to draw (1-32), top-origin
 *
 * Behavior:
 *   - Clamps rows to 1..32 and to available screen space (bottom edge)
 *   - Draws only the first 'rows' rows of the sprite, using mask combine
 *   - Writes to both gameplay buffers (A and B) to keep them in sync
 */
void blit_sprite_16x32_masked_rows(uint16_t pixel_x, uint16_t pixel_y, const uint8_t *sprite_data, uint8_t rows)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t rows_to_draw;
    const uint8_t *plane_base;
    const uint8_t *mask_base;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    uint16_t screen_row_bytes;
    uint8_t b0;
    uint8_t b1;
    uint8_t m0;
    uint8_t m1;

    /* Horizontal bounds: sprite is 16px wide and must not overflow */
    if (pixel_x >= SCREEN_WIDTH || pixel_x + 16 > SCREEN_WIDTH) {
        return;
    }

    /* Vertical bounds: top must be within screen */
    if (pixel_y >= SCREEN_HEIGHT) {
        return;
    }

    /* Clamp rows to 1..32 */
    if (rows == 0) {
        return;
    }
    if (rows > 32) {
        rows = 32;
    }

    /* Also clamp to available screen space to avoid bottom overflow */
    if ((uint16_t)(pixel_y + rows) > SCREEN_HEIGHT) {
        rows_to_draw = (uint8_t)(SCREEN_HEIGHT - pixel_y);
    } else {
        rows_to_draw = rows;
    }
    if (rows_to_draw == 0) {
        return;
    }

    base_offset = (uint16_t)((pixel_y * 320 + pixel_x) / 8);
    mask_base = sprite_data + 256;
    screen_row_bytes = SCREEN_WIDTH / 8; /* 40 */

    /* For each plane, copy rows to both buffers */
    for (plane = 0; plane < 4; plane++) {
        plane_base = sprite_data + plane * 64; /* 64 bytes per plane */

        /* Start at top row for both buffers */
        video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + base_offset);
        video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + base_offset);

        /* Enable plane for both reading and writing */
        enable_ega_plane_read_write(plane);

        for (row = 0; row < rows_to_draw; row++) {
            /* Two bytes per row */
            b0 = plane_base[row * 2 + 0];
            b1 = plane_base[row * 2 + 1];
            m0 = mask_base[row * 2 + 0];
            m1 = mask_base[row * 2 + 1];

            video_ptr_a[0] = (video_ptr_a[0] & m0) | (b0 & ~m0);
            video_ptr_a[1] = (video_ptr_a[1] & m1) | (b1 & ~m1);

            video_ptr_b[0] = (video_ptr_b[0] & m0) | (b0 & ~m0);
            video_ptr_b[1] = (video_ptr_b[1] & m1) | (b1 & ~m1);

            /* Advance to next screen row (40 bytes) */
            video_ptr_a += screen_row_bytes;
            video_ptr_b += screen_row_bytes;
        }
    }
}
/*
 * blit_sprite_16x8_masked - Blit a 16x8 masked EGA sprite to video memory
 *
 * Sprite format (80 bytes):
 *   Bytes 0-15:    Blue plane (2 bytes/row × 8 rows)
 *   Bytes 16-31:   Green plane
 *   Bytes 32-47:   Red plane
 *   Bytes 48-63:   Intensity plane
 *   Bytes 64-79:   Mask (2 bytes/row × 8 rows)
 */
void blit_sprite_16x8_masked(uint16_t pixel_x, uint16_t pixel_y, const uint8_t *sprite_data)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t col;
    uint8_t sprite_byte;
    uint8_t mask_byte;
    uint8_t current_a;
    uint8_t current_b;
    uint16_t video_offset;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    const uint8_t *plane_data;
    const uint8_t *mask_data;
    uint16_t plane_offset;

    base_offset = (pixel_y * 320 + pixel_x) / 8;
    mask_data = sprite_data + 64;

    for (plane = 0; plane < 4; plane++) {
        enable_ega_plane_read_write(plane);

        plane_offset = (uint16_t)(plane * 16);
        plane_data = sprite_data + plane_offset;

        for (row = 0; row < 8; row++) {
            video_offset = base_offset + (row * 40);

            video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + video_offset);
            video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + video_offset);

            for (col = 0; col < 2; col++) {
                sprite_byte = plane_data[row * 2 + col];
                mask_byte = mask_data[row * 2 + col];

                current_a = video_ptr_a[col];
                current_b = video_ptr_b[col];

                video_ptr_a[col] = (current_a & mask_byte) | (sprite_byte & ~mask_byte);
                video_ptr_b[col] = (current_b & mask_byte) | (sprite_byte & ~mask_byte);
            }
        }
    }
}

/*
 * blit_wxh - Blit a variable-size graphic to video memory
 * 
 * Renders a graphic of arbitrary width and height to video memory at the
 * specified offset within the video segment. This function writes to the
 * buffer offset provided (which should include any double-buffering offset).
 * 
 * The graphic data is organized as 4 planes (blue, green, red, intensity),
 * with each plane containing width_bytes * height bytes.
 * 
 * Input:
 *   dest_offset = offset within video memory segment 0xA000 (including buffer offset)
 *   graphic = pointer to graphic data (4 planes sequentially)
 *   width_bytes = width in bytes (pixels/8)
 *   height = height in pixels
 */
void blit_wxh(uint16_t dest_offset, const uint8_t __far *graphic, uint16_t width_bytes, uint16_t height)
{
    uint16_t plane_size = width_bytes * height;
    uint16_t row, col;
    const uint8_t __far *plane_ptr;
    uint8_t __far *dest_ptr;
    uint8_t plane;
    uint8_t plane_mask;
    
    /* Blit each plane */
    for (plane = 0; plane < 4; plane++) {
        plane_mask = 1 << plane;
        
        /* Select write plane */
        outp(0x3C4, 0x02);  /* Map Mask register */
        outp(0x3C5, plane_mask);
        
        /* Point to this plane's data */
        plane_ptr = graphic + (plane * plane_size);
        
        /* Blit all rows */
        for (row = 0; row < height; row++) {
            dest_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, dest_offset + (row * 40));
            for (col = 0; col < width_bytes; col++) {
                *dest_ptr++ = *plane_ptr++;
            }
        }
    }
}


/*
 * render_inventory_display - Render inventory items on the game UI
 *
 * Displays all inventory items that Comic has collected on the UI background.
 * Items are rendered at their fixed UI positions (from the original assembly).
 *
 * Inventory positions (pixel coordinates):
 *   Row 1 (Y=112):
 *     Blastola Cola: (232, 112) - shown if comic_firepower > 0
 *     Corkscrew:    (256, 112) - shown if comic_has_corkscrew = 1
 *     Door Key:     (280, 112) - shown if comic_has_door_key = 1
 *
 *   Row 2 (Y=136):
 *     Boots:        (232, 136) - shown if comic_jump_power > 4
 *     Lantern:      (256, 136) - shown if comic_has_lantern = 1
 *     Teleport Wand:(280, 136) - shown if comic_has_teleport_wand = 1
 *
 *   Row 3 (Y=160):
 *     Gems:         (232, 160) - shown if comic_num_treasures > 0
 *     Crown:        (256, 160) - shown if comic_num_treasures > 1
 *     Gold:         (280, 160) - shown if comic_num_treasures > 2
 *
 * All inventory items are 16x16 sprites with even/odd plane data.
 * Items are only rendered if Comic has collected them.
 */
void render_inventory_display(void)
{
    /* Row 1 (Y=112) */
    /* Always render Blastola Cola to keep both buffers in sync (prevents blinking) */
    if (comic_firepower > 0) {
        /* Render Blastola Cola at (232, 112) - sprite varies based on firepower level
         * Use unmasked blit to completely overwrite old sprite (no transparency) */
        const uint8_t *blastola_sprite = NULL;
        switch (comic_firepower) {
            case 1:
                blastola_sprite = sprite_blastola_cola_inventory_1_even_16x16m;
                break;
            case 2:
                blastola_sprite = sprite_blastola_cola_inventory_2_even_16x16m;
                break;
            case 3:
                blastola_sprite = sprite_blastola_cola_inventory_3_even_16x16m;
                break;
            case 4:
                blastola_sprite = sprite_blastola_cola_inventory_4_even_16x16m;
                break;
            case 5:
                blastola_sprite = sprite_blastola_cola_inventory_5_even_16x16m;
                break;
            default:
                blastola_sprite = sprite_blastola_cola_even_16x16m;
                break;
        }
        blit_sprite_16x16_unmasked(232, 112, blastola_sprite);
    }
    
    if (comic_has_corkscrew) {
        /* Render Corkscrew at (256, 112) */
        blit_sprite_16x16_masked(256, 112, sprite_corkscrew_even_16x16m);
    }
    
    if (comic_has_door_key) {
        /* Render Door Key at (280, 112) */
        blit_sprite_16x16_masked(280, 112, sprite_door_key_even_16x16m);
    }
    
    /* Row 2 (Y=136) */
    if (comic_jump_power > 4) {
        /* Render Boots at (232, 136) - shows when jump power exceeds default (Boots grant jump power 5) */
        blit_sprite_16x16_masked(232, 136, sprite_boots_even_16x16m);
    }
    
    if (comic_has_lantern) {
        /* Render Lantern at (256, 136) - collected in Castle level */
        blit_sprite_16x16_masked(256, 136, sprite_lantern_even_16x16m);
    }
    
    if (comic_has_teleport_wand) {
        /* Render Teleport Wand at (280, 136) */
        blit_sprite_16x16_masked(280, 136, sprite_teleport_wand_even_16x16m);
    }
    
    /* Row 3 (Y=160) - Treasures */
    if (comic_has_gems) {
        /* Render Gems at (232, 160) - first treasure */
        blit_sprite_16x16_masked(232, 160, sprite_gems_even_16x16m);
    }
    
    if (comic_has_crown) {
        /* Render Crown at (256, 160) - second treasure */
        blit_sprite_16x16_masked(256, 160, sprite_crown_even_16x16m);
    }
    
    if (comic_has_gold) {
        /* Render Gold at (280, 160) - third treasure */
        blit_sprite_16x16_masked(280, 160, sprite_gold_even_16x16m);
    }
}


/*
 * blit_8x16_sprite - Blit an 8x16 unmasked EGA sprite to both video buffers
 *
 * Sprite format: EGA planar (4 planes, 16 bytes each = 64 bytes total)
 * Blits to both the onscreen and offscreen video buffers.
 *
 * Input:
 *   pixel_x, pixel_y = top-left corner in pixel coordinates
 *   sprite_data = pointer to 64-byte EGA sprite (4 planes, 16 bytes each)
 */
void blit_8x16_sprite(uint16_t pixel_x, uint16_t pixel_y, const uint8_t __far *sprite_data)
{
    uint16_t base_offset;
    uint8_t plane;
    uint8_t row;
    uint8_t sprite_byte;
    uint16_t video_offset;
    uint8_t __far *video_ptr_a;
    uint8_t __far *video_ptr_b;
    const uint8_t *plane_data;
    uint16_t plane_offset;
    
    /* Calculate video memory offset for top-left corner of sprite */
    base_offset = (pixel_y * 320 + pixel_x) / 8;
    
    /* Blit each of the 4 color planes */
    for (plane = 0; plane < 4; plane++) {
        /* Enable writing to this plane */
        enable_ega_plane_write(plane);
        
        /* Plane data: Blue=0, Green=16, Red=32, Intensity=48 bytes offset */
        plane_offset = plane * 16;
        plane_data = sprite_data + plane_offset;
        
        /* Copy this plane to both buffers, 1 byte per row (8 pixels wide) */
        for (row = 0; row < 16; row++) {
            /* Calculate video offset for this row */
            video_offset = base_offset + (row * 40);
            
            /* Get pointers to both video buffers for this row */
            video_ptr_a = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_A + video_offset);
            video_ptr_b = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, GRAPHICS_BUFFER_GAMEPLAY_B + video_offset);
            
            /* Copy 1 byte per row (8 pixels in EGA format = 1 byte) */
            sprite_byte = plane_data[row];
            
            /* Write to both buffers */
            *video_ptr_a = sprite_byte;
            *video_ptr_b = sprite_byte;
        }
    }
}


/*
 * render_score_display - Render the 6-digit score on the game UI
 *
 * Displays the current score at the top of the screen.
 * Score is stored as 3 bytes in base-100 representation using the formula:
 *   score = score_bytes[0] + (score_bytes[1] * 100) + (score_bytes[2] * 10000)
 * 
 * Storage format (3 bytes, base-100):
 *   score_bytes[0] = rightmost digit pair (0-99, typically "00" since minimum award is 100 points)
 *   score_bytes[1] = middle digit pair (0-99, represents hundreds and thousands)
 *   score_bytes[2] = leftmost digit pair (0-99, represents ten-thousands and hundred-thousands)
 * 
 * Display format (6 decimal digits rendered at X=232-280):
 *   X=232-248: score_bytes[2] as 2 decimal digits (leftmost: ten-thousands place)
 *   X=248-264: score_bytes[1] as 2 decimal digits (middle: thousands place)
 *   X=264-280: score_bytes[0] as 2 decimal digits (rightmost: hundreds place, typically "00")
 * 
 * Each base-100 byte is converted to 2 decimal digits via division and modulo.
 * Example: score_bytes=[0, 20, 0] represents score of 0 + (20*100) + 0 = 2000 and displays as "002000"
 * Example: score_bytes=[0, 0, 5] represents score of 0 + 0 + (5*10000) = 50000 and displays as "050000"
 * Example: score_bytes=[5, 0, 0] represents score of 5 + 0 + 0 = 5 and displays as "000005"
 * 
 * Y coordinate is 24 (near top of screen).
 */
void render_score_display(void)
{
    const uint8_t __far *digit_sprites[10] = {
        sprite_score_digit_0_8x16,
        sprite_score_digit_1_8x16,
        sprite_score_digit_2_8x16,
        sprite_score_digit_3_8x16,
        sprite_score_digit_4_8x16,
        sprite_score_digit_5_8x16,
        sprite_score_digit_6_8x16,
        sprite_score_digit_7_8x16,
        sprite_score_digit_8_8x16,
        sprite_score_digit_9_8x16
    };
    
    uint8_t digit_idx;
    uint8_t base100_value;
    uint8_t high_decimal;
    uint8_t low_decimal;
    int16_t x_pos;
    uint16_t pixel_y = 24;
    
    /* Render 3 base-100 bytes as 6 decimal digits */
    /* Assembly starts at di=0x3e1 (X=264) for score[0] and moves left */
    for (digit_idx = 0; digit_idx < 3; digit_idx++) {
        base100_value = score_bytes[digit_idx];
        
        /* Convert base-100 byte to 2 decimal digits via division */
        /* Assembly uses repeated subtraction: high = base100/10, low = base100%10 */
        high_decimal = base100_value / 10;
        low_decimal = base100_value % 10;
        
        /* Calculate X position: start at 264 and move left by 16 pixels each iteration */
        x_pos = 264 - (digit_idx * 16);  /* 264, 248, 232 for indices 0, 1, 2 */
        
        /* Assembly's blit_score_digit does: inc di (move right 8), blit low, dec di, blit high */
        /* So it blits LOW digit first at x+8, then HIGH digit at x */
        if (low_decimal < 10) {
            blit_8x16_sprite((uint16_t)(x_pos + 8), pixel_y, digit_sprites[low_decimal]);
        }
        
        if (high_decimal < 10) {
            blit_8x16_sprite((uint16_t)x_pos, pixel_y, digit_sprites[high_decimal]);
        }
    }
}

