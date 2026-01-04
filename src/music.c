/**
 * Title Sequence Music Module
 * 
 * Provides music playback for the title sequence through the PC speaker.
 * Implementation: Uses PC speaker sound driver (sound.c/sound.h) with direct
 * hardware access. Sound advances via sound_advance_tick() called from game loop.
 */

#include "music.h"
#include "sound.h"
#include "sound_data.h"

/**
 * Play the title sequence music
 * 
 * Plays the SOUND_TITLE melody through the PC speaker.
 * The sound data contains (frequency, duration) pairs which are
 * advanced each game tick by sound_advance_tick() in the game loop.
 * 
 * Priority: 4 (same as assembly implementation)
 */
void play_title_music(void)
{
	play_sound(SOUND_TITLE, 4);
}

/**
 * Stop music playback
 * 
 * Stops the PC speaker and clears the sound playback state.
 */
void stop_music(void)
{
	stop_sound();
}

/**
 * Query whether music is currently playing
 * 
 * Returns the PC speaker sound playback status.
 */
uint8_t is_music_playing(void)
{
	return is_sound_playing();
}
