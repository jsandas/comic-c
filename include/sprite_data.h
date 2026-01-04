/*
 * sprite_data.h - Embedded sprite graphics data declarations
 * 
 * Declares binary sprite data that is compiled into the executable.
 * Each sprite is in EGA planar format with an 8x8 or 16x16 mask.
 */

#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

#include <stdint.h>

/* Life icon sprites (16x16 EGA masked sprites, 160 bytes each) */
extern const uint8_t sprite_life_icon_bright[160];
extern const uint8_t sprite_life_icon_dark[160];

#endif /* SPRITE_DATA_H */
