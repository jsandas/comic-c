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

/* Video display constants (used by game_main.c, graphics.c, doors.c, physics.c) */
#define SCREEN_WIDTH            320     /* in pixels - EGA mode 0x0D */
#define SCREEN_HEIGHT           200     /* in pixels - EGA mode 0x0D: 320x200 */
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

/* ===== Score Macros (Score Storage: 3-byte little-endian format) ===== */
/* The score is stored as 3 bytes in little-endian order:
 *   score_bytes[0] = LSB (low byte, bits 0-7)
 *   score_bytes[1] = middle byte (bits 8-15)
 *   score_bytes[2] = MSB (high byte, bits 16-23)
 * 
 * This supports values from 0 to 999,999 (3 decimal digits per byte: 0-255 maps to 0-999).
 * This format matches the original assembly implementation.
 */

/* score_get_value - Reconstruct a 32-bit score from three bytes
 * 
 * Combines the three score_bytes into a single 32-bit unsigned integer by:
 * 1. Shifting byte[2] (MSB) left 16 bits to the high byte position
 * 2. Shifting byte[1] (middle) left 8 bits to the middle byte position
 * 3. Using byte[0] (LSB) as-is in the low byte position
 * 4. ORing all three values together
 * 
 * Example: score_bytes = [0x12, 0x34, 0x56] produces 0x563412 (5,649,426 decimal)
 * 
 * The (uint32_t) casts ensure proper 32-bit arithmetic during shifts.
 */
#define score_get_value() (((uint32_t)score_bytes[2] << 16) | ((uint32_t)score_bytes[1] << 8) | (uint32_t)score_bytes[0])

/* score_set_value - Store a 32-bit value into three bytes (little-endian)
 * 
 * Decomposes a 32-bit unsigned integer into three bytes:
 * - byte[0] = low byte (value & 0xFF)
 * - byte[1] = middle byte ((value >> 8) & 0xFF)
 * - byte[2] = high byte ((value >> 16) & 0xFF)
 * 
 * Wrapped in do-while(0) to safely use in all contexts, including
 * after if statements without braces.
 */
#define score_set_value(v) do { score_bytes[0] = (v) & 0xFF; score_bytes[1] = ((v) >> 8) & 0xFF; score_bytes[2] = ((v) >> 16) & 0xFF; } while(0)

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

