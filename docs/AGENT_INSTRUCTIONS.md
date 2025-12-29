# Agent Instructions for Captain Comic C Refactor

## Project Overview

This project refactors The Adventures of Captain Comic (1990 DOS game) from x86-16 assembly to C, maintaining exact behavioral compatibility while keeping the code functional in 16-bit DOS real-mode.

## Key Principles

### 1. Behavioral Fidelity is Paramount
- The refactored C code must produce **exactly** the same behavior as the original assembly
- Even known bugs should be preserved for compatibility
- Use DOSBox save-state testing to validate each conversion
- Test scenarios are in `tests/scenarios/` with save states in `tests/savestates/`

### 2. Build Environment
- **Compiler**: Open Watcom C 16-bit v2.0 (beta)
- **Memory Model**: Large model (`-ml` flag) - separate code and data segments
- **Assembler**: NASM for remaining assembly code
- **Linker**: djlink (custom OMF format linker in `reference/disassembly/djlink/`)
- **Platform**: Docker container on macOS (no native Open Watcom for macOS)

### 3. Architecture Strategy
- **Assembly stub initialization**: Entry point, interrupt handlers, video setup remain in assembly
- **C main loop**: Game logic moved to C (`game_main()` function)
- **Incremental conversion**: Keep assembly functions, call from C, convert one by one
- **No crt0.obj**: Use minimal assembly stub, not Open Watcom runtime (saves overhead)

### 4. C/Assembly Integration

#### Global Variables
- Assembly exports globals with `global` directive (e.g., `global comic_x`)
- C declares with `extern` and `#pragma aux "*"` to prevent name mangling:
  ```c
  #pragma aux comic_x "*"
  extern uint8_t comic_x;
  ```

#### Function Calls
- Assembly functions called from C: export with `global`, declare in C with `#pragma aux "*"`
- C functions called from assembly: use underscore prefix `_game_main`
- Calling convention: cdecl (caller cleans stack)

#### Data Segment (DS)
- Assembly sets DS once at startup to point to data segment
- C code assumes DS is correct (large model)
- **Never change DS in C without restoring it immediately**

### 5. Conversion Priority Order

**Phase 1: Foundation** (CURRENT)
1. ✅ Docker build environment
2. ✅ Assembly stub calling C `game_main()`
3. ✅ Minimal C wrapper calling assembly functions
4. ✅ Global variable exports and header files
5. 🔄 Data file loaders (`.PT` → `.TT2` → `.SHP` → `.EGA`)

**Phase 2: Game Loop**
1. Extract game loop tick logic to C
2. Keep calling assembly for actors/rendering initially
3. Handle input state in C (reading `key_state_*` globals)

**Phase 3: Subsystems**
1. Rendering functions (blitting, video buffer management)
2. Physics and collision detection
3. Player movement and state management
4. Fireball handling

**Phase 4: AI and Complex Logic**
1. Enemy AI behaviors (bounce, leap, roll, seek, shy)
2. Item collection and inventory
3. Door and teleportation mechanics
4. Level/stage transitions

**Phase 5: Cleanup** (OPTIONAL)
1. Consider converting interrupt handlers to C (low priority)
2. Optimize and refactor for maintainability
3. Document any remaining quirks or limitations

### 6. File Organization

```
comic-c/
├── Dockerfile              # Docker build environment
├── Makefile                # Build targets (docker-build, compile, clean)
├── include/                # C header files
│   ├── globals.h          # Assembly global variable declarations
│   ├── assembly.h         # Assembly function declarations
│   └── file_loaders.h     # Data file format definitions
├── src/                    # C source files
│   ├── R5sw1991_c.asm     # Modified assembly (exports + calls C)
│   ├── game_main.c        # C game loop entry point
│   └── file_loaders.c     # Data file loading functions
├── build/                  # Build artifacts (generated)
│   ├── obj/               # Object files
│   └── COMIC.EXE          # Final executable
├── reference/              # Original assembly reference
│   └── disassembly/
│       ├── R5sw1991.asm   # Original commented disassembly
│       └── djlink/        # OMF linker
└── tests/                  # Testing framework
    ├── scenarios/         # Test scenario documentation
    ├── savestates/        # DOSBox save states (1.sav-4.sav)
    └── capture/           # Reference screenshots
```

### 7. Testing Workflow

1. **Make changes** to C or assembly code
2. **Compile** with `make compile` (runs in Docker)
3. **Copy executable** to `tests/` directory
4. **Run DOSBox** with deterministic config: `./run-dosbox.sh`
5. **Load save state** (F7 key in DOSBox-X)
6. **Execute test actions** as documented in scenario file
7. **Verify behavior** matches expected outcome
8. **Document results** in `tests/validation_log.txt`

### 8. Common Pitfalls to Avoid

❌ **Don't use Open Watcom crt0.obj** - it adds overhead and conflicts with assembly
❌ **Don't modify DS register** in C code without careful restoration
❌ **Don't assume standard C library** - we're in DOS real-mode, not modern OS
❌ **Don't optimize prematurely** - behavioral fidelity first, performance later
❌ **Don't skip testing** - even "simple" conversions can have subtle bugs

### 9. When Stuck or Uncertain

1. **Check the original assembly** in `reference/disassembly/R5sw1991.asm`
2. **Look for similar patterns** already converted (collision detection functions)
3. **Test incrementally** - make smallest possible changes, test immediately
4. **Document uncertainties** - add comments about unclear behavior
5. **Preserve assembly version** - keep original in comments for reference

### 10. Code Style and Standards

See `docs/CODING_STANDARDS.md` for detailed C coding conventions.

Key points:
- Use explicit types: `uint8_t`, `uint16_t`, `int8_t`
- Document assumptions about DOS environment
- Comment unusual patterns or bug-for-bug compatibility
- Keep functions small and focused
- Use meaningful variable names (assembly labels often cryptic)

## Questions or Issues?

Refer to:
- `docs/REFACTOR_PLAN.md` - Overall strategy and roadmap
- `docs/CODING_STANDARDS.md` - C code style guide
- `reference/disassembly/R5sw1991.asm` - Original assembly reference
- `tests/HOW_TO_CREATE_SCENARIOS.md` - Testing methodology
