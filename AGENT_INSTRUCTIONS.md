# AI Agent Instructions for Captain Comic Refactoring

## Project Context

You are working on refactoring the Captain Comic DOS game from x86 assembly to C. This is a 16-bit DOS real mode game originally written in assembly (~7,842 lines) that must be converted to C while maintaining **pixel-perfect compatibility** with the original.

**Critical Constraint:** Any change to game logic must produce identical visual output and timing as the original assembly version.

---

## Your Role

As an AI agent working on this codebase, you should:

1. **Understand the context** by reading these files:
   - `REFACTORING_PLAN.md` - Overall strategy and steps
   - `CODING_STANDARDS.md` - C coding conventions
   - `include/abi_conventions.h` - C/assembly interface rules
   - `include/structures.h` - Data structure definitions
   - `reference/disassembly/R5sw1991.asm` - Original assembly source

2. **Follow the incremental refactoring strategy**:
   - Convert one function at a time
   - Keep assembly wrappers intact
   - Validate pixel-perfect equivalence before proceeding

3. **Never break validation**:
   - All 5 test clips must pass before committing
   - If timing drift occurs, debug and fix before proceeding
   - Document any deviations in code comments

---

## Key Principles

### 1. Assembly is the Source of Truth
- The original assembly in `reference/disassembly/R5sw1991.asm` defines correct behavior
- When C behavior differs from assembly, **the assembly is right**
- Document assembly logic in wrapper comments before replacing

### 2. Pixel-Perfect Compatibility Required
- Visual output must be identical (validated via frame comparison)
- Timing must match (validated via DOSBox CPU cycles)
- No "close enough" - either identical or not done

### 3. Incremental Progress
- Convert small functions first (Tier 1: collision, geometry, items)
- Validate after each function conversion
- Don't proceed to complex functions until simple ones are proven

### 4. Assembly Shell Strategy
- Keep assembly structure intact with wrappers calling C
- Document original assembly logic in wrapper comments
- Easy rollback: comment out C call, uncomment original assembly

---

## Working with the Codebase

### Understanding Assembly Functions

When converting an assembly function to C:

1. **Read the assembly carefully**:
   ```nasm
   check_collision:
       mov ax, [player_x]
       mov bx, [player_y]
       ; ... logic ...
       ret
   ```

2. **Document the algorithm**:
   - What inputs (registers, memory)?
   - What outputs (registers, memory)?
   - What side effects?
   - What's the purpose?

3. **Create C equivalent**:
   ```c
   int check_collision(unsigned char x, unsigned char y) {
       // Algorithm: Check if tile at (x,y) is solid
       // Returns: 1 if collision, 0 if passable
       unsigned int tile_index = (y * MAP_WIDTH_TILES) + x;
       unsigned char tile_id = tile_map[tile_index];
       return (tile_id >= FIRST_SOLID_TILE);
   }
   ```

4. **Create assembly wrapper with documentation**:
   ```nasm
   check_collision:
       ; Original assembly logic:
       ; 1. Load player_x into AX, player_y into BX
       ; 2. Calculate tile_index = (y * 128) + x
       ; 3. Load tile_id from tile_map[tile_index]
       ; 4. Compare against FIRST_SOLID_TILE (ID 0x40)
       ; 5. Return 1 in AX if solid, 0 if passable
       ; Registers: AX=x, BX=y, returns AX=result
       ; Replaced: 2025-12-23 - Pure coordinate math, no hardware access
       
       extern _check_collision
       mov ax, [player_x]
       mov bx, [player_y]
       call _check_collision
       ret
   ```

### Testing Your Changes

After modifying C code:

1. **Build**:
   ```bash
   make clean
   make
   ```

2. **Run validation**:
   ```bash
   cd tests
   make test
   ```

3. **Check results**:
   - All 5 clips must pass (zero pixel differences)
   - CPU cycles must match original
   - If failure, examine diffs in `tests/diffs/clip_*/`

4. **Debug failures**:
   - Compare visual diffs to identify timing shifts vs logic bugs
   - Use DOSBox debugger to step through execution
   - Compare register states at breakpoints
   - Adjust C code until pixel-perfect match

5. **Clean up and commit**:
   ```bash
   rm -rf tests/diffs/*
   git add .
   git commit -m "Refactor: Convert check_collision to C"
   ```

---

## Coding Guidelines Summary

### Types
- Use `uint8_t`, `uint16_t` for exact-width types
- Avoid `long`, `float`, `double`
- Use `int` for booleans (0/1)

### Memory
- Small memory model: near pointers (2 bytes) by default
- Far pointers only for video memory: `unsigned char __far *`
- Static allocation only: no malloc/free
- Keep data under 64KB per segment

### Functions
- `__watcall` convention: parameters in AX, DX, BX, CX
- Return 0 for success, non-zero for error
- Preserve: BP, SI, DI, DS, SS
- May clobber: AX, BX, CX, DX, ES, FLAGS

### Style
- 4 spaces indentation
- Lowercase_with_underscores naming
- UPPERCASE_CONSTANTS
- Fine-grained includes
- Document WHY, not WHAT

### Validation
- Run `make test` before every commit
- All 5 clips must pass pixel-perfect
- Delete diffs after fixing: `rm -rf tests/diffs/*`

---

## Common Tasks

### Task: Convert Assembly Function to C

1. **Identify function** in `reference/disassembly/R5sw1991.asm`
2. **Understand algorithm**: read assembly carefully, note registers and logic
3. **Create C file** in `src/` (e.g., `src/collision.c`)
4. **Create header** in `include/` (e.g., `include/collision.h`)
5. **Write C implementation** matching assembly behavior
6. **Document assembly wrapper** with original logic
7. **Update Makefile** to compile new C file
8. **Test**: `make && make test`
9. **Debug if needed**: examine diffs, adjust C code
10. **Commit when passing**: `git commit`

### Task: Add New C Module

1. **Create source file**: `src/module.c`
2. **Create header file**: `include/module.h`
3. **Add to Makefile**:
   ```makefile
   OBJS += build/obj/module.obj
   
   build/obj/module.obj: src/module.c include/module.h
       $(WCC) $(WCCFLAGS) -fo=$@ $<
   ```
4. **Update assembly** to call C functions:
   ```nasm
   extern _module_function
   call _module_function
   ```
5. **Test and validate**

### Task: Debug Timing Drift

1. **Identify which clip fails**: usually higher complexity clips fail first
2. **Examine visual diffs**: `tests/diffs/clip_N/frame_XXXX.png`
3. **Look for patterns**:
   - Sprites shifted: timing drift (instruction scheduling)
   - Wrong sprites: logic bug (wrong algorithm)
   - Visual glitches: memory corruption or wrong blitting
4. **Use DOSBox debugger**:
   - Set breakpoint at function entry
   - Compare register values between assembly and C versions
   - Step through instruction-by-instruction
5. **Adjust C code**:
   - Reorder operations to match assembly
   - Check compiler output: `wcc -d1 -fo=module.obj module.c`
   - Try `-O0` if not already used
6. **Retest**: `make test`

### Task: Document Assembly Wrapper

When replacing assembly with C call:

```nasm
; Function: check_collision
; Purpose: Determine if player collides with map tile at (x,y)
;
; Original Algorithm:
;   1. Load coordinates: AX=x, BX=y
;   2. Validate bounds: x < 128, y < 10
;   3. Calculate index: (y << 7) + x  (y * 128 + x)
;   4. Load tile ID from tile_map[index]
;   5. Compare: tile_id >= 0x40 (FIRST_SOLID_TILE)
;   6. Return AX=1 (collision) or AX=0 (no collision)
;
; Registers Used:
;   Input:  AX = x coordinate (0-127)
;           BX = y coordinate (0-9)
;   Output: AX = collision flag (0 or 1)
;   Clobbered: CX, DX (temporary calculations)
;   Preserved: SI, DI, BP, DS
;
; Side Effects: None (pure function)
;
; Replaced: 2025-12-23
; Reason: Pure coordinate math, no hardware access, easy to test
; Complexity: Tier 1 (simple logic)
;
check_collision:
    ; Call C version
    extern _check_collision
    call _check_collision
    ret
```

---

## What to Avoid

### ❌ Don't Do This

1. **Change assembly without documenting**: Always comment original logic before replacing
2. **Commit without validation**: Must pass all 5 test clips
3. **Optimize prematurely**: Use `-O0` until proven working
4. **Assume compatibility**: Always test, never assume "it should work"
5. **Delete assembly wrappers**: Keep them indefinitely as reference
6. **Refactor multiple functions at once**: One function at a time
7. **Skip documentation**: Every wrapper needs full algorithm documentation
8. **Use modern C features**: No C99/C11 features, stick to C89
9. **Allocate on heap**: No malloc/free, use static arrays
10. **Ignore timing drift**: Must debug and fix, not proceed

### ✅ Do This

1. **Read assembly carefully** before writing C
2. **Document algorithm** in wrapper comments
3. **Validate after every change**: `make test`
4. **Start with Tier 1 functions**: collision, geometry, items
5. **Use exact-width types**: `uint8_t`, `uint16_t`
6. **Follow calling convention**: `__watcall`, parameters in registers
7. **Preserve assembly structure**: wrappers call C
8. **Debug systematically**: examine diffs, use debugger
9. **Commit frequently**: small, validated changes
10. **Ask questions**: consult assembly source when unclear

---

## Testing Workflow

### Every Change Must Follow This Process

```bash
# 1. Make changes to C code
vim src/collision.c

# 2. Build
make clean && make

# 3. Run validation
cd tests
make test

# 4. Check results
# - All clips pass: Proceed to commit
# - Some clips fail: Debug (see below)

# 5. Debug if needed
# Examine diffs
open diffs/clip_1/frame_0001.png

# Compare with original
# Use DOSBox debugger to step through

# Adjust C code
vim ../src/collision.c

# Rebuild and retest
cd .. && make && cd tests && make test

# 6. Clean up and commit when passing
rm -rf diffs/*
cd ..
git add src/collision.c include/collision.h
git commit -m "Refactor: Convert check_collision to C

- Tier 1 function: pure coordinate math
- Validated pixel-perfect on all 5 clips
- Preserved assembly wrapper with documentation"

# 7. Push (if on team project)
git push
```

---

## Validation Checklist

Before committing any change:

- [ ] Code compiles without warnings: `make`
- [ ] All 5 test clips pass: `make test`
- [ ] Zero pixel differences in all frames
- [ ] CPU cycles match original (check test output)
- [ ] Assembly wrapper documents original logic
- [ ] C code follows style guide (see `CODING_STANDARDS.md`)
- [ ] Function header documents behavior
- [ ] Diff images deleted: `rm -rf tests/diffs/*`
- [ ] Commit message describes change clearly

---

## File Reference Quick Guide

### Project Files
- `REFACTORING_PLAN.md` - Overall strategy
- `CODING_STANDARDS.md` - C style guide
- `AGENT_INSTRUCTIONS.md` - This file
- `README.md` - Project overview and build instructions

### Source Code
- `src/*.c` - C implementation files
- `include/*.h` - C header files
- `reference/disassembly/R5sw1991.asm` - Original assembly (7,842 lines)

### Build System
- `Makefile` - Main build system
- `tests/Makefile` - Validation automation
- `tests/dosbox_deterministic.conf` - DOSBox config for testing

### Validation
- `tests/golden/clip_{1-5}/input.rec` - Test recordings
- `tests/golden/clip_{1-5}/frames/` - Reference frames (generated)
- `tests/diffs/` - Diff images (generated on failure)
- `tests/compare_frames.py` - Frame comparison script

### Build Artifacts (not in git)
- `build/obj/*.obj` - Compiled object files
- `build/comic.exe` - Final executable
- `tests/golden/clip_*/frames/` - Generated frames
- `tests/diffs/` - Generated diff images

---

## Getting Started

When you begin working on this project:

1. **Read these documents**:
   - `REFACTORING_PLAN.md` (overall strategy)
   - `CODING_STANDARDS.md` (C conventions)
   - This file (`AGENT_INSTRUCTIONS.md`)

2. **Set up environment**:
   ```bash
   brew install dosbox-x open-watcom-v2 ffmpeg
   export WATCOM=$(brew --prefix open-watcom-v2)
   export PATH=$WATCOM/binl64:$PATH
   ```

3. **Build original**:
   ```bash
   cd reference/disassembly
   make R5sw1991.exe
   ```

4. **Record test clips** (if not done):
   ```bash
   # See REFACTORING_PLAN.md Step 1
   dosbox-x -conf tests/dosbox_deterministic.conf
   # Record 5 clips with reckey start/stop
   ```

5. **Start with Tier 1**:
   - Pick a simple function (e.g., `address_of_tile_at_coordinates`)
   - Read assembly, understand algorithm
   - Write C implementation
   - Test and validate
   - Commit when passing

---

## Questions and Support

### When Stuck

1. **Consult assembly source**: `reference/disassembly/R5sw1991.asm`
2. **Check wrapper documentation**: Assembly files have documented wrappers
3. **Examine test failures**: Visual diffs show what changed
4. **Use DOSBox debugger**: Step through execution
5. **Review similar functions**: See how others were converted

### Common Issues

**Issue:** C function doesn't match assembly behavior
- **Solution:** Read assembly more carefully, check register usage, verify algorithm

**Issue:** Timing drift (sprites shifted in frames)
- **Solution:** Use `-O0`, reorder operations to match assembly, check instruction scheduling

**Issue:** Visual glitches or corruption
- **Solution:** Check pointer arithmetic, verify segment:offset addressing, check buffer bounds

**Issue:** Build errors
- **Solution:** Check includes, verify types, ensure `-ml -s -0 -O0` flags

**Issue:** Validation test fails
- **Solution:** Examine `tests/diffs/`, compare with reference, debug C code

---

## Remember

- **Assembly is always right** - match its behavior exactly
- **Pixel-perfect is required** - close doesn't count
- **Validate frequently** - test after every change
- **Document thoroughly** - future you (and others) will thank you
- **Progress incrementally** - one function at a time
- **Keep assembly wrappers** - they're your safety net

Good luck with the refactoring! 🎮
