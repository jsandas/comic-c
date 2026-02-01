/*
 * actors.c - Actor system implementation (enemies, fireballs, items)
 * 
 * This file implements the actor management systems for Captain Comic:
 *   - Fireball spawning, movement, and collision
 *   - Item animation and collection
 *   - Enemy AI, spawning, and collision
 */

#include <dos.h>
#include <stdint.h>
#include <stdlib.h>
#include "actors.h"
#include "level_data.h"
#include "globals.h"
#include "graphics.h"
#include "physics.h"
#include "player.h"
#include "file_loaders.h"
#include "sprite_data.h"
#include "sound.h"
#include "sound_data.h"

/* ===== External Game State Variables ===== */
/*
 * These variables are defined in game_main.c and accessed here.
 * They represent the current game state during gameplay.
 */

/* Player state */
extern uint8_t comic_x;                    /* Comic X position (game units) */
extern uint8_t comic_y;                    /* Comic Y position (game units) */
extern uint8_t comic_facing;               /* COMIC_FACING_RIGHT or COMIC_FACING_LEFT */
extern uint8_t comic_firepower;            /* Number of active fireball slots (0-5) */
extern uint8_t comic_has_corkscrew;        /* 1 if Corkscrew item collected, 0 otherwise */
extern uint8_t comic_hp;                   /* Current hit points (0-10) */
extern uint8_t comic_has_shield;           /* 1 if Shield item collected, 0 otherwise */
extern uint8_t comic_has_door_key;         /* 1 if door key collected, 0 otherwise */
extern uint8_t comic_has_teleport_wand;    /* 1 if teleport wand collected, 0 otherwise */
extern uint8_t comic_has_lantern;          /* 1 if Lantern item collected, 0 otherwise */
extern uint8_t comic_jump_power;           /* Jump power level (4 default, 5 with Boots) */
extern uint8_t comic_hp_pending_increase;  /* Units of HP to award at 1 per tick */
extern uint8_t comic_num_lives;            /* Current number of lives */

/* Camera state */
extern uint16_t camera_x;                  /* Camera X position (game units) */

/* Rendering state */
extern uint16_t offscreen_video_buffer_ptr; /* Current offscreen buffer offset */

/* Game state */
extern uint8_t score_bytes[3];             /* Current score (3-byte little-endian value) */
/* Score macros (score_get_value and score_set_value) are defined in globals.h */
extern const level_t *current_level_ptr; /* Pointer to current level data */
extern uint8_t current_stage_number;       /* Current stage (0-based) */
extern uint8_t current_level_number;       /* Current level (0-based, 0=LAKE, 1=FOREST, etc.) */
extern uint8_t *current_tiles_ptr;         /* Pointer to current stage's tile map */
extern uint8_t tileset_last_passable;      /* Last passable tile ID (tiles > this are solid) */
/* Item tracking */
extern uint8_t items_collected[8][16];     /* Bitmap: items_collected[level][stage] */
extern uint8_t item_animation_counter;     /* 0 or 1, toggles for item animation */
extern uint8_t comic_num_treasures;        /* Number of treasures collected (0-3) */
extern uint8_t win_counter;                /* Win countdown counter; set to 200 when treasures == 3 */

/* Respawn timing */
extern uint8_t enemy_respawn_counter_cycle; /* Cycles: 20→40→60→80→100→20 */

/* Forward declarations for external functions */
extern void comic_dies(void);               /* Game over sequence from physics.c */
extern void comic_death_animation(void);    /* Death animation from game_main.c */
extern void award_points(uint16_t points);  /* Award points from game_main.c */
extern void award_extra_life(void);         /* Award extra life from game_main.c */

/* Death animation state */
extern uint8_t inhibit_death_by_enemy_collision; /* Set during death animation to prevent re-entry */
extern uint8_t comic_death_animation_finished;   /* Set after animation finishes to skip partial sprite in comic_dies */

/* ===== Actor Arrays ===== */
enemy_t enemies[MAX_NUM_ENEMIES];
fireball_t fireballs[MAX_NUM_FIREBALLS];
uint8_t enemy_shp_index[MAX_NUM_ENEMIES];

/* ===== Forward Declarations ===== */
static void comic_takes_damage(void);
static uint8_t is_tile_solid(uint8_t tile_id);
static uint8_t get_tile_at(uint8_t x, uint8_t y);
static const uint8_t *get_enemy_frame(uint8_t shp_index, uint8_t anim_index, uint8_t facing, uint16_t *out_frame_size);

/* ===== Helper Functions ===== */

/*
 * comic_takes_damage - Reduce Comic's HP and start shield/death animation
 * 
 * If Comic has shield, remove shield instead of losing HP.
 * If HP is already 0 when taking damage, trigger death animation sequence.
 * If HP > 0, decrement HP (sound plays in decrement_comic_hp).
 */
static void comic_takes_damage(void)
{
    if (comic_has_shield) {
        comic_has_shield = 0;
        play_sound(SOUND_DAMAGE, 2);
    } else if (comic_hp == 0) {
        /* HP is already 0 - Comic dies from this hit.
         * Only play SOUND_DEATH via comic_death_animation, NOT SOUND_DAMAGE.
         * This matches assembly behavior: no decrement_comic_hp() call here. */
        if (inhibit_death_by_enemy_collision == 0) {
            inhibit_death_by_enemy_collision = 1;
            comic_death_animation();
            comic_death_animation_finished = 1;
            comic_dies();
            inhibit_death_by_enemy_collision = 0;
        }
    } else {
        /* HP > 0: decrement HP (which plays SOUND_DAMAGE).
         * Death only occurs when taking damage WHILE HP is already 0. */
        decrement_comic_hp();
    }
}

/*
 * is_tile_solid - Check if a tile ID represents a solid tile
 * 
 * Parameters:
 *   tile_id - Tile ID to check
 * 
 * Returns:
 *   1 if solid, 0 if passable
 */
static uint8_t is_tile_solid(uint8_t tile_id)
{
    /* Match physics behavior: tiles greater than tileset_last_passable are solid */
    return (tile_id > tileset_last_passable) ? 1 : 0;
}

/*
 * get_tile_at - Get tile ID at coordinates
 * 
 * Parameters:
 *   x - X position in game units (0-255)
 *   y - Y position in game units (0-19)
 * 
 * Returns:
 *   Tile ID, or 0 if out of bounds
 */
static uint8_t get_tile_at(uint8_t x, uint8_t y)
{
    uint16_t tile_x, tile_y, offset;
    
    /* Get current tiles pointer (set by load_new_stage) */
    if (current_tiles_ptr == NULL) {
        return 0;
    }
    
    /* Convert game units to tile coordinates (2 game units = 1 tile) */
    tile_x = x / 2;
    tile_y = y / 2;
    
    /* Bounds check */
    if (tile_x >= MAP_WIDTH_TILES || tile_y >= MAP_HEIGHT_TILES) {
        return 0;
    }
    
    /* Calculate offset: y * MAP_WIDTH_TILES + x */
    offset = tile_y * MAP_WIDTH_TILES + tile_x;
    
    /* Read tile from map data */
    return current_tiles_ptr[offset];
}

/*
 * check_vertical_enemy_map_collision - Check for solid tiles when moving vertically
 * 
 * Enemies are 2 units wide, so when the X coordinate is odd (in the middle of a tile),
 * the enemy spans two horizontal tiles. This function checks both tiles when needed.
 * 
 * Parameters:
 *   x - X position in game units (0-255)
 *   y - Y position in game units (0-19)
 * 
 * Returns:
 *   true (1) if collision detected with solid tile, false (0) otherwise
 */
static uint8_t check_vertical_enemy_map_collision(uint8_t x, uint8_t y)
{
    uint8_t tile_x, tile_y;
    uint16_t tile_offset;
    uint8_t tile_id;
    
    if (current_tiles_ptr == NULL) {
        return 0;
    }
    
    /* Convert game units to tile coordinates (divide by 2) */
    tile_x = x >> 1;
    tile_y = y >> 1;
    
    /* Calculate tile offset: y/2 * MAP_WIDTH_TILES + x/2 */
    tile_offset = tile_y * MAP_WIDTH_TILES + tile_x;
    
    /* Check primary tile at (x, y) */
    tile_id = current_tiles_ptr[tile_offset];
    if (tile_id > tileset_last_passable) {
        return 1;  /* Solid collision */
    }
    
    /* If X coordinate is odd, enemy spans two tiles horizontally */
    if (x & 1) {
        /* Check tile to the right: (x+1, y) */
        if (tile_x + 1 < MAP_WIDTH_TILES) {
            tile_id = current_tiles_ptr[tile_offset + 1];
            if (tile_id > tileset_last_passable) {
                return 1;  /* Solid collision */
            }
        }
    }
    
    return 0;  /* No collision */
}

/*
 * check_horizontal_enemy_map_collision - Check for solid tiles when moving horizontally
 * 
 * Enemies are 2 units tall, so when the Y coordinate is odd (in the middle of a tile),
 * the enemy spans two vertical tiles. This function checks both tiles when needed.
 * 
 * Parameters:
 *   x - X position in game units (0-255)
 *   y - Y position in game units (0-19)
 * 
 * Returns:
 *   true (1) if collision detected with solid tile, false (0) otherwise
 */
static uint8_t check_horizontal_enemy_map_collision(uint8_t x, uint8_t y)
{
    uint8_t tile_x, tile_y;
    uint16_t tile_offset;
    uint8_t tile_id;
    
    if (current_tiles_ptr == NULL) {
        return 0;
    }
    
    /* Convert game units to tile coordinates (divide by 2) */
    tile_x = x >> 1;
    tile_y = y >> 1;
    
    /* Calculate tile offset: y/2 * MAP_WIDTH_TILES + x/2 */
    tile_offset = tile_y * MAP_WIDTH_TILES + tile_x;
    
    /* Check primary tile at (x, y) */
    tile_id = current_tiles_ptr[tile_offset];
    if (tile_id > tileset_last_passable) {
        return 1;  /* Solid collision */
    }
    
    /* If Y coordinate is odd, enemy spans two tiles vertically */
    if (y & 1) {
        /* Check tile below: (x, y+1) */
        if (tile_y + 1 < MAP_HEIGHT_TILES) {
            tile_id = current_tiles_ptr[tile_offset + MAP_WIDTH_TILES];
            if (tile_id > tileset_last_passable) {
                return 1;  /* Solid collision */
            }
        }
    }
    
    return 0;  /* No collision */
}

/*
 * get_enemy_frame - Resolve enemy sprite frame pointer
 * 
 * Uses SHP metadata to map animation index and facing into a frame index.
 * Returns NULL if the SHP is not loaded or invalid.
 */
static const uint8_t *get_enemy_frame(uint8_t shp_index, uint8_t anim_index, uint8_t facing, uint16_t *out_frame_size)
{
    uint8_t distinct = shp_get_num_distinct_frames(shp_index);
    uint8_t anim_type = shp_get_animation(shp_index);
    uint8_t horizontal = shp_get_horizontal(shp_index);
    uint8_t seq_len;
    uint8_t frame_in_seq;
    uint8_t base_index;

    if (out_frame_size) {
        *out_frame_size = shp_get_frame_size(shp_index);
    }

    if (distinct == 0) {
        return NULL;
    }

    seq_len = shp_get_animation_length(shp_index);
    if (seq_len == 0) {
        return NULL;
    }

    anim_index = (uint8_t)(anim_index % seq_len);

    if (anim_type == ENEMY_ANIMATION_ALTERNATE && distinct > 1) {
        if (anim_index < distinct) {
            frame_in_seq = anim_index;
        } else {
            /* Mirror back: e.g., 0,1,2,1 for distinct=3 */
            frame_in_seq = (uint8_t)((distinct - 2) - (anim_index - distinct));
        }
    } else {
        frame_in_seq = (uint8_t)(anim_index % distinct);
    }

    if (horizontal == ENEMY_HORIZONTAL_SEPARATE && facing == COMIC_FACING_RIGHT) {
        base_index = distinct;
    } else {
        base_index = 0;
    }

    return shp_get_frame(shp_index, (uint8_t)(base_index + frame_in_seq));
}

/* ===== Fireball System ===== */

/*
 * try_to_fire - Attempt to spawn a fireball
 */
void try_to_fire(void)
{
    int i;
    
    /* Check if Comic has firepower */
    if (comic_firepower == 0) {
        return;
    }
    
    /* Find an open fireball slot */
    for (i = 0; i < comic_firepower && i < MAX_NUM_FIREBALLS; i++) {
        if (fireballs[i].x == FIREBALL_DEAD && fireballs[i].y == FIREBALL_DEAD) {
            /* Found empty slot - spawn fireball */
            fireballs[i].y = comic_y + 1;
            fireballs[i].x = comic_x;
            
            /* Set velocity based on Comic's facing direction */
            if (comic_facing == COMIC_FACING_RIGHT) {
                fireballs[i].vel = FIREBALL_VELOCITY;  /* +2 */
            } else {
                fireballs[i].vel = -FIREBALL_VELOCITY; /* -2 */
            }
            
            fireballs[i].corkscrew_phase = 2;
            fireballs[i].animation = 0;
            fireballs[i].num_animation_frames = 2;
            
            play_sound(SOUND_FIRE, 0);
            
            return; /* Only spawn one fireball per call */
        }
    }
}

/*
 * handle_fireballs - Update all active fireballs
 */
void handle_fireballs(void)
{
    int i, j;
    int16_t rel_x;
    
    /* Skip if Comic has no firepower */
    if (comic_firepower == 0) {
        return;
    }
    
    /* Update each active fireball */
    for (i = 0; i < comic_firepower && i < MAX_NUM_FIREBALLS; i++) {
        /* Skip inactive fireballs */
        if (fireballs[i].x == FIREBALL_DEAD || fireballs[i].y == FIREBALL_DEAD) {
            continue;
        }
        
        /* Apply horizontal velocity */
        fireballs[i].x += fireballs[i].vel;
        
        /* Apply corkscrew motion if Comic has Corkscrew item */
        if (comic_has_corkscrew) {
            if (fireballs[i].corkscrew_phase == 2) {
                /* Phase 2→1: move down */
                fireballs[i].y++;
                fireballs[i].corkscrew_phase = 1;
            } else if (fireballs[i].corkscrew_phase == 1) {
                /* Phase 1→0→2: move up */
                fireballs[i].y--;
                fireballs[i].corkscrew_phase = 2;
            }
        }
        
        /* Advance animation frame */
        fireballs[i].animation++;
        if (fireballs[i].animation >= fireballs[i].num_animation_frames) {
            fireballs[i].animation = 0;
        }
        
        /* Check despawn conditions (off-screen) */
        if (fireballs[i].x < camera_x) {
            /* Off left edge */
            fireballs[i].x = FIREBALL_DEAD;
            fireballs[i].y = FIREBALL_DEAD;
            continue;
        }
        
        rel_x = (int16_t)((int)fireballs[i].x - (int)camera_x);
        if (rel_x > (PLAYFIELD_WIDTH - 2)) {
            /* Off right edge */
            fireballs[i].x = FIREBALL_DEAD;
            fireballs[i].y = FIREBALL_DEAD;
            continue;
        }
        
        /* Check collision with all enemies */
        for (j = 0; j < MAX_NUM_ENEMIES; j++) {
            int8_t y_diff, x_diff;
            
            /* Skip despawned or dying enemies */
            if (enemies[j].state != ENEMY_STATE_SPAWNED) {
                continue;
            }
            
            /* Check vertical overlap: 0 <= (fireball.y - enemy.y) <= 1 */
            y_diff = (int8_t)(fireballs[i].y - enemies[j].y);
            if (y_diff < 0 || y_diff > 1) {
                continue;
            }
            
            /* Check horizontal overlap: abs(fireball.x - enemy.x) <= 1 */
            x_diff = (int8_t)(fireballs[i].x - enemies[j].x);
            if (x_diff < -1 || x_diff > 1) {
                continue;
            }
            
            /* Collision detected! */
            enemies[j].state = ENEMY_STATE_WHITE_SPARK; /* Start death animation */
            fireballs[i].x = FIREBALL_DEAD;
            fireballs[i].y = FIREBALL_DEAD;
            award_points(3);  /* 3 * 100 = 300 points */
            play_sound(SOUND_HIT_ENEMY, 1);
            
            break; /* Fireball consumed, check next fireball */
        }
        
        /* Render fireball sprite if still active and in playfield */
        if (fireballs[i].x != FIREBALL_DEAD && fireballs[i].y != FIREBALL_DEAD) {
            int16_t pixel_x, pixel_y;
            const uint8_t *sprite_ptr;
            
            /* Calculate screen position relative to camera */
            rel_x = (int16_t)((int)fireballs[i].x - (int)camera_x);
            
            /* Check if in visible playfield (0-23 game units) */
            if (rel_x >= 0 && rel_x < PLAYFIELD_WIDTH) {
                /* Convert to pixel coordinates (8 pixels per game unit) + playfield offset */
                pixel_x = (rel_x * 8) + 8;
                pixel_y = (fireballs[i].y * 8) + 8;

                sprite_ptr = (fireballs[i].animation == 0)
                    ? sprite_fireball_0_16x8m
                    : sprite_fireball_1_16x8m;

                blit_sprite_16x8_masked((uint16_t)pixel_x, (uint16_t)pixel_y, sprite_ptr);
            }
        }
    }
}

/* ===== Item System ===== */

/*
 * handle_item - Update and render the current stage's item
 */
void handle_item(void)
{
    const stage_t *stage;
    uint8_t item_type, item_x, item_y;
    int16_t rel_x, x_diff, y_diff;
    uint8_t level_index, stage_index;
    
    if (current_level_ptr == NULL) {
        return;
    }
    
    /* Validate stage number is within bounds (0-2) */
    if (current_stage_number >= 3) {
        return;
    }
    
    /* Get current stage data */
    stage = &current_level_ptr->stages[current_stage_number];
    
    /* Check if stage has an item */
    item_type = stage->item_type;
    if (item_type == ITEM_UNUSED) {
        return; /* No item in this stage */
    }
    
    /* Check if already collected */
    level_index = current_level_number;
    stage_index = current_stage_number;
    
    if (items_collected[level_index][stage_index]) {
        return; /* Already collected */
    }
    
    /* Get item position */
    item_x = stage->item_x;
    item_y = stage->item_y;
    
    /* Check if item is visible in playfield */
    rel_x = (int16_t)((int)item_x - (int)camera_x);
    if (rel_x < 0 || rel_x > 22) {
        return; /* Off-screen */
    }
    
    /* Toggle animation counter */
    item_animation_counter = (item_animation_counter == 0) ? 1 : 0;
    
    /* Check collision with Comic */
    x_diff = (int16_t)((int)item_x - (int)comic_x);
    y_diff = (int16_t)((int)item_y - (int)comic_y);
    
    /* Horizontal: abs(item.x - comic_x) < 2 */
    if (x_diff >= -1 && x_diff <= 1) {
        /* Vertical: 0 <= (item.y - comic_y) < 4 */
        if (y_diff >= 0 && y_diff < 4) {
            /* Collision detected - collect item! */
            items_collected[level_index][stage_index] = 1;
            award_points(20);  /* 20 * 100 = 2000 points */
            play_sound(SOUND_COLLECT_ITEM, 3);
            
            /* Update game state based on item type */
            switch (item_type) {
                case ITEM_CORKSCREW:
                    comic_has_corkscrew = 1;
                    break;
                case ITEM_BLASTOLA_COLA:
                    if (comic_firepower < MAX_NUM_FIREBALLS) {
                        comic_firepower++;
                    }
                    break;
                case ITEM_BOOTS:
                    /* Grant increased jump power */
                    comic_jump_power = 5;
                    break;
                case ITEM_LANTERN:
                    /* Grant light in dark areas (Castle level) */
                    comic_has_lantern = 1;
                    break;
                case ITEM_SHIELD:
                    comic_has_shield = 1;
                    /* Check if Comic already has full HP */
                    if (comic_hp >= MAX_HP) {
                        /* Full HP: award an extra life */
                        award_extra_life();
                    } else {
                        /* Not full: schedule only the missing HP increments
                         * For example, if comic_hp=3 and MAX_HP=6, schedule 3 increments */
                        comic_hp_pending_increase = MAX_HP - comic_hp;
                    }
                    break;
                case ITEM_TELEPORT_WAND:
                    /* Grant teleport ability */
                    comic_has_teleport_wand = 1;
                    break;
                case ITEM_DOOR_KEY:
                    /* Grant door opening ability */
                    comic_has_door_key = 1;
                    break;
                case ITEM_CROWN:
                case ITEM_GOLD:
                case ITEM_GEMS:
                    /* Treasure collection and victory condition check:
                     * Collecting the three treasures (Crown, Gold, Gems) triggers the win condition.
                     * When comic_num_treasures reaches 3, set win_counter to 200.
                     * The game loop decrements win_counter each tick, and when it reaches 1,
                     * it triggers game_end_sequence() for the victory animation. */
                    if (comic_num_treasures < 3) {
                        comic_num_treasures++;
                        /* If all three treasures collected, trigger victory sequence */
                        if (comic_num_treasures == 3) {
                            win_counter = 200;  /* Set countdown to begin victory sequence when counter reaches 1 */
                        }
                    }
                    break;
                /* TODO: Implement other item types */
                default:
                    break;
            }
            
            return; /* Item collected, no need to render */
        }
    }
    
    /* Render item sprite */
    {
        int16_t pixel_x, pixel_y;
        const uint8_t *sprite_ptr = NULL;
        
        /* Calculate screen position relative to camera */
        rel_x = (int8_t)(item_x - camera_x);
        
        /* Convert to pixel coordinates (8 pixels per game unit) + playfield offset */
        pixel_x = (rel_x * 8) + 8;
        pixel_y = (item_y * 8) + 8;

        switch (item_type) {
            case ITEM_BLASTOLA_COLA:
                /* Always use base sprites for items in levels (not firepower-level variants) */
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_blastola_cola_even_16x16m
                    : sprite_blastola_cola_odd_16x16m;
                break;
            case ITEM_CORKSCREW:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_corkscrew_even_16x16m
                    : sprite_corkscrew_odd_16x16m;
                break;
            case ITEM_DOOR_KEY:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_door_key_even_16x16m
                    : sprite_door_key_odd_16x16m;
                break;
            case ITEM_BOOTS:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_boots_even_16x16m
                    : sprite_boots_odd_16x16m;
                break;
            case ITEM_LANTERN:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_lantern_even_16x16m
                    : sprite_lantern_odd_16x16m;
                break;
            case ITEM_TELEPORT_WAND:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_teleport_wand_even_16x16m
                    : sprite_teleport_wand_odd_16x16m;
                break;
            case ITEM_GEMS:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_gems_even_16x16m
                    : sprite_gems_odd_16x16m;
                break;
            case ITEM_CROWN:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_crown_even_16x16m
                    : sprite_crown_odd_16x16m;
                break;
            case ITEM_GOLD:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_gold_even_16x16m
                    : sprite_gold_odd_16x16m;
                break;
            case ITEM_SHIELD:
                sprite_ptr = (item_animation_counter == 0)
                    ? sprite_shield_even_16x16m
                    : sprite_shield_odd_16x16m;
                break;
            default:
                break;
        }

        if (sprite_ptr != NULL) {
            blit_sprite_16x16_masked((uint16_t)pixel_x, (uint16_t)pixel_y, sprite_ptr);
        }
    }
}

/* ===== Enemy System ===== */

/* Global flag to ensure no more than one enemy spawns per tick */
static int spawned_this_tick = 0;

/*
 * maybe_spawn_enemy - Attempt to spawn one enemy
 * 
 * Returns:
 *   1 if enemy was spawned, 0 otherwise
 */
int maybe_spawn_enemy(int enemy_index)
{
    const stage_t *stage;
    enemy_t *enemy;
    uint8_t spawn_x, spawn_y;
    int8_t y_search;
    static uint8_t spawn_offset_cycle = PLAYFIELD_WIDTH; /* Cycles: 24, 26, 28, 30 */
    
    /* Reset timer to 0 (matches ASM behavior) */
    enemies[enemy_index].spawn_timer_and_animation = 0;
    
    /* Check if another enemy already spawned this tick */
    if (spawned_this_tick) {
        return 0;
    }
    
    if (current_level_ptr == NULL) {
        return 0;
    }
    
    /* Validate stage number is within bounds (0-2) */
    if (current_stage_number >= 3) {
        return 0;
    }
    
    stage = &current_level_ptr->stages[current_stage_number];
    enemy = &enemies[enemy_index];
    
    /* Calculate spawn X position based on Comic's facing direction
     * Advance spawn_offset_cycle: cycles through PLAYFIELD_WIDTH, +2, +4, +6
     * This determines how far outside the playfield the enemy spawns: 0, 2, 4, or 6 units */
    spawn_offset_cycle += 2;
    if (spawn_offset_cycle >= PLAYFIELD_WIDTH + 7) {
        spawn_offset_cycle = PLAYFIELD_WIDTH;
    }
    
    if (comic_facing == COMIC_FACING_RIGHT) {
        /* Spawn to the right: camera_x + spawn_offset_cycle */
        spawn_x = (uint8_t)(camera_x + spawn_offset_cycle);
    } else {
        /* Spawn to the left: camera_x - (spawn_offset_cycle - PLAYFIELD_WIDTH + 2) */
        spawn_x = (uint8_t)(camera_x - (spawn_offset_cycle - PLAYFIELD_WIDTH + 2));
    }
    
    /* spawn_x is uint8_t (0-255), MAP_WIDTH is 256, so no bounds check needed */
    
    /* Find valid vertical spawn position: search UPWARD from Comic's feet
     * to find a non-solid tile above a solid tile (like the ASM does).
     * Start at Comic's feet (comic_y + 4), clamped to even boundary */
    spawn_y = (comic_y & 0xFE) + 4;  /* Clamp to even tile boundary, then add 4 (Comic's feet) */
    
    /* First, search upward to find a solid tile */
    for (y_search = 0; y_search < 20; y_search++) {
        uint8_t tile_at_y;
        
        if (spawn_y == 0) {
            /* Reached top of map without finding solid tile */
            return 0;
        }
        
        tile_at_y = get_tile_at(spawn_x, spawn_y);
        
        if (is_tile_solid(tile_at_y)) {
            /* Found a solid tile, now search upward for a non-solid tile above it */
            goto find_nonsolid;
        }
        
        /* Move up 2 units (one tile) */
        spawn_y -= 2;
    }
    
    /* No solid tile found */
    return 0;

find_nonsolid:
    /* Now search upward to find a non-solid tile above the solid tile */
    for (y_search = 0; y_search < 20; y_search++) {
        uint8_t tile_at_y;
        
        if (spawn_y == 0) {
            /* Reached top of map without finding non-solid tile */
            return 0;
        }
        
        tile_at_y = get_tile_at(spawn_x, spawn_y);
        
        if (!is_tile_solid(tile_at_y)) {
            /* Found a non-solid tile above solid ground - this is the spawn position */
            goto spawn_enemy;
        }
        
        /* Move up 2 units (one tile) */
        spawn_y -= 2;
    }
    
    /* No non-solid tile found */
    return 0;

spawn_enemy:
    /* Check if this is an unused enemy slot */
    if ((enemy->behavior & ~ENEMY_BEHAVIOR_FAST) == ENEMY_BEHAVIOR_UNUSED) {
        /* Undo the spawn - clear the flag and stay despawned */
        spawned_this_tick = 0;
        enemy->state = ENEMY_STATE_DESPAWNED;
        enemy->spawn_timer_and_animation = 100; /* Try again in 100 ticks */
        return 0;
    }
    
    /* Mark that we spawned an enemy this tick */
    spawned_this_tick = 1;
    
    /* Initialize enemy state */
    enemy->x = spawn_x;
    enemy->y = spawn_y;
    enemy->state = ENEMY_STATE_SPAWNED;
    enemy->spawn_timer_and_animation = 0; /* Reset animation frame */
    
    /* Initialize velocities based on behavior */
    switch (enemy->behavior & ~ENEMY_BEHAVIOR_FAST) {
        case ENEMY_BEHAVIOR_BOUNCE:
        case ENEMY_BEHAVIOR_SHY:
            enemy->x_vel = -1;
            enemy->y_vel = -1;
            enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
            break;
            
        case ENEMY_BEHAVIOR_LEAP:
        case ENEMY_BEHAVIOR_ROLL:
        case ENEMY_BEHAVIOR_SEEK:
        default:
            enemy->x_vel = 0;
            enemy->y_vel = 0;
            enemy->facing = COMIC_FACING_LEFT;
            break;
    }
    
    enemy->restraint = 0;
    
    return 1; /* Enemy spawned */
}

/*
 * handle_enemies - Update all enemies (AI, spawning, collision, rendering)
 */
void handle_enemies(void)
{
    int i;
    int16_t rel_x_enemy; /* used when rendering sparks */
    int16_t pixel_x, pixel_y; /* used when rendering sparks */
    const uint8_t *spark_ptr; /* used when rendering sparks */
    uint8_t spark_frame; /* used when rendering sparks */
    
    /* Reset spawn flag for this tick */
    spawned_this_tick = 0;
    
    /* Update each enemy slot */
    for (i = 0; i < MAX_NUM_ENEMIES; i++) {
        enemy_t *enemy = &enemies[i];
        int16_t x_diff, y_diff;
        
        /* Handle despawned state */
        if (enemy->state == ENEMY_STATE_DESPAWNED) {
            /* Decrement spawn timer */
            if (enemy->spawn_timer_and_animation > 0) {
                enemy->spawn_timer_and_animation--;
            }
            
            /* Attempt to spawn when timer reaches 0 */
            if (enemy->spawn_timer_and_animation == 0) {
                /* Always try - maybe_spawn_enemy will check the global flag
                 * If spawn fails, timer stays at 0 and will retry next tick */
                maybe_spawn_enemy(i);
            }
            continue;
        }
        
        /* Handle dying state (white spark or red spark) */
        if (enemy->state >= ENEMY_STATE_WHITE_SPARK && enemy->state != ENEMY_STATE_SPAWNED) {
            /* If the state is the "finished" value (ENEMY_STATE_WHITE_SPARK + 5 or ENEMY_STATE_RED_SPARK + 5),
             * immediately despawn without rendering the final frame to match original assembly behavior. */
            if (enemy->state == ENEMY_STATE_WHITE_SPARK + 5 || enemy->state == ENEMY_STATE_RED_SPARK + 5) {
                enemy->state = ENEMY_STATE_DESPAWNED;
                enemy->spawn_timer_and_animation = enemy_respawn_counter_cycle;
                /* Cycle respawn counter: 20→40→60→80→100→20 */
                enemy_respawn_counter_cycle += 20;
                if (enemy_respawn_counter_cycle > 100) {
                    enemy_respawn_counter_cycle = 20;
                }
                continue;
            }

            /* Declarations for rendering (C89: must appear before statements) */
            rel_x_enemy = (int16_t)((int)enemy->x - (int)camera_x);

            /* Render death animation (spark) - use state BEFORE incrementing */
            if (rel_x_enemy >= -1 && rel_x_enemy < PLAYFIELD_WIDTH + 1) {
                /* Convert to pixel coordinates */
                pixel_x = (rel_x_enemy * 8) + 8;
                pixel_y = (enemy->y * 8) + 8;

                if (enemy->state >= ENEMY_STATE_RED_SPARK) {
                    spark_frame = (uint8_t)((enemy->state - ENEMY_STATE_RED_SPARK) % 3);
                    switch (spark_frame) {
                        case 0: spark_ptr = sprite_red_spark_0_16x16m; break;
                        case 1: spark_ptr = sprite_red_spark_1_16x16m; break;
                        default: spark_ptr = sprite_red_spark_2_16x16m; break;
                    }
                } else {
                    spark_frame = (uint8_t)((enemy->state - ENEMY_STATE_WHITE_SPARK) % 3);
                    switch (spark_frame) {
                        case 0: spark_ptr = sprite_white_spark_0_16x16m; break;
                        case 1: spark_ptr = sprite_white_spark_1_16x16m; break;
                        default: spark_ptr = sprite_white_spark_2_16x16m; break;
                    }
                }

                if (spark_ptr != NULL) {
                    blit_sprite_16x16_masked((uint16_t)pixel_x, (uint16_t)pixel_y, spark_ptr);
                }
            }

            /* Advance animation state */
            enemy->state++;

            /* Check if animation complete */
            if (enemy->state == 7 || enemy->state == 13) {
                /* Death animation complete, despawn */
                enemy->state = ENEMY_STATE_DESPAWNED;
                enemy->spawn_timer_and_animation = enemy_respawn_counter_cycle;

                /* Cycle respawn counter: 20→40→60→80→100→20 */
                enemy_respawn_counter_cycle += 20;
                if (enemy_respawn_counter_cycle > 100) {
                    enemy_respawn_counter_cycle = 20;
                }
            }
            continue;
        }
        
        /* Handle spawned state */
        if (enemy->state == ENEMY_STATE_SPAWNED) {
            /* Advance animation frame */
            enemy->spawn_timer_and_animation++;
            if (enemy->spawn_timer_and_animation >= enemy->num_animation_frames) {
                enemy->spawn_timer_and_animation = 0;
            }
            
            /* Execute AI behavior */
            switch (enemy->behavior & ~ENEMY_BEHAVIOR_FAST) {
                case ENEMY_BEHAVIOR_BOUNCE:
                    enemy_behavior_bounce(enemy);
                    break;
                case ENEMY_BEHAVIOR_LEAP:
                    enemy_behavior_leap(enemy);
                    break;
                case ENEMY_BEHAVIOR_ROLL:
                    enemy_behavior_roll(enemy);
                    break;
                case ENEMY_BEHAVIOR_SEEK:
                    enemy_behavior_seek(enemy);
                    break;
                case ENEMY_BEHAVIOR_SHY:
                    enemy_behavior_shy(enemy);
                    break;
            }
            
            /* Check despawn distance (30 game units from Comic) */
            x_diff = (int16_t)((int)enemy->x - (int)comic_x);
            if (x_diff < -ENEMY_DESPAWN_RADIUS || x_diff > ENEMY_DESPAWN_RADIUS) {
                enemy->state = ENEMY_STATE_DESPAWNED;
                enemy->spawn_timer_and_animation = enemy_respawn_counter_cycle;
                continue;
            }
            
            /* Check collision with Comic */
            y_diff = (int16_t)((int)enemy->y - (int)comic_y);
            
            /* Horizontal: abs(enemy.x - comic_x) < 2 */
            if (x_diff >= -1 && x_diff <= 1) {
                /* Vertical: 0 <= (enemy.y - comic_y) < 4 */
                if (y_diff >= 0 && y_diff < 4) {
                    /* Collision detected! */
                    enemy->state = ENEMY_STATE_RED_SPARK; /* Start red spark death animation */
                    
                    /* Handle damage to Comic (shield loss, HP loss, or death) */
                    comic_takes_damage();
                    continue;
                }
            }
            
            /* Render enemy sprite if in playfield */
            {
                int16_t rel_x_enemy;
                int16_t pixel_x, pixel_y;
                uint16_t frame_size = 0;
                const uint8_t *frame_ptr;
                uint8_t shp_index = enemy_shp_index[i];
                
                /* Calculate screen position relative to camera */
                rel_x_enemy = (int16_t)((int)enemy->x - (int)camera_x);
                
                /* Check if in visible playfield (match assembly bounds: 0 to PLAYFIELD_WIDTH - 2) */
                if (rel_x_enemy >= 0 && rel_x_enemy <= PLAYFIELD_WIDTH - 2) {
                    /* Convert to pixel coordinates (8 pixels per game unit) + playfield offset */
                    pixel_x = (rel_x_enemy * 8) + 8;
                    pixel_y = (enemy->y * 8) + 8;

                    frame_ptr = get_enemy_frame(shp_index, enemy->spawn_timer_and_animation, enemy->facing, &frame_size);
                    
                    if (frame_ptr != NULL) {
                        if (frame_size == 160) {
                            /* Use masked blit for 16x16 .SHP sprites */
                            blit_sprite_16x16_masked((uint16_t)pixel_x, (uint16_t)pixel_y, frame_ptr);
                        } else if (frame_size == 320) {
                            blit_sprite_16x32_masked((uint16_t)pixel_x, (uint16_t)pixel_y, frame_ptr);
                        }
                    }
                } else {
                    /* Enemy is offscreen, skip rendering */
                }
            }
        }
    }
}

/* ===== Enemy AI Behaviors ===== */

/*
 * enemy_behavior_bounce - Diagonal bouncing AI
 */
void enemy_behavior_bounce(enemy_t *enemy)
{
    uint8_t next_x, next_y;
    uint8_t tile;
    int16_t camera_rel_x;
    
    /* Handle restraint */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }
    
    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }
    
    /* Horizontal movement */
    if (enemy->x_vel > 0) {
        /* Moving right */
        enemy->facing = COMIC_FACING_RIGHT;
        next_x = (uint8_t)(enemy->x + 2);
        tile = get_tile_at(next_x, enemy->y);
        if (is_tile_solid(tile)) {
            enemy->x_vel = -1;
        } else {
            enemy->x = (uint8_t)(enemy->x + 1);
            camera_rel_x = (int16_t)enemy->x - (int16_t)camera_x;
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;
            }
        }
    } else {
        /* Moving left */
        enemy->facing = COMIC_FACING_LEFT;
        if (enemy->x == 0) {
            enemy->x_vel = 1;
        } else {
            next_x = (uint8_t)(enemy->x - 1);
            tile = get_tile_at(next_x, enemy->y);
            if (is_tile_solid(tile)) {
                enemy->x_vel = 1;
            } else {
                enemy->x = next_x;
                camera_rel_x = (int16_t)enemy->x - (int16_t)camera_x;
                if (camera_rel_x <= 0) {
                    enemy->x_vel = 1;
                }
            }
        }
    }
    
    /* Vertical movement */
    if (enemy->y_vel > 0) {
        /* Moving down */
        if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
            enemy->y_vel = -1;
        } else {
            next_y = (uint8_t)(enemy->y + 2);
            tile = get_tile_at(enemy->x, next_y);
            if (is_tile_solid(tile)) {
                enemy->y_vel = -1;
            } else {
                enemy->y = (uint8_t)(enemy->y + 1);
                if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
                    enemy->y_vel = -1;
                }
            }
        }
    } else {
        /* Moving up */
        if (enemy->y == 0) {
            enemy->y_vel = 1;
        } else {
            next_y = (uint8_t)(enemy->y - 1);
            tile = get_tile_at(enemy->x, next_y);
            if (is_tile_solid(tile)) {
                enemy->y_vel = 1;
            } else {
                enemy->y = next_y;
                if (enemy->y == 0) {
                    enemy->y_vel = 1;
                }
            }
        }
    }
}

/*
 * enemy_behavior_leap - Jumping arc with low gravity
 */
void enemy_behavior_leap(enemy_t *enemy)
{
    uint8_t next_x;
    uint8_t proposed_y = enemy->y;
    uint8_t tile;
    int16_t camera_rel_x;
    int8_t vel_div_8;
    unsigned char collision;
    unsigned char just_jumped = 0; /* true for the tick a jump is initiated */
    

    
    /* Check current vertical velocity state */
    if (enemy->y_vel < 0) {
        vel_div_8 = enemy->y_vel >> 3;  /* Use arithmetic shift to match assembly */
        proposed_y = enemy->y + vel_div_8; /* Since vel_div_8 is negative, this moves up */
        if (proposed_y == enemy->y) {
            /* No vertical movement this tick */
        }
        
        /* Check top edge */
        if (proposed_y >= 254) { /* Underflow check */
            /* Hit top of playfield: restore to 0 but do not zero velocity; assembly just undoes position */
            proposed_y = 0;
        } else {
            /* Check collision */
            collision = 0;
            tile = get_tile_at(enemy->x, proposed_y);
            if (is_tile_solid(tile)) collision = 1;
            if (enemy->x & 1) {
                tile = get_tile_at(enemy->x + 1, proposed_y);
                if (is_tile_solid(tile)) collision = 1;
            }
            if (collision) {
                /* Hit ceiling - undo the position change but preserve y_vel; gravity will reduce upward speed */
            } else {
                /* Defer writing `enemy->y` until later; use proposed_y for subsequent checks */
            }
        }
    } else if (enemy->y_vel > 0) {
        vel_div_8 = enemy->y_vel >> 3;  /* Use arithmetic shift */
        proposed_y = enemy->y + vel_div_8;
        if (vel_div_8 == 0) {
            /* No vertical move this tick; will check ground at y+1 */
        }
        
        /* Check bottom edge */
        if (proposed_y >= PLAYFIELD_HEIGHT - 2) {
            debug_log("LEAP moving_down: fell off bottom -> despawn (y=%u)\n", (unsigned)proposed_y);
            /* Fell off bottom - despawn */
            enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
            enemy->y = PLAYFIELD_HEIGHT - 2;
            return;
        }
        
        /* Check collision with ground below */
        collision = 0;
        tile = get_tile_at(enemy->x, (uint8_t)(proposed_y + 1));
        if (is_tile_solid(tile)) collision = 1;
        if (enemy->x & 1) {
            tile = get_tile_at(enemy->x + 1, (uint8_t)(proposed_y + 1));
            if (is_tile_solid(tile)) collision = 1;
        }
        if (collision) {
            /* Collision detected when moving down: undo position change (keep current y)
             * and allow gravity to act on subsequent ticks. */
            proposed_y = enemy->y;
        } else {
            /* No collision: keep proposed_y at the advanced position. */
        }
    } else {
        /* y_vel == 0 - check if on ground */
        collision = 0;
        tile = get_tile_at(enemy->x, (uint8_t)(enemy->y + 2));
        if (is_tile_solid(tile)) collision = 1;
        if (enemy->x & 1) {
            tile = get_tile_at(enemy->x + 1, (uint8_t)(enemy->y + 2));
            if (is_tile_solid(tile)) collision = 1;
        }
        if (collision) {
            /* On ground - initiate jump
         * NOTE: increase initial upward impulse so the peak reaches ~6 game units
         * (original assembly used -7, which yields ~4 units; -10 yields ~6 units).
         * This matches expected game behaviour observed in the original release. */
            enemy->y_vel = -10; /* stronger initial leap for full arc */
            just_jumped = 1; /* assembly does not apply gravity on the same tick */

            /* Set horizontal velocity toward Comic */
            if (enemy->x < comic_x) {
                enemy->x_vel = 1;
            } else {
                enemy->x_vel = -1;
            }
        } else {
            /* Not on ground - start falling. Use an initial downward velocity
             * of 8 units (1 full game unit this tick) to match the original
             * game's quicker edge-fall behaviour. */
            enemy->y_vel = 8;
        }
    }
    
    /* Apply gravity every tick (unless we just started a jump; assembly skips gravity that tick) */
    if (!just_jumped) {
        enemy->y_vel += 2;  /* Gravity for LEAP behavior */
        if (enemy->y_vel > TERMINAL_VELOCITY) {
            enemy->y_vel = TERMINAL_VELOCITY;
        }
    } else {
        /* We just initiated a jump; do not apply gravity this tick */
    }



    /* Handle restraint */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }
    
    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }
    
    /* Horizontal movement */
    if (enemy->x_vel > 0) {
        /* Moving right */
        next_x = (uint8_t)(enemy->x + 2);
        collision = 0;
        tile = get_tile_at(next_x, proposed_y);
        if (is_tile_solid(tile)) collision = 1;
        if (proposed_y & 1) {
            tile = get_tile_at(next_x, proposed_y + 1);
            if (is_tile_solid(tile)) collision = 1;
        }
        if (collision) {
            /* Hit wall - bounce left */
            enemy->x_vel = -1;
        } else {
            enemy->x = (uint8_t)(enemy->x + 1);
            
            /* Check playfield right edge */
            camera_rel_x = (int16_t)enemy->x - (int16_t)camera_x;
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;
            } else {
                /* moved right */
            }
        }
    } else {
        /* Moving left */
        if (enemy->x == 0) {
            enemy->x_vel = 1;
        } else {
            next_x = (uint8_t)(enemy->x - 1);
            collision = 0;
            tile = get_tile_at(next_x, proposed_y);
            if (is_tile_solid(tile)) collision = 1;
            if (proposed_y & 1) {
                tile = get_tile_at(next_x, proposed_y + 1);
                if (is_tile_solid(tile)) collision = 1;
            }
            if (collision) {
                enemy->x_vel = 1;
            } else {
                enemy->x = next_x;

                /* Check playfield left edge */
                camera_rel_x = (int16_t)enemy->x - (int16_t)camera_x;
                if (camera_rel_x <= 0) {
                    enemy->x_vel = 1;
                } else {
                    /* moved left */
                }
            }
        }
    }

    /* Check for ground after movement */
    if (enemy->y_vel > 0) {
        collision = 0;
        tile = get_tile_at(enemy->x, (uint8_t)(proposed_y + 3));
        if (is_tile_solid(tile)) collision = 1;
        if (enemy->x & 1) {
            tile = get_tile_at(enemy->x + 1, (uint8_t)(proposed_y + 3));
            if (is_tile_solid(tile)) collision = 1;
        }
        if (collision) {
            /* Landed on ground.
             * Align the committed Y so that (enemy->y + 2) points at the same
             * tile that caused the landing check (proposed_y + 3).
             * This prevents the next-tick `y_vel == 0` ground check from
             * disagreeing with the landing detection and re-starting a fall.
             */
            uint8_t tile_y = (uint8_t)((proposed_y + 3) / 2);
            proposed_y = (uint8_t)(tile_y * 2 - 2); /* commit so enemy->y + 2 maps to tile_y*2 */
            enemy->y_vel = 0;

        }
    }

    /* Commit the provisional Y computed above (matches ASM which stores at function end) */
    enemy->y = proposed_y;

}



/*
 * enemy_behavior_roll - Ground-following toward player
 */
void enemy_behavior_roll(enemy_t *enemy)
{
    uint8_t next_x;
    uint8_t tile;
    
    if (enemy->y_vel > 0) {
        /* Falling - check if near bottom */
        if ((uint8_t)(enemy->y + 1) >= PLAYFIELD_HEIGHT - 2 - 1) {
            /* Near bottom - despawn */
            enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
            enemy->y = PLAYFIELD_HEIGHT - 2;
            return;
        }
    }
    
    /* Rolling - set direction toward Comic */
    if (enemy->x < comic_x) {
        enemy->x_vel = 1;
    } else if (enemy->x > comic_x) {
        enemy->x_vel = -1;
    } else {
        enemy->x_vel = 0;
    }
    
    /* Handle restraint */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }
    
    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }
    
    /* Horizontal movement */
    if (enemy->x_vel == 0) {
        /* Directly above/below Comic - skip movement */
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }
    
    if (enemy->x_vel > 0) {
        /* Moving right */
        next_x = (uint8_t)(enemy->x + 2);
        tile = get_tile_at(next_x, enemy->y);
        if (!is_tile_solid(tile)) {
            enemy->x = (uint8_t)(enemy->x + 1);
        }
    } else {
        /* Moving left */
        next_x = (uint8_t)(enemy->x - 1);
        tile = get_tile_at(next_x, enemy->y);
        if (!is_tile_solid(tile)) {
            enemy->x = next_x;
        }
    }
    
    /* Check for ground below */
    tile = get_tile_at(enemy->x, (uint8_t)(enemy->y + 3));
    if (!is_tile_solid(tile)) {
        /* No ground - start falling */
        enemy->y_vel = 1;
        return;
    }
    
    /* On ground - clamp to even boundary if just landed */
    if (enemy->y_vel != 0) {
        enemy->y = (uint8_t)((enemy->y + 1) & 0xFE);
    }
    enemy->y_vel = 0;
    
    /* Update facing */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
}

/*
 * enemy_behavior_seek - Pathfinding toward player
 *
 * Enhanced to match other AI routines:
 *  - width-aware tile checks when X is odd
 *  - consistent use of x_vel and facing
 *  - playfield-edge and fall/despawn handling
 *  - restraint normalization like other behaviors
 */
void enemy_behavior_seek(enemy_t *enemy)
{
    uint8_t next_x, next_y, tile;
    unsigned char hcollision = 0, vcollision = 0;
    int16_t camera_rel_x;

    /* Handle restraint (same pattern as other AIs) */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }

    /* --- Horizontal movement toward Comic (preferred) --- */
    if (enemy->x != comic_x) {
        if (enemy->x < comic_x) {
            /* Attempt step right (check ahead at x+2 like other handlers) */
            next_x = (uint8_t)(enemy->x + 1);
            hcollision = 0;
            tile = get_tile_at((uint8_t)(next_x + 1), enemy->y); /* check tile at x+2 */
            if (is_tile_solid(tile)) hcollision = 1;
            if (enemy->y & 1) {
                tile = get_tile_at((uint8_t)(next_x + 1), (uint8_t)(enemy->y + 1));
                if (is_tile_solid(tile)) hcollision = 1;
            }

            if (!hcollision) {
                enemy->x = next_x;
                enemy->x_vel = 1;
                camera_rel_x = (int16_t)enemy->x - (int16_t)camera_x;
                if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                    /* Hit right playfield edge — reverse like other AIs */
                    enemy->x_vel = -1;
                }
                enemy->facing = COMIC_FACING_RIGHT;
                return;
            } else {
                /* Blocked horizontally but keep intended direction */
                enemy->x_vel = 1;
            }
        } else {
            /* Attempt step left */
            next_x = (uint8_t)(enemy->x - 1);
            hcollision = 0;
            tile = get_tile_at((uint8_t)(next_x - 1), enemy->y); /* check tile at x-1 */
            if (is_tile_solid(tile)) hcollision = 1;
            if (enemy->y & 1) {
                tile = get_tile_at((uint8_t)(next_x - 1), (uint8_t)(enemy->y + 1));
                if (is_tile_solid(tile)) hcollision = 1;
            }

            if (!hcollision) {
                enemy->x = next_x;
                enemy->x_vel = -1;
                camera_rel_x = (int16_t)next_x - (int16_t)camera_x;
                if (camera_rel_x <= 0) {
                    /* Hit left playfield edge — reverse */
                    enemy->x_vel = 1;
                }
                enemy->facing = COMIC_FACING_LEFT;
                return;
            } else {
                enemy->x_vel = -1;
            }
        }
    } else {
        /* Same X as Comic */
        enemy->x_vel = 0;
    }

    /* --- Vertical fallback when horizontal movement is blocked or aligned --- */
    if (enemy->y != comic_y) {
        if (enemy->y < comic_y) {
            /* Move down */
            next_y = (uint8_t)(enemy->y + 1);

            /* If falling off bottom, despawn like other AIs */
            if (next_y >= PLAYFIELD_HEIGHT - 2) {
                enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
                enemy->y = PLAYFIELD_HEIGHT - 2;
                return;
            }

            vcollision = 0;
            tile = get_tile_at(enemy->x, (uint8_t)(next_y + 1));
            if (is_tile_solid(tile)) vcollision = 1;
            if (enemy->x & 1) {
                tile = get_tile_at(enemy->x + 1, (uint8_t)(next_y + 1));
                if (is_tile_solid(tile)) vcollision = 1;
            }

            if (!vcollision) {
                enemy->y = next_y;
            } else {
                /* blocked vertically — do nothing this tick (will try alternatives next tick) */
            }
        } else {
            /* Move up */
            next_y = (uint8_t)(enemy->y - 1);
            if (next_y == 0) {
                /* Bounce off top like other AIs */
                /* leave y as-is */
            } else {
                vcollision = 0;
                tile = get_tile_at(enemy->x, next_y);
                if (is_tile_solid(tile)) vcollision = 1;
                if (enemy->x & 1) {
                    tile = get_tile_at(enemy->x + 1, next_y);
                    if (is_tile_solid(tile)) vcollision = 1;
                }
                if (!vcollision) {
                    enemy->y = next_y;
                }
            }
        }
    }

    /* Update facing consistent with other behaviors */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;

    /* Normalize restraint state (same logic as enemy_behavior_shy) */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    } else if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    } else {
        if (enemy->restraint > ENEMY_RESTRAINT_MOVE_EVERY_TICK) {
            enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        }
    }
}


/*
 * enemy_behavior_shy - Flee when facing Comic
 */
void enemy_behavior_shy(enemy_t *enemy)
{
    uint8_t next_x, next_y;
    uint8_t tile;
    int8_t comic_facing_enemy;
    unsigned char vcollision;
    unsigned char hcollision;
    int16_t camera_rel_x;
    
    /* Check restraint (movement throttle) */
    if (enemy->restraint > 0) {
        enemy->restraint--;
        return; /* Skip movement this tick */
    }
    
    /* Determine if Comic is facing this enemy */
    if (comic_facing == COMIC_FACING_RIGHT && enemy->x > comic_x) {
        comic_facing_enemy = 1; /* Comic facing enemy on right */
    } else if (comic_facing == COMIC_FACING_LEFT && enemy->x < comic_x) {
        comic_facing_enemy = 1; /* Comic facing enemy on left */
    } else {
        comic_facing_enemy = 0; /* Comic facing away */
    }
    
    /* Behavior: flee upward when Comic faces enemy, otherwise seek Comic's Y */
    if (comic_facing_enemy) {
        /* Move up */
        enemy->y_vel = -1;
    } else {
        /* Move toward Comic's Y position */
        if (enemy->y < comic_y) {
            enemy->y_vel = 1;
        } else if (enemy->y > comic_y) {
            enemy->y_vel = -1;
        } else {
            enemy->y_vel = 0;
        }
    }
    
    /* Update Y position (handle top/bottom edges and tile collision, like assembly) */
    next_y = (uint8_t)(enemy->y + enemy->y_vel);
    /* Bottom of playfield bounce */
    if (enemy->y_vel > 0 && next_y >= PLAYFIELD_HEIGHT - 2) {
        enemy->y_vel = -1; /* bounce up */
    } else if (enemy->y_vel < 0 && next_y == 0) {
        enemy->y_vel = 1; /* bounce down off top */
    } else {
        /* Check vertical tiles (consider enemy width when x is odd) */
        vcollision = 0;
        if (enemy->y_vel > 0) {
            tile = get_tile_at(enemy->x, (uint8_t)(next_y + 1));
            if (is_tile_solid(tile)) vcollision = 1;
            if (enemy->x & 1) {
                tile = get_tile_at(enemy->x + 1, (uint8_t)(next_y + 1));
                if (is_tile_solid(tile)) vcollision = 1;
            }
            if (!vcollision) {
                enemy->y = next_y;
            } else {
                /* Bounce up */
                enemy->y_vel = -1;
            }
        } else if (enemy->y_vel < 0) {
            tile = get_tile_at(enemy->x, next_y);
            if (is_tile_solid(tile)) vcollision = 1;
            if (enemy->x & 1) {
                tile = get_tile_at(enemy->x + 1, next_y);
                if (is_tile_solid(tile)) vcollision = 1;
            }
            if (!vcollision) {
                enemy->y = next_y;
            } else {
                /* Bounce down */
                enemy->y_vel = 1;
            }
        }
    }

    /* Update X position with horizontal tile + playfield edge checks */
    if (enemy->x_vel > 0) {
        /* Moving right */
        next_x = (uint8_t)(enemy->x + 1);
        hcollision = 0;
        tile = get_tile_at((uint8_t)(next_x + 1), enemy->y); /* check tile at x+2 like assembly */
        if (is_tile_solid(tile)) hcollision = 1;
        if (enemy->y & 1) {
            tile = get_tile_at((uint8_t)(next_x + 1), (uint8_t)(enemy->y + 1));
            if (is_tile_solid(tile)) hcollision = 1;
        }
        if (hcollision) {
            enemy->x_vel = -1;
        } else {
            /* Check playfield right edge relative to camera */
            camera_rel_x = (int16_t)next_x - (int16_t)camera_x;
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;
            } else {
                enemy->x = next_x;
            }
        }
    } else if (enemy->x_vel < 0) {
        /* Moving left */
        if (enemy->x == 0) {
            enemy->x_vel = 1;
        } else {
            next_x = (uint8_t)(enemy->x - 1);
            hcollision = 0;
            tile = get_tile_at(next_x - 1, enemy->y); /* check tile at x-1 (assembly checks x-1 then uses dec) */
            if (is_tile_solid(tile)) hcollision = 1;
            if (enemy->y & 1) {
                tile = get_tile_at(next_x - 1, (uint8_t)(enemy->y + 1));
                if (is_tile_solid(tile)) hcollision = 1;
            }
            if (hcollision) {
                enemy->x_vel = 1;
            } else {
                camera_rel_x = (int16_t)next_x - (int16_t)camera_x;
                if (camera_rel_x <= 0) {
                    enemy->x_vel = 1;
                } else {
                    enemy->x = next_x;
                }
            }
        }
    }

    /* Update facing direction */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;

    /* Reset restraint: use same alternating behavior as other enemies */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    } else if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    } else {
        /* ENEMY_RESTRAINT_MOVE_EVERY_TICK (fast enemy) stays as-is */
        if (enemy->restraint > ENEMY_RESTRAINT_MOVE_EVERY_TICK) {
            enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK; /* normalize unexpected values */
        }
    }
}
