/**
 * PC Speaker Sound Module Header
 * 
 * Provides PC speaker sound playback through direct hardware access.
 * Uses PIT (Programmable Interval Timer) channel 2 connected to the PC speaker.
 * Sound is synchronized with game ticks via the INT 8 timer interrupt handler.
 * 
 * Note frequencies are calculated as:
 *   Frequency = 1193182 Hz (PIT base clock) / divisor
 * 
 * For example: NOTE_D3 (146.835 Hz) = 1193182 / 0x1fbe
 */

#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

/**
 * Sound operation codes - used with INT 3 handler
 */
#define SOUND_UNMUTE    0  /* Enable sound output */
#define SOUND_PLAY      1  /* Start playing a sound */
#define SOUND_MUTE      2  /* Disable sound output */
#define SOUND_STOP      3  /* Stop current sound */
#define SOUND_QUERY     4  /* Query sound status */

/**
 * Musical note frequencies (PIT divisors)
 * https://en.wikipedia.org/wiki/Piano_key_frequencies#List
 */
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

#define SOUND_TERMINATOR 0x0000  /* End-of-song marker */

/**
 * Sound data format: array of (frequency_divisor, duration_ticks) pairs
 * Sound data must be in code/data segment and terminated with SOUND_TERMINATOR
 * 
 * Example:
 *   sound_data: dw NOTE_D3, 3
 *               dw NOTE_E3, 3
 *               dw SOUND_TERMINATOR, 0
 */

/**
 * Play a sound through the PC speaker
 * 
 * @param sound_data - pointer to sound data array (frequency/duration pairs)
 * @param priority - sound priority (higher priority sounds can interrupt lower ones)
 */
void play_sound(uint16_t *sound_data, uint8_t priority);

/**
 * Stop the current sound
 */
void stop_sound(void);

/**
 * Unmute the PC speaker (enable sound output)
 */
void unmute_sound(void);

/**
 * Mute the PC speaker (disable sound output)
 */
void mute_sound(void);

/**
 * Query whether a sound is currently playing
 * 
 * @return 1 if sound is playing, 0 otherwise
 */
uint8_t is_sound_playing(void);

/**
 * Advance sound playback by one game tick
 * 
 * This function must be called once per game tick from the main game loop
 * to advance note playback. Without calling this, music will not play.
 * 
 * Usage:
 *   while (game_running) {
 *       // game logic
 *       sound_advance_tick();  // Call once per tick
 *       // render
 *   }
 */
void sound_advance_tick(void);

#endif /* SOUND_H */
