/*
 * globals.h - Assembly global variable declarations for C code
 * 
 * This header declares all assembly global variables that C code needs to access.
 * Uses #pragma aux "*" to prevent Open Watcom from adding underscore prefix.
 * 
 * The assembly must export these symbols with `global` directive.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

/* Prevent Open Watcom from adding underscore prefix to names */
#pragma aux game_tick_flag "*"
#pragma aux comic_x "*"
#pragma aux comic_y "*"
#pragma aux comic_x_checkpoint "*"
#pragma aux comic_y_checkpoint "*"
#pragma aux comic_facing "*"
#pragma aux comic_animation "*"
#pragma aux comic_is_falling_or_jumping "*"
#pragma aux comic_x_momentum "*"
#pragma aux comic_y_vel "*"
#pragma aux comic_jump_counter "*"
#pragma aux comic_hp "*"
#pragma aux comic_num_lives "*"
#pragma aux comic_firepower "*"
#pragma aux fireball_meter "*"
#pragma aux comic_has_corkscrew "*"
#pragma aux comic_has_door_key "*"
#pragma aux comic_has_boots "*"
#pragma aux comic_has_lantern "*"
#pragma aux comic_has_teleport_wand "*"
#pragma aux comic_is_teleporting "*"
#pragma aux camera_x "*"
#pragma aux current_level_number "*"
#pragma aux current_stage_number "*"
#pragma aux tileset_last_passable "*"
#pragma aux win_counter "*"
#pragma aux score "*"
#pragma aux key_state_esc "*"
#pragma aux key_state_f1 "*"
#pragma aux key_state_f2 "*"
#pragma aux key_state_open "*"
#pragma aux key_state_jump "*"
#pragma aux key_state_teleport "*"
#pragma aux key_state_left "*"
#pragma aux key_state_right "*"
#pragma aux key_state_fire "*"
#pragma aux tileset_graphics "*"
#pragma aux current_tiles_ptr "*"

/* Type definitions for game constants */
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef signed short int16;

/* Game constants */
#define LEVEL_NUMBER_LAKE       0
#define LEVEL_NUMBER_FOREST     1
#define LEVEL_NUMBER_SPACE      2
#define LEVEL_NUMBER_BASE       3
#define LEVEL_NUMBER_CAVE       4
#define LEVEL_NUMBER_SHED       5
#define LEVEL_NUMBER_CASTLE     6
#define LEVEL_NUMBER_COMP       7

#define MAP_WIDTH_TILES         128
#define MAP_HEIGHT_TILES        10
#define MAP_WIDTH               (MAP_WIDTH_TILES * 2)  /* in game units */
#define MAP_HEIGHT              (MAP_HEIGHT_TILES * 2) /* in game units */

#define PLAYFIELD_WIDTH         24  /* in game units */
#define PLAYFIELD_HEIGHT        MAP_HEIGHT

/* Video memory buffer offset for rendered map */
#define RENDERED_MAP_BUFFER     0x4000

#define MAX_HP                  6
#define MAX_FIREBALL_METER      12
#define MAX_NUM_LIVES           5
#define MAX_NUM_ENEMIES         4
#define MAX_NUM_FIREBALLS       5

#define COMIC_FACING_RIGHT      0
#define COMIC_FACING_LEFT       5

#define COMIC_STANDING          0
#define COMIC_RUNNING_1         1
#define COMIC_RUNNING_2         2
#define COMIC_RUNNING_3         3
#define COMIC_JUMPING           4

#define COMIC_GRAVITY           5
#define COMIC_GRAVITY_SPACE     3
#define TERMINAL_VELOCITY       23

/* Exit codes for game_main() */
#define GAME_EXIT_QUIT          0
#define GAME_EXIT_WIN           1
#define GAME_EXIT_GAMEOVER      2

/* Loop iteration results */
#define LOOP_CONTINUE           0
#define LOOP_EXIT_QUIT          1
#define LOOP_EXIT_WIN           2
#define LOOP_EXIT_GAMEOVER      3

/* Game tick flag - set by interrupt handler */
extern volatile uint8_t game_tick_flag;

/* Player state */
extern uint8_t comic_x;
extern uint8_t comic_y;
extern uint8_t comic_x_checkpoint;
extern uint8_t comic_y_checkpoint;
extern uint8_t comic_facing;
extern uint8_t comic_animation;
extern uint8_t comic_is_falling_or_jumping;
extern int8_t comic_x_momentum;
extern int8_t comic_y_vel;
extern uint8_t comic_jump_counter;
extern uint8_t comic_hp;
extern uint8_t comic_num_lives;
extern uint8_t comic_firepower;
extern uint8_t fireball_meter;

/* Player inventory flags */
extern uint8_t comic_has_corkscrew;
extern uint8_t comic_has_door_key;
extern uint8_t comic_has_boots;
extern uint8_t comic_has_lantern;
extern uint8_t comic_has_teleport_wand;
extern uint8_t comic_is_teleporting;

/* Camera and level state */
extern uint16_t camera_x;
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern uint8_t tileset_last_passable;
extern uint16_t win_counter;
extern uint8_t score[3];          /* 3-digit base-100 score array */
extern uint8_t score_10000_counter; /* Tracks 10k-point carries (increments on each carry into ten-thousands digit); awards extra life every 5 carries (50k total points) */

/* Input state (updated by interrupt handler) */
extern uint8_t key_state_esc;
extern uint8_t key_state_f1;
extern uint8_t key_state_f2;
extern uint8_t key_state_open;
extern uint8_t key_state_jump;
extern uint8_t key_state_teleport;
extern uint8_t key_state_left;
extern uint8_t key_state_right;
extern uint8_t key_state_fire;

/* Graphics and map data */
extern uint8_t tileset_graphics[];
extern uint8_t *current_tiles_ptr;

#endif /* GLOBALS_H */
