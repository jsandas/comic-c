/**
 * level_data.h - Level data structures and constants
 * 
 * Defines the data structures for levels, stages, enemies, doors, and items.
 * These match the structures from the original assembly code.
 */

#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <stdint.h>
#include "globals.h"

/* Maximum counts */
#define MAX_NUM_ENEMIES 4
#define MAX_NUM_DOORS 3

/* Enemy behavior constants */
#define ENEMY_BEHAVIOR_BOUNCE 1
#define ENEMY_BEHAVIOR_LEAP 2
#define ENEMY_BEHAVIOR_ROLL 3
#define ENEMY_BEHAVIOR_SEEK 4
#define ENEMY_BEHAVIOR_SHY 5
#define ENEMY_BEHAVIOR_UNUSED 0x7f
#define ENEMY_BEHAVIOR_FAST 0x80

/* Item type constants */
#define ITEM_CORKSCREW 0
#define ITEM_DOOR_KEY 1
#define ITEM_BOOTS 2
#define ITEM_LANTERN 3
#define ITEM_TELEPORT_WAND 4
#define ITEM_GEMS 5
#define ITEM_CROWN 6
#define ITEM_GOLD 7
#define ITEM_BLASTOLA_COLA 8
#define ITEM_SHIELD 14
#define ITEM_UNUSED 0xff

/* Exit and door unused markers */
#define EXIT_UNUSED 0xff
#define DOOR_UNUSED 0xff
#define SHP_UNUSED 0x00

/**
 * shp - Sprite (.SHP file) descriptor
 */
typedef struct {
    uint8_t num_distinct_frames;    /* Number of animation frames in file */
    uint8_t horizontal;              /* ENEMY_HORIZONTAL_DUPLICATED or ENEMY_HORIZONTAL_SEPARATE */
    uint8_t animation;               /* ENEMY_ANIMATION_LOOP or ENEMY_ANIMATION_ALTERNATE */
    char filename[14];               /* .SHP filename (null-padded to 14 bytes) */
} shp_t;

/**
 * door - Door descriptor (connects stages/levels)
 */
typedef struct {
    uint8_t y;                      /* Y coordinate in game units */
    uint8_t x;                      /* X coordinate in game units */
    uint8_t target_level;           /* Target level number */
    uint8_t target_stage;           /* Target stage number within level */
} door_t;

/**
 * enemy_record - Enemy spawn record (in stage data)
 */
typedef struct {
    uint8_t shp_index;              /* Index into level.shp array (0-3) */
    uint8_t behavior;               /* ENEMY_BEHAVIOR_* constant */
} enemy_record_t;

/**
 * stage - Single stage (128×10 map) descriptor
 */
typedef struct {
    uint8_t item_type;              /* ITEM_* constant */
    uint8_t item_y;                 /* Item Y coordinate in tiles */
    uint8_t item_x;                 /* Item X coordinate in tiles */
    uint8_t exit_l;                 /* Left exit: target stage number or EXIT_UNUSED */
    uint8_t exit_r;                 /* Right exit: target stage number or EXIT_UNUSED */
    door_t doors[MAX_NUM_DOORS];    /* Door array */
    enemy_record_t enemies[MAX_NUM_ENEMIES]; /* Enemy spawn records */
} stage_t;

/**
 * level - Complete level (tileset + 3 stages) descriptor
 */
typedef struct {
    char tt2_filename[14];          /* Tileset graphics filename */
    char pt0_filename[14];          /* Stage 0 map filename */
    char pt1_filename[14];          /* Stage 1 map filename */
    char pt2_filename[14];          /* Stage 2 map filename */
    uint8_t door_tile_ul;           /* Door tile: upper-left */
    uint8_t door_tile_ur;           /* Door tile: upper-right */
    uint8_t door_tile_ll;           /* Door tile: lower-left */
    uint8_t door_tile_lr;           /* Door tile: lower-right */
    shp_t shp[4];                   /* Enemy sprite descriptors */
    stage_t stages[3];              /* Three stages per level */
} level_t;

/* Array of pointers to level data (8 levels) */
extern const level_t* const level_data_pointers[8];

/* Individual level data structures */
extern const level_t level_data_lake;
extern const level_t level_data_forest;
extern const level_t level_data_space;
extern const level_t level_data_base;
extern const level_t level_data_cave;
extern const level_t level_data_shed;
extern const level_t level_data_castle;
extern const level_t level_data_comp;

#endif /* LEVEL_DATA_H */
