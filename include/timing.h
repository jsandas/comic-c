/*
 * timing.h - Timing and synchronization utilities
 * 
 * Provides timing functions for game tick synchronization and animations.
 */

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

/*
 * wait_n_ticks - Wait for a specified number of game ticks
 * 
 * Input:
 *   ticks = number of game ticks to wait (1 tick ≈ 55ms at ~18.2 Hz)
 * 
 * This function blocks until the specified number of ticks have elapsed.
 * Used for animations and timing delays in the game.
 */
extern void wait_n_ticks(uint16_t ticks);

#endif /* TIMING_H */
