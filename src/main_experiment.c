/*
 * main_experiment.c - Experimental C-only entry point
 * 
 * This is an experiment to initialize the game entirely from C code,
 * linking with Watcom's standard libraries to access DOS functions.
 */

#include <stdint.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>

/* XOR encryption key used for text obfuscation (not used in this experiment) */
#define XOR_ENCRYPTION_KEY 0x2D

/* Interrupt handler sentinel for verification */
#define INTERRUPT_HANDLER_INSTALL_SENTINEL 0x25

/* Helper macro to get the near offset of a pointer for DOS interrupts.
 * In the large memory model with a single data segment (DGROUP), this
 * extracts the 16-bit offset portion of a pointer for use in DS:DX addressing. */
#define DOS_OFFSET(ptr) ((uint16_t)(uintptr_t)(ptr))

/* Global variables for interrupt handling */
static uint8_t interrupt_handler_install_sentinel = 0;

/* Game tick flag - set by interrupt handler (IRQ 0) on odd cycles */
static volatile uint8_t game_tick_flag = 0;

/* Maximum joystick reads - calibrated based on CPU speed */
static uint16_t max_joystick_reads = 0;

/* Saved video mode - upper 8 bits are 0x00, lower 8 bits are the video mode */
static uint16_t saved_video_mode = 0;

/* Default keymap for keyboard configuration */
static uint8_t keymap[6] = {
    0x39,  /* SCANCODE_SPACE */
    0x52,  /* SCANCODE_INS */
    0x4B,  /* SCANCODE_LEFT */
    0x4D,  /* SCANCODE_RIGHT */
    0x38,  /* SCANCODE_ALT */
    0x3A   /* SCANCODE_CAPSLOCK */
};

/* Startup notice text (plain, not encrypted for this experiment) */
static const char STARTUP_NOTICE_TEXT[] = 
    "Captain Comic - C Experiment\r\n"
    "\r\n"
    "Press K for Keyboard setup\r\n"
    "Press J for Joystick calibration\r\n"
    "Press R for Registration info\r\n"
    "Press ESC to quit\r\n"
    "Press any other key to continue...\r\n"
    "\r\n";

/* Registration notice text */
static const char REGISTRATION_NOTICE_TEXT[] = 
    "Captain Comic - Experimental C Version\r\n"
    "\r\n"
    "This is an experimental C implementation\r\n"
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

/* Keyboard setup messages */
static const char KEYBOARD_SETUP_MESSAGE[] = 
    "Keyboard Setup\r\n"
    "(Simplified - using defaults)\r\n"
    "Press any key...\r\n$";

/* Joystick calibration messages */
static const char JOYSTICK_CALIB_MESSAGE[] = 
    "Joystick Calibration\r\n"
    "(Simplified - skipped)\r\n"
    "Press any key...\r\n$";

/* Title sequence message */
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
 * Note: In a real implementation, game_tick_flag would be set by the
 * interrupt handler (int8_handler) on IRQ 0. For this experiment, we
 * simulate the tick by using DOS INT 1Ah to read the system timer.
 */
void wait_n_ticks(uint16_t ticks)
{
    union REGS regs;
    uint32_t start_ticks, current_ticks;
    
    /* In a real implementation with interrupt handlers installed:
     * We would loop until game_tick_flag is set 'ticks' times,
     * and the interrupt handler would set game_tick_flag = 1 on odd IRQ 0 cycles.
     * 
     * For this experiment, we'll use DOS INT 1Ah to read the system timer.
     * The BIOS timer ticks at ~18.2065 Hz, and the game runs at half that (~9.1 Hz).
     */
    
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
 * calibrate_joystick_timing - Measure CPU speed for joystick calibration
 * 
 * This function measures how many IN instructions can execute during one
 * game tick. This count is used to calibrate joystick polling timing.
 * 
 * In the real implementation, an interrupt handler sets game_tick_flag = 1
 * on each timer tick. For this experimental version without interrupt handlers,
 * we use a pure C approach that simulates the timing: we loop performing IN
 * instructions while periodically checking a BIOS timer. When one tick elapses,
 * we calculate how many iterations completed.
 * 
 * The calibration process:
 * 1. Wait until the start of a tick boundary (wait_n_ticks)
 * 2. Get the starting BIOS tick count
 * 3. Loop performing IN instructions and checking the timer every 256 iterations
 * 4. When a tick elapses, stop and calculate the total iterations
 * 5. Calculate max_joystick_reads = count/28
 * 6. Apply weighted average toward 1280
 * 
 * Output:
 *   max_joystick_reads is set to the calibrated value
 */
void calibrate_joystick_timing(void)
{
    union REGS regs;
    uint32_t start_ticks, current_ticks;
    uint16_t iteration_count = 0;
    uint16_t inner_loop;
    uint16_t count;
    uint16_t adjusted;
    uint16_t difference;
    
    /* Wait until the beginning of a tick interval */
    wait_n_ticks(1);
    
    /* Get the starting tick count from BIOS */
    regs.h.ah = 0x00;  /* AH=0x00: read system timer */
    int86(0x1A, &regs, &regs);
    start_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
    
    /* Perform IN instructions in a tight loop, checking the timer periodically.
     * We count how many "outer loop" iterations we can complete in one tick. */
    do {
        /* Inner loop: perform multiple IN instructions (simulating instruction counting) */
        for (inner_loop = 0; inner_loop < 28; inner_loop++) {
            __asm {
                in al, dx       ; Dummy IN instruction
            }
        }
        
        iteration_count++;
        
        /* Check timer every so often (for responsiveness) */
        if ((iteration_count & 0x0F) == 0) {  /* Check every 16 iterations */
            regs.h.ah = 0x00;
            int86(0x1A, &regs, &regs);
            current_ticks = ((uint32_t)regs.w.cx << 16) | regs.w.dx;
        }
    } while ((current_ticks - start_ticks) < 1);  /* Continue for one tick */
    
    /* Calculate the actual count based on iterations and IN instructions per iteration */
    count = iteration_count * 28;
    
    /* Calculate max_joystick_reads = count/28 = iteration_count */
    adjusted = count / 28;
    max_joystick_reads = adjusted;
    
    /* Apply weighted average: inflate by 25% of the interval between
     * the calculated value and 1280, unless already greater than 1280.
     * 
     * Formula: max_joystick_reads = max(count/28, 0.75*(count/28) + 0.25*1280)
     */
    if (adjusted < 1280) {
        difference = 1280 - adjusted;
        difference >>= 2;  /* 0.25 * (1280 - adjusted) */
        max_joystick_reads = adjusted + difference;
    }
}

/*
 * install_handler_sentinel - Install and verify interrupt handler sentinel
 * 
 * This is a simplified version that just sets a sentinel value.
 * In the real game, this would install custom interrupt handlers and verify
 * they were installed correctly by having the INT 3 handler bit-flip the value.
 * 
 * For this experiment, we just simulate the bit-flip.
 */
int install_handler_sentinel(void)
{
    /* Set the sentinel value */
    interrupt_handler_install_sentinel = INTERRUPT_HANDLER_INSTALL_SENTINEL;
    
    /* In the real game, we would call INT 3 here with an unknown operation
     * and the handler would bit-flip the sentinel value. For this experiment,
     * we'll just simulate that by flipping it once. */
    interrupt_handler_install_sentinel ^= 0xFF;
    
    /* Don't verify here - the check happens later in check_interrupt_handler_install_sentinel() */
    return 1;  /* Success */
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
    
    /* Sets video mode 0x0D (13 decimal, 320x200 16-color EGA) */
    regs.h.ah = 0x00;  /* AH=0x00: set video mode */
    regs.h.al = 0x0D;  /* AL=0x0D: 320x200 16-color EGA */
    int86(0x10, &regs, &regs);
    
    /* Verify that video mode 0x0D (13 decimal) was actually set */
    regs.h.ah = 0x0F;  /* AH=0x0F: get video mode */
    int86(0x10, &regs, &regs);
    
    if (regs.h.al != 0x0D) {
        return 0;  /* Video mode 0x0D was not set */
    }
    
    /* Check for sufficient EGA memory (>=256K).
     * According to EGA BIOS documentation, after INT 10h AH=12h BL=10h:
     *   BL=0 -> 64K, BL=1 -> 128K, BL=2 -> 192K, BL=3 -> 256K.
     * We treat BL>=3 as "at least 256K" in case larger values are used
     * by some implementations to indicate more than 256K.
     */
    regs.h.ah = 0x12;  /* AH=0x12: get EGA info */
    regs.h.bl = 0x10;  /* BL=0x10: return EGA info */
    int86(0x10, &regs, &regs);
    
    if (regs.h.bl >= 3) {
        return 1;  /* Adequate EGA support */
    }
    
    return 0;  /* Insufficient memory */
}

/*
 * display_ega_error - Display EGA error message and exit
 * 
 * Sets text mode, displays error message, and returns (caller should exit).
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
}

/*
 * load_keymap_file - Load keyboard configuration from KEYS.DEF if it exists
 * 
 * Attempts to open and read KEYS.DEF (6 bytes) into the keymap array.
 * If the file doesn't exist, the default keymap is used.
 * 
 * Returns:
 *   0 if file doesn't exist (default keymap is used)
 *   1 if file was successfully loaded
 */
int load_keymap_file(void)
{
    union REGS regs;
    int file_handle;
    
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
    
    /* Close the file */
    regs.h.ah = 0x3E;  /* AH=0x3E: close file */
    regs.x.bx = file_handle;
    int86(0x21, &regs, &regs);
    
    return 1;
}

/*
 * check_interrupt_handler_install_sentinel - Verify interrupt handlers were installed
 * 
 * Checks that the interrupt_handler_install_sentinel value was bit-flipped by the
 * int3_handler, which should have been called during install_handler_sentinel().
 * 
 * Returns:
 *   0 if the sentinel check failed (handlers not installed)
 *   1 if the sentinel check passed (handlers were installed)
 */
int check_interrupt_handler_install_sentinel(void)
{
    /* Undo the bit-flip that install_handler_sentinel should have done */
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
 * Simplified stub for keyboard setup. In the real implementation, this would:
 * - Prompt for each key mapping (left, right, jump, fire, open, teleport)
 * - Verify no conflicts
 * - Optionally save to KEYS.DEF
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
    
    /* For this experiment, return to continue to title (simulating completion) */
    return 1;
}

/*
 * calibrate_joystick_interactive - Interactive joystick calibration
 * 
 * Simplified stub for joystick calibration. In the real implementation, this would:
 * - Prompt to center joystick and press button
 * - Prompt to move left/right/up/down and press button for each
 * - Calculate threshold values
 * - Allow cancellation with keyboard
 * 
 * Returns:
 *   0 to return to startup notice (cancelled or completed)
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
    
    /* Return to startup notice */
    return 0;
}

/*
 * title_sequence - Stub for title sequence
 * 
 * In the real game, this would display the title screen, story screens, etc.
 * For this experiment, just display a message and exit.
 */
void title_sequence(void)
{
    union REGS regs;
    
    /* Set video mode to text */
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int86(0x10, &regs, &regs);
    
    /* Display message */
    regs.h.ah = 0x09;
    regs.x.dx = DOS_OFFSET(TITLE_SEQUENCE_MESSAGE);
    int86(0x21, &regs, &regs);
}

/*
 * display_startup_notice - Display startup screen and wait for keypress
 * 
 * Implements the assembly function display_startup_notice in C.
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
        /* Setup cancelled or returned, show startup notice again */
        return display_startup_notice();
    }
    
    if (key_ascii == 'j' || key_ascii == 'J') {
        /* Joystick calibration */
        calibrate_joystick_interactive();
        /* Always return to startup notice after calibration */
        return display_startup_notice();
    }
    
    if (key_ascii == 'r' || key_ascii == 'R') {
        /* Registration/about information */
        display_registration_notice();
        /* Always return to startup notice after viewing registration */
        return display_startup_notice();
    }
    
    if (key_ascii == 27) {
        /* Escape key - exit program */
        return 0;
    }
    
    /* Any other key - continue to title sequence */
    return 1;
}

/*
 * main - C entry point
 * 
 * Entry point for the experimental C-only version.
 * Performs initialization steps from assembly main function:
 * - DS initialization (handled automatically by C runtime)
 * - Disable PC speaker
 * - Install interrupt handler sentinel (simplified)
 * - Calibrate CPU speed for joystick timing
 * - Save current video mode
 * - Check for sufficient EGA support and set video mode
 * - Load keymap from KEYS.DEF if present
 * - Verify interrupt handlers were installed
 * Then displays the startup notice and exits.
 */
int main(void)
{
    /* Note: Data segment (DS) initialization is handled automatically by
     * the Watcom C runtime, so we don't need to do it manually like in assembly.
     * The C runtime sets up DS to point to DGROUP before calling main(). */
    
    /* Disable the PC speaker */
    disable_pc_speaker();
    
    /* Install and verify interrupt handler sentinel (simplified) */
    if (!install_handler_sentinel()) {
        /* In the real game, this would terminate the program */
        /* For this experiment, we'll just continue */
    }
    
    /* Calibrate CPU speed for joystick timing */
    calibrate_joystick_timing();
    
    /* Save the current video mode for later restoration */
    save_video_mode();
    
    /* Check for sufficient EGA support and set video mode */
    if (!check_ega_support()) {
        /* EGA support is insufficient */
        display_ega_error();
        return 1;  /* Exit with error */
    }
    
    /* Load keyboard configuration from KEYS.DEF if it exists */
    load_keymap_file();
    
    /* Verify that the custom interrupt handlers were installed */
    if (!check_interrupt_handler_install_sentinel()) {
        /* Interrupt handlers were not installed */
        /* In the real game, this would terminate the program */
        /* For this experiment, we'll just display a message and continue */
        union REGS regs;
        regs.h.ah = 0x00;  /* Set text mode first */
        regs.h.al = 0x03;
        int86(0x10, &regs, &regs);
        return 1;  /* Exit with error */
    }
    
    /* Display the startup notice and handle user input */
    if (!display_startup_notice()) {
        /* User pressed Escape to exit */
        return 0;
    }
    
    /* User chose to continue - show title sequence */
    title_sequence();
    
    /* Exit normally */
    return 0;
}
