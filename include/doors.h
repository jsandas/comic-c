/**
 * doors.h - Door system for level transitions
 * 
 * Implements door collision detection, key requirement checking, and
 * level/stage transitions via doors.
 * 
 * Matches the original assembly behavior where doors are positioned on the map
 * and require the Door Key to open. Entering a door transitions to a new level
 * or stage and plays a transition sound.
 */

#ifndef DOORS_H
#define DOORS_H

#include <stdint.h>
#include "level_data.h"

/**
 * check_door_activation - Check if Comic is in front of a door and the open key is pressed
 * 
 * When the player presses the open key:
 * 1. Iterate through all doors in the current stage
 * 2. For each door, check:
 *    - Comic's Y coordinate matches door Y coordinate exactly (both in game units)
 *    - Comic's X coordinate is within 3 units of door X (0, 1, or 2 units offset)
 * 3. If a door matches and the player has the Door Key, activate the door
 * 
 * Door positioning uses game units (same as Comic's position):
 * - Both comic_x, comic_y and door.x, door.y are in game units
 * - No coordinate conversion is needed; direct comparison is used
 * - Comic is 2 game units wide
 * 
 * Output:
 *   Returns 1 if a door was activated, 0 otherwise
 *   If activated, calls load_new_level() or load_new_stage() as appropriate
 */
uint8_t check_door_activation(void);

/**
 * activate_door - Perform door transition to target level/stage
 * 
 * Input:
 *   door = pointer to door descriptor with target level/stage
 * 
 * Marks the entry point as coming from a door, sets current level/stage
 * to the door's target, plays transition sound, and loads the new level
 * or stage as appropriate.
 * 
 * If the target is in a different level, calls load_new_level().
 * If the target is in the same level, calls load_new_stage().
 */
void activate_door(const door_t *door);

/**
 * exit_door_animation - Play the door exit animation
 * 
 * Called when Comic arrives at a new stage via a door. Plays a 5-frame
 * animation showing the door opening, Comic stepping out, and the door
 * closing again.
 */
void exit_door_animation(void);

#endif /* DOORS_H */
