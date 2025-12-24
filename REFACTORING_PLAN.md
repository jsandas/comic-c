# Captain Comic R5 Assembly to C Refactoring Plan

## Goal

Convert the Captain Comic Revision 5 (sw1991) 16-bit DOS game from x86 assembly to C using Open Watcom compiler, maintaining functional compatibility with the original through incremental refactoring and validation using DOSBox-X checkpoint testing.

## Implementation Steps

### Step 1: Create Test Scenarios and DOSBox Checkpoints

**Objective:** Establish functional test scenarios and save states for validation.

**Tasks:**
- Install DOSBox-X: `brew install dosbox-x`
- Create `tests/dosbox_deterministic.conf` with:
  - `core=normal`
  - `cycles=fixed 3000`
  - `aspect=false`
  - `scaler=none`
- Run `reference/orig/R5sw1991/COMIC.EXE`
- Create 5 test scenarios ordered by complexity:
  1. **Scenario 1:** Collision detection (simplest - tile collision)
  2. **Scenario 2:** Initialization (game start, spawn positions)
  3. **Scenario 3:** Item pickup (item interaction)
  4. **Scenario 4:** Enemy encounter (AI/combat)
  5. **Scenario 5:** Door transition (level loading)
- Create DOSBox save states at key test points for each scenario
- Document test scenarios with expected behaviors for functional validation

**Deliverables:**
- DOSBox-X save states in `tests/savestates/`
- Test scenario documentation in `tests/scenarios/`
- Functional validation log template in `tests/validation_log.txt`

---

### Step 2: Install and Configure Open Watcom

**Objective:** Set up C compilation toolchain.

**Tasks:**
- Install Open Watcom: `brew install open-watcom-v2`
- Set environment variables:
  ```bash
  export WATCOM=$(brew --prefix open-watcom-v2)
  export PATH=$WATCOM/binl64:$PATH
  ```
- Create `Makefile` with:
  - Watcom's native dependency tracking (via `wcc -d`)
  - Compile `.c` files to OMF `.obj` in `build/obj/`
  - Compiler flags: `wcc -ml -s -0 -O0`
    - `-ml` = small memory model
    - `-s` = no stack checking
    - `-0` = 8086 target
    - `-O0` = no optimization (for determinism)
  - Output executable to `build/comic.exe`
- Test djlink integration
- Update `README.md` with installation, build, and validation instructions

**Deliverables:**
- Working `Makefile`
- Updated `README.md`
- Verified Open Watcom → djlink toolchain

---

### Step 3: Set Up Project Structure

**Objective:** Organize codebase for mixed C/assembly development.

**Tasks:**
- Create directories:
  - `src/` - C source code
  - `include/` - C header files
  - `build/obj/` - Compiled object files
  - `build/` - Final executable
  - `tests/savestates/` - DOSBox save states
  - `tests/scenarios/` - Test scenario documentation
- Update `.gitignore`:
  - Exclude `build/`
  - Keep `reference/` pristine (no build artifacts)
  - Commit save states (for testing)

**Deliverables:**
- Directory structure
- Updated `.gitignore`

---

### Step 4: Document Calling Conventions and Data Structures

**Objective:** Define C/assembly interface and data model.

**Tasks:**
- Create `include/abi_conventions.h`:
  - Specify `__watcall` as standard calling convention
  - Document register usage:
    - Parameters: AX, DX, BX, CX (in order)
    - Return values: AX (16-bit), DX:AX (32-bit)
    - Preserved: BP, SI, DI, DS, SS
    - Clobberable: AX, BX, CX, DX, ES, FLAGS
  - Small memory model with near pointers default
  - Far pointers only for video memory access (segment 0xA000)
  - Error handling via return codes (0=success, non-zero=error)
- Create `include/structures.h`:
  - C struct definitions from `reference/disassembly/R5sw1991.asm`:
    - `enemy` (position, velocity, animation, behavior, direction)
    - `fireball` (position, velocity, animation)
    - `door` (position, destination)
    - `stage` (tiles, items, exits)
    - `level` (filename, doors, stages, enemies)
    - `pt` (map tiles)
    - `shp` (sprite graphics metadata)

**Deliverables:**
- `include/abi_conventions.h`
- `include/structures.h`

---

### Step 5: Set Up Validation Framework

**Objective:** Establish functional testing approach using DOSBox-X checkpoints.

**Tasks:**
- Create functional test scenario documents in `tests/scenarios/`:
  - `scenario_1_collision.md` - Tile collision detection
  - `scenario_2_init.md` - Game initialization and spawn positions
  - `scenario_3_items.md` - Item collection and inventory
  - `scenario_4_enemy.md` - Enemy AI and movement patterns
  - `scenario_5_transitions.md` - Level loading and door entry/exit
- Create DOSBox-X save states at key test points:
  - `tests/savestates/level1_start.sav` - Fresh level 1 start
  - `tests/savestates/level1_with_enemy.sav` - Enemy present
  - `tests/savestates/level1_near_item.sav` - Near collectible item
- Create `tests/validation_log.txt` for documenting results
- Manual testing procedure: load state, perform action, compare results with assembly version

**Test Workflow:**
1. Load save state in original assembly (reference)
2. Perform test action (move player, collect item, etc.)
3. Document result (position, health, score, etc.)
4. Load identical save state in C version
5. Perform identical action
6. Compare results
7. Mark PASS (results match) or FAIL (results differ)

**Deliverables:**
- Scenario documentation files
- DOSBox save states for testing
- Validation log template

---

### Step 6: Refactor Tier 1 Functions with Documented Assembly Shell

**Objective:** Convert simple logic functions to C while maintaining assembly structure.

**Tier 1 Functions (Easy - Pure logic, no I/O):**
- Collision detection: `address_of_tile_at_coordinates`, `check_vertical_map_collision`, `check_horizontal_map_collision`
- Coordinate transformations
- Item collection: `collect_item`, item type handlers
- Sprite animation frame calculation

**Tasks:**
- Create C modules by domain:
  - `src/collision.c` - Collision detection functions
  - `src/items.c` - Item collection logic
  - `src/geometry.c` - Coordinate math
- Use fine-grained includes (only needed headers per module)
- Write C implementations matching assembly behavior
- Keep assembly structure intact with wrapper functions:
  ```nasm
  ; Assembly wrapper (stays indefinitely)
  check_collision:
      ; Original assembly code (commented for reference)
      ; mov ax, [player_x]
      ; mov bx, [player_y]
      ; ... original logic ...
      ; 
      ; Logic: Check if player at (x,y) collides with map tile
      ; Registers used: AX=x, BX=y, returns AX=1 (collision) or 0
      ; Replaced: 2025-12-23 - Pure coordinate math, no side effects
      
      ; Call C version
      extern _check_collision_c
      call _check_collision_c
      ret
  ```
- Add headers with fine-grained includes:
  - `include/collision.h`
  - `include/items.h`
  - `include/geometry.h`
- Update `Makefile` with Watcom dependency rules
- Ensure all C compilation uses `-O0` flag

**Deliverables:**
- `src/collision.c`, `src/items.c`, `src/geometry.c`
- Corresponding headers
- Documented assembly wrappers
- Updated `Makefile`

---

### Step 7: Validate Before Each Commit with Functional Testing

**Objective:** Ensure functional compatibility at each step.

**Validation Workflow:**

For each C function converted:

1. **Test with saved checkpoint:**
   - Load save state from `tests/savestates/`
   - Run original assembly version
   - Perform test action (move, collect item, etc.)
   - Document result (position, health, score)

2. **Test with C version:**
   - Load same save state
   - Perform identical action
   - Compare results

3. **If results match:**
   - Commit with message describing function conversion

4. **If results differ:**
   - Debug C code logic
   - Use DOSBox debugger to inspect state if needed
   - Compare with assembly algorithm
   - Adjust and retry

5. **Document in validation log:**
   ```
   Function: check_collision
   Status: PASS
   Assembly Result: Player X=20 (blocked by wall)
   C Version Result: Player X=20 (blocked by wall)
   Date: 2025-12-23
   ```

**Commit Criteria:**
- C function behavior matches assembly behavior
- Manual testing confirms compatibility
- Validation log updated

**Rollback Strategy:**
- If C function behavior doesn't match assembly, revert C code or toggle assembly wrapper
- Keep assembly version as source of truth
- Document issue in code comments for future reference

---

### Step 8: Progress Through Tier 2 and 3

**Objective:** Gradually refactor more complex functions.

**Tier 2 Functions (Medium - Data movement, some state):**
- Level/stage loading: `load_new_level`, `load_new_stage`, `load_pt_file`, `load_shp_files`
- Enemy initialization
- Game initialization

**Tier 3 Functions (Hard - Complex control flow):**
- Game loop: `game_loop`
- Player physics: `handle_fall_or_jump`
- Enemy AI: `handle_enemies`, behavior dispatch
- Fireball handling: `handle_fireballs`

**Tier 4 Functions (Very Hard - Hardware coupled, LEAVE IN ASSEMBLY):**
- Interrupt handlers: `int8_handler` (timer), `int9_handler` (keyboard), `int3_handler` (sound)
- EGA plane manipulation
- Video blitting (critical performance path)

**Tasks:**
- Refactor modules in tiers:
  - Tier 1: `src/collision.c`, `src/geometry.c`, `src/items.c` (pure logic)
  - Tier 2: `src/loading.c`, `src/initialization.c` (resource loading)
  - Tier 3: `src/physics.c`, `src/enemy.c` (complex state machines)
- Use fine-grained includes
- Maintain documented assembly shell
- Test each function with functional validation (checkpoints, manual testing)
- Leave Tier 4 functions (interrupts, EGA blitting) in assembly
- Document any functional differences in code comments

**Deliverables:**
- Tier 1-3 C implementations
- Corresponding headers
- Updated validation log with all test results
- All validation tests passing
- Documented assembly shell remains

---

## Key Principles

### Memory Model
- **Small memory model**: near pointers (2 bytes) for code and data
- **Far pointers**: only for video memory access (segment 0xA000)
- **Avoid segment crossing**: keep data within 64KB segments

### Calling Convention
- **Standard**: `__watcall` (register-based)
- **Parameters**: AX, DX, BX, CX (first 4 args)
- **Return**: AX (16-bit), DX:AX (32-bit)
- **Preserved**: BP, SI, DI, DS, SS

### Compilation Flags
- `-ml` = small memory model
- `-s` = disable stack checking (critical for minimal DOS code)
- `-0` = 8086 target (not 386+)
- `-O0` = no optimization (for determinism)

### Validation Strategy
- **Functional testing**: Behavior equivalence via checkpoint testing
- **Manual validation**: Load save state, perform action, compare results
- **5 test scenarios**: Collision, init, items, enemy, transitions
- **DOSBox-X checkpoints**: Save states at key test points
- **Deterministic setup**: Fixed cycles, hard-coded RNG seed
- **Documentation**: Validation log records all test results

### Assembly Shell Strategy
- **Keep assembly structure intact**: wrappers call C functions
- **Document original logic**: comment explaining assembly behavior, registers, algorithm
- **Easy rollback**: toggle between assembly and C implementations
- **Indefinite retention**: assembly remains as reference

### Error Handling
- **Return codes**: 0=success, non-zero=error
- **Match assembly**: use AX register convention
- **Functional validation**: Compare results with assembly version
- **No exceptions**: pure C, no C++ features

---

## Success Criteria

- **Functional equivalence**: C functions behave identically to assembly
- **Test coverage**: All 5 test scenarios pass manual validation
- **Build reproducibility**: clean builds produce consistent executables
- **Code maintainability**: C code is readable and documented
- **Assembly preservation**: original logic preserved in wrapper comments
- **Rollback safety**: can revert to assembly for any function if needed

---

## Timeline Estimate

- **Step 1-5**: Setup and infrastructure (1-2 weeks)
- **Step 6**: Tier 1 refactoring (2-3 weeks)
- **Step 7**: Validation workflow (ongoing)
- **Step 8**: Tier 2-3 refactoring (4-6 weeks)
- **Total**: 2-3 months for full migration

---

## Tools Required

- **DOSBox-X**: `brew install dosbox-x`
- **Open Watcom**: `brew install open-watcom-v2`
- **ffmpeg**: For frame extraction
- **Python 3**: For validation scripts (NumPy, OpenCV, Pillow)
- **Git**: Version control
- **Make**: Build automation

---

## References

- Original assembly: `reference/disassembly/R5sw1991.asm` (7,842 lines)
- Original executable: `reference/orig/R5sw1991/COMIC.EXE`
- Derived assets: `reference/deriv/R5sw1991/` (600+ files)
- Build system: `reference/disassembly/Makefile`
