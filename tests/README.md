# Captain Comic Functional Validation Tests

## Overview

This directory contains the validation framework for ensuring functional compatibility between the original assembly version and the C refactored version of Captain Comic. Since input recording is not available in DOSBox-X, we use functional testing focused on behavior equivalence rather than pixel-perfect frame matching.

## Functional Test Scenarios

We have 5 test scenarios ordered by complexity, focused on validating core game logic:

### Test 1: Collision Detection (Pure Logic)
- **Tests:** Tile collision detection, platform collision, boundary checking
- **Why First:** Pure math function, no side effects, easiest to verify
- **Validation:** Use DOSBox save state, test specific collision scenarios, verify correct tile detection
- **Example:** Position player at known (x,y), verify collision detection matches assembly

### Test 2: Game Initialization
- **Tests:** Level loading, player spawn position, initial state setup
- **Why Second:** Tests resource loading and state initialization
- **Validation:** Start fresh game, verify player position and health match expected values
- **Example:** Level 1 should spawn player at position (x=10, y=5)

### Test 3: Item Collection
- **Tests:** Item pickup logic, inventory updates, score changes
- **Why Third:** Tests state mutation and side effects
- **Validation:** Collect specific items, verify inventory and score updated correctly
- **Example:** Collect cola bottle, verify health increases by expected amount

### Test 4: Enemy Behavior
- **Tests:** Enemy AI, movement patterns, collision with player
- **Why Fourth:** Tests complex state machine behavior
- **Validation:** Observe enemy patterns over time, verify predictable behavior
- **Example:** Certain enemies should bounce predictably between walls

### Test 5: Level Transitions
- **Tests:** Door entry/exit, level loading, state persistence
- **Why Last:** Tests complex state transitions and resource management
- **Validation:** Enter door, verify new level loads correctly, exit and return to original level
- **Example:** Level state should be preserved when revisiting

## Directory Structure

```
tests/
├── dosbox_deterministic.conf    # DOSBox-X config for reproducible testing
├── README.md                     # This file
├── scenarios/                    # Functional test scenario documentation
│   ├── scenario_1_collision.md   # Collision detection test description
│   ├── scenario_2_init.md        # Game initialization test description
│   ├── scenario_3_items.md       # Item collection test description
│   ├── scenario_4_enemy.md       # Enemy behavior test description
│   └── scenario_5_transitions.md # Level transition test description
├── savestates/                   # DOSBox-X save states (for checkpoint testing)
│   ├── level1_start.sav          # Player spawned, ready to test
│   ├── level1_with_enemy.sav     # Enemy present for testing
│   └── level1_near_item.sav      # Near collectible item
└── validation_log.txt            # Manual testing results and notes
```

## Directory Structure

```
tests/
├── dosbox_deterministic.conf    # DOSBox-X config for reproducible testing
├── README.md                     # This file
├── compare_frames.py             # Frame comparison script (to be created)
├── Makefile                      # Validation automation (to be created)
├── golden/
│   ├── clip_1/
│   │   ├── input.rec            # Recorded keyboard inputs
│   │   └── frames/              # Extracted PNG frames (gitignored, regenerated)
│   ├── clip_2/
│   │   ├── input.rec
│   │   └── frames/
│   ├── clip_3/
│   │   ├── input.rec
│   │   └── frames/
│   ├── clip_4/
│   │   ├── input.rec
│   │   └── frames/
│   └── clip_5/
│       ├── input.rec
│       └── frames/
└── diffs/                        # Visual diff images (gitignored, generated on failure)
    ├── clip_1/
    ├── clip_2/
    ├── clip_3/
    ├── clip_4/
    └── clip_5/
```

## Recording Test Clips

### Prerequisites

1. Install DOSBox-X:
   ```bash
   brew install dosbox-x
   ```

2. Ensure original game executable exists:
   ```bash
   ls reference/orig/R5sw1991/COMIC.EXE
   ```

### Recording Procedure

For each clip (1-5):

1. **Start DOSBox-X with deterministic config:**
   ```bash
   dosbox-x -conf tests/dosbox_deterministic.conf
   ```

2. **Mount and run game:**
   ```
   mount c reference/orig/R5sw1991
   c:
   comic.exe
   ```

3. **Start input recording:**
   ```
   reckey start tests/golden/clip_1/input.rec
   ```

4. **Perform test actions** (see clip descriptions above)

5. **Stop input recording:**
   ```
   reckey stop
   ```

6. **Start video capture:**
   - Press `Ctrl+Alt+F5` (Mac: `Ctrl+Cmd+F5`)

7. **Replay recording to capture video:**
   ```
   reckey play tests/golden/clip_1/input.rec
   ```

8. **Stop video capture:**
   - Press `Ctrl+Alt+F5` again

9. **Move captured video:**
   ```bash
   mv captures/*.avi tests/golden/clip_1/capture.avi
   ```

10. **Extract frames:**
    ```bash
    cd tests/golden/clip_1
    mkdir -p frames
    ffmpeg -i capture.avi -r 60 -vcodec png frames/frame_%04d.png
    ```

11. **Commit input recording only:**
    ```bash
    git add tests/golden/clip_1/input.rec
    git commit -m "Add test clip 1: walk/jump"
    ```

Repeat for all 5 clips.

## Running Functional Tests

### Test Checklist for Each Function Conversion

When you convert an assembly function to C:

1. **Read the assembly** - Understand the algorithm and behavior
2. **Write C code** - Implement matching logic
3. **Test manually** - Run through test scenarios
4. **Compare behavior** - Verify C version matches assembly version
5. **Commit** - Mark function as converted

### Manual Testing Procedure

For each test scenario:

1. **Launch original assembly version:**
   ```bash
   dosbox-x -conf tests/dosbox_deterministic.conf
   mount c reference/orig/R5sw1991
   c:
   comic.exe
   ```

2. **Create save state at test starting point:**
   - Position game at specific scenario (e.g., near an item)
   - Press `Ctrl+Alt+F7` to save state
   - Save as `tests/savestates/scenario_1_checkpoint.sav`

3. **Perform test action:**
   - Execute the specific behavior being tested
   - Note the result (position, health, score, etc.)
   - Document in `tests/validation_log.txt`

4. **Load save state and test C version:**
   - Load state: `Ctrl+Alt+F8`, select the saved state
   - Perform same action
   - Compare results with assembly version
   - If different: debug C code and retry

### Documentation Template

Create `tests/scenarios/scenario_1_collision.md`:

```markdown
# Test Scenario 1: Collision Detection

## Purpose
Verify that C implementation of collision detection matches assembly behavior.

## Prerequisite State
- Player position: (x=10, y=5) on level 1
- Save state: `tests/savestates/level1_start.sav`

## Test Actions
1. Move player right to position (x=20, y=5)
2. Attempt to move into wall tile at (x=21, y=5)
3. Verify: Player stays at (x=20, y=5) - collision detected

## Expected Results
- Assembly: Player blocked by wall, stays at x=20
- C Version: Player blocked by wall, stays at x=20
- Status: ✓ PASS / ✗ FAIL

## Notes
- Tile ID 0x40 and above are solid (blocking)
- Tile ID 0x3F and below are passable
```

## Interpreting Test Results

### Test Pass
```
Scenario 1: Collision Detection
  Action: Move right into wall
  Assembly Result: Player X=20 (blocked)
  C Version Result: Player X=20 (blocked)
  Status: ✓ PASS
```

### Test Failure
```
Scenario 1: Collision Detection
  Action: Move right into wall
  Assembly Result: Player X=20 (blocked)
  C Version Result: Player X=21 (not blocked)
  Status: ✗ FAIL

Debug:
  - Check C function logic against assembly
  - Verify tile lookup algorithm
  - Compare returned collision value
  - Fix and retry
```

## Notes

- **Functional testing** focuses on behavior equivalence, not pixel-perfect frames
- **Save states** allow checkpoint-based testing of specific scenarios
- **DOSBox-X debugger** can help inspect game state during testing
- **Manual testing** requires careful documentation to ensure consistency
- **Hard-code RNG seed** in C code for deterministic behavior

## Troubleshooting

### Game doesn't behave consistently between runs
- Check DOSBox config: `core=normal`, `cycles=fixed 3000`
- Ensure no background processes are consuming CPU
- Verify keyboard input timing (human variation is normal)

### Can't find save state
- Create checkpoint manually:
  - Position game at desired state
  - Press `Ctrl+Alt+F7` to save
  - Select slot number
  - State saved to DOSBox-X directory

### Comparing results is tedious
- Use validation log: document expected vs actual results
- Take screenshots of both versions for visual comparison
- Note memory addresses of key variables for manual inspection

### DOSBox-X crashes or becomes unresponsive
- Try `cycles=3000` instead of `fixed 3000`
- Reduce video quality or scaler complexity
- Restart DOSBox-X if it hangs

## Future Enhancements

- Automated test harness with hardcoded expected values
- Memory inspection tools to compare register states
- Automated screenshot comparison at checkpoints
- Performance profiling via DOSBox cycle counting
- Unit tests for individual C functions before integration

