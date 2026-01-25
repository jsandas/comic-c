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

/* HP management - called from actors.c when Comic takes damage */
void decrement_comic_hp(void);

#endif /* PLAYER_H */
