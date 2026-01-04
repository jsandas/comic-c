/**
 * Sound Data Header
 * 
 * Declares all game sound effects and music for external linkage.
 * Format: pairs of (frequency_divisor, duration_ticks) terminated by SOUND_TERMINATOR
 * 
 * Actual definitions are in sound_data.c to avoid duplicate copies in each
 * translation unit that includes this header.
 */

#ifndef SOUND_DATA_H
#define SOUND_DATA_H

#include <stdint.h>
#include "sound.h"

/* Title sequence music */
extern const uint16_t SOUND_TITLE[];

/* Sound effects */
extern const uint16_t SOUND_DOOR[];
extern const uint16_t SOUND_DEATH[];
extern const uint16_t SOUND_TELEPORT[];
extern const uint16_t SOUND_SCORE_TALLY[];
extern const uint16_t SOUND_MATERIALIZE[];
extern const uint16_t SOUND_GAME_OVER[];
extern const uint16_t SOUND_STAGE_EDGE_TRANSITION[];
extern const uint16_t SOUND_TOO_BAD[];
extern const uint16_t SOUND_FIRE[];
extern const uint16_t SOUND_HIT_ENEMY[];
extern const uint16_t SOUND_DAMAGE[];
extern const uint16_t SOUND_COLLECT_ITEM[];
extern const uint16_t SOUND_EXTRA_LIFE[];

#endif /* SOUND_DATA_H */
