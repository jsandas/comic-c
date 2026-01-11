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
    .tt2_filename = "LAKE.TT2",
    .pt0_filename = "LAKE0.PT",
    .pt1_filename = "LAKE1.PT",
    .pt2_filename = "LAKE2.PT",
    
    /* Door tiles */
    .door_tile_ul = 16,
    .door_tile_ur = 17,
    .door_tile_ll = 16,
    .door_tile_lr = 17,
    
    /* Enemy sprite files */
    .shp = {
        {3, ENEMY_HORIZONTAL_DUPLICATED, ENEMY_ANIMATION_ALTERNATE, "fb.shp"},
        {3, ENEMY_HORIZONTAL_DUPLICATED, ENEMY_ANIMATION_ALTERNATE, "bug.shp"},
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

/* TODO: Add remaining 6 levels - SPACE, BASE, CAVE, SHED, CASTLE, COMP */
/* For now, create stub entries to allow compilation */

const level_t level_data_forest = {
    /* Filenames */
    .tt2_filename = "FOREST.TT2",
    .pt0_filename = "FOREST0.PT",
    .pt1_filename = "FOREST1.PT",
    .pt2_filename = "FOREST2.PT",
    
    /* Door tiles */
    .door_tile_ul = 48,
    .door_tile_ur = 49,
    .door_tile_ll = 48,
    .door_tile_lr = 49,
    
    /* Enemy sprite files */
    .shp = {
        {3, ENEMY_HORIZONTAL_SEPARATE, ENEMY_ANIMATION_ALTERNATE, "bird.shp"},
        {3, ENEMY_HORIZONTAL_SEPARATE, ENEMY_ANIMATION_ALTERNATE, "bird2.shp"},
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
    .tt2_filename = "SPACE.TT2",
    .pt0_filename = "SPACE0.PT",
    .pt1_filename = "SPACE1.PT",
    .pt2_filename = "SPACE2.PT",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* space0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* space1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* space2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
};

const level_t level_data_base = {
    .tt2_filename = "BASE.TT2",
    .pt0_filename = "BASE0.PT",
    .pt1_filename = "BASE1.PT",
    .pt2_filename = "BASE2.PT",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* base0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* base1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* base2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
};

const level_t level_data_cave = {
    .tt2_filename = "CAVE.TT2",
    .pt0_filename = "CAVE0.PT",
    .pt1_filename = "CAVE1.PT",
    .pt2_filename = "CAVE2.PT",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* cave0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* cave1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* cave2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
};

const level_t level_data_shed = {
    .tt2_filename = "SHED.TT2",
    .pt0_filename = "SHED0.PT",
    .pt1_filename = "SHED1.PT",
    .pt2_filename = "SHED2.PT",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* shed0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* shed1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* shed2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
};

const level_t level_data_castle = {
    .tt2_filename = "CASTLE.TT2",
    .pt0_filename = "CASTLE0.PT",
    .pt1_filename = "CASTLE1.PT",
    .pt2_filename = "CASTLE2.PT",
    .door_tile_ul = 48, .door_tile_ur = 49, .door_tile_ll = 48, .door_tile_lr = 49,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* castle0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* castle1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* castle2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
};

const level_t level_data_comp = {
    .tt2_filename = "COMP.TT2",
    .pt0_filename = "COMP0.PT",
    .pt1_filename = "COMP1.PT",
    .pt2_filename = "COMP2.PT",
    .door_tile_ul = 32, .door_tile_ur = 33, .door_tile_ll = 32, .door_tile_lr = 33,
    .shp = {
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""},
        {SHP_UNUSED, 0, 0, ""}
    },
    .stages = {
        /* comp0 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* comp1 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        },
        /* comp2 - stub */
        {
            .item_type = 0,
            .item_y = 0,
            .item_x = 0,
            .exit_l = 0,
            .exit_r = 0,
            .doors = {
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0},
                {DOOR_UNUSED, DOOR_UNUSED, 0, 0}
            },
            .enemies = {
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED},
                {0, ENEMY_BEHAVIOR_UNUSED}
            }
        }
    }
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
