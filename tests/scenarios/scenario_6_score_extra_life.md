# Test Scenario 6: Score-Based Extra Life (50,000-Point Regression)

## Purpose
Verify that the C implementation matches original assembly behavior for score-based extra lives:
- Increment an internal counter when scoring carries into the ten-thousands digit.
- Award exactly one extra life every 5 such carries (every 50,000 points).

This scenario guards against regressions where score increases do not grant lives at 50,000-point boundaries.

## Test Functions
- award_points (score carry handling)
- award_extra_life (life award side effects)

## Prerequisite State
Use a deterministic save state where score and lives are known and gameplay is stable (no immediate enemy/player damage).

Recommended start values:
- score = 000000
- comic_num_lives = 3 (or any known value below max)
- comic_hp < MAX_HP (to avoid max-life points-only path if lives become full during long runs)

## Test Cases

### Test 6.1: No Early Life Before 50,000
Expected behavior: scoring below 50,000 does not award an extra life.

Steps:
1. Start from known state.
2. Add points until score reaches 49,900 (or any value < 50,000).
3. Record score and comic_num_lives.

Validation:
- comic_num_lives is unchanged from baseline.

### Test 6.2: Life Award at 50,000
Expected behavior: crossing to 50,000 awards exactly one life.

Steps:
1. Continue from Test 6.1 state.
2. Add the next 100 points to reach 50,000.
3. Record score and comic_num_lives.

Validation:
- comic_num_lives increased by exactly 1.
- No double-award on the same threshold.

### Test 6.3: No Early Second Life Before 100,000
Expected behavior: no additional life is awarded between 50,100 and 99,900.

Steps:
1. Continue from Test 6.2 state.
2. Add points up to any value < 100,000.
3. Record comic_num_lives.

Validation:
- comic_num_lives unchanged from post-50,000 value.

### Test 6.4: Second Life at 100,000
Expected behavior: crossing to 100,000 awards one additional life.

Steps:
1. Continue from Test 6.3 state.
2. Add points to reach 100,000.
3. Record score and comic_num_lives.

Validation:
- comic_num_lives increased by exactly 1 relative to post-50,000 value.

## Validation Checklist

- [ ] Test 6.1: No life before 50,000 - PASS/FAIL
- [ ] Test 6.2: One life at 50,000 - PASS/FAIL
- [ ] Test 6.3: No extra life before 100,000 - PASS/FAIL
- [ ] Test 6.4: One additional life at 100,000 - PASS/FAIL

## Recording Results
Document in tests/validation_log.txt:

=== Scenario 6: Score-Based Extra Life (50,000 Regression) ===
Date: YYYY-MM-DD
Tester: [Name]

Test 6.1 - No Early Life Before 50,000
  Start Lives: [value]
  End Score: [value]
  End Lives: [value]
  Status: PASS/FAIL

Test 6.2 - Life Award at 50,000
  Start Lives: [value]
  End Score: [value]
  End Lives: [value]
  Expected Delta: +1
  Actual Delta: [value]
  Status: PASS/FAIL

Test 6.3 - No Early Second Life Before 100,000
  Start Lives: [value]
  End Score: [value]
  End Lives: [value]
  Status: PASS/FAIL

Test 6.4 - Second Life at 100,000
  Start Lives: [value]
  End Score: [value]
  End Lives: [value]
  Expected Delta: +1
  Actual Delta: [value]
  Status: PASS/FAIL

Overall: X/4 PASS

## Notes
- If comic_num_lives reaches maximum during this scenario, extra-life behavior changes to the points-only path by design. Prefer a start state that keeps lives below max through 100,000.
- Keep point increments deterministic and consistent between assembly and C runs.