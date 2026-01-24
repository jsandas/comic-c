# Captain Comic C Refactor Plan

## Project Status (As of 2026-01-24)

**Overall Progress**: Phase 4 Complete - All core game systems and UI rendering implemented in C  
**Latest Milestone**: HP/Shield meter system fully implemented in C  
**Current Focus**: Testing and validation (Phase 6)

### Recent Accomplishments
- ✅ **Interrupt Handlers fully implemented in pure C** - COMPLETED 2026-01-21
  - ✅ INT 8 (timer) handler using Open Watcom's `__interrupt` keyword
  - ✅ INT 9 (keyboard) handler with scancode queue
  - ✅ Direct hardware port access from C (port 0x60 for keyboard)
  - ✅ Proper interrupt chaining with `_chain_intr()`
  - ✅ Sound advancement integrated into INT 8 handler
  - ✅ Game tick flag management for timing synchronization
  - ✅ No assembly code required - pure C implementation
- ✅ **Sound System fully implemented in pure C** - COMPLETED 2026-01-21
  - ✅ PC speaker control via direct hardware access (ports 0x61, 0x42, 0x43)
  - ✅ PIT (Programmable Interval Timer) frequency control
  - ✅ Sound playback state machine with priority system
  - ✅ All 13 game sound effects defined and integrated
  - ✅ `sound_advance_tick()` integrated into main game loop
  - ✅ Mute/unmute functionality for user control
  - ✅ No assembly code required - pure C implementation
- ✅ Implemented game over screen with GAME OVER graphic overlay
- ✅ Implemented victory sequence with score tallying animation
- ✅ Added 3-byte (24-bit) score storage system (supports 0-999,999 points)
- ✅ Integrated game over/victory sequences into main game loop
- ✅ Respawn logic with checkpoint positions
- ✅ Lives tracking and game over condition detection
- ✅ All 8 level data definitions complete (LAKE, FOREST, SPACE, BASE, CAVE, SHED, CASTLE, COMP)
- ✅ Physics and collision detection fully implemented in C
- ✅ Actor systems (enemies, fireballs, items) fully implemented in C
- ✅ Special features (teleportation, beam in/out, pause) fully implemented in C
- ✅ Door system implemented with level transitions
- ✅ Sprite loading and rendering for enemies, items, and effects
- ✅ **Code review: Documented load_new_stage() implementation requirements**
- ✅ **Code review: Enhanced handle_teleport() with complete implementation spec**
- ✅ **Implemented rendering subsystem in C**
  - ✅ `blit_map_playfield_offscreen()` - Render map tiles with camera positioning
  - ✅ `blit_comic_playfield_offscreen()` - Render player sprite with animation
  - ✅ `swap_video_buffers()` - Double-buffering page flip
  - ✅ EGA plane management and video buffer infrastructure
  - ✅ Integer overflow/underflow prevention in coordinate calculations
- ✅ Complete remaining 6 level data definitions (SPACE, BASE, CAVE, SHED, CASTLE, COMP) - COMPLETED 2026-01-11
- ✅ **Implement physics and collision detection in C** - COMPLETED 2026-01-17
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
  - ✅ **Fixed critical physics bugs by comparing C implementation to assembly**:
    - ✅ Fixed double physics call on jump initiation (was processing twice per frame)
    - ✅ Corrected physics calculation order to match assembly exactly
    - ✅ Fixed jump counter logic (acceleration now applied correctly for 3 frames, not 2 or 4)
    - ✅ Verified all physics constants match original (GRAVITY=5, POWER=4, ACCELERATION=7)
    - ✅ Jump heights now match original game (confirmed by user testing)
- ✅ **Implement actor systems in C** - COMPLETED 2026-01-17
  - ✅ Complete enemy AI with all 5 behavior types
  - ✅ Fireball spawning, movement, and collision detection
  - ✅ Item collection and tracking system
  - ✅ Rendering infrastructure in place (sprite loading pending)
- ✅ **Implement special features in C** - COMPLETED 2026-01-19
  - ✅ `begin_teleport()` - Find safe landing spot with tile scanning
  - ✅ `handle_teleport()` - 6-frame teleport animation with camera movement
  - ✅ `pause_game()` - Pause screen display and keypress handling
  - ✅ `beam_in()` - 12-frame materialize animation for level start
  - ✅ `beam_out()` - 12-frame materialize animation for level completion
  - ✅ All teleport variables properly declared and integrated
  - ✅ Sound effects wired (SOUND_TELEPORT, SOUND_MATERIALIZE)

### Recent Accomplishments (Continued)
- ✅ **HP/Shield meter system fully implemented in C** - COMPLETED 2026-01-24
  - ✅ `increment_comic_hp()` - Increments HP (0-6), renders full sprite at calculated position, awards 1800 bonus at max
  - ✅ `decrement_comic_hp()` - Renders empty sprite before decrementing, plays SOUND_DAMAGE
  - ✅ `render_comic_hp_meter()` - Renders all 6 cells as full/empty based on current HP
  - ✅ HP meter displays at Y=82 with correct cell positions (248-288 pixels)
  - ✅ Pending increase mechanism: `comic_hp_pending_increase` increments 1 per game tick
  - ✅ Shield item collection with HP refill logic in `actors.c`
  - ✅ Extra life award when Shield collected with full HP
  - ✅ Visual health persistence during death/respawn sequences
  - ✅ Fixed MAX_HP constant from 7 to 6 (6-cell display)
  - ✅ Made `comic_hp_pending_increase` public for cross-module access
  - ✅ All necessary extern declarations in actors.c
  - ✅ Integration with game loop rendering phase
  - ✅ Build verified with zero errors/warnings
  - ✅ Committed to git repository (commit 0307d3d)

### Next Priority Items
1. **Game UI Rendering** (Phase 4 Step 4) ✅ **COMPLETE** - 2026-01-24
   - ✅ Inventory display (Door Key, Teleport Wand, Corkscrew, Shield) - COMPLETE
   - ✅ Score digit rendering (6-digit dynamic display) - COMPLETE
   - ✅ Fireball power meter visual display - COMPLETE
   - ✅ HP (Hit Points) meter visual display - COMPLETE
   - ✅ Shield item collection with HP refill logic - COMPLETE
2. **Testing and Validation** (Phase 6)
   - Create test scenarios for all implemented systems
   - Validate physics accuracy against original game
   - Validate AI behaviors and collision detection
   - Full level playthroughs in DOSBox
   - Verify save state compatibility


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

6. **Implement physics and collision in C** ✅ **COMPLETE** - 2026-01-17
   - ✅ `handle_fall_or_jump()` - Gravity, jumping, terminal velocity, collision detection
   - ✅ `move_left()` / `move_right()` - Horizontal movement with wall collision and stage transitions
   - ✅ `face_or_move_left()` / `face_or_move_right()` - Direction handling with animation
   - ✅ `address_of_tile_at_coordinates()` - Tile map lookup with bounds checking
   - ✅ Ground detection and landing mechanics
   - ✅ Fixed critical bugs: double physics call, calculation order, jump counter logic
   - ✅ Verified all physics constants match original assembly (GRAVITY=5, POWER=4, ACCEL=7)

7. **Implement actor systems in C** ✅ **COMPLETE** - 2026-01-17
   - ✅ Created `include/actors.h` with enemy, fireball, and item structures
   - ✅ Implemented `src/actors.c` with all actor management systems
   - ✅ `try_to_fire()` - Fireball spawning with meter depletion
   - ✅ `handle_fireballs()` - Fireball movement, corkscrew motion, collision with enemies
   - ✅ `handle_item()` - Item animation, collision detection, score awards
   - ✅ `handle_enemies()` - Enemy spawning, AI execution, collision with player
   - ✅ `maybe_spawn_enemy()` - Distance-based enemy spawning logic
   - ✅ All 5 enemy AI behaviors implemented:
     - ✅ `enemy_behavior_bounce()` - Diagonal bouncing (Fire Ball, Brave Bird)
     - ✅ `enemy_behavior_leap()` - Jumping with gravity (Bug-eyes, Blind Toad, Beach Ball)
     - ✅ `enemy_behavior_roll()` - Ground-following (Glow Globe)
     - ✅ `enemy_behavior_seek()` - Pathfinding toward player (Killer Bee)
     - ✅ `enemy_behavior_shy()` - Flee when facing Comic (Shy Bird, Spinner)
   - ✅ Enemy and fireball initialization in `load_new_stage()`
   - ✅ Item collection tracking with `items_collected[8][16]` array
   - ✅ Build complete with COMIC-C.EXE successfully generated
   
8. **Implement special features in C** ✅ **COMPLETE** - 2026-01-19
   - ✅ `begin_teleport()` - Teleport destination calculation with safe landing detection
   - ✅ `handle_teleport()` - Teleport animation with 6-frame effect at source and destination
   - ✅ `pause_game()` - Pause screen display with keypress handling
   - ✅ `beam_in()` - Beam in animation (12 frames) for game start
   - ✅ `beam_out()` - Beam out animation (12 frames) for game victory
   - ✅ All teleport-related variables declared and initialized
   - ✅ Sound effects integrated (SOUND_TELEPORT, SOUND_MATERIALIZE)
   - ✅ Sprite animation tables set up for materialize and teleport effects
   - ✅ Camera movement synchronized with teleport animation
   - ✅ Safe landing detection with solid/non-solid tile scanning

9. **Load and render sprites** ✅ **COMPLETE** - 2026-01-19
   - ✅ Implement `.SHP` file loading for enemy sprites
   - ✅ Load and render embedded sprite data for items and effects
   - ✅ Load and render fireball sprites
   - ✅ Integrate sprite rendering in actor handlers (enemies, items, fireballs)
   - ✅ Load and render spark sprites for death animations
   - ℹ️ **Note**: `SYS004.EGA` is title-sequence only; gameplay uses compiled embedded sprite data

### Phase 4: Remaining Game Features (IN PROGRESS - 2026-01-19)

**Goal**: Implement missing systems for complete gameplay

1. **Door System** ✅ **COMPLETE** - 2026-01-19
   - ✅ Created `include/doors.h` with door system declarations
   - ✅ Implemented `src/doors.c` with door mechanics:
     - ✅ `check_door_activation()` - Check if Comic is in front of a door and open key is pressed
     - ✅ Collision detection with 3-unit tolerance for X coordinate alignment
     - ✅ Door Key requirement checking
     - ✅ `activate_door()` - Transition to target level/stage via door
     - ✅ Save source level/stage for reciprocal door positioning
     - ✅ Conditional load: `load_new_level()` for different levels, `load_new_stage()` for same level
   - ✅ Integrated door checking into main game loop
   - ✅ Successfully compiled with zero warnings/errors

2. **Game Over and Victory Sequences** ✅ **COMPLETE** - 2026-01-20
   - ✅ Added `comic_num_treasures` variable for treasure tracking
   - ✅ Implemented `game_over()` - Display game over screen with graphic overlay
   - ✅ Implemented `game_end_sequence()` - Victory animation with score tallying
   - ✅ Implemented `award_points()` - Points system with 3-byte (24-bit) score storage (0-999,999)
   - ✅ Updated `comic_dies()` - Integrated game over check and respawn logic
   - ✅ Updated game loop - Call `game_end_sequence()` when win_counter reaches 1
   - ✅ Fixed `lose_a_life()` - Properly decrements lives and checks game over condition
   - ✅ Score overflow protection - Prevents score exceeding 999,999 points
   - ✅ Keyboard input handling - Clear buffer and wait for keystroke on game over/victory
   - ✅ Successfully compiled with zero warnings/errors

3. **Title Sequence and Menus** ✅ **COMPLETE** - 2026-01-21
   - ✅ `SYS004.EGA` loading and RLE decompression
   - ✅ Menu navigation and input handling
   - ✅ Story/intro sequence display

4. **Game UI Rendering** ✅ **COMPLETE** - 2026-01-24
   - **Inventory display** ✅ **COMPLETE** - 2026-01-21
     - ✅ `render_inventory_display()` - Renders collected inventory items on UI
     - ✅ Displays Door Key at (280, 112) when collected
     - ✅ Displays Teleport Wand at (280, 136) when collected
     - ✅ Displays Corkscrew at (256, 112) when collected
     - ✅ Integrated into main game loop after item rendering, before buffer swap
     - ✅ Uses 16x16 masked sprites for each item
     - ✅ Zero warnings/errors on build
   - **Score display** ✅ **COMPLETE** - 2026-01-21
     - ✅ `render_score_display()` - Renders 6-digit score on game UI
     - ✅ Displays at screen position (264, 24) near top right corner
     - ✅ Converts 3-byte base-100 score to 6 decimal digits
     - ✅ Each base-100 digit displayed as 2 decimal digits (0-99)
     - ✅ Uses 8x16 digit sprites (sprite_score_digit_0_8x16 through _9_8x16)
     - ✅ Supports scores from 0 to 999,999 points
     - ✅ Integrated into main game loop after inventory rendering
     - ✅ Zero warnings/errors on build
   - **Fireball power meter** ✅ **COMPLETE** - 2026-01-21
     - ✅ `increment_fireball_meter()` - Increments meter and renders appropriate cell sprite
     - ✅ `decrement_fireball_meter()` - Decrements meter and renders appropriate cell sprite
     - ✅ Displays at screen position (240, 54) with 6 cells
     - ✅ Each cell shows empty/half/full using 8x16 meter sprites
     - ✅ Fireball meter value ranges from 0-12 (6 cells × 2 units each)
     - ✅ Zero warnings/errors on build
   - **HP (Hit Points) meter** ✅ **COMPLETE** - 2026-01-24
     - ✅ `increment_comic_hp()` - Increments HP and renders full sprite, awards 1800 bonus points when full
     - ✅ `decrement_comic_hp()` - Decrements HP, renders empty sprite, and plays SOUND_DAMAGE
     - ✅ `render_comic_hp_meter()` - Renders all 6 cells as full/empty based on current HP
     - ✅ Displays at screen position (240, 82) with 6 cells at X = 248-288 pixels (8px apart)
     - ✅ HP value ranges from 0-6 (MAX_HP = 6 in globals.h)
     - ✅ Pending increase mechanism: `comic_hp_pending_increase` increments 1 per game tick
     - ✅ Visual persistence during respawn: preserves `comic_hp` value, only sets `comic_hp_pending_increase`
     - ✅ Zero warnings/errors on build
   - **Shield item collection** ✅ **COMPLETE** - 2026-01-24
     - ✅ Shield item refills HP via `comic_hp_pending_increase = MAX_HP` in actors.c
     - ✅ Extra life awarded when Shield collected with full HP (`comic_hp >= MAX_HP`)
     - ✅ Cross-module integration with extern declarations for `comic_hp_pending_increase`, `comic_num_lives`
     - ✅ `comic_takes_damage()` calls `decrement_comic_hp()` for proper animation
     - ✅ All changes committed to git (commit 0307d3d)
   - **Reference**: See `reference/disassembly/R5sw1991.asm` lines 5932-5987 for HP meter logic

### Phase 5: Sound and Input (Optional)

**Goal**: Convert remaining subsystems (lower priority)

1. **Sound system** ✅ **COMPLETE** - 2026-01-21
   - ✅ Note data structures defined in sound_data.h/sound_data.c
   - ✅ Sound playback state machine implemented in C (sound.c)
   - ✅ Priority system fully functional
   - ✅ PC speaker control via direct hardware access (no assembly needed)
   - ✅ All game sound effects integrated:
     - ✅ SOUND_FIRE - Fireball launch
     - ✅ SOUND_HIT_ENEMY - Enemy hit by fireball
     - ✅ SOUND_COLLECT_ITEM - Item collection
     - ✅ SOUND_DAMAGE - Shield break / player damage
     - ✅ SOUND_TOO_BAD - Death sequence
     - ✅ SOUND_GAME_OVER - Game over screen
     - ✅ SOUND_SCORE_TALLY - Victory score counting
     - ✅ SOUND_TITLE - Title and victory music
     - ✅ SOUND_TELEPORT - Teleportation effect
     - ✅ SOUND_MATERIALIZE - Beam in/out animations
     - ✅ SOUND_DOOR - Door transitions
     - ✅ SOUND_STAGE_EDGE_TRANSITION - Stage boundary crossing
   - ✅ `sound_advance_tick()` called from main game loop
   - ✅ Pure C implementation - no assembly code required

2. **Joystick handling** ⏭️ **SKIPPED** - 2026-01-21
   - **Decision**: Joystick support intentionally not implemented
   - **Rationale**: 
     - Very few users have physical joysticks for DOS games today
     - Keyboard controls are fully functional and preferred
     - Hardware-specific implementation adds complexity without modern value
   - **Already implemented in C** (CPU speed calibration only):
     - ✅ `calibrate_joystick()` - Measures CPU speed via timing loop (used for general timing, not joystick-specific)
     - ✅ `max_joystick_reads` variable - Stores calibrated timing value
     - ✅ `calibrate_joystick_interactive()` - Stub that displays message and returns
   - **Would still need for full joystick support**:
     - ❌ Axis reading via INT 15h (ah=84h, dx=1) or direct port 0x201 access
     - ❌ Button/switch reading via INT 15h (ah=84h, dx=0)
     - ❌ PIT counter timing for analog position measurement
     - ❌ Interactive calibration routine (center, left, right, up, down positions)
     - ❌ Threshold calculations (x_low, x_high, y_low, y_high)
     - ❌ Runtime polling in timer interrupt or main loop
     - ❌ Key state mapping (joystick axes → key_state_left/right/open/teleport)
     - ❌ Variables: `joystick_x_zero`, `joystick_y_zero`, `joystick_x_low`, `joystick_x_high`, `joystick_y_low`, `joystick_y_high`, `joystick_is_calibrated`
   - **Implementation reference**: See `reference/disassembly/R5sw1991.asm` lines 667-776 (calibration), 1282-1327 (INT 8 polling), 1517-1643 (INT 21 handler and axis reading)

3. **Interrupt handlers** ✅ **COMPLETE** - 2026-01-21
   - ✅ INT 8 (timer - sets `game_tick_flag`) - Implemented in C
   - ✅ INT 9 (keyboard - sets `key_state_*`) - Implemented in C
   - ✅ Pure C implementation using Open Watcom's `__interrupt` keyword
   - ✅ INT 8: Advances sound playback, sets game tick flag on odd interrupts
   - ✅ INT 9: Reads keyboard scancodes from port 0x60, stores in queue
   - ✅ Both handlers chain to original BIOS handlers using `_chain_intr()`
   - ✅ Proper interrupt installation/restoration with `_dos_getvect()` / `_dos_setvect()`
   - ✅ No assembly code required - direct hardware port access from C
   - **Implementation**: See [game_main.c](game_main.c#L469-L509) for `int8_handler` and `int9_handler`

### Phase 6: Testing and Validation (NOT STARTED)

**Goal**: Validate all implemented systems against original game behavior

1. **Testing Priorities**
   - Tier 1: Collision detection and physics accuracy
   - Tier 2: Actor spawning and AI behaviors
   - Tier 3: Item collection and score tracking
   - Tier 4: Full level playthroughs
   - Tier 5: Edge cases and original game bugs

2. **Test Methodology**

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
