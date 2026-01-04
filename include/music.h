/**
 * Title Sequence Music Module Header
 * 
 * Provides music playback for the title sequence through the PC speaker.
 * Implementation uses PC speaker sound driver (sound.c/sound.h).
 * 
 * Sound data (SOUND_TITLE melody) is defined in sound_data.asm and
 * played via INT 3 and INT 8 interrupt handlers (assembly).
 */

#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

/**
 * Play the title sequence music
 * 
 * Plays the SOUND_TITLE melody through the PC speaker.
 * The melody is a period-appropriate tune from the original game.
 */
void play_title_music(void);

/**
 * Stop music playback
 * 
 * Silences the PC speaker and stops current music.
 */
void stop_music(void);

/**
 * Query whether music is currently playing
 * 
 * @return 1 if music is playing, 0 otherwise
 */
uint8_t is_music_playing(void);

#endif /* MUSIC_H */
