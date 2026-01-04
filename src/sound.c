/**
 * PC Speaker Sound Driver (Pure C Implementation)
 * 
 * Direct hardware access to PC speaker through PIT (Programmable Interval Timer).
 * No assembly code - all sound control from C.
 * 
 * Hardware:
 *   Port 0x43 - PIT mode/command register
 *   Port 0x42 - PIT channel 2 data port (connected to PC speaker)
 *   Port 0x61 - System control port B (speaker gate and output)
 */

#include <dos.h>
#include <conio.h>
#include "sound.h"

/**
 * Sound playback state
 */
static struct {
	const uint16_t *data;           /* Pointer to sound data array */
	uint16_t offset;                /* Current position in sound data */
	uint16_t note_counter;          /* Ticks remaining for current note */
	uint8_t is_playing;             /* 1 if sound is playing */
	uint8_t is_enabled;             /* 1 if sound output is enabled */
	uint8_t priority;               /* Priority of current sound */
} sound_state = {0, 0, 0, 0, 1, 0};

/**
 * Program the PIT channel 2 frequency divisor
 * 
 * Frequency = 1193182 Hz / divisor
 * 
 * @param divisor - 16-bit frequency divisor
 */
static void pit_set_frequency(uint16_t divisor)
{
	uint8_t al;
	
	/* Disable speaker output while changing settings */
	al = inp(0x61);
	al &= 0xfc;  /* Clear bits 0 and 1 (speaker gate and output) */
	outp(0x61, al);
	
	/* Program PIT mode/command register
	 * 0xb6 = 0b10110110
	 *   10 = channel 2 (connected to PC speaker)
	 *   11 = access mode: lobyte/hibyte
	 *   011 = mode 3 (square wave generator)
	 *   0 = 16-bit binary
	 */
	outp(0x43, 0xb6);
	
	/* Write 16-bit frequency divisor to PIT channel 2 */
	outp(0x42, (uint8_t)(divisor & 0xff));      /* Low byte */
	outp(0x42, (uint8_t)((divisor >> 8) & 0xff)); /* High byte */
}

/**
 * Enable speaker output
 */
static void speaker_enable(void)
{
	uint8_t al = inp(0x61);
	al |= 0x03;  /* Set bits 0 and 1 to enable speaker */
	outp(0x61, al);
}

/**
 * Disable speaker output
 */
static void speaker_disable(void)
{
	uint8_t al = inp(0x61);
	al &= 0xfc;  /* Clear bits 0 and 1 to disable speaker */
	outp(0x61, al);
}

/**
 * Play a sound through the PC speaker
 * 
 * Sets up the sound data and prepares playback.
 * Actual note advancement happens in sound_advance_tick(),
 * which must be called once per game tick.
 * 
 * @param sound_data - pointer to sound data array (frequency/duration pairs)
 * @param priority - sound priority (higher priority sounds can interrupt lower ones)
 */
void play_sound(uint16_t *sound_data, uint8_t priority)
{
	/* Check if new sound has higher or equal priority */
	if (sound_state.is_playing && priority < sound_state.priority) {
		return;  /* Current sound has higher priority */
	}
	
	sound_state.data = sound_data;
	sound_state.offset = 0;
	sound_state.priority = priority;
	sound_state.is_playing = 1;
	sound_state.note_counter = 0;  /* Force immediate note load */
}

/**
 * Stop the current sound
 * 
 * Silences the speaker and clears playback state.
 */
void stop_sound(void)
{
	sound_state.is_playing = 0;
	sound_state.priority = 0;
	speaker_disable();
}

/**
 * Unmute the PC speaker
 * 
 * Enables speaker output without changing what's playing.
 */
void unmute_sound(void)
{
	sound_state.is_enabled = 1;
	if (sound_state.is_playing) {
		speaker_enable();
	}
}

/**
 * Mute the PC speaker
 * 
 * Disables speaker output without stopping playback.
 */
void mute_sound(void)
{
	sound_state.is_enabled = 0;
	speaker_disable();
}

/**
 * Query whether a sound is currently playing
 * 
 * @return 1 if sound is playing, 0 otherwise
 */
uint8_t is_sound_playing(void)
{
	return sound_state.is_playing;
}

/**
 * Advance sound playback by one game tick
 * 
 * This function must be called once per game tick to advance note playback.
 * Call this from your main game loop, not from interrupt handlers.
 * 
 * Typical usage in game loop:
 *   while (game_running) {
 *       // ... game logic ...
 *       sound_advance_tick();
 *       // ... render ...
 *   }
 */
void sound_advance_tick(void)
{
	uint16_t frequency;
	uint16_t duration;
	
	if (!sound_state.is_playing) {
		return;
	}
	
	/* If current note is still playing, decrement and continue */
	if (sound_state.note_counter > 0) {
		sound_state.note_counter--;
		return;
	}
	
	/* Time to load the next note from sound data */
	frequency = sound_state.data[sound_state.offset];
	duration = sound_state.data[sound_state.offset + 1];
	sound_state.offset += 2;  /* Each note is 2 words: frequency and duration */
	
	/* Check for end-of-sound marker */
	if (frequency == SOUND_TERMINATOR) {
		sound_state.is_playing = 0;
		sound_state.priority = 0;
		speaker_disable();
		return;
	}
	
	/* Set note duration (minus 1 since we consume this tick) */
	sound_state.note_counter = duration - 1;
	
	/* Program PIT with new frequency */
	pit_set_frequency(frequency);
	
	/* Enable speaker output if sound is unmuted */
	if (sound_state.is_enabled) {
		speaker_enable();
	}
}
