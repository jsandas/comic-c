# comic-c

C language refactor of **The Adventures of Captain Comic** - a 1990 DOS platformer originally written in x86-16 assembly.

This project converts the assembly codebase to C while maintaining exact behavioral compatibility with the original game (Revision 5).

## Project Status

**Overall** ✅ Phase 4 complete (core game systems and UI rendering implemented)

**Implemented**
- Full game loop, rendering, physics, and collision
- Actor systems (enemies, fireballs, items) with AI behaviors
- Interrupt handlers and PC speaker sound system
- Sprite loading and rendering (SHP, items, effects)
- Level data for all 8 levels and level/stage loading
- UI rendering (score, inventory, firepower, HP/shield)

**Current focus** 🔄 Testing and validation against the original game

## Quick Start

### Prerequisites
- Open Watcom 2 (installed locally in `watcom2/` directory)
- Make
- Download original revision 5 game (https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeathR5sw1991michaelA.Denioaction.zip) 
and extract contents to `reference/original/`

### Build Instructions

```bash
# Setup environment (one time)
source setvars.sh    # Configure Open Watcom paths

# Compile the project
make compile         # Compiles using local Open Watcom 2

# Run the game (in DOSBox)
./tests/run-dosbox.sh      # Launches DOSBox-X with the compiled executable
```

### Development Workflow

```bash
# Ensure environment is configured (if needed)
source setvars.sh

# Edit C source files
vim src/game_main.c

# Recompile
make compile

# Test in DOSBox
./tests/run-dosbox.sh

# Clean build artifacts
make clean
```

## Project Structure

- **`setvars.sh`** - Environment setup script for Open Watcom 2
- **`Makefile`** - Build targets (`compile`, `clean`)
- **`include/`** - C headers for core systems
  - `globals.h` - Shared game state
  - `actors.h`, `physics.h`, `doors.h` - Gameplay systems
  - `graphics.h`, `sprite_data.h` - Rendering and sprite data
  - `sound.h`, `sound_data.h`, `music.h` - Audio system
  - `level_data.h`, `file_loaders.h` - Level definitions and file formats
- **`src/`** - C source files
  - `game_main.c` - Entry point, game loop, level loading
  - `actors.c`, `physics.c`, `doors.c` - Gameplay systems
  - `graphics.c`, `sprite_data.c` - Rendering and sprite data
  - `sound.c`, `sound_data.c`, `music.c` - Audio system
  - `file_loaders.c`, `level_data.c` - File loading and level data
- **`build/`** - Build artifacts (generated)
  - `obj/` - Object files
  - `COMIC.EXE` - Final DOS executable
- **`reference/`** - Original assembly reference and assets
  - `disassembly/R5sw1991.asm` - Fully commented disassembly
  - `disassembly/djlink/` - OMF format linker
  - `original/` - Game assets (`.SHP`, `.TT2`, `.PT`, `.EGA` files)
- **`tests/`** - Testing and validation
  - `dosbox_deterministic.conf` - Reproducible DOSBox config
  - `savestates/` - DOSBox save states
  - `scenarios/` - Test scenario documentation
  - `run-dosbox.sh` - DOSBox launcher
- **`docs/`** - Documentation
  - `REFACTOR_PLAN.md` - Overall strategy and roadmap
  - `CODING_STANDARDS.md` - C code style guide
  - `GAME_LOOP_FLOW.md` - Game loop and behavior notes
- **`utils/`** - Development helpers (asset conversion scripts)
- **`watcom2/`** - Bundled Open Watcom toolchain

## Architecture

### C Language Implementation
- Pure C implementation for all game logic
- Open Watcom 16-bit compiler targeting DOS real-mode
- Entry point: `main()` in `src/game_main.c`
- Modular subsystems:
  - Game initialization and menus
  - Level loading and management
  - Rendering (video buffer, sprites, tiles)
  - Physics and collision detection
  - Game loop and input handling

### Original Assembly (Reference Only)
- `reference/disassembly/R5sw1991.asm` - 7,842 lines for historical reference
- Not compiled or used in the refactored version
- Preserved for inspection and understanding original behavior

## Technical Details

### Compiler and Linker
- **Compiler**: Open Watcom C 16-bit v2.0 (`wcc`)
- **Flags**: `-ml` (large memory model), `-s` (no stack checks), `-0` (8086 target)
- **Linker**: wlink (Open Watcom OMF format linker)

### Memory Model
- **Large model**: Separate 64 KB code and data segments
- **Calling convention**: cdecl (caller cleans stack) 

## Testing

### Methodology
1. Create test scenario in `tests/scenarios/`
2. Prepare DOSBox save state at checkpoint
3. Document expected behavior
4. Run test in DOSBox-X with deterministic config
5. Load save state (F7), execute test actions
6. Verify behavior matches original
7. Document results in `tests/validation_log.txt`

### Test Tiers
- **Tier 1**: Pure logic (collision detection)
- **Tier 2**: Data/state (file loading, initialization)
- **Tier 3**: Complex interactions (item collection, doors)
- **Tier 4**: AI and physics (enemy behaviors, movement)
- **Tier 5**: Full gameplay (level transitions, complete playthroughs)

## Contributing

See documentation:
- `docs/REFACTOR_PLAN.md` - Overall strategy and phases
- `docs/CODING_STANDARDS.md` - C code conventions
- `docs/AGENT_INSTRUCTIONS.md` - AI assistant guidelines
- `tests/HOW_TO_CREATE_SCENARIOS.md` - Testing methodology

## Resources

- **Original game**: https://github.com/jsandas/comic
- **Game info**: https://tcrf.net/The_Adventures_of_Captain_Comic_(DOS)
- **File formats**: http://www.shikadi.net/moddingwiki/Captain_Comic
- **Open Watcom**: https://open-watcom.github.io/open-watcom-v2-wikidocs/
- **NASM**: https://www.nasm.us/doc/

## License

This is a preservation and educational project. Original game copyright © 1990 Michael A. Denio.


### DOSBox Debugger
Opt + F12 starts debugger

C <segment>:<offset> - get disassembly code
s <segment>:<offset> - get stack


### Install Open Watcom2

Download latest tarball `ow-snapshot.tar.xz` from https://github.com/open-watcom/open-watcom-v2/tags

Extract into folder `watcom2` in project folder
