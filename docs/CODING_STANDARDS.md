# Coding Standards for Captain Comic C Refactor

## Overview

This document defines the C coding conventions for the Captain Comic refactor project. The goal is readable, maintainable code that matches the original assembly behavior exactly.

## General Principles

1. **Clarity over cleverness** - Prefer explicit, readable code
2. **Bug-for-bug compatibility** - Preserve original behavior, even bugs
3. **Document DOS-specific assumptions** - We're in 16-bit real-mode
4. **Test everything** - No change is too small to test

## File Organization

### Header Files

- Use include guards: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`
- Order: system includes → local includes → types → constants → declarations
- All assembly globals/functions need `#pragma aux "*"` to prevent name mangling

Example:
```c
#ifndef GLOBALS_H
#define GLOBALS_H

/* System includes */
#include <stdint.h>

/* Prevent name mangling */
#pragma aux comic_x "*"

/* Global declarations */
extern uint8_t comic_x;

#endif /* GLOBALS_H */
```

### Source Files

- Start with header comment explaining file purpose
- Include own header first, then dependencies
- Group related functions together with section comments

Example:
```c
/*
 * collision.c - Collision detection routines
 * 
 * Converted from R5sw1991.asm lines 5500-5800
 */

#include "collision.h"
#include "globals.h"

/* Helper functions */
static uint16_t calculate_tile_offset(uint8_t x, uint8_t y);

/* Public functions */
int check_collision(uint8_t x, uint8_t y);
```

## Naming Conventions

### Variables

- **Global variables**: Match assembly names exactly (e.g., `comic_x`, `comic_y`)
- **Local variables**: Descriptive snake_case (e.g., `tile_offset`, `is_solid`)
- **Constants**: SCREAMING_SNAKE_CASE (e.g., `MAP_WIDTH_TILES`)
- **Function parameters**: Descriptive snake_case

### Functions

- **Assembly functions**: Match assembly labels (e.g., `load_new_level`, `swap_video_buffers`)
- **C functions**: Descriptive snake_case (e.g., `calculate_tile_offset`, `is_tile_solid`)
- **Static helpers**: Prefix with `static` keyword

### Types

- Use explicit integer types: `uint8_t`, `uint16_t`, `int8_t`, `int16_t`
- **Never use `int`** - size is ambiguous in 16-bit DOS (it's 16-bit)
- Struct types: `typedef struct { ... } name_t;` with `_t` suffix

Example:
```c
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t facing;
} player_state_t;
```

## Code Formatting

### Indentation

- **4 spaces** per level (no tabs)
- Align related elements vertically when it improves readability

### Braces

- **K&R style** for functions: opening brace on same line
- Always use braces for control structures, even single statements

```c
void function_name(uint8_t param)
{
    if (condition) {
        do_something();
    } else {
        do_something_else();
    }
    
    for (i = 0; i < count; i++) {
        process(i);
    }
}
```

### Line Length

- Prefer **80 characters** max for readability
- Break long lines at logical points (after operators, commas)

### Spacing

- Space after keywords: `if (`, `while (`, `for (`
- No space for function calls: `function(arg)`
- Space around binary operators: `a + b`, `x == y`
- No space around unary operators: `!flag`, `~mask`, `*ptr`

## Comments

### File Headers

```c
/*
 * filename.c - Brief description
 * 
 * Longer explanation if needed.
 * Reference to original assembly: R5sw1991.asm lines XXXX-YYYY
 */
```

### Function Comments

```c
/*
 * Brief one-line summary
 * 
 * Longer explanation of what the function does, edge cases,
 * and any DOS-specific assumptions.
 * 
 * Parameters:
 *   x - Description of x
 *   y - Description of y
 * 
 * Returns:
 *   0 on success, -1 on error
 * 
 * Assembly reference: R5sw1991.asm:5500 (check_vertical_enemy_map_collision)
 */
int check_collision(uint8_t x, uint8_t y)
{
    /* Implementation */
}
```

### Inline Comments

- Use `/* */` for block comments
- Use `//` for end-of-line comments (Open Watcom C supports C++ style)
- Explain **why**, not **what** (code shows what)
- Document deviations from expected behavior or bug-for-bug compatibility

```c
/* Bug-for-bug compatibility: Original assembly incorrectly uses SUB instead of ADD
 * See R5sw1991.asm:5632 - joystick_x_low calculation bug */
threshold = (zero_value - extreme_value) / 2;  // Wrong but intentional!
```

## DOS-Specific Considerations

### Memory Model

- We use **large model** (`-ml` flag)
- Code and data in separate segments
- Far pointers for code, far pointers for data
- DS register always points to data segment (set by assembly at startup)

### No Standard Library Assumptions

- **Avoid `malloc`/`free`** - use static allocation or assembly-managed memory
- **Avoid `printf`** - use direct video memory manipulation or assembly functions
- **File I/O** - can use `fopen`/`fread`/`fclose` (Open Watcom provides DOS versions)
- **String functions** - can use `strcmp`, `strcpy`, etc. (they work in DOS)

### Video Memory Access

- Video memory at segment `0xa000`
- Use inline assembly or assembly functions for direct video access
- Never cache video memory pointers across function calls

Example:
```c
/* Write directly to video memory */
void write_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    uint16_t offset = (y * 320 + x) / 8;
    /* Use assembly function or inline asm */
    write_video_byte(offset, color);  // Calls assembly
}
```

### Inline Assembly

- Use Open Watcom inline assembly syntax when needed:
```c
void set_video_segment(uint16_t segment)
{
    __asm {
        mov ax, segment
        mov es, ax
    }
}
```

## Type Definitions

### Standard Types

```c
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
```

### Game-Specific Types

```c
/* Tile coordinate (game units, not pixels) */
typedef struct {
    uint8_t x;
    uint8_t y;
} tile_pos_t;

/* Velocity in fixed-point (1/8 game unit per tick) */
typedef int8_t velocity_t;
```

## Error Handling

- **Return codes**: 0 = success, -1 = error (Unix convention)
- **Booleans**: `0 = false`, `non-zero = true` (C89 convention)
- **NULL checks**: Always validate pointers before dereferencing

```c
int load_file(const char* filename, void* buffer)
{
    if (!filename || !buffer) {
        return -1;  // Invalid parameters
    }
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return -1;  // File not found
    }
    
    /* ... */
    
    fclose(fp);
    return 0;  // Success
}
```

## Assembly Integration

### Declaring Assembly Functions

```c
/* In header file */
#pragma aux assembly_function "*"
extern void assembly_function(uint8_t param);

/* In C code */
void call_assembly(void)
{
    assembly_function(42);  // Calls assembly directly
}
```

### Accessing Assembly Globals

```c
/* In header file */
#pragma aux comic_x "*"
extern uint8_t comic_x;

/* In C code */
void move_player(void)
{
    comic_x += 1;  // Modifies assembly global
}
```

### Calling C from Assembly

```asm
; In assembly file
extern _c_function

; Call C function
push ax          ; Parameter (right to left)
call _c_function
add sp, 2        ; Clean up stack (cdecl convention)
```

## Testing Requirements

### Function Testing

- Every converted function must have a test scenario
- Document test in `tests/scenarios/`
- Create save state if needed
- Verify with DOSBox-X

### Documentation

- Mark conversions with assembly line references:
```c
/* Converted from R5sw1991.asm:5500-5550 (check_vertical_enemy_map_collision) */
```

- Document any behavioral changes or uncertainties:
```c
/* TODO: Verify this matches assembly behavior at R5sw1991.asm:5532
 * The original code seems to have an off-by-one bug here */
```

## Example: Complete Function

```c
/*
 * check_vertical_collision - Check if player would collide vertically
 * 
 * Checks the map tile at player's position to determine if vertical
 * movement would result in a collision with a solid tile.
 * 
 * Parameters:
 *   x - X coordinate in game units
 *   y - Y coordinate in game units
 * 
 * Returns:
 *   1 if collision (solid tile), 0 if passable
 * 
 * Assembly reference: R5sw1991.asm:5500-5550 (check_vertical_enemy_map_collision)
 */
int check_vertical_collision(uint8_t x, uint8_t y)
{
    uint16_t tile_offset;
    uint8_t tile_id;
    
    /* Calculate tile offset from coordinates */
    /* Each tile is 16x16 pixels = 2 game units */
    tile_offset = (y / 2) * MAP_WIDTH_TILES + (x / 2);
    
    /* Safety check: ensure offset is within map bounds */
    if (tile_offset >= MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
        return 1;  // Out of bounds = solid
    }
    
    /* Read tile ID from map */
    tile_id = current_tiles_ptr[tile_offset];
    
    /* Compare to passable threshold */
    /* Tiles with ID > tileset_last_passable are solid */
    if (tile_id > tileset_last_passable) {
        return 1;  // Solid tile
    }
    
    return 0;  // Passable tile
}
```

## When In Doubt

1. Check the original assembly for exact behavior
2. Prefer explicit over implicit
3. Document unusual patterns
4. Test thoroughly
5. Ask for review if uncertain

## Tools and Validation

- **Compiler**: `wcc -ml -s -0` (large model, no stack checks, 8086 target)
- **Warnings**: Enable all warnings, fix them
- **Static analysis**: Use Open Watcom's built-in checks
- **Testing**: DOSBox-X with save states for validation
