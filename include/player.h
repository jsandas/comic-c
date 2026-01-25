/*
 * player.h - Player character state and health management
 * 
 * Functions for managing Comic's HP, damage, and related player state.
 * These are gameplay-logic functions that modify player state and may
 * have side effects (sound, rendering), but are fundamentally about
 * player health rather than pure graphics rendering.
 */

#ifndef PLAYER_H
#define PLAYER_H

/* HP meter management */
void decrement_comic_hp(void);
void increment_comic_hp(void);
void render_comic_hp_meter(void);

#endif /* PLAYER_H */
