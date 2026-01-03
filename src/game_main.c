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

/* Tell the compiler this function is called from assembly with FAR calling convention */
#pragma aux game_entry_c "*"

/*
 * game_entry_c - Main C entry point called from assembly
 * 
 * Called from R5sw1991_c.asm after title sequence completes.
 * Uses FAR calling convention (RETF) to match assembly's FAR call.
 * Currently a stub that returns immediately, causing assembly to
 * continue with load_new_level.
 * 
 * Future: This will contain the main game logic, calling assembly
 * functions as needed for low-level operations.
 */
void game_entry_c(void)
{
    /* TODO: Implement game logic here
     * 
     * Eventually this will:
     * 1. Initialize game state
     * 2. Call load_new_level() (assembly function)
     * 3. Run main game loop
     * 4. Handle level transitions
     * 5. Return only on game exit
     */
    
    /* For now, return immediately to let assembly continue */
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
