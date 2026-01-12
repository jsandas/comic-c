/**
 * physics.c - Physics and collision detection implementation
 * 
 * Implements gravity, jumping, movement, and collision detection for Comic.
 * This is translated from R5sw1991.asm handle_fall_or_jump, move_left, move_right, etc.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
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
extern uint8_t comic_jump_counter;
extern uint8_t comic_jump_power;
extern uint8_t comic_run_cycle;
extern uint8_t ceiling_stick_flag;
extern uint8_t tileset_last_passable;
extern uint8_t *current_tiles_ptr;
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern const level_t *current_level_ptr;
extern uint16_t camera_x;

/* External functions */
extern void comic_dies(void);
extern void load_new_stage(void);

/* Helper: Check if a tile is solid */
static uint8_t is_tile_solid(uint8_t tile_id)
{
    /* A tile is solid if its ID is > tileset_last_passable */
    extern uint8_t tileset_last_passable;
    return tile_id > tileset_last_passable;
}

/* Helper: Get tile at coordinates from current stage's map */
static uint8_t get_tile_at(uint8_t x, uint8_t y)
{
    extern uint8_t *current_tiles_ptr;
    uint16_t offset = address_of_tile_at_coordinates(x, y);
    
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
    
    extern uint8_t current_level_number;
    extern uint8_t ceiling_stick_flag;
    extern uint8_t key_state_jump;
    extern uint8_t key_state_left;
    extern uint8_t key_state_right;
    extern uint8_t comic_animation;
    
    int8_t delta_y;
    uint8_t head_tile;
    uint8_t head_solid;
    uint8_t foot_y;
    uint8_t foot_tile;
    uint8_t foot_solid;
    
    /* Decrement jump counter to track when upward acceleration phase ends */
    comic_jump_counter--;
    
    /* If jump counter expired (was 1, now 0), clamp it back to 1 
     * This marks the end of the upward acceleration phase */
    if (comic_jump_counter == 0) {
        comic_jump_counter = 1;
    } else if (key_state_jump) {
        /* Still in jump acceleration phase and jump key is held:
         * Apply upward acceleration */
        comic_y_vel -= 7;  /* Subtract 7 from velocity (going upward) */
    }
    
    /* If we weren't accelerating upward, clear the ceiling stick flag */
    if (comic_jump_counter <= 0 || !key_state_jump) {
        ceiling_stick_flag = 0;
    }
    
    /* Integrate velocity: move by comic_y_vel / 8 */
    delta_y = comic_y_vel / 8;
    comic_y += delta_y;
    
    /* Clamp to top of screen */
    if (comic_y < 0) {
        comic_y = 0;
    }
    
    /* Apply ceiling stick (push down 1 unit if against ceiling) */
    if (ceiling_stick_flag) {
        comic_y++;
        ceiling_stick_flag = 0;
    }
    
    /* Check bounds: if head is within 3 units of bottom, death by falling */
    if (comic_y >= PLAYFIELD_HEIGHT - 3) {
        comic_dies();  /* This function terminates the level */
        return;
    }
    
    /* Apply gravity */
    if (current_level_number == LEVEL_NUMBER_SPACE) {
        /* Reduced gravity in space level */
        comic_y_vel += (COMIC_GRAVITY - COMIC_GRAVITY_SPACE);
    } else {
        comic_y_vel += COMIC_GRAVITY;
    }
    
    /* Clamp downward velocity to terminal velocity */
    if (comic_y_vel > TERMINAL_VELOCITY) {
        comic_y_vel = TERMINAL_VELOCITY;
    }
    
    /* Adjust horizontal momentum based on input */
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
    
    /* Apply horizontal momentum with drag towards zero */
    if (comic_x_momentum < 0) {
        /* Negative momentum: drag towards zero */
        comic_x_momentum++;
        move_left();
    } else if (comic_x_momentum > 0) {
        /* Positive momentum: drag towards zero */
        comic_x_momentum--;
        move_right();
    }
    
    /* Check solidity above (for upward collision) */
    if (comic_y_vel < 0) {  /* Moving upward */
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
    
    /* Check solidity below (for downward collision / ground detection) */
    if (comic_y_vel > 0) {  /* Moving downward */
        foot_y = comic_y + 5;  /* Position 1 unit below feet */
        foot_tile = get_tile_at(comic_x, foot_y);
        foot_solid = is_tile_solid(foot_tile);
        
        /* Also check the tile to the right if Comic is between tiles */
        if (!foot_solid && (comic_x & 1)) {
            foot_tile = get_tile_at(comic_x + 1, foot_y);
            foot_solid = is_tile_solid(foot_tile);
        }
        
        /* Additional safety check: if foot_y >= PLAYFIELD_HEIGHT - 5,
         * we're looking outside the map bounds, so don't treat it as solid */
        if (foot_solid && foot_y < PLAYFIELD_HEIGHT - 5) {
            /* Hit the ground: snap to even tile boundary and land */
            comic_y = (comic_y + 1) & 0xfe;  /* Round up to next even number */
            comic_is_falling_or_jumping = 0;
            comic_y_vel = 0;
        } else {
            /* Still falling or jumping */
            comic_animation = COMIC_JUMPING;
        }
    } else {
        /* Still falling or jumping */
        comic_animation = COMIC_JUMPING;
    }
}

void move_left(void)
{
    /* Move Comic left if possible, handling stage edges and collisions */
    
    extern uint8_t current_stage_number;
    extern const level_t *current_level_ptr;
    extern uint16_t camera_x;
    extern uint8_t comic_run_cycle;
    
    const stage_t *stage;
    uint8_t new_x;
    uint8_t check_y;
    uint8_t tile_id;
    
    /* Check if at left edge of stage */
    if (comic_x == 0) {
        /* At left edge: check for stage transition */
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
    if (camera_x > 0 && (comic_x - camera_x) < PLAYFIELD_WIDTH / 2 - 2) {
        camera_x--;
    }
}

void move_right(void)
{
    /* Move Comic right if possible, handling stage edges and collisions */
    
    extern uint8_t current_stage_number;
    extern const level_t *current_level_ptr;
    extern uint16_t camera_x;
    extern uint8_t comic_run_cycle;
    
    const stage_t *stage;
    uint8_t new_x;
    uint8_t check_y;
    uint8_t check_tile_x;
    uint8_t tile_id;
    uint16_t max_camera_x;
    
    /* Check if at right edge of stage */
    if (comic_x >= MAP_WIDTH - 2) {
        /* At right edge: check for stage transition */
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
    if (camera_x < max_camera_x && (comic_x - camera_x) > PLAYFIELD_WIDTH / 2) {
        camera_x++;
    }
}

void face_or_move_left(void)
{
    /* If already facing left, move left. Otherwise just face left. */
    if (comic_facing == COMIC_FACING_LEFT) {
        move_left();
    } else {
        comic_facing = COMIC_FACING_LEFT;
    }
}

void face_or_move_right(void)
{
    /* If already facing right, move right. Otherwise just face right. */
    if (comic_facing == COMIC_FACING_RIGHT) {
        move_right();
    } else {
        comic_facing = COMIC_FACING_RIGHT;
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
