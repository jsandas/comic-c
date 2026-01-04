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
extern const uint8_t __far sprite_life_icon_bright[160];
extern const uint8_t __far sprite_life_icon_dark[160];

/* R4 (Revision 4) sprites */
extern const uint8_t __far sprite_R4_comic_jumping_left_16x32m[320];
extern const uint8_t __far sprite_R4_comic_jumping_right_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_1_left_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_1_right_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_2_left_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_2_right_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_3_left_16x32m[320];
extern const uint8_t __far sprite_R4_comic_running_3_right_16x32m[320];
extern const uint8_t __far sprite_R4_comic_standing_left_16x32m[320];
extern const uint8_t __far sprite_R4_comic_standing_right_16x32m[320];
extern const uint8_t __far sprite_R4_game_over_128x48[3072];
extern const uint8_t __far sprite_R4_pause_128x48[3072];

/* Blastola Cola sprites (16x16 masked) */
extern const uint8_t __far sprite_blastola_cola_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_1_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_1_odd_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_2_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_2_odd_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_3_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_3_odd_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_4_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_4_odd_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_5_even_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_inventory_5_odd_16x16m[160];
extern const uint8_t __far sprite_blastola_cola_odd_16x16m[160];

/* Item sprites (16x16 masked) */
extern const uint8_t __far sprite_boots_even_16x16m[160];
extern const uint8_t __far sprite_boots_odd_16x16m[160];
extern const uint8_t __far sprite_corkscrew_even_16x16m[160];
extern const uint8_t __far sprite_corkscrew_odd_16x16m[160];
extern const uint8_t __far sprite_crown_even_16x16m[160];
extern const uint8_t __far sprite_crown_odd_16x16m[160];
extern const uint8_t __far sprite_door_key_even_16x16m[160];
extern const uint8_t __far sprite_door_key_odd_16x16m[160];
extern const uint8_t __far sprite_gems_even_16x16m[160];
extern const uint8_t __far sprite_gems_odd_16x16m[160];
extern const uint8_t __far sprite_gold_even_16x16m[160];
extern const uint8_t __far sprite_gold_odd_16x16m[160];
extern const uint8_t __far sprite_lantern_even_16x16m[160];
extern const uint8_t __far sprite_lantern_odd_16x16m[160];
extern const uint8_t __far sprite_shield_even_16x16m[160];
extern const uint8_t __far sprite_shield_odd_16x16m[160];
extern const uint8_t __far sprite_teleport_wand_even_16x16m[160];
extern const uint8_t __far sprite_teleport_wand_odd_16x16m[160];

/* Comic character sprites (16x32 masked) */
extern const uint8_t __far sprite_comic_death_0_16x32m[320];
extern const uint8_t __far sprite_comic_death_1_16x32m[320];
extern const uint8_t __far sprite_comic_death_2_16x32m[320];
extern const uint8_t __far sprite_comic_death_3_16x32m[320];
extern const uint8_t __far sprite_comic_death_4_16x32m[320];
extern const uint8_t __far sprite_comic_death_5_16x32m[320];
extern const uint8_t __far sprite_comic_death_6_16x32m[320];
extern const uint8_t __far sprite_comic_death_7_16x32m[320];
extern const uint8_t __far sprite_comic_jumping_left_16x32m[320];
extern const uint8_t __far sprite_comic_jumping_right_16x32m[320];
extern const uint8_t __far sprite_comic_running_1_left_16x32m[320];
extern const uint8_t __far sprite_comic_running_1_right_16x32m[320];
extern const uint8_t __far sprite_comic_running_2_left_16x32m[320];
extern const uint8_t __far sprite_comic_running_2_right_16x32m[320];
extern const uint8_t __far sprite_comic_running_3_left_16x32m[320];
extern const uint8_t __far sprite_comic_running_3_right_16x32m[320];
extern const uint8_t __far sprite_comic_standing_left_16x32m[320];
extern const uint8_t __far sprite_comic_standing_right_16x32m[320];

/* Materialize/Teleport effect sprites (16x32 masked) */
extern const uint8_t __far sprite_materialize_0_16x32m[320];
extern const uint8_t __far sprite_materialize_1_16x32m[320];
extern const uint8_t __far sprite_materialize_2_16x32m[320];
extern const uint8_t __far sprite_materialize_3_16x32m[320];
extern const uint8_t __far sprite_materialize_4_16x32m[320];
extern const uint8_t __far sprite_materialize_5_16x32m[320];
extern const uint8_t __far sprite_materialize_6_16x32m[320];
extern const uint8_t __far sprite_materialize_7_16x32m[320];
extern const uint8_t __far sprite_materialize_8_16x32m[320];
extern const uint8_t __far sprite_materialize_9_16x32m[320];
extern const uint8_t __far sprite_materialize_10_16x32m[320];
extern const uint8_t __far sprite_materialize_11_16x32m[320];
extern const uint8_t __far sprite_teleport_0_16x32m[320];
extern const uint8_t __far sprite_teleport_1_16x32m[320];
extern const uint8_t __far sprite_teleport_2_16x32m[320];

/* Effect sprites */
extern const uint8_t __far sprite_fireball_0_16x8m[80];
extern const uint8_t __far sprite_fireball_1_16x8m[80];
extern const uint8_t __far sprite_red_spark_0_16x16m[160];
extern const uint8_t __far sprite_red_spark_1_16x16m[160];
extern const uint8_t __far sprite_red_spark_2_16x16m[160];
extern const uint8_t __far sprite_white_spark_0_16x16m[160];
extern const uint8_t __far sprite_white_spark_1_16x16m[160];
extern const uint8_t __far sprite_white_spark_2_16x16m[160];

/* UI sprites */
extern const uint8_t __far sprite_meter_empty_8x16[64];
extern const uint8_t __far sprite_meter_full_8x16[64];
extern const uint8_t __far sprite_meter_half_8x16[64];
extern const uint8_t __far sprite_score_digit_0_8x16[64];
extern const uint8_t __far sprite_score_digit_1_8x16[64];
extern const uint8_t __far sprite_score_digit_2_8x16[64];
extern const uint8_t __far sprite_score_digit_3_8x16[64];
extern const uint8_t __far sprite_score_digit_4_8x16[64];
extern const uint8_t __far sprite_score_digit_5_8x16[64];
extern const uint8_t __far sprite_score_digit_6_8x16[64];
extern const uint8_t __far sprite_score_digit_7_8x16[64];
extern const uint8_t __far sprite_score_digit_8_8x16[64];
extern const uint8_t __far sprite_score_digit_9_8x16[64];

/* Large screen sprites */
extern const uint8_t __far sprite_game_over_128x48[3072];
extern const uint8_t __far sprite_pause_128x48[3072];
extern const uint8_t __far sprite_high_scores_320x200[32000];

#endif /* SPRITE_DATA_H */
