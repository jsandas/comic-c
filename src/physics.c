/**
 * physics.c - Physics and collision detection implementation
 * 
 * Implements gravity, jumping, movement, and collision detection for Comic.
 * This is translated from R5sw1991.asm handle_fall_or_jump, move_left, move_right, etc.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "physics.h"
#include "globals.h"
#include "level_data.h"

/* External variables - game state */
extern uint8_t comic_x;
extern uint8_t comic_y;
extern int8_t comic_y_vel;
extern int8_t comic_x_momentum;
extern uint8_t comic_facing;
extern uint8_t comic_animation;
extern uint8_t comic_is_falling_or_jumping;
extern uint8_t minimum_jump_frames;
extern uint8_t comic_jump_counter;
extern uint8_t comic_jump_power;
extern uint8_t comic_run_cycle;
extern uint8_t ceiling_stick_flag;
extern uint8_t comic_fall_delay;
extern uint8_t tileset_last_passable;
extern uint8_t *current_tiles_ptr;
extern uint8_t current_level_number;

/* External function - debug logging */
extern void debug_log(const char *format, ...);
extern uint8_t current_stage_number;
extern const level_t *current_level_ptr;
extern uint16_t camera_x;
extern uint8_t landed_this_tick;

/* External input state (set by keyboard handling in game_main.c) */
extern uint8_t key_state_jump;
extern uint8_t key_state_left;
extern uint8_t key_state_right;

/* External functions */
extern void comic_dies(void);
extern void load_new_stage(void);

/* Helper: Check if a tile is solid */
static uint8_t is_tile_solid(uint8_t tile_id)
{
    /* A tile is solid if its ID is > tileset_last_passable */
    return tile_id > tileset_last_passable;
}
/* Helper: Get tile at coordinates from current stage's map
 * Note: x and y are in game units (0-255, 0-19), but address_of_tile_at_coordinates
 * expects tile coordinates (0-127, 0-9). Each tile is 2 game units, so we divide by 2. */
static uint8_t get_tile_at(uint8_t x, uint8_t y)
{
    uint16_t offset = address_of_tile_at_coordinates(x / 2, y / 2);
    
    /* Bounds check: if offset exceeds map size, return non-solid */
    if (offset >= MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
        return 0;  /* Non-solid tile outside map bounds */
    }
    
    if (current_tiles_ptr == NULL) {
        return 0;  /* No tile map loaded */
    }
    
    return current_tiles_ptr[offset];
}

void handle_fall_or_jump(void)
{
    /* Core physics loop: handles gravity, jumping, vertical collision,
     * horizontal momentum, and movement.
     * 
     * Key state variables:
     *   comic_jump_counter - When > 1, we can still accelerate upward
     *   comic_y_vel - Vertical velocity (1/8 game units per tick), signed
     *   comic_x_momentum - Horizontal momentum (-5 to +5)
     *   comic_x, comic_y - Position (game units)
     *   key_state_jump, key_state_left, key_state_right - Input flags
     */
    
    /* Use file-scope externs for current_level_number, ceiling_stick_flag,
     * comic_animation, and key_state_* declared above */
    
    int8_t delta_y;
    uint8_t head_tile;
    uint8_t head_solid;
    uint8_t foot_y;
    uint8_t foot_tile;
    uint8_t foot_solid;
    uint8_t platform_tile;
    uint8_t tile_row;
    
    /* DEBUG: Log jump physics state */
    debug_log("PHYSICS_DEBUG: falling=%d, counter=%d, jump_key=%d, vel=%d\n",
              comic_is_falling_or_jumping, comic_jump_counter, key_state_jump, comic_y_vel);
    
    /* If not in air (standing on ground), don't process physics - just return */
    if (comic_is_falling_or_jumping == 0) {
        return;
    }
    
    /* STEP 1: Decrement jump counter FIRST (matches assembly) */
    if (comic_jump_counter > 0) {
        comic_jump_counter--;
    }
    
    /* STEP 2: Check if counter expired AFTER decrement - if so, set to 1 as sentinel */
    if (comic_jump_counter == 0) {
        comic_jump_counter = 1;
        ceiling_stick_flag = 0;
    }
    /* STEP 3: Apply upward acceleration if counter still > 0 (after decrement, before 0→1 conversion) and jump key held */
    else if (key_state_jump) {
        /* Counter is still > 0 (not expired yet), so apply acceleration */
        comic_y_vel -= JUMP_ACCELERATION;  /* Accelerate upward */
    } else {
        /* Counter > 0 but jump key released */
        ceiling_stick_flag = 0;
    }
    
    /* Decrement minimum jump frames counter (if still using this mechanism) */
    if (minimum_jump_frames > 0) {
        minimum_jump_frames--;
    }

    /* STEP 4: Integrate velocity - move by comic_y_vel / 8 */
    delta_y = comic_y_vel >> 3;  /* Arithmetic shift right by 3 = divide by 8 */
    {
        int16_t new_y = (int16_t)comic_y + (int16_t)delta_y;
        if (new_y < 0) {
            comic_y = 0;
        } else if (new_y > 255) {
            comic_y = 255;
        } else {
            comic_y = (uint8_t)new_y;
        }
    }
    
    /* Apply ceiling stick (push down 1 unit if against ceiling) */
    if (ceiling_stick_flag) {
        comic_y++;
        ceiling_stick_flag = 0;
    }
    
    /* Check bounds: if too far down, death by falling */
    if (comic_y >= PLAYFIELD_HEIGHT - 3) {
        comic_dies();  /* This function terminates the level */
        return;
    }
    
    /* STEP 5: Apply gravity IMMEDIATELY after position update (matches assembly) */
    if (current_level_number == LEVEL_NUMBER_SPACE) {
        comic_y_vel += COMIC_GRAVITY_SPACE;
    } else {
        comic_y_vel += COMIC_GRAVITY;
    }
    
    /* Clamp to terminal velocity */
    if (comic_y_vel > TERMINAL_VELOCITY) {
        comic_y_vel = TERMINAL_VELOCITY;
    }
    
    /* STEP 6: Handle mid-air momentum and movement (matches assembly order) */
    if (key_state_left) {
        comic_facing = COMIC_FACING_LEFT;
        comic_x_momentum--;
        if (comic_x_momentum < -5) {
            comic_x_momentum = -5;
        }
    }
    
    if (key_state_right) {
        comic_facing = COMIC_FACING_RIGHT;
        comic_x_momentum++;
        if (comic_x_momentum > 5) {
            comic_x_momentum = 5;
        }
    }
    
    /* Apply horizontal movement with drag */
    if (comic_x_momentum < 0) {
        comic_x_momentum++;  /* Drag toward zero */
        move_left();
    }
    
    if (comic_x_momentum > 0) {
        comic_x_momentum--;  /* Drag toward zero */
        move_right();
    }
    
    /* STEP 7: Check ceiling collision (upward) */
    if (comic_y_vel < 0) {  /* Moving upward */
        /* Check at the top of Comic */
        head_tile = get_tile_at(comic_x, comic_y);
        head_solid = is_tile_solid(head_tile);
        
        /* Also check the tile to the right if Comic is between tiles */
        if (!head_solid && (comic_x & 1)) {
            head_tile = get_tile_at(comic_x + 1, comic_y);
            head_solid = is_tile_solid(head_tile);
        }
        
        if (head_solid) {
            /* Hit ceiling while jumping: stick and reset velocity */
            ceiling_stick_flag = 1;
            comic_y_vel = 0;
        }
    }
    
    /* STEP 8: Check ground collision (downward) */
    if (comic_y_vel > 0) {  /* Moving downward */
        /* Check 1 unit below Comic's feet: comic_y + 5 (matches original logic) */
        foot_y = comic_y + 5;
        foot_tile = get_tile_at(comic_x, foot_y);
        foot_solid = is_tile_solid(foot_tile);
        
        /* Also check the tile to the right if Comic is between tiles */
        if (!foot_solid && (comic_x & 1)) {
            foot_tile = get_tile_at(comic_x + 1, foot_y);
            foot_solid = is_tile_solid(foot_tile);
        }
        
        /* If we found solid ground, stop falling */
        if (foot_solid) {  /* Land as soon as we detect ground ahead */
            /* If we're below the map bottom, ignore the solid tile */
            if (comic_y >= PLAYFIELD_HEIGHT - 5) {
                comic_animation = COMIC_JUMPING;
                return;
            }
            /* Hit the ground: position Comic so his feet are just above the solid tile
             * Clamp Comic's feet to an even tile boundary (matches original logic). */
            
            /* DEBUG: Log landing info */
            tile_row = foot_y / 2;
            debug_log("PHYSICS_LAND: y=%d->%d, foot_y=%d, tile_row=%d, foot_tile=%d, vel=%d\n",
                    comic_y, comic_y - 2, foot_y, tile_row, foot_tile, comic_y_vel);
            
            comic_y = (uint8_t)((comic_y + 1) & 0xFE);  /* snap to even boundary */
            comic_is_falling_or_jumping = 0;
            comic_y_vel = 0;
            landed_this_tick = 1;  /* Signal main loop to skip ground movement this tick */
            /* Do NOT update comic_animation here - keep JUMPING to match assembly behavior.
             * The next tick, input processing will set running or standing animation. */
            return;  /* Exit early after landing */
        }
    }
    
    /* Still in air - set jumping animation */
    comic_animation = COMIC_JUMPING;
}

void move_left(void)
{
    /* Move Comic left if possible, handling stage edges and collisions */
    
    const stage_t *stage;
    uint8_t new_x;
    uint8_t check_y;
    uint8_t tile_id;
    
    /* Check if at left edge of stage */
    if (comic_x == 0) {
        /* At left edge: check for stage transition */
        /* Guard against NULL level pointer */
        if (current_level_ptr == NULL) {
            comic_x_momentum = 0;
            return;
        }
        stage = &current_level_ptr->stages[current_stage_number];
        if (stage->exit_l == EXIT_UNUSED) {
            /* No exit here, stop moving */
            comic_x_momentum = 0;
            return;
        }
        
        /* Stage transition to the left */
        comic_x = MAP_WIDTH - 2;
        current_stage_number = stage->exit_l;
        comic_y_vel = 0;
        load_new_stage();
        return;
    }
    
    /* Try moving left */
    new_x = comic_x - 1;
    check_y = comic_y + 3;  /* Check at knees */
    
    /* Check if we'd hit a wall */
    tile_id = get_tile_at(new_x, check_y);
    if (is_tile_solid(tile_id)) {
        /* Wall in the way, stop moving */
        comic_x_momentum = 0;
        return;
    }
    
    /* Can move left */
    comic_x = new_x;
    comic_animation = comic_run_cycle;
    
    /* Move camera left if appropriate */
    {
        int16_t relative_x = (int16_t)comic_x - (int16_t)camera_x;
        if (camera_x > 0 && relative_x < (int16_t)(PLAYFIELD_WIDTH / 2 - 2)) {
            camera_x--;
        }
    }
}

void move_right(void)
{
    /* Move Comic right if possible, handling stage edges and collisions */
    
    const stage_t *stage;
    uint8_t new_x;
    uint8_t check_y;
    uint8_t check_tile_x;
    uint8_t tile_id;
    uint16_t max_camera_x;
    
    /* Check if at right edge of stage */
    if (comic_x >= MAP_WIDTH - 2) {
        /* At right edge: check for stage transition */
        /* Guard against NULL level pointer */
        if (current_level_ptr == NULL) {
            comic_x_momentum = 0;
            return;
        }
        stage = &current_level_ptr->stages[current_stage_number];
        if (stage->exit_r == EXIT_UNUSED) {
            /* No exit here, stop moving */
            comic_x_momentum = 0;
            return;
        }
        
        /* Stage transition to the right */
        comic_x = 0;
        current_stage_number = stage->exit_r;
        comic_y_vel = 0;
        load_new_stage();
        return;
    }
    
    /* Try moving right */
    new_x = comic_x + 1;
    check_y = comic_y + 3;  /* Check at knees */
    check_tile_x = new_x + 1;  /* Look at tile 1 unit to the right */
    
    /* Check if we'd hit a wall */
    tile_id = get_tile_at(check_tile_x, check_y);
    if (is_tile_solid(tile_id)) {
        /* Wall in the way, stop moving */
        comic_x_momentum = 0;
        return;
    }
    
    /* Can move right */
    comic_x = new_x;
    comic_animation = comic_run_cycle;
    
    /* Move camera right if appropriate */
    max_camera_x = MAP_WIDTH - PLAYFIELD_WIDTH;
    {
        int16_t relative_x = (int16_t)comic_x - (int16_t)camera_x;
        if (camera_x < max_camera_x && relative_x > (int16_t)(PLAYFIELD_WIDTH / 2)) {
            camera_x++;
        }
    }
}

uint16_t address_of_tile_at_coordinates(uint8_t x, uint8_t y)
{
    /* Calculate byte offset into tile map for tile at (x, y)
     * 
     * Map layout is linear, row-major: tiles[y * 128 + x]
     * Coordinates should normally be valid (x: 0-127, y: 0-9)
     * If out of bounds, the returned offset will exceed valid map range.
     * Callers must check bounds before dereferencing.
     */
    return (uint16_t)y * MAP_WIDTH_TILES + x;
}
