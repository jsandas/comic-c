# Captain Comic C Refactor Plan

## Project Status (As of 2026-01-11)

**Overall Progress**: Phase 3 in progress - Game Logic Implementation  
**Latest Milestone**: Physics and collision detection fully implemented and linked  
**Current Focus**: Enemy AI and collision response systems

### Recent Accomplishments
- ✅ Migrated LAKE and FOREST level data from assembly to C
- ✅ Implemented complete `load_pt_file()` for tile map loading  
- ✅ Implemented `load_new_level()` in pure C
- ✅ Created comprehensive level data structures with type definitions
- ✅ Established error handling for file I/O operations
- ✅ **Implemented main game loop with tick synchronization**
- ✅ **Added input state tracking (all keyboard inputs)**
- ✅ **Implemented game state management (HP, lives, win counter, fireball meter)**
- ✅ **Refactored control flow to eliminate goto statements**
- ✅ **Created stub functions for all subsystems (physics, rendering, actors)**
- ✅ **Code review: Fixed simultaneous left/right key handling**
- ✅ **Code review: Clarified jump counter exhaustion logic**
- ✅ **Code review: Removed redundant forward declarations**
- ✅ **Code review: Consolidated fireball meter counter logic**
- ✅ **Code review: Fixed null pointer checks in tile access**
- ✅ **Code review: Fixed teleportation control flow (skip rendering instead of continue)**
- ✅ **Code review: Fixed fireball meter increment condition (only when not firing)**
- ✅ **Code review: Initialized registers in dos_idle()**
- ✅ **Code review: Clarified Y coordinate bounds in floor detection**
- ✅ **Code review: Refactored nested floor detection logic for clarity**
- ✅ **Code review: Enhanced documentation for address_of_tile_at_coordinates()**
- ✅ **Code review: Documented load_new_stage() implementation requirements**
- ✅ **Code review: Enhanced handle_teleport() with complete implementation spec**
- ✅ **Implemented rendering subsystem in C**
  - ✅ `blit_map_playfield_offscreen()` - Render map tiles with camera positioning
  - ✅ `blit_comic_playfield_offscreen()` - Render player sprite with animation
  - ✅ `swap_video_buffers()` - Double-buffering page flip
  - ✅ EGA plane management and video buffer infrastructure
  - ✅ Integer overflow/underflow prevention in coordinate calculations
- ✅ Complete remaining 6 level data definitions (SPACE, BASE, CAVE, SHED, CASTLE, COMP) - COMPLETED 2026-01-11
- ✅ **Implement physics and collision detection in C** - COMPLETED 2026-01-11
  - ✅ Created `include/physics.h` with physics constants and function declarations
  - ✅ Implemented `src/physics.c` with full physics engine
  - ✅ `handle_fall_or_jump()` - Gravity, jumping, velocity integration, collision detection
  - ✅ `move_left()`/`move_right()` - Horizontal movement with wall collision and stage transitions
  - ✅ `face_or_move_left()`/`face_or_move_right()` - Direction handling with animation
  - ✅ `address_of_tile_at_coordinates()` - Tile map lookup with bounds checking
  - ✅ Resolved variable scope issues (made 25+ physics-related variables non-static)
  - ✅ Fixed C89 variable declaration compliance (moved declarations to block start)
  - ✅ Implemented `comic_dies()` function for fall/death handling
  - ✅ Created global `current_level_ptr` for physics stage data access
  - ✅ Build complete with COMIC-C.EXE successfully generated (123KB, 0 errors)

### Next Priority Items
1. Implement enemy AI and actor management systems
2. Develop collision response for enemy interactions
3. Implement item pickup and door transitions
4. Test physics in actual game with DOSBox


---

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
- **Assembler**: NASM (used for assembling the reference disassembly and tooling); new development should be implemented in C
- **Linker**: djlink (custom OMF format linker)
- **Build Platform**: Local Open Watcom installation on macOS (via `setvars.sh`)

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

### Target State (C-Only Entry Point)
```
┌──────────────────────────────────┐
│      game_main.c                 │
│                                  │
│  • main() - Entry point          │
│  • disable_pc_speaker()          │
│  • install_interrupt_handlers()  │
│  • calibrate_joystick()          │
│  • save_video_mode()             │
│  • check_ega_support()           │
│  • display_startup_notice()      │
│  • setup_keyboard_interactive()  │
│  • title_sequence()              │
│  • terminate_program()           │
└──────────────────────────────────┘
              │
              ▼
         ┌──────────────────────────────┐
         │     Game Logic (C)           │
         │                              │
         │  • game_entry()              │
         │  • Main game loop            │
         │  • Calls subsystems          │
         └──────────────────────────────┘
              │
              ▼
    ┌─────────────────────────┐
    │  Subsystem modules (C)  │
    │                         │
    │  • collision.c          │
    │  • physics.c            │
    │  • rendering.c          │
    │  • ai.c                 │
    │  • file_loaders.c       │
    └─────────────────────────┘
              │
              ▼
  ┌────────────────────────────────────┐
  │      Assembly Reference (Inspect)  │
  │  Original assembly preserved for    │
  │  inspection and implementation      │
  │  guidance; prefer reimplementing    │
  │  behavior in C rather than adding   │
  │  new assembly.                      │
  └────────────────────────────────────┘
```

### Current State Note (As of 2026-01-03)

The refactoring approach has been revised to a **C-only entry point** model:

- **Assembly exits**: `R5sw1991_c.asm` is no longer the primary entry point
- **C handles initialization**: `main()` in `game_main.c` is now the true entry point
- **All initialization in C**: Startup, menus, joystick calibration, video mode setup, interrupt handler setup
- **Clean separation**: Assembly code is retained as a reference; any remaining assembly must have clear justification and tests and should not be the default implementation choice
- **Unified codebase**: `game_main.c` consolidates both `main_experiment.c` and original `game_main.c` functionality

**Rationale**: Moving from assembly initialization to pure C initialization provides:
1. **Simpler testing**: Can test initialization without DOS/assembly dependencies
2. **Better maintainability**: All startup logic in one place (C)
3. **Cleaner architecture**: C is the primary language, assembly is optional/helper
4. **Easier to extend**: Add new menus and features without touching assembly
5. **Future-ready**: Can eventually eliminate assembly entirely if desired

**Rationale**: Attempting a hybrid approach where C and assembly interleave within the game loop is not feasible without significant refactoring of the assembly code. The assembly extensively uses `jmp` to labels rather than `call`/`ret`, making it difficult to extract individual functions while maintaining control flow. Instead, we'll establish a clean boundary: assembly does all initialization and setup, then hands off to C for the main game logic. This allows us to rewrite the game logic in C from scratch rather than translating it piece by piece.

## Refactoring Strategy

### Phase 1: Foundation ✅ (COMPLETED)

**Goal**: Establish build infrastructure and minimal C integration

1. ✅ Create build environment
   - Initial Docker-based setup with Open Watcom 2, NASM, djlink
   - Transitioned to local Open Watcom installation on macOS
   - `setvars.sh` for environment configuration
   - Makefile with `compile` and `clean` targets
   - djlink built for macOS using clang++

2. ✅ Create C/Assembly bridge
   - Modified `R5sw1991_c.asm` with `global` exports for functions and variables
   - `globals.h` with `#pragma aux "*"` declarations for all game state variables
   - `assembly.h` for assembly function declarations with `__near` calling convention
   - Successfully resolved 16-bit calling convention issues (far vs. near calls)

3. ✅ Establish assembly initialization path
   - `R5sw1991_c.asm` handles entry point, DS init, interrupts, hardware, title sequence
   - `game_main.c` with `game_entry()` stub that returns immediately
   - Assembly calls `game_entry()` after title sequence
   - Assembly jumps to `load_new_level()` when C returns (temporary)

4. ✅ Data file loader infrastructure
   - `file_loaders.h` with file format definitions
   - `file_loaders.c` with `load_pt_file()` stub (not yet active)
   - Placeholder stubs for `.TT2`, `.SHP`, `.EGA` loaders
   - File I/O currently handled by assembly INT 21h calls

### Phase 2: C-Only Entry Point (COMPLETED - 2026-01-03)

**Goal**: Consolidate startup and menu logic into unified C entry point

✅ **Completed**:
1. **Consolidated main_experiment.c and game_main.c**
   - Merged 789 lines + 289 lines → 867 lines in single `game_main.c`
   - Preference for `main_experiment.c` functions (more complete initialization)
   - Added game loop stubs and level loading from original `game_main.c`

2. **Established C-only entry point**
   - `main()` in `game_main.c` is now the true application entry point
   - No more `game_entry_c()` being called from assembly
   - No more assembly initialization of interrupts/video/speaker

3. **Included all initialization functions**
   - `disable_pc_speaker()`, `save_video_mode()`, `check_ega_support()`
   - `install_interrupt_handlers()`, `calibrate_joystick()`
   - `load_keymap_file()`, `check_interrupt_handler_install_sentinel()`
   - `clear_keyboard_buffer()`, `display_ega_error()`

4. **Included all menu/UI functions**
   - `display_startup_notice()` - main menu with K/J/R/ESC options
   - `setup_keyboard_interactive()` - keyboard configuration
   - `calibrate_joystick_interactive()` - joystick setup
   - `display_registration_notice()` - about screen
   - `title_sequence()` - title/story sequence stub

5. **Removed assembly pragmas**
   - No more `#pragma aux game_entry_c`, `#pragma aux terminate_program_c`
   - All code is self-contained in C with standard DOS/Watcom includes

**Next steps**
   - Implement `game_entry()` with actual game logic
   - Reimplement rendering and physics in C (consult assembly for guidance only)
   - Port remaining subsystems from assembly to C (avoid adding new assembly)

### Phase 3: Game Logic Implementation (IN PROGRESS - 2026-01-04)

**Goal**: Implement main game loop and level management in C

✅ **Completed**:
1. **Level data migration** (LAKE, FOREST)
   - ✅ Migrated LAKE level data from R3_levels.asm to C
   - ✅ Migrated FOREST level data from R3_levels.asm to C
   - ✅ Created `level_data.h` with all structs and type definitions
   - ✅ Created `level_data.c` with level_data_lake and level_data_forest constants
   - ✅ Defined enemy sprite file references for each level
   - ✅ Defined door configurations and exit mappings
   - ✅ Remaining 6 levels (SPACE, BASE, CAVE, SHED, CASTLE, COMP) stubbed

2. **File loader implementation**
   - ✅ `load_pt_file()` - Load .PT tile map files fully implemented
   - ✅ Returns proper error codes on file not found or read failures
   - ✅ `load_tt2_file()` - Placeholder (TT2 format complex; lower priority)
   - ✅ `load_shp_file()` - Placeholder (sprite loader)
   - ✅ `load_ega_file()` - Placeholder (EGA graphics loader)

3. **Level loading in C**
   - ✅ `load_new_level()` - C implementation loads all three .PT files for stage
   - ✅ `load_new_stage()` - Stub function defined
   - ✅ `game_loop()` - **Fully implemented main game loop structure**
   - ✅ **All 8 level data definitions complete** (LAKE, FOREST, SPACE, BASE, CAVE, SHED, CASTLE, COMP)

4. **Main game loop implementation** ✅ **COMPLETE**
   - ✅ Tick-waiting logic with busy-wait synchronization
   - ✅ Input state variables (`key_state_*`) declared and tracked
   - ✅ Game state variables (HP, lives, win counter, fireball meter, player position, animation)
   - ✅ Win/lose condition checking framework
   - ✅ Proper control flow without goto statements
   - ✅ Teleportation handling with skip_rendering flag
   - ✅ Fall/jump, movement, pause, and fire input processing
   - ✅ Actor management calls (enemies, fireballs, items)
   - ✅ Video buffer swap integration
   - ✅ **Code quality improvements from full review:**
     - ✅ Fixed null pointer checks for tile map access
     - ✅ Clarified jump counter sentinel value usage
     - ✅ Consolidated and documented fireball meter counter logic
     - ✅ Refactored nested floor detection for clarity
     - ✅ Enhanced input handling documentation
     - ✅ Fixed CPU initialization in dos_idle()
     - ✅ Documented coordinate bounds and tile addressing

5. **Rendering subsystem in C**
   - ✅ `blit_map_playfield_offscreen()` - Render map tiles to offscreen buffer with camera positioning
   - ✅ `blit_comic_playfield_offscreen()` - Render Comic sprite with animation state selection
   - ✅ `swap_video_buffers()` - Page flip for double-buffering
   - ✅ Video buffer management and addressing
   - ✅ EGA plane write/read management
   - ✅ Sprite blitting with masking (`blit_sprite_16x16_masked`, `blit_sprite_16x32_masked`)
   - ✅ RLE decompression for fullscreen graphics
   - ✅ Palette management for title sequence fades
   - ✅ Safe integer arithmetic to prevent overflow/underflow in coordinate calculations

**In Progress / TODO**:  
6. **Implement physics and collision in C**
   - [ ] `handle_fall_or_jump()` - Gravity, jumping, terminal velocity
   - [ ] `face_or_move_left()` - Left movement with wall collision
   - [ ] `face_or_move_right()` - Right movement with wall collision
   - [ ] `address_of_tile_at_coordinates()` - Tile map lookup
   - [ ] Ground detection and landing mechanics

7. **Implement actor systems in C**
   - [ ] `handle_enemies()` - Enemy AI and rendering
   - [ ] `handle_fireballs()` - Fireball movement and collision
   - [ ] `handle_item()` - Item collision and rendering
   - [ ] `try_to_fire()` - Fireball spawning logic
   
8. **Implement special features in C**
   - [ ] `begin_teleport()` - Teleport destination calculation
   - [ ] `handle_teleport()` - Teleport animation and camera movement
   - [ ] `pause_game()` - Pause screen display
   
9. **Strategy**
   - Reimplement functions in C when feasible and preferred
   - Consult original assembly as implementation guidance
   - Only interface with original assembly when there is a clear, documented necessity and tests to justify it
   - Complete remaining 6 level data definitions before full game loop implementation

### Phase 4: Subsystem Migration (IN PROGRESS)

**Goal**: Convert individual subsystems from assembly to C

**Current Focus Areas** (as of 2026-01-04):
- **Rendering**: Video buffer management, map/sprite rendering
- **Physics**: Collision detection, player movement, gravity
- **Actors**: Enemy AI, fireball handling, item collection

#### Rendering Subsystem

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

#### Physics and Collision

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

#### Actor Management

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

#### Game State Management

**Goal**: Convert level loading, doors, teleportation to C

1. **File loading** ✅ (PARTIALLY COMPLETE - 2026-01-04)
   - ✅ `load_pt_file()` - load tile map - fully implemented
   - [ ] `load_tt2_file()` - load tileset graphics
   - [ ] `load_shp_file()` - load sprites
   - [ ] `load_ega_file()` - load fullscreen graphics with RLE decompression

2. **Level management** ✅ (PARTIALLY COMPLETE - 2026-01-04)
   - ✅ `load_new_level()` - set up level data (LAKE, FOREST implemented)
   - ✅ Level data constants defined in C (LAKE, FOREST complete; 6 more to go)
   - [ ] `load_new_stage()` - initialize stage (stub defined)
   - [ ] `render_map()` - pre-render entire map to video buffer
   - [ ] Level transition logic

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

### Phase 5: Sound and Input (Optional)

**Goal**: Convert remaining subsystems (lower priority)

1. **Sound system**
   - Note data structures
   - Sound playback state machine
   - Priority system
   - PC speaker control via INT 3 handler
   - *Keep in assembly only with strong justification and tests* (direct hardware, low value to convert)

2. **Joystick handling**
   - Calibration routines
   - Axis reading
   - Threshold detection
   - *Keep in assembly only with strong justification and tests* (INT 15h, low value to convert)

3. **Interrupt handlers**
   - INT 3 (sound)
   - INT 8 (timer - sets `game_tick_flag`)
   - INT 9 (keyboard - sets `key_state_*`)
   - *Keep in assembly only with strong justification and tests* (direct hardware, real-mode quirks)

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
# 1. Compile project
make compile

# 2. Test executable
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
- [x] C previously called assembly functions (legacy; minimized)
- [x] Basic file loader converted

### Milestone 2: Level Data and File Loading ✅ (COMPLETED 2026-01-04)
- [x] Level data structures defined in C
- [x] LAKE and FOREST level data migrated from assembly
- [x] Remaining 6 level stubs defined
- [x] File loader infrastructure implemented
- [x] `load_pt_file()` fully functional
- [x] `load_new_level()` C implementation working

### Milestone 3: Game Loop ✅
- [x] C game loop replaces assembly loop
- [x] Input handling in C
- [x] Win/lose/quit detection in C
- [ ] Tier 1 tests pass

### Milestone 4: Rendering ✅
- [x] Video buffer management in C
- [x] Map rendering in C
- [x] Sprite rendering in C
- [ ] Tier 2 tests pass
- [ ] Visual comparison matches original

### Milestone 5: Physics
- [ ] Collision detection fully in C
- [ ] Player movement in C
- [ ] Gravity and jumping in C
- [ ] Tier 1 tests pass
- [ ] Movement feels identical to original

### Milestone 6: Actors
- [ ] Enemy AI in C
- [ ] Fireball handling in C
- [ ] Item handling in C
- [ ] Tier 4 tests pass
- [ ] All behaviors work correctly

### Milestone 7: Full Gameplay
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
