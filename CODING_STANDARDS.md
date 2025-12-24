# Captain Comic C Coding Standards

## Overview

This document defines coding standards for C code in the Captain Comic refactoring project. These standards ensure consistency, maintainability, and compatibility with the 16-bit DOS environment.

---

## General Principles

### 1. Compatibility First
- Code must be pixel-perfect compatible with original assembly
- Timing must match original (validated via DOSBox CPU cycles)
- Behavior changes require explicit justification

### 2. Clarity Over Cleverness
- Write readable code that clearly expresses intent
- Document non-obvious logic and assumptions
- Preserve assembly logic documentation in wrapper comments

### 3. DOS 16-bit Constraints
- Small memory model (64KB code, 64KB data)
- Near pointers by default (2 bytes)
- Far pointers only for video memory (segment 0xA000)
- No dynamic memory allocation (use static arrays)
- No standard library functions that assume 32-bit or modern OS

---

## File Organization

### Directory Structure
```
src/
  collision.c      - Collision detection functions
  items.c          - Item collection logic
  geometry.c       - Coordinate math
  loading.c        - Level/stage loading
  physics.c        - Player physics
  enemy.c          - Enemy AI

include/
  abi_conventions.h    - C/assembly interface
  structures.h         - Data structure definitions
  collision.h          - Collision function prototypes
  items.h              - Item function prototypes
  geometry.h           - Geometry function prototypes
  loading.h            - Loading function prototypes
  physics.h            - Physics function prototypes
  enemy.h              - Enemy function prototypes
```

### Header Guards
```c
#ifndef COLLISION_H
#define COLLISION_H

// ... content ...

#endif /* COLLISION_H */
```

### Include Order
1. System headers (if any)
2. Project headers (fine-grained, only what's needed)
3. Module's own header last

```c
// collision.c
#include "structures.h"    // For enemy, tile structs
#include "geometry.h"      // For coordinate functions
#include "collision.h"     // Own header
```

---

## Naming Conventions

### Functions
- **Lowercase with underscores**: `check_collision`, `load_level`
- **Verb-noun format**: `get_tile_id`, `update_enemy_position`
- **Assembly callable functions**: Watcom adds `_` prefix automatically
  ```c
  // C: void init_graphics(void)
  // Assembly: extern _init_graphics
  ```

### Variables
- **Lowercase with underscores**: `player_x`, `enemy_count`
- **Descriptive names**: avoid single letters except loop counters
- **Hungarian notation**: avoid (not needed for clarity)

### Constants
- **Uppercase with underscores**: `MAP_WIDTH`, `MAX_ENEMIES`
- **Use `#define` or `enum`**:
  ```c
  #define MAX_ENEMIES 25
  #define MAP_WIDTH_TILES 128
  
  enum ItemType {
      ITEM_CORKSCREW = 0,
      ITEM_DOORKEY = 1,
      ITEM_BOOTS = 2
  };
  ```

### Structures
- **Lowercase with underscores**: `struct enemy`, `struct fireball`
- **Typedef only if it improves clarity**:
  ```c
  typedef struct enemy {
      unsigned char x;
      unsigned char y;
      unsigned char vel_x;
      unsigned char vel_y;
  } Enemy;
  ```

---

## Type Usage

### Standard Types
- **Use exact-width types** from `<stdint.h>` where precision matters:
  ```c
  uint8_t tile_id;      // 8-bit unsigned (0-255)
  int16_t velocity;     // 16-bit signed (-32768 to 32767)
  uint16_t address;     // 16-bit unsigned (0-65535)
  ```

### DOS Types
- **Segment:offset pointers**:
  ```c
  unsigned char __far *video_memory;  // Far pointer (4 bytes)
  unsigned int *tile_buffer;          // Near pointer (2 bytes)
  ```

### Avoid
- `long` (ambiguous size in 16-bit)
- `float`, `double` (no FPU on 8086, slow emulation)
- `bool` (use `int` with 0/1)

---

## Function Declarations

### Calling Convention
- **Default**: `__watcall` (implicit, no annotation needed)
- **Explicit for assembly compatibility**:
  ```c
  // Function called from assembly
  int __watcall check_collision(unsigned char x, unsigned char y);
  
  // Function using explicit registers
  #pragma aux get_tile parm [ax] [bx] value [ax];
  unsigned char get_tile(unsigned char x, unsigned char y);
  ```

### Error Handling
- **Return 0 for success, non-zero for error** (matches AX register convention)
  ```c
  int load_level(unsigned char level_num) {
      if (level_num >= MAX_LEVELS) {
          return 1;  // Error: invalid level
      }
      // ... load level ...
      return 0;  // Success
  }
  ```

### Parameter Passing
- **By value for small types** (char, int, pointers)
- **By pointer for structs** (avoid copying large structures)
  ```c
  // Good
  void update_enemy(Enemy *enemy);
  
  // Bad (copies entire struct on stack)
  void update_enemy(Enemy enemy);
  ```

---

## Code Style

### Indentation
- **4 spaces** (no tabs)
- **Braces on same line** for functions, control structures:
  ```c
  int check_collision(unsigned char x, unsigned char y) {
      if (x >= MAP_WIDTH) {
          return 1;
      }
      return 0;
  }
  ```

### Line Length
- **Maximum 100 characters**
- Break long lines at logical points:
  ```c
  unsigned int tile_address = MAP_BUFFER_BASE +
                              (y * MAP_WIDTH_TILES) +
                              x;
  ```

### Whitespace
- **Space after keywords**: `if (`, `while (`, `for (`
- **No space for function calls**: `check_collision(x, y)`
- **Space around operators**: `x + y`, `a == b`
- **One blank line between functions**

### Comments
- **Explain WHY, not WHAT**:
  ```c
  // Good
  // EGA requires 4 plane writes for full color update
  write_ega_pixel(x, y, color);
  
  // Bad
  // Write pixel to screen
  write_ega_pixel(x, y, color);
  ```

- **Document assembly wrapper logic**:
  ```nasm
  ; Original assembly logic:
  ; 1. Load player coordinates into AX/BX
  ; 2. Calculate tile address: (y * 128) + x
  ; 3. Check tile ID against collision table
  ; 4. Return 1 if solid, 0 if passable
  ; Registers: AX=x, BX=y, returns AX=result
  ; Replaced: 2025-12-23 - Pure coordinate math, no side effects
  ```

---

## Memory Management

### Static Allocation Only
- **No malloc/free**: DOS has limited heap
- **Use static arrays**:
  ```c
  static Enemy enemies[MAX_ENEMIES];
  static unsigned char tile_buffer[MAP_WIDTH_TILES * MAP_HEIGHT_TILES];
  ```

### Segment Awareness
- **Keep data within 64KB segments**
- **Use far pointers for video memory**:
  ```c
  unsigned char __far *video_ram = (unsigned char __far *)0xA0000000;
  ```

### Stack Usage
- **Minimize local variables** (small stack, ~2KB)
- **Avoid large local arrays**:
  ```c
  // Bad
  void process_level(void) {
      unsigned char buffer[10000];  // Too large for stack!
  }
  
  // Good
  static unsigned char buffer[10000];
  void process_level(void) {
      // Use static buffer
  }
  ```

---

## Assembly Integration

### Calling C from Assembly
```nasm
; Assembly code
extern _check_collision

check_collision_wrapper:
    mov ax, [player_x]    ; First param in AX (watcall)
    mov dx, [player_y]    ; Second param in DX
    call _check_collision ; Call C function
    ; Result in AX (0=no collision, 1=collision)
    ret
```

### Calling Assembly from C
```c
// C header
extern void __watcall set_video_mode(unsigned char mode);

// C code
set_video_mode(0x0D);  // Call assembly function
```

### Register Preservation
- **C functions automatically preserve**: BP, SI, DI, DS, SS
- **C functions may clobber**: AX, BX, CX, DX, ES, FLAGS
- **Document if non-standard**:
  ```c
  // WARNING: This function clobbers ES register
  void copy_to_video(unsigned char *src, unsigned int len);
  ```

---

## Performance Considerations

### Optimization Level
- **Use `-O0` (no optimization) initially** for determinism
- **Profile before optimizing**: use DOSBox CPU cycle counter
- **Optimize only hot paths** identified by profiling

### Avoid Expensive Operations
- **Division/Modulo**: Use bit shifts for power-of-2
  ```c
  // Good
  unsigned int offset = y << 7;  // y * 128 (2^7)
  
  // Bad
  unsigned int offset = y * 128;  // May use slow multiply
  ```

- **Function Calls**: Inline small, frequently called functions
  ```c
  static inline unsigned int tile_offset(unsigned char x, unsigned char y) {
      return (y << 7) + x;
  }
  ```

### Assembly for Critical Paths
- **Keep performance-critical code in assembly**:
  - EGA blitting (plane manipulation)
  - Interrupt handlers (timing-sensitive)
  - Inner loops with rep instructions

---

## Testing and Validation

### Every Function Must
1. **Match assembly behavior exactly** (validated via golden master)
2. **Document preconditions** (valid input ranges)
3. **Document postconditions** (expected results)
4. **Handle edge cases** (boundary conditions)

### Example
```c
/**
 * Calculate tile address from coordinates.
 * 
 * @param x Tile X coordinate (0-127)
 * @param y Tile Y coordinate (0-9)
 * @return Linear tile index (0-1279)
 * 
 * Preconditions:
 *   - x < MAP_WIDTH_TILES (128)
 *   - y < MAP_HEIGHT_TILES (10)
 * 
 * Postconditions:
 *   - Returns value in range [0, 1279]
 *   - Matches original assembly: (y * 128) + x
 */
unsigned int tile_address(unsigned char x, unsigned char y) {
    return (y << 7) + x;  // y * 128 + x
}
```

### Commit Checklist
- [ ] All 5 test clips pass pixel-perfect
- [ ] DOSBox reports identical CPU cycles
- [ ] No visual diffs remain
- [ ] Assembly wrapper documents original logic
- [ ] Code follows style guide
- [ ] Function headers document behavior

---

## Common Patterns

### Tile Coordinate Conversion
```c
// Tile coordinates to linear index
unsigned int tile_index = (tile_y * MAP_WIDTH_TILES) + tile_x;

// Pixel coordinates to tile coordinates
unsigned char tile_x = pixel_x / TILE_WIDTH;
unsigned char tile_y = pixel_y / TILE_HEIGHT;
```

### Collision Detection
```c
int check_tile_collision(unsigned char x, unsigned char y) {
    unsigned int index = tile_address(x, y);
    unsigned char tile_id = tile_map[index];
    return (tile_id >= FIRST_SOLID_TILE);
}
```

### Sprite Animation
```c
void update_sprite_frame(Enemy *enemy) {
    enemy->animation_counter++;
    if (enemy->animation_counter >= ANIMATION_DELAY) {
        enemy->animation_counter = 0;
        enemy->animation_frame++;
        if (enemy->animation_frame >= enemy->num_frames) {
            enemy->animation_frame = 0;
        }
    }
}
```

---

## Tools and Build

### Compiler Flags
```makefile
WCC = wcc
WCCFLAGS = -ml -s -0 -O0
# -ml = small memory model
# -s  = no stack checking
# -0  = 8086 target
# -O0 = no optimization
```

### Dependency Tracking
```makefile
# Watcom generates .d files for dependencies
%.obj: %.c
	$(WCC) $(WCCFLAGS) -d$*.d -fo=$@ $<

-include $(OBJS:.obj=.d)
```

### Linking
```makefile
comic.exe: $(OBJS)
	djlink -o $@ $^
```

---

## Questions and Exceptions

### When to Break Rules
- **Performance critical code**: Document why assembly is needed
- **Hardware access**: May require non-standard patterns
- **Timing sensitive code**: Explain timing constraints

### Getting Help
- Consult `reference/disassembly/R5sw1991.asm` for original logic
- Check assembly wrapper comments for documented behavior
- Run validation tests to verify compatibility
- Use DOSBox debugger to compare execution

---

## References

- Open Watcom C Compiler Documentation
- Captain Comic Assembly Source: `reference/disassembly/R5sw1991.asm`
- ABI Conventions: `include/abi_conventions.h`
- Refactoring Plan: `REFACTORING_PLAN.md`
