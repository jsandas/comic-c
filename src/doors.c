/**
 * doors.c - Door system implementation
 * 
 * Handles door collision detection, key checking, and level/stage transitions.
 * Matches the original assembly behavior for door mechanics.
 */

#include <stddef.h>
#include <stdint.h>
#include "doors.h"
#include "level_data.h"

/* External game state variables from game_main.c */
extern uint8_t comic_x;
extern uint8_t comic_y;
extern uint8_t comic_has_door_key;
extern uint8_t key_state_open;
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern const level_t *current_level_ptr;
extern int8_t source_door_level_number;
extern int8_t source_door_stage_number;

/* Forward declarations for functions in game_main.c */
int load_new_level(void);
void load_new_stage(void);

/**
 * enter_door - Stub for door entry animation
 * 
 * In the original assembly, this plays a 4-frame animation:
 * 1. Door halfway open (Comic in front)
 * 2. Door fully open
 * 3. Door halfway closed (Comic behind door)
 * 4. Door fully closed
 * 
 * For now, this is a stub that does nothing.
 * TODO: Implement full door animation sequence
 */
static void enter_door(uint8_t door_x, uint8_t door_y)
{
    /* Stub - animation not implemented yet */
    (void)door_x;
    (void)door_y;
}

/**
 * exit_door - Stub for door exit animation
 * 
 * In the original assembly, this plays a 5-frame animation:
 * 1. Door fully closed (with delay)
 * 2. Door halfway open (Comic behind door)
 * 3. Door fully open
 * 4. Door halfway closed (Comic in front)
 * 5. Door fully closed
 * 
 * For now, this is a stub that does nothing.
 * TODO: Implement full door animation sequence
 */
static void exit_door(void)
{
    /* Stub - animation not implemented yet */
}

/**
 * check_door_activation - Check if Comic is in front of a door and the open key is pressed
 * 
 * Assembly logic:
 * .check_open_input:
 *     cmp byte [cs:key_state_open], 1
 *     jne .check_teleport_input
 *     
 *     mov di, [current_stage_ptr]
 *     lea bx, [di + stage.doors]
 *     mov cx, 3               ; for all 3 doors
 *     mov ax, si              ; al = comic_y, ah = comic_x
 * .door_loop:
 *     mov dx, [bx]            ; dl = door.y, dh = door.x
 *     cmp al, dl              ; require exact match in y-coordinate
 *     jne .door_loop_next
 *     neg dh
 *     add dh, ah              ; dh = comic_x - door.x
 *     cmp dh, 3               ; require comic_x - door.x to be 0, 1, or 2
 *     jae .door_loop_next
 *     cmp byte [comic_has_door_key], 1
 *     jne .door_loop_next
 *     jmp activate_door       ; tail call
 * .door_loop_next:
 *     add bx, door_size       ; next door
 *     loop .door_loop
 */
uint8_t check_door_activation(void)
{
    int i;
    const door_t *door;
    int8_t x_offset;
    
    /* If the open key is not being pressed, no door activation */
    if (key_state_open != 1) {
        return 0;
    }
    
    /* Get current stage data */
    if (current_level_ptr == NULL) {
        return 0;  /* No level loaded */
    }
    
    /* Check all doors in the current stage */
    for (i = 0; i < 3; i++) {  /* MAX_NUM_DOORS */
        door = &current_level_ptr->stages[current_stage_number].doors[i];
        
        /* Door is unused if x or y is 0xff */
        if (door->x == DOOR_UNUSED || door->y == DOOR_UNUSED) {
            continue;
        }
        
        /* Check Y coordinate: must be exact match
         * comic_y is in game units, door.y is in tile coordinates
         * One tile = 2 game units, so door tile Y maps to game Y = door.y * 2 */
        if (comic_y != door->y) {
            continue;
        }
        
        /* Check X coordinate: must be within 3 units
         * comic_x is in game units (half-tiles), door.x is in tile coordinates
         * One tile = 2 game units, so door tile X maps to game X = door.x * 2
         * We allow comic_x - door.x to be 0, 1, or 2 */
        x_offset = comic_x - door->x;
        if (x_offset < 0 || x_offset > 2) {
            continue;
        }
        
        /* Check if player has the Door Key */
        if (comic_has_door_key != 1) {
            continue;  /* Door is locked, skip to next door */
        }
        
        /* All checks passed: activate this door */
        activate_door(door);
        return 1;  /* Door was activated */
    }
    
    /* No doors activated */
    return 0;
}

/**
 * activate_door - Perform door transition to target level/stage
 * 
 * Assembly logic:
 * activate_door:
 *     push bx
 *     mov ax, [bx + door.y]   ; al = door.y, ah = door.x
 *     call enter_door         ; (animation/sound)
 *     pop bx
 *     
 *     mov al, [current_level_number]
 *     mov [source_door_level_number], al
 *     mov al, [current_stage_number]
 *     mov [source_door_stage_number], al
 *     
 *     mov al, [bx + door.target_stage]
 *     mov [current_stage_number], al
 *     mov al, [bx + door.target_level]
 *     mov [current_level_number], al
 *     
 *     cmp al, [source_door_level_number]
 *     je .same_level
 *     jmp load_new_level      ; different level
 * .same_level:
 *     jmp load_new_stage      ; same level
 */
void activate_door(const door_t *door)
{
    /* Play door entry animation (stub for now) */
    enter_door(door->x, door->y);
    
    /* Save the source level and stage so the destination can find the
     * reciprocal door (for proper positioning when exiting) */
    source_door_level_number = current_level_number;
    source_door_stage_number = current_stage_number;
    
    /* Set the current level and stage to the door's target */
    current_stage_number = door->target_stage;
    current_level_number = door->target_level;
    
    /* If the target is in a different level, load the entire level
     * (including tileset, enemy graphics, etc.) */
    if (door->target_level != source_door_level_number) {
        load_new_level();
        /* After loading the level assets, load the stage */
        load_new_stage();
    } else {
        /* Same level: only load the new stage (map, enemies, items) */
        load_new_stage();
    }
}
