/**
 * physics.h - Physics and collision detection functions
 * 
 * Implements gravity, jumping, movement, and collision detection for Comic.
 */

#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>

/* Physics constants */
#define COMIC_GRAVITY           8   /* Gravity in normal levels (units of 1/8 game units per tick) */
#define COMIC_GRAVITY_SPACE     3   /* Reduced gravity in space level */
#define TERMINAL_VELOCITY       23  /* Maximum downward velocity (units of 1/8 game units per tick) */
#define JUMP_COUNTER_INITIAL    4   /* Initial value when jump starts (default without Boots) */
#define JUMP_ACCELERATION       7   /* Upward acceleration during jump (units of 1/8 per tick) */

/* Game dimensions */
#define MAP_WIDTH_TILES         128 /* Number of tile columns in map */
#define MAP_HEIGHT_TILES        10  /* Number of tile rows in map */
#define MAP_WIDTH               256 /* Map width in game units (half-tiles) */
#define MAP_HEIGHT              20  /* Map height in game units (half-tiles) */

/* Tile access functions */
uint16_t address_of_tile_at_coordinates(uint8_t x, uint8_t y);

/* Movement functions */
void handle_fall_or_jump(void);

/* Helper functions */
void move_left(void);
void move_right(void);

#endif /* PHYSICS_H */
