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
#include "file_loaders.h"
#include "sprite_data.h"

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

/* Camera state */
extern uint16_t camera_x;                  /* Camera X position (game units) */

/* Rendering state */
extern uint16_t offscreen_video_buffer_ptr; /* Current offscreen buffer offset */

/* Game state */
extern uint16_t score;                     /* Current score */
extern const level_t *current_level_ptr; /* Pointer to current level data */
extern uint8_t current_stage_number;       /* Current stage (0-based) */
extern uint8_t current_level_number;       /* Current level (0-based, 0=LAKE, 1=FOREST, etc.) */
extern uint8_t *current_tiles_ptr;         /* Pointer to current stage's tile map */
extern uint8_t tileset_last_passable;      /* Last passable tile ID (tiles > this are solid) */
/* Item tracking */
extern uint8_t items_collected[8][16];     /* Bitmap: items_collected[level][stage] */
extern uint8_t item_animation_counter;     /* 0 or 1, toggles for item animation */

/* Respawn timing */
extern uint8_t enemy_respawn_counter_cycle; /* Cycles: 20→40→60→80→100→20 */

/* Forward declarations for external functions */
extern void comic_dies(void);               /* Game over sequence from physics.c */

/* ===== Actor Arrays ===== */
enemy_t enemies[MAX_NUM_ENEMIES];
fireball_t fireballs[MAX_NUM_FIREBALLS];
uint8_t enemy_shp_index[MAX_NUM_ENEMIES];

/* ===== Forward Declarations ===== */
static void comic_takes_damage(void);
static void award_points(uint16_t points);
static uint8_t is_tile_solid(uint8_t tile_id);
static uint8_t get_tile_at(uint8_t x, uint8_t y);
static const uint8_t *get_enemy_frame(uint8_t shp_index, uint8_t anim_index, uint8_t facing, uint16_t *out_frame_size);

/* ===== Helper Functions ===== */

/*
 * comic_takes_damage - Reduce Comic's HP and start shield/death animation
 * 
 * If Comic has shield, remove shield instead of losing HP.
 * If HP reaches 0, trigger death sequence.
 */
static void comic_takes_damage(void)
{
    if (comic_has_shield) {
        comic_has_shield = 0;
        /* TODO: Play shield break sound */
    } else if (comic_hp > 0) {
        comic_hp--;
        if (comic_hp == 0) {
            /* Comic is dead - trigger game over sequence */
            comic_dies();
        }
    }
}

/*
 * award_points - Add points to score
 * 
 * Parameters:
 *   points - Number of points to add
 */
static void award_points(uint16_t points)
{
    /* Prevent overflow */
    if (score > 65535U - points) {
        score = 65535U;
    } else {
        score += points;
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
            
            /* TODO: Play SOUND_FIRE with priority 0 */
            
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
            award_points(300);
            /* TODO: Play SOUND_HIT_ENEMY with priority 1 */
            
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
            award_points(2000);
            /* TODO: Play SOUND_COLLECT_ITEM with priority 3 */
            
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
                case ITEM_SHIELD:
                    comic_has_shield = 1;
                    break;
                case ITEM_TELEPORT_WAND:
                    /* Grant teleport ability */
                    comic_has_teleport_wand = 1;
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
            /* Render death animation (spark) - use state BEFORE incrementing */
            {
                int16_t rel_x_enemy;
                int16_t pixel_x, pixel_y;
                const uint8_t *spark_ptr = NULL;
                uint8_t spark_frame;
                
                /* Calculate screen position relative to camera */
                rel_x_enemy = (int16_t)((int)enemy->x - (int)camera_x);
                
                /* Check if in visible playfield */
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
                    comic_takes_damage();
                    /* TODO: Play collision sound */
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
    
    /* Check restraint (movement throttle) */
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return; /* Skip movement this tick */
    }
    
    /* Transition slow enemies from MOVE_THIS_TICK to SKIP_THIS_TICK */
    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }
    
    /* Horizontal movement */
    next_x = enemy->x;
    if (enemy->x_vel > 0) {
        /* Moving right */
        enemy->facing = COMIC_FACING_RIGHT;
        
        /* Check tile collision at x+2 */
        tile = get_tile_at((uint8_t)(enemy->x + 2), enemy->y);
        if (is_tile_solid(tile)) {
            /* Hit solid tile, bounce left */
            enemy->x_vel = -1;
        } else {
            /* No tile collision, try to move */
            next_x = enemy->x + 1;
            
            /* Check playfield right edge */
            camera_rel_x = (int16_t)next_x - (int16_t)camera_x;
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                /* Hit right edge, bounce left */
                enemy->x_vel = -1;
                next_x = enemy->x;
            }
        }
    } else {
        /* Moving left */
        enemy->facing = COMIC_FACING_LEFT;
        
        /* Check left map edge */
        if (enemy->x == 0) {
            /* Hit left map edge, bounce right */
            enemy->x_vel = 1;
        } else {
            /* Check tile collision at x-1 */
            tile = get_tile_at((uint8_t)(enemy->x - 1), enemy->y);
            if (is_tile_solid(tile)) {
                /* Hit solid tile, bounce right */
                enemy->x_vel = 1;
            } else {
                /* No tile collision, try to move */
                next_x = enemy->x - 1;
                
                /* Check playfield left edge */
                camera_rel_x = (int16_t)next_x - (int16_t)camera_x;
                if (camera_rel_x <= 0) {
                    /* Hit left edge, bounce right */
                    enemy->x_vel = 1;
                    next_x = enemy->x;
                }
            }
        }
    }
    
    /* Vertical movement */
    next_y = enemy->y;
    if (enemy->y_vel > 0) {
        /* Moving down */
        if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
            /* At bottom edge, bounce up */
            enemy->y_vel = -1;
        } else {
            /* Check tile collision at y+2 */
            tile = get_tile_at(enemy->x, (uint8_t)(enemy->y + 2));
            if (is_tile_solid(tile)) {
                /* Hit solid tile, bounce up */
                enemy->y_vel = -1;
            } else {
                /* No collision, try to move */
                next_y = enemy->y + 1;
                
                /* Double-check bottom edge */
                if (next_y >= PLAYFIELD_HEIGHT - 2) {
                    enemy->y_vel = -1;
                    next_y = enemy->y;
                }
            }
        }
    } else {
        /* Moving up */
        if (enemy->y == 0) {
            /* At top edge, bounce down */
            enemy->y_vel = 1;
        } else {
            /* Check tile collision at y-1 */
            tile = get_tile_at(enemy->x, (uint8_t)(enemy->y - 1));
            if (is_tile_solid(tile)) {
                /* Hit solid tile, bounce down */
                enemy->y_vel = 1;
            } else {
                /* No collision, try to move */
                next_y = enemy->y - 1;
                
                /* Double-check top edge */
                if (next_y == 0) {
                    enemy->y_vel = 1;
                    next_y = enemy->y;
                }
            }
        }
    }
    
    /* Update position */
    enemy->x = next_x;
    enemy->y = next_y;
}

/*
 * enemy_behavior_leap - Jumping arc with gravity
 */
void enemy_behavior_leap(enemy_t *enemy)
{
    uint8_t next_y;
    uint8_t tile;
    
    /* Apply gravity (+2 per tick, capped at TERMINAL_VELOCITY = 23) */
    if (enemy->y_vel < 23) {
        enemy->y_vel += 2;
    }
    
    /* Calculate next Y position */
    next_y = enemy->y + enemy->y_vel;
    
    /* Check ground collision */
    tile = get_tile_at(enemy->x, next_y);
    if (is_tile_solid(tile)) {
        /* Landed on ground - jump! */
        enemy->y_vel = -7; /* Jump velocity */
        
        /* Jump toward Comic's X position */
        if (enemy->x < comic_x) {
            enemy->x_vel = 1;
        } else if (enemy->x > comic_x) {
            enemy->x_vel = -1;
        }
    } else {
        /* In air - update Y position */
        enemy->y = next_y;
    }
    
    /* Update X position if moving */
    if (enemy->x_vel != 0) {
        uint8_t next_x = enemy->x + enemy->x_vel;
        
        /* Check horizontal collision with walls */
        tile = get_tile_at(next_x, enemy->y);
        if (!is_tile_solid(tile)) {
            /* No wall, update position (uint8_t can't exceed map width) */
            enemy->x = next_x;
        } else {
            /* Hit wall - bounce */
            enemy->x_vel = -enemy->x_vel;
        }
    }
    
    /* Update facing direction */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
    
    /* Check if fell off bottom of map */
    if (enemy->y >= MAP_HEIGHT) {
        enemy->state = ENEMY_STATE_DESPAWNED;
        enemy->spawn_timer_and_animation = enemy_respawn_counter_cycle;
    }
}

/*
 * enemy_behavior_roll - Ground-following
 */
void enemy_behavior_roll(enemy_t *enemy)
{
    uint8_t next_x;
    uint8_t tile_below;
    uint8_t tile;
    
    /* Determine movement direction toward Comic */
    if (enemy->x < comic_x) {
        enemy->x_vel = 1;
    } else if (enemy->x > comic_x) {
        enemy->x_vel = -1;
    } else {
        enemy->x_vel = 0;
    }
    
    /* Check for ground beneath enemy */
    tile_below = get_tile_at(enemy->x, enemy->y + 1);
    if (!is_tile_solid(tile_below)) {
        /* No ground - fall */
        enemy->y_vel = 1;
        enemy->y += enemy->y_vel;
        return;
    }
    
    /* On ground - move horizontally */
    if (enemy->x_vel != 0) {
        next_x = enemy->x + enemy->x_vel;
        
        /* Check horizontal collision */
        tile = get_tile_at(next_x, enemy->y);
        if (!is_tile_solid(tile)) {
            /* No wall, update position (uint8_t can't exceed map width) */
            enemy->x = next_x;
        }
    }
    
    /* Clamp to even tile boundaries when landing */
    enemy->y = (enemy->y & 0xFE);
    
    /* Update facing direction */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
}

/*
 * enemy_behavior_seek - Pathfinding toward player
 */
void enemy_behavior_seek(enemy_t *enemy)
{
    uint8_t next_x, next_y;
    uint8_t tile;
    
    /* Check restraint (movement throttle) */
    if (enemy->restraint > 0) {
        enemy->restraint--;
        return; /* Skip movement this tick */
    }
    
    /* First match Comic's X, then match Y */
    if (enemy->x != comic_x) {
        /* Move horizontally toward Comic */
        if (enemy->x < comic_x) {
            enemy->x_vel = 1;
        } else {
            enemy->x_vel = -1;
        }
        
        next_x = enemy->x + enemy->x_vel;
        
        /* Check horizontal collision (uint8_t can't exceed MAP_WIDTH) */
        tile = get_tile_at(next_x, enemy->y);
        if (!is_tile_solid(tile)) {
            enemy->x = next_x;
        }
    } else {
        /* X aligned, now move vertically toward Comic */
        if (enemy->y != comic_y) {
            if (enemy->y < comic_y) {
                enemy->y_vel = 1;
            } else {
                enemy->y_vel = -1;
            }
            
            next_y = enemy->y + enemy->y_vel;
            
            /* Check vertical collision (next_y is uint8_t, can't be < 0) */
            tile = get_tile_at(enemy->x, next_y);
            if (!is_tile_solid(tile) && next_y < MAP_HEIGHT) {
                enemy->y = next_y;
            }
        }
    }
    
    /* Update facing direction */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
    
    /* Reset restraint */
    enemy->restraint = 1; /* Move every other tick */
}

/*
 * enemy_behavior_shy - Flee when facing Comic
 */
void enemy_behavior_shy(enemy_t *enemy)
{
    uint8_t next_x, next_y;
    uint8_t tile;
    int8_t comic_facing_enemy;
    
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
    
    /* Update Y position (next_y is uint8_t, can't be < 0) */
    next_y = enemy->y + enemy->y_vel;
    tile = get_tile_at(enemy->x, next_y);
    if (!is_tile_solid(tile) && next_y < MAP_HEIGHT) {
        enemy->y = next_y;
    } else {
        /* Bounce off obstacle */
        enemy->y_vel = -enemy->y_vel;
    }
    
    /* Update X position (always moving, uint8_t can't exceed MAP_WIDTH) */
    next_x = enemy->x + enemy->x_vel;
    tile = get_tile_at(next_x, enemy->y);
    if (!is_tile_solid(tile)) {
        enemy->x = next_x;
    } else {
        /* Bounce off obstacle */
        enemy->x_vel = -enemy->x_vel;
    }
    
    /* Update facing direction */
    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
    
    /* Reset restraint */
    enemy->restraint = 1; /* Move every other tick */
}
