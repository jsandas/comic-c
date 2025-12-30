/*
 * game_main.c - Main game loop in C
 * 
 * This is the C entry point called from assembly after initialization.
 * Initially wraps existing assembly functions, then incrementally converts
 * them to C.
 */

#include "globals.h"
#include "assembly.h"

/*
 * C implementation of the game loop
 * Translated from assembly at reference/disassembly/R5sw1991_c.asm:3861
 */
void game_loop(void)
{
    while (1) {
        /* Wait for game tick (IRQ0 timer interrupt sets game_tick_flag) */
        while (!game_tick_flag) {
            /* While waiting, recharge jump counter if on ground and not pressing jump */
            if (!comic_is_falling_or_jumping && !key_state_jump) {
                comic_jump_counter = comic_jump_power;
            }
        }
        game_tick_flag = 0;
        
        /* Check win condition */
        if (win_counter > 0) {
            win_counter--;
            if (win_counter == 1) {
                game_end_sequence();
                return; /* Exit after winning */
            }
        }
        
        /* Advance running animation cycle */
        comic_run_cycle++;
        if (comic_run_cycle > COMIC_RUNNING_3) {
            comic_run_cycle = COMIC_RUNNING_1;
        }
        
        /* Default to standing animation */
        comic_animation = COMIC_STANDING;
        
        /* Award pending HP */
        if (comic_hp_pending_increase > 0) {
            comic_hp_pending_increase--;
            increment_comic_hp();
        }
        
        /* Handle special states */
        if (comic_is_teleporting) {
            handle_teleport();
            goto handle_nonplayer_actors;
        }
        
        if (comic_is_falling_or_jumping) {
            handle_fall_or_jump();
            goto check_pause_input;
        }
        
        /* Jump input */
        if (key_state_jump && comic_jump_counter > 1) {
            comic_is_falling_or_jumping = 1;
            handle_fall_or_jump();
            goto check_pause_input;
        } else if (!key_state_jump) {
            comic_jump_counter = comic_jump_power;
        }
        
        /* Door opening - check if near door and has key */
        if (key_state_open && comic_has_door_key) {
            activate_door(); /* May load new level/stage */
            continue;
        }
        
        /* Teleport */
        if (key_state_teleport && comic_has_teleport_wand) {
            begin_teleport();
            continue;
        }
        
        /* Movement input */
        comic_x_momentum = 0;
        if (key_state_left) {
            comic_x_momentum = -5;
            face_or_move_left();
        }
        if (key_state_right) {
            comic_x_momentum = 5;
            face_or_move_right();
        }
        
        /* TODO: Floor checking logic from assembly
         * This checks if Comic walked off an edge and initiates falling.
         * For now handled by physics code in handle_fall_or_jump.
         */
        
check_pause_input:
        /* Pause menu */
        if (key_state_esc) {
            pause();
            /* Wait for key release to avoid flickering */
            while (key_state_esc) {
                /* busy wait */
            }
        }
        
        /* Fire input and fireball meter recovery */
        if (key_state_fire && fireball_meter > 0) {
            /* Fire key pressed and meter available: shoot fireball */
            try_to_fire();
            
            /* Meter adjustment based on counter alternating 2, 1, 2, 1, ...
               Counter == 1: reset to 2 (skip decrement)
               Counter == 2: decrement meter */
            if (fireball_meter_counter != 1) {
                decrement_fireball_meter();
            }
        }
        
        /* Always decrement counter (whether fire key pressed or not) */
        fireball_meter_counter--;
        if (fireball_meter_counter == 0) {
            /* Counter reached 0: increment meter and reset to 2 */
            increment_fireball_meter();
            fireball_meter_counter = 2;
        } else if (fireball_meter_counter == 1) {
            /* Counter just became 1: reset it to 2 */
            fireball_meter_counter = 2;
        }
        
        /* Render */
        blit_map_playfield_offscreen();
        blit_comic_playfield_offscreen();
        
handle_nonplayer_actors:
        handle_enemies();
        handle_fireballs();
        handle_item();
        swap_video_buffers();
    }
}

/* 
 * Main game function called from assembly
 * 
 * Called after:
 *   - DS segment initialized
 *   - Interrupt handlers installed
 *   - Video mode set to 320x200 16-color EGA
 *   - Title sequence completed
 *   - Lives initialized (comic_num_lives = MAX_NUM_LIVES - 1)
 * 
 * Returns:
 *   GAME_EXIT_QUIT (0) - user quit via ESC menu
 *   GAME_EXIT_WIN (1) - player won the game
 *   GAME_EXIT_GAMEOVER (2) - player lost all lives
 */
#pragma aux game_main "*"
int game_main(void)
{
    /* Load the initial level */
    load_new_level();
    
    /* Enter the main game loop (C implementation!) */
    game_loop();
    
    /* Game loop returns when game ends */
    return GAME_EXIT_QUIT;
}

/*
 * Future: Incremental conversion plan
 * 
 * Phase 3 (Rendering): Convert blit_* functions to C
 * Phase 4 (Physics): Convert handle_fall_or_jump, movement functions to C
 * Phase 5 (Actors): Convert handle_enemies, handle_fireballs, handle_item to C
 * Phase 6 (Game State): Convert door activation, teleport, win/lose logic to C
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
