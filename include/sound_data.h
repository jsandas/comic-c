/**
 * Sound Data Header
 * 
 * Defines the SOUND_TITLE melody as a C array.
 * Format: pairs of (frequency_divisor, duration_ticks) terminated by 0x0000
 */

#ifndef SOUND_DATA_H
#define SOUND_DATA_DATA_H

#include <stdint.h>

#define NOTE_C3         0x23a1  /* 130.817 Hz */
#define NOTE_D3         0x1fbe  /* 146.835 Hz */
#define NOTE_E3         0x1c48  /* 164.804 Hz */
#define NOTE_F3         0x1ab1  /* 174.621 Hz */
#define NOTE_G3         0x17c8  /* 195.989 Hz */
#define NOTE_A3         0x1530  /* 219.982 Hz */
#define NOTE_B3         0x12e0  /* 246.933 Hz */
#define NOTE_C4         0x11d1  /* 261.605 Hz */
#define NOTE_D4         0x0fdf  /* 293.670 Hz */
#define NOTE_E4         0x0e24  /* 329.608 Hz */
#define NOTE_F4         0x0d59  /* 349.190 Hz */
#define NOTE_Fsharp4    0x0c99  /* 369.979 Hz */
#define NOTE_G4         0x0be4  /* 391.978 Hz */
#define NOTE_A4         0x0a98  /* 439.964 Hz */
#define NOTE_B4         0x0974  /* 493.050 Hz */
#define NOTE_C5         0x08e9  /* 523.096 Hz */
#define NOTE_D5         0x07f1  /* 586.907 Hz */

#define SOUND_TERMINATOR 0x0000

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
