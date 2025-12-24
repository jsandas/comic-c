# Creating Functional Test Scenarios

## Overview

Functional test scenarios validate that refactored C code behaves identically to the original assembly. Each scenario focuses on a specific game subsystem and includes concrete test cases with observable, measurable outcomes.

## Scenario Template Structure

Each scenario file should include:

1. **Purpose** - What functionality is being tested
2. **Test Functions** - Which C functions this scenario validates
3. **Setup** - How to prepare the test environment
4. **Test Cases** - Specific, repeatable tests with expected outcomes
5. **Validation Checklist** - Quick pass/fail tracking
6. **Recording Results** - Format for documenting outcomes
7. **Debugging Tips** - Troubleshooting guidance

## Creating a New Scenario

### Step 1: Identify the Subsystem

Choose a game subsystem to test:
- **Tier 1 (Pure Logic):** Collision, geometry, math functions
- **Tier 2 (Data/State):** Item collection, initialization, loading
- **Tier 3 (Complex):** Enemy AI, physics, game loop

### Step 2: Define Observable Behaviors

List behaviors that can be measured or observed:
- ✅ **Good:** "Player Y position = 120"
- ✅ **Good:** "Health increases from 3 to 4"
- ✅ **Good:** "Enemy bounces at X = 200"
- ❌ **Bad:** "Game feels smooth"
- ❌ **Bad:** "Graphics look right"

### Step 3: Create Save States

For each test case, you need a DOSBox-X save state:

**Creating a Save State:**

1. Launch DOSBox-X with deterministic config:
   ```bash
   dosbox-x -conf tests/dosbox_deterministic.conf
   ```

2. Mount and run original game:
   ```
   mount c reference/orig/R5sw1991
   c:
   comic.exe
   ```

3. Navigate to desired test position in game

4. Press `F5` (or `Ctrl+F5`) to save state

5. Give it a descriptive name like:
   - `level1_start` - Beginning of level 1
   - `level1_near_door` - Standing next to door
   - `level1_near_cola` - Next to cola bottle item
   - `level2_enemy_present` - With enemy visible

6. Find save state files (location varies by OS):
   - macOS: `~/Library/Preferences/DOSBox-X/save/`
   - Linux: `~/.config/dosbox-x/save/`
   - Windows: `%APPDATA%\DOSBox-X\save\`

7. Copy to project: `tests/savestates/[name].sav`

### Step 4: Write Test Cases

For each test case, document:

**Format:**
```markdown
### Test X.Y: [Short Description]
**Expected Behavior:** [What should happen]

**Steps:**
1. Load `[savestate_name].sav`
2. Perform action (press key, wait time, etc.)
3. Observe result (player position, score, health, etc.)
4. Record result: "[Specific measurement]"

**Validation:**
1. Load same save state in C version
2. Perform identical action
3. Compare: [What to compare]
4. **PASS** if [condition], **FAIL** if [condition]
```

**Example:**
```markdown
### Test 3.1: Collect Cola Bottle
**Expected Behavior:** Health increases by 1, score increases by 100

**Steps:**
1. Load `level1_near_cola.sav`
2. Press RIGHT to walk to cola bottle
3. Observe health and score
4. Record result: "Health: 3→4, Score: 0→100"

**Validation:**
1. Load same save state in C version
2. Press RIGHT to walk to cola bottle
3. Compare: Health and score changes
4. **PASS** if both match, **FAIL** if different
```

### Step 5: Create Validation Checklist

Add a simple checklist for quick tracking:

```markdown
## Validation Checklist

- [ ] Test 1.1: [Description] - PASS/FAIL
- [ ] Test 1.2: [Description] - PASS/FAIL
- [ ] Test 1.3: [Description] - PASS/FAIL
```

### Step 6: Document Results Format

Show how to record results in `validation_log.txt`:

```markdown
## Recording Results

Document results in `tests/validation_log.txt`:

\```
=== Scenario X: [Name] ===
Date: YYYY-MM-DD
Tester: [Your Name]

Test X.1 - [Description]
  Assembly: [Measured result]
  C Version: [Measured result]
  Status: PASS/FAIL
  Notes: [Any observations]

Overall: X/Y PASS
\```
```

## Tips for Good Test Cases

### Make Tests Deterministic

✅ **Good:**
- Use fixed starting positions (save states)
- Use timer-based inputs ("hold key for 1 second")
- Measure specific values (positions, scores)

❌ **Bad:**
- Rely on timing ("press jump at the right moment")
- Use subjective measures ("enemy seems faster")
- Depend on randomness without seed control

### Make Tests Independent

Each test should:
- Start from a known save state
- Not depend on previous test results
- Be runnable in any order

### Make Tests Specific

Test one behavior at a time:
- ✅ "Floor collision prevents falling"
- ❌ "Movement works correctly"

### Make Tests Measurable

Use concrete, observable values:
- ✅ "Player X changes from 50 to 75"
- ✅ "Score increases by exactly 100"
- ❌ "Player moves smoothly"

## Observing Game State

### Visual Observation
- Player position (count tiles)
- Health/score display
- Item presence/absence
- Enemy positions

### DOSBox Debugger
Press `Alt+Pause` to enter debugger, then:

```
# View memory at address
d [segment]:[offset]

# View player position (example addresses)
d 0x1234:0x5678

# Continue execution
q
```

### Adding Debug Output

If you have access to C code:

```c
// Temporary debug output (remove before commit)
fprintf(stderr, "DEBUG: player_x=%d, player_y=%d\n", player_x, player_y);
```

## Example Scenarios

See existing scenarios for examples:
- [scenario_1_collision.md](scenario_1_collision.md) - Collision detection
- More to be created...

## Testing Workflow

1. **Create scenario file** following template
2. **Create save states** needed for tests
3. **Run tests on assembly** version (original)
4. **Document results** with specific measurements
5. **Convert function to C** (when ready)
6. **Run tests on C version** using same save states
7. **Compare results** - PASS if identical, FAIL if different
8. **Debug if needed** and iterate
9. **Update validation log** with final results
10. **Commit when passing** with reference to scenario

## Scenario Priorities

Create scenarios in this order to match refactoring tiers:

1. **Scenario 1: Collision** (Tier 1 - Pure logic)
2. **Scenario 2: Initialization** (Tier 2 - State setup)
3. **Scenario 3: Items** (Tier 2 - State mutation)
4. **Scenario 4: Enemy AI** (Tier 3 - Complex behavior)
5. **Scenario 5: Transitions** (Tier 3 - State changes)

## Questions?

If unsure about scenario design:
1. Start simple - test one function at a time
2. Make it concrete - use specific numbers
3. Make it repeatable - use save states
4. Focus on behavior - not implementation details
