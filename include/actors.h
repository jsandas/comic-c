/*
 * actors.h - Actor system data structures and function declarations
 * 
 * This header defines the data structures for enemies, fireballs, and items,
 * as well as the functions for managing these game actors.
 */

#ifndef ACTORS_H
#define ACTORS_H

#include <stdint.h>

/* ===== Enemy Constants ===== */
#define MAX_NUM_ENEMIES          4
#define ENEMY_DESPAWN_RADIUS     30   /* game units from Comic */

/* Enemy behavior types (stored in enemy.behavior field) */
#define ENEMY_BEHAVIOR_BOUNCE    1    /* Fire Ball, Brave Bird, etc. - diagonal bouncing */
#define ENEMY_BEHAVIOR_LEAP      2    /* Bug-eyes, Blind Toad, Beach Ball - jumping arc */
#define ENEMY_BEHAVIOR_ROLL      3    /* Glow Globe - ground-following */
#define ENEMY_BEHAVIOR_SEEK      4    /* Killer Bee - pathfinding toward player */
#define ENEMY_BEHAVIOR_SHY       5    /* Shy Bird, Spinner - flee when facing Comic */
#define ENEMY_BEHAVIOR_FAST      0x80 /* OR with behavior for fast movement */

/* Enemy state values */
#define ENEMY_STATE_DESPAWNED    0    /* Not spawned */
#define ENEMY_STATE_SPAWNED      1    /* Active and moving */
#define ENEMY_STATE_WHITE_SPARK  2    /* Death animation (killed by fireball), frames 2-6 */
#define ENEMY_STATE_RED_SPARK    8    /* Death animation (collided with Comic), frames 8-12 */

/* ===== Fireball Constants ===== */
#define MAX_NUM_FIREBALLS        5    /* Maximum firepower */
#define FIREBALL_DEAD            0xFF /* Inactive marker in x and y fields */
#define FIREBALL_VELOCITY        2    /* Horizontal velocity magnitude (±2) */

/* Item types and ITEM_UNUSED are defined in level_data.h */

/* ===== Enemy Data Structure ===== */
/*
 * Each enemy occupies 12 bytes.
 * Array of 4 enemies per stage.
 */
typedef struct {
    uint8_t  y;                         /* Y position (game units) */
    uint8_t  x;                         /* X position (game units) */
    int8_t   x_vel;                     /* Horizontal velocity (-1, 0, +1) */
    int8_t   y_vel;                     /* Vertical velocity (varies by behavior) */
    uint8_t  spawn_timer_and_animation; /* Dual purpose: spawn timer when despawned, animation frame when spawned */
    uint8_t  num_animation_frames;      /* Number of animation frames for this enemy */
    uint8_t  behavior;                  /* AI behavior type (1-5, bit 0x80 = fast) */
    uint16_t animation_frames_ptr;      /* Pointer to animation frame graphics (16-bit near pointer) */
    uint8_t  state;                     /* 0=despawned, 1=spawned, 2-6=white spark, 8-12=red spark */
    uint8_t  facing;                    /* 0=left, 5=right (offsets into animation frames) */
    uint8_t  restraint;                 /* Movement throttle (0=move, 1=skip, 2+=move every tick) */
} enemy_t;

/* ===== Fireball Data Structure ===== */
/*
 * Each fireball occupies 6 bytes.
 * Array of 5 fireballs (maximum firepower).
 */
typedef struct {
    uint8_t y;                    /* Y position (0xFF = inactive) */
    uint8_t x;                    /* X position (0xFF = inactive) */
    int8_t  vel;                  /* Horizontal velocity (+2 or -2) */
    uint8_t corkscrew_phase;      /* Vertical oscillation (1 or 2) */
    uint8_t animation;            /* Current animation frame */
    uint8_t num_animation_frames; /* Always 2 for fireballs */
} fireball_t;

/* ===== Global Actor Arrays ===== */
/*
 * These arrays store the state of all enemies and fireballs.
 * Declared in game_main.c as static arrays.
 */
extern enemy_t enemies[MAX_NUM_ENEMIES];
extern fireball_t fireballs[MAX_NUM_FIREBALLS];
extern uint8_t enemy_shp_index[MAX_NUM_ENEMIES];

/* ===== Actor System Functions ===== */

/*
 * try_to_fire - Attempt to spawn a fireball
 * 
 * Checks if Comic has firepower and if a fireball slot is available.
 * If both conditions are met, spawns a fireball at Comic's position
 * with velocity based on Comic's facing direction.
 * 
 * Called when the fire key is pressed and fireball meter is not empty.
 */
void try_to_fire(void);

/*
 * handle_fireballs - Update all active fireballs
 * 
 * For each active fireball (first comic_firepower slots):
 *   - Apply horizontal velocity (±2 per tick)
 *   - Apply corkscrew motion if Comic has Corkscrew item (vertical oscillation)
 *   - Advance animation frame
 *   - Check despawn conditions (off-screen)
 *   - Check collision with all enemies
 *   - Award points and kill enemy on collision
 * 
 * Called once per game tick.
 */
void handle_fireballs(void);

/*
 * handle_item - Update and render the current stage's item
 * 
 * Checks if the current stage has an item and if it's visible.
 * If visible:
 *   - Advance animation counter (toggles between 0 and 1)
 *   - Check collision with Comic
 *   - Award points and update game state on collection
 *   - Render item sprite at its position
 * 
 * Called once per game tick.
 */
void handle_item(void);

/*
 * handle_enemies - Update all enemies (AI, spawning, collision, rendering)
 * 
 * For each enemy slot (4 per stage):
 *   - If despawned: decrement spawn timer, spawn when timer reaches 0
 *   - If spawned: execute AI behavior, check collision, render
 *   - If dying: advance death animation, render spark
 * 
 * Enemy spawning logic:
 *   - Spawn position cycles: PLAYFIELD_WIDTH + {0, 2, 4, 6} units outside playfield
 *   - Spawn on side Comic is facing
 *   - Find vertical position: non-solid tile above solid tile
 * 
 * Collision with Comic:
 *   - Horizontal: abs(enemy.x - comic_x) < 2
 *   - Vertical: 0 <= (enemy.y - comic_y) < 4
 *   - On collision: start red spark death animation, hurt/kill Comic
 * 
 * Called once per game tick.
 */
void handle_enemies(void);

/*
 * maybe_spawn_enemy - Attempt to spawn one enemy
 * 
 * Helper function called by handle_enemies when an enemy's spawn timer reaches 0.
 * Only spawns 1 enemy per tick to avoid overwhelming the player.
 * 
 * Returns:
 *   1 if enemy was spawned
 *   0 if enemy was not spawned (no valid spawn position found or already spawned)
 */
int maybe_spawn_enemy(int enemy_index);

/*
 * enemy_behavior_bounce - Diagonal bouncing AI
 * 
 * Used by Fire Ball, Brave Bird, etc.
 * Moves ±1 per tick (if not throttled by restraint).
 * Bounces off solid tiles and playfield edges.
 * Independent x and y velocities.
 */
void enemy_behavior_bounce(enemy_t *enemy);

/*
 * enemy_behavior_leap - Jumping arc with gravity
 * 
 * Used by Bug-eyes, Blind Toad, Beach Ball.
 * Gravity: +2 per tick, capped at TERMINAL_VELOCITY (23).
 * Jump velocity: -7.
 * Jump toward Comic's x position.
 * Bounces off playfield edges, falls off bottom.
 */
void enemy_behavior_leap(enemy_t *enemy);

/*
 * enemy_behavior_roll - Ground-following
 * 
 * Used by Glow Globe.
 * Moves horizontally toward Comic.
 * Falls when no ground beneath (+1 y_vel).
 * Clamps to even tile boundaries when landing.
 */
void enemy_behavior_roll(enemy_t *enemy);

/*
 * enemy_behavior_seek - Pathfinding toward player
 * 
 * Used by Killer Bee.
 * First matches Comic's x, then matches y.
 * Moves ±1 per tick (if not throttled).
 * Stops when aligned on either axis.
 */
void enemy_behavior_seek(enemy_t *enemy);

/*
 * enemy_behavior_shy - Flee when facing Comic
 * 
 * Used by Shy Bird, Spinner.
 * When Comic faces enemy: move up.
 * When Comic faces away: move toward Comic's y position.
 * Bounces off solid tiles and edges.
 * Independent x/y velocities.
 */
void enemy_behavior_shy(enemy_t *enemy);

#endif /* ACTORS_H */
