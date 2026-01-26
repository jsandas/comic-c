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
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include "globals.h"
#include "graphics.h"
#include "timing.h"
#include "music.h"
#include "sound.h"
#include "sound_data.h"
#include "sprite_data.h"
#include "level_data.h"
#include "physics.h"
#include "file_loaders.h"
#include "actors.h"
#include "doors.h"

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

/* Debug log file handle (opened lazily) */
static int g_debug_file_handle = -1;

/* Close DEBUG.LOG if open */
void debug_log_close(void)
{
    if (g_debug_file_handle != -1) {
        _close(g_debug_file_handle);
        g_debug_file_handle = -1;
    }
}

/*
 * debug_log - Write debug message to DEBUG.LOG file
 * 
 * Appends formatted text to DEBUG.LOG file. Uses module-level file handle
 * for efficiency (file stays open between calls).
 * 
 * Call debug_log_close() before program termination to close the file handle.
 */
void debug_log(const char *format, ...)
{
    va_list args;
    char buffer[256];
    int len;
    
    /* Open DEBUG.LOG on first call (append mode) */
    if (g_debug_file_handle == -1) {
        g_debug_file_handle = _open("DEBUG.LOG", O_WRONLY | O_CREAT | O_APPEND | O_BINARY,
                          S_IREAD | S_IWRITE);
        if (g_debug_file_handle == -1) {
            return;  /* Can't open file, silently fail */
        }
    }
    
    /* Format the message safely (truncate to buffer size) */
    va_start(args, format);
    /* vsnprintf returns the number of characters that would have been written,
     * excluding the null terminator. The buffer is always null-terminated. */
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    /* Compute actual length to write based on the truncated buffer */
    len = (int)strlen(buffer);
    
    /* Write to file */
    if (len > 0) {
        _write(g_debug_file_handle, buffer, len);
    }
}

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
#define TELEPORT_DISTANCE       6   /* How many game units a teleport moves Comic horizontally */

/*
 * CHEAT CODES (for testing/debugging)
 * ------------------------------------
 * W key - Grant teleport wand (allows testing teleportation without finding item)
 * K key - Grant door key (allows testing door opening without finding item)
 */

/* EGA video memory segment address */
#define VIDEO_MEMORY_BASE       0xa000

/* BIOS keyboard buffer addresses */
#define BIOS_KEYBOARD_BUFFER_HEAD  0x041A
#define BIOS_KEYBOARD_BUFFER_TAIL  0x041C

/* Keyboard scancodes */
#define SCANCODE_ESC    0x01
#define SCANCODE_Q      0x10
#define SCANCODE_W      0x11
#define SCANCODE_O      0x18
#define SCANCODE_K      0x25
#define SCANCODE_L      0x26
#define SCANCODE_S      0x1F
#define SCANCODE_C      0x2E
#define SCANCODE_B      0x30

/* Global variables for initialization and game state */
static uint8_t interrupt_handler_install_sentinel = 0;
static volatile uint8_t game_tick_flag = 0;
static uint16_t max_joystick_reads = 0;
static uint16_t saved_video_mode = 0;
uint8_t current_level_number = LEVEL_NUMBER_FOREST;
uint8_t current_stage_number = 0;
level_t current_level;

/* Stage entry tracking (-2=first spawn, -1=boundary, >=0=door from level) */
int8_t source_door_level_number = -2;
int8_t source_door_stage_number = 0;

/* Checkpoint position (for respawn and boundary crossing) */
uint8_t comic_y_checkpoint = 12;
uint8_t comic_x_checkpoint = 14;

/* Game state variables */
uint8_t win_counter = 0;
uint8_t comic_x = 0;
uint8_t comic_y = 0;
uint8_t comic_animation = COMIC_STANDING;
uint8_t comic_facing = COMIC_FACING_RIGHT;
uint8_t comic_run_cycle = COMIC_RUNNING_1;
uint8_t comic_is_falling_or_jumping = 0;
uint8_t comic_is_teleporting = 0;
int8_t comic_x_momentum = 0;
int8_t comic_y_vel = 0;
uint8_t comic_jump_counter = 0;  /* Current jump countdown (0 when standing) */
uint8_t comic_jump_power = JUMP_POWER_DEFAULT;  /* Base jump power (4 default, 5 with Boots) */
uint8_t ceiling_stick_flag = 0;  /* Whether Comic is jumping upward against a ceiling */
uint8_t comic_fall_delay = 0;  /* Ticks to hover at jump apex */
const level_t *current_level_ptr = NULL;  /* Pointer to current level data */
uint8_t comic_hp = 0;  /* Start at 0, will fill to MAX_HP via pending_increase */
uint8_t comic_hp_pending_increase = MAX_HP;  /* Fill HP at game start */
uint16_t camera_x = 0;
/* Landing sentinel: set by physics when hitting ground; clears each tick */
uint8_t landed_this_tick = 0;

/* Death animation state - prevents re-entering death animation during enemy collisions */
uint8_t inhibit_death_by_enemy_collision = 0;
/* Flag set after death animation finishes to skip partial sprite rendering in comic_dies */
uint8_t comic_death_animation_finished = 0;

/* Teleport state */
static uint8_t teleport_animation = 0;
static uint8_t teleport_source_x = 0;
static uint8_t teleport_source_y = 0;
static uint8_t teleport_destination_x = 0;
static uint8_t teleport_destination_y = 0;
static uint8_t teleport_camera_counter = 0;
static int8_t teleport_camera_vel = 0;

/* Input state variables (set by keyboard interrupt handler) */
uint8_t key_state_esc = 0;
uint8_t key_state_jump = 0;
uint8_t key_state_fire = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;
uint8_t key_state_open = 0;
uint8_t key_state_teleport = 0;
static uint8_t key_state_cheat_wand = 0;  /* W key for teleport wand cheat */
static uint8_t key_state_cheat_key = 0;   /* K key for door key cheat */
static uint8_t key_state_cheat_cola = 0;  /* B key for Blastola Cola cheat */
static uint8_t key_state_cheat_corkscrew = 0; /* C key for Corkscrew cheat */
static uint8_t key_state_cheat_boots = 0; /* O key for Boots cheat */
static uint8_t key_state_cheat_lantern = 0; /* L key for Lantern cheat */
static uint8_t key_state_cheat_shield = 0; /* S key for Shield cheat */

/* Previous frame input state (used for edge-triggered input like jump) */
static uint8_t previous_key_state_jump = 0;
static uint8_t previous_key_state_teleport = 0;
static uint8_t previous_key_state_cheat_wand = 0;
static uint8_t previous_key_state_cheat_key = 0;
static uint8_t previous_key_state_cheat_cola = 0;
static uint8_t previous_key_state_cheat_corkscrew = 0;
static uint8_t previous_key_state_cheat_boots = 0;
static uint8_t previous_key_state_cheat_lantern = 0;
static uint8_t previous_key_state_cheat_shield = 0;

/* Flag set when teleport key edge is detected in update_keyboard_input() */
static uint8_t teleport_key_pressed = 0;

/* Minimum jump frames counter - ensures quick taps get at least 2 frames of acceleration */
uint8_t minimum_jump_frames = 0;

/* Fireball state */
static uint8_t fireball_meter = 0;   /* Start with empty fireball meter */
static uint8_t fireball_meter_counter = 2;

/* Item collection state */
uint8_t comic_has_door_key = 0;
uint8_t comic_has_teleport_wand = 0;
uint8_t comic_firepower = 0;           /* Number of active fireball slots (0-5) */
uint8_t comic_has_corkscrew = 0;       /* 1 if Corkscrew item collected */
uint8_t comic_has_shield = 0;          /* 1 if Shield item collected */
uint8_t comic_has_lantern = 0;         /* 1 if Lantern item collected (only affects darkness in Castle level) */

/* Score - 3 bytes (24-bit value) to store up to 999,999 points */
uint8_t score_bytes[3] = {0, 0, 0};  /* Little-endian: byte 0 is LSB, byte 2 is MSB */
/* Score macros (score_get_value and score_set_value) are defined in globals.h */

/* Item collection tracking (per level and stage) */
uint8_t items_collected[8][16];        /* Bitmap: items_collected[level][stage] */
uint8_t item_animation_counter = 0;    /* 0 or 1, toggles for item animation */

/* Enemy respawn timer cycle */
uint8_t enemy_respawn_counter_cycle = 20; /* Cycles: 20→40→60→80→100→20 */

/* Tileset buffer - holds data from .TT2 file */
uint8_t tileset_last_passable;
uint8_t tileset_flags;
uint8_t tileset_graphics[128 * 128];  /* Up to 128 16x16 tiles */

/* Stage data - three .PT files per level */
static pt_file_t pt0;
static pt_file_t pt1;
static pt_file_t pt2;
uint8_t *current_tiles_ptr = NULL;  /* Points to current stage's tile map */
static uint8_t comic_num_lives = 0;
uint8_t comic_num_treasures = 0;  /* Number of treasures collected (CROWN, GOLD, GEMS - 0-3). When == 3, triggers victory sequence */

/* Offscreen buffer pointer (0x0000 or 0x2000) - start with A as offscreen when B is displayed */
uint16_t offscreen_video_buffer_ptr = GRAPHICS_BUFFER_GAMEPLAY_A;

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
void blit_map_playfield_offscreen(void);
void blit_comic_playfield_offscreen(void);
void blit_comic_partial_playfield_offscreen(uint16_t max_height);
void swap_video_buffers(void);
void render_inventory_display(void);
void render_score_display(void);
static void clear_bios_keyboard_buffer(void);
static void clear_scancode_queue(void);
static void update_keyboard_input(void);
static void handle_cheat_codes(void);
static void game_over(void);
static void game_end_sequence(void);
void award_points(uint16_t points);  /* Exported for use by actors.c */

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

/* Keyboard scancode capture buffer (direct from INT 9) */
#define MAX_SCANCODE_QUEUE 16
static uint8_t scancode_queue[MAX_SCANCODE_QUEUE];
static uint8_t scancode_queue_head = 0;
static uint8_t scancode_queue_tail = 0;

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
 * Reads the keyboard scancode, stores it in a queue, and calls the original handler.
 */
static void __interrupt int9_handler(void)
{
    uint8_t scancode;
    uint8_t next_head;
    
    /* Read keyboard scancode from port 0x60 */
    scancode = inp(0x60);
    
    /* Store scancode in queue if there's room */
    next_head = (scancode_queue_head + 1) % MAX_SCANCODE_QUEUE;
    if (next_head != scancode_queue_tail) {
        scancode_queue[scancode_queue_head] = scancode;
        scancode_queue_head = next_head;
    }
    
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
 * - Clear keyboard buffers to prevent key leakage
 * - Mutes the PC speaker
 * - Restores the saved video mode
 * - Exits to DOS
 */
void terminate_program(void)
{
    union REGS regs;
    uint8_t port_value;

    /* Close debug log if open */
    debug_log_close();
    
    /* Restore original interrupt handlers */
    restore_interrupt_handlers();
    
    /* Clear keyboard buffers to prevent key leakage to terminal */
    clear_bios_keyboard_buffer();
    clear_scancode_queue();
    
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
 * Plays the extra life sound, increments the number of lives if not already at maximum,
 * and updates the UI display with a bright life icon.
 */
void award_extra_life(void)
{
    uint16_t x_pixel, y_pixel;
    
    /* Play the extra life sound */
    play_sound(SOUND_EXTRA_LIFE, 4);
    
    /* Check if already at max lives */
    if (comic_num_lives >= MAX_NUM_LIVES) {
        /* Already at maximum lives: refill HP and award 22500 points (75 + 75 + 75 = 225 * 100)
         * Set HP refill unconditionally - if HP is already full, increment_comic_hp will award bonus points */
        comic_hp_pending_increase = MAX_HP;
        award_points(75);
        award_points(75);
        award_points(75);
        return;
    }
    
    /* Increment lives if below max */
    comic_num_lives++;
    
    /* Display the bright life icon at the appropriate position
     * Icons start at x=24, then each life is at x = 24 + (life_count * 24) pixels
     * After incrementing, comic_num_lives contains the NEW count */
    x_pixel = 24 + (comic_num_lives * 24);
    y_pixel = 180;
    
    /* Blit the bright life icon to both gameplay buffers */
    blit_sprite_16x16_masked(x_pixel, y_pixel, sprite_life_icon_bright);
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
 *    - File open and header read are CRITICAL: returns -1 if they fail
 *    - Incomplete data read is non-critical: logs warning but continues with partial tileset
 * 3. Loads the three .PT files (stage maps) into pt0, pt1, pt2 structures - logs warnings on failure
 * 4. Performs lantern check for castle level (TODO: implement lantern blackout)
 * 5. TODO: Load .SHP files for enemy sprites
 * 
 * Error Handling:
 *   - Tileset (TT2) file open failure: CRITICAL, returns -1 (cannot render level)
 *   - Tileset (TT2) header read failure: CRITICAL, returns -1 (corrupted file)
 *   - Tileset (TT2) data read incomplete: CRITICAL, returns -1 (no background for level)
 *   - Map (PT) failures: logs warning but continues, uses empty map
 *   - Sprite (SHP) failures: logged by load_level_shp_files, continues
 * 
 * Input:
 *   current_level_number = level to load (0-7)
 * 
 * Output:
 *   current_level = filled with level data
 *   tileset_graphics = array of tile images from .TT2 file (may be partial)
 *   pt0, pt1, pt2 = stage maps from .PT files
 * 
 * Returns:
 *   0 on success (files opened and headers read successfully)
 *   -1 on critical error (invalid level number, file open failed, or header read failed)
 */
int load_new_level(void)
{
    int file_handle;
    unsigned bytes_read;
    const level_t* source_level;
    unsigned i;
    
    /* TT2 file header structure */
    struct {
        uint8_t last_passable;
        uint8_t unused1;
        uint8_t unused2;
        uint8_t flags;
    } tt2_header;
    
    /* Validate level number */
    if (current_level_number >= 8) {
        fprintf(stderr, "ERROR: load_new_level: Invalid level number %d (must be 0-7)\n", 
                current_level_number);
        return -1;
    }
    
    /* Copy level data from static data to current_level */
    source_level = level_data_pointers[current_level_number];
    if (source_level == NULL) {
        fprintf(stderr, "ERROR: load_new_level: Level %d data pointer is NULL\n", 
                current_level_number);
        return -1;
    }
    memcpy(&current_level, source_level, sizeof(level_t));
    
    /* Load the .TT2 file (tileset graphics) - CRITICAL */
    file_handle = _open(current_level.tt2_filename, O_RDONLY | O_BINARY);
    if (file_handle == -1) {
        /* Tileset file not found - this is a critical failure */
        fprintf(stderr, "ERROR: load_new_level: Cannot open tileset file '%s' for level %d\n",
                current_level.tt2_filename, current_level_number);
        return -1;
    }
    
    /* Read TT2 file header (4 bytes) */
    bytes_read = _read(file_handle, &tt2_header, sizeof(tt2_header));
    if (bytes_read != sizeof(tt2_header)) {
        /* Failed to read header - this is a critical failure */
        fprintf(stderr, "ERROR: load_new_level: Failed to read TT2 header from '%s' "
                "(expected %u bytes, got %u)\n",
                current_level.tt2_filename, (unsigned)sizeof(tt2_header), bytes_read);
        _close(file_handle);
        return -1;
    }
    
    /* Extract header fields */
    tileset_last_passable = tt2_header.last_passable;
    tileset_flags = tt2_header.flags;

    /* The TT2 file format is: 4-byte header + uncompressed tile data (variable size).
     * Each tile is 128 bytes (4 planes × 32 bytes per plane).
     * Not all levels use 128 tiles - some use fewer, so file sizes vary (~4KB to ~11KB).
     * We read as much as available, then zero-fill the rest of the buffer.
     * NOTE: _read() may not return all requested bytes in one call, so we loop until EOF. */
    {
        uint8_t *buf_ptr = tileset_graphics;
        unsigned bytes_remaining = sizeof(tileset_graphics);
        unsigned total_bytes_read = 0;
        
        while (bytes_remaining > 0) {
            bytes_read = _read(file_handle, buf_ptr, bytes_remaining);
            if (bytes_read == 0) {
                /* End of file reached - this is expected for variable-size TT2 files */
                break;
            }
            if (bytes_read == -1) {
                /* Read error */
                _close(file_handle);
                fprintf(stderr, "ERROR: load_new_level: Read error while loading TT2 data from '%s'\n",
                        current_level.tt2_filename);
                return -1;
            }
            total_bytes_read += bytes_read;
            buf_ptr += bytes_read;
            bytes_remaining -= bytes_read;
        }
        
        _close(file_handle);
        
        /* Zero-fill any remaining space in the tileset buffer */
        if (bytes_remaining > 0) {
            memset(buf_ptr, 0, bytes_remaining);
        }
        
        bytes_read = total_bytes_read;
    }
    
    /* Log how much tileset data we read (should be at least some amount > 0) */
    if (bytes_read == 0) {
        fprintf(stderr, "WARNING: load_new_level: No tileset data read from '%s'. Level may render incorrectly.\n",
                current_level.tt2_filename);
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
    
    /* Load the three .PT files for this level - non-critical */
    if (load_pt_file(current_level.pt0_filename, &pt0) != 0) {
        fprintf(stderr, "WARNING: load_new_level: Failed to load primary map '%s'\n",
                current_level.pt0_filename);
        /* Initialize with empty map if load fails */
        memset(&pt0, 0, sizeof(pt0));
        pt0.width = MAP_WIDTH_TILES;
        pt0.height = MAP_HEIGHT_TILES;
    }
    
    if (load_pt_file(current_level.pt1_filename, &pt1) != 0) {
        fprintf(stderr, "WARNING: load_new_level: Failed to load secondary map '%s'\n",
                current_level.pt1_filename);
        /* Initialize with empty map if load fails */
        memset(&pt1, 0, sizeof(pt1));
        pt1.width = MAP_WIDTH_TILES;
        pt1.height = MAP_HEIGHT_TILES;
    }
    
    if (load_pt_file(current_level.pt2_filename, &pt2) != 0) {
        fprintf(stderr, "WARNING: load_new_level: Failed to load tertiary map '%s'\n",
                current_level.pt2_filename);
        /* Initialize with empty map if load fails */
        memset(&pt2, 0, sizeof(pt2));
        pt2.width = MAP_WIDTH_TILES;
        pt2.height = MAP_HEIGHT_TILES;
    }
    
    /* Load .SHP files referenced by this level into runtime cache */
    load_level_shp_files(&current_level);

    return 0;  /* Success - tileset loaded and at least one playable state achieved */
}

/*
 * Stub implementations for game loop helper functions
 * These will be implemented progressively as the refactor continues
 */

static void handle_teleport(void)
{
    /* Teleportation animation handler - 6 frames (0-5)
     * Draws teleport effect at both source and destination positions
     * with a 1-frame delay at destination. Camera moves during early frames. */
    const uint8_t *teleport_sprites[5];
    const uint8_t *sprite_ptr;
    uint8_t anim_frame;
    int16_t rel_x;
    uint16_t pixel_x, pixel_y;
    
    /* Teleport animation sprite table (5 frames) */
    teleport_sprites[0] = sprite_teleport_0_16x32m;
    teleport_sprites[1] = sprite_teleport_1_16x32m;
    teleport_sprites[2] = sprite_teleport_2_16x32m;
    teleport_sprites[3] = sprite_teleport_1_16x32m;  /* Frame 1 repeated */
    teleport_sprites[4] = sprite_teleport_0_16x32m;  /* Frame 0 repeated */
    
    /* Move camera if counter is non-zero */
    if (teleport_camera_counter > 0) {
        /* Use signed arithmetic to prevent underflow, then clamp to valid range */
        int16_t new_camera_x = (int16_t)camera_x + teleport_camera_vel;
        camera_x = (new_camera_x < 0) ? 0 : (uint8_t)new_camera_x;
        teleport_camera_counter--;
    }
    
    /* Render map background */
    blit_map_playfield_offscreen();
    
    /* Update Comic's actual position at frame 3 */
    if (teleport_animation == 3) {
        comic_y = teleport_destination_y;
        comic_x = teleport_destination_x;
    }
    
    /* Blit Comic sprite */
    blit_comic_playfield_offscreen();
    
    /* Blit teleport animation at source position (frames 0-4) */
    anim_frame = teleport_animation;
    if (anim_frame < 5) {
        rel_x = (int16_t)((int)teleport_source_x - (int)camera_x);
        if (rel_x >= 0 && rel_x < PLAYFIELD_WIDTH) {
            pixel_x = 8 + (rel_x * 8);
            pixel_y = 8 + (teleport_source_y * 8);
            sprite_ptr = teleport_sprites[anim_frame];
            blit_sprite_16x32_masked(pixel_x, pixel_y, sprite_ptr);
        }
    }
    
    /* Blit teleport animation at destination (frames 1-5, delayed by 1) */
    if (teleport_animation >= 1) {
        anim_frame = teleport_animation - 1;  /* Delayed by 1 frame */
        if (anim_frame < 5) {
            rel_x = (int16_t)((int)teleport_destination_x - (int)camera_x);
            if (rel_x >= 0 && rel_x < PLAYFIELD_WIDTH) {
                pixel_x = 8 + (rel_x * 8);
                pixel_y = 8 + (teleport_destination_y * 8);
                sprite_ptr = teleport_sprites[anim_frame];
                blit_sprite_16x32_masked(pixel_x, pixel_y, sprite_ptr);
            }
        }
    }
    
    /* Advance animation frame */
    teleport_animation++;
    
    /* End teleport at frame 6 */
    if (teleport_animation >= 6) {
        comic_is_teleporting = 0;
    }
}

/* Physics functions implemented in physics.c */
/* handle_fall_or_jump is defined in physics.c */

static void begin_teleport(void)
{
    /* Find teleport destination and initiate teleport animation
     * Teleports 6 units in facing direction, finds safe landing spot */
    uint8_t dest_x, dest_y;
    int16_t camera_rel_x;
    int16_t dest_camera_rel;
    int16_t camera_movement;
    uint8_t search_y;
    uint8_t solid_found;
    uint8_t nonsolid_count;
    uint8_t max_camera_x;
    
    /* Initialize destination to Comic's current position */
    dest_x = comic_x;
    dest_y = comic_y;
    
    /* Calculate camera-relative position of Comic */
    camera_rel_x = (int16_t)((int)comic_x - (int)camera_x);
    
    /* Initialize camera movement */
    teleport_camera_counter = 0;
    
    /* Determine destination based on facing direction */
    if (comic_facing == COMIC_FACING_LEFT) {
        teleport_camera_vel = -1;  /* Move camera left */
        
        /* Check if too close to left edge */
        if (camera_rel_x < TELEPORT_DISTANCE) {
            goto remain_in_place;
        }
        
        dest_x -= TELEPORT_DISTANCE;
        
        /* Calculate camera movement needed */
        dest_camera_rel = camera_rel_x - TELEPORT_DISTANCE;
        if (dest_camera_rel < (PLAYFIELD_WIDTH / 2 - 2)) {
            /* Destination is left of center, need to move camera */
            camera_movement = (PLAYFIELD_WIDTH / 2 - 2) - dest_camera_rel;
            if (camera_x >= camera_movement) {
                teleport_camera_counter = (uint8_t)camera_movement;
            } else {
                teleport_camera_counter = camera_x;
            }
        }
    } else {
        /* Facing right */
        teleport_camera_vel = 1;  /* Move camera right */
        
        /* Check if too close to right edge */
        if (camera_rel_x >= (PLAYFIELD_WIDTH - TELEPORT_DISTANCE - 1)) {
            goto remain_in_place;
        }
        
        dest_x += TELEPORT_DISTANCE;
        
        /* Calculate camera movement needed */
        dest_camera_rel = camera_rel_x + TELEPORT_DISTANCE;
        if (dest_camera_rel > (PLAYFIELD_WIDTH / 2)) {
            /* Destination is right of center, need to move camera */
            max_camera_x = MAP_WIDTH - PLAYFIELD_WIDTH;
            camera_movement = dest_camera_rel - (PLAYFIELD_WIDTH / 2);
            if ((camera_x + camera_movement) <= max_camera_x) {
                teleport_camera_counter = (uint8_t)camera_movement;
            } else {
                teleport_camera_counter = (uint8_t)(max_camera_x - camera_x);
            }
        }
    }
    
    /* Round destination x to even tile boundary */
    dest_x &= 0xFE;
    
    /* Search for safe landing spot (solid tile with 2 empty tiles above) */
    search_y = PLAYFIELD_HEIGHT - 2;  /* Start from bottom */
    solid_found = 0;
    
    while (search_y > 0 && !solid_found) {
        uint16_t tile_offset = address_of_tile_at_coordinates(dest_x / 2, search_y / 2);
        uint8_t tile_id = current_tiles_ptr[tile_offset];
        
        if (tile_id > tileset_last_passable) {
            /* Found solid tile, now check for 2 non-solid tiles above */
            uint8_t check_y = search_y;
            nonsolid_count = 0;
            
            while (check_y >= 2 && nonsolid_count < 2) {
                check_y -= 2;
                tile_offset = address_of_tile_at_coordinates(dest_x / 2, check_y / 2);
                tile_id = current_tiles_ptr[tile_offset];
                if (tile_id > tileset_last_passable) {
                    /* Hit another solid tile, insufficient clearance */
                    break;
                } else {
                    nonsolid_count++;
                }
            }
            
            if (nonsolid_count == 2) {
                /* Found safe landing - 2 empty tiles above solid */
                dest_y = check_y;
                solid_found = 1;
                break;
            }
            /* If insufficient clearance, continue searching downward */
        }
        search_y -= 2;
    }
    
    if (!solid_found) {
        goto remain_in_place;
    }
    
    goto finish_teleport;
    
remain_in_place:
    /* No safe destination found, teleport to current position */
    dest_x = comic_x;
    dest_y = comic_y;
    teleport_camera_counter = 0;
    
finish_teleport:
    /* Initialize teleport state */
    teleport_animation = 0;
    teleport_source_x = comic_x;
    teleport_source_y = comic_y;
    teleport_destination_x = dest_x;
    teleport_destination_y = dest_y;
    comic_is_teleporting = 1;
    
    /* Play teleport sound */
    play_sound(SOUND_TELEPORT, 2);
}

/* face_or_move_left and face_or_move_right are implemented in game_main.c */

static void pause_game(void)
{
    /* Display pause screen and wait for keypress
     * Renders the current game state with a pause graphic overlay, then loops
     * waiting for keyboard input. If 'Q' or 'q' is pressed, terminates the game.
     * Any other key returns to gameplay.
     * Uses the game's own keyboard system (scancode queue from INT 9 handler)
     * to avoid conflicts with BIOS INT 16h. */
    uint8_t scancode;
    uint8_t is_break;
    uint8_t code;
    
    /* Reset key_state_esc so the main loop doesn't get stuck waiting for release */
    key_state_esc = 0;
    
    /* Clear both keyboard buffers before entering pause loop */
    clear_bios_keyboard_buffer();
    clear_scancode_queue();
    
    /* Loop until we get a valid key to unpause or quit */
    while (1) {
        
        /* Render current game state to offscreen buffer */
        blit_map_playfield_offscreen();
        
        /* Blit pause graphic (128x48 pixels positioned at X=40, Y=64) */
        blit_wxh(offscreen_video_buffer_ptr + ((64 * 40) + (40 / 8)),
                 sprite_pause_128x48,
                 128 / 8,  /* 128 pixels = 16 bytes */
                 48);
        
        /* Swap buffers to display the pause screen */
        swap_video_buffers();
        wait_n_ticks(1);
        
        /* Check if any scancode is available in the queue */
        if (scancode_queue_head != scancode_queue_tail) {
            /* Get next scancode from queue */
            scancode = scancode_queue[scancode_queue_tail];
            scancode_queue_tail = (scancode_queue_tail + 1) % MAX_SCANCODE_QUEUE;
            
            /* Skip extended key prefix (0xE0) - just like update_keyboard_input() does */
            if (scancode == 0xE0) {
                continue;
            }
            
            /* Handle break (key release) codes - skip them */
            is_break = (scancode & 0x80) != 0;
            code = scancode & 0x7F;
            
            /* Only process key press events (not key releases) */
            if (!is_break) {
                if (code == SCANCODE_Q) {
                    terminate_program();
                } else {
                    /* Any other key returns to the game */
                    return;
                }
            }
        }
    }
}

static void beam_in(void)
{
    /* Beam-in animation when Comic starts the game
     * 12-frame materialize animation, Comic appears after frame 6 */
    const uint8_t *materialize_sprites[12];
    uint8_t frame;
    int16_t rel_x;
    uint16_t pixel_x, pixel_y;
    const uint8_t *sprite_ptr;
    
    /* Materialize animation sprite table (12 frames) */
    materialize_sprites[0] = sprite_materialize_0_16x32m;
    materialize_sprites[1] = sprite_materialize_1_16x32m;
    materialize_sprites[2] = sprite_materialize_2_16x32m;
    materialize_sprites[3] = sprite_materialize_3_16x32m;
    materialize_sprites[4] = sprite_materialize_4_16x32m;
    materialize_sprites[5] = sprite_materialize_5_16x32m;
    materialize_sprites[6] = sprite_materialize_6_16x32m;
    materialize_sprites[7] = sprite_materialize_7_16x32m;
    materialize_sprites[8] = sprite_materialize_8_16x32m;
    materialize_sprites[9] = sprite_materialize_9_16x32m;
    materialize_sprites[10] = sprite_materialize_10_16x32m;
    materialize_sprites[11] = sprite_materialize_11_16x32m;
    
    /* Initial delay: 15 ticks with map visible */
    for (frame = 0; frame < 15; frame++) {
        blit_map_playfield_offscreen();
        handle_item();  /* Render items */
        swap_video_buffers();
        wait_n_ticks(1);
    }
    
    /* Play materialize sound */
    play_sound(SOUND_MATERIALIZE, 4);
    
    /* Animate materialize effect (12 frames) */
    for (frame = 0; frame < 12; frame++) {
        /* Render background */
        blit_map_playfield_offscreen();
        
        /* Render items */
        handle_item();
        
        /* Blit materialize animation frame - this shows Comic fading in */
        rel_x = (int16_t)((int)comic_x - (int)camera_x);
        if (rel_x >= 0 && rel_x < PLAYFIELD_WIDTH) {
            pixel_x = 8 + (rel_x * 8);
            pixel_y = 8 + (comic_y * 8);
            sprite_ptr = materialize_sprites[frame];
            blit_sprite_16x32_masked(pixel_x, pixel_y, sprite_ptr);
        }
        
        /* Show full Comic sprite after frame 6 to make Comic fully visible */
        if (frame > 6) {
            blit_comic_playfield_offscreen();
        }
        
        /* Swap buffers and wait */
        swap_video_buffers();
        wait_n_ticks(1);
    }
    
    /* Final frame with Comic */
    blit_map_playfield_offscreen();
    handle_item();  /* Render items */
    blit_comic_playfield_offscreen();
    swap_video_buffers();
    wait_n_ticks(1);
}

static void beam_out(void)
{
    /* Beam-out animation when Comic wins the game
     * 12-frame materialize animation with inverted Comic visibility (Comic disappears after frame 6) */
    const uint8_t *materialize_sprites[12];
    uint8_t frame;
    int16_t rel_x;
    uint16_t pixel_x, pixel_y;
    const uint8_t *sprite_ptr;
    
    /* Materialize animation sprite table (12 frames) */
    materialize_sprites[0] = sprite_materialize_0_16x32m;
    materialize_sprites[1] = sprite_materialize_1_16x32m;
    materialize_sprites[2] = sprite_materialize_2_16x32m;
    materialize_sprites[3] = sprite_materialize_3_16x32m;
    materialize_sprites[4] = sprite_materialize_4_16x32m;
    materialize_sprites[5] = sprite_materialize_5_16x32m;
    materialize_sprites[6] = sprite_materialize_6_16x32m;
    materialize_sprites[7] = sprite_materialize_7_16x32m;
    materialize_sprites[8] = sprite_materialize_8_16x32m;
    materialize_sprites[9] = sprite_materialize_9_16x32m;
    materialize_sprites[10] = sprite_materialize_10_16x32m;
    materialize_sprites[11] = sprite_materialize_11_16x32m;
    
    /* Play materialize sound */
    play_sound(SOUND_MATERIALIZE, 4);
    
    /* Initial frame with Comic */
    blit_map_playfield_offscreen();
    blit_comic_playfield_offscreen();
    swap_video_buffers();
    wait_n_ticks(1);
    
    /* Animate materialize effect (12 frames) */
    for (frame = 0; frame < 12; frame++) {
        /* Render background */
        blit_map_playfield_offscreen();
        
        /* Show Comic sprite for first 6 frames */
        if (frame < 6) {
            blit_comic_playfield_offscreen();
        }
        
        /* Blit materialize animation frame */
        rel_x = (int16_t)((int)comic_x - (int)camera_x);
        if (rel_x >= 0 && rel_x < PLAYFIELD_WIDTH) {
            pixel_x = 8 + (rel_x * 8);
            pixel_y = 8 + (comic_y * 8);
            sprite_ptr = materialize_sprites[frame];
            blit_sprite_16x32_masked(pixel_x, pixel_y, sprite_ptr);
        }
        
        /* Swap buffers and wait */
        swap_video_buffers();
        wait_n_ticks(1);
    }
    
    /* Final frame without Comic */
    blit_map_playfield_offscreen();
    swap_video_buffers();
    
    /* Wait 6 additional ticks */
    wait_n_ticks(6);
}

static void decrement_fireball_meter(void)
{
    uint8_t old_meter_val;
    uint8_t cell_num;
    uint16_t x_pos;
    const uint8_t __far *sprite_to_render;
    
    if (fireball_meter == 0) {
        return;
    }
    
    /* Save old meter value before decrementing */
    old_meter_val = fireball_meter;
    
    /* Decrement the meter */
    fireball_meter--;
    
    /* Calculate cell position using (old_meter + 1) / 2
     * This matches the assembly: mov ah, al; inc al; shr al, 1 */
    cell_num = (old_meter_val + 1) >> 1;
    
    /* No need to cap - the offset naturally goes up to 6 for meter 12 */
    
    /* Determine sprite based on OLD meter value (before decrement):
     * If OLD meter is odd: GRAPHIC_METER_EMPTY
     * If OLD meter is even: GRAPHIC_METER_HALF */
    if (old_meter_val & 1) {
        /* Old meter was odd - draw empty cell */
        sprite_to_render = sprite_meter_empty_8x16;
    } else {
        /* Old meter was even - draw half-full cell */
        sprite_to_render = sprite_meter_half_8x16;
    }
    
    /* Calculate X position: base at 240 (not 248), add cell_num * 8 pixels
     * This matches the assembly: pixel_coords(240, 54) + ax */
    x_pos = 240 + (cell_num << 3);
    
    /* Render the meter cell at Y=54 */
    blit_8x16_sprite(x_pos, 54, sprite_to_render);
}

static void increment_fireball_meter(void)
{
    uint8_t meter_val;
    uint8_t cell_num;
    uint16_t x_pos;
    const uint8_t __far *sprite_to_render;
    
    if (fireball_meter > MAX_FIREBALL_METER - 1) {
        return;
    }
    
    fireball_meter++;
    
    /* After incrementing, use the NEW meter value for calculations */
    meter_val = fireball_meter;
    
    /* Calculate which cell to display: cell_num = (meter - 1) / 2
     * This maps: meter 1-2→cell 0, 3-4→cell 1, 5-6→cell 2, ..., 11-12→cell 5 */
    cell_num = (meter_val - 1) >> 1;
    
    /* Cap cell at 5 to stay within the 6-cell display box */
    if (cell_num > 5) {
        cell_num = 5;
    }
    
    /* Determine which sprite to render for the final cell:
     * If meter is odd: GRAPHIC_METER_HALF
     * If meter is even: GRAPHIC_METER_FULL */
    if (meter_val & 1) {
        /* Odd - draw half-full cell */
        sprite_to_render = sprite_meter_half_8x16;
    } else {
        /* Even - draw full cell */
        sprite_to_render = sprite_meter_full_8x16;
    }
    
    /* Calculate X position: base at 248 (not 240), add 8 pixels per cell
     * Since video buffer is in bytes (40 bytes = 320 pixels), each byte = 8 pixels */
    x_pos = 248 + (cell_num << 3);
    
    /* Render the meter cell at Y=54 */
    blit_8x16_sprite(x_pos, 54, sprite_to_render);
}

void blit_map_playfield_offscreen(void)
{
    /* Blit the visible playfield region from the rendered map buffer
     * into the offscreen video buffer, plane by plane.
     * Uses EGA planar layout: all planes at same offsets, selected via plane registers.
     *  - camera_x is in game units (1 unit == 8 pixels == 1 byte horizontally)
     *  - Playfield top-left pixel is at (8,8)
     *  - Rendered map uses 256-byte stride; maximum valid camera_x = 256 - PLAYFIELD_WIDTH
     */
    const uint16_t screen_bytes_per_row = SCREEN_WIDTH / 8; /* 40 */
    const uint16_t rendered_bytes_per_row = 256; /* Rendered map uses 256-byte stride */
    const uint16_t playfield_pixel_rows = PLAYFIELD_HEIGHT * 8; /* 20 * 8 = 160 */
    const uint16_t playfield_bytes_per_row = PLAYFIELD_WIDTH; /* 24 bytes */
    const uint16_t max_camera_x = rendered_bytes_per_row - playfield_bytes_per_row; /* 232 */
    uint8_t plane;
    uint16_t row;

    /* Validate camera position is within rendered map bounds.
     * If camera_x exceeds max_camera_x, the max_src_bytes calculation would underflow.
     * This should never happen if load_new_stage correctly bounds camera_x, but we
     * validate here as a safety check. */
    if (camera_x > max_camera_x) {
        /* Invalid camera position; cannot safely render */
        return;
    }

    for (plane = 0; plane < 4; plane++) {
        /* Enable reading and writing for this plane */
        enable_ega_plane_read_write(plane);

        for (row = 0; row < playfield_pixel_rows; row++) {
            /* Source offset calculation using larger type to prevent overflow
             * Max value: 0x4000 + (159 * 256) + camera_x ≈ 57K, fits in uint16_t
             * but use unsigned long for intermediate to be safe */
            unsigned long src_calc = RENDERED_MAP_BUFFER + ((unsigned long)row * rendered_bytes_per_row) + camera_x;
            uint16_t src_offset;
            uint16_t dst_offset;
            uint16_t max_src_bytes;
            uint16_t bytes_to_copy;
            
            /* Validate calculation fits within segment before assigning to uint16_t */
            if (src_calc > 65535UL) {
                /* Source offset would overflow; skip this row */
                continue;
            }
            src_offset = (uint16_t)src_calc;
            
            dst_offset = offscreen_video_buffer_ptr + ((8 + row) * screen_bytes_per_row) + (8 / 8);

            /* Calculate maximum bytes available to read from this row.
             * Formula: remaining bytes in row = row_start + 256 - src_offset_within_row
             * Which simplifies to: (256 - camera_x - 0) = 256 - camera_x bytes available
             * for the first row. But this depends on the row number in the rendered buffer.
             * 
             * Safer approach: bytes available = bytes from camera_x to end of rendered row
             * = rendered_bytes_per_row - camera_x = 256 - camera_x
             * (This is constant across all rows since stride is uniform) */
            max_src_bytes = rendered_bytes_per_row - camera_x;
            
            bytes_to_copy = playfield_bytes_per_row;
            if (bytes_to_copy > max_src_bytes) {
                bytes_to_copy = max_src_bytes;
            }

            /* Copy the row for this plane using helper that performs far-pointer copy */
            copy_plane_bytes(src_offset, dst_offset, bytes_to_copy);
        }
    }
}

void blit_comic_playfield_offscreen(void)
{
    /* Choose the correct 16x32 sprite based on animation and facing */
    const uint8_t __far *sprite_ptr = NULL;
    int rel_x_units;              /* Signed: can be negative for left-of-camera sprites */
    int pixel_x_signed;           /* Signed calculation to avoid underflow */
    int pixel_y_signed;           /* Signed calculation */

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
    
    /* Bounds check: Reject sprites that extend beyond the screen edges.
     * 
     * NOTE: blit_sprite_16x32_masked does NOT implement horizontal clipping.
     * It rejects any sprite where pixel_x + 16 > SCREEN_WIDTH. Therefore,
     * we must reject sprites earlier here if they would be partially off-screen.
     * 
     * rel_x_units range interpretation (each unit = 8 pixels):
     *   -2: sprite left edge at pixel_x = (-2 * 8) + 8 = -8 (completely off-left)
     *   -1: sprite left edge at pixel_x = (-1 * 8) + 8 = 0 (would be partially off)
     *    0: sprite left edge at pixel_x = 0 + 8 = 8 pixels from left margin (safe)
     *   PLAYFIELD_WIDTH: rightmost safe position (sprite extends 16 pixels)
     * 
     * For smooth scrolling, sprites at the very edge (rel_x_units = -1) are rejected
     * to prevent clipping issues. This means sprites briefly disappear as they scroll
     * off-screen rather than fading out gradually.
     * 
     * TODO: Implement proper horizontal clipping in blit_sprite_16x32_masked to allow
     * sprites to be drawn partially off-screen with smooth edge transitions.
     */
    if (rel_x_units < 0 || rel_x_units >= PLAYFIELD_WIDTH) {
        /* Offscreen horizontally; skip drawing */
        return;
    }

    /* Perform signed arithmetic first to avoid integer underflow when rel_x_units is negative.
     * For example, rel_x_units = -1:
     *   Signed:   (-1 * 8) + 8 = -8 + 8 = 0 (CORRECT)
     *   Unsigned: (unsigned)(-8) + 8 = 65528 + 8 = 65536 (WRONG underflow)
     * Cast to unsigned only for the function call. */
    pixel_x_signed = (rel_x_units * 8) + 8;
    pixel_y_signed = (int)comic_y * 8 + 8;

    /* Blit 16x32 masked sprite to the offscreen buffers */
    blit_sprite_16x32_masked((uint16_t)pixel_x_signed, (uint16_t)pixel_y_signed, (const uint8_t *)sprite_ptr);
}

void blit_comic_partial_playfield_offscreen(uint16_t max_height)
{
    /* Clipped version of blit_comic_playfield_offscreen for falling death rendering.
     * Renders Comic's sprite but limits the height to prevent extending beyond playfield bottom.
     * 
     * max_height: Maximum number of pixels to render (0-32).
     *             Used to clip sprite when Comic is partially off bottom of screen.
     */
    const uint8_t __far *sprite_ptr = NULL;
    int rel_x_units;              /* Signed: can be negative for left-of-camera sprites */
    int pixel_x_signed;           /* Signed calculation to avoid underflow */
    int pixel_y_signed;           /* Signed calculation */

    /* Validate max_height parameter */
    if (max_height == 0 || max_height > 32) {
        return;
    }

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
    
    /* Horizontal bounds check: reject sprites outside playfield width */
    if (rel_x_units < 0 || rel_x_units >= PLAYFIELD_WIDTH) {
        /* Offscreen horizontally; skip drawing */
        return;
    }

    pixel_x_signed = (rel_x_units * 8) + 8;
    pixel_y_signed = (int)comic_y * 8 + 8;

    /* Check if the full 32-pixel sprite fits within the visible playfield.
     * 
     * NOTE: We cannot use blit_sprite_16x8_masked or blit_sprite_16x16_masked
     * to render partial portions of a 16x32 sprite. Those functions expect
     * sprite data in completely different formats (16x8 = 32 bytes, 16x16 = 160 bytes)
     * whereas our sprite pointers point to 16x32 sprites (320 bytes). Passing a 16x32
     * sprite pointer to a function expecting 16x8 data causes it to read the wrong
     * bytes, resulting in corrupted graphics.
     * 
     * For the falling death animation, we simply skip rendering if the sprite would
     * be clipped. This is acceptable since Comic is falling off-screen anyway.
     */
    if (max_height >= 32) {
        /* Full sprite fits - render it */
        blit_sprite_16x32_masked((uint16_t)pixel_x_signed, (uint16_t)pixel_y_signed, (const uint8_t *)sprite_ptr);
    }
    /* else: Sprite would be partially clipped - skip rendering to avoid corruption */
}

void swap_video_buffers(void)
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
    uint16_t x_pos;
    
    if (comic_hp >= MAX_HP) {
        /* HP is already full; award 1800 bonus points */
        award_points(18);  /* 18 * 100 = 1800 points */
        return;
    }
    
    /* Increment HP */
    comic_hp++;
    
    /* Render the HP meter cell at Y=82, base X=240
     * Each unit of HP is 8 pixels wide */
    x_pos = 240 + (comic_hp << 3);
    blit_8x16_sprite(x_pos, 82, sprite_meter_full_8x16);
}

void decrement_comic_hp(void)
{
    uint16_t x_pos;
    
    if (comic_hp == 0) {
        return;
    }
    
    /* Render the empty sprite at the current HP position before decrementing */
    x_pos = 240 + (comic_hp << 3);
    blit_8x16_sprite(x_pos, 82, sprite_meter_empty_8x16);
    
    /* Decrement HP */
    comic_hp--;
    
    /* Play damage sound */
    play_sound(SOUND_DAMAGE, 2);  /* priority 2 */
}

static void render_comic_hp_meter(void)
{
    uint8_t i;
    uint16_t x_pos;
    
    /* Render exactly MAX_HP cells (cells 1 through MAX_HP) */
    /* Cell N is at x = 240 + (N * 8) */
    for (i = 1; i <= MAX_HP; i++) {
        x_pos = 240 + (i * 8);
        if (i <= comic_hp) {
            /* Full cell */
            blit_8x16_sprite(x_pos, 82, sprite_meter_full_8x16);
        } else {
            /* Empty cell */
            blit_8x16_sprite(x_pos, 82, sprite_meter_empty_8x16);
        }
    }
}

/* address_of_tile_at_coordinates is now defined in physics.c */

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
    
    /* For each plane */
    for (plane = 0; plane < 4; plane++) {
        /* Set plane mask */
        outp(0x3c4, 0x02);           /* Write to Sequencer Index register */
        outp(0x3c5, 1 << plane);     /* Write plane mask to Sequencer Data register */

        /* Minimal synchronization to ensure sequencer register write completes
         * before accessing video memory. A single I/O read is sufficient on
         * standard VGA/EGA hardware and avoids millisecond-scale delays. */
        (void)inp(0x3DA);            /* Read from VGA status register */
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
            dst_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, dst_offset);
            
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
    uint8_t tile_x;
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
            clear_ptr = (uint8_t __far *)MK_FP(VIDEO_MEMORY_BASE, clear_offset);
            
            /* Write 256 bytes of zero to this row */
            for (i = 0; i < 256; i++) {
                clear_ptr[i] = 0;
            }
        }
    }
    
    /* Iterate through all tiles in the map (128×10) */
    for (tile_y = 0; tile_y < MAP_HEIGHT_TILES; tile_y++) {
        for (tile_x = 0; tile_x < MAP_WIDTH_TILES; tile_x++) {
            /* Calculate index into tile map, casting to uint16_t to avoid overflow:
             * tile_y (0-9) * MAP_WIDTH_TILES (128) + tile_x (0-127) = 0-1279 */
            tile_index = (uint16_t)tile_y * MAP_WIDTH_TILES + tile_x;
            
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
    const stage_t *current_stage_ptr;
    int cam_x;
    
    /* Get the current level data pointer */
    if (current_level_number < 8) {
        /* Use the global current_level_ptr, which is accessible to physics.c */
        current_level_ptr = level_data_pointers[current_level_number];
    } else {
        return;  /* Invalid level */
    }
    
    /* Ensure the level data pointer is valid before using it */
    if (current_level_ptr == NULL) {
        current_tiles_ptr = NULL;
        return;
    }
    
    /* Set current_tiles_ptr based on current stage (0, 1, or 2) */
    if (current_stage_number < 3) {
        current_stage_ptr = &current_level_ptr->stages[current_stage_number];
        
        /* Determine which tile map to use */
        if (current_stage_number == 0) {
            current_tiles_ptr = pt0.tiles;
        } else if (current_stage_number == 1) {
            current_tiles_ptr = pt1.tiles;
        } else {
            current_tiles_ptr = pt2.tiles;
        }
        
        /* Initialize Comic's position based on entry method */
        if (source_door_level_number >= 0) {
            /* Entering via a door: search for reciprocal door and spawn at door.x + 1 */
            int i;
            int door_found = 0;
            
            for (i = 0; i < 3; i++) {
                if (current_stage_ptr->doors[i].target_level == source_door_level_number &&
                    current_stage_ptr->doors[i].target_stage == source_door_stage_number) {
                    /* Found reciprocal door */
                    comic_y = current_stage_ptr->doors[i].y;
                    comic_x = current_stage_ptr->doors[i].x + 1;
                    
                    /* Update checkpoint for respawn */
                    comic_y_checkpoint = comic_y;
                    comic_x_checkpoint = comic_x;
                    
                    door_found = 1;
                    break;
                }
            }
            
            if (!door_found) {
                /* Panic: no reciprocal door found (shouldn't happen) */
                comic_x = 12;
                comic_y = 8;
            }
        } else {
            /* Entering via left/right boundary or first spawn: use checkpoint */
            comic_y = comic_y_checkpoint;
            comic_x = comic_x_checkpoint;
        }
        
        /* Initialize Comic's physics state
         * Start with Comic standing (not falling), let the game loop detect if he needs to fall */
        comic_is_falling_or_jumping = 0;
        comic_y_vel = 0;  /* Start at rest */
        comic_jump_counter = 0;  /* Jump counter will be reinitialized when needed */
        
        /* Initialize camera to center on Comic, clamped to valid range.
         * Formula: camera_x = clamp(comic_x - (PLAYFIELD_WIDTH/2 - 1), 0, MAP_WIDTH - PLAYFIELD_WIDTH)
         * This positions the camera so Comic is centered in the playfield. */
        cam_x = (int)comic_x - (PLAYFIELD_WIDTH / 2 - 1);
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
    
    /* Initialize enemies from stage data */
    if (current_stage_number < 3 && current_stage_ptr != NULL) {
        int i;
        for (i = 0; i < MAX_NUM_ENEMIES; i++) {
            const enemy_record_t *enemy_rec = &current_stage_ptr->enemies[i];
            
            /* Copy enemy definition data */
            enemies[i].behavior = enemy_rec->behavior;
            enemy_shp_index[i] = enemy_rec->shp_index;
            enemies[i].num_animation_frames = shp_get_animation_length(enemy_rec->shp_index);
            if (enemies[i].num_animation_frames == 0) {
                enemies[i].num_animation_frames = 1;
            }
            enemies[i].animation_frames_ptr = 0; /* TODO: Set from SHP file */
            
            /* Initialize spawn state */
            enemies[i].state = ENEMY_STATE_DESPAWNED;
            enemies[i].facing = COMIC_FACING_LEFT;
            enemies[i].spawn_timer_and_animation = 20; /* Spawn after 20 ticks */
            enemies[i].restraint = 0;
            
            /* Clear position and velocity */
            enemies[i].x = 0;
            enemies[i].y = 0;
            enemies[i].x_vel = 0;
            enemies[i].y_vel = 0;
        }
    }
    
    /* Clear all fireballs */
    {
        int i;
        for (i = 0; i < MAX_NUM_FIREBALLS; i++) {
            fireballs[i].x = FIREBALL_DEAD;
            fireballs[i].y = FIREBALL_DEAD;
            fireballs[i].vel = 0;
            fireballs[i].corkscrew_phase = 0;
            fireballs[i].animation = 0;
            fireballs[i].num_animation_frames = 2;
        }
    }
    
    /* Clear teleporting flag */
    comic_is_teleporting = 0;
    
    /* Pre-render the entire map into RENDERED_MAP_BUFFER */
    render_map();

    /* Ensure the initial frame is drawn and displayed: replicate the
     * assembly behavior of blitting and swapping buffers at load time. */
    blit_map_playfield_offscreen();
    /* Skip blitting Comic here - if doing beam_in, Comic should not be visible yet;
     * if not doing beam_in (entering via door), the game loop will render Comic normally */
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

    /* Play beam-in animation on first spawn (source_door_level_number == -2) */
    if (source_door_level_number == -2) {
        beam_in();
    } else if (source_door_level_number >= 0) {
        /* Entered via door: play exit door animation */
        exit_door_animation();
    }
    
    /* Reset source_door_level_number to -1 (normal entry) for future transitions */
    source_door_level_number = -1;
}

/*
 * comic_death_animation - Play the 8-frame death animation when Comic takes lethal damage
 * 
 * This function displays an 8-frame animation of Comic's death using the sprite_comic_death_*
 * frames. It continues to handle enemies and items during the animation so they can move
 * and be rendered.
 * 
 * Called from:
 * - handle_enemies() in actors.c when Comic collides with an enemy while HP == 0
 * - comic_takes_damage() when HP is reduced to zero (if implemented)
 * 
 * The animation sequence:
 * - Frames 0-3: Comic sprite is drawn under the death animation
 * - Frames 4-7: Only death animation is shown (Comic is "gone")
 * - Each frame lasts 1 tick
 */
void comic_death_animation(void)
{
    uint8_t frame;
    const uint8_t *death_frame_ptr;
    int16_t pixel_x;
    int16_t pixel_y;
    
    /* Play the death sound effect */
    play_sound(SOUND_DEATH, 4);  /* priority 4 */
    
    /* Play all 8 frames of the death animation */
    for (frame = 0; frame < 8; frame++) {
        /* Blit the map to the offscreen buffer */
        blit_map_playfield_offscreen();
        
        /* For the first 4 frames (0-3), draw Comic under the death animation */
        if (frame < 4) {
            blit_comic_playfield_offscreen();
        }
        
        /* Continue updating enemies while the animation plays */
        handle_enemies();
        
        /* Get the death animation frame sprite (16x32) */
        switch (frame) {
            case 0: death_frame_ptr = sprite_comic_death_0_16x32m; break;
            case 1: death_frame_ptr = sprite_comic_death_1_16x32m; break;
            case 2: death_frame_ptr = sprite_comic_death_2_16x32m; break;
            case 3: death_frame_ptr = sprite_comic_death_3_16x32m; break;
            case 4: death_frame_ptr = sprite_comic_death_4_16x32m; break;
            case 5: death_frame_ptr = sprite_comic_death_5_16x32m; break;
            case 6: death_frame_ptr = sprite_comic_death_6_16x32m; break;
            case 7: death_frame_ptr = sprite_comic_death_7_16x32m; break;
            default: death_frame_ptr = sprite_comic_death_0_16x32m; break;
        }
        
        /* Calculate Comic's pixel position for blitting the death animation.
         * Add +8 offsets to align within the playfield (top-left at 8,8),
         * matching blit_comic_playfield_offscreen and enemy rendering. */
        pixel_x = (((int16_t)comic_x - (int16_t)camera_x) * 8) + 8;
        pixel_y = ((int16_t)comic_y * 8) + 8;
        
        /* Blit the death animation frame with transparency to the offscreen buffer */
        blit_sprite_16x32_masked((uint16_t)pixel_x, (uint16_t)pixel_y, death_frame_ptr);
        
        /* Continue updating items while the animation plays */
        handle_item();
        
        /* Display the frame */
        swap_video_buffers();
        
        /* Wait 1 tick per frame */
        wait_n_ticks(1);
    }
    
    /* Clear comic_animation to prevent any sprite rendering.
     * Set to 255 (invalid value) rather than 0 (COMIC_STANDING) to ensure
     * blit_comic_playfield_offscreen() returns early without rendering anything. */
    comic_animation = 255;
}

/*
 * comic_dies - Handle Comic's death
 * 
 * This function is called when Comic falls off the bottom of the screen
 * or when his HP is reduced to zero. It:
 * 1. Sets Comic's animation based on the type of death
 * 2. For falling deaths: blits partial Comic sprite (feet off screen)
 * 3. For enemy collision deaths: skips sprite rendering (already played animation)
 * 4. Plays the "Too Bad" sound effect
 * 5. Either respawns Comic with one less life, or ends the game
 * 
 * Uses comic_death_animation_finished flag to distinguish:
 * - Flag == 0: falling death (show partial sprite)
 * - Flag == 1: enemy collision death (skip sprite, jump to .too_bad equivalent)
 */
void comic_dies(void)
{
    /* Check if we're coming from the death animation (enemy collision) */
    if (comic_death_animation_finished == 0) {
        /* This is a falling death - show Comic partially visible as he falls off */
        uint16_t visible_height;
        
        comic_animation = COMIC_JUMPING;
        
        if (comic_y < PLAYFIELD_HEIGHT) {
            /* Calculate how many pixels of the sprite are actually visible on screen.
             * Playfield height = 20 units = 160 pixels. If Comic is at unit Y position,
             * then visible_height (in pixels) = (20 - comic_y) * 8.
             * This represents how much of the sprite is above the playfield bottom. */
            visible_height = (PLAYFIELD_HEIGHT - (int)comic_y) * 8;
            
            /* Clamp to sprite height (32 pixels) */
            if (visible_height > 32) {
                visible_height = 32;
            }
            
            /* Blit the map and clipped Comic sprite */
            blit_map_playfield_offscreen();
            blit_comic_partial_playfield_offscreen(visible_height);
            swap_video_buffers();
            
            /* Wait briefly to show the death frame */
            wait_n_ticks(1);
        }
    } else {
        /* Enemy collision death - set animation to invalid value to prevent any sprite rendering.
         * Using 255 (invalid) rather than 0 (COMIC_STANDING) to ensure no sprite is drawn. */
        comic_animation = 255;
        /* Clear the flag for next time comic_dies is called */
        comic_death_animation_finished = 0;
    }
    
    /* Common path for both death types */
    blit_map_playfield_offscreen();
    swap_video_buffers();
    
    /* Wait for effect to display */
    wait_n_ticks(2);
    
    /* Play "Too Bad" sound */
    play_sound(SOUND_TOO_BAD, 2);
    
    /* Wait for sound to finish */
    wait_n_ticks(15);
    
    /* Lose a life */
    lose_a_life();
    
    /* Check if any lives remain */
    if (comic_num_lives == 0) {
        /* No lives left - show game over screen */
        game_over();
        return;
    }
    
    /* Reset core movement/animation state for respawn (match assembly intent) */
    comic_run_cycle = 0;
    comic_is_falling_or_jumping = 0;
    comic_x_momentum = 0;
    comic_y_vel = 0;
    comic_animation = COMIC_STANDING;
    
    /* HP refill behavior on respawn: keep visible meter state and only refill missing cells */
    /* This preserves the current HP display and avoids bonus points while refilling */
    comic_hp_pending_increase = (comic_hp < MAX_HP) ? (MAX_HP - comic_hp) : 0;
    
    /* Reset fireball meter counter per assembly */
    fireball_meter_counter = 2;
    
    /* Ensure we are not considered teleporting and not coming from a door */
    comic_is_teleporting = 0;
    source_door_level_number = -1;
    
    /* Reload stage to position Comic at checkpoint, clamp camera, and redraw */
    load_new_stage();
}

/*
 * blit_game_over_graphic - Blit the GAME OVER graphic to offscreen buffer
 * 
 * Blits the 128x48 GAME OVER sprite using plane-by-plane rendering.
 * The sprite data is organized as 4 sequential planes of 768 bytes each.
 * 
 * Parameters:
 *   pixel_offset - Offset in video memory where graphic should be blitted
 */
static void blit_game_over_graphic(uint16_t pixel_offset)
{
    uint8_t plane;
    
    /* Blit the game over graphic using the plane-by-plane method
     * The graphic is 16 bytes wide (128 pixels / 8) and 48 pixels tall
     * Sprite data layout: 768 bytes per plane (128 pixels × 48 pixels / 8)
     * Blue plane (0-767) + Green plane (768-1535) + Red plane (1536-2303) + Intensity plane (2304-3071) */
    for (plane = 1; plane <= 8; plane *= 2) {
        uint16_t src_offset = 0;
        uint16_t dst_offset = pixel_offset;
        uint8_t row;
        uint8_t col;
        uint8_t plane_index;
        const uint8_t *src;
        uint8_t __far *video_mem = (uint8_t __far *)0xa0000000L;
        
        /* Calculate plane index from plane mask (1->0, 2->1, 4->2, 8->3) */
        if (plane == 1) plane_index = 0;
        else if (plane == 2) plane_index = 1;
        else if (plane == 4) plane_index = 2;
        else plane_index = 3;  /* plane == 8 */
        
        /* Offset source pointer to correct plane (768 bytes per plane) */
        src = (const uint8_t *)sprite_R4_game_over_128x48 + (plane_index * 768);
        
        /* Set plane write mask */
        _outp(0x3CE, 0x05);     /* GC Index: Graphics Mode */
        _outp(0x3CF, 0x02);     /* Graphics Mode: Write Mode 2 */
        _outp(0x3CE, 0x08);     /* GC Index: Bit Mask */
        _outp(0x3CF, 0xFF);     /* Bit Mask: all bits */
        _outp(0x3C4, 0x02);     /* SC Index: Map Mask */
        _outp(0x3C5, plane);    /* Map Mask: current plane */
        
        /* Blit the graphic */
        for (row = 0; row < 48; row++) {
            for (col = 0; col < 16; col++) {
                video_mem[offscreen_video_buffer_ptr + dst_offset + col] = src[src_offset + col];
            }
            src_offset += 16;
            dst_offset += SCREEN_WIDTH / 8;
        }
    }
    
    /* Restore graphics mode */
    _outp(0x3CE, 0x05);
    _outp(0x3CF, 0x00);
    _outp(0x3C4, 0x02);
    _outp(0x3C5, 0x0F);
}

/*
 * game_over - Display game over screen and end game
 * 
 * Displays the "GAME OVER" graphic on screen, plays the game over sound,
 * then waits for a keystroke before terminating. This is called when the
 * player runs out of lives and falls off the bottom of a stage.
 */
static void game_over(void)
{
    uint16_t pixel_offset;
    
    /* Render the map to the offscreen buffer */
    blit_map_playfield_offscreen();
    
    /* Blit the GAME OVER graphic to the offscreen buffer
     * GRAPHIC_GAME_OVER is 128x48 pixels at screen position (40, 64)
     * pixel_coords(40, 64) = 40/8 + 64*40 = 5 + 2560 = 2565 */
    pixel_offset = 40 / 8 + (64 * (SCREEN_WIDTH / 8));
    blit_game_over_graphic(pixel_offset);
    
    /* Swap video buffers and wait */
    wait_n_ticks(1);
    swap_video_buffers();
    
    /* Play game over sound effect */
    play_sound(SOUND_GAME_OVER, 2);
    
    /* Clear the BIOS keyboard buffer and wait for a keystroke */
    clear_bios_keyboard_buffer();
    
    /* Wait for keystroke using BIOS interrupt 0x16 (INT 16h, AH=0x00: get keystroke) */
    {
        union REGS regs;
        regs.h.ah = 0x00;  /* Get keystroke */
        int86(0x16, &regs, &regs);
    }
    
    /* Jump to terminate program (original calls do_high_scores then terminate) */
    terminate_program();
}

/*
 * game_end_sequence - Play victory sequence when player wins
 * 
 * Called when all three treasures have been collected. This function:
 * 1. Plays the beam-out animation (Comic disappears)
 * 2. Awards 20,000 base points
 * 3. Awards 10,000 points for each remaining life
 * 4. Plays the title theme
 * 5. Shows the win graphic (sys002.ega)
 * 6. Waits for a keystroke
 * 7. Returns to main menu or terminates
 */
static void game_end_sequence(void)
{
    uint8_t life_counter;
    uint16_t points_awarded;
    uint16_t pixel_offset;
    
    /* Play the beam-out animation */
    beam_out();
    
    /* Award 20,000 points for winning (20 x 1,000 points) */
    for (points_awarded = 0; points_awarded < 20; points_awarded++) {
        /* Play score tally sound */
        play_sound(SOUND_SCORE_TALLY, 3);
        award_points(10);  /* 10 * 100 = 1000 points per loop, 20 loops = 20,000 total */
        wait_n_ticks(1);
    }
    
    /* Award points for remaining lives (10,000 points per life) */
    life_counter = comic_num_lives;
    while (life_counter > 0) {
        /* Award 10,000 points for this life (10 x 1,000 points) */
        for (points_awarded = 0; points_awarded < 10; points_awarded++) {
            /* Play score tally sound */
            play_sound(SOUND_SCORE_TALLY, 3);
            award_points(10);  /* 10 * 100 = 1000 points per loop, 10 loops = 10,000 per life */
            wait_n_ticks(1);
        }
        
        life_counter--;
        wait_n_ticks(3);  /* Brief pause between lives */
    }
    
    /* Play the title theme */
    play_sound(SOUND_TITLE, 4);
    
    /* Load and display the win graphic (sys002.ega)
     * TODO: Implement RLE decompression and display of win graphic
     * For now, just show the map */
    blit_map_playfield_offscreen();
    swap_video_buffers();
    
    /* Wait for graphic display and fade in */
    wait_n_ticks(20);
    
    /* Clear keyboard buffer and wait for keystroke */
    clear_bios_keyboard_buffer();
    {
        union REGS regs;
        regs.h.ah = 0x00;  /* Get keystroke */
        int86(0x16, &regs, &regs);
    }
    
    /* Show the game over screen (same as loss, per assembly code) */
    blit_map_playfield_offscreen();
    
    /* Blit the GAME OVER graphic (same 128x48 at position 40,64) */
    pixel_offset = 40 / 8 + (64 * (SCREEN_WIDTH / 8));
    blit_game_over_graphic(pixel_offset);
    
    swap_video_buffers();
    
    /* Clear keyboard buffer and wait for keystroke before high scores */
    clear_bios_keyboard_buffer();
    {
        union REGS regs;
        regs.h.ah = 0x00;  /* Get keystroke */
        int86(0x16, &regs, &regs);
    }
    
    /* Call do_high_scores and then terminate */
    /* TODO: implement do_high_scores() if needed */
    terminate_program();
}

/*
 * award_points - Award points to the player's score using base-100 arithmetic
 * 
 * Adds points to the player's score by performing base-100 addition with carry.
 * Score is stored as three base-100 bytes where each byte holds 0-99 and represents
 * a pair of decimal digits. The formula is: score = byte[0] + (byte[1]*100) + (byte[2]*10000)
 * 
 * Parameters:
 *   points - Points to add in base-100 units (0-99 typical per call)
 *            Each unit represents 100 decimal points.
 *            Example: award_points(20) adds 2000 points, resulting in byte[1] = 20 → displays "002000"
 *            Example: award_points(18) adds 1800 points
 * 
 * The base-100 encoding stores pairs of decimal digits in each byte:
 *   - byte[0]: rightmost pair (ones and tens digits: displays "00" to "99")
 *   - byte[1]: middle pair (hundreds and thousands digits: displays "00" to "99")
 *   - byte[2]: leftmost pair (ten-thousands and hundred-thousands: displays "00" to "99")
 * Full 6-digit display: "AABBCC" where AA=byte[2], BB=byte[1], CC=byte[0]
 * The rightmost 2 digits (byte[0]) are always "00" since award_points gives increments of 100.
 * Maximum score is 999,999 ([99, 99, 99]).
 */
void award_points(uint16_t points)
{
    uint8_t carry = 0;
    uint8_t i;
    
    /* Add points to byte[0] with carry propagation */
    score_bytes[0] += (uint8_t)points;
    if (score_bytes[0] >= 100) {
        carry = 1;
        score_bytes[0] -= 100;
    } else {
        carry = 0;
    }
    
    /* Propagate carry through remaining bytes */
    for (i = 1; i < 3 && carry; i++) {
        score_bytes[i] += carry;
        if (score_bytes[i] >= 100) {
            carry = 1;
            score_bytes[i] -= 100;
        } else {
            carry = 0;
        }
    }
    
    /* If still have carry after byte 2, cap at maximum score 999,999 */
    if (carry) {
        score_bytes[0] = 99;
        score_bytes[1] = 99;
        score_bytes[2] = 99;
    }
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
 * clear_scancode_queue - Clear our local scancode queue
 * 
 * Clears the scancode_queue by resetting head and tail pointers.
 * This is used after pausing to discard any scancodes queued while paused.
 */
static void clear_scancode_queue(void)
{
    scancode_queue_head = 0;
    scancode_queue_tail = 0;
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
 * handle_cheat_codes - Process cheat code inputs
 * 
 * Checks for cheat code key presses and grants appropriate items/abilities.
 * Uses edge-triggered detection (0→1 transition) to activate once per key press
 * without blocking the game loop.
 * 
 * Current cheats:
 *   W key - Grant teleport wand for testing teleportation feature
 *   K key - Grant door key for testing door opening feature
 *   B key - Grant Blastola Cola for testing fireball feature
 *   C key - Grant Corkscrew for testing fireball corkscrew motion
 *   O key - Grant Boots for testing increased jump power
 *   L key - Grant Lantern for testing Castle level lighting
 */
static void handle_cheat_codes(void)
{
    /* W key: Grant teleport wand (edge-triggered on press) */
    if (key_state_cheat_wand && !previous_key_state_cheat_wand) {
        if (comic_has_teleport_wand == 0) {
            comic_has_teleport_wand = 1;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* K key: Grant door key (edge-triggered on press) */
    if (key_state_cheat_key && !previous_key_state_cheat_key) {
        if (comic_has_door_key == 0) {
            comic_has_door_key = 1;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* B key: Grant Blastola Cola (edge-triggered on press) */
    if (key_state_cheat_cola && !previous_key_state_cheat_cola) {
        if (comic_firepower < MAX_NUM_FIREBALLS) {
            comic_firepower++;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* C key: Grant Corkscrew (edge-triggered on press) */
    if (key_state_cheat_corkscrew && !previous_key_state_cheat_corkscrew) {
        if (comic_has_corkscrew == 0) {
            comic_has_corkscrew = 1;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* O key: Grant Boots (edge-triggered on press) */
    if (key_state_cheat_boots && !previous_key_state_cheat_boots) {
        if (comic_jump_power < 5) {
            comic_jump_power = 5;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* L key: Grant Lantern (edge-triggered on press) */
    if (key_state_cheat_lantern && !previous_key_state_cheat_lantern) {
        if (comic_has_lantern == 0) {
            comic_has_lantern = 1;
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
    
    /* S key: Grant Shield (edge-triggered on press) */
    if (key_state_cheat_shield && !previous_key_state_cheat_shield) {
        if (comic_has_shield == 0) {
            comic_has_shield = 1;
            /* Check if Comic already has full HP */
            if (comic_hp >= MAX_HP) {
                /* Full HP: award an extra life */
                award_extra_life();
            } else {
                /* Not full: schedule only the missing HP increments */
                comic_hp_pending_increase = MAX_HP - comic_hp;
            }
            /* Play item collection sound for feedback */
            play_sound(SOUND_COLLECT_ITEM, 3);
        }
    }
}

/*
 * update_keyboard_input - Read keyboard state and update key_state variables
 * 
 * Polls the BIOS keyboard buffer and reads scancodes to update the key_state
 * variables based on the configured keymap.
 * 
 * The key_state variables are:
 * - key_state_jump: Space (jump)
 * - key_state_fire: Insert (fire)
 * - key_state_left: Left arrow (move left)
 * - key_state_right: Right arrow (move right)
 * - key_state_open: Alt (open door)
 * - key_state_teleport: CapsLock (teleport)
 * - key_state_esc: Escape (quit)
 * 
 * Keymaps are loaded from KEYS.DEF if present; otherwise defaults are used.
 */
static void update_keyboard_input(void)
{
    uint8_t scancode;
    uint8_t key_count = 0;
    uint8_t is_break;
    uint8_t code;
    static uint8_t extended_prefix = 0;
    
    /* Process all scancodes in the queue */
    while (scancode_queue_head != scancode_queue_tail) {
        /* Get next scancode from queue */
        scancode = scancode_queue[scancode_queue_tail];
        scancode_queue_tail = (scancode_queue_tail + 1) % MAX_SCANCODE_QUEUE;
        
        key_count++;
        
        if (scancode == 0xE0) {
            extended_prefix = 1;
            continue;
        }

        /* Handle break (key release) codes */
        is_break = (scancode & 0x80) != 0;
        code = scancode & 0x7F;
        (void)extended_prefix; /* prefix retained for future use */
        extended_prefix = 0;
        
        /* Compare scancode with configured keymap */
        if (code == keymap[0]) {
            key_state_jump = (uint8_t)(!is_break);  /* SPACE */
        } else if (code == keymap[1]) {
            key_state_fire = (uint8_t)(!is_break);  /* INSERT */
        } else if (code == keymap[2]) {
            key_state_left = (uint8_t)(!is_break);  /* LEFT ARROW */
        } else if (code == keymap[3]) {
            key_state_right = (uint8_t)(!is_break); /* RIGHT ARROW */
        } else if (code == keymap[4]) {
            key_state_open = (uint8_t)(!is_break);  /* ALT */
        } else if (code == keymap[5]) {
            key_state_teleport = (uint8_t)(!is_break);  /* CAPSLOCK */
            /* Detect edge transition: key pressed (break=0) and was previously released */
            if (!is_break && !previous_key_state_teleport) {
                teleport_key_pressed = 1;  /* Flag for game loop to process */
            }
        } else if (code == SCANCODE_ESC) {
            key_state_esc = (uint8_t)(!is_break);  /* ESCAPE - hardcoded */
        } else if (code == SCANCODE_W) {
            key_state_cheat_wand = (uint8_t)(!is_break);  /* W key - cheat code */
        } else if (code == SCANCODE_K) {
            key_state_cheat_key = (uint8_t)(!is_break);  /* K key - cheat code */
        } else if (code == SCANCODE_B) {
            key_state_cheat_cola = (uint8_t)(!is_break);  /* B key - cheat code */
        } else if (code == SCANCODE_C) {
            key_state_cheat_corkscrew = (uint8_t)(!is_break);  /* C key - cheat code */
        } else if (code == SCANCODE_O) {
            key_state_cheat_boots = (uint8_t)(!is_break);  /* O key - cheat code */
        } else if (code == SCANCODE_L) {
            key_state_cheat_lantern = (uint8_t)(!is_break);  /* L key - cheat code */
        } else if (code == SCANCODE_S) {
            key_state_cheat_shield = (uint8_t)(!is_break);  /* S key - cheat code */
        }
    }
}

/*
 * face_or_move_left - Face or move Comic left
 * 
 * Implements the original assembly behavior:
 * - If Comic is already facing left, move left
 * - If Comic is facing right, only change facing direction (don't move)
 * 
 * This creates the effect where pressing the opposite direction first changes
 * the sprite, then movement happens on the next frame.
 */
static void face_or_move_left(void)
{
    if (comic_facing == COMIC_FACING_LEFT) {
        /* Already facing left, so move left */
        move_left();
    } else {
        /* Facing right, so just change facing direction */
        comic_facing = COMIC_FACING_LEFT;
    }
}

/*
 * face_or_move_right - Face or move Comic right
 * 
 * Implements the original assembly behavior:
 * - If Comic is already facing right, move right
 * - If Comic is facing left, only change facing direction (don't move)
 * 
 * This creates the effect where pressing the opposite direction first changes
 * the sprite, then movement happens on the next frame.
 */
static void face_or_move_right(void)
{
    if (comic_facing == COMIC_FACING_RIGHT) {
        /* Already facing right, so move right */
        move_right();
    } else {
        /* Facing left, so just change facing direction */
        comic_facing = COMIC_FACING_RIGHT;
    }
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
        /* Reset landing sentinel for this tick */
        landed_this_tick = 0;
        
        /* Read keyboard input and update key_state variables */
        update_keyboard_input();
        
        /* Process cheat codes (for testing/debugging) */
        handle_cheat_codes();
        
        /* Initiate jump if conditions are met (matching original assembly):
         * - Player is standing (not in air)
         * - Jump key transitioned from released to pressed (edge-triggered, not level-triggered)
         * - Jump power has been recharged (comic_jump_power > 1)
         * 
         * Edge-triggering prevents repeated jumps while the key is continuously held.
         * The key must transition from 0 (released) to 1 (pressed) to initiate a jump.
         */
        if (comic_is_falling_or_jumping == 0 && key_state_jump && !previous_key_state_jump && comic_jump_power > 1) {
            /* Start a new jump: reset counter to power, mark as in air */
            comic_jump_counter = comic_jump_power;
            comic_is_falling_or_jumping = 1;
            minimum_jump_frames = 0;  /* No forced frames - rely on key press duration only */
        }
        
        /* Update previous frame state for edge-triggered input */
        previous_key_state_jump = key_state_jump;
        previous_key_state_teleport = key_state_teleport;
        previous_key_state_cheat_wand = key_state_cheat_wand;
        previous_key_state_cheat_key = key_state_cheat_key;
        previous_key_state_cheat_cola = key_state_cheat_cola;
        previous_key_state_cheat_corkscrew = key_state_cheat_corkscrew;
        previous_key_state_cheat_boots = key_state_cheat_boots;
        previous_key_state_cheat_lantern = key_state_cheat_lantern;
        previous_key_state_cheat_shield = key_state_cheat_shield;
        
        /* Check for win condition
         * When the player wins, win_counter is set to a delay value (e.g., 200).
         * It decrements each tick until it reaches 1, at which point the game
         * end sequence begins. The value 1 (not 0) is the trigger point, allowing
         * win_counter to distinguish between: 0=no win, 1=trigger sequence, >1=counting down */
        if (win_counter != 0) {
            win_counter--;
            if (win_counter == 1) {
                /* Player has won - play victory sequence and exit game loop */
                game_end_sequence();
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
            /* Call physics to handle gravity and collisions (single call per frame) */
            handle_fall_or_jump();
            
            /* Check open input (doors) - only if not falling/jumping and open key is pressed */
            if (comic_is_falling_or_jumping == 0 && key_state_open == 1) {
                check_door_activation();
            }
            
            /* Check teleport input - only if not falling/jumping and teleport key was pressed this frame */
            if (comic_is_falling_or_jumping == 0 && teleport_key_pressed && comic_has_teleport_wand != 0) {
                begin_teleport();
                /* begin_teleport sets comic_is_teleporting, which will be handled
                 * at the top of the next loop iteration. Skip to actor handling. */
                skip_rendering = 1;
            }
            /* Handle left/right movement - only if not falling/jumping, not teleporting,
             * and did NOT just land this tick (assembly jumps to pause after landing). */
            else if (comic_is_falling_or_jumping == 0 && landed_this_tick == 0) {
                uint8_t foot_y;
                uint16_t foot_offset;
                uint8_t foot_tile;
                uint8_t foot_solid;
                uint8_t right_foot_solid;

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

                /* Check for floor below Comic (walked off an edge) */
                foot_y = comic_y + 4;
                foot_offset = address_of_tile_at_coordinates(comic_x / 2, foot_y / 2);
                foot_solid = 0;
                right_foot_solid = 0;
                if (current_tiles_ptr != NULL && foot_offset < MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
                    foot_tile = current_tiles_ptr[foot_offset];
                    if (foot_tile > tileset_last_passable) {
                        foot_solid = 1;
                    }
                    if ((comic_x & 1) && (foot_offset + 1) < MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
                        foot_tile = current_tiles_ptr[foot_offset + 1];
                        if (foot_tile > tileset_last_passable) {
                            right_foot_solid = 1;
                        }
                    }
                }

                if (!foot_solid && !right_foot_solid) {
                    /* Start falling after walking off an edge */
                    comic_y_vel = 8;
                    if (comic_x_momentum > 0) {
                        comic_x_momentum = +2;
                    } else if (comic_x_momentum < 0) {
                        comic_x_momentum = -2;
                    }
                    comic_is_falling_or_jumping = 1;
                    comic_jump_counter = 1;
                }
            }
            
            /* Clear the teleport flag after checking */
            teleport_key_pressed = 0;
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
            render_comic_hp_meter();
        }
        
        /* Handle enemies, fireballs, and items */
        handle_enemies();
        handle_fireballs();
        handle_item();
        
        /* Render inventory display items on the UI */
        render_inventory_display();
        
        /* Render score display on the UI */
        render_score_display();
        
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
    int i, j;
    
    /* Initialize items_collected array */
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 16; j++) {
            items_collected[i][j] = 0;
        }
    }
    
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
