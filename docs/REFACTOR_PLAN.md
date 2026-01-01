# Captain Comic C Refactor Plan

## Project Goal

Refactor The Adventures of Captain Comic (1990 DOS platformer) from x86-16 assembly to C while maintaining **exact behavioral compatibility** with the original game. The refactored code must still run in 16-bit DOS real-mode.

## Motivation

- **Maintainability**: C is easier to understand and modify than assembly
- **Portability**: C code can be more easily adapted to other platforms
- **Learning**: Understand classic game architecture and DOS programming
- **Preservation**: Document and preserve a piece of gaming history

## Technical Constraints

### Environment
- **Target**: 16-bit DOS real-mode (Intel 8086/8088 compatible)
- **Compiler**: Open Watcom C 16-bit v2.0
- **Memory Model**: Large (`-ml`) - separate 64KB code and data segments
- **Assembler**: NASM for remaining assembly code
- **Linker**: djlink (custom OMF format linker)
- **Build Platform**: Docker container (macOS host lacks Open Watcom)

### Compatibility Requirements
- Must produce identical behavior to original `COMIC.EXE` (Revision 5)
- Even known bugs must be preserved for compatibility
- Save states from original must work with refactored version
- Game assets (`.PT`, `.TT2`, `.SHP`, `.EGA` files) remain unchanged

## Architecture Overview

### Current State (Original Assembly)
```
┌─────────────────────────────────────┐
│   R5sw1991.asm (7,842 lines)        │
│                                     │
│  • Entry point & initialization    │
│  • Interrupt handlers (INT 3,8,9)  │
│  • Video/sound hardware control    │
│  • Main game loop                  │
│  • Physics, collision, AI          │
│  • Rendering (EGA graphics)        │
│  • File loading                    │
└─────────────────────────────────────┘
```

### Target State (C + Assembly Hybrid)
```
┌──────────────────────┐   ┌──────────────────────┐
│   R5sw1991_c.asm     │◄──│    game_main.c       │
│                      │   │                      │
│  • Entry point       │   │  • Main game loop    │
│  • DS initialization │   │  • Input handling    │
│  • Interrupt setup   │   │  • Game logic        │
│  • Hardware init     │   └──────────────────────┘
│  • Title sequence    │            │
└──────────────────────┘            ▼
         │              ┌──────────────────────┐
         │              │   Subsystem modules  │
         │              │                      │
         │              │  • collision.c       │
         │              │  • physics.c         │
         │              │  • rendering.c       │
         │              │  • ai.c              │
         │              │  • file_loaders.c    │
         │              └──────────────────────┘
         │                       │
         ▼                       ▼
┌────────────────────────────────────┐
│     Assembly Functions             │
│  (gradually replaced by C)         │
│                                    │
│  • swap_video_buffers()            │
│  • handle_enemies()                │
│  • blit_map_playfield_offscreen()  │
│  • etc.                            │
└────────────────────────────────────┘
```

### Current State Note (As of 2025-12-31)

The **Target State** diagram above represents the *eventual* goal. However, the current implementation takes a pragmatic approach:

- **Assembly remains in control**: `R5sw1991_c.asm` orchestrates the entire program flow
- **C acts as framework**: The C headers (`globals.h`, `assembly.h`) establish the interface for future porting
- **No C/Assembly boundary crossing for game loop**: Instead of calling C functions with complex calling conventions, assembly calls `load_new_level()` directly
- **Game loop entirely in assembly**: `load_new_level()` → `load_new_stage()` → `game_loop()` (all assembly)
- **game_main.c is a placeholder**: Currently contains no active code, just commented-out pseudocode showing the intended game loop structure

**Rationale**: Early attempts to call C functions from 16-bit assembly exposed calling convention incompatibilities with djlink. Rather than spend time on C/Assembly boundaries, we've prioritized getting the game running, with the C framework ready for incremental porting.

## Refactoring Strategy

### Phase 1: Foundation ✅ (COMPLETED)

**Goal**: Establish build infrastructure and minimal C integration

1. ✅ Create Docker build environment
   - Dockerfile with Open Watcom 2, NASM, djlink
   - Makefile with `docker-build` and `compile` targets
   - Volume mounting for workspace

2. ✅ Create C/Assembly bridge
   - Modified `R5sw1991_c.asm` with `global` exports for functions and variables
   - `globals.h` with `#pragma aux "*"` declarations for all game state variables
   - `assembly.h` for assembly function declarations with `__near` calling convention
   - Successfully resolved 16-bit calling convention issues (far vs. near calls)

3. ✅ Establish direct assembly control
   - `R5sw1991_c.asm` remains the primary control flow
   - `game_main.c` is a placeholder for future C implementation
   - Assembly directly calls `load_new_level()` which transitions to game
   - Game loop runs entirely in assembly (`load_new_level` → `load_new_stage` → `game_loop`)
   - This eliminates complex C/Assembly boundary crossing for now

4. ✅ Data file loader infrastructure
   - `file_loaders.h` with file format definitions
   - `file_loaders.c` with `load_pt_file()` stub (not yet active)
   - Placeholder stubs for `.TT2`, `.SHP`, `.EGA` loaders
   - File I/O currently handled by assembly INT 21h calls

### Phase 2: Incremental Function Migration (NEXT)

**Goal**: Start moving assembly functions to C one at a time

**Strategy**: Rather than trying to move entire subsystems at once, migrate individual functions:
1. Create C implementation of a function
2. Declare it in `assembly.h` as `extern`
3. Remove assembly implementation
4. Assembly calls C function via same interface
5. No calling convention issues since assembly and C are in same memory model

**Suggested starting points** (simple, isolated):
1. **render_map()** - Pre-render entire map to video buffer
   - Takes no parameters, no return value
   - Only modifies video memory
   - Can validate by comparing rendered output

2. **swap_video_buffers()** - Switch between 0x0000 and 0x2000 buffers
   - One parameter (delay count)
   - No side effects besides video
   - Simplest possible function

3. **wait_n_ticks()** - Wait for N game ticks
   - One parameter (tick count)
   - Reads `game_tick_flag` global
   - Core timing mechanism

**Migration checklist**:
- [ ] Copy assembly logic to C function
- [ ] Test with equivalent assembly code still present (side-by-side)
- [ ] Compare behavior (timing, state changes)
- [ ] Remove assembly version
- [ ] Recompile and test with C version
- [ ] Commit working state

### Phase 3: Game Loop Migration (FUTURE)

**Goal**: Move main game loop logic from assembly to C

Once individual function migration is working, consider implementing `game_loop_iteration()` in C:
1. Implement tick-waiting logic
2. Input state checking
3. Call assembly functions for complex subsystems
4. Win/lose condition checking

### Phase 4: Rendering Subsystem

**Goal**: Convert graphics rendering to C

1. **Video buffer management**
   - `swap_video_buffers()` - switch between 0x0000 and 0x2000
   - Track `offscreen_video_buffer_ptr` in C

2. **Map rendering**
   - `blit_map_playfield_offscreen()` - copy visible map tiles
   - Calculate camera scrolling offsets
   - Handle 24-game-unit playfield width

3. **Sprite rendering**
   - `blit_comic_playfield_offscreen()` - draw player character
   - Handle animation frames and facing direction
   - Implement transparency masking

4. **Enemy/item rendering**
   - Draw enemy sprites at their positions
   - Handle item graphics
   - Implement explosion/death animations

5. **Validation**
   - Test with Tier 2 scenarios (data/state manipulation)
   - Verify pixel-perfect rendering
   - Confirm no graphical glitches

### Phase 5: Physics and Collision

**Goal**: Convert movement and collision detection to C

1. **Tile-based collision** ✅ (PARTIALLY COMPLETE)
   - ✅ `address_of_tile_at_coordinates()` - convert coords to tile offset
   - ✅ `check_vertical_enemy_map_collision()` - vertical collision
   - ✅ `check_horizontal_enemy_map_collision()` - horizontal collision
   - Additional collision helpers as needed

2. **Player physics**
   - `handle_fall_or_jump()` - gravity and jumping
   - Implement fixed-point velocity (1/8 game unit per tick)
   - Terminal velocity handling
   - Ground detection and landing

3. **Player movement**
   - `move_left()`, `move_right()` - horizontal movement
   - Handle `comic_x_momentum` (-5 to +5 range)
   - Wall collision and stopping
   - Animation frame progression

4. **Validation**
   - Test with Tier 1 scenarios (collision detection)
   - Verify physics matches original (jump height, fall speed)
   - Confirm pixel-perfect movement

### Phase 6: Actor Management

**Goal**: Convert enemy AI and projectile handling to C

1. **Fireball system**
   - `handle_fireballs()` - update all active fireballs
   - Movement and collision detection
   - Corkscrew trajectory (sine wave)
   - Fireball-enemy collision
   - Animation frames

2. **Item handling**
   - `handle_item()` - item spawning and collection
   - Collision with player
   - Inventory updates
   - Score and meter adjustments

3. **Enemy spawning**
   - `maybe_spawn_enemy()` - spawn enemies as player approaches
   - Distance-based spawning
   - Despawn when out of range (30 game units)
   - Initialize enemy state struct

4. **Enemy AI behaviors**
   - `enemy_behavior_bounce()` - Fire Ball, Brave Bird, etc.
   - `enemy_behavior_leap()` - Bug-eyes, Blind Toad, Beach Ball
   - `enemy_behavior_roll()` - Glow Globe
   - `enemy_behavior_seek()` - Killer Bee
   - `enemy_behavior_shy()` - Shy Bird, Spinner

5. **Enemy-player collision**
   - Damage to player (reduce HP)
   - Shield protection logic
   - Enemy death (white spark animation)
   - Comic death sequence

6. **Validation**
   - Test with Tier 4 scenarios (enemy AI)
   - Verify each behavior type works correctly
   - Confirm collision timing matches original

### Phase 7: Game State Management

**Goal**: Convert level loading, doors, teleportation to C

1. **File loading** (PARTIALLY COMPLETE)
   - ✅ `load_pt_file()` - load tile map
   - `load_tt2_file()` - load tileset graphics
   - `load_shp_file()` - load sprites
   - `load_ega_file()` - load fullscreen graphics with RLE decompression

2. **Level management**
   - `load_new_level()` - set up level data
   - `load_new_stage()` - initialize stage
   - `render_map()` - pre-render entire map to video buffer
   - Level transition logic

3. **Door system**
   - `activate_door()` - door collision and key check
   - `enter_door()` - transition to connected stage
   - Handle door destinations (level, stage, position)
   - Animation sequence

4. **Teleportation**
   - `begin_teleport()` - initiate teleport (check wand)
   - `handle_teleport()` - animation and position update
   - Move player 6 game units horizontally
   - `comic_is_teleporting` state management

5. **Special events**
   - `pause()` - pause menu (ESC key)
   - `game_over()` - death and continue screen
   - `game_end_sequence()` - victory sequence
   - High score management

6. **Validation**
   - Test with Tier 5 scenarios (level transitions, doors)
   - Verify door connections work correctly
   - Confirm teleportation positioning exact

### Phase 8: Sound and Input (Optional)

**Goal**: Convert remaining subsystems (lower priority)

1. **Sound system**
   - Note data structures
   - Sound playback state machine
   - Priority system
   - PC speaker control via INT 3 handler
   - *May keep in assembly* (direct hardware, low value to convert)

2. **Joystick handling**
   - Calibration routines
   - Axis reading
   - Threshold detection
   - *May keep in assembly* (INT 15h, low value to convert)

3. **Interrupt handlers**
   - INT 3 (sound)
   - INT 8 (timer - sets `game_tick_flag`)
   - INT 9 (keyboard - sets `key_state_*`)
   - *Likely keep in assembly* (direct hardware, real-mode quirks)

## Testing Strategy

### Test Tiers

**Tier 1: Pure Logic** - No side effects, deterministic
- Collision detection functions
- Coordinate calculations
- Tile address computations

**Tier 2: Data/State** - Modifies game state
- File loading
- Initialization routines
- Score/inventory updates

**Tier 3: Complex Interactions**
- Item collection
- Door activation
- Teleportation

**Tier 4: AI and Physics**
- Enemy behaviors
- Player movement
- Projectile handling

**Tier 5: Full Gameplay**
- Level transitions
- Complete playthroughs
- Edge cases and bugs

### Testing Methodology

1. **Create scenario** in `tests/scenarios/scenario_N_description.md`
2. **Prepare save state** at appropriate checkpoint
3. **Document test steps** with expected outcomes
4. **Run in DOSBox-X** with deterministic config
5. **Load save state** (F7)
6. **Execute test actions**
7. **Verify behavior** matches expected
8. **Take screenshots** if visual validation needed
9. **Document results** in `tests/validation_log.txt`

### Existing Test Infrastructure

- ✅ `dosbox_deterministic.conf` - reproducible timing
- ✅ `run-dosbox.sh` - launch script
- ✅ Save states: `1.sav`, `2.sav`, `3.sav`, `4.sav`
- ✅ Test scenario: `scenario_1_collision.md`
- ✅ Validation log format established

## File Organization

```
comic-c/
├── Dockerfile                      # Docker build environment
├── Makefile                        # Build targets
├── README.md                       # Project overview
├── docs/                           # Documentation
│   ├── AGENT_INSTRUCTIONS.md       # AI assistant guidelines
│   ├── CODING_STANDARDS.md         # C code style guide
│   └── REFACTOR_PLAN.md            # This file
├── include/                        # C header files
│   ├── globals.h                   # Assembly global variables
│   ├── assembly.h                  # Assembly function declarations
│   ├── file_loaders.h              # Data file format definitions
│   ├── collision.h                 # Collision detection (future)
│   ├── physics.h                   # Physics routines (future)
│   ├── rendering.h                 # Graphics rendering (future)
│   └── ai.h                        # Enemy AI (future)
├── src/                            # C source files
│   ├── R5sw1991_c.asm              # Modified assembly (exports + calls C)
│   ├── game_main.c                 # C game loop entry point
│   ├── file_loaders.c              # Data file loading
│   ├── collision.c                 # Collision detection (future)
│   ├── physics.c                   # Physics routines (future)
│   ├── rendering.c                 # Graphics rendering (future)
│   └── ai.c                        # Enemy AI (future)
├── build/                          # Build artifacts (generated)
│   ├── obj/                        # Object files (*.obj)
│   └── COMIC.EXE                   # Final executable
├── reference/                      # Original assembly reference
│   ├── Makefile                    # Original build
│   └── disassembly/
│       ├── R5sw1991.asm            # Original commented disassembly
│       ├── R3_levels.asm           # Level data
│       └── djlink/                 # OMF linker
└── tests/                          # Testing framework
    ├── dosbox_deterministic.conf   # DOSBox-X config
    ├── run-dosbox.sh               # Test runner
    ├── validation_log.txt          # Test results log
    ├── HOW_TO_CREATE_SCENARIOS.md  # Testing guide
    ├── README.md                   # Test overview
    ├── scenarios/                  # Test scenario documentation
    │   └── scenario_1_collision.md
    ├── savestates/                 # DOSBox save states
    │   ├── 1.sav
    │   ├── 2.sav
    │   ├── 3.sav
    │   └── 4.sav
    └── capture/                    # Reference screenshots
```

## Build Workflow

### First-Time Setup

```bash
# 1. Build Docker image
make docker-build

# 2. Compile project
make compile

# 3. Test executable
cd tests
./run-dosbox.sh
```

### Development Iteration

```bash
# 1. Edit C or assembly source
vim src/game_main.c

# 2. Compile
make compile

# 3. Test in DOSBox
cd tests
./run-dosbox.sh

# 4. Load save state and verify behavior
# (Inside DOSBox: F7 to load, execute test, verify)

# 5. Clean build artifacts if needed
make clean
```

## Success Criteria

### Milestone 1: Foundation ✅
- [x] Docker build environment operational
- [x] C code compiles without errors
- [x] Executable links successfully
- [x] Assembly stub calls C `game_main()`
- [x] C calls assembly functions
- [x] Basic file loader converted

### Milestone 2: Game Loop
- [ ] C game loop replaces assembly loop
- [ ] Input handling in C
- [ ] Win/lose/quit detection in C
- [ ] Tier 1 tests pass

### Milestone 3: Rendering
- [ ] Video buffer management in C
- [ ] Map rendering in C
- [ ] Sprite rendering in C
- [ ] Tier 2 tests pass
- [ ] Visual comparison matches original

### Milestone 4: Physics
- [ ] Collision detection fully in C
- [ ] Player movement in C
- [ ] Gravity and jumping in C
- [ ] Tier 1 tests pass
- [ ] Movement feels identical to original

### Milestone 5: Actors
- [ ] Enemy AI in C
- [ ] Fireball handling in C
- [ ] Item handling in C
- [ ] Tier 4 tests pass
- [ ] All behaviors work correctly

### Milestone 6: Full Gameplay
- [ ] Level loading in C
- [ ] Door system in C
- [ ] Teleportation in C
- [ ] Tier 5 tests pass
- [ ] Complete playthrough possible

### Final Success
- [ ] All 8 levels playable
- [ ] All enemy behaviors working
- [ ] All items collectible
- [ ] Victory sequence works
- [ ] Game over screen works
- [ ] No regressions vs. original
- [ ] Code is maintainable and documented

## Risk Assessment

### High Risk
- **Timing differences**: C code may execute at different speed than assembly
  - *Mitigation*: Game is tick-based, not frame-based; timing controlled by INT 8 handler
- **Memory model issues**: Far pointers, segment addressing could cause bugs
  - *Mitigation*: Use large model, keep DS pointing to data segment, test incrementally

### Medium Risk
- **Behavioral differences**: Subtle bugs in conversion lead to incorrect behavior
  - *Mitigation*: Extensive save-state testing, keep assembly version for comparison
- **Compiler optimizations**: Open Watcom may optimize away important behavior
  - *Mitigation*: Use `-s` flag (no stack checks), test thoroughly, use `volatile` if needed

### Low Risk
- **Build environment**: Docker adds complexity to build process
  - *Mitigation*: Makefile abstracts Docker commands, well-documented setup
- **File I/O**: C file operations may behave differently in DOS
  - *Mitigation*: Open Watcom provides DOS-compatible C library

## Timeline Estimate

- **Phase 1 (Foundation)**: ✅ Complete - 1 week
- **Phase 2 (Game Loop)**: 1-2 weeks
- **Phase 3 (Rendering)**: 2-3 weeks
- **Phase 4 (Physics)**: 2-3 weeks
- **Phase 5 (Actors)**: 3-4 weeks
- **Phase 6 (Game State)**: 2-3 weeks
- **Phase 7 (Optional)**: 1-2 weeks

**Total: 12-18 weeks** for complete conversion

## References

- Original assembly: `reference/disassembly/R5sw1991.asm`
- Testing guide: `tests/HOW_TO_CREATE_SCENARIOS.md`
- Coding standards: `docs/CODING_STANDARDS.md`
- Agent instructions: `docs/AGENT_INSTRUCTIONS.md`
- Open Watcom docs: https://open-watcom.github.io/open-watcom-v2-wikidocs/
- NASM docs: https://www.nasm.us/doc/
- Captain Comic info: http://www.shikadi.net/moddingwiki/Captain_Comic

## Questions and Decisions

### Open Questions
1. Should interrupt handlers be converted to C or left in assembly?
   - **Decision**: Leave in assembly (Phase 7, optional)
2. Should we use Open Watcom C library or direct DOS INT 21h calls?
   - **Decision**: Use C library where possible (simpler, already tested)
3. What level of optimization should we target?
   - **Decision**: Correctness first, optimization later (if needed)

### Architectural Decisions
1. **Memory model**: Large model (separate code/data segments)
   - Chosen for compatibility with assembly structure
2. **Calling convention**: cdecl (caller cleans stack)
   - Standard for Open Watcom, matches assembly expectations
3. **Entry point**: Assembly stub calls C
   - Avoids crt0.obj overhead, full control over initialization
4. **Global variables**: Remain in assembly, accessed via extern
   - Simplifies integration, avoids data duplication
