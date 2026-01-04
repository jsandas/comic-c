# comic-c

C language refactor of **The Adventures of Captain Comic** - a 1990 DOS platformer originally written in x86-16 assembly.

This project converts the assembly codebase to C while maintaining exact behavioral compatibility with the original game (Revision 5).

## Project Status

**Phase 1: Foundation** ✅ COMPLETE
- Docker build environment with Open Watcom 2
- C/Assembly integration infrastructure (legacy)
- Minimal C game loop with minimized legacy assembly usage
- First data file loader converted (`.PT` file format)

**Next Phase**: Game loop migration from assembly to C

## Quick Start

### Prerequisites
- Docker (for macOS/Linux - provides Open Watcom 2 build environment)
- Make
- Game assets from: https://github.com/jsandas/comic
  - Place `.SHP`, `.TT2`, `.PT`, `.EGA` files in `reference/original/`

### Build Instructions

```bash
# First time setup
make docker-build    # Build Docker image with Open Watcom 2 + NASM

# Compile the project
make compile         # Runs compilation inside Docker container

# Run the game (in DOSBox)
cd tests
./run-dosbox.sh      # Launches DOSBox-X with the compiled executable
```

### Development Workflow

```bash
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

- **`Dockerfile`** - Docker image with Open Watcom 2, NASM, djlink
- **`Makefile`** - Build targets (`docker-build`, `compile`, `clean`, `shell`)
- **`include/`** - C header files
  - `globals.h` - Assembly global variable declarations
  - `assembly.h` - Assembly function declarations
  - `file_loaders.h` - Data file format definitions
- **`src/`** - C source files
  - `R5sw1991_c.asm` - Modified assembly (initialization + calls C)
  - `game_main.c` - C game loop entry point
  - `file_loaders.c` - Data file loaders
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

### Original Assembly (R5sw1991.asm)
- 7,842 lines of x86-16 assembly
- Pure DOS real-mode, 16-bit
- Custom interrupt handlers (INT 3, 8, 9)
- Direct hardware access (EGA video, PC speaker, keyboard)

### Refactored C + Assembly Hybrid
```
Assembly Stub (R5sw1991_c.asm)          C Code (src/*.c)
┌─────────────────────────┐            ┌────────────────────┐
│ • Entry point (main:)   │            │ • game_main()      │
│ • DS initialization     │───calls───►│ • Game loop        │
│ • Interrupt handlers    │            │ • Input handling   │
│ • Hardware init         │            │ • Game logic       │
│ • Title sequence        │◄──calls────│ • File loading     │
└─────────────────────────┘            └────────────────────┘
         │                                      │
         └──────────────┬───────────────────────┘
                        │
                        ▼
           ┌────────────────────────────────┐
           │ Assembly Reference (legacy)    │
           │ (preserved for inspection only)│
           │                                │
           │ • load_new_level() (legacy)     │
           │ • handle_enemies() (legacy)     │
           │ • swap_video_buffers() (legacy) │
           │ • etc.                           │
           └────────────────────────────────┘
```

### Why Not Use Open Watcom crt0.obj?
- **Size overhead**: Adds 2-4 KB to 33 KB original executable
- **Conflicts**: Assembly already has complete initialization
- **Control**: Custom interrupt handlers need precise setup
- **Solution**: Minimal assembly stub, full control over environment

## Technical Details

### Compiler and Linker
- **Compiler**: Open Watcom C 16-bit v2.0 (`wcc`)
- **Flags**: `-ml` (large memory model), `-s` (no stack checks), `-0` (8086 target)
- **Assembler**: NASM (OMF object format output)
- **Linker**: djlink (custom DOS OMF linker)

### Memory Model
- **Large model**: Separate 64 KB code and data segments
- **DS register**: Points to data segment (ensure DS is correct; legacy assembly previously set it)
- **Calling convention**: cdecl (caller cleans stack)

### C/Assembly Integration (legacy)
- Legacy assembly may export globals: `global comic_x` (reference only)
- C may declare corresponding externs: `#pragma aux comic_x "*"` + `extern uint8_t comic_x;` (prefer migrating globals to C)
- Legacy assembly exports functions (for reference): `global load_new_level`
- Historically C called assembly entrypoints; prefer C equivalents instead of adding new assembly
- C functions called from legacy assembly: `call _game_main` (historical detail) 

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
