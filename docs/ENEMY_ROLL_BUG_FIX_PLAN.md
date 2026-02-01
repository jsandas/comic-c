# Enemy Behavior Roll - Bug Fix Plan

## Summary
The C implementation of `enemy_behavior_roll` has several bugs compared to the assembly implementation. This document outlines the issues and the plan to fix them.

## Bugs Identified

### 1. Falling State Y Position Increment (CRITICAL)
**Problem:** In the falling state, the C code increments `enemy->y` immediately and persists it even when not despawning.

**Assembly behavior:**
```assembly
.falling:
    mov ax, [si + enemy.y]      ; Load current position
    inc al                       ; Increment Y in register only
    cmp al, PLAYFIELD_HEIGHT - 2 - 1
    jl .goto_horizontal          ; If not despawning, jump without storing
    ; Only stores Y if despawning:
    mov al, PLAYFIELD_HEIGHT - 2
    mov word [si + enemy.y], ax
```

**C code (WRONG):**
```c
if (enemy->y_vel > 0) {
    enemy->y += 1;  // ❌ Stored immediately!
    if (enemy->y >= PLAYFIELD_HEIGHT - 2 - 1) {
        enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
        enemy->y = PLAYFIELD_HEIGHT - 2;
        return;
    }
    return;  // ❌ Y was modified but shouldn't have been!
}
```

**Fix:** Don't modify `enemy->y` in the falling check - just check if falling and return early if not despawning. The position update happens later in the function.

---

### 2. Ground Landing Y Clamping (CRITICAL)
**Problem:** The condition for clamping Y to even boundary is inverted.

**Assembly behavior:**
```assembly
.landed:
    sub al, 3                    ; Undo the +3 from collision check
    cmp byte [si + enemy.y_vel], 0  ; Were we falling?
    je .done                     ; If NOT falling (y_vel == 0), skip clamping
    inc al
    and al, 0xfe                 ; Clamp to even boundary ONLY if was falling
    mov byte [si + enemy.y_vel], 0
```

**C code (WRONG):**
```c
if (enemy->y_vel == 0) {  // ❌ Inverted condition!
    enemy->y = (uint8_t)(enemy->y & 0xFE);
}
enemy->y_vel = 0;
```

**Fix:** Change condition to `if (enemy->y_vel != 0)` to clamp only when landing from a fall.

---

### 3. Missing Width-Aware Collision Detection (HIGH PRIORITY)
**Problem:** The C code uses simple `get_tile_at()` which doesn't check adjacent tiles when the enemy is at odd coordinates.

**Assembly behavior:**
- `check_horizontal_enemy_map_collision`: Checks tile at (x, y) and also (x, y+1) if Y is odd
- `check_vertical_enemy_map_collision`: Checks tile at (x, y) and also (x+1, y) if X is odd

**Why this matters:** Enemies are 2 units wide and 2 units tall. When positioned at odd coordinates, they span two tiles in one direction.

**C code (INCOMPLETE):**
```c
tile = get_tile_at(next_x, enemy->y);  // ❌ Only checks one tile!
```

**Fix:** Implement the two collision check functions from assembly.

---

### 4. Spurious Facing Update (MINOR)
**Problem:** C code updates `enemy->facing` at the end, but the assembly does not set or check facing in this function.

**C code (EXTRA):**
```c
enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
```

**Fix:** Remove this line to match assembly behavior (facing is managed elsewhere or not needed for roll behavior).

---

## Implementation Plan

### Phase 1: Create Collision Detection Functions

#### Function 1: `check_vertical_enemy_map_collision`
**Purpose:** Check for solid tiles when moving vertically (up/down)  
**Logic:**
- Check tile at (x, y)
- If X coordinate is odd, also check tile at (x+1, y)
- Return `true` if any checked tile is solid

**Signature:**
```c
static bool check_vertical_enemy_map_collision(uint8_t x, uint8_t y);
```

**Implementation details:**
```c
static bool check_vertical_enemy_map_collision(uint8_t x, uint8_t y)
{
    uint8_t tile_x = x / 2;
    uint8_t tile_y = y / 2;
    uint16_t tile_offset = tile_y * MAP_WIDTH_TILES + tile_x;
    uint8_t tile_id = current_tiles_ptr[tile_offset];
    
    /* Check primary tile */
    if (tile_id > tileset_last_passable) {
        return true;  /* Solid */
    }
    
    /* If X is odd, check tile to the right */
    if (x & 1) {
        tile_id = current_tiles_ptr[tile_offset + 1];
        if (tile_id > tileset_last_passable) {
            return true;  /* Solid */
        }
    }
    
    return false;  /* Not solid */
}
```

#### Function 2: `check_horizontal_enemy_map_collision`
**Purpose:** Check for solid tiles when moving horizontally (left/right)  
**Logic:**
- Check tile at (x, y)
- If Y coordinate is odd, also check tile at (x, y+1)
- Return `true` if any checked tile is solid

**Signature:**
```c
static bool check_horizontal_enemy_map_collision(uint8_t x, uint8_t y);
```

**Implementation details:**
```c
static bool check_horizontal_enemy_map_collision(uint8_t x, uint8_t y)
{
    uint8_t tile_x = x / 2;
    uint8_t tile_y = y / 2;
    uint16_t tile_offset = tile_y * MAP_WIDTH_TILES + tile_x;
    uint8_t tile_id = current_tiles_ptr[tile_offset];
    
    /* Check primary tile */
    if (tile_id > tileset_last_passable) {
        return true;  /* Solid */
    }
    
    /* If Y is odd, check tile below */
    if (y & 1) {
        tile_id = current_tiles_ptr[tile_offset + MAP_WIDTH_TILES];
        if (tile_id > tileset_last_passable) {
            return true;  /* Solid */
        }
    }
    
    return false;  /* Not solid */
}
```

**Required globals/constants:**
- `extern uint8_t *current_tiles_ptr;` - Pointer to current map tiles
- `extern uint8_t tileset_last_passable;` - Last passable tile ID in tileset
- `#define MAP_WIDTH_TILES 128` - Map width in tiles

---

### Phase 2: Fix `enemy_behavior_roll`

#### Change 1: Fix Falling State Logic
**Before:**
```c
if (enemy->y_vel > 0) {
    /* Falling */
    enemy->y += 1;  // ❌ Don't modify here!
    if (enemy->y >= PLAYFIELD_HEIGHT - 2 - 1) {
        enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
        enemy->y = PLAYFIELD_HEIGHT - 2;
        return;
    }
    return;  // ❌ Falls through to horizontal movement
}
```

**After:**
```c
if (enemy->y_vel > 0) {
    /* Falling - check if near bottom */
    if ((uint8_t)(enemy->y + 1) >= PLAYFIELD_HEIGHT - 2 - 1) {
        /* Despawn at bottom */
        enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
        enemy->y = PLAYFIELD_HEIGHT - 2;
        return;
    }
    /* Apply falling movement and continue to horizontal section */
    enemy->y = (uint8_t)(enemy->y + 1);
    goto horizontal_movement;
}
```

#### Change 2: Use New Collision Functions
**Before:**
```c
if (enemy->x_vel > 0) {
    next_x = (uint8_t)(enemy->x + 2);
    tile = get_tile_at(next_x, enemy->y);  // ❌ Incomplete
    if (!is_tile_solid(tile)) {
        enemy->x = (uint8_t)(enemy->x + 1);
    }
}
```

**After:**
```c
if (enemy->x_vel > 0) {
    uint8_t test_x = (uint8_t)(enemy->x + 2);
    if (!check_horizontal_enemy_map_collision(test_x, enemy->y)) {
        enemy->x = (uint8_t)(enemy->x + 1);
    }
}
```

#### Change 3: Fix Ground Clamping Logic
**Before:**
```c
if (enemy->y_vel == 0) {  // ❌ Wrong condition!
    enemy->y = (uint8_t)(enemy->y & 0xFE);
}
enemy->y_vel = 0;
```

**After:**
```c
if (enemy->y_vel != 0) {  // ✓ Only clamp if was falling
    enemy->y = ((enemy->y + 1) & 0xFE);  // Align to even boundary
}
enemy->y_vel = 0;
```

#### Change 4: Remove Facing Update
**Before:**
```c
/* Update facing */
enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
```

**After:**
```c
/* (removed) */
```

---

### Phase 3: Update Other Enemy Behaviors (Optional but Recommended)

The following behaviors should also be updated to use the new collision functions:
- `enemy_behavior_bounce` - Uses simple `get_tile_at()` calls
- `enemy_behavior_leap` - Manually checks odd coordinates, should use helpers
- `enemy_behavior_seek` - Likely has same issues
- `enemy_behavior_shy` - Likely has same issues

---

## Testing Plan

1. **Unit Testing:**
   - Test collision functions with various X/Y coordinates (even and odd)
   - Verify solid vs passable tile detection
   
2. **Integration Testing:**
   - Spawn Glow Globe (roll enemy) on various terrain types
   - Verify it rolls toward player correctly
   - Verify it falls when ground disappears
   - Verify it despawns at bottom of map
   - Verify it lands on even boundaries after falling
   
3. **Regression Testing:**
   - Test other enemy types to ensure no breakage
   - Compare behavior against DOSBox recording if available

---

## Files to Modify

1. **src/actors.c**
   - Add `check_vertical_enemy_map_collision()` function (before behavior functions)
   - Add `check_horizontal_enemy_map_collision()` function (before behavior functions)
   - Fix `enemy_behavior_roll()` function
   - Optionally update other enemy behaviors

2. **include/actors.h** (if exporting functions)
   - Declare collision functions if needed by other modules (probably not needed - keep them static)

3. **include/level_data.h** or **include/globals.h** (if needed)
   - Ensure `current_tiles_ptr` and `tileset_last_passable` are properly declared
   - Ensure `MAP_WIDTH_TILES` is defined

---

## Assembly Reference - Complete Function Flow

For reference, here's the complete flow of `enemy_behavior_roll` in assembly:

```
1. Load y_vel and check if falling (y_vel > 0)
   - If falling: increment Y in register, check bottom boundary, despawn if needed
   - If falling but not despawning: jump to horizontal section
   
2. If rolling (y_vel <= 0): Determine x_vel based on comic_x
   - Compare enemy.x to comic_x
   - Set x_vel: +1 (right), -1 (left), or 0 (aligned)
   
3. Horizontal section:
   - Check restraint (skip this tick or move)
   - If x_vel == 0: set restraint to MOVE_THIS_TICK and skip movement
   - If x_vel > 0: check collision at x+2, move to x+1 if clear
   - If x_vel < 0: check collision at x-1, move if clear
   
4. Vertical section (ground check):
   - Check tile at y+3
   - If no ground: set y_vel = 1 (start falling)
   - If ground and was falling: clamp Y to even boundary, set y_vel = 0
   - If ground and wasn't falling: just ensure y_vel = 0
   
5. Store updated position and return
```

---

## Priority Order

1. **HIGH:** Create collision functions (Phase 1)
2. **HIGH:** Fix falling Y increment bug (Phase 2, Change 1)
3. **HIGH:** Update to use collision functions (Phase 2, Change 2)
4. **HIGH:** Fix ground clamping condition (Phase 2, Change 3)
5. **LOW:** Remove facing update (Phase 2, Change 4)
6. **MEDIUM:** Update other behaviors (Phase 3)

---

## Expected Outcomes

After implementing these fixes:
- Glow Globe enemies will behave identically to the original assembly code
- Collision detection will properly account for 2-unit enemy width/height
- Enemy position updates will be consistent with assembly logic
- Code will be more maintainable with reusable collision functions
