/*
 * rendering.h - Graphics rendering declarations
 * 
 * Functions for rendering the game to video memory.
 */

#ifndef RENDERING_H
#define RENDERING_H

#include "globals.h"

/* Prevent Open Watcom from adding underscore prefix */
#pragma aux render_map_c "*"

/*
 * Award points to the player. Points are added with carry propagation
 * through a base-100 digit system (0-99 per digit). Every 50000 points
 * (5 carries into the ten-thousands digit) awards an extra life.
 * 
 * Input:
 *   points = points to add (0-255); values >= 100 are accumulated via
 *            base-100 carry propagation and are supported but uncommon.
 * 
 * Modifies:
 *   score[] array - updates the digit values
 *   score_10000_counter - tracks carries for extra life milestones
 */
void award_points_c(uint8_t points);

/*
 * Pre-render the entire map to the offscreen video buffer.
 * 
 * This function takes the current map tiles (from current_tiles_ptr) and
 * renders them to the 40KB buffer at 0xa000:0x4000 (RENDERED_MAP_BUFFER).
 * Each tile is 16x16 pixels and is blitted plane by plane (EGA color planes).
 * 
 * The video memory layout is interleaved: each row of tiles is 256 bytes apart,
 * allowing multiple rows of tiles to fit within one "logical row" of video memory.
 * 
 * Prerequisites:
 *   - current_tiles_ptr must point to valid tile data (128x10 tiles)
 *   - tileset_graphics must contain the tile graphics (128 tiles * 128 bytes each)
 *   - VGA must be in 320x200 16-color mode
 * 
 * No parameters, no return value. Modifies video memory directly.
 */
extern void __near render_map_c(void);

#endif /* RENDERING_H */
