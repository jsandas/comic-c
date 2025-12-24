# Scenario 1: Collision Detection

## Purpose
Validate that tile collision detection logic works identically in C as in the original assembly code.

## Test Functions
- `check_vertical_map_collision`
- `check_horizontal_map_collision`
- `address_of_tile_at_coordinates`

## Setup

### Required Save States
- `tests/savestates/1.sav` - Player at start of Level 1
- `tests/savestates/2.sav` - Player near wall
- `tests/savestates/3.sav` - Player at platform edge

### How to Create Save States (Reference)
1. Launch DOSBox-X with deterministic config:
   ```bash
   cd /Users/hedge/workspace/comic-c
   dosbox-x -conf tests/dosbox_deterministic.conf
   ```
2. Mount and run the game:
   ```
   mount c reference/orig/R5sw1991
   c:
   comic.exe
   ```
3. Start new game (press Enter at title screen)
4. When player spawns in Level 1, immediately press `F12+S` to save state
5. Name it: `level1_start`
6. Copy from DOSBox-X save directory to `tests/savestates/`

## Test Cases

### Test 1.1: Floor Collision (Player Standing)
**Expected Behavior:** Player should not fall through solid floor tiles

**Steps:**
1. Load `1.sav (level1_start)` in original COMIC.EXE
2. Note player Y position (compare screen capture `capture/level1_start.png`)
3. Wait 2 seconds (player should remain at same Y position)
4. Record result: "Player Y position: [value], not falling"

**Validation:**
1. Load same save state in C version
2. Wait 2 seconds
3. Compare: Player Y position should match original
4. **PASS** if Y positions match, **FAIL** if player falls or moves

### Test 1.2: Wall Collision (Horizontal Movement)
**Expected Behavior:** Player cannot walk through walls

**Steps:**
1. Load `2.sav (level1_wall)`
2. Hold RIGHT arrow key for 1 second
3. Note player X position when hitting wall (compare screen capture `capture/level1_wall.png`)
4. Record result: "Player X position: [value] at wall"

**Validation:**
1. Load same save state in C version
2. Hold RIGHT arrow key for 1 second
3. Compare: Player X position should match original
4. **PASS** if positions match, **FAIL** if player walks through wall

### Test 1.3: Platform Edge Detection
**Expected Behavior:** Player should fall when walking off platform edge

**Steps:**
1. Load `3.sav (level1_edge)`
2. Walk slowly towards edge
3. Note exact X position where player starts falling (compare screen capture `capture/level1_edge.png` and `capture/level1_drop.png`)
4. Record result: "Player falls at X: [value]"

**Validation:**
1. Load same save state in C version
2. Walk to same edge
3. Compare: Fall should start at same X position
4. **PASS** if fall positions match, **FAIL** if different

### Test 1.4: Ceiling Collision (Jump)
**Expected Behavior:** Player jump should be blocked by ceiling tiles

**Steps:**
1. Load `4.sav (level1_ceiling)`
2. Press and hold UP arrow (jump)
3. Note maximum Y position reached (compare screen capture `capture/level1_ceiling.png`)
4. Record result: "Player max Y: [value], stopped by ceiling"

**Validation:**
1. Load same save state in C version
2. Perform same jump in same location
3. Compare: Maximum Y position should match
4. **PASS** if max Y matches, **FAIL** if player goes through ceiling

## Validation Checklist

- [ ] Test 1.1: Floor collision - PASS/FAIL
- [ ] Test 1.2: Wall collision - PASS/FAIL
- [ ] Test 1.3: Platform edge - PASS/FAIL
- [ ] Test 1.4: Ceiling collision - PASS/FAIL

## Recording Results

Document results in `tests/validation_log.txt`:

```
=== Scenario 1: Collision Detection ===
Date: 2025-12-23
Tester: [Your Name]

Test 1.1 - Floor Collision
  Assembly: Player Y = 120 (not falling)
  C Version: Player Y = 120 (not falling)
  Status: PASS

Test 1.2 - Wall Collision
  Assembly: Player X = 45 (blocked)
  C Version: Player X = 45 (blocked)
  Status: PASS

Test 1.3 - Platform Edge
  Assembly: Falls at X = 80
  C Version: Falls at X = 80
  Status: PASS

Test 1.4 - Ceiling Collision
  Assembly: Player max Y = 90
  C Version: Player max Y = 90
  Status: PASS

Overall: 4/4 PASS
```

## Debugging Tips

If a test fails:

1. **Enable DOSBox debugger:**
   - Press `Alt+Pause` to enter debugger
   - View memory at player position variables
   - Step through assembly code

2. **Check C implementation:**
   - Add printf debugging (if DOSBox allows)
   - Compare algorithm logic with assembly
   - Verify coordinate math

3. **Inspect assembly wrapper:**
   - Ensure parameters passed correctly
   - Check return value handling
   - Verify calling convention

## Notes

- Collision detection is pure logic (Tier 1 function)
- Should be deterministic given same inputs
- No randomness or timing dependencies
- Good starting point for C conversion
