/**
 * Sound Data Header
 * 
 * Defines the SOUND_TITLE melody as a C array.
 * Format: pairs of (frequency_divisor, duration_ticks) terminated by 0x0000
 */

#ifndef SOUND_DATA_H
#define SOUND_DATA_H

#include <stdint.h>
#include "sound.h"

/* Title sequence music - melody from the original game */
static const uint16_t SOUND_TITLE[] = {
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_A3, 3,
	NOTE_A3, 6,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_G3, 6,
	NOTE_F3, 6,
	NOTE_E3, 6,
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_G3, 3,
	NOTE_G3, 5,
	NOTE_G3, 3,
	NOTE_G3, 3,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 6,
	NOTE_D3, 6,
	NOTE_D3, 3,
	NOTE_E3, 3,
	NOTE_F3, 6,
	NOTE_A3, 3,
	NOTE_A3, 5,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_A3, 3,
	NOTE_G3, 6,
	NOTE_F3, 7,
	NOTE_E3, 12,
	NOTE_A3, 6,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 3,
	NOTE_D3, 9,
	NOTE_F3, 3,
	NOTE_E3, 6,
	NOTE_D3, 12,
	NOTE_A3, 14,
	NOTE_G3, 3,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_F3, 13,
	NOTE_D3, 13,
	NOTE_G3, 15,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_D3, 3,
	NOTE_E3, 13,
	NOTE_C3, 13,
	NOTE_A3, 16,
	NOTE_G3, 3,
	NOTE_F3, 3,
	NOTE_E3, 3,
	NOTE_F3, 13,
	NOTE_D3, 11,
	NOTE_A3, 6,
	NOTE_G3, 3,
	NOTE_F3, 6,
	NOTE_E3, 3,
	NOTE_D3, 10,
	NOTE_F3, 3,
	NOTE_E3, 6,
	NOTE_D3, 10,
	SOUND_TERMINATOR, 0
};

#endif /* SOUND_DATA_H */
