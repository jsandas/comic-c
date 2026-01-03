/*
 * game_main.c - Main game loop in C
 * 
 * This is the C entry point called from assembly after initialization
 * (entry point, DS init, interrupt handlers, hardware setup, title sequence).
 * 
 * Assembly calls game_entry_c() after the title sequence completes, then
 * jumps to load_new_level when this function returns.
 */

#include "globals.h"
#include "assembly.h"

/* Tell the compiler these functions are called from assembly */
#pragma aux game_entry_c "*"
#pragma aux terminate_program_c "*"

/* Sound operation codes (from assembly) */
#define SOUND_MUTE 2

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

/*
 * Simple string copy helper
 */
static void copy_string(char* dest, const char* src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/*
 * load_new_level_c - C stub for loading a new level
 * 
 * Attempts to open the TT2 file for the current level using DOS interrupt.
 * Outputs a simple message to indicate success or failure.
 * 
 * Currently a stub for testing - will eventually handle full level loading.
 */
void load_new_level_c(void)
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
 * terminate_program_c - Terminate the program cleanly (C implementation)
 * 
 * Mutes sound, restores video mode, restores interrupt handlers, and exits to DOS.
 * This is a pure C implementation of the assembly terminate_program function.
 * Currently not wired up - assembly still uses its own terminate_program.
 * Will be used later once more logic is moved to C.
 * 
 * NOTE: Uses single asm block to prevent compiler from generating epilogue code
 * after the DOS exit call (which never returns).
 */
void __far terminate_program_c(void)
{
    __asm {
        ; Mute sound using INT 3
        mov ax, SOUND_MUTE
        int 3
        
        ; Restore video mode using INT 10h
        mov ax, cs:saved_video_mode
        int 0x10
        
        ; Restore interrupt handlers (inlined from restore_interrupt_handlers)
        push ds
        xor ax, ax
        mov ds, ax
        cli
        
        ; Restore INT 9 (keyboard) - vector at 0000:0024
        mov bx, 36        ; 9*4
        mov ax, cs:saved_int9_handler_segment
        mov [bx+2], ax
        mov ax, cs:saved_int9_handler_offset
        mov [bx], ax
        
        ; Restore INT 8 (timer) - vector at 0000:0020
        mov bx, 32        ; 8*4
        mov ax, cs:saved_int8_handler_segment
        mov [bx+2], ax
        mov ax, cs:saved_int8_handler_offset
        mov [bx], ax
        
        ; Restore INT 35 (Ctrl-Break) - vector at 0000:008C
        mov bx, 140       ; 35*4
        mov ax, cs:saved_int35_handler_segment
        mov [bx+2], ax
        mov ax, cs:saved_int35_handler_offset
        mov [bx], ax
        
        ; Restore INT 3 (sound) - vector at 0000:000C
        mov bx, 12        ; 3*4
        mov ax, cs:saved_int3_handler_segment
        mov [bx+2], ax
        mov ax, cs:saved_int3_handler_offset
        mov [bx], ax
        
        sti
        pop ds
        
        ; Exit to DOS with INT 21h, AH=4Ch (never returns)
        mov ah, 0x4c
        xor al, al
        int 0x21
    }
}

/*
 * game_entry_c - Main C entry point called from assembly
 * 
 * Called from R5sw1991_c.asm after title sequence completes.
 * Uses FAR calling convention (RETF) to match assembly's FAR call.
 * Currently calls load_new_level_c stub, then returns to assembly
 * which continues with load_new_level.
 * 
 * Future: This will contain the main game logic, calling assembly
 * functions as needed for low-level operations.
 */
void game_entry_c(void)
{
    /* Call the level loading stub */
    load_new_level_c();
    
    /* Return to assembly which will continue with load_new_level */
    return;
}

/*
 * Future: game_loop_iteration() - C implementation of one game tick
 * 
 * This will replace game_loop_asm() when ported to C.
 * For now, it's a placeholder showing the intended structure.
 */
#if 0
int game_loop_iteration(void)
{
    /* Wait for game tick */
    while (!game_tick_flag) {
        /* Recharge jump counter if not falling/jumping */
        if (!comic_is_falling_or_jumping && !key_state_jump) {
            /* recharge_jump_counter() */
        }
    }
    game_tick_flag = 0;
    
    /* Check win condition */
    if (win_counter > 0) {
        win_counter--;
        if (win_counter == 1) {
            game_end_sequence();  /* Assembly function */
            return LOOP_EXIT_WIN;
        }
    }
    
    /* Update animation counters */
    /* comic_run_cycle++; */
    
    /* Award pending HP */
    /* if (comic_hp_pending_increase > 0) increment_comic_hp(); */
    
    /* Handle player state */
    if (comic_is_teleporting) {
        handle_teleport();  /* Assembly function */
        goto handle_nonplayer_actors;
    }
    
    if (comic_is_falling_or_jumping) {
        handle_fall_or_jump();  /* Assembly function */
        goto handle_nonplayer_actors;
    }
    
    /* Input handling */
    if (key_state_jump /* && !exhausted */) {
        /* start_jump(); */
    }
    if (key_state_open /* && near_door && has_key */) {
        /* activate_door(); */
    }
    if (key_state_teleport && comic_has_teleport_wand) {
        /* begin_teleport(); */
    }
    if (key_state_left) {
        /* face_or_move_left(); */
    }
    if (key_state_right) {
        /* face_or_move_right(); */
    }
    
    /* Check for floor (start falling if walked off edge) */
    /* if (!standing_on_solid) start_falling(); */
    
    /* Input: pause/fire */
    if (key_state_esc) {
        pause();  /* Assembly function */
        /* May return LOOP_EXIT_QUIT if user chose to quit */
    }
    if (key_state_fire && fireball_meter > 0) {
        /* try_to_fire(); */
    } else {
        /* recover_fireball_meter(); */
    }
    
    /* Render */
    blit_map_playfield_offscreen();   /* Assembly function */
    blit_comic_playfield_offscreen(); /* Assembly function */
    
handle_nonplayer_actors:
    handle_enemies();   /* Assembly function */
    handle_fireballs(); /* Assembly function */
    handle_item();      /* Assembly function */
    swap_video_buffers(); /* Assembly function */
    
    return LOOP_CONTINUE;
}
#endif
