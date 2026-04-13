# Assembly vs C Game Loop Alignment Review (2026-04-12)

Scope:
- Assembly reference: reference/disassembly/R5sw1991.asm
- C implementation: src/game_main.c, src/physics.c, src/actors.c, src/doors.c
- Coverage: main loop branches and all direct loop descendants (teleport, fall/jump path, doors, enemies, fireballs, items, swap)

## Overall Assessment

- Core structure is substantially aligned.
- Several intentional modernization changes exist (CPU yielding, edge-triggered input, cheat hooks).
- Multiple behavioral divergences are still present, including at least two that are likely visible in gameplay (item animation cadence and teleport tick sequencing).

## Branch-by-Branch Alignment Matrix

Legend: Aligned, Partial, Diverged, Intentional

1. Tick wait loop and recharge logic
- Assembly: game_loop busy-wait and recharge in wait path (R5sw1991.asm:3867-3893)
- C: while(game_tick_flag != 1) with recharge + dos_idle (src/game_main.c:3750-3761)
- Status: Partial (intentional CPU behavior change)

2. Tick consume, win countdown, run cycle, default animation, pending HP
- Assembly: R5sw1991.asm:3895-3930
- C: src/game_main.c:3764-3832
- Status: Aligned

3. Teleport active gate
- Assembly: teleport path jumps directly to non-player handlers, skipping pause/fire (R5sw1991.asm:3932-3936)
- C: handle_teleport runs, then goto handle_nonplayer_actors skips pause/fire (src/game_main.c:3835-3853)
- Status: Aligned (fixed 2026-04-12)

4. Falling/jumping gate and return target
- Assembly: when in air, jump to handle_fall_or_jump, then return to pause section (skip door/teleport/left/right/floor checks) (R5sw1991.asm:3938-3945 and R5sw1991.asm:4421,4441)
- C: always calls handle_fall_or_jump in non-teleport path, then may still run door/teleport checks if landing occurred this tick (src/game_main.c:3841-3857 and src/physics.c:241)
- Status: Diverged

5. Jump input start condition
- Assembly: level-triggered key check (R5sw1991.asm:3943-3956)
- C: edge-triggered with previous state (src/game_main.c:3783-3791)
- Status: Intentional

6. Door activation branch
- Assembly: exact y, x delta in [0..2], has key (R5sw1991.asm:3961-3980)
- C: same checks in check_door_activation (src/doors.c:299-357)
- Status: Aligned

7. Teleport input branch
- Assembly: key press + wand => begin_teleport then immediate recheck of teleport branch in same tick (R5sw1991.asm:3982-3991)
- C: edge flag + wand => begin_teleport then handle_teleport immediately, then goto handle_nonplayer_actors (src/game_main.c:3849-3860)
- Status: Aligned (fixed 2026-04-12)

8. Left/right and floor walk-off logic
- Assembly: R5sw1991.asm:3993-4045
- C: src/game_main.c:3857-3910
- Status: Aligned (with landing-gate caveat from branch 4)

9. Pause and fire/meter branch
- Assembly: pause busy-wait; fire/meter asymmetry and counter cycling (R5sw1991.asm:4047-4067)
- C: pause uses dos_idle while waiting; meter logic matches asymmetry (src/game_main.c:3912-3950)
- Status: Partial (pause CPU behavior intentionally modernized)

10. Render + non-player + swap ordering
- Assembly: blit map/comic, then enemies/fireballs/item, then swap (R5sw1991.asm:4069-4075)
- C: same order plus extra UI rendering calls (src/game_main.c:3953-3971)
- Status: Partial (extra UI work in C)

11. Enemy loop structure
- Assembly: state dispatch order spawned/despawned/dying in specific flow (R5sw1991.asm:4653-4849)
- C: despawned, then dying, then spawned (src/actors.c:876-1031)
- Status: Partial

12. Fireball loop + collision pass
- Assembly: movement/render pass over comic_firepower slots, then separate collision pass over MAX_NUM_FIREBALLS (R5sw1991.asm:5653-5832)
- C: single per-slot pass over comic_firepower only, collision before render per slot (src/actors.c:381-513)
- Status: Partial

13. Item update/render path
- Assembly: item animation counter advances in collect_item_or_blit_offscreen (R5sw1991.asm:6453-6510)
- C: item animation counter advances after blit in handle_item only (src/actors.c)
- Status: Aligned (fixed 2026-04-12)

## Confirmed Differences and Omissions

## High Priority

1. Teleport tick sequencing ~~differs from assembly~~ **FIXED 2026-04-12**
- Fix applied:
  - begin_teleport() now immediately calls handle_teleport() in same tick: src/game_main.c
  - Both teleport-start and ongoing teleport ticks use goto handle_nonplayer_actors to skip pause/fire.
- Was:
  - Assembly begins teleport and immediately routes through teleport branch in same tick: R5sw1991.asm:3989-3991 and R5sw1991.asm:3932-3936
  - C deferred handle_teleport to next loop and still ran pause/fire during teleport ticks.

2. In-air return path mismatch after handle_fall_or_jump
- Evidence:
  - Assembly returns from physics directly to pause section (skips open/teleport/left/right/floor): R5sw1991.asm:4421 and R5sw1991.asm:4441
  - C may still evaluate door/teleport on the same tick after landing: src/game_main.c:3844-3853
- Impact:
  - Landing frame can process interactions earlier than assembly.
- Recommendation:
  - Add a strict post-physics gate to skip open/teleport/left/right/floor path when physics was entered from an in-air state that tick.

3. Item animation counter advances in two places in C ~~**FIXED 2026-04-12**~~
- Fix applied:
  - Removed early toggle from handle_item (was firing before collision check and on off-screen frames).
  - Removed unconditional advance from swap_video_buffers.
  - Counter now advances once, after the blit, in handle_item's render path only: src/actors.c
  - Sequence now matches assembly: even frame → counter 0→1, odd frame → counter 1→2→0.
- Was:
  - Assembly counter advance in item rendering path: R5sw1991.asm:6507-6510
  - C toggled in item handling and incremented in swap: src/actors.c:562 and src/game_main.c:2568-2570

4. Enemy death spark underlay missing
- Evidence:
  - Assembly blits enemy under spark for early spark frames: R5sw1991.asm:4824-4846
  - C dying branch renders spark only: src/actors.c:894-956
- Impact:
  - Visible death animation difference.
- Recommendation:
  - Add enemy-under-spark render for first spark frames to match assembly.

## Medium Priority

5. Enemy respawn timer semantics differ (possible off-by-one)
- Evidence:
  - Assembly decrements timer and spawns on underflow: R5sw1991.asm:4667-4671
  - C decrements to zero and spawns immediately at zero: src/actors.c:880-887
- Impact:
  - Spawn timing may be one tick earlier in C.
- Recommendation:
  - Decide fidelity target; if matching assembly, replicate underflow semantics.

6. Fireball processing model differs
- Evidence:
  - Assembly collision pass is separate and scans MAX_NUM_FIREBALLS: R5sw1991.asm:5758-5832
  - C merges movement+collision and only iterates comic_firepower slots: src/actors.c:391 and src/actors.c:445
- Impact:
  - Potential frame-order differences in render/collision and edge behavior when slot count changes.
- Recommendation:
  - Low to medium risk. Align only if deterministic replay or frame-perfect parity is required.

7. Enemy state dispatch ordering differs
- Evidence:
  - Assembly dispatch flow: R5sw1991.asm:4661-4664
  - C handles dying before spawned: src/actors.c:894 then src/actors.c:957
- Impact:
  - Usually equivalent, but subtle frame-order changes can happen around transitions.
- Recommendation:
  - Consider reordering for fidelity once higher-priority issues are closed.

## Intentional/Justified Differences (Keep Unless Doing Frame-Perfect Mode)

1. Edge-triggered jump and teleport input
- C uses previous key state and teleport edge flag (src/game_main.c:3783-3791 and src/game_main.c:3658)
- Assembly is level-triggered (R5sw1991.asm:3943-3956 and R5sw1991.asm:3982-3991)
- Keep for modern feel, unless strict authenticity mode is desired.

2. CPU yielding in wait loops
- C calls dos_idle in tick wait and pause wait (src/game_main.c:3760 and src/game_main.c:3918)
- Assembly busy-waits.
- Keep for system friendliness.

3. Debug/testing cheat hooks
- C includes cheat handling in loop (src/game_main.c:3774 and src/game_main.c:3586)
- Not present in assembly loop.
- Keep for development builds; gate out for authenticity build if needed.

## Recommended Execution Plan

Phase 1: Fix high-impact fidelity gaps
1. Teleport same-tick routing:
- After begin_teleport in loop, run teleport branch logic immediately (or structured equivalent) and skip pause/fire for that frame.

2. In-air return-path fidelity:
- Ensure handle_fall_or_jump path cannot fall through to door/teleport/movement checks on same tick.

3. Item animation cadence:
- Remove one of the two item_animation_counter updates and validate frame cadence vs assembly capture.

4. Enemy death underlay:
- Render enemy beneath spark in early spark frames.

Phase 2: Timing and frame-order parity
1. Enemy respawn timer underflow semantics.
2. Enemy state dispatch order alignment.
3. Fireball two-pass flow alignment (if parity tests show mismatch).

Phase 3: Optional authenticity modes
1. Keep current modern defaults.
2. Add strict-authenticity toggle to switch:
- edge-triggered vs level-triggered input,
- busy-wait vs yielding behavior,
- cheat hooks enabled/disabled.

## Should These Be Changed?

Guidance by objective:
- If target is functional modern remake with good feel:
  - Keep intentional input/CPU improvements.
  - Fix only clearly visible divergences: item cadence, teleport sequencing, enemy death underlay.
- If target is deterministic assembly parity:
  - Implement all high and medium changes above and validate with save-state frame comparisons.

## Suggested Validation Scenarios

1. Teleport edge timing test:
- Press teleport on first eligible frame and capture whether animation starts same tick.

2. Landing-frame interaction test:
- Land while holding open/teleport and verify whether action triggers on landing frame.

3. Item blink cadence test:
- Record item frame alternation period before/after fix.

4. Enemy death visual test:
- Confirm enemy sprite is visible under spark for early death frames.

5. Enemy respawn timing test:
- Count ticks from despawn to respawn start against assembly reference.

6. Fireball collision order test:
- Validate whether fireball appears on collision frame and compare with assembly captures.
