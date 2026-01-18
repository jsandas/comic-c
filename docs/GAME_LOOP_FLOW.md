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

## Nonplayer Actor Handling

### handle_enemies (from R5sw1991.asm:4653)

Processes all spawned enemies, handles their behavior, collision detection with Comic, and animation.

```
handle_enemies:
- Initialize enemy_spawned_this_tick = 0
- For each enemy slot (MAX_NUM_ENEMIES total):
  
  .despawned (state = ENEMY_STATE_DESPAWNED):
  - Decrement enemy spawn_timer_and_animation
  - If timer expired:
    - Call maybe_spawn_enemy (attempts to spawn this enemy)
  
  .spawned (state = ENEMY_STATE_SPAWNED):
  - Increment spawn_timer_and_animation (animation counter)
  - If animation counter >= num_animation_frames:
    - Wrap animation counter back to 0
  - Branch on enemy.behavior (1-5):
    - behavior 1: Call enemy_behavior_bounce
    - behavior 2: Call enemy_behavior_leap
    - behavior 3: Call enemy_behavior_roll
    - behavior 4: Call enemy_behavior_seek
    - behavior 5: Call enemy_behavior_shy
  - All behavior subroutines jump back to .check_despawn when done
  
  .check_despawn:
  - Compare enemy's distance from Comic
  - If enemy.x is > ENEMY_DESPAWN_RADIUS away from Comic.x:
    - Despawn this enemy
  
  .check_collision_with_comic:
  - If horizontal distance (abs(enemy.x - comic.x)) >= 2:
    - Skip collision check
  - If enemy.y < comic.y or (enemy.y - comic.y) >= 4:
    - Skip collision check
  - Else (collision detected):
    - Set enemy.state = ENEMY_STATE_RED_SPARK (start red spark animation)
    - If comic_hp == 0 AND inhibit_death_by_enemy_collision == 0:
      - Set inhibit_death_by_enemy_collision = 1
      - Call comic_death_animation
      - Set inhibit_death_by_enemy_collision = 0
      - Jump to comic_dies
    - Else:
      - Decrement comic_hp
      - Play SOUND_HIT_BY_ENEMY
  
  .blit:
  - Calculate camera-relative x-coordinate: enemy.x - camera_x
  - Check playfield bounds:
    - If camera-relative x < 0: skip to .next_enemy (off left edge)
    - If camera-relative x >= PLAYFIELD_WIDTH - 2: skip to .next_enemy (off right edge)
  - Construct graphics pointer:
    - Get current animation frame index: al = enemy.spawn_timer_and_animation
    - Add facing direction offset: al += enemy.facing
    - Look up frame pointer: si = enemy.animation_frames_ptr[al]
    - Set mask pointer: bp = si + 128 (mask data follows frame data)
  - Call blit_16xH_with_mask_playfield_offscreen with:
    - ah = enemy.y (row coordinate)
    - al = camera-relative x (column coordinate)
    - si = enemy animation frame graphics pointer
    - bp = mask pointer (for transparency)
    - cx = 16 (enemy sprite height in pixels)
  
  .dying (state >= ENEMY_STATE_WHITE_SPARK):
  - Enemy is in a death/spark animation sequence:
    - States 2-6: white spark animation is playing
    - State 7: white spark animation finished
    - States 8-12: red spark animation is playing
    - State 13: red spark animation finished
  - Normalize animation counter: bl = enemy.state
  - Check if white spark animation is complete:
    - If bl == 7: jump to .despawn
    - If bl < 7: still in white spark, jump to .advance_spark_animation
  - Check if red spark animation is complete:
    - Subtract offset: bl -= (ENEMY_STATE_RED_SPARK - ENEMY_STATE_WHITE_SPARK)
    - If bl != 7: still in red spark, jump to .advance_spark_animation
    - If bl == 7: jump to .despawn
  
  .advance_spark_animation:
  - Calculate camera-relative x-coordinate: enemy.x - camera_x
  - Despawn dying enemies that leave the playfield:
    - If camera-relative x < 0: jump to .despawn
    - If camera-relative x >= PLAYFIELD_WIDTH - 2: jump to .despawn
  - Check animation frame count: if bl <= 4 (first 3 frames):
    - Blit the enemy sprite under the spark animation (using same procedure as .blit above)
  
  .blit_spark:
  - Construct spark animation graphics pointer:
    - Get animation state: al = enemy.state
    - Subtract 2 to 0-index into ANIMATION_TABLE_SPARKS: al -= 2
    - Look up spark frame pointer: si = ANIMATION_TABLE_SPARKS[al]
    - Set mask pointer: bp = si + 128
  - Call blit_16xH_with_mask_playfield_offscreen with spark animation frame (same parameters as regular blit)
  - Advance spark animation counter: enemy.state++
  - Jump to .enemy_loop_next
  
  .despawn:
  - Set enemy.state = ENEMY_STATE_DESPAWNED
  - Initialize respawn timer: enemy.spawn_timer_and_animation = enemy_respawn_counter_cycle
  - Advance the global respawn counter cycle:
    - enemy_respawn_counter_cycle += 20
    - If >= 120: wrap around to 20 (cycles respawn timing)
  - Jump to .enemy_loop_next
  
  .enemy_loop_next:
  - Restore loop counter: pop cx
  - Advance to next enemy slot: si += enemy_size
  - Decrement enemy counter: cx--
  - If cx == 0: jump to return (all enemies processed)
  - Else: jump back to .enemy_loop
```

### handle_fireballs (from R5sw1991.asm:5653)

Manages fireball movement, animation, playfield bounds checking, and collision detection with enemies.

```
handle_fireballs:
- If comic_firepower == 0:
  - Return (no fireballs to process)

.movement_loop: [For each fireball slot, up to comic_firepower slots]
- Check if fireball slot is active:
  - If fireball.y == FIREBALL_DEAD AND fireball.x == FIREBALL_DEAD:
    - Skip to next fireball
  
  .moving_left or .moving_right:
  - If fireball.vel < 0:
    - Negate fireball.vel and move fireball left: fireball.x += fireball.vel (negative)
  - Else (fireball.vel > 0):
    - Move fireball right: fireball.x += fireball.vel
  
  .check_playfield_bounds:
  - Calculate camera-relative x-coordinate
  - If fireball is off the left edge (camera-relative x < 0):
    - Deactivate fireball (set to FIREBALL_DEAD)
  - If fireball is off the right edge (camera-relative x >= PLAYFIELD_WIDTH - 2):
    - Deactivate fireball
  
  .handle_corkscrew:
  - If comic_has_corkscrew == 1:
    - Decrement fireball.corkscrew_phase
    - If phase == 0:
      - Move fireball down: fireball.y += 1
      - Set phase = 2
    - Else:
      - Move fireball up: fireball.y -= 1
  
  .animate:
  - Store updated fireball.y and fireball.x
  - Increment fireball.animation (animation frame counter)
  - If animation >= num_animation_frames:
    - Wrap animation back to 0
  - Blit fireball at its current location with current animation frame

.check_collision: [Nested loop checking all fireballs vs all enemies]
- For each fireball slot:
  - If fireball is inactive (FIREBALL_DEAD):
    - Skip to next fireball
  - For each enemy slot:
    - If enemy.state != ENEMY_STATE_SPAWNED:
      - Skip to next enemy
    - Check vertical collision: (fireball.y - enemy.y) must be 0 or 1
    - Check horizontal collision: abs(fireball.x - enemy.x) must be <= 1
    - If collision found:
      - Set enemy.state = ENEMY_STATE_WHITE_SPARK (white spark animation)
      - Deactivate fireball (set to FIREBALL_DEAD)
      - Award 300 points (3 units)
      - Play SOUND_HIT_ENEMY with priority 1
      - Continue to next fireball
```

### handle_item (from R5sw1991.asm:4145)

Manages item collection or rendering in the current stage.

```
handle_item:
- Get current_stage_ptr
- If stage.item_type == ITEM_UNUSED:
  - Return (no item in this stage)

- Calculate items_collected index for current stage:
  - index = (current_stage_number << 4) + current_level_number
  - If items_collected[index] == 1:
    - Return (item already collected in this stage)

- Get item coordinates from stage data:
  - item_y = stage.item_y
  - item_x = stage.item_x
  
- Calculate camera-relative x-coordinate:
  - camera_relative_x = item_x - camera_x
  - If item is off the left edge (camera_relative_x < 0):
    - Return (skip rendering)
  - If item is off the right edge (camera_relative_x >= 22):
    - Return (skip rendering)

- Call collect_item_or_blit_offscreen(stage.item_type):
  - Checks if Comic collides with item
  - If collision detected:
    - Mark item as collected in items_collected bitmap
    - Update Comic's power-ups based on item type:
      - Shield: Increase comic_hp, comic_num_lives
      - Boots: Increase comic_jump_power
      - Fireball: Increase comic_firepower
      - Corkscrew: Set comic_has_corkscrew = 1
      - Door Key: Set comic_has_door_key = 1
      - Lantern: Set comic_has_lantern = 1
      - Teleport Wand: Set comic_has_teleport_wand = 1
      - Treasure: Increment comic_num_treasures
      - Extra Life: Award extra life (or refill HP if at max lives)
    - Award 2000 points for collecting item
  - Else (no collision):
    - Blit item to offscreen buffer at its current location with animation
```

### swap_video_buffers (from R5sw1991.asm:4120)

Swaps the offscreen video buffer to become the visible buffer and vice versa. Updates offscreen_video_buffer_ptr to point to the new offscreen buffer.

```
swap_video_buffers:
- Get current offscreen_video_buffer_ptr (0x0000 or 0x2000)
- Call switch_video_buffer:
  - Switches which buffer is displayed on screen
- Toggle offscreen_video_buffer_ptr to the other value:
  - If was 0x0000, set to 0x2000
  - If was 0x2000, set to 0x0000
- Return
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
