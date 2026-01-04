/**
 * Sound Data Header
 * 
 * Declares the SOUND_TITLE melody for external linkage.
 * Format: pairs of (frequency_divisor, duration_ticks) terminated by 0x0000
 * 
 * Actual definition is in sound_data.c to avoid duplicate copies in each
 * translation unit that includes this header.
 */

#ifndef SOUND_DATA_H
#define SOUND_DATA_H

#include <stdint.h>
#include "sound.h"

/* Title sequence music - melody from the original game */
extern const uint16_t SOUND_TITLE[];

#endif /* SOUND_DATA_H */
