# Captain Comic .SHP File Format

## Overview
.SHP files contain enemy sprite animation frames in a raw format. Each frame is 16×16 pixels and is stored in EGA format with a separate alpha mask. The files are simple concatenations of frame data with no header.

## File Structure

### File Layout
The .SHP file is a simple concatenation of raw frame data with NO file header. Data is read directly into memory.

**Total file size calculation:**
```
total_bytes = num_frames * 160  (where num_frames is determined by horizontal and animation flags)
```

### Frame Data Format

Each frame is **160 bytes**:
- **128 bytes**: Graphics data (16×16 pixels, 4 EGA planes)
- **32 bytes**: Mask (16×16 pixels, 1 plane)

#### Graphics Data (128 bytes)
The 16×16 pixel sprite is stored in EGA planar format (4 planes for colors):

**Layout in memory:**
```
Plane 0 (Red):    32 bytes   (4 rows × 2 words per row, 16 pixels = 2 bytes per row)
Plane 1 (Green):  32 bytes   (4 rows × 2 words per row)
Plane 2 (Blue):   32 bytes   (4 rows × 2 words per row)
Plane 3 (Intense):32 bytes   (4 rows × 2 words per row)
────────────────────────────
Total:           128 bytes
```

**Plane organization:**
- 16 pixels per row ÷ 8 pixels per byte = 2 bytes per row
- 16 rows of pixels
- 4 planes total

**Byte offset breakdown for a single frame at offset F:**
```
F + 0:    Plane 0, rows 0-15
F + 32:   Plane 1, rows 0-15
F + 64:   Plane 2, rows 0-15
F + 96:   Plane 3, rows 0-15
F + 128:  Mask, rows 0-15
```

#### Mask (32 bytes)
Alpha mask for transparency, using the same layout as plane data:
```
Byte offset 128-159 within the frame:  32 bytes (16 rows × 2 bytes per row)
```

Mask operation in rendering:
```c
video_memory &= mask_data;  // Clear pixels where mask is 0
video_memory |= graphics_data;  // Set pixels where graphics data is 1
```

## Frame Count and Orientation

### File Contents Based on `horizontal` and `animation` Flags

The `shp_t` structure in the file contains metadata:

```c
typedef struct {
    uint8_t num_distinct_frames;    // Frames actually stored in file (1-N)
    uint8_t horizontal;              // ENEMY_HORIZONTAL_DUPLICATED (1) or ENEMY_HORIZONTAL_SEPARATE (2)
    uint8_t animation;               // ENEMY_ANIMATION_LOOP (0) or ENEMY_ANIMATION_ALTERNATE (1)
    char filename[14];               // .SHP filename (null-padded)
} shp_t;
```

### How File Contents are Determined

#### If `horizontal == ENEMY_HORIZONTAL_DUPLICATED` (value 1):
- File contains: `num_distinct_frames` frames
- Each frame is for LEFT-FACING orientation
- RIGHT-FACING frames are created by DUPLICATING pointers to the left-facing frames
- Total frames read: `num_distinct_frames * 160 bytes`

**Animation frame cycle calculation:**
```
total_animation_frames = num_distinct_frames + animation
  If animation == ENEMY_ANIMATION_ALTERNATE: +1 (adds reversed frame)
  If animation == ENEMY_ANIMATION_LOOP: +0 (no extra frame)

Memory needed: num_distinct_frames * 160 bytes (regardless of animation value)
```

#### If `horizontal == ENEMY_HORIZONTAL_SEPARATE` (value 2):
- File contains: `num_distinct_frames * 2` frames
- First `num_distinct_frames` frames are for LEFT-FACING
- Next `num_distinct_frames` frames are for RIGHT-FACING
- Total frames read: `num_distinct_frames * 2 * 160 bytes`

**Animation frame cycle calculation:**
```
total_animation_frames = num_distinct_frames + animation
  Same as above, but applied to each orientation separately
```

## Animation Frame Organization in Memory

After loading, frames are organized in a 10-entry animation cycle array (two orientations × 5 frames max):

```
Index  0-4:   LEFT-facing animation frames
Index  5-9:   RIGHT-facing animation frames
Index  4:     UNUSED (separator between orientations)
```

### Animation Cycle Generation

The loader creates an animation cycle from the file frames using these rules:

#### For ENEMY_ANIMATION_LOOP (`animation == 0`):
```
animation_cycle = [frame0, frame1, frame2, ..., frameN]
                = [0, 1, 2, ..., num_distinct_frames-1]
Plays as: 0 → 1 → 2 → 3 → 0 → 1 → 2 → 3 → ...
```

#### For ENEMY_ANIMATION_ALTERNATE (`animation == 1`):
```
animation_cycle = [frame0, frame1, ..., frameN, frameN-2]
                = [0, 1, 2, ..., num_distinct_frames-1, num_distinct_frames-3]
Plays as: 0 → 1 → 2 → 1 → 0 → 1 → 2 → 1 → ...
```

The duplicate frame at position `num_distinct_frames + 1` points to frame `num_distinct_frames - 2`, creating the "bounce back" effect.

## Loading Procedure (from Assembly Code Analysis)

### Load Flow

1. **Calculate bytes to read:**
   ```
   if (horizontal == ENEMY_HORIZONTAL_DUPLICATED)
       num_frames = num_distinct_frames
   else
       num_frames = num_distinct_frames * 2
   
   total_bytes = num_frames * 160
   ```

2. **Open file using DOS interrupt:**
   ```
   DOS int 0x21, ah=0x3d: open existing file
   Input: filename from shp_t.filename
   Output: file handle in AX
   ```

3. **Read all frame data:**
   ```
   DOS int 0x21, ah=0x3f: read from file
   Input: file handle (BX), destination buffer (DS:DX), byte count (CX)
   ```

4. **Build animation cycle:**
   For each orientation (left-facing, then right-facing):
   - Store pointers to each frame in animation cycle array
   - If ENEMY_ANIMATION_ALTERNATE: add pointer to `num_distinct_frames - 2` frame
   - If ENEMY_HORIZONTAL_DUPLICATED: reuse left-facing pointers for right-facing

## C Implementation Notes

### Frame Access Pattern

Given a loaded frame buffer and animation cycle array:

```c
// Access animation frame index N for an enemy
uint8_t *animation_frames = enemy_animation_frames[enemy_index];  // array of 10 pointers
uint16_t frame_pointer = animation_frames[current_animation_index];
uint8_t *frame_data = (uint8_t *)frame_pointer;

// Extract graphics (128 bytes) and mask (32 bytes)
uint8_t *graphics = frame_data + 0;     // 128 bytes
uint8_t *mask = frame_data + 128;       // 32 bytes

// Access specific plane
uint8_t *plane0 = graphics + 0;         // Red (32 bytes)
uint8_t *plane1 = graphics + 32;        // Green (32 bytes)
uint8_t *plane2 = graphics + 64;        // Blue (32 bytes)
uint8_t *plane3 = graphics + 96;        // Intense (32 bytes)

// Access pixel row within plane (2 bytes per row, 8 pixels per byte)
uint16_t row_0_plane0 = plane0[0:2];    // Bytes 0-1 = row 0
uint16_t row_1_plane0 = plane0[2:2];    // Bytes 2-3 = row 1
uint16_t row_N_plane0 = plane0[N*2:2];  // Bytes N*2 to N*2+1
```

### Rendering with Mask

When rendering a frame to video memory:

```c
// For each plane (0-3)
{
    uint16_t plane_mask = (1 << plane_index);  // 1, 2, 4, or 8
    
    for (int row = 0; row < 16; row++) {
        uint16_t mask_data = *(uint16_t *)(mask + row * 2);
        uint16_t plane_data = *(uint16_t *)(plane_data_ptr + row * 2);
        
        // Render operation (in 16-bit words):
        video_memory &= ~(mask_data & plane_mask);  // Clear masked area
        video_memory |= (plane_data & plane_mask);  // Set graphics
    }
}
```

## Example Files

### Fire Ball (BALL.SHP)
- Likely: `num_distinct_frames = 4`, `horizontal = ENEMY_HORIZONTAL_DUPLICATED`
- File size: 4 × 160 = 640 bytes
- Animation: left-facing 4-frame loop + right-facing (duplicated)

### Bug-eyes (BUG.SHP)
- Likely: `num_distinct_frames = 3`, `horizontal = ENEMY_HORIZONTAL_SEPARATE`
- File size: 6 × 160 = 960 bytes
- Animation: left + right, each 3 frames with alternating animation

## References

- **EGA Graphics Format**: 4-plane planar format, 320×200 mode, 16-color palette
- **Assembly Source**: `reference/disassembly/R5sw1991.asm` lines 2074-2183
- **Plane Rendering**: Lines 2741-2809 (blit_16xH_plane_with_mask)
- **Structure Definitions**: Lines 7213-7218
