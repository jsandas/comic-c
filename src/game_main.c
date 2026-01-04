/*
 * game_main.c - Main game entry point in C
 * 
 * This is the consolidated C entry point that:
 * - Initializes the system (video, interrupts, CPU calibration)
 * - Displays startup menus
 * - Runs the title sequence
 * - Loads levels and runs the game
 * 
 * No calls to or from assembly in R5sw1991_c.asm - everything is C code.
 * Based on main_experiment.c with additions for game loop from game_main.c.
 */

#include <stdint.h>
#include <stddef.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include "globals.h"
#include "graphics.h"
#include "timing.h"
#include "music.h"
#include "sound.h"

/* Runtime library symbol for large model code */
int _big_code_ = 1;

/* XOR encryption key used for text obfuscation (not used in this version) */
#define XOR_ENCRYPTION_KEY 0x2D

/* Interrupt handler sentinel for verification */
#define INTERRUPT_HANDLER_INSTALL_SENTINEL 0x25

/* Helper macro to get the near offset of a pointer for DOS interrupts.
 * In the large memory model with a single data segment (DGROUP), this
 * extracts the 16-bit offset portion of a pointer for use in DS:DX addressing. */
#define DOS_OFFSET(ptr) ((uint16_t)(uintptr_t)(ptr))

/* Target value for joystick timing calibration weighted average.
 * This represents a baseline for CPU-speed calibration, used in the weighted average
 * formula: max_joystick_reads = 0.75 * (measured) + 0.25 * (target) */
#define JOYSTICK_TARGET_VALUE 1280

/* Game level constants */
#define LEVEL_NUMBER_LAKE       0
#define LEVEL_NUMBER_FOREST     1
#define LEVEL_NUMBER_SPACE      2
#define LEVEL_NUMBER_BASE       3
#define LEVEL_NUMBER_CAVE       4
#define LEVEL_NUMBER_SHED       5
#define LEVEL_NUMBER_CASTLE     6
#define LEVEL_NUMBER_COMP       7

/* Global variables for initialization and game state */
static uint8_t interrupt_handler_install_sentinel = 0;
static volatile uint8_t game_tick_flag = 0;
static uint16_t max_joystick_reads = 0;
static uint16_t saved_video_mode = 0;
static uint8_t current_level_number = LEVEL_NUMBER_LAKE;

/* Default keymap for keyboard configuration */
static uint8_t keymap[6] = {
    0x39,  /* SCANCODE_SPACE */
    0x52,  /* SCANCODE_INS */
    0x4B,  /* SCANCODE_LEFT */
    0x4D,  /* SCANCODE_RIGHT */
    0x38,  /* SCANCODE_ALT */
    0x3A   /* SCANCODE_CAPSLOCK */
};

/* Level names for TT2 files */
static const char* level_names[] = {
    "LAKE",    /* LEVEL_NUMBER_LAKE = 0 */
    "FOREST",  /* LEVEL_NUMBER_FOREST = 1 */
    "SPACE",   /* LEVEL_NUMBER_SPACE = 2 */
    "BASE",    /* LEVEL_NUMBER_BASE = 3 */
    "CAVE",    /* LEVEL_NUMBER_CAVE = 4 */
    "SHED",    /* LEVEL_NUMBER_SHED = 5 */
    "CASTLE",  /* LEVEL_NUMBER_CASTLE = 6 */
    "COMP"     /* LEVEL_NUMBER_COMP = 7 */
};

/* Startup notice text */
static const char STARTUP_NOTICE_TEXT[] = 
    "Captain Comic - C Implementation\r\n"
    "\r\n"
    "Press K for Keyboard setup\r\n"
    "Press J for Joystick calibration\r\n"
    "Press R for Registration info\r\n"
    "Press ESC to quit\r\n"
    "Press any other key to continue...\r\n"
    "\r\n";

/* Registration notice text */
static const char REGISTRATION_NOTICE_TEXT[] = 
    "Captain Comic - C Implementation\r\n"
    "\r\n"
    "This is a C implementation\r\n"
    "of the Captain Comic initialization code.\r\n"
    "\r\n"
    "Original game by Michael A. Denio\r\n"
    "\r\n"
    "Press any key to return...\r\n"
    "\r\n";

/* Error message for insufficient EGA support */
static const char VIDEO_MODE_ERROR_MESSAGE[] = 
    "This program requires an EGA adapter with 256K installed\r\n$";

/* KEYS.DEF filename */
static const char FILENAME_KEYMAP[] = "KEYS.DEF$";

/* Setup messages */
static const char KEYBOARD_SETUP_MESSAGE[] = 
    "Keyboard Setup\r\n"
    "(Simplified - using defaults)\r\n"
    "Press any key...\r\n$";

static const char JOYSTICK_CALIB_MESSAGE[] = 
    "Joystick Calibration\r\n"
    "(Simplified - skipped)\r\n"
    "Press any key...\r\n$";

static const char TITLE_SEQUENCE_MESSAGE[] = 
    "Title Sequence\r\n"
    "(Would display title, story, and start game)\r\n"
    "Exiting...\r\n$";

/*
 * disable_pc_speaker - Disable the PC speaker
 * 
 * Disconnects PIT channel 2 from the speaker by clearing the low 2 bits
 * of port 0x61.
 */
void disable_pc_speaker(void)
{
    uint8_t port_value;
    
    /* Read current value from port 0x61 */
    port_value = inp(0x61);
    
    /* Clear bits 0 and 1 to disconnect PIT channel 2 from speaker */
    port_value &= 0xFC;
    
    /* Write back the modified value */
    outp(0x61, port_value);
}

/*
 * wait_n_ticks - Wait for n game ticks to elapse
 * 
 * Input:
 *   ticks = number of ticks to wait
 * Output:
 *   game_tick_flag is set to 0
 * 
 * Uses DOS INT 1Ah to read the system timer for timing.
 */
void wait_n_ticks(uint16_t ticks)
{
    union REGS regs;
    uint32_t start_ticks, current_ticks;
    
    /* Get starting tick count from BIOS */
    regs.h.ah = 0x00;  /* AH=0x00: read system timer */
    int86(0x1A, &regs, &regs);
    start_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
    
    /* Wait for the requested number of ticks to elapse */
    do {
        regs.h.ah = 0x00;
        int86(0x1A, &regs, &regs);
        current_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
    } while ((current_ticks - start_ticks) < ticks);
    
    game_tick_flag = 0;
}

/*
 * calibrate_joystick - Measure CPU speed for joystick calibration
 * 
 * This function measures how many IN instructions can execute during one
 * game tick. This count is used to calibrate joystick polling timing.
 * 
 * Output:
 *   max_joystick_reads is set to the calibrated value
 */
void calibrate_joystick(void)
{
    union REGS regs;
    uint32_t start_ticks, current_ticks;
    uint16_t iteration_count = 0;
    uint16_t inner_loop;
    uint32_t adjusted;     /* Calibrated joystick read count */
    uint32_t difference;   /* Used for weighted average calculation */
    
    /* Wait until the beginning of a tick interval */
    wait_n_ticks(1);
    
    /* Get the starting tick count from BIOS */
    regs.h.ah = 0x00;  /* AH=0x00: read system timer */
    int86(0x1A, &regs, &regs);
    start_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
    
    /* Initialize current_ticks to start_ticks */
    current_ticks = start_ticks;
    
    /* Initialize DX to a safe port for IN operations */
    __asm {
        mov dx, 0x80    ; DX = safe delay port
    }
    
    /* Perform IN instructions in a tight loop, checking the timer periodically */
    do {
        /* Inner loop: perform multiple IN instructions */
        for (inner_loop = 0; inner_loop < 28; inner_loop++) {
            __asm {
                in al, dx       ; Dummy IN instruction from port 0x80
            }
        }
        
        iteration_count++;
        
        /* Check timer every so often */
        if ((iteration_count & 0x0F) == 0) {  /* Check every 16 iterations */
            regs.h.ah = 0x00;
            int86(0x1A, &regs, &regs);
            current_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
        }
    } while ((current_ticks - start_ticks) < 1);  /* Continue for one tick */
    
    /* The number of iterations is our calibrated joystick read count */
    adjusted = iteration_count;
    max_joystick_reads = adjusted;
    
    /* Apply weighted average toward JOYSTICK_TARGET_VALUE */
    if (adjusted < JOYSTICK_TARGET_VALUE) {
        difference = JOYSTICK_TARGET_VALUE - adjusted;
        difference >>= 2;  /* 0.25 * (1280 - adjusted) */
        max_joystick_reads = adjusted + difference;
    }
}

/* Saved interrupt handler addresses */
static void (__interrupt *saved_int8_handler)(void);   /* Timer tick handler */
static void (__interrupt *saved_int9_handler)(void);   /* Keyboard handler */

/*
 * int8_handler - Timer interrupt (INT 8) handler
 * 
 * Advances music on every interrupt cycle (to match the assembly implementation).
 * Sets the game tick flag on every other interrupt to signal the main loop.
 * Uses inline assembly to define the interrupt handler.
 */
static uint8_t irq0_parity = 0;

static void __interrupt int8_handler(void)
{
    /* Advance music on every interrupt cycle */
    sound_advance_tick();
    
    /* Toggle parity and set game tick flag only on odd interrupts */
    irq0_parity = (irq0_parity + 1) % 2;
    if (irq0_parity == 1) {
        game_tick_flag = 1;
    }
    
    /* Call the original timer interrupt handler to maintain system timing */
    if (saved_int8_handler) {
        _chain_intr((void (__interrupt *)())saved_int8_handler);
    }
}

/*
 * int9_handler - Keyboard interrupt (INT 9) handler
 * 
 * Reads the keyboard scancode and calls the original handler.
 * Uses inline assembly to define the interrupt handler.
 */
static void __interrupt int9_handler(void)
{
    uint8_t scancode;
    
    /* Read keyboard scancode */
    scancode = inp(0x60);
    
    /* Call the original keyboard interrupt handler */
    if (saved_int9_handler) {
        _chain_intr((void (__interrupt *)())saved_int9_handler);
    }
}

/*
 * install_interrupt_handlers - Install custom interrupt handlers
 * 
 * Installs custom handlers for:
 *   INT 8: Timer tick - sets game_tick_flag
 *   INT 9: Keyboard - reads scancode and chains to original
 * 
 * Saves the original handlers so they can be restored on exit.
 */
void install_interrupt_handlers(void)
{
    /* Save original INT 8 (timer) handler and install custom handler */
    saved_int8_handler = _dos_getvect(0x08);
    _dos_setvect(0x08, (void (__interrupt *)())int8_handler);
    
    /* Save original INT 9 (keyboard) handler and install custom handler */
    saved_int9_handler = _dos_getvect(0x09);
    _dos_setvect(0x09, (void (__interrupt *)())int9_handler);
    
    /* Set the sentinel value to indicate handlers were installed */
    interrupt_handler_install_sentinel = INTERRUPT_HANDLER_INSTALL_SENTINEL;
    
    /* Simulate handler bit-flip for verification */
    interrupt_handler_install_sentinel ^= 0xFF;
}

/*
 * save_video_mode - Save the current video mode
 * 
 * Reads the current video mode using INT 10h AH=0x0F and stores it
 * in the saved_video_mode variable for later restoration.
 */
void save_video_mode(void)
{
    union REGS regs;
    
    /* Get current video mode */
    regs.h.ah = 0x0F;  /* AH=0x0F: get video mode */
    int86(0x10, &regs, &regs);
    
    /* Store in saved_video_mode: AH=0x00 (set video mode), AL=current mode */
    saved_video_mode = ((uint16_t)0x00 << 8) | regs.h.al;
}

/*
 * check_ega_support - Check for sufficient EGA support and set video mode
 * 
 * Sets video mode 0x0D (13 decimal, 320x200 16-color EGA) and verifies it was set.
 * Also checks that the EGA has at least 256K of memory.
 * 
 * Returns:
 *   0 if EGA support is insufficient
 *   1 if EGA support is adequate
 */
int check_ega_support(void)
{
    union REGS regs;
    
    /* Sets video mode 0x0D (320x200 16-color EGA) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x0D;  /* AL=0x0D: 320x200 16-color EGA */
    int86(0x10, &regs, &regs);
    
    /* Verify that video mode 0x0D was actually set */
    regs.h.ah = 0x0F;  /* AH=0x0F: get video mode */
    int86(0x10, &regs, &regs);
    
    if (regs.h.al != 0x0D) {
        return 0;  /* Video mode 0x0D was not set */
    }
    
    /* Check for sufficient EGA memory (>=256K) */
    regs.h.ah = 0x12;  /* AH=0x12: get EGA info */
    regs.h.bl = 0x10;  /* BL=0x10: return EGA info */
    int86(0x10, &regs, &regs);
    
    if (regs.h.bl >= 3) {
        return 1;  /* Adequate EGA support */
    }
    
    return 0;  /* Insufficient memory */
}

/*
 * restore_interrupt_handlers - Restore original interrupt handlers
 * 
 * Restores the original handlers for INT 8 and INT 9 that were saved
 * when install_interrupt_handlers was called.
 */
void restore_interrupt_handlers(void)
{
    /* Restore original INT 8 (timer) handler */
    if (saved_int8_handler) {
        _dos_setvect(0x08, (void (__interrupt *)())saved_int8_handler);
    }
    
    /* Restore original INT 9 (keyboard) handler */
    if (saved_int9_handler) {
        _dos_setvect(0x09, (void (__interrupt *)())saved_int9_handler);
    }
}

/*
 * terminate_program - Cleanup and exit the program
 * 
 * Performs cleanup operations before exiting:
 * - Restore original interrupt handlers
 * - Mutes the PC speaker
 * - Restores the saved video mode
 * - Exits to DOS
 */
void terminate_program(void)
{
    union REGS regs;
    uint8_t port_value;
    
    /* Restore original interrupt handlers */
    restore_interrupt_handlers();
    
    /* Mute the sound */
    port_value = inp(0x61);
    port_value &= 0xFC;  /* Clear bits 0 and 1 */
    outp(0x61, port_value);
    
    /* Restore the saved video mode via INT 10h */
    regs.h.ah = (saved_video_mode >> 8) & 0xFF;  /* Extract upper byte (0x00) */
    regs.h.al = saved_video_mode & 0xFF;         /* Extract lower byte (mode) */
    int86(0x10, &regs, &regs);
    
    /* Terminate the program via DOS INT 21h AH=0x4c */
    regs.h.ah = 0x4c;  /* AH=0x4c: terminate with return code */
    regs.h.al = 0x00;  /* Return code 0 (success) */
    int86(0x21, &regs, &regs);
    
    /* This point should never be reached */
    while (1) {
        /* Infinite loop in case int86 returns */
    }
}

/*
 * display_ega_error - Display EGA error message and exit
 * 
 * Sets text mode, displays error message, and exits the program.
 */
void display_ega_error(void)
{
    union REGS regs;
    
    /* Set video mode to text (mode 2: 80x25 text) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x02;  /* AL=0x02: 80x25 text */
    int86(0x10, &regs, &regs);
    
    /* Write error message to standard output */
    regs.h.ah = 0x09;  /* AH=0x09: write string to standard output */
    regs.x.dx = DOS_OFFSET(VIDEO_MODE_ERROR_MESSAGE);
    int86(0x21, &regs, &regs);
    
    /* Cleanup and exit */
    terminate_program();
}

/*
 * load_keymap_file - Load keyboard configuration from KEYS.DEF if it exists
 * 
 * Attempts to open and read KEYS.DEF (6 bytes) into the keymap array.
 * If the file doesn't exist or read fails, the default keymap is used.
 * 
 * Returns:
 *   0 if file doesn't exist or read fails (default keymap is used)
 *   1 if file was successfully read and loaded
 */
int load_keymap_file(void)
{
    union REGS regs;
    int file_handle;
    uint16_t bytes_read;
    
    /* Try to open KEYS.DEF */
    regs.h.ah = 0x3D;  /* AH=0x3D: open existing file */
    regs.h.al = 0x00;  /* AL=0x00: open for reading */
    regs.x.dx = DOS_OFFSET(FILENAME_KEYMAP);
    int86(0x21, &regs, &regs);
    
    /* If file doesn't exist (carry flag set), return 0 */
    if (regs.x.cflag) {
        return 0;
    }
    
    file_handle = regs.x.ax;
    
    /* Read 6 bytes from the file into keymap */
    regs.h.ah = 0x3F;  /* AH=0x3F: read from file */
    regs.x.bx = file_handle;
    regs.x.cx = 6;     /* Read 6 bytes */
    regs.x.dx = DOS_OFFSET(keymap);
    int86(0x21, &regs, &regs);
    
    /* Check for read errors (carry flag set) */
    if (regs.x.cflag) {
        /* Read failed, close the file and return error */
        regs.h.ah = 0x3E;  /* AH=0x3E: close file */
        regs.x.bx = file_handle;
        int86(0x21, &regs, &regs);
        return 0;  /* Return error - use default keymap */
    }
    
    /* Check that we actually read 6 bytes (AX contains bytes read) */
    bytes_read = regs.x.ax;
    if (bytes_read != 6) {
        /* Read incomplete or unexpected amount of data */
        regs.h.ah = 0x3E;  /* AH=0x3E: close file */
        regs.x.bx = file_handle;
        int86(0x21, &regs, &regs);
        return 0;  /* Return error - use default keymap */
    }
    
    /* Close the file */
    regs.h.ah = 0x3E;  /* AH=0x3E: close file */
    regs.x.bx = file_handle;
    int86(0x21, &regs, &regs);
    
    /* Check for close errors (carry flag set) */
    if (regs.x.cflag) {
        return 0;  /* Close failed - treat as error, use default keymap */
    }
    
    return 1;
}

/*
 * check_interrupt_handler_install_sentinel - Verify interrupt handlers were installed
 * 
 * Returns:
 *   0 if the sentinel check failed
 *   1 if the sentinel check passed
 */
int check_interrupt_handler_install_sentinel(void)
{
    /* Undo the bit-flip that install_interrupt_handlers should have done */
    interrupt_handler_install_sentinel ^= 0xFF;
    
    /* Check if the value matches the original sentinel */
    if (interrupt_handler_install_sentinel != INTERRUPT_HANDLER_INSTALL_SENTINEL) {
        return 0;  /* Sentinel check failed */
    }
    
    return 1;  /* Sentinel check passed */
}

/*
 * clear_keyboard_buffer - Clear the BIOS keyboard buffer
 * 
 * Sets the buffer tail equal to the head to effectively clear all pending keystrokes.
 */
void clear_keyboard_buffer(void)
{
    /* Access BIOS data area at segment 0x0000 */
    uint8_t __far *bios_data = (uint8_t __far *)0x00000000L;
    uint8_t head;
    
    /* Disable interrupts while manipulating the buffer */
    _disable();
    
    /* Read head pointer and set tail equal to it */
    head = bios_data[0x041A];  /* BIOS_KEYBOARD_BUFFER_HEAD */
    bios_data[0x041C] = head;  /* BIOS_KEYBOARD_BUFFER_TAIL */
    
    /* Re-enable interrupts */
    _enable();
}

/*
 * display_registration_notice - Display registration information
 * 
 * Shows registration/about screen and waits for any key to return.
 * 
 * Returns:
 *   0 to indicate should return to startup notice
 */
int display_registration_notice(void)
{
    union REGS regs;
    const char *text_ptr;
    char ch;
    
    /* Set video mode to 80x25 text (mode 3) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x03;  /* AL=0x03: 80x25 text */
    int86(0x10, &regs, &regs);
    
    /* Display the registration text */
    text_ptr = REGISTRATION_NOTICE_TEXT;
    while ((ch = *text_ptr++) != 0) {
        /* Output character using teletype mode */
        regs.h.ah = 0x0E;  /* AH=0x0E: teletype output */
        regs.h.al = ch;
        regs.h.bh = 0;     /* Page 0 */
        int86(0x10, &regs, &regs);
    }
    
    /* Wait for any keystroke */
    regs.h.ah = 0x00;  /* AH=0x00: get keystroke */
    int86(0x16, &regs, &regs);
    
    return 0;  /* Return to startup notice */
}

/*
 * setup_keyboard_interactive - Interactive keyboard configuration
 * 
 * Simplified stub for keyboard setup.
 * 
 * Returns:
 *   0 to return to startup notice
 *   1 to continue to title sequence
 */
int setup_keyboard_interactive(void)
{
    union REGS regs;
    
    /* Set video mode to 80x25 text (mode 2) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x02;  /* AL=0x02: 80x25 text */
    int86(0x10, &regs, &regs);
    
    /* Display message */
    regs.h.ah = 0x09;  /* AH=0x09: write string to standard output */
    regs.x.dx = DOS_OFFSET(KEYBOARD_SETUP_MESSAGE);
    int86(0x21, &regs, &regs);
    
    /* Wait for keystroke */
    regs.h.ah = 0x00;
    int86(0x16, &regs, &regs);
    
    return 1;  /* Continue to title */
}

/*
 * calibrate_joystick_interactive - Interactive joystick calibration
 * 
 * Simplified stub for joystick calibration.
 * 
 * Returns:
 *   0 to return to startup notice
 */
int calibrate_joystick_interactive(void)
{
    union REGS regs;
    
    /* Set video mode to 80x25 text (mode 2) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x02;  /* AL=0x02: 80x25 text */
    int86(0x10, &regs, &regs);
    
    /* Display message */
    regs.h.ah = 0x09;  /* AH=0x09: write string to standard output */
    regs.x.dx = DOS_OFFSET(JOYSTICK_CALIB_MESSAGE);
    int86(0x21, &regs, &regs);
    
    /* Wait for keystroke */
    regs.h.ah = 0x00;
    int86(0x16, &regs, &regs);
    
    return 0;  /* Return to startup notice */
}

/*
 * title_sequence - Display title sequence with graphics and transitions
 * 
 * Orchestrates the complete title sequence flow:
 *   1. Title screen (SYS000.EGA) with fade-in, 14 tick delay
 *   2. Story screen (SYS001.EGA) with fade-in, wait for keystroke
 *   3. UI background (SYS003.EGA) loaded and duplicated
 *   4. Items screen (SYS004.EGA), wait for keystroke
 *   5. Switch to gameplay buffer
 * 
 * Uses graphics module for loading, buffer switching, and palette effects.
 */
void title_sequence(void)
{
    union REGS regs;
    
    /* Set EGA graphics mode 0x0D (320x200 16-color) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x0D;  /* AL=0x0D: 320x200 16-color EGA */
    int86(0x10, &regs, &regs);
    
    /* Initialize EGA graphics controller for correct write mode */
    init_ega_graphics();
    
    /* Set palette explicitly - BIOS defaults may be wrong */
    init_default_palette();
    
    /* Step 1: Load and display title screen (SYS000.EGA) */
    if (load_fullscreen_graphic(FILENAME_TITLE_GRAPHIC, GRAPHICS_BUFFER_TITLE_TEMP1) != 0) {
        /* Failed to load - skip title sequence */
        return;
    }
    switch_video_buffer(GRAPHICS_BUFFER_TITLE_TEMP1);
    palette_darken();
    palette_fade_in();
    
    /* Start title music */
    play_title_music();
    
    wait_n_ticks(14);  /* Display title for ~770ms (14 ticks at ~55ms each) */
    
    /* Step 2: Load and display story screen (SYS001.EGA) */
    if (load_fullscreen_graphic(FILENAME_STORY_GRAPHIC, GRAPHICS_BUFFER_TITLE_TEMP2) != 0) {
        /* Failed to load - stop music and skip remaining sequence */
        stop_music();
        return;
    }
    switch_video_buffer(GRAPHICS_BUFFER_TITLE_TEMP2);
    palette_darken();
    palette_fade_in();
    
    /* Wait for keystroke */
    regs.h.ah = 0x00;  /* AH=0x00: get keystroke */
    int86(0x16, &regs, &regs);
    
    /* Step 3: Load UI background (SYS003.EGA) into both gameplay buffers */
    if (load_fullscreen_graphic(FILENAME_UI_GRAPHIC, GRAPHICS_BUFFER_GAMEPLAY_A) != 0) {
        /* Failed to load - stop music and skip remaining sequence */
        stop_music();
        return;
    }
    /* Copy UI from buffer A to buffer B for static background (8000 bytes per plane) */
    copy_ega_plane(GRAPHICS_BUFFER_GAMEPLAY_A, GRAPHICS_BUFFER_GAMEPLAY_B, 8000);
    
    /* Step 4: Load and display items screen (SYS004.EGA) */
    if (load_fullscreen_graphic(FILENAME_ITEMS_GRAPHIC, GRAPHICS_BUFFER_TITLE_TEMP1) != 0) {
        /* Failed to load - stop music and skip items screen */
        stop_music();
        return;
    }
    switch_video_buffer(GRAPHICS_BUFFER_TITLE_TEMP1);
    
    /* Wait for keystroke */
    regs.h.ah = 0x00;  /* AH=0x00: get keystroke */
    int86(0x16, &regs, &regs);
    
    /* Stop title music before transitioning to gameplay */
    stop_music();
    
    /* Step 5: Switch to gameplay buffer B (with UI background) */
    switch_video_buffer(GRAPHICS_BUFFER_GAMEPLAY_B);
}

/*
 * display_startup_notice - Display startup screen and wait for keypress
 * 
 * Sets text mode, displays text, and waits for a keypress.
 * Based on the key pressed:
 *   - 'k' or 'K': Keyboard setup
 *   - 'j' or 'J': Joystick calibration
 *   - 'r' or 'R': Registration/about information
 *   - Escape (27): Exit program
 *   - Any other key: Continue to title sequence
 * 
 * Returns:
 *   0 to exit program
 *   1 to continue to title sequence
 */
int display_startup_notice(void)
{
    union REGS regs;
    const char *text_ptr;
    char ch;
    uint8_t key_ascii;
    
    /* Main menu loop */
    while (1) {
        /* Set video mode to 80x25 text (mode 3) */
        regs.h.ah = 0x00;  /* AH=0x00: set video mode */
        regs.h.al = 0x03;  /* AL=0x03: 80x25 text */
        int86(0x10, &regs, &regs);
        
        /* Display the plain text */
        text_ptr = STARTUP_NOTICE_TEXT;
        while ((ch = *text_ptr++) != 0) {
            /* Output character using teletype mode */
            regs.h.ah = 0x0E;  /* AH=0x0E: teletype output */
            regs.h.al = ch;
            regs.h.bh = 0;     /* Page 0 */
            int86(0x10, &regs, &regs);
        }
        
        /* Clear the BIOS keyboard buffer before waiting for input */
        clear_keyboard_buffer();
        
        /* Wait for a keystroke */
        regs.h.ah = 0x00;  /* AH=0x00: get keystroke */
        int86(0x16, &regs, &regs);
        key_ascii = regs.h.al;  /* AL contains ASCII code */
        
        /* Check which key was pressed and take appropriate action */
        if (key_ascii == 'k' || key_ascii == 'K') {
            /* Keyboard setup */
            if (setup_keyboard_interactive()) {
                return 1;  /* Setup complete, continue to title */
            }
            continue;
        }
        
        if (key_ascii == 'j' || key_ascii == 'J') {
            /* Joystick calibration */
            calibrate_joystick_interactive();
            continue;
        }
        
        if (key_ascii == 'r' || key_ascii == 'R') {
            /* Registration/about information */
            display_registration_notice();
            continue;
        }
        
        if (key_ascii == 27) {
            /* Escape key - exit program */
            return 0;
        }
        
        /* Any other key - continue to title sequence */
        return 1;
    }
}

/*
 * copy_string - Simple string copy helper
 */
static void copy_string(char* dest, const char* src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/*
 * load_new_level - Load a new level
 * 
 * Attempts to open the TT2 file for the current level using DOS interrupt.
 * Outputs a simple message to indicate success or failure.
 * 
 * This is a stub that demonstrates the C-only approach to level loading,
 * without depending on assembly functions.
 */
void load_new_level(void)
{
    char filename[16];
    uint16_t file_handle;
    uint16_t result;
    const char* message;
    
    /* Build the filename: LEVELNAME.TT2 */
    if (current_level_number < 8) {
        const char* level_name = level_names[current_level_number];
        char* p = filename;
        
        /* Copy level name */
        copy_string(p, level_name);
        p += 4; /* LAKE, BASE, CAVE, SHED, COMP are 4 chars */
        if (current_level_number == LEVEL_NUMBER_FOREST || 
            current_level_number == LEVEL_NUMBER_CASTLE) {
            p += 2; /* FOREST and CASTLE are 6 chars */
        } else if (current_level_number == LEVEL_NUMBER_SPACE) {
            p += 1; /* SPACE is 5 chars */
        }
        
        /* Append .TT2 extension */
        copy_string(p, ".TT2");
        
        /* Try to open the file using DOS INT 21h, AH=3Dh */
        __asm {
            push ds
            lea dx, filename
            mov ax, 0x3d00      ; AH=3Dh: open file, AL=00: read-only
            int 0x21
            pop ds
            jc open_failed
            mov file_handle, ax
            mov result, 0       ; success
            jmp done
        open_failed:
            mov result, 1       ; failure
        done:
        }
        
        /* Close the file if opened successfully */
        if (result == 0) {
            __asm {
                mov bx, file_handle
                mov ah, 0x3e        ; AH=3Eh: close file
                int 0x21
            }
            message = "TT2 file opened successfully\r\n$";
        } else {
            message = "TT2 file open failed\r\n$";
        }
        
        /* Output message using DOS INT 21h, AH=09h */
        __asm {
            push ds
            lea dx, message
            mov ah, 0x09        ; AH=09h: write string to stdout
            int 0x21
            pop ds
        }
    }
}

/*
 * game_loop_iteration - C skeleton for one game loop iteration
 * 
 * This is a placeholder structure showing the intended organization of
 * the main game loop when ported to C. Currently not used, but documents
 * the intended flow.
 * 
 * The actual game loop would:
 * - Wait for game ticks
 * - Check win conditions
 * - Update player state
 * - Handle input
 * - Update enemies and other actors
 * - Render and swap buffers
 */
#if 0
int game_loop_iteration(void)
{
    /* Wait for game tick */
    while (!game_tick_flag) {
        /* Idle */
    }
    game_tick_flag = 0;
    
    /* Check win condition */
    if (win_counter > 0) {
        win_counter--;
        if (win_counter == 1) {
            /* Signal game won */
            return 1;
        }
    }
    
    /* Handle player state */
    /* ... update player position, animation, etc ... */
    
    /* Handle input */
    /* ... process keyboard input ... */
    
    /* Update enemies and other actors */
    /* ... */
    
    /* Render */
    /* ... render map and sprites ... */
    
    return 0;  /* Continue game loop */
}
#endif

/*
 * main - C entry point
 * 
 * Entry point for the C-only version.
 * Performs all initialization in C:
 * - DS initialization (handled by C runtime)
 * - Disable PC speaker
 * - Install interrupt handler sentinel
 * - Calibrate CPU speed
 * - Save current video mode
 * - Check for sufficient EGA support
 * - Load keymap from KEYS.DEF if present
 * - Verify interrupt handlers were installed
 * - Display startup notice and handle menus
 * - Run title sequence
 * - Exit
 */
int main(void)
{
    /* Disable the PC speaker */
    disable_pc_speaker();
    
    /* Install interrupt handler sentinel */
    install_interrupt_handlers();
    
    /* Calibrate CPU speed for joystick timing */
    calibrate_joystick();
    
    /* Save the current video mode for later restoration */
    save_video_mode();
    
    /* Check for sufficient EGA support and set video mode */
    if (!check_ega_support()) {
        /* EGA support is insufficient */
        display_ega_error();  /* displays error and terminates */
        /* Never returns */
    }
    
    /* Load keyboard configuration from KEYS.DEF if it exists */
    load_keymap_file();
    
    /* Verify that the custom interrupt handlers were installed */
    if (!check_interrupt_handler_install_sentinel()) {
        /* Interrupt handlers were not installed */
        /* For this version, we'll just continue anyway */
        union REGS regs;
        regs.h.ah = 0x00;  /* Set text mode first */
        regs.h.al = 0x03;
        int86(0x10, &regs, &regs);
    }
    
    /* Display the startup notice and handle user input */
    if (!display_startup_notice()) {
        /* User pressed Escape to exit */
        terminate_program();
        return 0;
    }
    
    /* User chose to continue - show title sequence */
    title_sequence();
    
    /* Cleanup and exit */
    terminate_program();
    
    return 0;
}
