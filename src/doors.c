/**
 * doors.c - Door system implementation
 * 
 * Handles door collision detection, key checking, and level/stage transitions.
 * Matches the original assembly behavior for door mechanics.
 */

#include <stddef.h>
#include <stdint.h>
#include <dos.h>
#include <conio.h>
#include "doors.h"
#include "level_data.h"
#include "sound.h"
#include "sound_data.h"

/* Video memory segment */
#define VIDEO_MEMORY_BASE 0xa000
#define SCREEN_WIDTH 320
#define PLAYFIELD_OFFSET_X 8
#define PLAYFIELD_OFFSET_Y 8

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
extern void wait_n_ticks(uint16_t ticks);
extern void swap_video_buffers(void);
extern void blit_map_playfield_offscreen(void);
extern void blit_comic_playfield_offscreen(void);
extern uint16_t camera_x;
extern uint16_t offscreen_video_buffer_ptr;
extern level_t current_level;
extern uint8_t tileset_graphics[];

/**
 * Helper functions for door rendering
 */

/* Global variable to store door blit offset for door rendering functions */
static uint16_t door_blit_offset = 0;

/**
 * blank_door_offscreen - Black out a 32×32 pixel door region
 * 
 * Fills the door area with black pixels in all 4 EGA planes.
 */
static void blank_door_offscreen(void)
{
    uint8_t __far *video_mem;
    uint16_t row;
    uint8_t plane;
    
    /* For each EGA plane */
    for (plane = 1; plane <= 8; plane <<= 1) {
        /* Set up EGA plane mask */
        outp(0x3C4, 0x02);  /* SC Index: Map Mask */
        outp(0x3C5, plane); /* SC Data: plane mask */
        
        /* Point to door location in offscreen buffer */
        video_mem = MK_FP(VIDEO_MEMORY_BASE, door_blit_offset + offscreen_video_buffer_ptr);
        
        /* Fill 32 rows × 4 bytes (32 pixels wide) with zeros */
        for (row = 0; row < 32; row++) {
            video_mem[0] = 0;
            video_mem[1] = 0;
            video_mem[2] = 0;
            video_mem[3] = 0;
            video_mem += SCREEN_WIDTH / 8;  /* Next row */
        }
    }
}

/**
 * blit_door_halfopen_offscreen - Draw half-open door
 * 
 * Draws the door tiles with only half of each tile visible to create
 * a half-open effect. The left half of left tiles and right half of
 * right tiles are drawn, with the rest blanked out.
 */
static void blit_door_halfopen_offscreen(void)
{
    uint8_t door_tile_ul = current_level.door_tile_ul;
    uint8_t door_tile_ur = current_level.door_tile_ur;
    uint8_t door_tile_ll = current_level.door_tile_ll;
    uint8_t door_tile_lr = current_level.door_tile_lr;
    const uint8_t *tile_graphic;
    uint8_t __far *video_mem;
    uint16_t row;
    uint8_t plane;
    uint16_t tile_offset;
    
    /* Each tile is 128 bytes (16×16 pixels = 2 bytes/row × 16 rows × 4 planes) */
    
    /* For each EGA plane */
    for (plane = 1; plane <= 8; plane <<= 1) {
        /* Set up EGA plane mask */
        outp(0x3C4, 0x02);  /* SC Index: Map Mask */
        outp(0x3C5, plane); /* SC Data: plane mask */
        
        /* Upper-left tile: draw right half only */
        tile_graphic = &tileset_graphics[door_tile_ul * 128];
        video_mem = MK_FP(VIDEO_MEMORY_BASE, door_blit_offset + offscreen_video_buffer_ptr);
        /* Skip to the plane data for this plane (32 bytes per plane) */
        tile_offset = ((plane == 1) ? 0 : (plane == 2) ? 32 : (plane == 4) ? 64 : 96);
        for (row = 0; row < 16; row++) {
            video_mem[0] = tile_graphic[tile_offset + row * 2 + 1];  /* Right byte */
            video_mem += SCREEN_WIDTH / 8;
        }
        
        /* Upper-right tile: draw left half only */
        tile_graphic = &tileset_graphics[door_tile_ur * 128];
        video_mem = MK_FP(VIDEO_MEMORY_BASE, door_blit_offset + offscreen_video_buffer_ptr + 3);
        for (row = 0; row < 16; row++) {
            video_mem[0] = tile_graphic[tile_offset + row * 2];  /* Left byte */
            video_mem += SCREEN_WIDTH / 8;
        }
        
        /* Lower-left tile: draw right half only */
        tile_graphic = &tileset_graphics[door_tile_ll * 128];
        video_mem = MK_FP(VIDEO_MEMORY_BASE, door_blit_offset + offscreen_video_buffer_ptr + 16 * (SCREEN_WIDTH / 8));
        for (row = 0; row < 16; row++) {
            video_mem[0] = tile_graphic[tile_offset + row * 2 + 1];  /* Right byte */
            video_mem += SCREEN_WIDTH / 8;
        }
        
        /* Lower-right tile: draw left half only */
        tile_graphic = &tileset_graphics[door_tile_lr * 128];
        video_mem = MK_FP(VIDEO_MEMORY_BASE, door_blit_offset + offscreen_video_buffer_ptr + 16 * (SCREEN_WIDTH / 8) + 3);
        for (row = 0; row < 16; row++) {
            video_mem[0] = tile_graphic[tile_offset + row * 2];  /* Left byte */
            video_mem += SCREEN_WIDTH / 8;
        }
    }
}

/**
 * wait_1_tick_and_swap - Helper to wait one tick and swap buffers
 */
static void wait_1_tick_and_swap(void)
{
    wait_n_ticks(1);
    swap_video_buffers();
}

/**
 * enter_door - Play door entry animation
 * 
 * In the original assembly, this plays a 4-frame animation:
 * 1. Door halfway open (Comic in front)
 * 2. Door fully open
 * 3. Door halfway closed (Comic behind door)
 * 4. Door fully closed
 */
static void enter_door(uint8_t door_x, uint8_t door_y)
{
    int16_t camera_relative_x;
    uint16_t pixel_offset;
    
    /* Calculate camera-relative position and pixel offset for door */
    camera_relative_x = door_x - camera_x;
    
    /* Convert game coordinates to pixel offset in video memory
     * door_y and camera_relative_x are in game units (8 pixels each)
     * Pixel offset = (door_y * 8 * SCREEN_WIDTH) + (camera_relative_x * 8)
     * In planar mode, divide by 8: (door_y * SCREEN_WIDTH) + camera_relative_x */
    pixel_offset = (PLAYFIELD_OFFSET_Y + door_y * 8) * (SCREEN_WIDTH / 8) + 
                   (PLAYFIELD_OFFSET_X / 8) + camera_relative_x;
    door_blit_offset = pixel_offset;
    
    /* Play door sound */
    play_sound(SOUND_DOOR, 4);
    
    /* Frame 1: Door halfway open (Comic in front) */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_door_halfopen_offscreen();
    blit_comic_playfield_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 2: Door fully open */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_comic_playfield_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 3: Door halfway closed (Comic behind door) */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_comic_playfield_offscreen();
    blit_door_halfopen_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 4: Door fully closed */
    blit_map_playfield_offscreen();
    wait_1_tick_and_swap();
}

/**
 * exit_door_animation - Play door exit animation
 * 
 * In the original assembly, this plays a 5-frame animation:
 * 1. Door fully closed (with delay)
 * 2. Door halfway open (Comic behind door)
 * 3. Door fully open
 * 4. Door halfway closed (Comic in front)
 * 5. Door fully closed
 * 
 * Public wrapper for the exit animation, called from load_new_stage.
 */
void exit_door_animation(void)
{
    int16_t camera_relative_x;
    uint16_t pixel_offset;
    
    /* Calculate camera-relative position and pixel offset for door
     * Comic is at door position (having just arrived through it)
     * door.x = comic_x - 1 (both in game units) */
    camera_relative_x = comic_x - 1 - camera_x;
    
    /* Convert game coordinates to pixel offset in video memory
     * comic_x and comic_y are in game units (8 pixels each) */
    pixel_offset = (PLAYFIELD_OFFSET_Y + comic_y * 8) * (SCREEN_WIDTH / 8) + 
                   (PLAYFIELD_OFFSET_X / 8) + camera_relative_x;
    door_blit_offset = pixel_offset;
    
    /* Initial delay */
    wait_n_ticks(3);
    
    /* Play door sound */
    play_sound(SOUND_DOOR, 4);
    
    /* Frame 1: Door fully closed */
    blit_map_playfield_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 2: Door halfway open (Comic behind door) */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_comic_playfield_offscreen();
    blit_door_halfopen_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 3: Door fully open */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_comic_playfield_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 4: Door halfway closed (Comic in front) */
    blit_map_playfield_offscreen();
    blank_door_offscreen();
    blit_door_halfopen_offscreen();
    blit_comic_playfield_offscreen();
    wait_1_tick_and_swap();
    
    /* Frame 5: Door fully closed */
    blit_map_playfield_offscreen();
    blit_comic_playfield_offscreen();
    wait_1_tick_and_swap();
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
    
    /* Validate stage number is within bounds (0-2) */
    if (current_stage_number >= 3) {
        return 0;  /* Invalid stage number */
    }
    
    /* Check all doors in the current stage */
    for (i = 0; i < 3; i++) {  /* MAX_NUM_DOORS */
        door = &current_level_ptr->stages[current_stage_number].doors[i];
        
        /* Door is unused if x or y is 0xff */
        if (door->x == DOOR_UNUSED || door->y == DOOR_UNUSED) {
            continue;
        }
        
        /* Check Y coordinate: must be exact match
         * Both comic_y and door.y are in game units (same coordinate system) */
        if (comic_y != door->y) {
            continue;
        }
        
        /* Check X coordinate: must be within 3 units
         * Both comic_x and door.x are in game units (same coordinate system)
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
