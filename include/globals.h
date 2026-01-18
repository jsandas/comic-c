/*
 * globals.h - Game constants and type definitions
 * 
 * This header contains game-wide constants and type definitions.
 * As of 2026-01-03, the C-only entry point model means most game state
 * is now local to game_main.c for initialization/menu logic.
 * 
 * Future: Will contain declarations for assembly global variables as needed
 * when assembly functions are called from C.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

/* Type definitions */
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef int8_t   int8;
typedef int16_t  int16;

/* Debug logging (writes to DEBUG.LOG) */
void debug_log(const char *fmt, ...);
void debug_log_close(void);

/* ===== Game Map Constants ===== */
#define MAP_WIDTH_TILES         128     /* Map width in tiles */
#define MAP_HEIGHT_TILES        10      /* Map height in tiles */
#define MAP_WIDTH               256     /* MAP_WIDTH_TILES * 2, in game units (8 pixels = 1 unit) */
#define MAP_HEIGHT              20      /* MAP_HEIGHT_TILES * 2, in game units */

/* Playfield dimensions (visible portion during gameplay) */
#define PLAYFIELD_WIDTH         24      /* in game units */
#define PLAYFIELD_HEIGHT        20      /* in game units (MAP_HEIGHT) */

#define SCREEN_WIDTH            320     /* in pixels */
#define RENDERED_MAP_BUFFER     0x4000  /* offset in video segment 0xa000 */

/* ===== Level Numbers ===== */
#define LEVEL_NUMBER_LAKE       0
#define LEVEL_NUMBER_FOREST     1
#define LEVEL_NUMBER_SPACE      2
#define LEVEL_NUMBER_BASE       3
#define LEVEL_NUMBER_CAVE       4
#define LEVEL_NUMBER_SHED       5
#define LEVEL_NUMBER_CASTLE     6
#define LEVEL_NUMBER_COMP       7

/* ===== Sprite Format Constants (from .SHP files) ===== */
#define ENEMY_HORIZONTAL_DUPLICATED  1  /* right-facing frames copied from left-facing */
#define ENEMY_HORIZONTAL_SEPARATE    2  /* right-facing frames stored separately */
#define ENEMY_ANIMATION_LOOP         0  /* animation loops 0,1,2,3,0,1,2,3... */
#define ENEMY_ANIMATION_ALTERNATE    1  /* animation alternates 0,1,2,1,0,1,2,1... */

/* ===== Comic (Player) Animation States ===== */
#define COMIC_STANDING          0
#define COMIC_RUNNING_1         1
#define COMIC_RUNNING_2         2
#define COMIC_RUNNING_3         3
#define COMIC_JUMPING           4

/* ===== Comic Facing Direction ===== */
#define COMIC_FACING_RIGHT      0
#define COMIC_FACING_LEFT       5      /* offset for left-facing frames */

/* ===== Player State (will be needed when porting game logic to C) ===== */
/* Note: Currently these are in assembly if used there */
/* extern uint8_t comic_x; */
/* extern uint8_t comic_y; */
/* extern uint8_t comic_animation; */
/* extern uint8_t comic_facing; */

/* ===== Game State (currently local to game_main.c) ===== */
/* Note: Now local to game_main.c for C-only initialization */
/* extern volatile uint8_t game_tick_flag; */
/* extern uint16_t saved_video_mode; */
/* extern uint8_t current_level_number; */
/* extern uint8_t current_stage_number; */

/* ===== Input State (updated by interrupt handler) ===== */
/* extern uint8_t key_state_esc; */
/* extern uint8_t key_state_jump; */

/* ===== Graphics and Map Data ===== */
/* extern uint8_t tileset_graphics[]; */
/* extern uint8_t *current_tiles_ptr; */

#endif /* GLOBALS_H */

