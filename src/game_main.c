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
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "globals.h"
#include "graphics.h"
#include "timing.h"
#include "music.h"
#include "sound.h"
#include "sprite_data.h"
#include "level_data.h"
#include "file_loaders.h"

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

/* Game constants */
#define MAX_NUM_LIVES           5
#define MAX_HP                  7

/* BIOS keyboard buffer addresses */
#define BIOS_KEYBOARD_BUFFER_HEAD  0x041A
#define BIOS_KEYBOARD_BUFFER_TAIL  0x041C

/* Global variables for initialization and game state */
static uint8_t interrupt_handler_install_sentinel = 0;
static volatile uint8_t game_tick_flag = 0;
static uint16_t max_joystick_reads = 0;
static uint16_t saved_video_mode = 0;
static uint8_t current_level_number = LEVEL_NUMBER_FOREST;
static uint8_t current_stage_number = 0;
static level_t current_level;

/* Game state variables */
static uint8_t win_counter = 0;
static uint8_t comic_x = 0;
static uint8_t comic_y = 0;
static uint8_t comic_animation = COMIC_STANDING;
static uint8_t comic_facing = COMIC_FACING_RIGHT;
static uint8_t comic_run_cycle = COMIC_RUNNING_1;
static uint8_t comic_is_falling_or_jumping = 0;
static uint8_t comic_is_teleporting = 0;
static int8_t comic_x_momentum = 0;
static int8_t comic_y_vel = 0;
static uint8_t comic_jump_counter = 4;
static uint8_t comic_jump_power = 4;
static uint8_t comic_hp = MAX_HP;
static uint8_t comic_hp_pending_increase = 0;
static uint16_t camera_x = 0;

/* Input state variables (set by keyboard interrupt handler) */
static uint8_t key_state_esc = 0;
static uint8_t key_state_jump = 0;
static uint8_t key_state_fire = 0;
static uint8_t key_state_left = 0;
static uint8_t key_state_right = 0;
static uint8_t key_state_open = 0;
static uint8_t key_state_teleport = 0;

/* Fireball state */
static uint8_t fireball_meter = 100;
static uint8_t fireball_meter_counter = 2;

/* Item collection state */
static uint8_t comic_has_door_key = 0;
static uint8_t comic_has_teleport_wand = 0;

/* Tileset buffer - holds data from .TT2 file */
static uint8_t tileset_last_passable;
static uint8_t tileset_flags;
static uint8_t tileset_graphics[128 * 128];  /* Up to 128 16x16 tiles */

/* Stage data - three .PT files per level */
static pt_file_t pt0;
static pt_file_t pt1;
static pt_file_t pt2;
static uint8_t *current_tiles_ptr = NULL;  /* Points to current stage's tile map */
static uint8_t comic_num_lives = 0;

/* Offscreen buffer pointer (0x0000 or 0x2000) - start with A as offscreen when B is displayed */
static uint16_t offscreen_video_buffer_ptr = GRAPHICS_BUFFER_GAMEPLAY_A;

/* Item animation counter (0 or 1) */
static uint8_t item_animation_counter = 0;

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

/* Forward declarations */
int load_new_level(void);
void load_new_stage(void);
void game_loop(void);

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
 * award_extra_life - Award an extra life to the player
 * 
 * Increments the number of lives if not already at maximum.
 * Updates the UI display with a bright life icon.
 */
void award_extra_life(void)
{
    uint16_t x_pixel, y_pixel;
    
    /* Increment lives if below max */
    if (comic_num_lives < MAX_NUM_LIVES) {
        comic_num_lives++;
        
        /* Display the bright life icon at the appropriate position
         * Icons start at x=24, then each life is at x = 24 + (life_count * 24) pixels
         * After incrementing, comic_num_lives contains the NEW count */
        x_pixel = 24 + (comic_num_lives * 24);
        y_pixel = 180;
        
        /* Blit the bright life icon to both gameplay buffers */
        blit_sprite_16x16_masked(x_pixel, y_pixel, sprite_life_icon_bright);
    }
}

/*
 * lose_a_life - Subtract a life from the player
 * 
 * Decrements the number of lives if greater than 0.
 * Updates the UI display with a dark life icon.
 */
void lose_a_life(void)
{
    uint16_t x_pixel, y_pixel;
    uint8_t old_lives;
    
    /* Subtract a life if any remain */
    if (comic_num_lives > 0) {
        /* Save the current life count before decrementing */
        old_lives = comic_num_lives;
        
        /* Decrement the life count */
        comic_num_lives--;
        
        /* Display the dark (unavailable) life icon at the position of the life we just lost
         * Use the OLD value (before decrement) to calculate position
         * Icons are at x = 24 + (life_count * 24) pixels */
        x_pixel = 24 + (old_lives * 24);
        y_pixel = 180;
        
        /* Blit the dark life icon to both gameplay buffers */
        blit_sprite_16x16_masked(x_pixel, y_pixel, sprite_life_icon_dark);
    }
}

/*
 * initialize_lives_sequence - Play the animation of awarding initial lives
 * 
 * Awards MAX_NUM_LIVES lives one at a time with animation, then subtracts
 * one to represent the life in use at the start of the game. This matches
 * the behavior of the original assembly code.
 * 
 * The sequence:
 * 1. Save current sound mute state
 * 2. Mute sound
 * 3. Award MAX_NUM_LIVES lives one by one with 1 tick delay between each
 * 4. Stop sound and restore mute state
 * 5. Wait 3 ticks
 * 6. Subtract 1 life (the one currently in use)
 * 7. Continue to load_new_level
 * 
 * Output:
 *   comic_num_lives = MAX_NUM_LIVES - 1
 */
void initialize_lives_sequence(void)
{
    uint8_t i;
    uint8_t sound_was_enabled;
    
    /* Save current sound enable state and mute sound
     * This prevents the extra life sound from actually playing */
    sound_was_enabled = is_sound_enabled();
    mute_sound();
    
    /* Award MAX_NUM_LIVES lives, one at a time with animation */
    for (i = 0; i < MAX_NUM_LIVES; i++) {
        wait_n_ticks(1);  /* Wait 1 tick between each life awarded */
        award_extra_life();
    }
    
    /* Stop the extra life sound that's playing (but muted) */
    stop_sound();
    
    /* Restore the saved sound enable state */
    if (sound_was_enabled) {
        unmute_sound();
    }
    
    /* Wait 3 ticks before subtracting the initial life */
    wait_n_ticks(3);
    
    /* Subtract 1 life (the one in use at game start) */
    lose_a_life();
    
    /* Sanity check: verify lives == MAX_NUM_LIVES - 1 */
    if (comic_num_lives != MAX_NUM_LIVES - 1) {
        /* This is dead code from the original assembly that sets win_counter
         * to trigger an instant win. We'll omit it for now. */
        /* win_counter = 200; */
    }
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

    /* Draw an immediate visible overlay directly into the gameplay buffers
     * so it is visible on the title/UI screens (before gameplay starts). */
    {
        const uint16_t dst_x_pix = 40;
        const uint16_t dst_y_pix = 64;
        const uint16_t dst_byte_x = dst_x_pix / 8;
        const uint16_t bytes_per_row = SCREEN_WIDTH / 8; /* 40 */
        uint8_t plane, row, col;

        /* Ensure the buffer with the UI is visible (we set to B later anyway) */
        switch_video_buffer(GRAPHICS_BUFFER_GAMEPLAY_B);

    }

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
    
    /* Initialize lives and start the game */
    /* In the original assembly, this jumps to initialize_lives_sequence */
    initialize_lives_sequence();
    
    /* After lives are initialized, load the level and run game loop */
    if (load_new_level() != 0) {
        /* Failed to load level - cannot continue */
        return;
    }
    
    load_new_stage();
    
    /* Run main game loop */
    game_loop();
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
 * Loads the level specified by current_level_number:
 * 1. Copies level data from static level_data_pointers array to current_level
 * 2. Opens and reads the .TT2 file (tileset graphics with 16x16 tile images)
 * 3. Loads the three .PT files (stage maps) into pt0, pt1, pt2 structures
 * 4. Performs lantern check for castle level (TODO: implement lantern blackout)
 * 5. TODO: Load .SHP files for enemy sprites
 * 
 * Input:
 *   current_level_number = level to load (0-7)
 * 
 * Output:
 *   current_level = filled with level data
 *   tileset_graphics = array of tile images from .TT2 file
 *   pt0, pt1, pt2 = stage maps from .PT files
 * 
 * Returns:
 *   0 on success
 *   -1 on error (invalid level number, file not found, read error)
 */
int load_new_level(void)
{
    int file_handle;
    unsigned bytes_read;
    const level_t* source_level;
    unsigned i;
    uint8_t __far *video_ptr;
    uint8_t *compressed_buffer;
    unsigned decompressed_size;
    uint16_t j;
    
    /* TT2 file header structure */
    struct {
        uint8_t last_passable;
        uint8_t unused1;
        uint8_t unused2;
        uint8_t flags;
    } tt2_header;
    

    
    /* Validate level number */
    if (current_level_number >= 8) {
        return -1;  /* Invalid level number */
    }
    
    /* Copy level data from static data to current_level */
    source_level = level_data_pointers[current_level_number];
    memcpy(&current_level, source_level, sizeof(level_t));
    
    /* Load the .TT2 file (tileset graphics) */
    file_handle = _open(current_level.tt2_filename, O_RDONLY | O_BINARY);
    if (file_handle == -1) {
        /* Tileset file not found */
        memset(tileset_graphics, 0, sizeof(tileset_graphics));
        tileset_last_passable = 0;
        tileset_flags = 0;
    } else {
        /* Read TT2 file header (4 bytes) */
        bytes_read = _read(file_handle, &tt2_header, sizeof(tt2_header));
        if (bytes_read != sizeof(tt2_header)) {
            _close(file_handle);
            memset(tileset_graphics, 0, sizeof(tileset_graphics));
            tileset_last_passable = 0;
            tileset_flags = 0;
        } else {
            /* Extract header fields */
            tileset_last_passable = tt2_header.last_passable;
            tileset_flags = tt2_header.flags;

            /* The TT2 file format is: 4-byte header + uncompressed tile data (tile-major order).
             * Each tile is 128 bytes (4 planes × 32 bytes per plane).
             * We read the tiles directly into tileset_graphics. */
            bytes_read = _read(file_handle, tileset_graphics, sizeof(tileset_graphics));
            _close(file_handle);
        }
    }
    
    /* Lantern check: If in castle without lantern, black out tiles */
    if (current_level_number == LEVEL_NUMBER_CASTLE) {
        /* TODO: Check comic_has_lantern when that variable is available */
        /* For now, skip the blackout */
        /* if (comic_has_lantern != 1) { */
        /*     for (i = 0; i < sizeof(tileset_graphics); i++) { */
        /*         tileset_graphics[i] = 0; */
        /*     } */
        /* } */
    }
    
    /* Load the three .PT files for this level */
    if (load_pt_file(current_level.pt0_filename, &pt0) != 0) {
        /* Initialize with empty map if load fails */
        memset(&pt0, 0, sizeof(pt0));
        pt0.width = MAP_WIDTH_TILES;
        pt0.height = MAP_HEIGHT_TILES;
    }
    
    if (load_pt_file(current_level.pt1_filename, &pt1) != 0) {
        /* Initialize with empty map if load fails */
        memset(&pt1, 0, sizeof(pt1));
        pt1.width = MAP_WIDTH_TILES;
        pt1.height = MAP_HEIGHT_TILES;
    }
    
    if (load_pt_file(current_level.pt2_filename, &pt2) != 0) {
        /* Initialize with empty map if load fails */
        memset(&pt2, 0, sizeof(pt2));
        pt2.width = MAP_WIDTH_TILES;
        pt2.height = MAP_HEIGHT_TILES;
    }
    
    /* Load .SHP files referenced by this level into runtime cache */
    load_level_shp_files(&current_level);

    return 0;  /* Success - always proceed even if some files missing */
}

/*
 * Stub implementations for game loop helper functions
 * These will be implemented progressively as the refactor continues
 */

static void handle_teleport(void)
{
    /* TODO: Implement full teleportation animation and logic
     * 
     * Teleportation is a 6-frame animation (0-5) with special behavior:
     * 1. Frames 0-5: Animate at source position
     * 2. Frames 1-5: Animate at destination position (delayed by 1 frame)
     * 3. Frame 3: Actual position change (collision with enemies uses new position)
     * 4. Camera movement: Controlled by teleport_camera_counter/teleport_camera_vel
     *    - Camera moves for first teleport_camera_counter ticks
     *    - Amount per tick is teleport_camera_vel
     * 5. Frame 6: Animation complete, set comic_is_teleporting = 0
     * 
     * Current behavior:
     * - Immediately ends teleportation (WRONG - should animate for 6 frames)
     * 
     * Required variables (from begin_teleport):
     * - teleport_source_y/x: Position where teleport started
     * - teleport_destination_y/x: Position where teleport ends
     * - teleport_animation: Current frame (0-6)
     * - teleport_camera_counter: Frames during which camera should move
     * - teleport_camera_vel: Camera movement per frame
     */
    comic_is_teleporting = 0;  /* TODO: Remove this and implement full animation */
}

static void handle_fall_or_jump(void)
{
    /* TODO: Implement gravity, jumping, and ground collision */
}

static void begin_teleport(void)
{
    /* TODO: Implement teleport initiation */
}

static void face_or_move_left(void)
{
    /* TODO: Implement left movement and collision */
    comic_facing = COMIC_FACING_LEFT;
    if (comic_is_falling_or_jumping == 0) {
        comic_animation = comic_run_cycle;
    }
}

static void face_or_move_right(void)
{
    /* TODO: Implement right movement and collision */
    comic_facing = COMIC_FACING_RIGHT;
    if (comic_is_falling_or_jumping == 0) {
        comic_animation = comic_run_cycle;
    }
}

static void pause_game(void)
{
    /* TODO: Implement pause screen */
}

static void try_to_fire(void)
{
    /* TODO: Implement fireball firing logic */
}

static void decrement_fireball_meter(void)
{
    if (fireball_meter > 0) {
        fireball_meter--;
    }
}

static void increment_fireball_meter(void)
{
    if (fireball_meter < 100) {
        fireball_meter++;
    }
}

static void blit_map_playfield_offscreen(void)
{
    /* Blit the visible playfield region from the rendered map buffer
     * into the offscreen video buffer, plane by plane.
     * Uses EGA planar layout: all planes at same offsets, selected via plane registers.
     *  - camera_x is in game units (1 unit == 8 pixels == 1 byte horizontally)
     *  - Playfield top-left pixel is at (8,8)
     */
    const uint16_t screen_bytes_per_row = SCREEN_WIDTH / 8; /* 40 */
    const uint16_t rendered_bytes_per_row = 256; /* Rendered map uses 256-byte stride */
    const uint16_t playfield_pixel_rows = PLAYFIELD_HEIGHT * 8; /* 20 * 8 = 160 */
    const uint16_t playfield_bytes_per_row = PLAYFIELD_WIDTH; /* 24 bytes */
    uint8_t plane;
    uint16_t row;

    for (plane = 0; plane < 4; plane++) {
        /* Enable reading and writing for this plane */
        enable_ega_plane_read_write(plane);

        for (row = 0; row < playfield_pixel_rows; row++) {
            /* Source offset within RENDERED_MAP_BUFFER: rendered tiles start at row 0, not row 8 */
            uint16_t src_offset = RENDERED_MAP_BUFFER + (row * rendered_bytes_per_row) + camera_x;
            uint16_t dst_offset = offscreen_video_buffer_ptr + ((8 + row) * screen_bytes_per_row) + (8 / 8);

            /* Ensure we do not read past the rendered map bounds */
            uint16_t max_src_bytes = (rendered_bytes_per_row * 200) - ((8 + row) * rendered_bytes_per_row) - camera_x;
            uint16_t bytes_to_copy = playfield_bytes_per_row;
            if (bytes_to_copy > max_src_bytes) {
                bytes_to_copy = max_src_bytes;
            }

            /* Copy the row for this plane using helper that performs far-pointer copy */
            copy_plane_bytes(src_offset, dst_offset, bytes_to_copy);
        }
    }
}

static void blit_comic_playfield_offscreen(void)
{
    /* Choose the correct 16x32 sprite based on animation and facing */
    const uint8_t __far *sprite_ptr = NULL;
    int rel_x_units;              /* 16-bit on target */
    unsigned int pixel_x;        /* pixel coordinates (0-319) */
    unsigned int pixel_y;        /* pixel coordinates (0-199) */

    if (comic_animation == COMIC_STANDING) {
        sprite_ptr = (comic_facing == COMIC_FACING_LEFT)
            ? sprite_R4_comic_standing_left_16x32m
            : sprite_R4_comic_standing_right_16x32m;
    } else if (comic_animation == COMIC_JUMPING) {
        sprite_ptr = (comic_facing == COMIC_FACING_LEFT)
            ? sprite_R4_comic_jumping_left_16x32m
            : sprite_R4_comic_jumping_right_16x32m;
    } else {
        /* Running cycle 1..3 */
        switch (comic_run_cycle) {
            case COMIC_RUNNING_1:
                sprite_ptr = (comic_facing == COMIC_FACING_LEFT)
                    ? sprite_R4_comic_running_1_left_16x32m
                    : sprite_R4_comic_running_1_right_16x32m;
                break;
            case COMIC_RUNNING_2:
                sprite_ptr = (comic_facing == COMIC_FACING_LEFT)
                    ? sprite_R4_comic_running_2_left_16x32m
                    : sprite_R4_comic_running_2_right_16x32m;
                break;
            case COMIC_RUNNING_3:
            default:
                sprite_ptr = (comic_facing == COMIC_FACING_LEFT)
                    ? sprite_R4_comic_running_3_left_16x32m
                    : sprite_R4_comic_running_3_right_16x32m;
                break;
        }
    }

    if (sprite_ptr == NULL) {
        return;
    }

    /* Compute pixel coordinates relative to camera and playfield offset (8,8) */
    rel_x_units = (int)comic_x - (int)camera_x;
    if (rel_x_units < -2 || rel_x_units > (PLAYFIELD_WIDTH + 2)) {
        /* Offscreen horizontally; skip drawing */
        return;
    }

    pixel_x = (unsigned int)(rel_x_units * 8) + 8; /* +8 pixel playfield left margin */
    pixel_y = (unsigned int)comic_y * 8 + 8;      /* vertical positioning */



    /* Blit 16x32 masked sprite to the offscreen buffers */
    blit_sprite_16x32_masked((uint16_t)pixel_x, (uint16_t)pixel_y, (const uint8_t *)sprite_ptr);
}

static void handle_enemies(void)
{
    /* TODO: Implement enemy AI and rendering */
}

static void handle_fireballs(void)
{
    /* TODO: Implement fireball movement and collision */
}

static void handle_item(void)
{
    /* TODO: Implement item collision and rendering */
}

static void swap_video_buffers(void)
{
    /* Display the offscreen buffer and then toggle which buffer is offscreen */
    switch_video_buffer(offscreen_video_buffer_ptr);

    /* Toggle between 0x0000 and 0x2000 */
    if (offscreen_video_buffer_ptr == GRAPHICS_BUFFER_GAMEPLAY_A) {
        offscreen_video_buffer_ptr = GRAPHICS_BUFFER_GAMEPLAY_B;
    } else {
        offscreen_video_buffer_ptr = GRAPHICS_BUFFER_GAMEPLAY_A;
    }

    /* Advance item animation counter (0 -> 1 -> 0) */
    item_animation_counter++;
    if (item_animation_counter >= 2) {
        item_animation_counter = 0;
    }
}

static void increment_comic_hp(void)
{
    if (comic_hp < MAX_HP) {
        comic_hp++;
    }
}

static uint16_t address_of_tile_at_coordinates(uint8_t x, uint8_t y)
{
    /* Calculate byte offset into tile map
     * 
     * Input coordinates should be valid:
     *   x: 0-127 (MAP_WIDTH_TILES - 1)
     *   y: 0-9 (MAP_HEIGHT_TILES - 1)
     * 
     * If coordinates are out of bounds, the returned offset will exceed
     * MAP_WIDTH_TILES * MAP_HEIGHT_TILES (1280). Callers must check the
     * returned value against this limit before dereferencing the tile map.
     * 
     * Map layout: Linear array, row-major order
     *   tiles[y*128 + x] = tile ID at (x, y)
     * 
     * Offset = y * MAP_WIDTH_TILES + x
     */
    return (uint16_t)y * MAP_WIDTH_TILES + x;
}

/*
 * blit_tile_to_map - Blit a single 16x16 tile to the pre-rendered map buffer
 * 
 * Input:
 *   tile_id = ID of tile in tileset (0-127)
 *   tile_x = column position in map (0-127)
 *   tile_y = row position in map (0-9)
 *   tileset_ptr = pointer to tileset graphics data
 * 
 * The tileset contains up to 128 tiles, each 16x16 pixels.
 * Each tile is 128 bytes (32 bytes per plane × 4 planes).
 * The rendered map buffer is at 0xa000:4000, organized as:
 *   - Each screen row is 256 bytes (40 bytes per pixel row + padding)
 *   - 128 tiles per row (each tile is 2 bytes wide)
 *   - 16 pixel rows per tile
 *   - 4 planes stored sequentially
 * 
 * Tiles are blitted plane-by-plane to RENDERED_MAP_BUFFER at offset:
 *   buffer_offset = (tile_y * 16 * 256) + (tile_x * 2)
 */
static void blit_tile_to_map(uint8_t tile_id, uint8_t tile_x, uint8_t tile_y, const uint8_t *tileset_ptr)
{
    uint8_t plane;
    uint8_t row;
    uint16_t src_offset;
    uint16_t dst_offset;
    uint8_t byte0, byte1;
    uint8_t __far *dst_ptr;
    int i;
    
    /* For each plane */
    for (plane = 0; plane < 4; plane++) {
        /* Set plane mask */
        outp(0x3c4, 0x02);
        outp(0x3c5, 1 << plane);
        
        /* Delay to ensure register is set */
        for (i = 0; i < 10; i++) inp(0x3c5);
        
        /* For each row in the tile */
        for (row = 0; row < 16; row++) {
            /* Source offset in tileset_ptr:
             * Tile ID selects base: tile_id * 128
             * Plane selects within tile: plane * 32
             * Row selects within plane: row * 2
             */
            src_offset = (tile_id * 128) + (plane * 32) + (row * 2);
            byte0 = tileset_ptr[src_offset + 0];
            byte1 = tileset_ptr[src_offset + 1];
            
            /* Destination offset in RENDERED_MAP_BUFFER:
             * Base position for this tile: (tile_y * 16 * 256) + (tile_x * 2)
             * Plus row offset: row * 256
             */
            dst_offset = RENDERED_MAP_BUFFER + (tile_y * 16 * 256) + (tile_x * 2) + (row * 256);
            dst_ptr = (uint8_t __far *)MK_FP(0xa000, dst_offset);
            
            /* Write the bytes */
            dst_ptr[0] = byte0;
            dst_ptr[1] = byte1;
        }
    }
}

/*
 * render_map - Pre-render entire tileset into RENDERED_MAP_BUFFER
 * 
 * Iterates through the current level's tile map and renders all tiles
 * into RENDERED_MAP_BUFFER (0xa000:4000). This buffer is then used by
 * blit_map_playfield_offscreen() to render the visible portion each frame.
 * 
 * Preconditions:
 *   - current_tiles_ptr must point to a valid tile map (128×10)
 *   - tileset_graphics[] must contain loaded tileset data
 *   - RENDERED_MAP_BUFFER region must be accessible
 */
static void render_map(void)
{
    uint16_t tile_x;
    uint8_t tile_y;
    uint16_t tile_index;
    uint8_t tile_id;
    uint8_t plane;
    uint16_t clear_offset;
    uint16_t clear_row;
    uint8_t __far *clear_ptr;
    uint16_t i;
    

    for (plane = 0; plane < 4; plane++) {
        /* Set Sequencer Map Mask for this plane */
        outp(0x3c4, 0x02);      /* SC Index: Map Mask */
        outp(0x3c5, 1 << plane); /* SC Data: plane mask */
        
        /* Clear all rows of this plane */
        for (clear_row = 0; clear_row < 160; clear_row++) {
            clear_offset = RENDERED_MAP_BUFFER + (clear_row * 256);
            clear_ptr = (uint8_t __far *)MK_FP(0xa000, clear_offset);
            
            /* Write 256 bytes of zero to this row */
            for (i = 0; i < 256; i++) {
                clear_ptr[i] = 0;
            }
        }
    }
    

    
    /* Iterate through all tiles in the map (128×10) */
    for (tile_y = 0; tile_y < MAP_HEIGHT_TILES; tile_y++) {
        for (tile_x = 0; tile_x < MAP_WIDTH_TILES; tile_x++) {
            /* Calculate index into tile map */
            tile_index = (uint16_t)tile_y * MAP_WIDTH_TILES + tile_x;
            
            /* Bounds check */
            if (tile_index >= MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
                continue;
            }
            
            /* Get tile ID from current level's tile map */
            if (current_tiles_ptr != NULL) {
                tile_id = current_tiles_ptr[tile_index];
            } else {
                tile_id = 0;  /* No map loaded */
            }
            
            /* Blit this tile to the rendered map buffer */
            if (tile_id != 0) {
                blit_tile_to_map(tile_id, tile_x, tile_y, tileset_graphics);
            }
        }
    }


}


/*
 * load_new_stage - Initialize stage data and render the map
 * 
 * Loads the stage specified by current_stage_number and initializes all game state.
 * 
 * This function should:
 * 1. Set current_tiles_ptr and current_stage_ptr based on current_stage_number
 * 2. Call render_map to display the stage background
 * 3. Handle door entry: if arriving via door (source_door_*), find the reciprocal door
 *    and position Comic in front of it; otherwise use comic_y_checkpoint/comic_x_checkpoint
 * 4. Clear comic_is_teleporting flag (may be set if respawning mid-teleport)
 * 5. Despawn all fireballs by clearing the fireballs array
 * 6. Initialize comic_x and comic_y from checkpoints (or door position)
 * 7. Calculate and set camera_x with bounds clamping:
 *    camera_x = clamp(comic_x - (PLAYFIELD_WIDTH/2 - 1), 0, MAP_WIDTH - PLAYFIELD_WIDTH)
 * 8. Initialize all enemies from current_stage.enemies with:
 *    - Copy behavior, sprite index, and animation data from stage definition
 *    - Set enemy.state = ENEMY_STATE_DESPAWNED (not spawned yet)
 *    - Set enemy.facing = ENEMY_FACING_LEFT
 *    - Set enemy.spawn_timer_and_animation = 20 (spawn 20 ticks after stage load)
 * 
 * Currently, only step 1 is implemented. The remaining steps must be completed
 * for the game to function properly.
 */
void load_new_stage(void)
{
    const level_t *current_level_ptr;
    const stage_t *current_stage_ptr;
    int cam_x;
    
    /* Get the current level data pointer */
    if (current_level_number < 8) {
        current_level_ptr = level_data_pointers[current_level_number];
    } else {
        return;  /* Invalid level */
    }
    
    /* Set current_tiles_ptr based on current stage (0, 1, or 2) */
    if (current_stage_number < 3 && current_level_ptr != NULL) {
        current_stage_ptr = &current_level_ptr->stages[current_stage_number];
        
        /* Determine which tile map to use */
        if (current_stage_number == 0) {
            current_tiles_ptr = pt0.tiles;
        } else if (current_stage_number == 1) {
            current_tiles_ptr = pt1.tiles;
        } else {
            current_tiles_ptr = pt2.tiles;
        }
        
        /* Initialize Comic's position from the first door in the stage
         * (simplified: using first door's position + 1 to center Comic)
         */
        if (current_stage_ptr->doors[0].target_level != DOOR_UNUSED) {
            comic_y = current_stage_ptr->doors[0].y;
            comic_x = current_stage_ptr->doors[0].x + 1;
        } else {
            /* Default position if no doors found */
            comic_x = 12;
            comic_y = 8;
        }
        
        /* Initialize camera to center on Comic, clamped to valid range */
        cam_x = (int)comic_x - (PLAYFIELD_WIDTH - 2) / 2;
        if (cam_x < 0) {
            camera_x = 0;
        } else if (cam_x > MAP_WIDTH - PLAYFIELD_WIDTH) {
            camera_x = MAP_WIDTH - PLAYFIELD_WIDTH;
        } else {
            camera_x = (uint16_t)cam_x;
        }
    } else {
        current_tiles_ptr = NULL;
    }
    
    /* Pre-render the entire map into RENDERED_MAP_BUFFER */
    render_map();

    /* Ensure the initial frame is drawn and displayed: replicate the
     * assembly behavior of blitting and swapping buffers at load time. */
    blit_map_playfield_offscreen();
    blit_comic_playfield_offscreen();
    swap_video_buffers();

#ifdef ENABLE_SHP_SMOKE_TEST
    /* Smoke test: if a 16x32 SHP frame is loaded for shp index 0, blit its
     * first frame at a visible location so we can verify loader correctness. */
    {
        const uint8_t *frame0 = shp_get_frame(0, 0);
        if (frame0 && shp_get_frame_size(0) == 320) {
            blit_sprite_16x32_masked(100, 64, frame0);
            swap_video_buffers();
        }
    }
#endif


}



/*
 * clear_bios_keyboard_buffer - Clear the BIOS keyboard buffer
 * 
 * Clears the BIOS keyboard buffer by setting the tail pointer equal to the
 * head pointer. This prevents old keystrokes from being read.
 */
static void clear_bios_keyboard_buffer(void)
{
    uint8_t __far *bios_data = (uint8_t __far *)0x00000000L;
    uint16_t head = *(uint16_t __far *)(bios_data + BIOS_KEYBOARD_BUFFER_HEAD);
    *(uint16_t __far *)(bios_data + BIOS_KEYBOARD_BUFFER_TAIL) = head;
}

/*
 * dos_idle - Yield CPU time to other processes
 * 
 * Calls DOS idle interrupt (INT 28h) to allow other processes to run.
 * This reduces CPU usage during busy-wait loops in DOS/Windows environments.
 * The call is safe even if no other processes are running.
 */
static void dos_idle(void)
{
    union REGS regs;
    /* Initialize registers to avoid passing garbage values */
    memset(&regs, 0, sizeof(regs));
    /* INT 28h: DOS idle interrupt - yields CPU time */
    int86(0x28, &regs, &regs);
}

/*
 * game_loop - Main game loop
 * 
 * This is the main game loop that runs continuously until the game ends.
 * Each iteration:
 * 1. Clears the BIOS keyboard buffer
 * 2. Waits for a game tick (set by interrupt handler)
 * 3. Processes input and updates game state
 * 4. Renders the frame
 * 5. Handles non-player actors (enemies, fireballs, items)
 * 
 * The loop continues until win_counter reaches 1 (win condition) or the
 * player dies with no remaining lives.
 */
void game_loop(void)
{
    uint16_t tile_addr;
    uint8_t tile_value;
    uint8_t skip_rendering;
    
    while (1) {
        skip_rendering = 0;
        
        /* Clear the BIOS keyboard buffer */
        clear_bios_keyboard_buffer();
        
        /* Busy-wait until int8_handler sets game_tick_flag */
        while (game_tick_flag != 1) {
            /* While waiting, reinitialize comic_jump_counter if Comic is not
             * in the air and the player is not pressing the jump button.
             * This recharge happens continuously during the wait, ensuring the
             * jump counter is always ready when a tick arrives. This provides
             * responsive jump input timing. */
            if (comic_is_falling_or_jumping == 0 && key_state_jump == 0) {
                comic_jump_counter = comic_jump_power;
            }
            
            /* Yield CPU time to reduce CPU usage during wait */
            dos_idle();
        }
        
        /* Clear the tick flag */
        game_tick_flag = 0;
        
        /* Check for win condition
         * When the player wins, win_counter is set to a delay value (e.g., 200).
         * It decrements each tick until it reaches 1, at which point the game
         * end sequence begins. The value 1 (not 0) is the trigger point, allowing
         * win_counter to distinguish between: 0=no win, 1=trigger sequence, >1=counting down */
        if (win_counter != 0) {
            win_counter--;
            if (win_counter == 1) {
                /* TODO: implement game_end_sequence() */
                return;
            }
        }
        
        /* Advance comic_run_cycle in the cycle COMIC_RUNNING_1, COMIC_RUNNING_2, COMIC_RUNNING_3 */
        comic_run_cycle++;
        if (comic_run_cycle > COMIC_RUNNING_3) {
            comic_run_cycle = COMIC_RUNNING_1;
        }
        
        /* Put Comic in standing state by default; may be overridden by input or physics */
        comic_animation = COMIC_STANDING;
        
        /* Award pending HP increase (one unit per tick) */
        if (comic_hp_pending_increase > 0) {
            comic_hp_pending_increase--;
            increment_comic_hp();
        }
        
        /* Handle teleportation */
        if (comic_is_teleporting != 0) {
            handle_teleport();
            /* Teleport handles its own rendering, skip rendering phase */
            skip_rendering = 1;
        }
        /* Handle falling, jumping, and movement only if not teleporting */
        else {
            /* Handle falling or jumping */
            if (comic_is_falling_or_jumping != 0) {
                handle_fall_or_jump();
            }
            /* Check jump input (only if not already falling/jumping) */
            else if (key_state_jump == 1) {
                /* Only jump if comic_jump_counter is not exhausted
                 * comic_jump_counter == 1 means "exhausted" (used as a sentinel value)
                 * This prevents jumping while already in the air */
                if (comic_jump_counter > 1) {
                    comic_is_falling_or_jumping = 1;
                    handle_fall_or_jump();
                }
            } else {
                /* Not pressing jump; recharge jump counter for the next frame.
                 * Note: The busy-wait loop above also recharges the counter
                 * continuously while waiting, providing responsive jump timing.
                 * This recharge ensures the counter is reset after jump key release
                 * in case the condition was not met during the busy-wait. */
                comic_jump_counter = comic_jump_power;
            }
            
            /* Check open input (doors) - only if not falling/jumping */
            if (comic_is_falling_or_jumping == 0 && key_state_open == 1) {
                /* TODO: Check if in front of door and activate it */
                /* This requires stage data structures to be implemented */
            }
            
            /* Check teleport input - only if not falling/jumping */
            if (comic_is_falling_or_jumping == 0 && key_state_teleport == 1 && comic_has_teleport_wand != 0) {
                begin_teleport();
                /* begin_teleport sets comic_is_teleporting, which will be handled
                 * at the top of the next loop iteration. Skip to actor handling. */
                skip_rendering = 1;
            }
            /* Handle left/right movement - only if not falling/jumping and not teleporting */
            else if (comic_is_falling_or_jumping == 0) {
                comic_x_momentum = 0;
                /* Note: If both left and right keys are pressed simultaneously,
                 * right movement takes priority (momentum is set to -5 then
                 * immediately overwritten to +5). This matches the original
                 * assembly behavior. */
                if (key_state_left == 1) {
                    comic_x_momentum = -5;
                    face_or_move_left();
                }
                if (key_state_right == 1) {
                    comic_x_momentum = +5;
                    face_or_move_right();
                }
                
                /* Check for floor beneath Comic */
                /* We check comic_y + 4 to see if there's a solid tile 4 units below Comic.
                 * Valid comic_y values are 0-9 (can occupy any tile row in a 10-tile map).
                 * Adding 4 could put us at y=13 if comic_y=9, which is out of bounds.
                 * The bounds check below (tile_addr < MAP_WIDTH_TILES * MAP_HEIGHT_TILES)
                 * catches this and treats out-of-bounds as passable (no floor). */
                tile_addr = address_of_tile_at_coordinates(comic_x, comic_y + 4);
                /* Look up the tile ID from the current stage's tile map */
                if (current_tiles_ptr != NULL && tile_addr < MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
                    tile_value = current_tiles_ptr[tile_addr];
                } else {
                    tile_value = 0;  /* Treat as passable if no tile map loaded or out of bounds */
                }
                
                /* Check if there's solid ground directly beneath Comic */
                if (tile_value > tileset_last_passable) {
                    /* Primary tile is solid; we're standing on something */
                    /* Do nothing - remain on ground */
                } else {
                    /* Primary tile is passable. If Comic is halfway between tiles,
                     * also check the secondary tile (tile to the right) */
                    uint8_t secondary_tile_solid = 0;
                    if ((comic_x & 1) && current_tiles_ptr != NULL && tile_addr + 1 < MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
                        /* Comic is odd-positioned (halfway between tiles) */
                        tile_value = current_tiles_ptr[tile_addr + 1];
                        if (tile_value > tileset_last_passable) {
                            secondary_tile_solid = 1;
                        }
                    }
                    
                    /* Start falling if both primary and secondary tiles are passable */
                    if (!secondary_tile_solid) {
                        comic_y_vel = 8;  /* Initial falling velocity */
                        
                        /* After walking off edge, Comic has 2 units of momentum */
                        if (comic_x_momentum < 0) {
                            comic_x_momentum = -2;
                        } else if (comic_x_momentum > 0) {
                            comic_x_momentum = +2;
                        }
                        
                        comic_is_falling_or_jumping = 1;
                        /* Set counter to 1 (exhausted) to prevent mid-air jumping */
                        comic_jump_counter = 1;
                    }
                }
            }
        }
        
        /* Check escape key (pause) */
        if (key_state_esc == 1) {
            pause_game();
            /* Wait for escape key release */
            while (key_state_esc == 1) {
                dos_idle();  /* Yield CPU while waiting for key release */
            }
        }
        
        /* Check fire input */
        if (key_state_fire == 1) {
            if (fireball_meter > 0) {
                try_to_fire();
                
                /* fireball_meter increases/decreases at a rate of 1 unit per 2 ticks.
                 * fireball_meter_counter alternates 2, 1, 2, 1, ... to track when to adjust.
                 * When firing: decrement meter when counter is 2 (before decrementing counter)
                 * When not firing: increment meter when counter wraps from 1 to 0 */
                if (fireball_meter_counter != 1) {
                    /* Counter is 2; decrement meter before decrementing counter */
                    decrement_fireball_meter();
                }
            }
        }
        
        /* Always decrement counter (whether firing or not) */
        fireball_meter_counter--;
        if (fireball_meter_counter == 0) {
            /* Counter wrapped from 1 to 0. Increment meter only if NOT firing.
             * When firing: meter decrements every 2 ticks (only on counter==2)
             * When not firing: meter increments every 2 ticks (on counter wrap)
             * This asymmetry creates different rates: -1/2 ticks vs +1/2 ticks */
            if (key_state_fire != 1) {
                /* Not firing; allow meter to recharge */
                increment_fireball_meter();
            }
            /* Always wrap counter back to 2 */
            fireball_meter_counter = 2;
        }
        
        /* Render the map and Comic (unless teleport already handled it) */
        if (!skip_rendering) {
            blit_map_playfield_offscreen();
            blit_comic_playfield_offscreen();
        }
        
        /* Handle enemies, fireballs, and items */
        handle_enemies();
        handle_fireballs();
        handle_item();
        swap_video_buffers();
    }
}

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
