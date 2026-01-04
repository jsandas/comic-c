/**
 * level_data.c - Level data definitions
 * 
 * Contains the static data for all 8 levels in the game.
 * This data was originally in R3_levels.asm
 * 
 * TODO: Complete migration of all level data from R3_levels.asm
 * Currently only LAKE and FOREST levels are fully defined as examples.
 */

#include "level_data.h"

/* LAKE Level Data */
const level_t level_data_lake = {
    /* Filenames */
    .tt2_filename = "lake.tt2     ",
    .pt0_filename = "lake0.pt     ",
    .pt1_filename = "lake1.pt     ",
    .pt2_filename = "lake2.pt     ",
    
    /* Door tiles */
    .door_tile_ul = 16,
    .door_tile_ur = 17,
    .door_tile_ll = 16,
    .door_tile_lr = 17,
    
    /* Enemy sprite files */
    .shp = {
        {3, ENEMY_HORIZONTAL_DUPLICATED, ENEMY_ANIMATION_ALTERNATE, "fb.shp       "},
        {3, ENEMY_HORIZONTAL_DUPLICATED, ENEMY_ANIMATION_ALTERNATE, "bug.shp      "},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    
    /* Three stages */
    .stages = {
        /* lake0 */
        {
            .item_type = ITEM_BLASTOLA_COLA,
            .item_y = 12,
            .item_x = 112,
            .exit_l = 1,
            .exit_r = EXIT_UNUSED,
            .doors = {
                {10, 118, 1, 2},
                {14, 248, 5, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {1, ENEMY_BEHAVIOR_LEAP},
                {1, ENEMY_BEHAVIOR_LEAP}
            }
        },
        /* lake1 */
        {
            .item_type = ITEM_SHIELD,
            .item_y = 10,
            .item_x = 178,
            .exit_l = 2,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {1, ENEMY_BEHAVIOR_LEAP},
                {1, ENEMY_BEHAVIOR_LEAP}
            }
        },
        /* lake2 */
        {
            .item_type = ITEM_BLASTOLA_COLA,
            .item_y = 4,
            .item_x = 124,
            .exit_l = EXIT_UNUSED,
            .exit_r = 1,
            .doors = {
                {14, 10, 4, 0},
                {6, 110, 2, 0},
                {8, 74, 3, 1}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {1, ENEMY_BEHAVIOR_LEAP},
                {1, ENEMY_BEHAVIOR_LEAP}
            }
        }
    }
};

/* TODO: Add remaining 7 levels - FOREST, SPACE, BASE, CAVE, SHED, CASTLE, COMP */
/* For now, create stub entries to allow compilation */

const level_t level_data_forest = {
    /* Filenames */
    .tt2_filename = "forest.tt2   ",
    .pt0_filename = "forest0.pt   ",
    .pt1_filename = "forest1.pt   ",
    .pt2_filename = "forest2.pt   ",
    
    /* Door tiles */
    .door_tile_ul = 48,
    .door_tile_ur = 49,
    .door_tile_ll = 48,
    .door_tile_lr = 49,
    
    /* Enemy sprite files */
    .shp = {
        {3, ENEMY_HORIZONTAL_SEPARATE, ENEMY_ANIMATION_ALTERNATE, "bird.shp     "},
        {3, ENEMY_HORIZONTAL_SEPARATE, ENEMY_ANIMATION_ALTERNATE, "bird2.shp    "},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    
    /* Three stages */
    .stages = {
        /* forest0 */
        {
            .item_type = ITEM_BLASTOLA_COLA,
            .item_y = 14,
            .item_x = 12,
            .exit_l = EXIT_UNUSED,
            .exit_r = 1,
            .doors = {
                {12, 12, 6, 0},  /* Door to castle stage 0 */
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* forest1 */
        {
            .item_type = ITEM_SHIELD,
            .item_y = 10,
            .item_x = 118,
            .exit_l = 0,
            .exit_r = 2,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {1, ENEMY_BEHAVIOR_SHY}
            }
        },
        /* forest2 */
        {
            .item_type = ITEM_DOOR_KEY,
            .item_y = 2,
            .item_x = 160,
            .exit_l = 1,
            .exit_r = EXIT_UNUSED,
            .doors = {
                {12, 238, 0, 0},  /* Door to lake stage 0 */
                {14, 160, 7, 2},  /* Door to comp stage 2 */
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {1, ENEMY_BEHAVIOR_SHY},
                {0, ENEMY_BEHAVIOR_BOUNCE},
                {1, ENEMY_BEHAVIOR_SHY}
            }
        }
    }
};

const level_t level_data_space = {
    .tt2_filename = "space.tt2    ",
    .pt0_filename = "space0.pt    ",
    .pt1_filename = "space1.pt    ",
    .pt2_filename = "space2.pt    ",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    /* TODO: Complete space level data */
};

const level_t level_data_base = {
    .tt2_filename = "base.tt2     ",
    .pt0_filename = "base0.pt     ",
    .pt1_filename = "base1.pt     ",
    .pt2_filename = "base2.pt     ",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    /* TODO: Complete base level data */
};

const level_t level_data_cave = {
    .tt2_filename = "cave.tt2     ",
    .pt0_filename = "cave0.pt     ",
    .pt1_filename = "cave1.pt     ",
    .pt2_filename = "cave2.pt     ",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    /* TODO: Complete cave level data */
};

const level_t level_data_shed = {
    .tt2_filename = "shed.tt2     ",
    .pt0_filename = "shed0.pt     ",
    .pt1_filename = "shed1.pt     ",
    .pt2_filename = "shed2.pt     ",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    /* TODO: Complete shed level data */
};

const level_t level_data_castle = {
    .tt2_filename = "castle.tt2   ",
    .pt0_filename = "castle0.pt   ",
    .pt1_filename = "castle1.pt   ",
    .pt2_filename = "castle2.pt   ",
    .door_tile_ul = 48, .door_tile_ur = 49, .door_tile_ll = 48, .door_tile_lr = 49,
    /* TODO: Complete castle level data */
};

const level_t level_data_comp = {
    .tt2_filename = "comp.tt2     ",
    .pt0_filename = "comp0.pt     ",
    .pt1_filename = "comp1.pt     ",
    .pt2_filename = "comp2.pt     ",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    /* TODO: Complete comp level data */
};

/* Level data pointer array */
const level_t* const level_data_pointers[8] = {
    &level_data_lake,
    &level_data_forest,
    &level_data_space,
    &level_data_base,
    &level_data_cave,
    &level_data_shed,
    &level_data_castle,
    &level_data_comp
};
