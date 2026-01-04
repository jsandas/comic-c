# comic-c

C language refactor of **The Adventures of Captain Comic** - a 1990 DOS platformer originally written in x86-16 assembly.

This project converts the assembly codebase to C while maintaining exact behavioral compatibility with the original game (Revision 5).

## Project Status

**Phase 1: Foundation** ✅ COMPLETE
- Local Open Watcom 2 build environment setup
- C language entry point and initialization
- Data file loader converted (`.PT` file format)
- Level data migration (LAKE, FOREST complete)

**Phase 2: Game Logic** 🔄 IN PROGRESS
- Level data migration and file loading infrastructure
- Next: Game loop implementation, rendering system, physics

## Quick Start

### Prerequisites
- Open Watcom 2 (installed locally in `watcom2/` directory)
- Make
- Game assets from: https://github.com/jsandas/comic
  - Place `.SHP`, `.TT2`, `.PT`, `.EGA` files in `reference/original/`

### Build Instructions

```bash
# Setup environment (one time)
source setvars.sh    # Configure Open Watcom paths

# Compile the project
make compile         # Compiles using local Open Watcom 2

# Run the game (in DOSBox)
cd tests
./run-dosbox.sh      # Launches DOSBox-X with the compiled executable
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
cd tests && ./run-dosbox.sh

# Clean build artifacts
make clean
```

## Project Structure

- **`setvars.sh`** - Environment setup script for Open Watcom 2
- **`Makefile`** - Build targets (`compile`, `clean`)
- **`include/`** - C header files
  - `globals.h` - Game state variables and data structures
  - `file_loaders.h` - Data file format definitions
  - `graphics.h` - Graphics and rendering functions
  - `level_data.h` - Level data structures
- **`src/`** - C source files
  - `game_main.c` - Main entry point and game initialization
  - `file_loaders.c` - Data file loaders
  - `graphics.c` - Graphics and rendering
  - `level_data.c` - Level definitions
- **`build/`** - Build artifacts (generated)
  - `obj/` - Object files
  - `COMIC.EXE` - Final DOS executable
- **`reference/`** - Original assembly reference
  - `disassembly/R5sw1991.asm` - Fully commented disassembly
  - `disassembly/djlink/` - OMF format linker
  - `original/` - Game assets (`.SHP`, `.TT2`, `.PT`, `.EGA` files)
- **`tests/`** - Testing framework
  - `savestates/` - DOSBox save states (1.sav-4.sav)
  - `scenarios/` - Test scenario documentation
  - `dosbox_deterministic.conf` - Reproducible DOSBox config
- **`docs/`** - Documentation
  - `REFACTOR_PLAN.md` - Overall strategy and roadmap
  - `CODING_STANDARDS.md` - C code style guide
  - `AGENT_INSTRUCTIONS.md` - AI assistant guidelines

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
