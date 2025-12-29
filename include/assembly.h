/*
 * assembly.h - Declarations for assembly functions called from C
 * 
 * These functions remain in assembly and are called from C code.
 * Use #pragma aux "*" to prevent name mangling.
 */

#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "globals.h"

/* Prevent Open Watcom from adding underscore prefix */
#pragma aux load_new_level "*"
#pragma aux load_new_stage "*"
#pragma aux game_loop "*"
#pragma aux swap_video_buffers "*"
#pragma aux wait_n_ticks "*"
#pragma aux blit_map_playfield_offscreen "*"
#pragma aux blit_comic_playfield_offscreen "*"
#pragma aux handle_enemies "*"
#pragma aux handle_fireballs "*"
#pragma aux handle_item "*"
#pragma aux handle_fall_or_jump "*"
#pragma aux handle_teleport "*"
#pragma aux pause "*"
#pragma aux game_over "*"
#pragma aux game_end_sequence "*"
#pragma aux render_map "*"

/* Level and stage management */
extern void load_new_level(void);
extern void load_new_stage(void);
extern void render_map(void);

/* Main game loop (assembly implementation) */
extern void game_loop(void);

/* Video/rendering functions */
extern void swap_video_buffers(void);
extern void blit_map_playfield_offscreen(void);
extern void blit_comic_playfield_offscreen(void);

/* Actor handling functions */
extern void handle_enemies(void);
extern void handle_fireballs(void);
extern void handle_item(void);
extern void handle_fall_or_jump(void);
extern void handle_teleport(void);

/* Game state functions */
extern void pause(void);
extern void game_over(void);
extern void game_end_sequence(void);

/* Utility functions */
extern void wait_n_ticks(uint16_t n);

#endif /* ASSEMBLY_H */
