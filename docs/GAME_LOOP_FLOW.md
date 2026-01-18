# Game Loop and Jumping Flow in Captain Comic

## Main Game Loop (`game_loop` from R5sw1991.asm:3861)

The game loop continuously cycles, waiting for game ticks and processing player input and game logic on each tick.

### Initial Loop (waiting for tick)
```
game_loop:
- Clear BIOS keyboard buffer (head = tail)
- BUSY WAIT: if cs:game_tick_flag != 1, stay in this loop
  - While waiting (if NOT falling/jumping AND NOT pressing jump):
    - Recharge jump counter: comic_jump_counter = comic_jump_power
    - Loop back to wait for tick
```

### Tick Processing (game_loop.tick)
```
.tick:
- Set cs:game_tick_flag = 0
- Check win condition:
  - If win_counter = 0, skip to .not_won_yet
  - If win_counter > 0:
    - Decrement win_counter
    - If win_counter == 1, jump to game_end_sequence (game won!)
```

### Animation and HP Processing (game_loop.not_won_yet)
```
.not_won_yet:
- Advance comic_run_cycle: ch = (comic_run_cycle + 1) % 4
  - If ch == COMIC_RUNNING_3 + 1 (4), reset to COMIC_RUNNING_1 (1)
- Set comic_animation = COMIC_STANDING (may be overridden later)
- If comic_hp_pending_increase > 0:
  - Decrement comic_hp_pending_increase
  - Call increment_comic_hp
```

### Teleport Check (game_loop.check_teleport)
```
.check_teleport:
- If comic_is_teleporting = 1:
  - Call handle_teleport
  - Jump to .handle_nonplayer_actors (skip rest of input processing)
```

### Falling/Jumping Check (game_loop.check_falling_or_jumping)
```
.check_falling_or_jumping:
- si = comic_y (holds both comic_y and comic_x in 16-bit register)
- If comic_is_falling_or_jumping = 1:
  - Jump to handle_fall_or_jump
- Else continue to check_jump_input
```

### Jump Input Check (game_loop.check_jump_input)
```
.check_jump_input:
- If cs:key_state_jump != 1:
  - Jump to .recharge_jump_counter
- If comic_jump_counter <= 1:
  - Jump to .check_open_input (jump counter exhausted, can't jump)
- Else (jump button pressed AND counter > 1):
  - Set comic_is_falling_or_jumping = 1
  - Jump to handle_fall_or_jump

.recharge_jump_counter:
- comic_jump_counter = comic_jump_power
```

### Door Opening (game_loop.check_open_input)
```
.check_open_input:
- If cs:key_state_open != 1:
  - Jump to .check_teleport_input
- Loop through all 3 doors in current stage:
  - Check if:
    - Comic's y-coordinate matches door's y-coordinate exactly
    - Comic's x-coordinate is within 3 units of door's x-coordinate
    - Comic has the Door Key
  - If all match: call activate_door (loads new level/stage, returns to game_loop)
  - Else: check next door
```

### Teleport Input (game_loop.check_teleport_input)
```
.check_teleport_input:
- If cs:key_state_teleport != 1:
  - Jump to .check_left_input
- If comic_has_teleport_wand != 1:
  - Jump to .check_left_input
- Else:
  - Call begin_teleport
  - Jump to .check_teleport
```

### Left/Right Movement (game_loop.check_left_input / check_right_input)
```
.check_left_input:
- Set comic_x_momentum = 0
- If cs:key_state_left = 1:
  - Set comic_x_momentum = -5
  - Call face_or_move_left (modifies si)

.check_right_input:
- If cs:key_state_right = 1:
  - Set comic_x_momentum = +5
  - Call face_or_move_right (modifies si)
```

### Floor Detection (game_loop.check_for_floor)
```
.check_for_floor:
- Get tile at (comic_x, comic_y + 4) [just below Comic's feet]
- If tile is solid:
  - Jump to .check_pause_input (Comic is standing)
- If Comic is on tile boundary (comic_x is even):
  - Jump to .check_pause_input (solid ground)
- Else if Comic is between tiles (comic_x is odd):
  - Check tile to the right at (comic_x+1, comic_y + 4)
  - If solid, jump to .check_pause_input
  
.no_floor: [Comic walked off an edge]
- Set comic_y_vel = 8 (initial falling speed)
- Set comic_x_momentum based on current momentum:
  - If momentum > 0: comic_x_momentum = 2 (preserve rightward momentum)
  - If momentum < 0: comic_x_momentum = -2 (preserve leftward momentum)
- Set comic_is_falling_or_jumping = 1
- Set comic_jump_counter = 1 (treated as depleted jump)
```

### Pause, Fire, and Rendering
```
.check_pause_input:
- Save comic_y to si (pause routine will trash it)
- If cs:key_state_esc = 1:
  - Call pause
  - Wait for ESC release to avoid flicker
  - Restore si from comic_y

.check_fire_input:
- If cs:key_state_fire = 1 AND fireball_meter > 0:
  - Call try_to_fire
  - Manage fireball_meter (decreases when firing, increases when not)

.blit_map_and_comic:
- Call blit_map_playfield_offscreen
- Call blit_comic_playfield_offscreen

.handle_nonplayer_actors:
- Call handle_enemies
- Call handle_fireballs
- Call handle_item
- Call swap_video_buffers

JMP game_loop [infinite loop back to top]
```

---

## Jump and Fall Handling (`handle_fall_or_jump` from R5sw1991.asm:4274)

This function is called when `comic_is_falling_or_jumping = 1`. It applies upward acceleration from jumping, downward acceleration from gravity, and checks for collisions.

### Jump Counter Management
```
handle_fall_or_jump:
- Decrement comic_jump_counter
- If comic_jump_counter == 0:
  - Jump to .jump_counter_expired
  - (Set comic_jump_counter = 1 as minimum; jump phase is over)
- Else (counter still > 0, can continue accelerating upward):
  - If cs:key_state_jump = 1:
    - comic_y_vel -= 7 (accelerate upward)
  - Else:
    - Jump to .not_accelerating_upward

.not_accelerating_upward:
- Set ceiling_stick_flag = 0 (no longer pressing against ceiling)
```

### Velocity Integration
```
.integrate_vel:
- al = comic_y_vel (raw velocity in 1/8 game units per tick)
- cl = al (save for gravity calculation later)
- al = al / 8 (convert to game units: sign-extend right shift by 3)
- bx = si (bx now holds both y and x coordinates)
- bl = bl + al (comic_y += comic_y_vel / 8)
- If bl < 0:
  - bl = 0 (clamp to top of screen)
- bl = bl + ceiling_stick_flag (push down 1 unit if against ceiling)
- ceiling_stick_flag = 0
- si = bx (return updated coordinates)
- If bl < PLAYFIELD_HEIGHT - 3:
  - Jump to .apply_gravity (continue with normal gravity application)
- Else (bl >= PLAYFIELD_HEIGHT - 3):
  - Jump to comic_dies (Comic fell too far down)
```

### Gravity Application
```
.apply_gravity:
- If current_level_number = LEVEL_NUMBER_SPACE (2):
  - cl = cl - (COMIC_GRAVITY - COMIC_GRAVITY_SPACE) [subtract 2, so total gravity is 3]
- cl = cl + COMIC_GRAVITY [add 5]
  - Result: normal gravity = 5, space gravity = 3
- If cl > TERMINAL_VELOCITY + 1 (24):
  - cl = TERMINAL_VELOCITY (23)
- comic_y_vel = cl (store updated velocity)
```

### Mid-Air Momentum and Control
```
[Execution falls through sequentially from .l3 to .check_left_input]

.check_left_input:
- cl = comic_x_momentum
- If cs:key_state_left = 1:
  - Set comic_facing = COMIC_FACING_LEFT
  - Decrement cl (decrease rightward momentum or increase leftward)
  - If cl < -5: cl = -5 (clamp to -5)

.check_right_input:
- If cs:key_state_right = 1:
  - Set comic_facing = COMIC_FACING_RIGHT
  - Increment cl (decrease leftward momentum or increase rightward)
  - If cl > 5: cl = 5 (clamp to 5)

.check_move_left:
- Store possibly-modified cl in comic_x_momentum
- If cl < 0 (moving/accelerating left):
  - Increment comic_x_momentum (drag toward 0)
  - Call move_left

.check_move_right:
- If cl > 0 (moving/accelerating right):
  - Decrement comic_x_momentum (drag toward 0)
  - Call move_right
```

### Collision Detection - Upward (Head)
```
.check_solidity_upward:
- Get tile at (comic_x, comic_y) [Comic's head]
- If tile is solid:
  - Jump to .head_in_solid_tile
- If Comic is on tile boundary (comic_x is even):
  - Jump to .check_solidity_downward
- Else (comic_x is odd):
  - Check tile to the right at (comic_x+1, comic_y)
  - If solid, jump to .head_in_solid_tile

.head_in_solid_tile:
- If comic_y_vel > 0:
  - Jump to .check_solidity_downward (moving downward, ignore ceiling)
- Else (moving upward, hit a ceiling):
  - Set ceiling_stick_flag = 1
  - Set comic_y_vel = 0 (stop upward movement)
  - Jump to .still_falling_or_jumping
```

### Collision Detection - Downward (Feet)
```
.check_solidity_downward:
- If comic_y_vel <= 0:
  - Jump to .still_falling_or_jumping (not moving downward)
- cx = si (cx now holds coordinates)
- Get tile at (comic_x, comic_y + 5) [just below Comic's feet]
- If tile is NOT solid:
  - Check if Comic is on tile boundary (comic_x is even)
  - If odd, check tile to the right at (comic_x+1, comic_y+5)
  - If that's also not solid:
    - Jump to .still_falling_or_jumping (still in air)

.hit_the_ground:
- Bounds check: if comic_y >= PLAYFIELD_HEIGHT - 5 (garbage below map):
  - Jump to .still_falling_or_jumping (treat as still falling)
- Else (legitimate ground collision):
  - Clamp comic_y to even tile boundary:
    - cl = (comic_y + 1) & 0xfe [rounds up to nearest even]
  - si = cx (return updated coordinates)
  - Set comic_is_falling_or_jumping = 0
  - Set comic_y_vel = 0
  - Jump to game_loop.check_pause_input

.still_falling_or_jumping:
- Set comic_animation = COMIC_JUMPING
- Jump to game_loop.check_pause_input (skip door/teleport/movement checks)
```

---

## End Sequence

```
game_end_sequence:
- Award points (20,000 base + 10,000 per remaining life)
- Play victory theme
- Display high scores
- Jump to terminate_program
```
