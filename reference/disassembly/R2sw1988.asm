; Commented disassembly of The Adventures of Captain Comic, Revision 2.
;   nasm -f obj -o R2sw1988.obj R2sw1988.asm
;   djlink -o R2sw1988.exe R2sw1988.obj

bits	16

; Keyboard scancodes.
; https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
SCANCODE_ALT		equ	56
SCANCODE_SPACE		equ	57
SCANCODE_CAPSLOCK	equ	58
SCANCODE_F1		equ	59
SCANCODE_F4		equ	62
SCANCODE_LEFT		equ	75
SCANCODE_RIGHT		equ	77
SCANCODE_INS		equ	82

; Relative to segment 0x0000.
BIOS_KEYBOARD_BUFFER_HEAD	equ	0x41a
BIOS_KEYBOARD_BUFFER_TAIL	equ	0x41c

; These match the order of levels under level_data.
LEVEL_NUMBER_LAKE	equ	0
LEVEL_NUMBER_FOREST	equ	1
LEVEL_NUMBER_SPACE	equ	2
LEVEL_NUMBER_BASE	equ	3
LEVEL_NUMBER_CAVE	equ	4
LEVEL_NUMBER_SHED	equ	5
LEVEL_NUMBER_CASTLE	equ	6
LEVEL_NUMBER_COMP	equ	7

; Item codes.
ITEM_CORKSCREW		equ	0
ITEM_DOOR_KEY		equ	1
ITEM_BOOTS		equ	2
ITEM_LANTERN		equ	3
ITEM_TELEPORT_WAND	equ	4
ITEM_GEMS		equ	5
ITEM_CROWN		equ	6
ITEM_GOLD		equ	7
ITEM_BLASTOLA_COLA	equ	8
ITEM_SHIELD		equ	14

; Operation codes for int3_handler.
SOUND_UNMUTE		equ	0
SOUND_PLAY		equ	1
SOUND_MUTE		equ	2
SOUND_STOP		equ	3
SOUND_QUERY		equ	4

; Codes that indicate that a .SHP file slot, stage edge, or door is unused.
SHP_UNUSED		equ	0x00
EXIT_UNUSED		equ	0xff
DOOR_UNUSED		equ	0xff
ITEM_UNUSED		equ	0xff

; Values of shp.horizontal, which controls whether the sprite has separate
; left- and right-facing graphics.
ENEMY_HORIZONTAL_DUPLICATED	equ	1	; make right-facing frames by copying the left-facing frames
ENEMY_HORIZONTAL_SEPARATE	equ	2	; right-facing frames are stored separately
; Values of shp.animation, which controls whether sprite animation loops from
; the beginning or alternates forward and backward.
ENEMY_ANIMATION_LOOP		equ	0	; animation goes 0, 1, 2, 3, 0, 1, 2, 3, ...
ENEMY_ANIMATION_ALTERNATE	equ	1	; animation goes 0, 1, 2, 1, 0, 1, 2, 1, ...

MAP_WIDTH_TILES		equ	128
MAP_HEIGHT_TILES	equ	10
; A game unit is 8 pixels, half a tile.
MAP_WIDTH		equ	MAP_WIDTH_TILES * 2	; in game units
MAP_HEIGHT		equ	MAP_HEIGHT_TILES * 2	; in game units
; The size of the visible portion of the map during gameplay.
PLAYFIELD_WIDTH		equ	24			; in game units
PLAYFIELD_HEIGHT	equ	MAP_HEIGHT		; in game units

SCREEN_WIDTH		equ	320	; in pixels

; The offset of the 40 KB buffer of video memory (inside segment 0xa000) into
; which the entire map for the current stage is rendered.
RENDERED_MAP_BUFFER	equ	0x4000

; Possible values of comic_animation. Comic has 5 frames of animation in each
; direction (5 facing right, 5 facing left).
COMIC_STANDING		equ	0
COMIC_RUNNING_1		equ	1
COMIC_RUNNING_2		equ	2
COMIC_RUNNING_3		equ	3
COMIC_JUMPING		equ	4

; Possible values of comic_facing. comic_facing is used, along with
; comic_animation, to index GRAPHICS_COMIC. The graphic used is
; GRAPHICS_COMIC[comic_facing + comic_animation].
COMIC_FACING_RIGHT	equ	0
COMIC_FACING_LEFT	equ	5

; Possible values of enemy.behavior. ENEMY_BEHAVIOR_FAST may be bitwise ORed
; with any of the other constants.
; http://www.shikadi.net/moddingwiki/Captain_Comic_Map_Format#Executable_file
ENEMY_BEHAVIOR_BOUNCE	equ	1	; like Fire Ball, Brave Bird, Space Pollen, Saucer, Spark, Atom, F.W. Bros.
ENEMY_BEHAVIOR_LEAP	equ	2	; like Bug-eyes, Blind Toad, Beach Ball
ENEMY_BEHAVIOR_ROLL	equ	3	; like Glow Globe
ENEMY_BEHAVIOR_SEEK	equ	4	; like Killer Bee
ENEMY_BEHAVIOR_SHY	equ	5	; like Shy Bird, Spinner
ENEMY_BEHAVIOR_UNUSED	equ	0x7f
ENEMY_BEHAVIOR_FAST	equ	0x80	; move every tick, not every other tick

; Possible values of enemy.state. Values 2..6 and 8..12 are rather animation
; counters that mean the enemy is dying due to being hit by a fireball or
; colliding with Comic, respectively.
ENEMY_STATE_DESPAWNED	equ	0
ENEMY_STATE_SPAWNED	equ	1
ENEMY_STATE_WHITE_SPARK	equ	2
ENEMY_STATE_RED_SPARK	equ	8

; Possible values of enemy.restraint. This field governs whether an enemy is
; slow or fast. A slow enemy alternates between ENEMY_RESTRAINT_MOVE_THIS_TICK
; and ENEMY_RESTRAINT_SKIP_THIS_TICK, moving only on the "move" ticks. Fast
; enemies move every tick. There is a bug with fast enemies in some behavior
; types, however. Though they start in ENEMY_RESTRAINT_MOVE_EVERY_TICK, their
; enemy.restraint field increments on every tick, until it overflows to
; ENEMY_RESTRAINT_MOVE_THIS_TICK and they become a slow enemy. The bug affects
; ENEMY_BEHAVIOR_LEAP, ENEMY_BEHAVIOR_ROLL, and ENEMY_BEHAVIOR_SEEK, but not
; ENEMY_BEHAVIOR_BOUNCE and ENEMY_BEHAVIOR_SHY. Additionally,
; ENEMY_BEHAVIOR_ROLL enemies become slow if they pass a tick where Comic is
; directly above or below them and not moving left or right, and
; ENEMY_BEHAVIOR_SEEK enemies become slow if they bump into a solid map tile.
ENEMY_RESTRAINT_MOVE_THIS_TICK	equ	0	; move this tick; transition to ENEMY_RESTRAINT_SKIP_THIS_TICK
ENEMY_RESTRAINT_SKIP_THIS_TICK	equ	1	; don't move this tick; transition to ENEMY_RESTRAINT_MOVE_THIS_TICK
ENEMY_RESTRAINT_MOVE_EVERY_TICK	equ	2	; values 2..255 all mean "move every tick"

; ENEMY_FACING_* constants are the opposite of COMIC_FACING_*.
ENEMY_FACING_RIGHT	equ	5
ENEMY_FACING_LEFT	equ	0

; Enemies despawn when they are this many horizontal units or more away from
; Comic.
ENEMY_DESPAWN_RADIUS	equ	30

MAX_NUM_ENEMIES		equ	4	; each stage may contain up to 4 enemies
MAX_NUM_DOORS		equ	3	; each stage may contain up to 3 doors

MAX_HP			equ	6	; the HP meter has 12 pips but increases/decreases 2 at a time
MAX_FIREBALL_METER	equ	12	; the fireball meter uses all 12 pips
MAX_NUM_LIVES		equ	5

FIREBALL_DEAD		equ	0xff	; this constant in fireball.y and fireball.x means the fireball is not active
MAX_NUM_FIREBALLS	equ	5	; Comic's maximum firepower and the maximum number of fireballs that can exist

NUM_HIGH_SCORES		equ	10	; the number of entries in the high score list

TELEPORT_DISTANCE	equ	6	; how many game units a teleport moves Comic horizontally

COMIC_GRAVITY		equ	5	; gravity applied to Comic in all levels but space.
COMIC_GRAVITY_SPACE	equ	3	; gravity applied to Comic in the space level.

; TERMINAL_VELOCITY is the maximum downward velocity when falling. It applies
; to Comic himself and to ENEMY_BEHAVIOR_LEAP enemies. (But not
; ENEMY_BEHAVIOR_ROLL; gravity works differently for them.)
TERMINAL_VELOCITY	equ	23	; in units of 1/8 game units per tick, as in comic_y_vel


; Convert (x, y) pixel coordinates to a memory offset. There are SCREEN_WIDTH
; pixels in a row and 8 pixels per byte (in each EGA plane).
%define pixel_coords(x, y)	(((y)*SCREEN_WIDTH+(x))/8)


; The cs register normally points to this section.
section code
sectalign	16

; Dead code: executable actually starts at ..start below, according to the EXE
; header.
	jmp main
	nop

; The program leaves the programmable interrupt handler channel 0 (IRQ 0)
; running with its default frequency divider of 65536, giving a frequency of
; approximately 18.2065 Hz. But the game's tick rate is half that, advancing
; only on odd cycles.
; https://wiki.osdev.org/Programmable_Interval_Timer#Channel_0
game_tick_flag		db	0	; is set to 1 by int8_handler on odd IRQ 0 cycles
irq0_parity		db	0	; cycles 0, 1, 0, 1, ... on each call to int8_handler

; Sound control variables.
sound_is_playing	db	0	; is a sound currently playing?
sound_is_enabled	db	1	; 0 = muted, 1 = unmuted
sound_priority		db	0	; priority of the currently playing sound (0 if no sound is playing)
sound_data_offset	dw	0	; offset of the address of the currently playing sound
sound_note_counter	dw	0	; how many IRQ 0 cycles remain in the current note
sound_data_segment	dw	0	; segment of the address of the currently playing sound

; Keyboard state variables, set by int9_handler.
key_state_esc		db	0
key_state_f1		db	0
key_state_f2		db	0
key_state_f3		db	0	; written but never read
key_state_f4		db	0	; written but never read
key_state_open		db	0
key_state_jump		db	0
key_state_teleport	db	0
key_state_left		db	0
key_state_right		db	0
key_state_fire		db	0

; Default keyboard scancode mappings; may be overridden by configuration or
; KEYS.DEF.
keymap:
scancode_jump		db	SCANCODE_SPACE
scancode_fire		db	SCANCODE_INS
scancode_left		db	SCANCODE_LEFT
scancode_right		db	SCANCODE_RIGHT
scancode_open		db	SCANCODE_ALT
scancode_teleport	db	SCANCODE_CAPSLOCK

; The scancode of the most recent key release event (set in int9_handler).
recent_scancode		db	0


; Install interrupt handlers and load the keymap from KEYS.DEF if present. Jump
; to display_startup_notice when done.
; Output:
;   keymap = loaded from KEYS.DEF if present
..start:
main:
	cld

	; Initialize ds to point at the data segment. It will remain pointing
	; there most of the time, except when we occasionally need to use ds:si
	; for some other purpose.
	mov ax, data
	mov ds, ax

	; Disable sound.
	; https://wiki.osdev.org/PC_Speaker#Through_the_Programmable_Interval_Timer_.28PIT.29
	in al, 0x61
	and al, 0xfc	; unset the 2 low bits to disconnect PIT channel 2 from the speaker
	out 0x61, al

	mov ax, 0x0002	; ah=0x00: set video mode; al=2: 80×25 text
	int 0x10

	; Install our custom interrupt handlers. We store a magic value in
	; interrupt_handler_install_sentinel and expect int3_handler to
	; bit-flip it. Later, at .check_interrupt_handler_install_sentinel, we
	; check that the value is bit-flipped as expected. If it is not, it
	; means our interrupt handlers somehow did not get installed.
.INTERRUPT_HANDLER_INSTALL_SENTINEL	equ	0x25
	call install_interrupt_handlers
	mov al, .INTERRUPT_HANDLER_INSTALL_SENTINEL
	; When given an unknown operation code in al, the int3_handler
	; (otherwise devoted to handling sound) bit-flips the operation code
	; and returns it in al.
	int3	; call int3_handler
	mov [interrupt_handler_install_sentinel], al	; al should be bit-flipped here

; Load key mappings from KEYS.DEF, if present.
.try_load_keymap_file:
	lea dx, [FILENAME_KEYMAP]	; "KEYS.DEF"
	mov ax, 0x3d00	; ah=0x3d: open existing file
	int 0x21
	jc .check_interrupt_handler_install_sentinel	; if file doesn't exist, silently skip keymap loading

	mov bx, ax	; bx = file handle
	; keymap is in the cs segment. File reads go into a buffer starting at
	; ds:dx, so temporarily assign ds=cs.
	push ds
	mov ax, cs
	mov ds, ax
	lea dx, [keymap]	; ds:dx = destination buffer
	mov cx, 6	; cx = number of bytes to read
	mov ax, 0x3f00	; ah=0x3f: read from file or device
	int 0x21
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21
	pop ds		; point ds at the data segment again

.check_interrupt_handler_install_sentinel:
	; Check that out custom interrupts were installed successfully, by
	; checking the value that should have been modified by int3_handler in
	; the `int3` call above.
	xor byte [interrupt_handler_install_sentinel], 0xff	; undo the bit-flip that int3_handler should have done
	lea bx, [STARTUP_NOTICE_TEXT]
	cmp byte [interrupt_handler_install_sentinel], .INTERRUPT_HANDLER_INSTALL_SENTINEL	; as expected?
	je display_startup_notice	; all good, continue
	jmp title_sequence

; Display STARTUP_NOTICE_TEXT and wait for a keypress to configure the
; keyboard, quit, or begin the game.
display_startup_notice:
	call display_xor_decrypt	; decrypt and display STARTUP_NOTICE_TEXT

	xor ax, ax	; ah=0x00: get keystroke; returned al is ASCII code
	int 0x16
	cmp al, 'k'
	je setup_keyboard
	cmp al, 'K'
	je setup_keyboard
	jmp check_ega_support

; Do the interactive keyboard setup. Optionally save the configured key mapping
; to KEYS.DEF. Jump to title_sequence when done.
setup_keyboard:
	push ds		; temporarily work relative the data segment containing input-related strings
	mov ax, input_config_strings
	mov ds, ax

	call input_unmapped_scancode	; eat the 'k'/'K' that was already pressed and released to get us to this screen

	mov ax, 0x0002	; ah=0x00: set video mode; al=2: 80×25 text
	int 0x10

	lea dx, [STR_DEFINE_KEYS]	; "Define Keys"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21

	; Initialize the key mapping. input_unmapped_scancode blocks until a
	; key is pressed that does not conflict with one of these variables
	; already assigned. scancode_teleport does not need to be initialized
	; because it is assigned last; there are no later key assignments that
	; would need to be compared against it.
	mov byte [cs:scancode_jump], 0
	mov byte [cs:scancode_left], 0
	mov byte [cs:scancode_right], 0
	mov byte [cs:scancode_fire], 0
	mov byte [cs:scancode_open], 0

	lea dx, [STR_MOVE_LEFT]		; "Move Left"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_left], al

	lea dx, [STR_MOVE_RIGHT]	; "Move Right"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_right], al

	lea dx, [STR_JUMP]		; "Jump"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_jump], al

	lea dx, [STR_FIREBALL]		; "Fireball"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_fire], al

	lea dx, [STR_OPEN_DOOR]		; "Open Door"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_open], al

	lea dx, [STR_TELEPORT]		; "Teleport"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	call input_unmapped_scancode
	mov byte [cs:scancode_teleport], al

	; Clear the BIOS keyboard buffer.
	xor ax, ax
	mov es, ax	; set es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head

	lea dx, [STR_THIS_SETUP_OK]	; "This setup OK? (y/n)"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	xor ax, ax	; ah=0x00: get keystroke
	int 0x16
	cmp al, 'n'
	je .start_over
	cmp al, 'N'
	je .start_over
	; Any key other than 'n'/'N' means the setup is OK.

	lea dx, [STR_SAVE_SETUP_TO_DISK]	; "Save setup to disk? (y/n)"
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21
	xor ax, ax	; ah=0x00: get keystroke
	int 0x16
	cmp al, 'y'
	je .save_keymap_file
	cmp al, 'Y'
	je .save_keymap_file
	; Any key other than 'y'/'Y' means don't save the keymap.

	pop ds		; revert the temporary data segment
	jmp check_ega_support

.start_over:
	pop ds		; revert the temporary data segment
	jmp setup_keyboard

.save_keymap_file:
	pop ds		; revert the temporary data segment
	lea dx, [FILENAME_KEYMAP]	; "KEYS.DEF"
	mov ax, 0x3c00	; ah=0x3c: create or truncate file
	xor cx, cx	; file attributes
	int 0x21
	jc .create_failed	; failure to create the file is a fatal error

	mov bx, ax	; bx = file handle
	mov si, ds	; save ds
	mov ax, cs
	mov ds, ax	; temporarily set ds = cs
	mov ax, 0x4000	; ah=0x40: write to file
	mov cx, 6	; cx = number of bytes to write
	lea dx, [keymap]	; ds:dx = data to write
	int 0x21
	mov ds, si	; restore ds
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21

	jmp check_ega_support
	nop		; dead code

.create_failed:
	jmp terminate_program.no_audiovideo_cleanup

; Wait for a key to be pressed whose scancode has not already been assigned to
; scancode_jump, scancode_left, scancode_right, scancode_fire, or
; scancode_open; is not Escape; and is within the range of permitted scancodes.
; Display a text representation of the scancode.
; Output:
;   al = scancode of key pressed
input_unmapped_scancode:
	; Loop until a key is released.
	mov byte [cs:recent_scancode], 0	; set in int9_handler
.loop:
	cmp byte [cs:recent_scancode], 0
	je .loop

	mov bl, [cs:recent_scancode]
	xor bh, bh	; bx = scancode

	; Compare to already-mapped scancodes.
	cmp bl, [cs:scancode_jump]
	je input_unmapped_scancode
	cmp bl, [cs:scancode_left]
	je input_unmapped_scancode
	cmp bl, [cs:scancode_right]
	je input_unmapped_scancode
	cmp bl, [cs:scancode_fire]
	je input_unmapped_scancode
	cmp bl, [cs:scancode_open]
	je input_unmapped_scancode
	; No need to compare against scancode_teleport, because it is the last
	; mapping to be assigned and so cannot pre-empt any other mappings.

	; bl now contains a scancode that is not mapped to any game action.
	; Check it against other reserved scancodes.
	dec bx
	jz input_unmapped_scancode	; Escape is reserved, try again
	cmp bx, SCANCODE_INS - 1	; scancode > Ins? (subtract 1 to compensate for bx decrement)
	jg input_unmapped_scancode	; scancodes outside the range 2..82 are not allowed

	; Display a text representation of the scancode. Multiply by 8 to index
	; SCANCODE_LABELS.
	shl bx, 1
	shl bx, 1
	shl bx, 1	; (scancode - 1) * 8
	lea dx, [SCANCODE_LABELS]
	add dx, bx	; SCANCODE_LABELS[scancode - 1]
	mov ah, 0x09	; ah=0x09: write string to standard output
	int 0x21

	mov al, [cs:recent_scancode]	; return the scancode in al
	ret

check_ega_support:
	; Check for sufficient EGA support.
	mov ax, 0x000d	; ah=0x00: set video mode; al=13: 320×200 16-color EGA
	int 0x10
	mov ax, 0x0f00	; ah=0x0f: get video mode
	int 0x10
	cmp al, 13	; was video mode 13 actually set?
	jne .video_mode_error	; if not, it's a fatal error
	mov ax, 0x1200	; ah=0x12, bl=0x10: get EGA info
	mov bx, 0x0010
	int 0x10
	cmp bl, 3	; bl=3: 256K installed memory
	jge title_sequence	; all good with EGA support

.video_mode_error:
	mov ax, 0x0002	; ah=0x00: set video mode; al=2: 80×25 text
	int 0x10
	mov ah, 0x09	; ah=0x09: write string to standard output
	lea dx, [VIDEO_MODE_ERROR_MESSAGE]	; "This program requires an EGA adapter with 256K installed\n\r"
	int 0x21
	jmp terminate_program.no_audiovideo_cleanup

; Switch into graphics mode. Show the title graphic and await keypresses to
; advance through the story screen and items/enemies screen. Jump to
; initialize_lives_sequence when done.
title_sequence:
	lea dx, [FILENAME_TITLE_GRAPHIC]	; "sys000.ega"

	mov ax, 0xa000
	mov es, ax	; es points to video memory

	; The program uses various 8 KB buffers within the video memory segment
	; 0xa000. The two most important are a000:0000 and a000:2000, which are
	; the ones swapped between on every game tick for double buffering. The
	; variable offscreen_video_buffer_ptr and function swap_video_buffers
	; handle double buffering, swapping the displayed offset between 0x0000
	; and 0x2000.
	;
	; The title sequence juggles a few video buffers, loading fullscreen
	; graphics from .EGA files into memory and switching to them as
	; appropriate. sys000.ega is loaded into a000:8000 and displayed
	; immediately. sys001.ega is loaded into a000:a000 after 10 ticks have
	; elapsed, but not immediately displayed. Then sys003.ega is loaded
	; into *both* buffers a000:0000 and a000:2000, but also not immediately
	; displayed. sys003.ega contains the gameplay UI and so needs to be in
	; the double buffers. Then video buffer switches to a000:a000 to
	; display sys001.ega. sys004.ega is loaded into a000:8000 (replacing
	; sys000.ega) and displayed after a keypress. Finally, we switch to the
	; buffer a000:2000, which contains sys003.ega, after another keypress,
	; in preparation for starting gameplay.
	;
	; The complete rendered map for the current stage also lives in video
	; memory, between a000:4000 and a000:dfff. That happens after the title
	; sequence is over, so it doesn't conflict with the use of that region
	; of memory here. See render_map and blit_map_playfield_offscreen.

	; Load the title graphic into video buffer 0x8000.
	mov di, 0x8000
	call load_fullscreen_graphic

	; Start playing the title music.
	lea bx, [SOUND_TITLE]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	; Switch to the title graphic and fade in.
	call palette_darken
	mov cx, 0x8000	; switch to video buffer 0x8000, into which we loaded the title screen graphic
	call switch_video_buffer
	call palette_fade_in

	mov ax, 10	; linger on the title screen for 10 ticks
	call wait_n_ticks

	; Load the story graphic into video buffer 0xa000.
	lea dx, [FILENAME_STORY_GRAPHIC]	; "sys001.ega"
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	mov di, 0xa000
	call load_fullscreen_graphic

	; Load the UI graphic into video buffer 0x0000.
	lea dx, [FILENAME_UI_GRAPHIC]	; "sys003.ega"
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	xor di, di	; video buffer 0x0000
	call load_fullscreen_graphic

	; Copy the UI graphic from video buffer 0x0000 to video buffer 0x2000
	; (these are the two buffers that swap every tick during gameplay). We
	; need to copy the graphic one plane at a time, into the same nominal
	; buffer.
	push ds
	mov ax, 0xa000
	mov ds, ax	; ds points to video memory
	; Copy plane 0 from video buffer 0x0000 to video buffer 0x2000.
	mov ah, 1	; ah = mask for plane 0
	xor bx, bx	; bl = index of plane 0
	call enable_ega_plane_write
	mov cx, 4000	; 4000 words = 8000 bytes = size of one plane
	xor si, si
	mov di, 0x2000
	rep movsw	; copy from ds:si to es:di
	; Copy plane 1 from video buffer 0x0000 to video buffer 0x2000.
	mov ah, 2	; ah = mask for plane 1
	mov bx, 1	; bl = index of plane 1
	call enable_ega_plane_write
	mov cx, 4000	; 4000 words = 8000 bytes = size of one plane
	xor si, si
	mov di, 0x2000
	rep movsw	; copy from ds:si to es:di
	; Copy plane 2 from video buffer 0x0000 to video buffer 0x2000.
	mov ah, 4	; ah = mask for plane 2
	mov bx, 2	; bl = index of plane 2
	call enable_ega_plane_write
	mov cx, 4000	; 4000 words = 8000 bytes = size of one plane
	xor si, si
	mov di, 0x2000
	rep movsw	; copy from ds:si to es:di
	; Copy plane 3 from video buffer 0x0000 to video buffer 0x2000.
	mov ah, 8	; ah = mask for plane 3
	mov bx, 3	; bl = index of plane 3
	call enable_ega_plane_write
	mov cx, 4000	; 4000 words = 8000 bytes = size of one plane
	xor si, si
	mov di, 0x2000
	rep movsw	; copy from ds:si to es:di
	pop ds

	; Switch to the story graphic and fade in.
	call palette_darken
	mov cx, 0xa000	; switch to video buffer 0xa000, into which we loaded the story graphic
	call switch_video_buffer
	call palette_fade_in

	; Load the items graphic into video buffer 0x8000 (over the title
	; screen graphic).
	lea dx, [FILENAME_ITEMS_GRAPHIC]	; "sys004.ega"
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	mov di, 0x8000
	call load_fullscreen_graphic

	; Clear the BIOS keyboard buffer.
	xor ax, ax
	mov es, ax	; es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head

	; Wait for a keystroke at the story screen.
	int 0x16	; ah=0x00: get keystroke

	; Switch to the items graphic.
	mov cx, 0x8000
	call switch_video_buffer

	; Wait for a keystroke at the items screen.
	xor ax, ax	; ah=0x00: get keystroke
	int 0x16	; wait for any keystroke

	; Switch to the UI graphic.
	mov cx, 0x2000
	call switch_video_buffer

	jmp initialize_lives_sequence

.terminate_program_trampoline:
	jmp terminate_program

; Play the animation of awarding the player their initial lives, first counting
; up to MAX_NUM_LIVES, then subtracting one to represent the life in use at the
; start of the game. Jump to load_new_level when done, using the default
; current_level_number value of LEVEL_NUMBER_FOREST and default
; current_stage_number value of 0.
; Output:
;   comic_num_lives = MAX_NUM_LIVES - 1
initialize_lives_sequence:
	mov al, [cs:sound_is_enabled]
	push ax		; save sound mute status
	mov ax, SOUND_MUTE	; so the extra life sound doesn't play
	int3
	mov cx, MAX_NUM_LIVES	; initially award max lives
.loop:
	mov ax, 1
	call wait_n_ticks	; wait 1 tick between each life awarded
	push cx		; award_extra_life uses cx
	call award_extra_life
	pop cx
	loop .loop

	mov ax, SOUND_STOP	; stop the extra life sound that is playing but muted
	int3
	pop ax
	mov [cs:sound_is_enabled], al	; restore the saved sound mute status

	mov ax, 3
	call wait_n_ticks	; wait 3 ticks before subtracting the initial life
	call lose_a_life	; subtract 1 life

	; Start playing the title music recapitulation.
	lea bx, [SOUND_TITLE_RECAP]
	mov ax, SOUND_PLAY
	mov cx, 1	; priority 1
	int3

	cmp byte [comic_num_lives], MAX_NUM_LIVES - 1	; is the number of lives 1 less than the max?
	je .num_lives_ok	; always true
	; A weird line of dead code here: if, after counting up to the max and
	; subtracting 1, the number of lives is not 1 less than the max, then
	; set the game to end after 200 ticks. (Actually 199 ticks, because it
	; ends when the counter reaches 1.)
	; https://tcrf.net/The_Adventures_of_Captain_Comic_(DOS)#Unused_Instant-Win_Mode
	mov byte [win_counter], 200
.num_lives_ok:
	jmp load_new_level

; Set the palette entries for colors 2, 10, and 12 (ordinarily green, bright
; green, bright red) to black (rgbRGB 000000). The effect is instant, with no
; delay.
palette_darken:
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x0002	; bh = color value (black), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x000a	; bh = color value (black), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x000c	; bh = color value (black), bl = palette register index
	int 0x10
	ret

; Fade the palette entries for colors 2, 10, and 12 to their natural colors
; (green, bright green, and bright red) over 5 ticks.
palette_fade_in:
	; Set all three palette entries to dark gray.
	; I don't know why the selected color is 0x18 (= rgbRGB 011000). It
	; seems like dark gray should set all the low-intensity color bits;
	; i.e., 0x38 (= rgbRGB 111000) (which also works).
	; https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter#Color_palette
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1802	; bh = color value (dark gray), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x180a	; bh = color value (dark gray), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x180c	; bh = color value (dark gray), bl = palette register index
	int 0x10

	mov ax, 1
	call wait_n_ticks

	; Set all three palette entries to light gray (rgbRGB 000111).
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x0702	; bh = color value (light gray), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x070a	; bh = color value (light gray), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x070c	; bh = color value (light gray), bl = palette register index
	int 0x10

	mov ax, 1
	call wait_n_ticks

	; Set palette entries 10 and 12 to white, keeping entry 2 at light gray.
	; Here too, I don't understand why the color 0x1f (= rgbRGB 011111) is
	; used instead of 0x3f (= rgbRGB 111111).
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x0702	; bh = color value (light gray), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1f0a	; bh = color value (white), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1f0c	; bh = color value (white), bl = palette register index
	int 0x10

	mov ax, 1
	call wait_n_ticks

	; Set palette entry 2 to green (its natural color) and 10 to bright
	; green (its natural color); and keep entry 12 at white.
	; Here I would have expected 0x3a (= rgbRGB 111010) for bright green.
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x0202	; bh = color value (green), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1a0a	; bh = color value (bright green), bl = palette register index
	int 0x10
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1f0c	; bh = color value (white), bl = palette register index
	int 0x10

	mov ax, 1
	call wait_n_ticks

	; Set palette entry 12 to bright red (its natural color).
	; I would have expected color 0x3c (= rgbRGB 111100).
	mov ax, 0x1000	; ah=0x10: set single palette register
	mov bx, 0x1c0c	; bh = color value (bright red), bl = palette register index
	int 0x10

	mov ax, 1
	call wait_n_ticks

	ret

; Wait for n game ticks to elapse.
; Input:
;   ax = number of ticks
; Output:
;   ax = 0
;   game_tick_flag = 0
wait_n_ticks:
.loop:
	cmp byte [cs:game_tick_flag], 1
	jne .loop	; loop until the current tick is finished
	mov byte [cs:game_tick_flag], 0
	dec ax		; decrement counter
	jne .loop	; wait again, until counter is zero
	ret

; Mute the sound, restore video mode 2, restore the original interrupt
; handlers, and exit.
terminate_program:
	mov ax, SOUND_MUTE
	int3
	mov ax, 0x0002	; ah=0x00: set video mode; al=2: 80×25 text
	int 0x10

	lea bx, [TERMINATE_PROGRAM_TEXT]
	call display_xor_decrypt

; Just restore original interrupt handlers and terminate.
.no_audiovideo_cleanup:
	call restore_interrupt_handlers
	mov ah, 0x4c	; ah=0x4c: terminate with return code
	int 0x21

; Decrypt an obfuscated string and display it on the console. The end of the
; string is marked by the byte value 0x1a (encrypted value 0x3f).
; Input:
;   bx = pointer to encrypted string
display_xor_decrypt:
.loop:
	mov al, [bx]	; get next byte into al
	inc bx
	xor al, XOR_ENCRYPTION_KEY	; decrypt
	cmp al, 0x1a	; terminator sentinel?
	je .finished
	push bx
	mov bx, 0x0007	; bh=0x00: page number 0, bl=0x0c: color 7
	mov ah, 0x0e	; ah=0x0e: teletype output
	int 0x10	; output al
	pop bx
	jmp .loop	; and try the next byte
.finished:
	ret

saved_int9_handler_offset	dw	0
saved_int9_handler_segment	dw	0
; INT 9 is called for keyboard events. Update the state of the key_state_*
; variables and updates recent_scancode in the case of a key press. Call the
; original INT 9 handler too.
; Input:
;   saved_int9_handler_offset:saved_int9_handler_segment = address of original INT 9 handler
;   scancode_jump, scancode_fire, scancode_left, scancode_right, scancode_open,
;     scancode_teleport = scancodes for game actions
; Output:
;   recent_scancode = scancode of the most recent key release (not modified on key press events)
;   key_state_jump, key_state_fire, key_state_left, key_state_right,
;     key_state_open, key_state_teleport, key_state_esc, key_state_f1,
;     key_state_f2, key_state_f3, key_state_f4 = set to 0 or 1 according to
;     whether the key is currently released or pressed
int9_handler:
	push ax
	push bx
	push cx
	push dx
	; Read the scancode into al and call the original handler before
	; continuing.
	in al, 0x60	; read the keyboard scancode
	push ax		; save it
	pushf		; push flags for recursive call to original INT 9 handler
	call far [cs:saved_int9_handler_offset]	; call the original INT 9 handler
	pop ax		; al = keyboard scancode
	mov dx, 1	; dl distinguishes key pressed/released; initially assume pressed
	test al, 0x80	; most significant bit cleared means key pressed; set means key released
	jz .continue	; if 0, it was indeed a press
.released:
	and al, 0x7f
	mov [cs:recent_scancode], al	; clear high bit and store scancode of released key
	xor dx, dx	; unset the "key pressed" flag before continuing
.continue:
	; Check for the scancodes that are mapped to game actions.
	cmp al, [cs:scancode_jump]
	je .jump
	cmp al, [cs:scancode_fire]
	je .fire
	cmp al, [cs:scancode_left]
	je .left
	cmp al, [cs:scancode_right]
	je .right
	cmp al, [cs:scancode_open]
	je .open
	cmp al, [cs:scancode_teleport]
	jne .other
.teleport:
	mov [cs:key_state_teleport], dl
	jmp .return
.jump:
	mov [cs:key_state_jump], dl
	jmp .return
.fire:
	mov [cs:key_state_fire], dl
	jmp .return
.left:
	mov [cs:key_state_left], dl
	jmp .return
.right:
	mov [cs:key_state_right], dl
	jmp .return
.open:
	mov [cs:key_state_open], dl
	jmp .return
.other:
	; Check for the non-mappable scancodes.
	cmp al, 1	; scancode == Escape?
	je .esc
	sub al, SCANCODE_F1	; scancode < F1?
	jb .return
	cmp al, SCANCODE_F4 - SCANCODE_F1 + 1	; scancode > F4?
	jae .return
	lea bx, [key_state_f1]
	xor ah, ah
	add bx, ax
	mov [cs:bx], dl	; set key_state_f1, key_state_f2, key_state_f3, or key_state_f4
	jmp .return
.esc:
	mov [cs:key_state_esc], dl
.return:
	pop dx
	pop cx
	pop bx
	pop ax
	iret

saved_int35_handler_offset	dw	0
saved_int35_handler_segment	dw	0
; INT 35 is called for Ctrl-Break. Jump to terminate_program.
int35_handler:
	jmp terminate_program

saved_int8_handler_offset	dw	0
saved_int8_handler_segment	dw	0
; INT 8 is called for every cycle of the programmable interval timer (IRQ 0).
; Poll the F1 and F2 keys (on odd interrupts only) and advance the current
; sound playback. Tail call into the original INT 8 handler.
; Input:
;   saved_int8_handler_offset:saved_int8_handler_segment = address of original INT 8 handler
;   irq0_parity = even/odd counter of calls to this interrupt handler
;   key_state_f1 = if 1, unmute the sound
;   key_state_f2 = if 1, mute the sound
;   sound_is_playing = if 1, deal with the currently playing sound
;   sound_data_segment:sound_data_offset = address of the current sound
;   sound_note_counter = how many more interrupts to continue playing the current note in the current sound
;   sound_is_enabled = if 1, actually send sound data to the PC speaker
; Output:
;   irq0_parity = opposite of its input value
;   game_tick_flag = set to 1 if irq0_parity was odd
;   sound_data_offset = advanced by 4 bytes if a note transition occurred
;   sound_is_playing = set to 0 if the current sound finished playing
;   sound_priority = set to 0 if the current sound finished playing
int8_handler:
	push ax

	; irq0_parity keeps track of whether we are in an even or an odd call
	; to this interrupt handler. irq0_parity advances 0→1 or 1→0 on each
	; interrupt.
	mov al, [cs:irq0_parity]
	inc al
	cmp al, 2	; did irq0_parity overflow to 2?
	jl .store_irq0_parity	; irq0_parity was 0, now is 1
	; Otherwise irq0_parity was 1, now is 2 (and will become 1→0 at .wrap_irq0_parity below).

.irq0_parity_odd:
	; On odd interrupts, poll the F1/F2 keys.
	mov byte [cs:game_tick_flag], 1	; flag the beginning of a game tick

.try_F1:
	cmp byte [cs:key_state_f1], 1
	jne .try_F2
	mov ax, SOUND_UNMUTE
	int3		; call sound control interrupt
.try_F2:
	cmp byte [cs:key_state_f2], 1
	jne .wrap_irq0_parity
	mov ax, SOUND_MUTE
	int3		; call sound control interrupt

.wrap_irq0_parity:
	xor al, al	; wrap irq0_parity to 0

.store_irq0_parity:
	; Store the new value of irq0_parity, which has just transitioned
	; either 0→1 or 1→0.
	mov [cs:irq0_parity], al

.sound:
	push es
	push bx
	push dx
	; Deal with sound on both even and odd interrupts.
	mov ax, [cs:sound_data_segment]
	mov es, ax
	cmp byte [cs:sound_is_playing], 1
	jne .disable_sound_output

	; A sound is currently playing.
	dec word [cs:sound_note_counter]	; is the current note over yet?
	jg .return	; if not, we're done for now
	; Time to change to the next note. The data for the next note is stored
	; as 2 consecutive words. The first is a frequency divider value, and
	; the second is a duration.
	mov bx, [cs:sound_data_offset]
	add word [cs:sound_data_offset], 2	; get the next frequency divider
	mov ax, [es:bx]
	or ax, ax	; a frequency divider of 0 is a sentinel indicating end of sound
	jz .sound_finished

	; Program the frequency divider into PIT channel 2 (connected to the PC
	; speaker).
	mov bx, ax
	in al, 0x61
	and al, 0xfc	; disable sound output while we change settings
	out 0x61, al
	; Write the frequency divider value.
	; https://wiki.osdev.org/Programmable_Interval_Timer#I.2FO_Ports
	; 0xb6 = 0b10110110
	;          10 = channel 2 (connected to the PC speaker)
	;            11 = access mode lobyte/hibyte
	;              011 = mode 3 (square wave generator)
	;                 0 = 16-bit binary
	mov al, 0xb6
	out 0x43, al	; PIT mode/command register
	mov al, bl	; low byte of frequency divider
	out 0x42, al	; PIT channel 2 data port
	mov al, bh	; high byte of frequency divider
	out 0x42, al	; PIT channel 2 data port

	mov bx, [cs:sound_data_offset]
	add word [cs:sound_data_offset], 2	; get the duration of the next note
	mov ax, [es:bx]
	mov [cs:sound_note_counter], ax		; and store in sound_note_counter

	cmp byte [cs:sound_is_enabled], 1	; if the sound is muted, just return
	jne .return

	in al, 0x61
	or al, 0x03	; enable sound output
	out 0x61, al
	jmp .return

.sound_finished:
	mov byte [cs:sound_is_playing], 0
	mov byte [cs:sound_priority], 0

.disable_sound_output:
	in al, 0x61
	and al, 0xfc	; disable sound output
	out 0x61, al

.return:
	pop dx
	pop bx
	pop es
	pop ax
	jmp far [cs:saved_int8_handler_offset]	; tail call into the original interrupt handler

saved_int3_handler_offset	dw	0
saved_int3_handler_segment	dw	0
; INT 3 controls the sound. It is called into explicitly by other parts of the
; code using the `int3` instruction. Each sound has an associated priority. A
; new sound will replace the current sound only if the new sound has greater or
; equal priority.
; Input:
;   al = operation code:
;     al=0: unmute
;     al=1: play a sound
;       dx:bx = pointer to the beginning of sound data
;       cl = priority of the new sound
;     al=2: mute
;     al=3: stop the current sound effect
;     al=4: query whether a sound is currently playing
;     any other value of al, set al = ~al and return
;   sound_priority = priority of the currently playing sound (0 if none playing)
;   sound_is_playing = set to 1 if a sound is currently playing, 0 otherwise
; Output:
;   ax = if al was 4 on input, set to the value of sound_is_playing
;   al = if al was not in 0..4, the bit-flipped value of the al input
;   sound_is_enabled = set to 0 with al == 2, set to 1 with al == 0
;   sound_data_offset:sound_data_segment = set to dx:bx with al == 1
;   sound_is_playing = set to 1 with al == 1, set to 0 with al == 3
;   sound_priority = set to the greater of the old and new priorities with al == 0, set to 0 with al == 3
;   sound_note_counter = set to 1 with al == 1
int3_handler:
	; Dispatch on the operation code in al.
	or al, al
	je .unmute
	cmp al, SOUND_PLAY
	je .play
	cmp al, SOUND_MUTE
	je .mute
	cmp al, SOUND_STOP
	je .stop
	cmp al, SOUND_QUERY
	je .query
	; For any other value, invert al and return. This action is used by the
	; main function to check for the correct installation of interrupt
	; handlers.
	xor al, 0xff
	jmp .return
.play:
	cmp [cs:sound_priority], cl	; is the new sound's priority less than what's already playing?
	jg .return
	mov [cs:sound_priority], cl	; raise sound_priority to that of the new sound
	cli
	mov byte [cs:sound_is_playing], 1
	mov [cs:sound_data_offset], bx
	mov ax, ds
	mov [cs:sound_data_segment], ax
	mov word [cs:sound_note_counter], 1
	sti
	jmp .return
.unmute:
	cli
	mov byte [cs:sound_is_enabled], 1
	sti
	jmp .disable_sound
.mute:
	cli
	mov byte [cs:sound_is_enabled], 0
	sti
	jmp .disable_sound
.stop:
	cli
	mov byte [cs:sound_is_playing], 0
	mov byte [cs:sound_priority], 0
	sti
	jmp .disable_sound
.query:
	mov al, [cs:sound_is_playing]
	xor ah, ah
	jmp .return
.disable_sound:
	in al, 0x61
	and al, 0xfc
	out 0x61, al
.return:
	iret

; Install the custom interrupt handlers int3_handler, int8_handler,
; int9_handler, and int35_handler. Store the addresses of the original
; handlers.
; Output:
;   saved_int3_handler_segment:saved_int3_handler_offset = address of original INT 3 handler
;   saved_int8_handler_segment:saved_int8_handler_offset = address of original INT 8 handler
;   saved_int9_handler_segment:saved_int9_handler_offset = address of original INT 9 handler
;   saved_int35_handler_segment:saved_int35_handler_offset = address of original INT 35 handler
install_interrupt_handlers:
	; The interrupt vector table starts at 0000:0000. Each entry is 4
	; bytes: 2 bytes offset, 2 bytes segment.
	; https://wiki.osdev.org/Interrupt_Vector_Table
	push ds
	xor ax, ax
	mov ds, ax
	cli

	; INT 9
	lea bx, [saved_int9_handler_offset]
	mov ax, [9*4+2]		; original segment
	mov [cs:bx+2], ax	; save it
	mov ax, [9*4+0]		; original offset
	mov [cs:bx+0], ax	; save it
	lea ax, [int9_handler]
	mov [9*4+0], ax		; overwrite offset
	mov [9*4+2], cs		; overwrite segment

	; INT 8
	lea bx, [saved_int8_handler_offset]
	mov ax, [8*4+2]		; original segment
	mov [cs:bx+2], ax	; save it
	mov ax, [8*4+0]		; original offset
	mov [cs:bx+0], ax	; save it
	lea ax, [int8_handler]
	mov [8*4+0], ax		; overwrite offset
	mov [8*4+2], cs		; overwrite segment

	; INT 35
	lea bx, [saved_int35_handler_offset]
	mov ax, [35*4+2]	; original segment
	mov [cs:bx+2], ax	; save it
	mov ax, [35*4+0]	; original offset
	mov [cs:bx+0], ax	; save it
	lea ax, [int35_handler]
	mov [35*4+0], ax	; overwrite offset
	mov [35*4+2], cs	; overwrite segment

	; INT 3
	lea bx, [saved_int3_handler_offset]
	mov ax, [3*4+2]		; original segment
	mov [cs:bx+2], ax	; save it
	mov ax, [3*4+0]		; original offset
	mov [cs:bx+0], ax	; save it
	lea ax, [int3_handler]
	mov [3*4+0], ax		; overwrite offset
	mov [3*4+2], cs		; overwrite segment

	sti
	pop ds
	ret

; Restore the original handlers for INT 3, INT 8, INT 9, and INT 31 that were
; replaced in install_interrupt_handlers.
; Input:
;   saved_int3_handler_segment:saved_int3_handler_offset = address of original INT 3 handler
;   saved_int8_handler_segment:saved_int8_handler_offset = address of original INT 8 handler
;   saved_int9_handler_segment:saved_int9_handler_offset = address of original INT 9 handler
;   saved_int35_handler_segment:saved_int35_handler_offset = address of original INT 35 handler
restore_interrupt_handlers:
	; The interrupt vector table starts at 0000:0000. Each entry is 4
	; bytes: 2 bytes offset, 2 bytes segment.
	; https://wiki.osdev.org/Interrupt_Vector_Table
	push ds
	xor ax, ax
	mov ds, ax
	cli

	; INT 9
	lea bx, [saved_int9_handler_offset]
	mov ax, [cs:bx+2]	; saved segment
	mov [9*4+2], ax		; restore it
	mov ax, [cs:bx+0]	; saved offset
	mov [9*4+0], ax		; restore it

	; INT 8
	lea bx, [saved_int8_handler_offset]
	mov ax, [cs:bx+2]	; saved segment
	mov [8*4+2], ax		; restore it
	mov ax, [cs:bx+0]	; saved offset
	mov [8*4+0], ax		; restore it

	; INT 35
	lea bx, [saved_int35_handler_offset]
	mov ax, [cs:bx+2]	; saved segment
	mov [35*4+2], ax	; restore it
	mov ax, [cs:bx+0]	; saved offset
	mov [35*4+0], ax	; restore it

	; INT 3
	lea bx, [saved_int3_handler_offset]
	mov ax, [cs:bx+2]	; saved segment
	mov [3*4+2], ax		; restore it
	mov ax, [cs:bx+0]	; saved offset
	mov [3*4+0], ax		; restore it

	sti
	pop ds
	ret

; Load a fullscreen graphic from a .EGA file and decode its to a specified
; destination buffer.
; Input:
;   ds:dx = address of filename
;   es:di = 32000-byte destination buffer
load_fullscreen_graphic:
	; Load the entire file into load_fullscreen_graphic_buffer, without
	; decoding.
	mov ax, 0x3d00	; ah=0x3d: open existing file
	int 0x21
	jnc .open_ok	; failure to open a .EGA file is a fatal error
	jmp title_sequence.terminate_program_trampoline
.open_ok:
	mov bx, ax	; bx = file handle
	push ds
	mov ax, input_config_strings
	mov ds, ax	; load_fullscreen_graphic_buffer is in the input_config_strings segment
	lea dx, [load_fullscreen_graphic_buffer]	; ds:dx = destination buffer
	mov cx, 0x7fff	; cx = number of bytes to read (overflow possible here; buffer is only 0x3e82 bytes)
	mov ax, 0x3f00	; ah=0x3f: read from file or device
	int 0x21	; no error check
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21

	; Decode file contents as RLE.
	; http://www.shikadi.net/moddingwiki/Captain_Comic_Image_Format#File_format
	; Ignore the first word, which is the plane size, always 8000.
	lea ax, [load_fullscreen_graphic_buffer + 2]
	mov si, ax
	mov ah, 1	; blue plane mask
	xor bx, bx	; blue plane index
	call enable_ega_plane_write
	call rle_decode
	mov ah, 2	; green plane mask
	mov bx, 1	; green plane index
	call enable_ega_plane_write
	call rle_decode
	mov ah, 4	; red plane mask
	mov bx, 2	; red plane index
	call enable_ega_plane_write
	call rle_decode
	mov ah, 8	; intensity plane mask
	mov bx, 3	; intensity plane index
	call enable_ega_plane_write
	call rle_decode
	pop ds
	ret

; Decode RLE data until a certain number of bytes have been decoded.
; Input:
;   ds:si = input RLE data
;   es:di = output buffer
;   load_fullscreen_graphic_buffer = first word is the number of bytes to decode
;     (returns when at least that many bytes have been written to es:di)
; Output:
;   ds:si = advanced
;   es:di = unchanged
rle_decode:
	; http://www.shikadi.net/moddingwiki/Captain_Comic_Image_Format#File_format
	mov bx, di
.loop:
	lodsb		; read from [ds:si] into al
	test al, 0x80	; is the high bit set?
	jnz .repeat
.copy:
	; High bit not set means copy the next n bytes.
	xor ah, ah
	mov cx, ax	; cx = n
	rep movsb	; copy n bytes from ds:si to es:di
	jmp .next
.repeat:
	; High bit set means repeat the next byte n times.
	xor ah, ah
	and al, 0x7f	; unset the high bit
	mov cx, ax	; cx = n
	lodsb		; read from [ds:si] into al
	rep stosb	; repeat al into [es:di] n times
.next:
	mov ax, di
	sub ax, bx	; how many bytes have been written so far?
	cmp ax, [load_fullscreen_graphic_buffer]	; compare with the plane size at the beginning of the buffer
	jl .loop	; loop until we have decoded enough
	mov di, bx
	ret

; Initialize global data structures as appropriate for the value of
; current_level_number. Jump to load_shp_files when done.
;
; A "level" is a group of three stages sharing the same tileset, like CASTLE,
; FOREST, etc. A "stage" is a single 128×10 map, corresponding to a .PT file.
;
; Input:
;   current_level_number = level number to load
;   comic_has_lantern = if != 1, and current_level_number == LEVEL_NUMBER_CASTLE, black out tile graphics
; Output:
;   current_level = filled-out level struct, copied from level_data
;   tileset_buffer = array of tile graphics
load_new_level:
	; Copy the level data from LEVEL_DATA_POINTERS[current_level_number] to
	; the structure at current_level.
	lea bx, [LEVEL_DATA_POINTERS]
	mov al, [current_level_number]
	xor ah, ah
	shl ax, 1
	add bx, ax
	mov si, [bx]	; LEVEL_DATA_POINTERS[current_level_number]
	lea di, [current_level]
	push ds
	pop es		; es = ds; es:di points to current_level
	mov ax, level_data
	mov ds, ax	; ds = level_data; ds:si points to static level data
	mov cx, level_size	; cx = number of bytes to copy
	rep movsb	; copy from ds:si to es:di
	push es
	pop ds		; restore the usual ds

	; Load the .TT2 file (tileset graphics).
	lea dx, [current_level + level.tt2_filename]
	mov ax, 0x3d00	; ah=0x3d: open existing file
	int 0x21
	jnc .tt2_open_ok	; failure to open a .TT2 file is a fatal error
	jmp .terminate_program_trampoline
	nop		; dead code
.tt2_open_ok:
	mov bx, ax	; bx = file handle
	lea dx, [tileset_buffer]	; ds:dx = destination buffer
	mov cx, tileset_graphics_size	; cx = number of bytes to read
	mov ax, 0x3f00	; ah=0x3f: read from file or device
	int 0x21
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21
	jmp .lantern_check
	nop

.terminate_program_trampoline:
	jmp terminate_program

.lantern_check:
	; If the current level is the castle, and the player does not have the
	; Lantern, black out the tile graphics.
	cmp byte [current_level_number], LEVEL_NUMBER_CASTLE
	jne .load_pt_files	; not in castle
	cmp byte [comic_has_lantern], 1
	je .load_pt_files	; in castle but have lantern
	; In castle, without lantern.
	push ds
	pop es		; es = ds
	xor ax, ax	; value to overwrite with (zero)
	lea di, [tileset_graphics]	; es:di = tileset_graphics
	mov cx, tileset_graphics_size
	rep stosb	; overwrite all tileset graphics (starting at es:di) with zeroes

.load_pt_files:
	; Load the three .PT files for this level.
	lea dx, [current_level + level.pt0_filename]
	lea cx, [pt0]
	call load_pt_file
	lea dx, [current_level + level.pt1_filename]
	lea cx, [pt1]
	call load_pt_file
	lea dx, [current_level + level.pt2_filename]
	lea cx, [pt2]
	call load_pt_file

	jmp load_shp_files
	nop		; dead code

; Load a .PT file into a pt struct.
; Input:
;   ds:dx = filename
;   ds:cx = destination pt struct
load_pt_file:
	push cx
	mov ax, 0x3d00	; ah=0x3d: open existing file
	int 0x21
	jnc .open_ok	; failure to open a .PT file is a fatal error
	pop cx
	jmp load_new_level.terminate_program_trampoline
.open_ok:
	mov bx, ax	; bx = file handle
	pop dx		; ds:dx = destination buffer
	mov cx, pt_size	; cx = number of bytes to read
	mov ax, 0x3f00	; ah=0x3f: read from file or device
	int 0x21
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21
	ret

; Load the four .SHP files named in the current_level struct
; Input:
;   current_level = level struct with initialized .shp member
; Output:
;   enemy_graphics = array[4][10] of 16×16 masked graphics
;   enemy_animation_frames = array[4][10] of pointers into enemy_graphics (possibly with duplicates)
;   enemy_num_animation_frames = array[4] of animation frame counts
load_shp_files:
	; Load .SHP file 0.
	lea si, [current_level + level.shp + 0*shp_size]
	cmp byte [si + shp.num_distinct_frames], 0	; current_level.shp[0].num_distinct_frames == 0?
	je .done
	lea di, [enemy0_graphics]
	lea dx, [enemy0_animation_frames]
	mov al, [si + shp.num_distinct_frames]	; number of animation frames actually stored in the file
	add al, [si + shp.animation]		; if animation == 1, there's one more frame, which is a duplicate of another one
	mov [enemy_num_animation_frames + 0], al	; enemy_num_animation_frames[0] = current_level.shp[0].num_distinct_frames + current_level.shp[0].animation
	call load_shp_file

	; Load .SHP file 1.
	lea si, [current_level + level.shp + 1*shp_size]
	cmp byte [si + shp.num_distinct_frames], 0	; current_level.shp[1].num_distinct_frames == 0?
	je .done
	lea di, [enemy1_graphics]
	lea dx, [enemy1_animation_frames]
	mov al, [si + shp.num_distinct_frames]	; number of animation frames actually stored in the file
	add al, [si + shp.animation]		; if animation == 1, there's one more frame, which is a duplicate of another one
	mov [enemy_num_animation_frames + 1], al	; enemy_num_animation_frames[1] = current_level.shp[1].num_distinct_frames + current_level.shp[1].animation
	call load_shp_file

	; Load .SHP file 2.
	lea si, [current_level + level.shp + 2*shp_size]
	cmp byte [si + shp.num_distinct_frames], 0	; current_level.shp[2].num_distinct_frames == 0?
	je .done
	lea di, [enemy2_graphics]
	lea dx, [enemy2_animation_frames]
	mov al, [si + shp.num_distinct_frames]	; number of animation frames actually stored in the file
	add al, [si + shp.animation]		; if animation == 1, there's one more frame, which is a duplicate of another one
	mov [enemy_num_animation_frames + 2], al	; enemy_num_animation_frames[2] = current_level.shp[2].num_distinct_frames + current_level.shp[2].animation
	call load_shp_file

	; Load .SHP file 3.
	lea si, [current_level + level.shp + 3*shp_size]
	cmp byte [si + shp.num_distinct_frames], 0	; current_level.shp[3].num_distinct_frames == 0?
	je .done
	lea di, [enemy3_graphics]
	lea dx, [enemy3_animation_frames]
	mov al, [si + shp.num_distinct_frames]	; number of animation frames actually stored in the file
	add al, [si + shp.animation]		; if animation == 1, there's one more frame, which is a duplicate of another one
	mov [enemy_num_animation_frames + 3], al	; enemy_num_animation_frames[3] = current_level.shp[3].num_distinct_frames + current_level.shp[3].animation
	call load_shp_file

.done:
	jmp load_new_stage

; Load a .SHP file. Store the graphics data and an animation cycle pointing
; into graphics, possibly by duplicates depending on whether the enemy has
; distinct left/right facing graphics, and its animation mode.
; Input:
;   ds:si = address of initialized shp struct
;   es:di = array[10] of 16×16 masked graphics
;   ds:dx = animation cycle, array[10] of pointers into es:di array
load_shp_file:
	push dx		; animation cycle array

	mov al, [si + shp.num_distinct_frames]
	xor ah, ah	; num_frames = num_distinct_frames
	cmp byte [si + shp.horizontal], ENEMY_HORIZONTAL_DUPLICATED
	je .l1
	; If horizontal != ENEMY_HORIZONTAL_DUPLICATED, assume
	; horizontal == ENEMY_HORIZONTAL_SEPARATE. Double the number of frames
	; to account for separate left-facing and right-facing graphics.
	shl ax, 1	; num_frames *= 2
.l1:
	; Each frame is 160 bytes long: 128 bytes for the four EGA planes, and
	; 32 bytes for the mask. Compute the total number of bytes to read.
	mov bx, ax
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; num_frames * 128 (the size of one 16×16 EGA graphic)
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1	; num_frames * 32 (the size of a mask)
	add ax, bx	; num_bytes = num_frames * 160

	push ax
	lea dx, [si + shp.filename]
	mov ax, 0x3d00	; ah=0x3d: open existing file
	int 0x21
	jnc .open_ok	; failure to open a .SHP file is a fatal error
	jmp load_new_level.terminate_program_trampoline
.open_ok:
	; Read the file data into the destination buffer.
	mov bx, ax	; bx = file handle
	mov dx, di	; ds:dx = destination buffer
	pop cx		; cx = number of bytes to read
	mov ax, 0x3f00	; ah=0x3f: read from file or device
	int 0x21
	mov ax, 0x3e00	; ah=0x3e: close file
	int 0x21

	pop bx		; bx = animation cycle array
	push bx

.left_facing:
	; Reference the left-facing frames.
	mov dx, di	; Point dx at the beginning of graphics data
	mov cl, [si + shp.num_distinct_frames]
.left_facing_loop:
	mov [bx], dx	; store the current graphic in the output array
	add bx, 2	; advance the output array index
	add dx, 160	; advance to the next graphic
	dec cl		; for each frame
	jne .left_facing_loop

	; Does this animation loop, or alternate?
	cmp byte [si + shp.animation], ENEMY_ANIMATION_LOOP
	je .right_facing
	; If animation != ENEMY_ANIMATION_LOOP, assume
	; animation == ENEMY_ANIMATION_ALTERNATE. Make frame n a copy of
	; frame n-2.
	sub dx, 2*160	; seek backward 2 frames
	mov [bx], dx	; store a duplicate reference for this frame
	add dx, 2*160	; return to where we were

.right_facing:
	; Read the right-facing frames. Depending on the value of horizontal,
	; these may be duplicates of the left-facing frames or may be separate
	; graphics.
	cmp byte [si + shp.horizontal], ENEMY_HORIZONTAL_SEPARATE
	je .l2
	; If horizontal != ENEMY_HORIZONTAL_SEPARATE, assume
	; horizontal == ENEMY_HORIZONTAL_DUPLICATED. Reset dx to the beginning
	; of the graphics data.
	mov dx, di
.l2:
	pop bx		; bx = animation cycle array
	push bx
	add bx, 2*ENEMY_FACING_RIGHT	; right-facing frames start at index 5 of the output array (index 4 is unused)

	; Reference the right-facing frames.
	mov cl, [si + shp.num_distinct_frames]
.right_facing_loop:
	mov [bx], dx	; store the current graphic in the output array
	add bx, 2	; advance the output array index
	add dx, 160	; advance to the next graphic
	dec cl		; for each frame
	jne .right_facing_loop

	; Does this animation loop, or alternate?
	cmp byte [si + shp.animation], ENEMY_ANIMATION_LOOP
	je .done
	sub dx, 2*160	; seek backward 2 frames
	mov [bx], dx	; store a duplicate reference for this frame

.done:
	pop bx
	ret

; Initialize global data structures as appropriate for the value of
; current_stage_number. Render the map into RENDERED_MAP_BUFFER. Despawn all
; fireballs. Initialize enemy data structures. Jump to game_loop when done.
; Input:
;   current_level = filled-out level struct
;   current_stage_number = stage number to load
;   source_door_level_number = level number we came from, if via a door
;     -2 = beam in at the start of game
;     -1 = not entering this stage via a door
;     other = level number containing the door we came through
;   source_door_stage_number = stage number we came from, if via a door
; Output:
;   current_tiles_ptr = address of the map tiles for this stage
;   current_stage_ptr = address of the stage struct for this stage
;   RENDERED_MAP_BUFFER = filled in with the current map
;   comic_y, comic_x = Comic's initial coordinates in the stage
;   comic_y_checkpoint, comic_x_checkpoint = Comic's initial position in the stage
;   camera_x = camera position appropriate for Comic's position
;   fireballs = despawned
;   enemies = initialized
load_new_stage:
	; Set current_tiles_ptr and current_stage_ptr according to
	; current_stage_number.
	mov al, [current_stage_number]
	lea dx, [pt0 + pt.tiles]
	lea cx, [current_level + level.stages + 0*stage_size]
	or al, al	; current_stage_number == 0?
	jz .set_current_stage_ptrs
	lea dx, [pt1 + pt.tiles]
	lea cx, [current_level + level.stages + 1*stage_size]
	dec al		; current_stage_number == 1?
	jz .set_current_stage_ptrs
	lea dx, [pt2 + pt.tiles]
	lea cx, [current_level + level.stages + 2*stage_size]
.set_current_stage_ptrs:
	mov [current_tiles_ptr], dx
	mov [current_stage_ptr], cx

	call render_map

	; Are we entering this stage via a door? If so, we need to find the
	; door in this stage that links back to the stage we came from. Comic
	; will be located in front of that door.
	cmp byte [source_door_level_number], 0
	jl .comic_located	; negative means we didn't arrive via a door

	; Search for a door in this stage that points back to the level and
	; stage we came from.
	mov si, [current_stage_ptr]
	lea si, [si + stage.doors]
	mov al, [source_door_level_number]
	mov ah, [source_door_stage_number]
	mov cx, MAX_NUM_DOORS	; for each door
.door_loop:
	cmp al, [si + door.target_level]
	jne .door_loop_next	; door's target level doesn't match, try the next door
	cmp ah, [si + door.target_stage]
	je .door_found		; got a match
.door_loop_next:
	add si, door_size	; next door
	loop .door_loop
	jmp terminate_program	; panic if no reciprocal door was found
.door_found:
	mov ax, [si + door.y]	; al = door.y, ah = door.x
	add ah, 1	; shift right one half-tile to center Comic in the door
	mov [comic_y_checkpoint], ax	; comic_y_checkpoint = door.y, comic_x_checkpoint = door.x + 1

.comic_located:
	; End any teleport that may be in progress on entering the stage. Comic
	; may have died mid-teleport and is now respawning.
	mov byte [comic_is_teleporting], 0

	; Despawn all fireballs.
	mov cx, MAX_NUM_FIREBALLS	; for each fireball slot
	lea bx, [fireballs]
	mov ax, (FIREBALL_DEAD<<8)|FIREBALL_DEAD
.fireball_loop:
	mov [bx], ax	; set fireball.x and fireball.y to FIREBALL_DEAD
	add bx, fireball_size	; next fireball
	loop .fireball_loop

	mov ax, [comic_y_checkpoint]	; al = comic_y_checkpoint, ah = comic_x_checkpoint
	mov [comic_y], ax		; comic_y = comic_y_checkpoint, comic_x = comic_x_checkpoint

	; Set camera_x = clamp(comic_x - 1 - PLAYFIELD_WIDTH/2, 0, MAP_WIDTH - PLAYFIELD_WIDTH)
	mov al, ah
	xor ah, ah	; ax = comic_x
	sub ax, (PLAYFIELD_WIDTH - 2)/2	; camera_x = comic_x - 1 - PLAYFIELD_WIDTH/2
	jae .camera_x_above_min
	xor ax, ax	; camera_x = 0
.camera_x_above_min:
	cmp ax, MAP_WIDTH - PLAYFIELD_WIDTH
	jb .camera_x_below_max
	mov ax, MAP_WIDTH - PLAYFIELD_WIDTH	; camera_x = MAP_WIDTH - PLAYFIELD_WIDTH
.camera_x_below_max:
	mov [camera_x], ax

	; Initialize enemies.
	mov si, [current_stage_ptr]
	lea si, [si + stage.enemies]
	lea di, [enemies]
	mov cx, MAX_NUM_ENEMIES	; for each enemy slot
.enemy_loop:
	; Skip initializing this enemy if its shp_index is 0xff. This code is
	; never actually used, as unused enemy slots are actually marked with
	; enemy_record.behavior == ENEMY_BEHAVIOR_UNUSED and have
	; enemy_record.shp_index == 0.
	mov al, [si + enemy_record.shp_index]
	cmp al, 0xff
	jne .valid_shp_index
	; It's not clear what this next instruction is supposed to do. An
	; enemy.restraint of 0xff would just overflow to
	; ENEMY_RESTRAINT_MOVE_THIS_TICK in the next tick.
	mov byte [di + enemy.restraint], 0xff
	jmp .enemy_loop_next

.valid_shp_index:
	push cx

	; num_animation_frames
	xor ah, ah
	mov cx, ax	; cx = enemy_record.shp_index
	lea bx, [enemy_num_animation_frames]
	add bx, ax
	mov bl, [bx]
	mov [di + enemy.num_animation_frames], bl	; enemy.num_animation_frames = enemy_num_animation_frames[enemy_record.shp_index]

	; animation_frames_ptr (points to a subarray of enemy_animation_frames)
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; enemy_record.shp_index * 16
	shl cx, 1
	shl cx, 1	; enemy_record.shp_index * 4
	add ax, cx	; enemy_record.shp_index * 20
	lea dx, [enemy_animation_frames]	; a 4-element array whose members are arrays of 10 words
	add dx, ax
	mov [di + enemy.animation_frames_ptr], dx	; enemy.animation_frames_ptr = enemy_animation_frames[enemy_record.shp_index]

	; behavior
	mov al, [si + enemy_record.behavior]
	mov [di + enemy.behavior], al

	mov byte [di + enemy.state], ENEMY_STATE_DESPAWNED
	mov byte [di + enemy.facing], ENEMY_FACING_LEFT
	mov byte [di + enemy.spawn_timer_and_animation], 20	; all enemies scheduled to spawn 20 ticks after entering a new stage

	; restraint
	mov byte [di + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK	; initially assume it's a slow enemy
	cmp al, ENEMY_BEHAVIOR_FAST	; is the ENEMY_BEHAVIOR_FAST bit set?
	jb .l1
	sub byte [di + enemy.behavior], ENEMY_BEHAVIOR_FAST	; unset the ENEMY_BEHAVIOR_FAST bit
	mov byte [di + enemy.restraint], ENEMY_RESTRAINT_MOVE_EVERY_TICK	; fast enemies move every tick
.l1:
	pop cx

.enemy_loop_next:
	add di, enemy_size		; next enemy slot
	add si, enemy_record_size	; next enemy_record
	loop .enemy_loop

	cmp byte [source_door_level_number], -2
	je .first_spawn	; beginning of the game, play beam_in animation
	cmp byte [source_door_level_number], -1
	je .start_game_loop	; didn't arrive via a door
	call exit_door	; arriving via a door, play door animation
	mov byte [source_door_level_number], -1	; unset the "came through a door" flag
.start_game_loop:
	jmp game_loop
.first_spawn:
	call beam_in
	mov byte [source_door_level_number], -1	; unset the "first spawn" flag
	jmp game_loop

; Return the address of the map tile at the given coordinates.
; Input:
;   ah = x-coordinate in game units
;   al = y-coordinate in game units
; Output:
;   bx = address of map tile
address_of_tile_at_coordinates:
	push ax
	mov bl, ah
	xor bh, bh	; bx = x
	xor ah, ah	; ax = y

	; Tiles measure 2 game units in both directions: divide coordinates by 2.
	shr bx, 1
	shr ax, 1

	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply by 128 (MAP_WIDTH_TILES)
	add ax, bx	; y/2*128 + x/2

	mov bx, [current_tiles_ptr]
	add bx, ax	; current_tiles_ptr[y/2*128 + x/2]
	pop ax
	ret

; Check for map collision of an enemy-sized box at the given coordinates,
; presumed to be moving vertically. If the x-coordinate is even, check only the
; map tile that contains the given coordinates; if the x-coordinate is odd,
; check that tile and also the one to its right.
; Input:
;   ah = x-coordinate in game units
;   al = y-coordinate in game units
; Output:
;   cf = 1 if collision, 0 if no collision
check_vertical_enemy_map_collision:
	mov cx, ax	; cl = y, ch = x
	mov bl, ah
	xor bh, bh	; bx = x
	xor ah, ah	; ax = y

	; Tiles measure 2 units in both directions; divide coordinates by 2.
	shr bx, 1
	shr ax, 1

	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply by 128 (MAP_WIDTH_TILES)
	add ax, bx	; y/2*128 + x/2

	mov bx, [current_tiles_ptr]
	add bx, ax	; current_tiles_ptr[y/2*128 + x/2]
	mov ax, cx	; restore original al = y, ah = x

	mov cl, [tileset_last_passable]
	cmp [bx], cl	; is the tile solid?
	jg .return_collision	; if so, return
	; Is the x-coordinate odd (in the middle of a tile)? I.e., does a
	; 2-unit-wide enemy also touch the next tile to the right?
	test ah, 1
	jz .return_no_collision	; we're on an even tile boundary, no need to test another tile
	cmp [bx + 1], cl	; is the next tile to the right solid?
	jg .return_collision
.return_no_collision:
	clc		; clear carry flag
	ret
.return_collision:
	stc		; set carry flag
	ret

; Check for map collision of an enemy-sized box at the given coordinates,
; presumed to be moving horizontally. If the y-coordinate is even, check only
; the map tile that contains the given coordinates; if the y-coordinate is odd,
; check that tile and also the next one down.
; Input:
;   ah = x-coordinate in game units
;   al = y-coordinate in game units
; Output:
;   cf = 1 if collision, 0 if no collision
check_horizontal_enemy_map_collision:
	mov cx, ax	; cl = y, ch = x
	mov bl, ah
	xor bh, bh	; bx = x
	xor ah, ah	; ax = y

	; Tiles measure 2 units in both directions; divide coordinates by 2.
	shr bx, 1
	shr ax, 1

	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply by 128 (MAP_WIDTH_TILES)
	add ax, bx	; y/2*128 + x/2

	mov bx, [current_tiles_ptr]
	add bx, ax	; current_tiles_ptr[y/2*128 + x/2]
	mov ax, cx	; restore original al = y, ah = x

	mov cl, [tileset_last_passable]
	cmp [bx], cl	; is the tile solid?
	jg .return_collision	; if so, return
	; Is the y-coordinate odd (in the middle of a tile)? I.e., does a
	; 2-unit-tail enemy also touch the next tile down?
	test al, 1
	jz .return_no_collision	; we're on an even tile boundary, no need to test another tile
	cmp [bx + MAP_WIDTH_TILES], cl	; is the next tile down solid?
	jg .return_collision
.return_no_collision:
	clc		; clear carry flag
	ret
.return_collision:
	stc		; set carry flag
	ret

; Blit the entire map into the 40 KB buffer of video memory starting at
; a000:4000.
; Input:
;   current_tiles_ptr = tiles of the map to render
render_map:
	mov ax, 0xa000
	mov es, ax	; es points to video memory

	mov cx, (MAP_HEIGHT_TILES<<8)|MAP_WIDTH_TILES	; ch = map height, cl = map width
	mov di, RENDERED_MAP_BUFFER	; es:di points to the destination graphics buffer
	mov si, [current_tiles_ptr]	; ds:si points to the map tiles
.loop:
	lodsb		; get the next tile ID into ax
	xor ah, ah
	push si
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply tile ID by 128, the size of one 16×16 tile graphic
	lea si, [tileset_graphics]
	add si, ax	; tileset_buffer[tile_id], pointer to graphic for this tile ID
	mov ah, 1
	call blit_tile_plane
	mov ah, 2
	call blit_tile_plane
	mov ah, 4
	call blit_tile_plane
	mov ah, 8
	call blit_tile_plane
	; blit_tile_plane spaces each row of pixels in this tile 256 bytes
	; apart. So the first tile is written to offsets 0, 1, 256, 257, 512,
	; 513, .... The next tile will be written offset by 2 bytes; i.e., to
	; 2, 3, 258, 259, 514, 515, ....
	add di, 2	; advance blit write pointer by 16 pixels
	pop si
	dec cl		; next column
	jne .loop
	; At the end of one row of tiles, our blit write pointer is set to 256,
	; which is just after the first row of pixels, but blit_tile_plane has
	; also written an additional 15 rows of pixels after that. Advance the
	; blit write pointer to the place for the next row of tiles.
	add di, 0x0f00
	mov cl, MAP_WIDTH_TILES	; reinitialize the number of map columns
	dec ch		; next row
	jne .loop
	ret

; Blit a single plane of a 16×16 map tile graphic to video memory. The length
; of each row of pixels in the video buffer is assumed to be 256 bytes (i.e., 2
; bytes per tile in a map that is 128 tiles wide).
; Input:
;   ds:si = address of graphic
;   es:di = destination address (points into video memory)
;   ah = plane mask (1, 2, 4, 8)
blit_tile_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	mov bx, 16	; number of rows
	push di
.loop:
	movsw		; copy 2 bytes from ds:si to es:di
	add di, MAP_WIDTH_TILES*2-2	; next destination row (each map row is 256 bytes; compensate for the 2 bytes we just wrote)
	dec bx		; next row
	jne .loop
	pop di
	ret

; Blit the currently visible portion of the map to the offscreen video buffer.
; Input:
;   camera_x = x-coordinate of left edge of playfield
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
blit_map_playfield_offscreen:
	push ds
	mov si, RENDERED_MAP_BUFFER	; copy from this buffer
	add si, [camera_x]	; scroll to the visible part of the map
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(8, 8)	; pixel coordinates of top left of playfield

	mov ax, 0xa000
	mov es, ax	; es points to video memory
	mov ds, ax	; ds points to video memory

	mov ah, 1	; plane mask
	xor bx, bx	; plane index
	call blit_map_playfield_offscreen_plane
	mov ah, 2	; plane mask
	inc bx		; plane index
	call blit_map_playfield_offscreen_plane
	mov ah, 4	; plane mask
	inc bx		; plane index
	call blit_map_playfield_offscreen_plane
	mov ah, 8	; plane mask
	inc bx		; plane index
	call blit_map_playfield_offscreen_plane
	pop ds
	ret

; Blit Comic at his current position, relative to the camera, to the offscreen
; video buffer.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
;   comic_animation = COMIC_STANDING, COMIC_RUNNING_1, COMIC_RUNNING_2, COMIC_RUNNING_3, or COMIC_JUMPING
;   camera_x = x-coordinate of left edge of playfield
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
blit_comic_playfield_offscreen:
	lea si, [GRAPHICS_COMIC]
	mov al, [comic_facing]
	mov ah, [comic_animation]
	; Index GRAPHICS_COMIC by comic_facing and comic_animation.
	; (comic_facing is 0 or 5 and comic_animation is 0..4.)
	add al, ah
	xor ah, ah	; index = comic_facing + comic_animation
	mov dx, ax
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; index * 128
	shl dx, 1
	shl dx, 1
	shl dx, 1
	shl dx, 1
	shl dx, 1	; index * 32
	add ax, dx	; index * 160
	shl ax, 1	; index * 320 (the size of a 16×32 graphic plus mask)
	add si, ax	; GRAPHICS_COMIC[comic_facing + comic_animation]
	mov cx, 32	; number of rows
	mov bp, si
	add bp, 256	; address of mask
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	mov dx, [camera_x]
	sub ah, dl	; camera-relative x-coordinate
	call blit_16xH_with_mask_playfield_offscreen
	ret

; Blit a masked graphic that is 16 pixels wide by cx pixels tall, at the given
; camera-relative coordinates, to the offscreen video buffer.
; Input:
;   ah = camera-relative x-coordinate
;   al = camera-relative y-coordinate
;   cx = number of rows in graphic
;   ds:si = address of graphic
;   ds:bp = address of mask
;   camera_x = x-coordinate of left edge of playfield
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
blit_16xH_with_mask_playfield_offscreen:
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(8, 8)	; top left of playfield

	; Compute the address in video memory at which to blit the graphic. The
	; original coordinates are in game units, so we must compute a quantity
	; that is 8 times bigger than it would be if the original coordinates
	; had been in pixels.
	mov dl, ah
	xor dh, dh	; dx = x
	xor ah, ah	; ax = y
	mov bh, al
	xor bl, bl	; bx = y * 256
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; y * 64
	add ax, bx	; y * 64 + y * 256 = y * 320
	add ax, dx	; y * 320 + x
	add di, ax

	mov ah, 1	; plane mask
	xor bx, bx	; plane index
	call blit_16xH_plane_with_mask
	mov ah, 2	; plane mask
	mov bx, 1	; plane index
	call blit_16xH_plane_with_mask
	mov ah, 4	; plane mask
	mov bx, 2	; plane index
	call blit_16xH_plane_with_mask
	mov ah, 8	; plane mask
	mov bx, 3	; plane index
	call blit_16xH_plane_with_mask
	ret

; Blit a single plane of a playfield-sized region of a map buffer to video
; memory.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   bl = plane index (0, 1, 2, 3)
;   ds:si = source address (points into rendered map in memory)
;   es:di = destination address (points into video memory)
; Output:
;   ds:si = unchanged
;   es:di = unchanged
blit_map_playfield_offscreen_plane:
	push si
	push di

	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask
	mov dx, 0x3ce	; GC Index register
	mov al, 4	; GC Read Map Select register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_gfx.cpp#l90
	out dx, al
	inc dx		; GC Data register
	mov al, bl
	out dx, al	; write plane index

	mov dx, PLAYFIELD_HEIGHT*8	; 8 rows of pixels per unit of height
.loop:
	mov cx, PLAYFIELD_WIDTH*8/8/2	; 8 columns of pixels per map tile at 1 bit per pixel and 2 bytes per word
	rep movsw	; copy from ds:si to es:di
	add si, (MAP_WIDTH - PLAYFIELD_WIDTH)*8/8	; next map row (counting the pixels we just read)
	add di, SCREEN_WIDTH/8 - PLAYFIELD_WIDTH*8/8	; next screen row (skip ahead SCREEN_WIDTH pixels, counting the ones we just wrote)
	dec dx
	jne .loop

	pop di
	pop si
	ret

; Enable an EGA plane for write.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   bl = plane index (0, 1, 2, 3)
enable_ega_plane_write:
	; https://www.jagregory.com/abrash-black-book/#at-the-core
	; https://www.jagregory.com/abrash-black-book/#color-plane-manipulation
	; https://wiki.osdev.org/VGA_Hardware#Read.2FWrite_logic
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask
	mov dx, 0x3ce	; GC Index register
	mov al, 4	; GC Read Map Select register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_gfx.cpp#l90
	out dx, al
	inc dx		; GC Data register
	mov al, bl
	out dx, al	; write plane index
	ret

; Blit a single plane of a masked graphic that is 16 pixels wide by cx pixels
; tall. Takes as destination not pixel coordinates or game coordinates, but a
; precomputed address in video memory.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   bl = plane index (0, 1, 2, 3)
;   cx = number of rows in graphic
;   ds:si = address of graphic
;   ds:bp = address of mask
;   es:di = destination address (points into video memory)
blit_16xH_plane_with_mask:
	push di

	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask
	mov dx, 0x3ce	; GC Index register
	mov al, 4	; GC Read Map Select register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_gfx.cpp#l90
	out dx, al
	inc dx		; GC Data register
	mov al, bl
	out dx, al	; write plane index

	mov bx, bp	; bx = address of mask
	mov dx, cx	; save cx
.loop:
	mov ax, [bx]
	and [es:di], ax	; blank video memory inside the mask
	lodsw		; load [ds:si] into ax
	or [es:di], ax	; write the graphics data (assumed to be 0 outside the mask)
	add di, SCREEN_WIDTH/8	; next screen row
	add bx, 2	; next graphic row
	loop .loop
	mov cx, dx	; restore cx

	pop di
	ret

; Change the video start offset.
; Input:
;   cx = new video start offset
switch_video_buffer:
	; https://www.jagregory.com/abrash-black-book/#at-the-core
	mov dx, 0x3d4	; CRTC Index register
	mov al, 0x0c	; CRTC Start Address High register
	out dx, al
	inc dx		; CRTC Data register
	mov al, ch
	out dx, al	; write high byte

	mov dx, 0x3d4	; CRTC Index register
	mov al, 0x0d	; CRTC Start Address Low register
	out dx, al
	inc dx		; CRTC Data register
	mov al, cl
	out dx, al	; write low byte

	ret

; This function is like blit_comic_playfield_offscreen except that it only draws the
; upper cx rows of Comic's graphic, not all 32. Use this when Comic partially
; overlaps the bottom of the screen.
; Input:
;   cx = the number of rows to draw
;   bx = 32 - cx
;   camera_x = x-coordinate of left edge of playfield
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
;   comic_animation = COMIC_STANDING, COMIC_RUNNING_1, COMIC_RUNNING_2, COMIC_RUNNING_3, or COMIC_JUMPING
blit_comic_partial_playfield_offscreen:
	push bx		; number of rows not to draw
	push cx		; number of rows to draw

	lea si, [GRAPHICS_COMIC]
	; Index GRAPHICS_COMIC by comic_facing and comic_animation.
	; (comic_facing is 0 or 5 and comic_animation is 0..4.)
	mov al, [comic_facing]
	mov ah, [comic_animation]
	add al, ah
	xor ah, ah	; index = comic_facing + comic_animation
	mov dx, ax
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; index * 128
	shl dx, 1
	shl dx, 1
	shl dx, 1
	shl dx, 1
	shl dx, 1	; index * 32
	add ax, dx	; index * 160
	shl ax, 1	; index * 320 (size of a 16×32 graphic plus mask)
	add si, ax	; GRAPHICS_COMIC[comic_facing + comic_animation]
	mov bp, si
	add bp, 256	; address of mask
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	mov dx, [camera_x]
	sub ah, dl	; camera-relative x-coordinate

	pop cx		; number of rows to draw
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(8, 8)	; top left of playfield

	; Compute the address in video memory at which to blit the graphic. The
	; original coordinates are in game units, so we must compute a quantity
	; that is 8 times bigger than it would be if the original coordinates
	; had been in pixels.
	mov dl, ah	; dl = camera-relative x-coordinate
	xor dh, dh	; dx = x
	xor ah, ah	; ax = y
	mov bh, al
	xor bl, bl	; bx = y * 256
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; y * 64
	add ax, bx	; y * 64 + y * 256 = y * 320
	add ax, dx	; y * 320 + x
	add di, ax

	mov ah, 1	; plane mask
	xor bx, bx	; plane index
	call blit_16xH_plane_with_mask

	; Advance the graphic pointer by the number of rows we did not draw, at
	; 2 bytes per row.
	pop bx		; number of rows not to draw
	add si, bx
	add si, bx	; si += bx * 2
	push bx		; number of rows not to draw
	mov ah, 2	; plane mask
	mov bx, 1	; plane index
	call blit_16xH_plane_with_mask

	pop bx		; number of rows not to draw
	add si, bx
	add si, bx	; si += bx * 2
	push bx		; number of rows not to draw
	mov ah, 4	; plane mask
	mov bx, 2	; plane index
	call blit_16xH_plane_with_mask

	pop bx		; number of rows not to draw
	add si, bx
	add si, bx	; si += bx * 2
	mov ah, 8	; plane mask
	mov bx, 3	; plane index
	call blit_16xH_plane_with_mask

	ret

; Play the animation for Comic exiting a door.
; Input:
;   comic_x, comic_y = door is placed at (comic_x - 1, comic_y)
;   camera_x = x-coordinate of left edge of playfield
exit_door:
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	mov bx, [camera_x]
	sub ah, bl	; camera-relative x-coordinate
	sub ah, 1	; door.x = comic_x - 1
	mov dl, ah
	xor dh, dh	; dx = door.x
	xor ah, ah	; ax = door.y
	mov bh, al
	xor bl, bl	; bx = door.y * 256
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; door.y * 64
	add ax, bx	; door.y * 320
	add ax, dx	; door.y * 320 + door.x
	mov di, pixel_coords(8, 8)	; pointer to top left of playfield in video memory
	add di, ax	; address of top left of door
	mov [door_blit_offset], di	; pass it to subroutines in a variable

	mov ax, 3
	call wait_n_ticks

	lea bx, [SOUND_DOOR]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	; Door fully closed.
	call blit_map_playfield_offscreen
	call wait_1_tick_and_swap_video_buffers

	; Door halfway open.
	call blit_map_playfield_offscreen
	call blank_door_offscreen
	call blit_comic_playfield_offscreen	; Comic is behind door
	call blit_door_halfopen_offscreen
	call wait_1_tick_and_swap_video_buffers

	; Door fully open.
	call blit_map_playfield_offscreen
	call blank_door_offscreen
	call blit_comic_playfield_offscreen
	call wait_1_tick_and_swap_video_buffers

	; Door halfway closed.
	call blit_map_playfield_offscreen
	call blit_door_halfopen_offscreen	; useless instruction; the next one blanks what this one blits
	call blank_door_offscreen
	call blit_door_halfopen_offscreen
	call blit_comic_playfield_offscreen	; Comic is in front of door
	call wait_1_tick_and_swap_video_buffers

	; Door fully closed.
	call blit_map_playfield_offscreen
	call blit_comic_playfield_offscreen
	call wait_1_tick_and_swap_video_buffers

	ret

; Play the animation for Comic entering a door.
; Input:
;   ah = x-coordinate of door
;   al = y-coordinate of door
;   camera_x = x-coordinate of left edge of playfield
enter_door:
	mov bx, [camera_x]
	sub ah, bl	; camera-relative x-coordinate
	mov dl, ah
	xor dh, dh	; dx = door.x
	xor ah, ah	; ax = door.y
	mov bh, al
	xor bl, bl	; bx = door.y * 256
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; door.y * 64
	add ax, bx	; door.y * 320
	add ax, dx	; door.y * 320 + door.x
	mov di, pixel_coords(8, 8)	; pointer to top left of playfield in video memory
	add di, ax	; address of top left of door
	mov [door_blit_offset], di	; pass it to subroutines in a variable

	lea bx, [SOUND_DOOR]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	; Door halfway open.
	call blit_map_playfield_offscreen
	call blank_door_offscreen
	call blit_door_halfopen_offscreen
	call blit_comic_playfield_offscreen	; Comic is in front of door
	call wait_1_tick_and_swap_video_buffers

	; Door fully open.
	call blit_map_playfield_offscreen
	call blank_door_offscreen
	call blit_comic_playfield_offscreen
	call wait_1_tick_and_swap_video_buffers

	; Door halfway closed.
	call blit_map_playfield_offscreen
	call blit_door_halfopen_offscreen	; useless instruction; the next one blanks what this one blits
	call blank_door_offscreen
	call blit_comic_playfield_offscreen	; Comic is behind door
	call blit_door_halfopen_offscreen
	call wait_1_tick_and_swap_video_buffers

	; Door fully closed.
	call blit_map_playfield_offscreen
	call wait_1_tick_and_swap_video_buffers

	ret

; Draw a half-open door to the offscreen video buffer. The half-open effect is
; achieved by only drawing the left or right half of each door tile, and
; filling in the rest with black.
; Input:
;   door_blit_offset = offset within the offscreen video segment at which to blit
;   current_level.door_tile_ul = upper-left door tile
;   current_level.door_tile_ur = upper-right door tile
;   current_level.door_tile_ll = lower-left door tile
;   current_level.door_tile_lr = lower-right door tile
blit_door_halfopen_offscreen:
	; Upper-left door tile.
	lea bx, [current_level + level.door_tile_ul]
	mov al, [bx + 0]	; level.door_tile_ul
	call graphic_for_tile_id	; si = graphic
	inc si		; look at the right half of the tile graphic
	mov di, [door_blit_offset]
	push bx
	call blit_half_16x16_offscreen	; blit only the right half of the tile
	pop bx

	; Upper-right door tile.
	mov al, [bx + 1]	; level.door_tile_ur
	call graphic_for_tile_id	; si = graphic
	mov di, [door_blit_offset]
	add di, 3	; move 24 pixels to the right
	push bx
	call blit_half_16x16_offscreen	; blit only the left half of the tile
	pop bx

	; Lower-left door tile.
	mov al, [bx + 2]	; level.door_tile_ll
	call graphic_for_tile_id	; si = graphic
	inc si		; look at the right half of the tile graphic
	mov di, [door_blit_offset]
	add di, 16*40	; move 16 pixels down
	push bx
	call blit_half_16x16_offscreen	; blit only the right half of the tile
	pop bx

	; Lower-right door tile.
	mov al, [bx + 3]	; level.door_tile_lr
	call graphic_for_tile_id	; si = graphic
	mov di, [door_blit_offset]
	add di, 16*40 + 3	; move 16 pixels down and 24 pixels to the right
	call blit_half_16x16_offscreen	; blit only the left half of the tile

	ret

; Return a pointer to the graphic for a given tile ID in the current tileset.
; Input:
;   ax = tile ID
; Output:
;   si = pointer to graphic
graphic_for_tile_id:
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; tile_id * 128
	lea si, [tileset_graphics]
	add si, ax	; tileset_graphics[tile_id]
	ret

; Blit the left 8×16 half of a 16×16 graphic to the offscreen video buffer.
; This is used for half-open door tiles.
; Input:
;   ds:si = graphic (2 bytes per row, 32 bytes per plane, 128 bytes total)
;   es:di = destination address (points into video memory)
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_half_16x16_offscreen:
	add di, [offscreen_video_buffer_ptr]
	mov ah, 1
	call blit_half_16x16_offscreen_plane
	mov ah, 2
	call blit_half_16x16_offscreen_plane
	mov ah, 4
	call blit_half_16x16_offscreen_plane
	mov ah, 8
	call blit_half_16x16_offscreen_plane
	ret

; Blit a single plane of the left 8×16 half of a 16×16 graphic to the offscreen
; video buffer.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   ds:si = graphic (2 bytes per row, 32 bytes total)
;   es:di = destination address (points into video memory)
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_half_16x16_offscreen_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	mov bx, 16	; number of rows
	push di
.loop:
	movsb		; copy 1 byte from ds:si to es:di
	inc si		; skip over the other half of the graphic
	add di, SCREEN_WIDTH/8 - 1	; next screen row (considering the 1 byte we just wrote)
	dec bx		; next graphic row
	jne .loop
	pop di
	ret

; Black out a 32×32 pixel region starting at door_blit_offset in the offscreen
; video buffer.
; Input:
;   door_blit_offset = offset within the offscreen video segment at which to blit
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
blank_door_offscreen:
	mov ah, 1
	call blank_door_offscreen_plane
	mov ah, 2
	call blank_door_offscreen_plane
	mov ah, 4
	call blank_door_offscreen_plane
	mov ah, 8
	call blank_door_offscreen_plane
	ret

; Black out a single plane of a 32×32 pixel region starting at door_blit_offset
; in the offscreen video buffer.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   door_blit_offset = offset within the offscreen video segment at which to blit
;   offscreen_video_buffer_ptr = whichever of video buffers 0x0000 or 0x2000 is not on screen at the moment
blank_door_offscreen_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	mov di, [door_blit_offset]
	add di, [offscreen_video_buffer_ptr]
	mov cx, 32	; doors are 32 rows talls
	xor ax, ax
.loop:
	stosw		; set word [es:di] = 0 (left half of door)
	stosw		; set word [es:di] = 0 (right half of door)
	add di, SCREEN_WIDTH/8 - 4	; next screen row (considering the 4 bytes we just wrote)
	loop .loop

	ret

; Wait until the beginning of the next tick and call swap_video_buffers.
wait_1_tick_and_swap_video_buffers:
	mov ax, 1
	call wait_n_ticks
	call swap_video_buffers
	ret

; Play the animation of Comic dying from taking damage.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   camera_x = x-coordinate of left edge of playfield
comic_death_animation:
	lea bx, [SOUND_DEATH]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	mov cx, 8	; 8 frames of animation
	lea si, [ANIMATION_COMIC_DEATH]
.loop:
	push cx		; loop counter
	push si		; current animation frame

	push cx
	call blit_map_playfield_offscreen
	pop cx

	cmp cx, 4	; draw Comic under the death animation for the first 4 frames
	jle .l1
	call blit_comic_playfield_offscreen
.l1:
	call handle_enemies	; enemies continue to move while Comic is dying

	pop si		; current animation frame
	push si
	mov bp, si
	add bp, 256	; address of mask
	mov dx, [camera_x]
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	sub ah, dl	; camera-relative x-coordinate
	mov cx, 32	; number of rows in graphic
	call blit_16xH_with_mask_playfield_offscreen

	call handle_item	; items continue to animate while Comic is dying

	call swap_video_buffers

	mov ax, 1
	call wait_n_ticks

	pop si		; current animation frame
	add si, 320	; next animation frame

	pop cx		; loop counter
	loop .loop

	ret

; Find a teleport destination and enter the teleport state.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
;   camera_x = x-coordinate of left edge of playfield
; Output:
;   teleport_camera_vel = how far to move the camera per tick in upcoming ticks
;   teleport_camera_counter = how many upcoming ticks during which the camera will move
;   teleport_source_x, teleport_source_y = comic_x, comic_y
;   teleport_destination_x, teleport_destination_y = coordinates of teleport destination
;   teleport_animation = 0
;   comic_is_teleporting = 1
begin_teleport:
	; ax tracks the teleport destination, which is finally assigned to
	; (teleport_destination_y, teleport_destination_x) at .finish.
	; al = teleport_destination_y, ah = teleport_destination_x.
	; Initially set it to Comic's position.
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	mov bx, [camera_x]
	mov dx, bx
	mov byte [teleport_camera_counter], 0
	; I'm not sure why this does `neg`, `add` instead of just `sub`.
	neg bl		; -camera_x
	add bl, ah	; comic_x - camera_x
	; bl = camera-relative x-coordinate of Comic

	cmp byte [comic_facing], COMIC_FACING_RIGHT
	je .facing_right

.facing_left:
	mov word [teleport_camera_vel], -1	; move the camera left 1 unit per tick during the teleport

	sub bl, TELEPORT_DISTANCE	; is Comic close to the left edge of the playfield?
	jb .remain_in_place	; if teleport would take Comic off the left edge of the playfield, remain in place

	sub ah, TELEPORT_DISTANCE	; teleport destination column is 6 units to the left
	; Figure out how to set teleport_camera_counter, which is the number of
	; ticks during which the camera should move during the teleport. It may
	; be less than the animation duration of 6 ticks.
	sub bl, PLAYFIELD_WIDTH/2 - 2	; is the destination on the right side of the screen?
	jae .find_a_safe_landing	; if so, then don't move the camera at all
	; The destination of the teleport is on the left side of the screen.
	; We'll need to move the camera during the teleport in order to put
	; Comic in the center of the playfield when the teleport is over. (The
	; logic here doesn't take into the account that ah will be rounded down
	; later, which is a bug.)
	neg bl		; bl = PLAYFIELD_WIDTH/2 - 2 - ah; how far the destination is to the left of center
	sub dl, bl	; would moving that far take the camera off the left edge of the stage?
	jae .set_teleport_camera_counter
	add bl, dl	; if so, move the camera only far enough to take the camera all the way to the left edge
	jmp .set_teleport_camera_counter

.facing_right:
	mov word [teleport_camera_vel], +1	; move the camera right 1 unit per tick during the teleport

	cmp bl, PLAYFIELD_WIDTH - TELEPORT_DISTANCE - 1	; is Comic close to the right edge of the playfield?
	jae .remain_in_place	; if teleport would take Comic off the right edge of the playfield, remain in place

	add ah, TELEPORT_DISTANCE	; teleport destination column is 6 units to the right
	; Figure out how to set teleport_camera_counter, which is the number of
	; ticks during which the camera should move during the teleport. It may
	; be less than the animation duration of 6 ticks.
	add bl, TELEPORT_DISTANCE
	sub bl, PLAYFIELD_WIDTH/2	; is the destination on the left side of the screen?
	jle .find_a_safe_landing	; if so, then don't move the camera at all
	; The destination of the teleport is on the right side of the screen.
	; We'll need to move the camera during the teleport in order to put
	; Comic in the center of the playfield when the teleport is over. (The
	; logic here doesn't take into the account that ah will be rounded down
	; later, which is a bug.)
	add dl, bl	; dl = comic_x + TELEPORT_DISTANCE - PLAYFIELD_WIDTH/2
	cmp dl, MAP_WIDTH - PLAYFIELD_WIDTH	; would moving that far take the camera off the right edge of the stage?
	jle .set_teleport_camera_counter
	sub dl, MAP_WIDTH - PLAYFIELD_WIDTH
	sub bl, dl	; if so, only move enough to take the camera all the way to the right edge
	; Fall through to .set_teleport_camera_counter.

.set_teleport_camera_counter:
	mov byte [teleport_camera_counter], bl

.find_a_safe_landing:
	mov cl, [tileset_last_passable]
	; Now ah contains the column column we're trying to teleport to. Search
	; that column for a landing zone, starting from the bottom. A landing
	; zone is a solid tile with two non-solid tiles above it.
	mov al, PLAYFIELD_HEIGHT - 2	; start looking at the bottom row of tiles
	and ah, 0xfe	; round destination x-coordinate down to an even tile boundary
	call address_of_tile_at_coordinates	; bx = address of the tile ID we're currently looking at

	; First look for a solid tile.
.find_solid_loop:
	cmp [bx], cl	; is the tile we're looking at solid?
	jg .find_nonsolid_loop_start	; found a solid tile, now look for an empty space above it
.find_solid_loop_next:
	sub bx, MAP_WIDTH_TILES	; next tile up in the map
	sub al, 2	; next tile up in y-coordinate
	jne .find_solid_loop	; keep looking for a solid tile until we reach the top of the map
	jmp .remain_in_place	; hit the top of the map without finding a solid tile
	nop		; dead code

	; Now look for two consecutive non-solid tiles.
.find_nonsolid_loop:
	cmp [bx], cl	; is the tile solid?
	jle .found_a_nonsolid	; not solid
.find_nonsolid_loop_start:
	; Every time we hit a solid tile, we have to start the count over.
	mov dx, 2	; dx counts the number of consecutive non-solid tiles we still have to find
.find_nonsolid_loop_next:
	sub bx, MAP_WIDTH_TILES	; next tile up in the map
	sub al, 2	; next tile up in y-coordinate
	jae .find_nonsolid_loop	; keep looking for non-solid tiles until we reach the top of the map
	jmp .remain_in_place	; got to the top of the map without finding 2 non-solid tiles
	nop		; dead code
.found_a_nonsolid:
	dec dx		; decrement the count of consecutive non-solid tiles
	jne .find_nonsolid_loop_next	; still have another one to find
	jmp .finish	; that's enough, found a safe landing zone

.remain_in_place:
	; Could not find a teleport destination. Teleport to Comic's current
	; position.
	mov ax, [comic_y]

.finish:
	mov byte [teleport_animation], 0	; teleport animation begins
	mov [teleport_destination_y], ax	; al and ah are the destination coordinates found by the logic above
	mov ax, [comic_y]
	mov [teleport_source_y], ax	; set the source coordinates to Comic's current coordinates
	mov byte [comic_is_teleporting], 1

	lea bx, [SOUND_TELEPORT]
	mov ax, SOUND_PLAY
	mov cx, 2	; priority 2
	int3

	ret

; Do one tick of the teleport process. Render the map within the playfield and
; blit Comic and the current frames of the teleport animation.
; Input:
;   teleport_camera_counter = how many more ticks the camera should move teleport_camera_vel per tick
;   teleport_camera_vel = how far to move the camera per tick while teleport_camera_counter is nonzero
;   teleport_source_x, teleport_source_y = coordinates of source of teleport
;   teleport_destination_x, teleport_destination_y = coordinates of destination of teleport
; Output:
;   camera_x = increased/decreased by teleport_camera_vel, if teleport_camera_counter not already 0
;   comic_x, comic_y = set to (teleport_destination_x, teleport_destination_y) starting at teleport_animation == 3
;   teleport_camera_counter = decremented by 1 if not already 0
;   teleport_animation = incremented by 1
;   comic_is_teleporting = 1 if the teleport is still in progress, 0 if finished
handle_teleport:
	cmp byte [teleport_camera_counter], 0	; are we still scheduled to move the camera during this teleport?
	je .camera_moved

	; Move the camera to keep up with the motion of the teleport.
	mov bx, [teleport_camera_vel]
	add [camera_x], bx
	dec byte [teleport_camera_counter]

.camera_moved:
	; Render the map.
	call blit_map_playfield_offscreen

	; Comic's own position actually changes (for the purposes of collision
	; with enemies, etc.) at frame 3 of the 6-frame teleport animation.
	cmp byte [teleport_animation], 3
	jne .blit_at_source
	mov ax, [teleport_destination_y]
	mov [comic_y], ax	; comic_y = teleport_destination_y, comic_x = teleport_destination_x

	; We draw the teleport animation at both the source and destination;
	; the animation at the destination is delayed by 1 frame. There are 5
	; frames of animation defined in ANIMATION_TABLE_TELEPORT. We draw at
	; the source during frames 0..4 and draw at the destination during
	; frames 1..5.
.blit_at_source:
	call blit_comic_playfield_offscreen	; blit Comic, whether currently at the teleport source or destination
	mov al, [teleport_animation]
	cmp al, 5	; done animating at the source?
	je .blit_at_destination

	xor ah, ah
	shl ax, 1
	lea si, [ANIMATION_TABLE_TELEPORT]
	add si, ax
	mov si, [si]	; ANIMATION_TABLE_TELEPORT[teleport_animation]
	mov bp, si
	add bp, 256	; address of mask
	mov ax, [teleport_source_y]	; al = teleport_source_y, ah = teleport_source_x
	sub ah, [camera_x]	; camera-relative x-coordinate
	mov cx, 32	; number of rows in graphic
	call blit_16xH_with_mask_playfield_offscreen

.blit_at_destination:
	mov al, [teleport_animation]
	sub al, 1	; not yet ready to start animating at the destination?
	jb .next_animation_frame

	xor ah, ah
	shl ax, 1
	lea si, [ANIMATION_TABLE_TELEPORT]
	add si, ax
	mov si, [si]	; ANIMATION_TABLE_TELEPORT[teleport_animation]
	mov bp, si
	add bp, 256	; address of mask
	mov ax, [teleport_destination_y]	; al = teleport_destination_y, ah = teleport_destination_x
	sub ah, [camera_x]	; camera-relative x-coordinate
	mov cx, 32	; number of rows in graphic
	call blit_16xH_with_mask_playfield_offscreen

.next_animation_frame:
	inc byte [teleport_animation]	; next frame of animation
	cmp byte [teleport_animation], 6	; are we all done with the teleport?
	jne .l2
	mov byte [comic_is_teleporting], 0
.l2:
	ret

; Pause the game. Display the pause graphic and wait for a keypress. If the
; keypress is 'q' or 'Q', jump to terminate_program.
pause:
	call blit_map_playfield_offscreen
	mov word [blit_WxH_width], 128/8	; graphic width is 128 pixels
	mov word [blit_WxH_height], 48		; graphic height is 48 pixels
	lea si, [GRAPHIC_PAUSE]
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(40, 64)	; screen coordinates
	call blit_WxH
	call wait_1_tick_and_swap_video_buffers

	; Clear the BIOS keyboard buffer. (Does not disable/reenable interrupts
	; while doing it.)
	xor cx, cx
	mov es, cx	; set es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head
	xor ax, ax	; ah=0x00: get keystroke
	int 0x16	; wait for any keystroke
	cmp al, 'q'
	jne .l1
.quit:
	jmp terminate_program	; terminate immediately if 'q' or 'Q' is pressed
.l1:
	cmp al, 'Q'
	je .quit
	ret

; Display the game over graphic, play SOUND_GAME_OVER, and wait for a keypress.
; Call do_high_scores and then jump to terminate_program.
game_over:
	call blit_map_playfield_offscreen
	mov word [blit_WxH_width], 128/8	; graphic width is 128 pixels
	mov word [blit_WxH_height], 48		; graphic height is 48 pixels
	lea si, [GRAPHIC_GAME_OVER]
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(40, 64)	; screen coordinates
	call blit_WxH
	call wait_1_tick_and_swap_video_buffers

	lea bx, [SOUND_GAME_OVER]
	mov ax, SOUND_PLAY
	mov cx, 2	; priority 2
	int3

.await_keystroke_for_high_scores:
	; Clear the BIOS keyboard buffer. (Does not disable/reenable interrupts
	; while doing it.)
	xor cx, cx
	mov es, cx	; set es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head
	xor ax, ax	; ah=0x00: get keystroke
	int 0x16	; wait for any keystroke
	call do_high_scores
	jmp terminate_program

; Blit a variable-size graphic.
; Input:
;   ds:si = address of graphic
;   di = offset relative to segment 0xa000 to which to blit
;   blit_WxH_width = width of graphic in bytes (1/8 the width in pixels)
;   blit_WxH_height = height of graphic in pixels
blit_WxH:
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	mov ah, 1
	call blit_WxH_plane
	mov ah, 2
	call blit_WxH_plane
	mov ah, 4
	call blit_WxH_plane
	mov ah, 8
	call blit_WxH_plane
	ret

; Blit a single plane of a variable-size graphic.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   ds:si = address of graphic
;   di = offset relative to segment 0xa000 to which to blit
;   blit_WxH_width = width of graphic in bytes (1/8 the width in pixels)
;   blit_WxH_height = height of graphic in pixels
blit_WxH_plane:
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	push di
	mov dx, di
	mov bx, [blit_WxH_height]
.loop:
	mov cx, [blit_WxH_width]
	rep movsb	; copy one row from ds:si to es:di
	add dx,	SCREEN_WIDTH/8	; next screen row
	mov di, dx
	dec bx		; next graphic row
	jne .loop
	pop di
	ret

; Run the sequence for when Comic has collected all three treasures and won the
; game.
; Output:
;   score = increased by 20000 points, plus 10000 points for each remaining life (including lives gained while adding points)
game_end_sequence:
	call beam_out

	; Award 20000 points for winning.
	mov cx, 20	; We will award 1000 points, 20 times
.score_loop_1:
	push cx

	lea bx, [SOUND_SCORE_TALLY]
	mov ax, SOUND_PLAY
	mov cx, 3	; priority 3
	int3

	mov al, 10	; 1000 points
	call award_points

	mov ax, 1
	call wait_n_ticks

	pop cx
	loop .score_loop_1

	; Award points for remaining lives.
.life_loop:
	cmp byte [comic_num_lives], 0
	je .play_theme	; break when comic_num_lives reaches 0

	; 10000 points for each life remaining.
	mov cx, 10	; We will award 1000 points, 10 times
.score_loop_2:
	push cx

	lea bx, [SOUND_SCORE_TALLY]
	mov ax, SOUND_PLAY
	mov cx, 3	; priority 3
	int3

	mov al, 10	; 1000 points
	call award_points

	mov ax, 1
	call wait_n_ticks

	pop cx
	loop .score_loop_2

	call lose_a_life	; next life

	mov ax, 3
	call wait_n_ticks

	jmp .life_loop

.play_theme:
	lea bx, [SOUND_TITLE]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	; Load the win graphic into the offscreen video buffer.
	lea dx, [FILENAME_WIN_GRAPHIC]	; "sys002.ega"
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	mov di, [offscreen_video_buffer_ptr]
	call load_fullscreen_graphic

	call palette_darken
	call swap_video_buffers	; swap the video buffer to point to the graphic
	call palette_fade_in

	mov ax, 20
	call wait_n_ticks

	; Clear the BIOS keyboard buffer. (Does not disable/reenable interrupts
	; while doing it.)
	xor ax, ax
	mov es, ax	; set es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head

	xor ax, ax	; ah=0x00: get keystroke
	int 0x16	; wait for any keystroke

	; Here we duplicate the game_over function, minus the playing of
	; SOUND_GAME_OVER.
	call blit_map_playfield_offscreen
	mov word [blit_WxH_width], 128/8	; graphic width is 128 pixels
	mov word [blit_WxH_height], 48		; graphic height is 48 pixels
	lea si, [GRAPHIC_GAME_OVER]
	mov di, [offscreen_video_buffer_ptr]
	add di, pixel_coords(40, 64)	; screen coordinates
	call blit_WxH
	call swap_video_buffers

	jmp game_over.await_keystroke_for_high_scores	; tail call

; Play the beam-out animation.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   camera_x = x-coordinate of left edge of playfield
beam_out:
	lea bx, [SOUND_MATERIALIZE]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	call blit_map_playfield_offscreen
	call blit_comic_playfield_offscreen
	call swap_video_buffers

	mov ax, 1
	call wait_n_ticks

	mov cx, 12	; 12 frames of animation
	lea si, [ANIMATION_MATERIALIZE]
.animation_loop:
	push cx
	push si

	push cx
	call blit_map_playfield_offscreen
	pop cx
	cmp cx, 6	; draw Comic under the animation for the first 6 frames of the animation
	jle .l1
	call blit_comic_playfield_offscreen
.l1:
	pop si
	push si
	mov bp, si
	add bp, 256	; address of mask
	mov dx, [camera_x]
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	sub ah, dl	; position of Comic relative to camera
	mov cx, 32	; height of graphic
	call blit_16xH_with_mask_playfield_offscreen	; blit the next frame of the animation
	call swap_video_buffers

	mov ax, 1
	call wait_n_ticks

	pop si
	add si, 320	; next frame of animation
	pop cx
	loop .animation_loop

	call blit_map_playfield_offscreen
	call swap_video_buffers
	; No handle_item; we assume there are no items onscreen.

	; Wait an additional 6 ticks at the end.
	mov ax, 6
	call wait_n_ticks

	ret

; Play the beam-in animation.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   camera_x = x-coordinate of left edge of playfield
beam_in:
	; Initially idle for 15 ticks.
	mov cx, 15
.delay_loop:
	push cx
	call blit_map_playfield_offscreen
	call handle_item
	call swap_video_buffers
	mov ax, 1
	call wait_n_ticks
	pop cx
	loop .delay_loop

	; Then additionally wait until SOUND_TITLE_RECAP stops playing.
	mov ax, SOUND_QUERY
	int3		; get whether a sound is playing in al
	or ax, ax
	jz .play_sound	; break if no sound playing
	mov cx, 1
	jmp .delay_loop	; otherwise wait a tick and check again

.play_sound:
	lea bx, [SOUND_MATERIALIZE]
	mov ax, SOUND_PLAY
	mov cx, 4	; priority 4
	int3

	mov cx, 12	; 12 frames of animation
	lea si, [ANIMATION_MATERIALIZE]
.animation_loop:
	push cx
	push si

	push cx
	call blit_map_playfield_offscreen
	pop cx

	cmp cx, 6	; start drawing Comic after 6 frames of animation
	jg .l1
	call blit_comic_playfield_offscreen
.l1:
	pop si
	push si
	mov bp, si
	add bp, 256	; address of mask
	mov dx, [camera_x]
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	sub ah, dl	; position of Comic relative to camera
	mov cx, 32	; height of graphic
	call blit_16xH_with_mask_playfield_offscreen	; blit the next frame of the animation
	call handle_item
	call swap_video_buffers

	mov ax, 1
	call wait_n_ticks

	pop si
	add si, 320	; next frame of animation
	pop cx
	loop .animation_loop

	call blit_map_playfield_offscreen
	call blit_comic_playfield_offscreen
	call handle_item
	call swap_video_buffers

	; Wait an additional 1 tick at the end.
	mov ax, 1
	call wait_n_ticks

	ret

; Main game loop. Block until it's time for a game tick, do one tick's worth of
; game logic, then repeat. When win_counter reaches 1, jump to
; game_end_sequence.
;
; Input:
;   game_tick_flag = game_loop blocks until int8_handler sets this variable to 1
;   win_counter = 0 if the win condition is not activated yet; otherwise counter until game end
;   key_state_jump, key_state_fire, key_state_left, key_state_right,
;     key_state_open, key_state_teleport, key_state_esc = state of inputs
; Output:
;   game_tick_flag = 0
;   win_counter = decremented by 1 if not already 0
;
; Reads and writes many other global variables, either directly in game_loop or
; in subroutines.
game_loop:
	; Clear the BIOS keyboard buffer. (Does not disable/reenable interrupts
	; while doing it.)
	xor cx, cx
	mov es, cx	; set es = 0x0000
	mov cl, [es:BIOS_KEYBOARD_BUFFER_HEAD]
	mov [es:BIOS_KEYBOARD_BUFFER_TAIL], cl	; assign tail = head

	; Busy-wait until int8_handler sets game_tick_flag.
	cmp byte [cs:game_tick_flag], 1
	je .tick

	; While waiting, reinitialize comic_jump_counter if Comic is not in the
	; air and the player is not pressing the jump button.
	cmp byte [comic_is_falling_or_jumping], 1
	je game_loop
	cmp byte [cs:key_state_jump], 1
	je game_loop
	; On the ground and not pressing jump; recharge for another jump.
	mov al, [comic_jump_power]
	mov [comic_jump_counter], al
	jmp game_loop

.tick:
	mov byte [cs:game_tick_flag], 0

	; If win_counter is nonzero, we are waiting out the clock to start the
	; game end sequence. The game ends when win_counter decrements to 1.
	cmp byte [win_counter], 0
	je .not_won_yet
	dec byte [win_counter]
	cmp byte [win_counter], 1
	jne .not_won_yet
	jmp game_end_sequence

.not_won_yet:
	; Advance comic_run_cycle in the cycle COMIC_RUNNING_1,
	; COMIC_RUNNING_2, COMIC_RUNNING_3.
	mov ch, [comic_run_cycle]
	inc ch
	cmp ch, COMIC_RUNNING_3 + 1	; comic_run_cycle overflow?
	jne .l1
	mov ch, COMIC_RUNNING_1	; comic_run_cycle overflowed; reset it
.l1:
	mov [comic_run_cycle], ch

	; Put Comic in the standing state by default for this tick; it may be
	; set otherwise according to input or other factors.
	mov byte [comic_animation], COMIC_STANDING

	; Is there HP pending to award?
	cmp byte [comic_hp_pending_increase], 0
	je .check_teleport
	; If so, award 1 unit of it.
	dec byte [comic_hp_pending_increase]
	call increment_comic_hp

.check_teleport:
	; Are we teleporting?
	cmp byte [comic_is_teleporting], 0
	je .check_falling_or_jumping
	call handle_teleport
	jmp .handle_nonplayer_actors	; skip blitting the map and Comic, because handle_teleport does it

.check_falling_or_jumping:
	mov si, [comic_y]	; si holds both comic_y and comic_x
	; Are we jumping or falling?
	cmp byte [comic_is_falling_or_jumping], 0
	je .check_jump_input
	jmp handle_fall_or_jump	; handle_fall_or_jump jumps back into game_loop.check_pause_input

.check_jump_input:
	; Is the jump key being pressed?
	cmp byte [cs:key_state_jump], 1
	jne .recharge_jump_counter

	; And is comic_jump_counter not exhausted? (I don't see how it could
	; be, seeing as we've checked that comic_is_falling_or_jumping == 0
	; already above.)
	cmp byte [comic_jump_counter], 1
	je .check_open_input

	; Initiate a new jump.
	mov byte [comic_is_falling_or_jumping], 1
	jmp handle_fall_or_jump	; handle_fall_or_jump jumps back into game_loop.check_pause_input

.recharge_jump_counter:
	mov al, [comic_jump_power]
	mov [comic_jump_counter], al

.check_open_input:
	; Is the open key being pressed?
	cmp byte [cs:key_state_open], 1
	jne .check_teleport_input

	; The open key is being pressed, now let's see if we're actually in
	; front of a door.
	mov di, [current_stage_ptr]
	lea bx, [di + stage.doors]
	mov cx, 3	; for all 3 doors
	mov ax, si	; al = comic_y, ah = comic_x
.door_loop:
	mov dx, [bx]	; dl = door.y, dh = door.x
	cmp al, dl	; require exact match in y-coordinate
	jne .door_loop_next
	neg dh
	add dh, ah	; dh = comic_y - door.y
	cmp dh, 3	; require comic_x - door.y to be 0, 1, or 2
	jae .door_loop_next	; jae treats the comparison as unsigned: negative dh becomes large positive
	; Pressing the open key, in front of a door, but do we have the Door
	; Key?
	cmp byte [comic_has_door_key], 1
	jne .door_loop_next
	; Pressing the open key, in front of a door, holding the Door Key.
	jmp activate_door	; tail call; activate_door calls load_new_level or load_new_stage, which jump back into game_loop
.door_loop_next:
	add bx, door_size	; next door
	loop .door_loop

.check_teleport_input:
	; Is the teleport key being pressed?
	cmp byte [cs:key_state_teleport], 1
	jne .check_left_input

	; The teleport key is being pressed. Do we have the Teleport Wand?
	cmp byte [comic_has_teleport_wand], 1
	jne .check_left_input

	; Start a teleport.
	call begin_teleport
	jmp .check_teleport

.check_left_input:
	mov byte [comic_x_momentum], 0
	; Is the left key being pressed?
	cmp byte [cs:key_state_left], 1
	jne .check_right_input

	mov byte [comic_x_momentum], -5
	call face_or_move_left	; modifies si

.check_right_input:
	; Is the right key being pressed?
	cmp byte [cs:key_state_right], 1
	jne .check_for_floor

	mov byte [comic_x_momentum], +5
	call face_or_move_right	; modifies si

.check_for_floor:
	mov ax, si	; al = comic_y, ah = comic_x
	add al, 4	; comic_y + 4 is the y-coordinate just below Comic's feet
	call address_of_tile_at_coordinates	; bx = address of tile under Comic's feet
	mov cl, [tileset_last_passable]
	cmp byte [bx], cl	; is it solid?
	jg .check_pause_input

	test ah, 1	; is Comic halfway between two tiles (comic_x is odd)?
	jz .no_floor
	cmp [bx + 1], cl	; if so, also check the solidity of the tile 1 to the right
	jg .check_pause_input

.no_floor:
	; We're not standing on solid ground (just walked off an edge). Start
	; falling. The gravity when just starting to fall is 8 (i.e., 1 unit
	; per tick for the first tick), but later in handle_fall_or_jump
	; gravity will become 5 (0.625 units per tick per tick), or 3 in the
	; space level (0.375 units per tick per tick).
	mov byte [comic_y_vel], 8
	; After walking off an edge, Comic has 2 units of momentum (will keep
	; moving in that direction for 2 ticks, even if a direction button is
	; not pressed).
	cmp byte [comic_x_momentum], 0
	jl .moving_left
	je .start_falling
.moving_right:
	mov byte [comic_x_momentum], +2
	jmp .start_falling
.moving_left:
	mov byte [comic_x_momentum], -2

.start_falling:
	; Now falling, as if with a fully depleted jump.
	mov byte [comic_is_falling_or_jumping], 1
	mov byte [comic_jump_counter], 1

.check_pause_input:
	mov [comic_y], si	; pause trashes si, so save it

	; Is the escape key being pressed?
	cmp byte [cs:key_state_esc], 1
	jne .check_fire_input

	call pause

.check_fire_input:
	; Is the fire key being pressed?
	cmp byte [cs:key_state_fire], 1
	jne .recover_fireball_meter	; if not, allow the fireball meter to recharge

	; The fire key is being pressed. Is the fireball meter depleted?
	cmp byte [fireball_meter], 0
	je .blit_map_and_comic

	; Shoot a fireball.
	mov ax, si	; al = comic_y, ah = comic_x
	inc al		; fireball's initial y-coordinate is 1 unit lower than the top of Comic's head
	call try_to_fire

	; fireball_meter increases/decreases at a rate of 1 unit per 2 ticks.
	; fireball_meter_counter alternates 2, 1, 2, 1, ... to let us know when
	; we should adjust the meter. We decrement fireball_meter when
	; fireball_meter_counter is 2 (and the fire key is being pressed), and
	; increment fireball_meter when fireball_meter_counter is 1 (and the
	; fire key is not being pressed).
	cmp byte [fireball_meter_counter], 1
	je .wrap_fireball_meter_counter
	; fireball_meter_counter is 2; decrement fireball_meter and decrement
	; fireball_meter_counter.
	call decrement_fireball_meter

.recover_fireball_meter:
	dec byte [fireball_meter_counter]
	jnz .blit_map_and_comic
	; fireball_meter_counter was 1; increment fireball_meter and wrap
	; fireball_meter_counter.
	call increment_fireball_meter

.wrap_fireball_meter_counter:
	; fireball_meter_counter was 1 this tick; reset it to 2.
	mov byte [fireball_meter_counter], 2

.blit_map_and_comic:
	call blit_map_playfield_offscreen
	call blit_comic_playfield_offscreen

.handle_nonplayer_actors:
	call handle_enemies
	call handle_fireballs
	call handle_item
	call swap_video_buffers

	jmp game_loop

; Call switch_video_buffer on 0x0000 or 0x2000, whichever is not onscreen at
; the moment.
; Input:
;   offscreen_video_buffer_ptr = 0x0000 or 0x2000, whichever is currently offscreen
;   item_animation_counter = whether to to draw an even or odd animation frame for the item
; Output:
;   offscreen_video_buffer_ptr = swapped to the other value
;   item_animation_counter = advanced by one position
swap_video_buffers:
	mov cx, [offscreen_video_buffer_ptr]
	call switch_video_buffer
	; Change 0x0000 to 0x2000 and 0x2000 to 0x0000.
	sub cx, 0x2000
	je .l1
	mov cx, 0x2000
.l1:
	mov [offscreen_video_buffer_ptr], cx
	ret

; If there is an item in the current stage and it has not been collected yet,
; collect it if it is colliding with Comic, otherwise blit it at its location.
; Input:
;   comic_x, comic_y = coordinates of Comic
;   camera_x = x-coordinate of left edge of playfield
;   items_collected = bitmap of stages' item collection status
; Output:
;   items_collected = updated bitmap of stages' item collection status
;   item_animation_counter = advanced by one position
;   comic_num_lives = incremented by 1 if collecting a Shield with full HP
;   comic_hp_pending_increase = set to MAX_HP if collecting a Shield
;   comic_firepower, comic_jump_power, comic_has_corkscrew, comic_has_door_key,
;     comic_has_lantern, comic_has_teleport_wand, comic_num_treasures = updated as appropriate
;   score = increased by 2000 points if collecting an item
handle_item:
	mov di, [current_stage_ptr]
	cmp byte [di + stage.item_type], ITEM_UNUSED
	je .done	; there is no item in this stage

	lea bx, [items_collected]
	mov al, [current_stage_number]
	shl al, 1
	shl al, 1
	shl al, 1
	shl al, 1
	add al, [current_level_number]
	xor ah, ah
	add bx, ax
	cmp byte [bx], 1	; items_collected[current_level_number][current_stage_number] == 1?
	je .done	; already collected the item in this stage

	mov ax, [di + stage.item_y]	; al = stage.item_y, ah = stage.item_x
	mov dx, [camera_x]
	sub ah, dl	; item's camera-relative x-coordinate
	or ah, ah
	jl .done	; item is off the left side of the playfield
	cmp ah, 22
	jg .done	; item is off the right side of the playfield

	mov bl, [di + stage.item_type]
	call collect_item_or_blit_offscreen

.done:
	ret

; Handle a Comic death by falling. Blit what of Comic remains in the playfield
; and play SOUND_TOO_BAD. If this was the last life, jump to game_over. If
; there are lives remaining, subtract one, reinitialize state variables, and
; jump to load_new_stage.comic_located to respawn.
; Input:
;   si = coordinates of Comic
;   comic_num_lives = if 0, game over; otherwise respawn
; Output:
;   comic_x, comic_y = copied from si in input
;   comic_run_cycle = 0
;   comic_is_falling_or_jumping = 0
;   comic_x_momentum = 0
;   comic_y_vel = 0
;   comic_jump_counter = 0
;   comic_animation = COMIC_STANDING
;   comic_hp = 0
;   comic_hp_pending_increase = MAX_HP
;   fireball_meter_counter = 2
comic_dies:
	mov ax, si	; al = comic_y, ah = comic_x
	mov [comic_y], ax	; store both comic_y and comic_x
	cmp al, PLAYFIELD_HEIGHT - 1	; is Comic completely off the bottom of the screen?
	jg .too_bad

	; Comic is at least partially still onscreen. Blit the part of him that
	; is still visible.
	mov byte [comic_animation], COMIC_JUMPING
	mov cx, 20
	sub cl, al	; amount of comic visible in game units: 20 - comic_y
	shl cx, 1
	shl cx, 1
	shl cx, 1	; amount of comic visible in pixels: (20 - comic_y) * 8
	push cx
	call blit_map_playfield_offscreen
	pop cx
	mov bx, 32	; total height of a Comic graphic
	sub bx, cx	; 32 - (20 - comic_y) * 8
	call blit_comic_partial_playfield_offscreen
	call swap_video_buffers
	mov ax, 1
	call wait_n_ticks

; Jump to this label for a death by HP depletion (as handle_enemies does).
.too_bad:
	call blit_map_playfield_offscreen
	call swap_video_buffers

	mov ax, 2
	call wait_n_ticks

	mov ax, SOUND_PLAY
	lea bx, [SOUND_TOO_BAD]
	mov cx, 4	; priority 4
	int3

	mov ax, 15
	call wait_n_ticks

	cmp byte [comic_num_lives], 0	; any lives remaining?
	jne .respawn

	; All out of lives.
	jmp game_over

.respawn:
	call lose_a_life
	mov byte [comic_run_cycle], 0
	mov byte [comic_is_falling_or_jumping], 0	; bug: Comic is considered to be "on the ground" (can jump and teleport) immediately after respawning, even if in the air
	mov byte [comic_x_momentum], 0
	mov byte [comic_y_vel], 0
	mov byte [comic_jump_counter], 0	; bug: jumping immediately after respawning underflows comic_jump_counter (hover glitch)
	mov byte [comic_animation], COMIC_STANDING
	mov byte [comic_hp_pending_increase], MAX_HP	; let the HP fill up from zero after respawning
	mov byte [fireball_meter_counter], 2
	; comic_is_teleporting is set to 0 in load_new_stage.comic_located.
	jmp load_new_stage.comic_located	; respawn in the same stage at (comic_x_checkpoint, comic_y_checkpoint)

; Handle Comic movement when comic_is_falling_or_jumping == 1. Apply upward
; acceleration due to jumping, apply downward acceleration due to gravity,
; check for solid ground, and handle midair left/right movement.
; Input:
;   si = coordinates of Comic
;   key_state_left, key_state_right, key_state_jump = state of inputs
;   current_level_number = if LEVEL_NUMBER_SPACE, use lower gravity
;   comic_jump_counter = how many ticks Comic can continue moving upward
;   comic_x_momentum = Comic's current x momentum
;   comic_y_vel = Comic's current y velocity, in units of 1/8 game units per tick
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
; Output:
;   si = updated coordinates of Comic
;   comic_is_falling_or_jumping = updated
;   comic_jump_counter = decremented by 1 unless already at 1
;   comic_x_momentum = updated
;   comic_y_vel = updated
;   comic_facing = updated
handle_fall_or_jump:
	; Are we still in the state where a jump can continue accelerating
	; upward? When comic_jump_counter is 1, the upward part of the jump is
	; over.
	dec byte [comic_jump_counter]	; decrement the counter
	jz .jump_counter_expired	; when it hits bottom, this jump can no longer accelerate upward

	; We're still able to accelerate upward in this jump. Is the jump key
	; still being pressed?
	cmp byte [cs:key_state_jump], 1
	jne .integrate_vel

	; We're still accelerating upward in this jump. Subtract a fixed value
	; from the vertical velocity.
	sub byte [comic_y_vel], 7
	jmp .integrate_vel

.jump_counter_expired:
	; The upward part of the jump is over (comic_jump_counter was 1 when
	; the function was called, and just became 0). Clamp it to a minimum
	; value of 1.
	inc byte [comic_jump_counter]

.integrate_vel:
	mov al, [comic_y_vel]
	mov cl, al
	sar al, 1
	sar al, 1
	sar al, 1	; comic_y_vel / 8
	mov bx, si	; bl = comic_y, bh = comic_x
	add bl, al	; comic_y += comic_y_vel / 8

	jge .l1		; did comic_y just become negative (above the top of the screen)?
	mov bl, 0	; clip to the top of the screen

.l1:
	mov si, bx	; return value in si, with modified comic_y

	; Is the top of Comic's head within 3 units of the bottom of the screen
	; (i.e., his feet are below the bottom of the screen)?
	cmp bl, PLAYFIELD_HEIGHT - 3
	jb .apply_gravity
	jmp comic_dies	; if so, it's a death by falling

.apply_gravity:
	cmp byte [current_level_number], LEVEL_NUMBER_SPACE	; is this the space level?
	jne .l2
	sub cl, COMIC_GRAVITY - COMIC_GRAVITY_SPACE	; if so, low gravity
.l2:
	add cl, COMIC_GRAVITY	; otherwise, gravity is normal

	; Clip downward velocity.
	cmp cl, TERMINAL_VELOCITY + 1
	jl .l3
	mov cl, TERMINAL_VELOCITY
.l3:
	mov [comic_y_vel], cl

	; Adjust comic_x_momentum based on left/right inputs.
.check_left_input:
	mov cl, [comic_x_momentum]
	; Is the left key being pressed?
	cmp byte [cs:key_state_left], 1
	jne .check_right_input
	mov byte [comic_facing], COMIC_FACING_LEFT	; immediately face left when in the air
	dec cl		; decrement horizontal momentum
	cmp cl, -5
	jge .check_right_input
	mov cl, -5	; clamp momentum to not go below -5

.check_right_input:
	cmp byte [cs:key_state_right], 1
	jne .check_move_left
	mov byte [comic_facing], COMIC_FACING_RIGHT	; immediately face right when in the air
	inc cl		; increment horizontal momentum
	cmp cl, +5
	jle .check_move_left
	mov cl, +5	; clamp momentum to not go above +5

.check_move_left:
	mov [comic_x_momentum], cl	; store the possibly modified horizontal momentum
	or cl, cl
	jge .check_move_right
	inc byte [comic_x_momentum]	; drag momentum towards 0
	call move_left
.check_move_right:
	mov al, [comic_x_momentum]
	or al, al
	jle .check_solidity_upward
	dec byte [comic_x_momentum]	; drag momentum towards 0
	call move_right

	; While moving upward, we check the solidity of the tile Comic's head
	; is in. While moving downward, we instead check the solidity of the
	; tile under his feet.
.check_solidity_upward:
	mov ax, si	; al = comic_y, ah = comic_x
	call address_of_tile_at_coordinates	; find the tile Comic's head is in
	mov dl, [tileset_last_passable]
	cmp [bx], dl	; is it a solid tile?
	jg .head_in_solid_tile

	test ah, 1	; is Comic halfway between two tiles (comic_x is odd)?
	je .check_solidity_downward
	cmp [bx + 1], dl	; if so, also check the solidity of the tile 1 to right
	jle .check_solidity_downward

.head_in_solid_tile:
	mov byte [comic_y_vel], 8	; bounce downward off the ceiling

.check_solidity_downward:
	; Are we even moving downward?
	cmp byte [comic_y_vel], 0
	jle .still_falling_or_jumping

	mov cx, si	; cl = comic_y, ch = comic_x
	mov ax, cx	; al = comic_y, ah = comic_x
	add al, 5	; comic_y + 5, the y-coordinate 1 unit below Comic's feet
	call address_of_tile_at_coordinates
	mov dl, [tileset_last_passable]
	cmp [bx], dl	; is it a solid tile?
	jg .hit_the_ground

	test ah, 1	; is Comic halfway between two tiles (comic_x is odd)?
	je .still_falling_or_jumping
	cmp [bx + 1], dl	; if so, also check the solidity of the tile 1 to right
	jg .hit_the_ground

.still_falling_or_jumping:
	mov byte [comic_animation], COMIC_JUMPING
	jmp game_loop.check_pause_input	; jump back into game_loop, skipping its open/teleport/left/right/collision code

.hit_the_ground:
	; Above, under .check_solidity_downward, we checked the tile at
	; (comic_x, comic_y + 5) and found it to be solid. But if comic_y is 15
	; or greater (i.e., comic_y + 5 is 20 or greater), that tile lookup
	; actually looked at garbage data outside the map. Override the earlier
	; decision and act as if the tile had not been solid, because there can
	; be nothing solid below the bottom of the map.
	cmp cl, PLAYFIELD_HEIGHT - 5
	jb .l4
	jmp .still_falling_or_jumping
.l4:
	; Now we actually are about to hit the ground. Clamp Comic's feet to an
	; even tile boundary and reset his vertical velocity to 0.
	inc cl
	and cl, 0xfe	; clamp to an even tile boundary: comic_y = floor((comic_y + 1)/2)*2
	mov si, cx	; return value in si
	mov byte [comic_is_falling_or_jumping], 0	; no longer falling or jumping
	mov byte [comic_y_vel], 0	; stop moving vertically
	jmp game_loop.check_pause_input	; jump back into game_loop, skipping its open/teleport/left/right/collision code

; If Comic is not already facing left, face left. If already facing left, jump
; to move_left.
; Input:
;   si = coordinates of Comic
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
; Output:
;   si = updated coordinates of Comic
;   comic_facing = COMIC_FACING_LEFT
;   comic_facing = updated
;   camera_x = moved left if appropriate
;   comic_animation = comic_run_cycle if now moving left
face_or_move_left:
	cmp byte [comic_facing], COMIC_FACING_LEFT	; already facing left?
	je move_left	; if so, move left
	mov byte [comic_facing], COMIC_FACING_LEFT	; otherwise, just face left
	jmp move_left.return

; Move Comic to the left if possible. If hitting a stage edge, jump to
; stage_edge_transition.
; Input:
;   si = coordinates of Comic
; Output:
;   si = updated coordinates of Comic
;   camera_x = moved left if appropriate
;   comic_animation = comic_run_cycle if now moving left
move_left:
	mov ax, si	; al = comic_y, ah = comic_x
	or ah, ah	; comic_x == 0 (at the far left edge of a stage)?
	jne .not_at_stage_edge

	; We're at the left edge of a stage. Check for a stage transition here.
	mov di, [current_stage_ptr]
	cmp byte [di + stage.exit_l], EXIT_UNUSED	; is there a left exit?
	je .halt_and_return	; if not, stop moving and return

	; There is a stage transition here. Load the new stage.
	mov ah, MAP_WIDTH - 2	; put Comic at the far right of the new stage
	mov bl, [di + stage.exit_l]	; target stage
	jmp stage_edge_transition	; tail call

.not_at_stage_edge:
	dec ah		; try moving left 1 half-tile
	add al, 3
	call address_of_tile_at_coordinates	; bx = address of tile at Comic's knees
	sub al, 3
	mov ch, [tileset_last_passable]
	cmp [bx], ch	; did we try to walk into a solid tile?
	jg .halt_and_return	; if so, stop moving and return

	mov si, ax	; return value in si, with modified comic_x

	; Comic is now moving; enter a run animation. comic_run_cycle is
	; COMIC_RUNNING_1, COMIC_RUNNING_2, or COMIC_RUNNING_3.
	mov ch, [comic_run_cycle]
	mov [comic_animation], ch

	; Move the camera left too, unless the camera is already at the far
	; left edge, or Comic is on the right half of the screen (keep the
	; camera still until he reaches the center).
	mov bx, [camera_x]
	sub ah, bl	; ah = comic_x - camera_x
	or bx, bx
	jz .return	; camera is already at far left edge
	cmp ah, PLAYFIELD_WIDTH/2 - 2
	jae .return	; Comic is on the right half of the screen
	dec word [camera_x]
	jmp .return

.halt_and_return:
	mov byte [comic_x_momentum], 0	; stop moving horizontally
.return:
	ret

; If Comic is not already facing right, face right. If already facing right,
; jump to move_right.
; Input:
;   si = coordinates of Comic
;   comic_facing = COMIC_FACING_RIGHT or COMIC_FACING_LEFT
; Output:
;   si = updated coordinates of Comic
;   comic_facing = COMIC_FACING_RIGHT
;   camera_x = moved left if appropriate
;   comic_animation = comic_run_cycle if now moving left
face_or_move_right:
	cmp byte [comic_facing], COMIC_FACING_RIGHT	; already facing right?
	je move_right	; if so, move right
	mov byte [comic_facing], COMIC_FACING_RIGHT	; otherwise, just face right
	jmp move_left.return

; Move Comic to the right if possible. If hitting a stage edge, jump to
; stage_edge_transition.
; Input:
;   si = coordinates of Comic
; Output:
;   si = updated coordinates of Comic
;   camera_x = moved left if appropriate
;   comic_animation = comic_run_cycle if now moving right
move_right:
	mov ax, si	; al = comic_y, ah = comic_x
	cmp ah, MAP_WIDTH - 2	; comic_x == 254 (at the far right edge of a stage)?
	jne .not_at_stage_edge

	; We're at the right edge of a stage. Check for a stage transition
	; here.
	mov di, [current_stage_ptr]
	cmp byte [di + stage.exit_r], EXIT_UNUSED	; is there a right exit?
	je move_left.halt_and_return	; if not, stop moving and return

	; There is a stage transition here. Load the new stage.
	xor ah, ah	; put Comic at the far left of the new stage
	mov bl, [di + stage.exit_r]	; target stage
	jmp stage_edge_transition	; tail call
	nop		; dead code

.not_at_stage_edge:
	; The next line looks like a copy-paste error. The Z flag cannot be set
	; at this point.
	je move_left.halt_and_return	; useless instruction

	add ah, 2	; look at the tile that is 1 unit to the right of Comic's knees
	add al, 3
	call address_of_tile_at_coordinates	; bx = address of tile at Comic's feet
	sub al, 3
	dec ah		; if Comic is not blocked, this will be the new comic_x (1 unit to the right of where it was)
	mov ch, [tileset_last_passable]
	cmp [bx], ch	; did we try to walk into a solid tile?
	jg move_left.halt_and_return	; if so, stop moving and return

	mov si, ax	; return value in si, with modified comic_x

	; Comic is now moving; enter a run animation. comic_run_cycle is
	; COMIC_RUNNING_1, COMIC_RUNNING_2, or COMIC_RUNNING_3.
	mov ch, [comic_run_cycle]
	mov [comic_animation], ch

	; Move the camera right too, unless the camera is already at the far
	; right edge, or Comic is on the left half of the screen (keep the
	; camera still until he reaches the center).
	mov bx, [camera_x]
	sub ah, bl	; ah = comic_x - camera_x
	cmp bx, MAP_WIDTH - PLAYFIELD_WIDTH
	jge move_left.return	; camera is already at far right edge
	cmp ah, PLAYFIELD_WIDTH / 2
	jle move_left.return	; Comic is on the left half of the screen
	inc word [camera_x]
	jmp move_left.return

; Enter a new stage via a stage edge transition. Play
; SOUND_STAGE_EDGE_TRANSITION and jump to load_new_stage.
; Input:
;   ah = x-coordinate of Comic
;   al = y-coordinate of Comic
;   bl = new stage number
; Output:
;   comic_x_checkpoint = ah
;   comic_y_checkpoint = al
;   comic_y_vel = 0
;   source_door_level_number = -1
;   current_stage_number = bl
stage_edge_transition:
	mov [comic_y_checkpoint], ax	; assign comic_y_checkpoint and comic_x_checkpoint simultaneously
	mov byte [comic_y_vel], 0
	mov byte [source_door_level_number], -1	; we are not entering the new stage via a door
	mov [current_stage_number], bl

	mov ax, SOUND_PLAY
	lea bx, [SOUND_STAGE_EDGE_TRANSITION]
	mov cx, 4	; priority 4
	int3

	jmp load_new_stage	; tail call

; Go through the door pointed to by bx. Call enter_door, then load_new_level or
; load_new_level as appropriate.
; Input:
;   bx = pointer to door struct
;   current_level_number, current_stage_number = where the door is located
; Output:
;   current_level_number, current_stage_number = target of the door
;   source_door_level_number = former current_level_number
;   source_door_stage_number = former current_stage_number
activate_door:
	push bx
	mov ax, [bx + door.y]	; al = door.y, ah = door.x
	call enter_door
	pop bx

	; Mark that we're entering the new level/stage via a door.
	mov al, [current_level_number]
	mov [source_door_level_number], al
	mov al, [current_stage_number]
	mov [source_door_stage_number], al

	; Set the current level/stage to wherever the door leads.
	mov al, [bx + door.target_stage]
	mov [current_stage_number], al
	mov al, [bx + door.target_level]
	mov [current_level_number], al

	cmp al, [source_door_level_number]	; it it another stage in the same level?
	je .same_level
	jmp load_new_level	; different level means we need to reload the whole level (including map tiles, enemy graphics, etc.)
.same_level:
	jmp load_new_stage	; same level means we only need to load the new stage

; Handle enemy AI and spawning for one game tick.
; Input:
;   enemies = table of enemies in the current stage
; Output:
;   enemy_spawned_this_tick = 1 if an enemy was spawned this tick, 0 otherwise
handle_enemies:
	mov byte [enemy_spawned_this_tick], 0

	lea si, [enemies]
	mov cx, MAX_NUM_ENEMIES	; for each enemy slot
.enemy_loop:
	push cx		; enemy loop counter
	; Is this enemy spawned, despawned, or dying?
	cmp byte [si + enemy.state], ENEMY_STATE_SPAWNED
	je .spawned	; enemy.state == ENEMY_STATE_SPAWNED
	jb .despawned	; enemy.state == ENEMY_STATE_DESPAWNED
	jmp .dying	; enemy.state >= 2 means an enemy death animation is playing

.despawned:
	sub byte [si + enemy.spawn_timer_and_animation], 1	; advance the spawn timer
	jae .awaiting_respawn
	; Spawn counter is finished; try to spawn the enemy in this slot.
	jmp maybe_spawn_enemy	; jumps back to .enemy_loop_next
.awaiting_respawn:
	jmp .enemy_loop_next

.spawned:
	; This is a spawned enemy. Advance the animation counter.
	mov al, [si + enemy.spawn_timer_and_animation]
	inc al
	cmp al, [si + enemy.num_animation_frames]	; animation counter hit the number of animation frames?
	jne .l1
	xor al, al	; wrap back to animation frame 0
.l1:
	mov [si + enemy.spawn_timer_and_animation], al	; store modified animation counter

	; Branch based on enemy.behavior. The enemy_behavior_* subroutines jump
	; back to .check_despawn when finished.
	mov al, [si + enemy.behavior]
	dec al
	jz .switch_behavior_bounce	; enemy.behavior == 1
	dec al
	jz .switch_behavior_leap	; enemy.behavior == 2
	dec al
	jz .switch_behavior_roll	; enemy.behavior == 3
	dec al
	jz .switch_behavior_seek	; enemy.behavior == 4
	dec al
	jz .switch_behavior_shy		; enemy.behavior == 5
	jmp .enemy_loop_next		; skip this enemy if its behavior is unknown
.switch_behavior_bounce:
	jmp enemy_behavior_bounce
.switch_behavior_leap:
	jmp enemy_behavior_leap
.switch_behavior_roll:
	jmp enemy_behavior_roll
.switch_behavior_seek:
	jmp enemy_behavior_seek
.switch_behavior_shy:
	jmp enemy_behavior_shy

.check_despawn:
	; enemy_behavior_* subroutines jump back to this point.
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	mov bx, ax		; bl = enemy.y, bh = enemy.x
	mov dx, [comic_y]	; dl = comic_y, dh = comic_x
	sub bh, dh	; bh = enemy.x - comic_x
	jae .l2
	neg bh		; bh = abs(enemy.x - comic_x)
.l2:
	; Despawn enemies that are far away from Comic.
	cmp bh, ENEMY_DESPAWN_RADIUS
	jb .check_collision_with_comic
	jmp .despawn

.check_collision_with_comic:
	; Check for collision with Comic. First check in the horizontal
	; direction.
	cmp bh, 2	; abs(enemy.x - comic_x) >= 2?
	jae .blit	; if so, no collision

	; The enemy is within collision range horizontally. Now check in the
	; vertical direction.
	sub bl, dl	; enemy.y - comic_y < 0?
	jb .blit	; if so, no collision
	cmp bl, 4	; enemy.y - comic_y >= 4?
	jae .blit	; if so, no collision

	; Enemy collides with Comic.
	mov byte [si + enemy.state], ENEMY_STATE_RED_SPARK	; start the enemy's red spark animation.
	; Does this hit kill Comic?
	cmp byte [comic_hp], 0
	jne .hurt_comic
	cmp byte [inhibit_death_by_enemy_collision], 0	; don't re-enter Comic's death animation if it is already in progress
	jne .hurt_comic

.kill_comic:
	mov byte [inhibit_death_by_enemy_collision], 1	; death animation is in progress; don't allow enemies to re-kill Comic
	call comic_death_animation	; calls recursively back into handle_enemies
	mov byte [inhibit_death_by_enemy_collision], 0	; death animation is finished
	jmp comic_dies.too_bad

.hurt_comic:
	; The collision with an enemy didn't kill Comic, only hurt him. (Or
	; he's taking hits while inhibit_death_by_enemy_collision != 0.)
	push si
	call decrement_comic_hp
	pop si
	jmp .dying

.blit:
	mov dx, [camera_x]
	sub ah, dl	; enemy's camera-relative x-coordinate
	; Is the enemy within the playfield?
	cmp ah, 0
	jl .enemy_loop_next	; off the left side, no need to blit
	cmp ah, PLAYFIELD_WIDTH - 2
	jg .enemy_loop_next	; off the right side, no need to blit

	push si
	push ax
	mov al, [si + enemy.spawn_timer_and_animation]	; al = current animation frame
	add al, [si + enemy.facing]
	xor ah, ah
	shl ax, 1
	mov si, [si + enemy.animation_frames_ptr]
	add si, ax
	mov si, [si]	; enemy.animation_frames_ptr[enemy.spawn_timer_and_animation + enemy.facing]
	mov bp, si
	add bp, 128	; address of mask
	mov cx, 16	; number of rows in graphic
	pop ax		; ah = y-coordinate, al = camera-relative x-coordinate
	call blit_16xH_with_mask_playfield_offscreen
	pop si

.enemy_loop_next:
	; maybe_spawn_enemy jumps back to this point.
	pop cx		; enemy loop counter
	add si, enemy_size	; next enemy slot
	dec cx
	je .l3		; done with all enemies?
	jmp .enemy_loop
.l3:
	ret

.dying:
	; The enemy is spawned but in the process of dying. Check if a spark
	; animation is still playing.
	;  2..6: in white spark animation
	;     7: finished white spark animation
	; 8..12: in red spark animation
	;    13: finished red spark animation
	; In bl, we store a normalized animation counter that is in the range
	; 2..7, no matter which of the two animations is playing.
	mov bl, [si + enemy.state]
	cmp bl, 2 + 5	; in white spark animation?
	je .goto_despawn	; finished the white spark animation
	jl .advance_spark_animation	; still in white spark animation
	sub bl, ENEMY_STATE_RED_SPARK - ENEMY_STATE_WHITE_SPARK
	cmp bl, 2 + 5	; in red spark animation?
	jne .advance_spark_animation	; still in red spark animation
	; Otherwise, finished red spark animation.
.goto_despawn:
	jmp .despawn

.advance_spark_animation:
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	mov dx, [camera_x]
	sub ah, dl	; camera-relative x-coordinate

	; Despawn dying enemies that leave the playfield.
	cmp ah, 0
	jl .despawn
	cmp ah, PLAYFIELD_WIDTH - 2
	jg .despawn

	cmp bl, 2 + 2	; blit the enemy under the spark for the first 3 frames (bl in 2..4)
	jg .blit_spark

	; Blit the enemy under the spark.
	push si
	push ax
	mov al, [si + enemy.spawn_timer_and_animation]	; al = current animation frame
	add al, [si + enemy.facing]
	xor ah, ah
	shl ax, 1
	mov si, [si + enemy.animation_frames_ptr]
	add si, ax
	mov si, [si]	; enemy.animation_frames_ptr[enemy.spawn_timer_and_animation + enemy.facing]
	mov bp, si
	add bp, 128	; address of mask
	mov cx, 16	; number of rows in graphic
	pop ax		; ah = y-coordinate, al = camera-relative x-coordinate
	push ax
	call blit_16xH_with_mask_playfield_offscreen
	pop ax
	pop si

.blit_spark:
	push si
	push ax
	mov al, [si + enemy.state]
	xor ah, ah
	sub al, 2	; subtract 2 from the animation counter to 0-index ANIMATION_TABLE_SPARKS
	shl ax, 1
	lea si, [ANIMATION_TABLE_SPARKS]
	add si, ax
	mov si, [si]	; ANIMATION_TABLE_SPARKS[enemy.state - 2]
	mov bp, si
	add bp, 128	; address of mask
	mov cx, 16	; number of rows in graphic
	pop ax		; ah = y-coordinate, al = camera-relative x-coordinate
	call blit_16xH_with_mask_playfield_offscreen
	pop si

	inc byte [si + enemy.state]	; advance the spark animation counter
	jmp .enemy_loop_next

.despawn:
	mov byte [si + enemy.state], ENEMY_STATE_DESPAWNED
	; Schedule the enemy slot to respawn after the number of ticks in
	; enemy_respawn_counter_cycle.
	mov al, [enemy_respawn_counter_cycle]
	mov [si + enemy.spawn_timer_and_animation], al	; initialize respawn counter
	; Advance enemy_respawn_counter_cycle.
	add al, 20
	cmp al, 120	; hit the top of the cycle?
	jl .l4
	mov al, 20	; wrap around to the bottom
.l4:
	mov [enemy_respawn_counter_cycle], al
	jmp .enemy_loop_next

; AI for ENEMY_BEHAVIOR_LEAP: like Bug-eyes, Blind Toad, Beach Ball. Jump
; towards Comic, under low gravity. Bounce off the top, left, and right edges
; of the playfield, and fall off the bottom. ENEMY_BEHAVIOR_LEAP always has
; enemy.facing == ENEMY_FACING_RIGHT, so it requires the enemy graphic to look
; good whether moving right or left (which is why Bug-eyes and Blind Toad look
; towards the camera).
enemy_behavior_leap:
	mov bl, byte [si + enemy.y_vel]
	or bl, bl	; is enemy.y_vel positive, negative, or zero?
	jl .moving_up	; enemy.y_vel < 0
	jg .moving_down	; enemy.y_vel > 0

	; enemy.y_vel == 0. This means that it is either at the top of its
	; leap, or on the ground before making another leap. Check for solid
	; ground beneath the enemy.
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	add al, 2	; 2 units down: just beneath the enemy
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 2) (and the one to its right, if enemy.x is odd)
	jc .l1		; the tile is solid
	jmp .start_falling	; the tile is not solid
.l1:
	jmp .begin_leap

.moving_up:
	; enemy.y_vel < 0
	mov dl, bl	; dl = enemy.y_vel
	sar bl, 1
	sar bl, 1
	sar bl, 1	; enemy.y_vel / 8
	neg bl		; -enemy.y_vel / 8
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	; I'm not sure why this does `neg`, `sub` instead of just `add`.
	sub al, bl	; enemy.y + enemy.y_vel/8
	jb .undo_position_change	; hit the top of the playfield
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + enemy.y_vel/8) (and the one to its right, if enemy.x is odd)
	jc .undo_position_change	; hit a solid map tile
	jmp .apply_gravity

.moving_down:
	; enemy.y_vel > 0
	mov dl, bl	; dl = enemy.y_vel
	sar bl, 1
	sar bl, 1
	sar bl, 1	; enemy.y_vel / 8
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	add al, bl	; enemy.y + enemy.y_vel/8
	cmp al, PLAYFIELD_HEIGHT - 2	; is the enemy above the bottom of the playfield?
	jl .keep_moving_down

	; The enemy is touching the bottom of the playfield or below it. Depawn
	; the enemy.
	mov byte [si + enemy.state], ENEMY_STATE_WHITE_SPARK + 5	; kill enemy, but without a spark animation
	mov al, PLAYFIELD_HEIGHT - 2	; clamp to the bottom of the playfield (don't overlap it)
	mov [si + enemy.y], ax
	jmp handle_enemies.check_despawn

.keep_moving_down:
	inc al
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + enemy.y_vel/8 + 1) (and the one to its right, if enemy.x is odd)
	dec al
	jnc .apply_gravity	; the tile is not solid

.start_falling:
	mov dl, [si + enemy.y_vel]
.undo_position_change:
	; Restore the enemy position in (ah, al), undoing any changes due to
	; velocity.
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x

.apply_gravity:
	; dl == enemy.y_vel. Gravity applies on each tick, regardless of
	; enemy.restraint.
	add dl, 2	; gravity is 2 units per tick per tick
	cmp dl, TERMINAL_VELOCITY + 1	; clamp to terminal velocity
	jl .l2
	mov dl, TERMINAL_VELOCITY
.l2:
	mov [si + enemy.y_vel], dl	; store updated enemy.y_vel

	; Should the enemy skip this tick?
	cmp byte [si + enemy.restraint], ENEMY_RESTRAINT_SKIP_THIS_TICK
	je .skip_this_tick

	; If the enemy is a slow enemy, transition from
	; ENEMY_RESTRAINT_MOVE_THIS_TICK to ENEMY_RESTRAINT_SKIP_THIS_TICK. If
	; the enemy is a fast enemy, increment enemy.restraint until it
	; overflows to ENEMY_RESTRAINT_MOVE_THIS_TICK and the enemy becomes a
	; slow enemy (which is a bug).
	inc byte [si + enemy.restraint]

	mov dx, [camera_x]
	cmp byte [si + enemy.x_vel], +1	; moving right?
	jne .moving_left

.moving_right:
	; Unlike in .moving_left, here there is no check for collision with the
	; edge of the map.
	add ah, 2
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x + 2, enemy.y) (and the one below it, if enemy.y is odd)
	dec ah		; provisionally set enemy.x = enemy.x + 1
	jnc .check_playfield_right	; no collision, keep the enemy.x update
	dec ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_left	; and bounce back to the left
.check_playfield_right:
	mov bh, ah	; bh = enemy.x
	sub bh, dl	; camera-relative x-coordinate
	jb .check_for_ground		; off the left side of the playfield (not sure why this check is done, it should be redundant with the next one)
	cmp bh, PLAYFIELD_WIDTH - 2	; touching the right edge of the playfield?
	jb .check_for_ground		; not at the right edge of the playfield, carry on
.bounce_left:
	; Hit a solid tile, or the right edge of the playfield, while moving
	; right. Bounce back to the left.
	mov byte [si + enemy.x_vel], -1
	jmp .check_for_ground

.moving_left:
	or ah, ah	; enemy.x == 0? (Enemy is at the left edge of the map?)
	jz .bounce_right
	dec ah		; provisionally set enemy.x = enemy.x - 1
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x - 1, enemy.y) (and the one below it, if enemy.y is odd)
	jnc .check_playfield_left	; no collision, keep the enemy.x update
	inc ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_right	; and bounce back to the right
.check_playfield_left:
	mov bh, ah	; bh = enemy.x
	cmp bh, dl	; enemy.x > camera_x?
	ja .check_for_ground
.bounce_right:
	; Hit a solid tile, or the right edge of the playfield, while moving
	; right. Bounce back to the left.
	mov byte [si + enemy.x_vel], +1
	jmp .check_for_ground

.skip_this_tick:
	; This is a slow enemy that just skipped a tick. Move the next tick.
	mov byte [si + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK

.check_for_ground:
	; Check vertical collision.
	mov dx, ax
	cmp byte [si + enemy.y_vel], 0
	jle .goto_done	; still moving up
	add al, 3	; check the tile below the enemy
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 3) (and the one to its right, if enemy.x is odd)
	jc .landed	; the tile is solid

.goto_done:
	jmp .done

.landed:
	inc dl
	and dl, 0xfe	; clamp to an even tile boundary
	mov byte [si + enemy.y_vel], 0	; reset vertical velocity
	jmp .done

.begin_leap:
	mov ax, [comic_y]	; al = comic_y, ah = comic_x
	mov ch, +1	; ch will become enemy.x_vel; initially assume moving right
	mov dx, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	cmp ah, dh	; is enemy to the left or right of Comic?
	jae .l3		; enemy.x < comic_x, keep ch = +1
	mov ch, -1	; enemy.x >= comic_x, set ch = -1
.l3:
	mov [si + enemy.x_vel], ch	; store computed enemy.x
	mov byte [si + enemy.y_vel], -7	; initial leap velocity

.done:
	mov [si + enemy.y], dx	; store updated position
	jmp handle_enemies.check_despawn

; AI for ENEMY_BEHAVIOR_BOUNCE: Fire Ball, Brave Bird, Space Pollen, Saucer,
; Spark, Atom, F.W. Bros. Move diagonally and bounce off solid map tiles and
; the edges of the playfield.
enemy_behavior_bounce:
	mov dx, [camera_x]
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x

	; Should the enemy skip this tick?
	cmp byte [si + enemy.restraint], ENEMY_RESTRAINT_SKIP_THIS_TICK
	jg .horizontal	; ENEMY_RESTRAINT_MOVE_EVERY_TICK
	jne .move_this_tick	; ENEMY_RESTRAINT_MOVE_THIS_TICK
	jmp .skip_this_tick	; ENEMY_RESTRAINT_SKIP_THIS_TICK

.move_this_tick:
	; This enemy is a slow enemy in ENEMY_RESTRAINT_MOVE_THIS_TICK state.
	; Transition to ENEMY_RESTRAINT_SKIP_THIS_TICK.
	inc byte [si + enemy.restraint]

.horizontal:
	cmp byte [si + enemy.x_vel], +1	; moving left or right?
	jne .moving_left

.moving_right:
	mov byte [si + enemy.facing], ENEMY_FACING_RIGHT
	add ah, 2
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x + 2, enemy.y) (and the one below it, if enemy.y is odd)
	dec ah		; provisionally set enemy.x = enemy.x + 1
	jnc .check_playfield_right	; no collision, keep the enemy.x update
	dec ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_left	; and bounce back to the left
.check_playfield_right:
	mov bh, ah
	sub bh, dl	; enemy.x - camera_x
	jb .vertical	; off the left side of the playfield, keep moving right
	cmp bh, PLAYFIELD_WIDTH - 2
	jb .vertical	; left of the right side of the playfield, keep moving right
.bounce_left:
	; We hit a solid tile or the right edge of the playfield, while moving
	; right.
	mov byte [si + enemy.x_vel], -1	; bounce back to the left
	jmp .vertical

.moving_left:
	mov byte [si + enemy.facing], ENEMY_FACING_LEFT
	or ah, ah	; hit the left side of the map?
	jz .bounce_right
	dec ah		; provisionally set enemy.x = enemy.x - 1
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x - 1, enemy.y) (and the one below it, if enemy.y is odd)
	jnc .check_playfield_left	; no collision, keep the enemy.x update
	inc ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_right	; and bounce back to the right
.check_playfield_left:
	mov bh, ah	; bh = enemy.x
	cmp bh, dl	; enemy.x > camera_x?
	ja .vertical
.bounce_right:
	mov byte [si + enemy.x_vel], +1	; bounce back to the right

.vertical:
	cmp byte [si + enemy.y_vel], +1	; moving down or up?
	jne .moving_up

.moving_down:
	cmp al, PLAYFIELD_HEIGHT - 2	; hit the bottom of the playfield?
	jge .bounce_up
	add al, 2
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 2) (and the one to its right, if enemy.x is odd)
	dec al		; provisionally set enemy.y = enemy.y + 1
	jnc .check_playfield_bottom	; no collision, keep the enemy.y update
	dec al		; hit a solid map tile, undo the enemy.y update
	jmp .bounce_up	; and bounce back up
.check_playfield_bottom:
	; Is the bottom of the enemy touching the bottom of the playfield?
	; Don't let the enemy cross the bottom edge, where we would have to
	; worry about overflowing the map boundary in map collision checks.
	cmp al, PLAYFIELD_HEIGHT - 2
	jl .done
.bounce_up:
	mov byte [si + enemy.y_vel], -1	; bounce back up
	jmp .done

.moving_up:
	or al, al	; hit the top of the playfield?
	jz .bounce_down

	dec al
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y - 1) (and the one to its right, if enemy.x is odd)
	jnc .check_playfield_top	; no collision, keep the enemy.y update
	inc al		; hit a solid map tile, undo the enemy.y update
	jmp .bounce_down
.check_playfield_top:
	or al, al	; hit the top of the playfield?
	jnz .done
.bounce_down:
	mov byte [si + enemy.y_vel], +1	; bounce back down
	jmp .done

.skip_this_tick:
	; This is a slow enemy that just skipped a tick; move the next tick.
	mov byte [si + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK

.done:
	mov [si + enemy.y], ax	; store updated position
	jmp handle_enemies.check_despawn

; AI for ENEMY_BEHAVIOR_ROLL: Glow Globe. Roll towards Comic when there is
; ground, fall when there is no ground.
enemy_behavior_roll:
	mov ch, [si + enemy.y_vel]
	or ch, ch	; falling downward?
	jg .falling
	jmp .rolling
	nop		; dead code

.falling:
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	inc al
	cmp al, PLAYFIELD_HEIGHT - 2 - 1	; close to the bottom of the playfield?
	jl .goto_horizontal	; cannot change direction while falling

	mov byte [si + enemy.state], ENEMY_STATE_WHITE_SPARK + 5	; kill enemy, but without a spark animation
	mov al, PLAYFIELD_HEIGHT - 2	; clamp to the bottom of the playfield (don't overlap it)
	mov word [si + enemy.y], ax
	jmp handle_enemies.check_despawn

.goto_horizontal:
	jmp .horizontal

.rolling:
	mov dx, [comic_y]	; dl = comic_y, dh = comic_x
	mov ch, +1	; ch will become enemy.x_vel; initially assume moving right
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x
	cmp dh, ah	; is enemy to the left or right of Comic?
	je .stay	; enemy.x == comic_x, don't move left or right
	jae .store_x_vel	; enemy.x > comic_x, keep ch = +1
	mov ch, -1	; enemy.x < comic_x, set ch = -1
	jmp .store_x_vel

.stay:
	xor ch, ch	; enemy.x_vel = 0

.store_x_vel:
	mov byte [si + enemy.x_vel], ch

.horizontal:
	; Should the enemy skip this tick?
	cmp byte [si + enemy.restraint], ENEMY_RESTRAINT_SKIP_THIS_TICK
	je .skip_this_tick

	; If the enemy is a slow enemy, transition from
	; ENEMY_RESTRAINT_MOVE_THIS_TICK to ENEMY_RESTRAINT_SKIP_THIS_TICK. If
	; the enemy is a fast enemy, increment enemy.restraint until it
	; overflows to ENEMY_RESTRAINT_MOVE_THIS_TICK and the enemy becomes a
	; slow enemy (which is a bug).
	inc byte [si + enemy.restraint]

	mov dx, [camera_x]	; useless instruction

	cmp byte [si + enemy.x_vel], 0	; is enemy.x_vel positive, negative, or zero?
	; Reuse the .skip_this_tick label for the case where the enemy is not
	; moving horizontally. This has the side effect of changing a fast
	; enemy into a slow enemy (which is a bug).
	je .skip_this_tick	; enemy.x_vel == 0
	jl .moving_left	; enemy.x_vel < 0

.moving_right:
	add ah, 2
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x + 2, enemy.y) (and the one below it, if enemy.y is odd)
	dec ah		; provisionally set enemy.x = enemy.x + 1
	jnc .vertical	; no collision, keep the enemy.x update
	dec ah		; hit a solid map tile, undo the enemy.x update
	jmp .vertical

.moving_left:
	dec ah		; provisionally set enemy.x = enemy.x - 1
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x - 1, enemy.y) (and the one below it, if enemy.y is odd)
	jnc .vertical	; no collision, keep the enemy.x update
	inc ah		; hit a solid map tile, undo the enemy.x update
	jmp .vertical

.skip_this_tick:
	; This is either a slow enemy that just skipped a tick, or an enemy
	; that didn't move this tick because it was directly above or below
	; Comic. Move the next tick. If it was a fast enemy, it becomes a slow
	; enemy here.
	mov byte [si + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK

.vertical:
	add al, 3	; check the tile below the enemy
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 3) (and the one to its right, if enemy.x is odd)
	jc .landed	; there was collision

	sub al, 3	; undo change from the collision check
	mov byte [si + enemy.y_vel], 1	; no ground, start falling
	jmp .done
	nop		; dead code

.landed:
	sub al, 3	; undo change from the collision check
	cmp byte [si + enemy.y_vel], 0	; were we falling?
	je .done	; we were not falling

	inc al
	and al, 0xfe	; clamp to an even tile boundary
	mov byte [si + enemy.y_vel], 0	; no longer falling

.done:
	mov word [si + enemy.y], ax	; store updated position
	jmp handle_enemies.check_despawn

; AI for ENEMY_BEHAVIOR_SEEK: Killer Bee. First move horizontally to match
; Comic's x-coordinate, then move vertically to match his y-coordinate.
enemy_behavior_seek:
	mov dx, [comic_y]	; dl = comic_y, dh = comic_x
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x

	; Should the enemy skip this tick?
	cmp byte [si + enemy.restraint], ENEMY_RESTRAINT_SKIP_THIS_TICK
	je .skip_this_tick

	; If the enemy is a slow enemy, transition from
	; ENEMY_RESTRAINT_MOVE_THIS_TICK to ENEMY_RESTRAINT_SKIP_THIS_TICK. If
	; the enemy is a fast enemy, increment enemy.restraint until it
	; overflows to ENEMY_RESTRAINT_MOVE_THIS_TICK and the enemy becomes a
	; slow enemy (which is a bug).
	inc byte [si + enemy.restraint]

.horizontal:
	mov bx, ax	; bl = enemy.y, bh = enemy.x
	sub bh, dh	; is the enemy to the left or the right of Comic?
	je .vertical	; enemy is directly above or below Comic
	jae .moving_left	; enemy.x > comic_x

.moving_right:
	mov byte [si + enemy.facing], ENEMY_FACING_RIGHT
	add ah, 2
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x + 2, enemy.y) (and the one below it, if enemy.y is odd)
	dec ah		; provisionally set enemy.x = enemy.x + 1
	jnc .done	; no collision, keep the enemy.x update
	dec ah		; hit a solid map tile, undo the enemy.x update
	jmp .vertical	; cannot move right, try moving vertically

.moving_left:
	mov byte [si + enemy.facing], ENEMY_FACING_LEFT
	dec ah		; provisionally set enemy.x = enemy.x - 1
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x - 1, enemy.y) (and the one below it, if enemy.y is odd)
	jnc .done	; no collision, keep the enemy.x update
	inc ah		; hit a solid map tile, undo the enemy.x update
	jmp .vertical	; cannot move left, try moving vertically

.skip_this_tick:
	; This is a slow enemy that just skipped a tick. Move the next tick.
	; The .skip_this_tick label is also used for the case where the enemy
	; is not moving horizontally (.vertical, .moving_down, .moving_up
	; below). This has the side effect of changing a fast enemy into a slow
	; enemy (which is a bug).
	mov byte [si + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK

.done:
	mov [si + enemy.y], ax	; store updated position
	jmp handle_enemies.check_despawn

.vertical:
	mov bx, ax	; bl = enemy.y, bh = enemy.x
	sub bl, dl	; is the enemy above or below Comic?
	je .skip_this_tick	; enemy is directly on top of Comic's head
	jae .moving_up	; enemy.y >= comic_y

.moving_down:
	add al, 2
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 2) (and the one to its right, if enemy.x is odd)
	dec al		; provisionally set enemy.y = enemy.y + 1
	jnc .done	; no collision, keep the enemy.y update
	dec al		; hit a solid map tile, undo the enemy.y update
	jmp .skip_this_tick

.moving_up:
	dec al		; provisionally set enemy.y = enemy.y - 1
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y - 1) (and the one to its right, if enemy.x is odd)
	jnc .done	; no collision, keep the enemy.y update
	inc al		; hit a solid map tile, undo the enemy.y update
	jmp .skip_this_tick

; AI for ENEMY_BEHAVIOR_SHY enemies: Shy Bird, Spinner. Move diagonally,
; bouncing off the left and right edges of the playfield. Vertical movement
; depends on which way Comic is facing. When Comic is facing towards the enemy,
; fly upward and clip against the top of the playfield. When Comic is facing
; away from the enemy, fly in whatever direction gets closer to Comic: down if
; above him; up if below.
enemy_behavior_shy:
	mov dx, [camera_x]
	mov ax, [si + enemy.y]	; al = enemy.y, ah = enemy.x

	; Should the enemy skip this tick?
	cmp byte [si + enemy.restraint], ENEMY_RESTRAINT_SKIP_THIS_TICK
	jg .horizontal	; ENEMY_RESTRAINT_MOVE_EVERY_TICK
	jne .move_this_tick	; ENEMY_RESTRAINT_MOVE_THIS_TICK
	jmp .skip_this_tick	; ENEMY_RESTRAINT_SKIP_THIS_TICK

.move_this_tick:
	; This enemy is a slow enemy in ENEMY_RESTRAINT_MOVE_THIS_TICK state.
	; Transition to ENEMY_RESTRAINT_SKIP_THIS_TICK.
	inc byte [si + enemy.restraint]

.horizontal:
	cmp byte [si + enemy.x_vel], +1	; is the enemy moving left or right?
	jne .moving_left

.moving_right:
	mov byte [si + enemy.facing], ENEMY_FACING_RIGHT
	add ah, 2
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x + 2, enemy.y) (and the one below it, if enemy.y is odd)
	dec ah		; provisionally set enemy.x = enemy.x + 1
	jnc .check_playfield_right	; no collision, keep the enemy.x update
	dec ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_left	; and bounce back to the left
.check_playfield_right:
	mov bh, ah
	sub bh, dl	; enemy.x - camera_x
	jb .vertical	; off the left side of the playfield, keep moving right
	cmp bh, PLAYFIELD_WIDTH - 2
	jb .vertical	; left of the right side of the playfield, keep moving right
.bounce_left:
	; We hit a solid tile or the right edge of the playfield, while moving
	; right.
	mov byte [si + enemy.x_vel], -1	; bounce back to the left
	jmp .vertical

.moving_left:
	mov byte [si + enemy.facing], ENEMY_FACING_LEFT
	or ah, ah	; hit the left side of the map?
	jz .bounce_right
	dec ah		; provisionally set enemy.x = enemy.x - 1
	call check_horizontal_enemy_map_collision	; check the solidity of the tile at (enemy.x - 1, enemy.y) (and the one below it, if enemy.y is odd)
	jnc .check_playfield_left	; no collision, keep the enemy.x update
	inc ah		; hit a solid map tile, undo the enemy.x update
	jmp .bounce_right	; and bounce back to the right
.check_playfield_left:
	mov bh, ah	; bh = enemy.x
	cmp bh, dl	; enemy.x > camera_x?
	ja .vertical
.bounce_right:
	mov byte [si + enemy.x_vel], +1	; bounce back to the right

.vertical:
	; Vertical movement depends on whether Comic is facing the enemy or
	; not. Facing towards means move up; facing away means move down.
	mov dx, [comic_y]	; dl = comic_y, dh = comic_x
	sub dh, ah	; is the enemy to the left or right of Comic?
	je .comic_facing_away	; enemy.x == comic_x, consider that facing away
	jb .right_of_comic	; enemy.x > comic_x

.left_of_comic:
	cmp byte [comic_facing], COMIC_FACING_LEFT
	jne .comic_facing_away	; Comic is facing right, away from enemy
	jmp .moving_up	; Comic is facing left, towards enemy

.right_of_comic:
	cmp byte [comic_facing], COMIC_FACING_RIGHT
	jne .comic_facing_away	; Comic is facing left, away from enemy
	jmp .moving_up	; Comic is facing right, towards enemy

.comic_facing_away:
	; Move vertically to get closer to Comic: down if above Comic; up if
	; below.
	sub dl, al	; is the enemy above or below Comic?
	je .done	; enemy.y == comic_y
	jb .moving_up	; enemy.y > comic_y

.moving_down:
	cmp al, PLAYFIELD_HEIGHT - 2	; hit the bottom of the playfield?
	jge .bounce_up

	add al, 2
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y + 2) (and the one to its right, if enemy.x is odd)
	dec al		; provisionally set enemy.y = enemy.y + 1
	jnc .check_playfield_bottom	; no collision, keep the enemy.y update
	dec al		; hit a solid map tile, undo the enemy.y update
	jmp .bounce_up	; and bounce back up
.check_playfield_bottom:
	; Is the bottom of the enemy touching the bottom of the playfield?
	; Don't let the enemy cross the bottom edge, where we would have to
	; worry about overflowing the map boundary in map collision checks.
	cmp al, PLAYFIELD_HEIGHT - 2
	jl .done
.bounce_up:
	mov byte [si + enemy.y_vel], -1	; bounce back up
	jmp .done

.moving_up:
	or al, al	; hit the top of the playfield?
	jz .bounce_down

	dec al
	call check_vertical_enemy_map_collision	; check the solidity of the tile at (enemy.x, enemy.y - 1) (and the one to its right, if enemy.x is odd)
	jnc .check_playfield_top	; no collision, keep the enemy.y update
	inc al		; hit a solid map tile, undo the enemy.y update
	jmp .bounce_down
.check_playfield_top:
	or al, al	; hit the top of the playfield?
	jnz .done
.bounce_down:
	mov byte [si + enemy.y_vel], +1	; bounce back down
	jmp .done

.skip_this_tick:
	; This is a slow enemy that just skipped a tick; move the next tick.
	mov byte [si + enemy.restraint], ENEMY_RESTRAINT_MOVE_THIS_TICK

.done:
	mov word [si], ax	; store enemy.y and enemy.x
	jmp handle_enemies.check_despawn

; Spawn an enemy in an enemy slot, if it's a used slot, enemy.behavior !=
; ENEMY_BEHAVIOR_UNUSED, and there's a place to spawn it. Jump back to
; handle_enemies.enemy_loop_next when finished.
; Input:
;   si = pointer to enemy structure
;   enemy_spawned_this_tick = spawn an enemy only if this is 0
;   comic_facing, camera_x = enemies are spawned outside the playfield in the direction Comic is facing
;   comic_y = enemies spawn at Comic's foot level or higher
;   enemy_respawn_position_cycle = minus 20, the number of units outside the playfield to spawn the enemy
; Output:
;   si = filled in
;   enemy_spawned_this_tick = 1 if the enemy slot is used, 0 if unused
;   enemy_respawn_position_cycle = cycled to the next element in the sequence 24, 26, 28, 30, 24, ...
maybe_spawn_enemy:
	mov byte [si + enemy.spawn_timer_and_animation], 0
	cmp byte [enemy_spawned_this_tick], 0	; has an enemy already been spawned this tick?
	jne .goto_done	; if so, don't spawn any more this tick

.find_horizontal_position:
	mov ax, [camera_x]
	; Advance enemy_respawn_position_cycle. This variable determines how
	; far outside the playfield the enemy is spawned: 0, 2, 4, or 6 units.
	mov ah, [enemy_respawn_position_cycle]
	add ah, 2
	cmp ah, PLAYFIELD_WIDTH + 7	; hit the top of the cycle?
	jb .l1
	mov ah, PLAYFIELD_WIDTH	; wrap around to the bottom
.l1:
	mov [enemy_respawn_position_cycle], ah

	; Spawn the enemy in whatever direction Comic is facing.
	cmp byte [comic_facing], COMIC_FACING_RIGHT
	je .try_spawn_right

.try_spawn_left:
	; If facing left, flip the position around to the left side of the
	; playfield.
	sub ah, PLAYFIELD_WIDTH - 2
	sub al, ah	; camera_x + (PLAYFIELD_WIDTH - 2) - enemy_respawn_position_cycle
	jae .find_vertical_position	; spawn only if the computed horizontal position is 0 or greater

.goto_done:
	jmp .done

.try_spawn_right:
	add al, ah	; camera_x + enemy_respawn_position_cycle
	; This next check looks like it's trying to prevent spawning when the
	; computed exceeds 255; i.e., it is past the right edge of the map. But
	; it doesn't do that: instead it prevents spawning when the camera
	; position is less than or equal to 127 and the computed horizontal
	; position is 128 or greater. The bug is treating the numbers as signed
	; integers, not unsigned. The instruction was probably meant to be
	; `jb`/`jc` (for unsigned integers), not `jo` (for signed integers).
	; The effect of the bug is that there's a safe area in the middle of
	; each stage, in which enemies will never spawn, as long as Comic is
	; facing right. Enemies will spawn off the right edge of the map, which
	; actually puts them near the far left of the map, where they will
	; despawn on the next tick because they are outside the
	; ENEMY_DESPAWN_RADIUS. You can see this in certain stages by shooting
	; at the right edge of the map; occasionally you will hear the sound of
	; an enemy being hit by a fireball.
	jo .done

.find_vertical_position:
	; We know the horizontal position at which we want to spawn; now find a
	; vertical position. We look for a non-solid tile above a solid tile.
	; The logic here is similar to that in begin_teleport, but simpler
	; because we are only looking for one non-solid tile, not two
	; consecutive.
	mov ah, al	; ah = horizontal position
	mov al, [comic_y]
	and al, 0xfe	; clamp to an even tile boundary
	add al, 4	; start looking for a vertical position at the level of Comic's feet, rounded towards the top of the playfield

	mov cl, [tileset_last_passable]
	call address_of_tile_at_coordinates	; bx = address of the tile ID we're currently looking at

	; First look for a solid tile.
.find_solid_loop:
	cmp [bx], cl	; is the tile we're looking at solid?
	jg .find_nonsolid_loop_start	; found a solid tile, now look for an empty space above it
.find_solid_loop_next:
	sub bx, MAP_WIDTH_TILES	; next tile up in the map
	sub al, 2	; next tile up in y-coordinate
	jnz .find_solid_loop	; keep looking for a solid tile until we reach the top of the map
	jmp handle_enemies.enemy_loop_next	; no solid tiles; give up on spawning this enemy

	; Now look for a non-solid tiles.
.find_nonsolid_loop:
	cmp [bx], cl	; is the tile solid?
	jle .spawn	; not solid, this is the spot
.find_nonsolid_loop_start:
	sub bx, MAP_WIDTH_TILES	; next tile up in the map
	sub al, 2	; next tile up in y-coordinate
	jae .find_nonsolid_loop	; keep looking for a non-solid tile until we reach the top of the map
	jmp handle_enemies.enemy_loop_next	; no non-solid tiles; give up on spawning this enemy

.spawn:
	mov byte [enemy_spawned_this_tick], 1
	mov [si + enemy.y], ax	; initialize enemy.y and enemy.x
	mov byte [si + enemy.state], ENEMY_STATE_SPAWNED
	cmp byte [si + enemy.behavior], ENEMY_BEHAVIOR_BOUNCE
	je .finish_spawn_bounce_shy
	cmp byte [si + enemy.behavior], ENEMY_BEHAVIOR_SHY
	je .finish_spawn_bounce_shy
	cmp byte [si + enemy.behavior], ENEMY_BEHAVIOR_UNUSED
	jne .finish_spawn_leap_roll_seek

	; We tried to spawn an unused enemy slot (enemy.behavior ==
	; ENEMY_BEHAVIOR_UNUSED). Undo the spawn and reset the counter.
	mov byte [enemy_spawned_this_tick], 0
	mov byte [si + enemy.state], ENEMY_STATE_DESPAWNED
	mov byte [si + enemy.spawn_timer_and_animation], 100	; try this slot again after 100 ticks
	jmp handle_enemies.enemy_loop_next

.finish_spawn_bounce_shy:
	; ENEMY_BEHAVIOR_BOUNCE and ENEMY_BEHAVIOR_SHY start out moving up and
	; left.
	mov byte [si + enemy.x_vel], -1
	mov byte [si + enemy.y_vel], -1
	jmp handle_enemies.enemy_loop_next

.finish_spawn_leap_roll_seek:
	; ENEMY_BEHAVIOR_LEAP, ENEMY_BEHAVIOR_ROLL, and ENEMY_BEHAVIOR_SEEK
	; start out motionless.
	mov byte [si + enemy.x_vel], 0
	mov byte [si + enemy.y_vel], 0

.done:
	jmp handle_enemies.enemy_loop_next

; Shoot a fireball, if there's an open fireball slot in the first
; comic_firepower slots.
; Input:
;   ah = x-coordinate
;   al = y-coordinate
;   comic_firepower = number of fireball slots unlocked by Blastola Cola
;   comic_facing = which direction to shoot the fireball
try_to_fire:
	mov cl, [comic_firepower]
	or cl, cl	; comic_firepower == 0?
	jz .done	; if so, we're done

	; Look for an open fireball slot.
	xor ch, ch	; cx = comic_firepower
	lea si, [fireballs]	; for each fireball slot
.loop:
	; Is this slot open?
	cmp word [si + fireball.y], (FIREBALL_DEAD<<8)|FIREBALL_DEAD	; check fireball.y and fireball.x simultaneously
	jne .loop_next

	; Found an open slot. Assign x- and y-coordinates.
	mov [si + fireball.y], ax

	; Velocity depends on which way Comic is facing.
	mov al, +2	; velocity is +2
	cmp byte [comic_facing], COMIC_FACING_RIGHT
	je .store_vel
	mov al, -2	; unless facing left, then velocity is -2
.store_vel:
	mov [si + fireball.vel], al

	; Assign phase.
	mov byte [si + fireball.corkscrew_phase], 2

	mov ax, SOUND_PLAY
	lea bx, [SOUND_FIRE]
	mov cx, 0	; priority 0
	int3
	ret

.loop_next:
	add si, fireball_size	; next fireball slot
	loop .loop	; until reaching comic_firepower

.done:
	ret

; Handle fireball movement for one game tick. Move each active fireball
; horizontally according to fireball.vel, and vertically according to
; fireball.corkscrew_phase. Despawn fireballs that exit the playfield. Check
; for collision between fireballs and enemies.
; Input:
;   comic_firepower = maximum number of fireballs there may be
;   camera_x = fireballs that exit the playfield are despawned
; Output:
;   fireballs = colliding fireballs are put in FIREBALL_DEAD state
;   enemies = colliding enemies are put in ENEMY_STATE_WHITE_SPARK state
;   score = increased by 300 points for every collision found
handle_fireballs:
	mov cl, [comic_firepower]
	or cl, cl	; comic_firepower == 0?
	jnz .nonzero_firepower
	ret		; no fireball slots means nothing to do

.nonzero_firepower:
	xor ch, ch	; cx = comic_firepower
	lea si, [fireballs]	; for each fireball slot
.movement_loop:
	push cx
	; Is this slot active?
	mov ax, [si + fireball.y]	; al = fireball.y, ah = fireball.x
	cmp ax, (FIREBALL_DEAD<<8)|FIREBALL_DEAD	; check fireball.y and fireball.x simultaneously
	je .movement_loop_next	; this fireball is not active

	mov dl, [si + fireball.vel]
	or dl, dl	; is fireball.vel positive or negative?
	jg .moving_right
.moving_left:
	; I'm not sure why this does `neg`, `sub` here instead of just `add`.
	neg dl
	sub ah, dl	; fireball.x + fireball.vel
	jb .deactivate	; deactivate fireballs that go off the left edge of the map (redundant with .check_playfield_bounds below)
	jmp .check_playfield_bounds
.moving_right:
	add ah, dl	; fireball.x + fireball.vel

.check_playfield_bounds:
	mov dx, [camera_x]
	mov dh, ah
	sub dh, dl	; camera-relative x-coordinate
	jb .deactivate	; deactivate fireballs that go off the left edge of the playfield

	cmp dh, PLAYFIELD_WIDTH - 2
	jle .handle_corkscrew	; deactivate fireballs that go off the right edge of the playfield

.deactivate:
	mov word [si + fireball.y], (FIREBALL_DEAD<<8)|FIREBALL_DEAD
	jmp .movement_loop_next

.handle_corkscrew:
	cmp byte [comic_has_corkscrew], 1	; does Comic have the Corkscrew?
	jne .animate	; if not, skip adjusting the fireball's phase

	; When fireball.corkscrew_phase is 2, move the fireball 1 unit
	; downward; when the phase is 1, move the fireball 1 unit upward.
	dec byte [si + fireball.corkscrew_phase]
	jz .wrap_corkscrew_phase
	inc al		; fireball.y + 1
	jmp .animate
.wrap_corkscrew_phase:
	dec al		; fireball.y - 1
	mov byte [si + fireball.corkscrew_phase], 2

.animate:
	mov [si + fireball.y], ax	; assign fireball.y and fireball.x simultaneously

	mov al, [si + fireball.animation]
	inc al		; next animation frame
	cmp al, [si + fireball.num_animation_frames]	; hit the end of the animation?
	jne .store_animation
	xor al, al	; wrap fireball.animation when it reaches fireball.num_animation_frames
.store_animation:
	mov [si + fireball.animation], al

	; Blit the fireball's current animation frame at its current location.
	mov dx, [camera_x]
	mov ax, [si + fireball.y]	; al = fireball.y, ah = fireball.x
	sub ah, dl	; camera-relative x-coordinate
	push si
	push ax
	mov al, [si + fireball.animation]
	xor ah, ah	; ax = fireball.animation
	shl ax, 1
	lea si, [ANIMATION_TABLE_FIREBALL]
	add si, ax
	mov si, [si]	; ANIMATION_TABLE_FIREBALL[fireball.animation]
	mov bp, si
	add bp, 64	; address of mask
	mov cx, 8	; number of rows in graphic
	pop ax		; camera-relative coordinates
	call blit_16xH_with_mask_playfield_offscreen
	pop si

.movement_loop_next:
	pop cx
	add si, fireball_size	; next fireball slot
	dec cx
	jz .check_collision	; break the loop after looking at the first comic_firepower slots
	jmp .movement_loop

	; A nested loop that checks every fireball for collision with every
	; enemy.
.check_collision:
	lea si, [fireballs]
	mov cx, MAX_NUM_FIREBALLS	; for each fireball slot (not limited to comic_firepower here for some reason)
.collision_loop:
	push cx
	mov dx, [si + fireball.y]	; dl = fireball.y, dh = fireball.x
	cmp dx, (FIREBALL_DEAD<<8)|FIREBALL_DEAD	; check fireball.y and fireball.x simultaneously
	je .collision_loop_next	; this fireball is not active

	lea di, [enemies]
	mov cx, MAX_NUM_ENEMIES	; for each enemy slot
.enemy_loop:
	cmp byte [di + enemy.state], ENEMY_STATE_SPAWNED
	jne .enemy_loop_next	; this enemy is not spawned

	mov ax, [di + enemy.y]	; al = enemy.y, ah = enemy.x
	mov bx, dx	; bl = fireball.y, bh = fireball.x
	; Vertical distance must be 0 or 1.
	sub bl, al	; fireball.y - enemy.y
	jb .enemy_loop_next	; (fireball.y - enemy.y) < 0
	cmp bl, 1
	jg .enemy_loop_next	; (fireball.y - enemy.y) > 1
	; Horizontal distance must be -1, 0, or +1.
	sub bh, ah	; fireball.x - enemy.x
	jae .l1
	neg bh		; abs(fireball.x - enemy.x)
.l1:
	cmp bh, 1
	jle .collision_found	; abs(fireball.x - enemy.x) <= 1

.enemy_loop_next:
	add di, enemy_size	; next enemy slot
	loop .enemy_loop

.collision_loop_next:
	pop cx
	add si, fireball_size	; next fireball slot
	loop .collision_loop
	ret

.collision_found:
	; Found a fireball–enemy collision.
	mov byte [di + enemy.state], ENEMY_STATE_WHITE_SPARK	; start the enemy's white spark animation
	mov word [si + fireball.y], (FIREBALL_DEAD<<8)|FIREBALL_DEAD	; deactivate fireball

	push si
	push di
	mov al, 3	; 300 points for killing an enemy with a fireball
	call award_points
	pop di
	pop si

	mov ax, SOUND_PLAY
	lea bx, [SOUND_HIT_ENEMY]
	mov cx, 1	; priority 1
	int3

	jmp .collision_loop_next	; next fireball slot

; Award the player an extra life and play the extra life sound. If the player
; already has MAX_NUM_LIVES lives, refill HP and award 22500 points.
; Output:
;   comic_num_lives = incremented by 1 unless already at MAX_NUM_LIVES
;   comic_hp_pending_increase = MAX_HP if comic_num_lives was MAX_NUM_LIVES
;   score = increased by 22500 points if comic_num_lives was MAX_NUM_LIVES
award_extra_life:
	mov ax, 1	; play sound
	lea bx, [SOUND_EXTRA_LIFE]
	mov cx, 4	; priority 4
	int3

	mov al, [comic_num_lives]
	cmp al, 5	; already the maximum number of lives?
	je .points_only

	inc al		; add a life
	mov [comic_num_lives], al

	; Brighten the life icon.
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(24, 180)
	xor ah, ah
	add di, ax
	add di, ax
	add di, ax	; add 24 pixels to the x-coordinate for every life
	lea si, [GRAPHIC_LIFE_ICON]
	jmp blit_16x16	; tail call
	nop		; dead code

.points_only:
	; If already at maximum lives, refill HP and award 22500 points.
	mov byte [comic_hp_pending_increase], MAX_HP
	mov al, 75
	call award_points
	mov al, 75
	call award_points
	mov al, 75
	call award_points
	ret

; Subtract a life if comic_num_lives is greater than 0.
; Output:
;   comic_num_lives = decremented by 1 unless already at 0
lose_a_life:
	mov al, [comic_num_lives]
	cmp al, 0
	je .no_lives_left
	dec byte [comic_num_lives]	; subtract a life
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(24, 180)
	xor ah, ah
	add di, ax
	add di, ax
	add di, ax	; add 24 pixels to the x-coordinate for every life
	lea si, [GRAPHIC_LIFE_ICON]
	add si, GRAPHICS_ITEMS_ODD_FRAMES - GRAPHICS_ITEMS_EVEN_FRAMES	; the dark version of the icon is a fixed distance away
	jmp blit_16x16	; tail call
	nop		; dead code
.no_lives_left:
	ret

; Blit a 16×16 graphic to both the onscreen and offscreen video buffers,
; without a mask.
; Input:
;   ds:si = graphic (2 bytes per row, 32 bytes per plane, 128 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_16x16:
	mov ah, 1
	call blit_16x16_plane
	mov ah, 2
	call blit_16x16_plane
	mov ah, 4
	call blit_16x16_plane
	mov ah, 8
	call blit_16x16_plane
	ret

; Blit a single plane of a 16×16 graphic to both the onscreen and offscreen
; video buffers, without a mask.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   ds:si = graphic (2 bytes per row, 32 bytes per plane, 128 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_16x16_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	; Blit relative to the 0x0000 video buffer.
	mov bx, 16	; number of rows
	push si
	push di
.loop_0000:
	movsw		; copy 2 bytes from ds:si to es:di
	add di, 38	; next destination row (each row is 40 bytes; compensate for the 2 bytes we just wrote)
	dec bx
	jne .loop_0000	; for each row
	pop di
	pop si

	; Do it again with the 0x2000 video buffer.
	mov bx, 16	; number of rows
	push di
	add di, 0x2000	; now do the other buffer
.loop_2000:
	movsw		; copy 2 bytes from ds:si to es:di
	add di, 38	; next destination row (each row is 40 bytes; compensate for the 2 bytes we just wrote)
	dec bx
	jne .loop_2000
	pop di

	ret

; Add 1 unit to comic_hp, unless comic_hp is already at MAX_HP, in which case
; award 1800 points.
; Output:
;   comic_hp = incremented by 1 unless already at MAX_HP
;   score = increased by 1800 point if comic_hp was MAX_HP
increment_comic_hp:
	mov al, [comic_hp]
	cmp al, MAX_HP
	je .overcharge

	inc al		; increment HP
	mov [comic_hp], al

	; Add a unit to the HP meter in the UI.
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(240, 82)
	xor ah, ah
	add di, ax	; add 8 pixels to the x-coordinate for each unit of HP
	lea si, [GRAPHIC_METER_FULL]
	jmp blit_8x16	; tail call

.overcharge:
	; HP is already full.
	mov al, 18	; 1800 points for every unit of overcharge
	call award_points
	ret

; Play SOUND_DAMAGE and subtract 1 unit from comic_hp, unless already at 0.
; Output:
;   comic_hp = decremented by 1 unless already at 0
decrement_comic_hp:
	lea bx, [SOUND_DAMAGE]
	mov ax, SOUND_PLAY
	mov cx, 2	; priority 2
	int3

	mov al, [comic_hp]
	cmp al, 0
	je .done	; no HP remaining to take away

	; Remove a unit from the HP meter in the UI.
	dec byte [comic_hp]	; decrement HP
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(240, 82)
	xor ah, ah
	add di, ax	; add 8 pixels to the x-coordinate for each unit of HP
	lea si, [GRAPHIC_METER_EMPTY]
	jmp blit_8x16	; tail call
	nop		; dead code

.done:
	ret

; Add 1 unit to fireball_meter, unless already at MAX_FIREBALL_METER.
; Output:
;   fireball_meter = incremented by 1 unless already at MAX_FIREBALL_METER
increment_fireball_meter:
	mov al, [fireball_meter]
	cmp al, MAX_FIREBALL_METER
	je .done

	inc al		; increment the meter
	mov [fireball_meter], al

	; The fireball meter is conceptually made up to 6 cells, each of which
	; may be empty, half-full, or full. A half-full cell represents 1 unit
	; and a full cell represents 2. When the meter increments from even to
	; odd, we advance a cell from empty to half-full. When the meter
	; increments from odd to even, we advance a cell from half-full to
	; full.
	mov ah, al
	inc al
	shr al, 1	; number of cells used (final one may be half-full or full)

	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(240, 54)
	; If the meter is odd, draw the final cell as GRAPHIC_METER_HALF. If
	; the meter is even, draw the final cell as GRAPHIC_METER_FULL.
	lea si, [GRAPHIC_METER_HALF]	; initially assume odd
	test ah, 1	; check least significant bit
	jnz .l1
	lea si, [GRAPHIC_METER_FULL]	; no, it was even
.l1:
	xor ah, ah
	add di, ax	; add 8 pixels to the x-coordinate for each used cell
	jmp blit_8x16	; tail call
	nop		; dead code

.done:
	ret

; Subtract 1 unit from fireball_meter, unless already at MAX_FIREBALL_METER.
; Output:
;   fireball_meter = decremented by 1 unless already at 0
decrement_fireball_meter:
	mov al, [fireball_meter]
	cmp al, 0
	je .done

	dec byte [fireball_meter]	; decrement the meter

	mov ah, al
	inc al
	shr al, 1	; number of cells used (final one may be empty or half-full)

	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(240, 54)
	; If the meter is odd, draw the final cell as GRAPHIC_METER_HALF. If
	; the meter is even, draw the final cell as GRAPHIC_METER_EMPTY.
	lea si, [GRAPHIC_METER_HALF]	; initially assume odd
	test ah, 1	; check least significant bit
	jz .l1
	lea si, [GRAPHIC_METER_EMPTY]	; no, it was even
.l1:
	xor ah, ah
	add di, ax	; add 8 pixels to the x-coordinate for each used cell
	jmp blit_8x16	; tail call
	nop		; dead code

.done:
	ret

; Blit an 8×16 graphic to both the onscreen and offscreen video buffers,
; without a mask. This is used for meters and score digits.
; Input:
;   ds:si = graphic (1 byte per row, 16 bytes per plane, 64 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_8x16:
	mov ah, 1
	call blit_8x16_plane
	mov ah, 2
	call blit_8x16_plane
	mov ah, 4
	call blit_8x16_plane
	mov ah, 8
	call blit_8x16_plane
	ret

; Blit a single plane of an 8×16 graphic to both the onscreen and offscreen
; video buffers, without a mask. This is used for meters and score digits.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   ds:si = graphic (1 byte per row, 16 bytes per plane, 64 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_8x16_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	; Blit relative to the 0x0000 video buffer.
	mov bx, 16	; number of rows
	push si
	push di
.loop_0000:
	movsb		; copy 1 byte from ds:si to es:di
	add di, 39	; next destination row (each row is 40 bytes; compensate for the 1 byte we just wrote)
	dec bx
	jne .loop_0000	; for each row
	pop di
	pop si

	; Do it again with the 0x2000 video buffer.
	mov bx, 16	; number of rows
	push di
	add di, 0x2000	; now do the other buffer
.loop_2000:
	movsb		; copy 1 byte from ds:si to es:di
	add di, 39	; next destination row (each row is 40 bytes; compensate for the 1 byte we just wrote)
	dec bx
	jne .loop_2000
	pop di

	ret

; Collect a Blastola Cola. Increment comic_firepower and blit the inventory
; graphic.
; Output:
;   comic_firepower = incremented by 1 if not already at MAX_NUM_FIREBALLS
collect_blastola_cola:
	mov al, [comic_firepower]
	cmp al, MAX_NUM_FIREBALLS	; already have max firepower?
	je .done

	inc al	; increment firepower
	mov byte [comic_firepower], al

	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(232, 112)
	lea si, [GRAPHIC_BLASTOLA_COLA_EVEN]	; Blastola Cola inventory graphics follow GRAPHIC_BLASTOLA_COLA_EVEN (which is effectively index 0 in the array we're indexing)
	xor ah, ah
	mov bx, ax
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; comic_firepower * 128
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1	; comic_firepower * 32
	add ax, bx	; comic_firepower * 160 (the size of a 16×16 graphic plus mask)
	add si, ax	; GRAPHIC_BLASTOLA_COLA_EVEN[comic_firepower]
	jmp blit_16x16_evenodd	; tail call
	nop		; useless instruction

.done:
	ret

; Blit a 16×16 graphic to video buffer 0x0000, and its counterpart to video
; buffer 0x2000, both without a mask. The graphic is assumed to in the
; GRAPHICS_ITEMS_EVEN_FRAMES memory area and its counterpart is at the same
; relative offset within GRAPHICS_ITEMS_ODD_FRAMES.
; Input:
;   ds:si = graphic (2 bytes per row, 32 bytes per plane, 128 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_16x16_evenodd:
	mov ah, 1
	call blit_16x16_evenodd_plane
	mov ah, 2
	call blit_16x16_evenodd_plane
	mov ah, 4
	call blit_16x16_evenodd_plane
	mov ah, 8
	call blit_16x16_evenodd_plane
	ret

; Blit a single plane of a 16×16 graphic to video buffer 0x0000, and its
; counterpart to video buffer 0x2000, both without a mask. The graphic is
; assumed to in the GRAPHICS_ITEMS_EVEN_FRAMES memory area and its counterpart
; is at the same relative offset within GRAPHICS_ITEMS_ODD_FRAMES.
; Input:
;   ah = plane mask (1, 2, 4, 8)
;   ds:si = graphic (2 bytes per row, 32 bytes per plane, 128 bytes total)
;   es:di = destination address in video buffer 0x0000
; Output:
;   ds:si = advanced
;   es:di = unchanged
blit_16x16_evenodd_plane:
	; Enable the selected plane.
	mov dx, 0x3c4	; SC Index register
	mov al, 2	; SC Map Mask register: https://sourceforge.net/p/dosbox/code-0/HEAD/tree/dosbox/tags/RELEASE_0_74_3/src/hardware/vga_seq.cpp#l66
	out dx, al
	inc dx		; SC Data register
	mov al, ah
	out dx, al	; write plane mask

	; Blit relative to the 0x0000 video buffer.
	mov bx, 16	; number of rows
	push si
	push di
.loop_0000:
	movsw		; copy 2 bytes from ds:si to es:di
	add di, 38	; next destination row (each row is 40 bytes; compensate for the 2 bytes we just wrote)
	dec bx
	jne .loop_0000	; for each row
	pop di
	pop si

	; Do it again with the 0x2000 video buffer.
	mov bx, 16	; number of rows
	push di
	add di, 0x2000	; now do the other buffer
	add si, GRAPHICS_ITEMS_ODD_FRAMES - GRAPHICS_ITEMS_EVEN_FRAMES
.loop_2000:
	movsw		; copy 2 bytes from ds:si to es:di
	add di, 38	; next destination row (each row is 40 bytes; compensate for the 2 bytes we just wrote)
	dec bx
	jne .loop_2000
	sub si, GRAPHICS_ITEMS_ODD_FRAMES - GRAPHICS_ITEMS_EVEN_FRAMES
	pop di

	ret

; Award the player al*100 points. al must be in the range 0..99. Increment
; score_10000_counter if the score_10000_counter overflows. When
; score_10000_counter reaches 5, call award_extra_life and reset
; score_10000_counter to 0.
; Output:
;   score = increased by al*100 points
;   score_10000_counter = (score_10000_counter + 1) % 5 if adding the points carried into the third digit
;   comic_num_lives = max(comic_num_lives + 1, MAX_NUM_LIVES) if score_10000_counter overflowed to 0
award_points:
	mov bx, 0xa000
	mov es, bx	; es points to video memory

	mov di, 0x3e1	; pixel coordinates (264, 24) of the least significant digit on screen
	lea bx, [score]	; least significant digit in score array
	push bx		; stash the pointer for extra-life purposes below

.loop:
	mov ah, [bx]
	add ah, al	; add the remaining points to this digit
	xor al, al
	cmp ah, 100	; overflow in this digit?
	jb .no_overflow
	inc al		; carry a 1 into the next digit
	sub ah, 100	; take this digit mod 100
.no_overflow:
	mov [bx], ah	; store the altered digit in the score array
	; Blit the altered digit
	push ax
	push bx
	call blit_score_digit
	pop bx
	pop ax
	inc bx		; consider the next digit
	sub di, 2	; scoot the pixel coordinates to the left by 16 pixels
	or al, al	; is there any carry for the next digit?
	jne .loop	; if so, go to the next digit (no check for overflow of the score array)

	; The player earns an extra life every 50000 points. We track this by
	; counting the number of times a score increment causes a carry into
	; the third digit, which is the number of times the ten thousands digit
	; has increased. When it happens 5 times, we award a life and reset the
	; counter.
	pop ax
	inc ax		; ax points to the second digit of the score array
	sub bx, ax	; bx points one place past the last digit that we carried into
	jle .return	; if we did not carry into the second digit, return
	inc byte [score_10000_counter]	; increment the count of how many times we have carried into the ten thousands digit
	cmp byte [score_10000_counter], 5	; have we reached 50000?
	jl .return	; if not, return
	mov byte [score_10000_counter], 0	; reset the counter
	call award_extra_life	; award a life
.return:
	ret

; Blit one base-100 score digit (2 decimal digits) to both the onscreen and
; offscreen video buffers.
; Input:
;   ah = digit to display
;   es:di = destination address
; Output:
;   es:di = unchanged
blit_score_digit:
	inc di		; we will blit the low decimal digit first, so move the coordinates 8 pixels to the right

	; Division/modulus via repeated subtraction. al will store the high
	; decimal digit and ah will store the low
	xor al, al
.division_loop:
	sub ah, 10	; keep subtracting 10 until it underflows
	jb .finished
	inc al		; no underflow yet; increment high decimal digit
	jmp .division_loop
.finished:
	add ah, 10	; undo the underflow in the low decimal digit
	push ax

	; Blit the low decimal digit.
	mov al, ah
	xor ah, ah
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply by 64, the number of bytes in an 8×16 graphic
	lea si, [GRAPHICS_SCORE_DIGITS]
	add si, ax	; index GRAPHICS_SCORE_DIGITS by the low decimal digit
	call blit_8x16

	; Blit the high decimal digit.
	dec di		; move the coordinates back 8 pixels to the left
	pop ax
	xor ah, ah
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; multiply by 64, the number of bytes in an 8×16 graphic
	lea si, [GRAPHICS_SCORE_DIGITS]
	add si, ax	; index GRAPHICS_SCORE_DIGITS by the low decimal digit
	jmp blit_8x16	; tail call

; Collect the Corkscrew.
; Output:
;   comic_has_corkscrew = 1
collect_corkscrew:
	mov byte [comic_has_corkscrew], 1
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(256, 112)
	lea si, [GRAPHIC_CORKSCREW_EVEN]
	jmp blit_16x16_evenodd	; tail call

; Collect the Door Key.
; Output:
;   comic_has_door_key = 1
collect_door_key:
	mov byte [comic_has_door_key], 1
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(280, 112)
	lea si, [GRAPHIC_DOOR_KEY_EVEN]
	jmp blit_16x16_evenodd	; tail call

; Collect the Boots.
; Output:
;   comic_jump_power = 5
collect_boots:
	mov byte [comic_jump_power], 5
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(232, 136)
	lea si, [GRAPHIC_BOOTS_EVEN]
	jmp blit_16x16_evenodd	; tail call

; Collect the Lantern.
; Output:
;   comic_has_lantern = 1
collect_lantern:
	mov byte [comic_has_lantern], 1
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(256, 136)
	lea si, [GRAPHIC_LANTERN_EVEN]
	jmp blit_16x16_evenodd	; tail call

; Collect the Teleport Wand.
; Output:
;   comic_has_teleport_wand = 1
collect_teleport_wand:
	mov byte [comic_has_teleport_wand], 1
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(280, 136)
	lea si, [GRAPHIC_TELEPORT_WAND_EVEN]
	jmp blit_16x16_evenodd	; tail call

; Collect the Gems.
; Output:
;   comic_num_treasures = incremented by 1
;   comic_num_lives = incremented by 1 unless already at MAX_NUM_LIVES
;   win_counter = 20 if comic_num_treasures became 3
collect_gems:
	call award_extra_life
	inc byte [comic_num_treasures]
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(232, 160)
	lea si, [GRAPHIC_GEMS_EVEN]
	call blit_16x16_evenodd
	cmp byte [comic_num_treasures], 3
	je begin_win_countdown
	ret

; Collect the Crown.
; Output:
;   comic_num_treasures = incremented by 1
;   comic_num_lives = incremented by 1 unless already at MAX_NUM_LIVES
;   win_counter = 20 if comic_num_treasures became 3
collect_crown:
	call award_extra_life
	inc byte [comic_num_treasures]
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(256, 160)
	lea si, [GRAPHIC_CROWN_EVEN]
	call blit_16x16_evenodd
	cmp byte [comic_num_treasures], 3
	je begin_win_countdown
	ret

; Initialize win_counter to await the game_end_sequence.
; Output:
;   win_counter = 20
begin_win_countdown:
	mov byte [win_counter], 20
	ret

; Collect the Gold.
; Output:
;   comic_num_treasures = incremented by 1
;   comic_num_lives = incremented by 1 unless already at MAX_NUM_LIVES
;   win_counter = 20 if comic_num_treasures became 3
collect_gold:
	call award_extra_life
	inc byte [comic_num_treasures]
	mov bx, 0xa000
	mov es, bx	; es points to video memory
	mov di, pixel_coords(280, 160)
	lea si, [GRAPHIC_GOLD_EVEN]
	call blit_16x16_evenodd
	cmp byte [comic_num_treasures], 3
	je begin_win_countdown
	ret

; Collect an item if Comic is touching it; otherwise blit the item to the
; offscreen video buffer.
; Input:
;   ah = camera-relative x-coordinate of item
;   al = camera-relative y-coordinate of item
;   bl = item type
;   comic_x, comic_y = coordinates of Comic
;   camera_x = x-coordinate of left edge of playfield
;   item_animation_counter = whether to to draw an even or odd animation frame for the item
; Output:
;   items_collected = updated bitmap of stages' item collection status
;   item_animation_counter = advanced by one position
;   comic_num_lives = incremented by 1 if collecting a Shield with full HP
;   comic_hp_pending_increase = set to MAX_HP if collecting a Shield
;   comic_firepower, comic_jump_power, comic_has_corkscrew, comic_has_door_key,
;     comic_has_lantern, comic_has_teleport_wand, comic_num_treasures = updated as appropriate
;   score = increased by 2000 points if collecting an item
collect_item_or_blit_offscreen:
	push ax		; item coordinates
	push bx		; item type

	; Convert camera-relative coordinates to absolute coordinates.
	mov bx, ax	; bl = y-coordinate, bh = camera-relative x-coordinate
	mov ax, [camera_x]
	add bh, al	; bh = absolute x-coordinate

	; Check horizontal proximity.
	mov dx, [comic_y]	; dl = comic_y, dh = comic_x
	sub bh, dh	; bh = item_x - comic_x
	jae .l1
	neg bh		; abs(item_x - comic_x)
.l1:
	cmp bh, 2	; is the item's x-coordinate within 1 game unit of Comic's?
	jae .blit	; abs(item_x - comic_x) >= 2

	; Check vertical proximity.
	sub bl, dl	; item_y - comic_y
	jb .blit	; item_y - comic_y < 0
	cmp bl, 4	; is the item's y-coordinate between 0 (top of head) and 3 (knees) units from Comic's?
	jae .blit	; item_y - comic_y >= 4

	; We're close enough in both dimensions; pick up the item.
	jmp collect_item	; tail call
	nop		; dead code

.blit:
	pop bx		; bl = item_type
	xor bh, bh
	mov ax, bx
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1	; item_type * 128
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1
	shl bx, 1	; item_type * 32
	add ax, bx	; item_type * 160 (size of a 16×16 EGA graphic with mask)
	lea si, [GRAPHICS_ITEMS_EVEN_FRAMES]
	; Advance item_animation_counter (0→1, 1→0). It controls whether to
	; draw an even or an odd animation frame.
	inc byte [item_animation_counter]
	cmp byte [item_animation_counter], 2	; item_animation_counter overflowed?
	jne .l2
	mov byte [item_animation_counter], 0	; wrap item_animation_counter back to 0
	add si, GRAPHICS_ITEMS_ODD_FRAMES - GRAPHICS_ITEMS_EVEN_FRAMES	; draw an odd frame
.l2:
	add si, ax	; address of graphic
	mov bp, si
	add bp, 128	; address of mask
	mov cx, 16	; height of graphic
	pop ax		; al = camera-relative y-coordinate, ah = camera-relative x-coordinate
	call blit_16xH_with_mask_playfield_offscreen
	ret

; Collect an item.
; Input:
;   top word in stack = item type
;   next-to-top word in stack = camera-relative coordinates of item (unused)
; Output:
;   sp = decremented by 4 (2 words popped off)
;   items_collected = updated bitmap of stages' item collection status
;   comic_num_lives = incremented by 1 if collecting a Shield with full HP
;   comic_hp_pending_increase = set to MAX_HP if collecting a Shield
;   comic_firepower, comic_jump_power, comic_has_corkscrew, comic_has_door_key,
;     comic_has_lantern, comic_has_teleport_wand, comic_num_treasures = updated as appropriate
;   score = increased by 2000 points
collect_item:
	lea bx, [SOUND_COLLECT_ITEM]
	mov ax, SOUND_PLAY
	mov cx, 3	; priority 3
	int3

	mov al, 20	; 2000 points
	call award_points

	; Mark item as collected in items_collected.
	lea bx, [items_collected]
	mov al, [current_stage_number]
	shl al, 1
	shl al, 1
	shl al, 1
	shl al, 1	; current_stage_number * 16
	add al, [current_level_number]	; current_stage_number * 16 + current_level_number
	xor ah, ah
	add bx, ax
	mov byte [bx], 1	; items_collected[current_stage_number][current_level_number] = 1

	pop bx		; bx = item type
	pop ax		; al = item's camera-relative coordinates (unused)
	cmp bl, ITEM_SHIELD
	jne .check_corkscrew	; the item is not a Shield

	; Picked up a Shield. Do we already have full HP?
	cmp byte [comic_hp], MAX_HP
	jne .not_already_full_hp

	; If so, award an extra life.
	call award_extra_life
	ret

.not_already_full_hp:
	; If not, schedule MAX_HP units of HP to be awarded in the future.
	mov byte [comic_hp_pending_increase], MAX_HP
	ret

.check_corkscrew:
	or bx, bx	; ITEM_CORKSCREW?
	jne .check_door_key
	jmp collect_corkscrew

.check_door_key:
	dec bl		; ITEM_DOOR_KEY?
	jne .check_boots
	jmp collect_door_key

.check_boots:
	dec bl		; ITEM_BOOTS?
	jne .check_lantern
	jmp collect_boots

.check_lantern:
	dec bl		; ITEM_LANTERN?
	jne .check_teleport_wand
	jmp collect_lantern

.check_teleport_wand:
	dec bl		; ITEM_TELEPORT_WAND?
	jne .check_gems
	jmp collect_teleport_wand

.check_gems:
	dec bl		; ITEM_GEMS?
	jne .check_crown
	jmp collect_gems

.check_crown:
	dec bl		; ITEM_CROWN?
	jne .check_gold
	jmp collect_crown

.check_gold:
	dec bl		; ITEM_GOLD?
	jne .check_blastola_cola
	jmp collect_gold

.check_blastola_cola:
	; Any other item type is Blastola Cola by default.
	jmp collect_blastola_cola

alignb	16

; Do high scores. Display the high score graphic and load the high scores file.
; If the player got a new high score, prompt them to enter a name and save an
; updated high scores file.
; Input:
;   score = will be ranked among existing high scores
do_high_scores:
	mov ax, 0x000d	; ah=0x00: set video mode; al=13: 320×200 16-color EGA
	int 0x10

	cld
	; Load the high scores graphic into video buffer 0x0000.
	lea dx, [FILENAME_HIGH_SCORES_GRAPHIC]	; "sys005.ega"
	mov ax, 0xa000
	mov es, ax	; es points to video memory
	xor di, di	; video buffer 0x0000
	call load_fullscreen_graphic

	mov ax, data
	mov es, ax	; es points to the data segment
	jmp .try_open_high_scores

.load_defaults:
	push es

	mov bx, NUM_HIGH_SCORES
	mov ax, ds
	mov es, ax	; es = ds
	lea di, [high_scores]
.load_defaults_loop:
	mov cx, high_score_size
	lea si, [DEFAULT_HIGH_SCORE]
	rep movsb	; copy high_score_size bytes from ds:si to es:di
	dec bx
	jne .load_defaults_loop

	pop es
	jmp .rank_player_score
	nop		; dead code

.try_open_high_scores:
	mov ax, 0x3d00	; ah=0x3d: open existing file
	lea dx, [FILENAME_HIGH_SCORES]	; "COMIC.HGH"
	int 0x21
	jc .load_defaults	; load default high scores if opening the file failed

	; We were able to open the high scores file.
	mov bx, ax	; bx = file handle
	mov cx, NUM_HIGH_SCORES * high_score_size
	lea dx, [high_scores]
	mov ah, 0x3f	; ah=0x3f: read from file or device
	int 0x21
	mov ah, 0x3e	; ah=0x3e: close file
	int 0x21

.rank_player_score:
	; See if the player ranks among the existing high scores. The entries
	; in the high_scores array are sorted with the highest scores first.
	lea si, [high_scores]	; for each high score
	mov cx, NUM_HIGH_SCORES
.rank_loop:
	push cx
	push si

	; The score is an array of 3 bytes, where each byte represents a
	; base-100 digit, arranged in order from least significant to most
	; significant. (See the comment above the "score" label.) We compare
	; two scores by comparing them bytewise, starting at the end.
	add si, high_score.score + 2	; most significant digit of high score
	lea di, [score + 2]		; most significant digit of player score
	mov cx, 3	; 3 digits in a score
.compare_loop:
	mov al, [si]	; digit of high score
	cmp al, [di]	; digit of player score
	jl .new_high_score	; high score < player score, this is a new high score
	jg .rank_loop_next	; player score < high score, move on to the next high score
	; equal
	dec si	; next high score digit
	dec di	; next player score digit
	loop .compare_loop

.rank_loop_next:
	pop si
	add si, high_score_size	; next high score
	pop cx
	loop .rank_loop

	; Got to the end of the list without finding a high score that the
	; player beat, just display the high scores.
	jmp .display

.new_high_score:
	; Now we'll insert the player's score into the high scores table,
	; dropping the lowest existing score.

	pop si	; si = address of the high score that was beaten by the player score
	pop cx	; cx = number of high scores that are beaten by the player score
	push si

	mov ax, high_score_size
	mul cl
	mov cx, ax	; cx = high_score_size * (number of lower high scores)

	; Copy the scores that are lower than the player score.
	lea di, [high_scores_tmp]
	push si
	push di
	push cx
	rep movsb	; copy cx bytes from ds:si to high_scores_tmp
	pop cx		; cx = high_score_size * (number of lower high scores)
	pop si		; si = high_scores_tmp
	pop di		; di = highest high score beaten by the player score

	; Copy back all but the lowest score.
	add di, high_score_size	; advance destination pointer by 1
	sub cx, high_score_size	; decrement number of entries to copy by 1
	rep movsb	; copy cx bytes from high_scores_tmp to es:di

	pop di	; si = address of the high score that was beaten by the player score; i.e., where to insert the player score
	push di

	; Copy the player score into the data structure.
	add di, high_score.score
	lea si, [score]
	mov cx, 3
	rep movsb	; copy 3 bytes from score to high_score.score

	; Display the "Enter Your Name" prompt.
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	mov dx, 0x0a0e	; row 10, column 14
	int 0x10
	mov bx, 9	; color 9
	lea dx, [STR_GREAT_SCORE]	; "Great Score!"
	call display_dollar_terminated_string
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	mov dx, 0x0b05	; row 11, column 5
	int 0x10
	mov bx, 10	; color 10
	lea dx, [STR_ENTER_YOUR_NAME]	; "Enter Your Name: "
	call display_dollar_terminated_string

	; Read the name from the keyboard.
	lea dx, [high_score_name_buffer]
	mov ah, 0x0a	; ah=0x0a: buffered input
	int 0x21
	mov cl, [high_score_name_buffer_n]
	xor ch, ch	; cx = number of characters read
	; Copy it into the high score table.
	lea si, [high_score_name_buffer_text]
	pop di		; di = address of the high score that was beaten by the player score; i.e., where to insert the player score
	rep movsb	; copy cx bytes from high_score_name_buffer_text to high_score.name
	mov byte [di], '$'	; terminate the string

	; Erase the prompt.
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	mov dx, 0x0a05	; row 10, column 5
	int 0x10
	mov bx, 1	; color 1
	lea dx, [STR_BLANKS]
	call display_dollar_terminated_string
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	mov dx, 0x0b05	; row 11, column 5
	int 0x10
	mov bx, 2	; color 2
	lea dx, [STR_BLANKS]
	call display_dollar_terminated_string

	mov ax, 0x3c00	; ah=0x3c: create or truncate file
	xor cx, cx	; file attributes
	lea dx, [FILENAME_HIGH_SCORES]	; "COMIC.HGH"
	int 0x21
	jc .display	; skip saving the high scores if file can't be opened

	mov bx, ax	; bx = file handle
	mov cx, NUM_HIGH_SCORES * high_score_size
	lea dx, [high_scores]
	mov ah, 0x40	; ah=0x40: write to file
	int 0x21
	mov ah, 0x3e	; ah=0x3e: close file
	int 0x21

.display:
	; Display the high score table, possibly now including the player
	; score.
	mov cx, NUM_HIGH_SCORES
	lea si, [high_scores]	; for each high score entry

	mov dh, 7	; row 7
.display_loop:
	push cx		; save loop counter
	push dx		; save cursor position

	; dh is row
	mov dl, 8	; column 8
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	int 0x10

	mov bx, 11	; color 11
	mov dx, si
	call display_dollar_terminated_string

	pop dx		; restore cursor position
	push dx		; save cursor position

	; dh is row
	mov dl, 24	; column 24
	mov ah, 0x02	; ah=0x02: set cursor position
	mov bh, 0	; bh=0x00: page number 0
	int 0x10

	mov bx, si
	add bx, high_score.score
	call display_score

	pop dx		; restore cursor position
	pop cx		; restore loop counter
	add dh, 1	; next row
	cmp cx, NUM_HIGH_SCORES	; is this the first loop iteration?
	jne .display_loop_next
	add dh, 1	; put a blank row under the top score
.display_loop_next:
	add si, high_score_size	; next high score entry
	loop .display_loop

	xor ax, ax	; ah=0x00: get keystroke
	int 0x16	; wait for any keystroke
	ret

; Display the score array as a decimal string at the current cursor position,
; in color 12.
; Input:
;   bx = pointer to 3-byte score array
display_score:
	mov al, [bx + 2]
	call display_score_digit
	mov al, [bx + 1]
	call display_score_digit
	mov al, [bx + 0]
	call display_score_digit
	lea dx, [STR_00]
	mov bx, 12	; color 12
	call display_dollar_terminated_string
	ret

; Display a single base-100 score digit as two base-10 digits at the current
; cursor position, in color 12.
; Input:
;   al = digit to display
display_score_digit:
	push bx

	; cl will contain the ones decimal digit and al will contain the tens
	; decimal digit.
	mov cl, al
	xor al, al
.loop:
	sub cl, 10	; division/remainder by repeated subtraction
	jl .loop_break
	inc al		; increment the tens digit
	jmp .loop
.loop_break:
	add al, '0'
	add cl, 10	; the division loop goes one step too far; undo the final subtraction

	; Display the tens digit.
	push cx
	; al = character to write
	mov ah, 0x0e	; ah=0x0e: teletype output
	mov bx, 0x000c	; bh=0x00: page number 0, bl=0x0c: color 12
	int 0x10

	; Display the ones digit.
	pop ax
	add al, '0'
	mov ah, 0x0e	; ah=0x0e: teletype output
	int 0x10

	pop bx
	ret

; Display a '$'-terminated string at the current cursor position.
; Input:
;   ds:dx = address to string
display_dollar_terminated_string:
	push si
	push ax
	mov si, dx
	mov ah, 0x0e	; ah=0x0e: teletype output
.loop:
	lodsb		; set al to [si] and advance si
	cmp al, '$'
	je .loop_break
	int 0x10
	jmp .loop
.loop_break:
	pop ax
	pop si
	ret

; The ds register normally points to this section.
section data
sectalign	16

VIDEO_MODE_ERROR_MESSAGE	db `This program requires an EGA adapter with 256K installed\n\r$`

interrupt_handler_install_sentinel	db	0
; source_door_level_number and source_door_stage_number are set to the
; level/stage we just came from, if we are entering the current stage via a
; door. The special value source_door_level_number == -1 means that we are
; entering this stage at a left/right boundary, not via a door. The special
; value source_door_level_number == -2 is set only at the start of the game and
; means we are entering the stage by beaming in to the planet's surface.
source_door_level_number	db	-2
source_door_stage_number	db	0
current_level_number	db	LEVEL_NUMBER_FOREST
current_stage_number	db	0
comic_y			db	0
comic_x			db	0
camera_x		dw	0
comic_num_lives		db	0
comic_hp		db	0
comic_firepower		db	0	; how many Blastola Colas Comic has collected
fireball_meter		db	0
comic_jump_power	db	4
comic_has_corkscrew	db	0
comic_has_door_key	db	0
comic_has_lantern	db	0
comic_has_teleport_wand	db	0
comic_num_treasures	db	0
; The score is an array of 3 bytes, stored in base-100 representation. score[0]
; is the least significant digit. The number stored here one one-hundredth of
; the score shown to the player, which always has a "00" appended to the end.
score			db	0, 0, 0
score_10000_counter	db	0

; Array of booleans indicating whether the item in each stage has been
; collected. Index as: items_collected[stage*16 + level]. Only the first 8
; elements in each row are used.)
items_collected:
resb	16
resb	16
resb	16

; https://en.wikipedia.org/wiki/Piano_key_frequencies#List
; Convert a divisor D into a frequency as 1193182 / D.
SOUND_TERMINATOR	equ	0x0000
NOTE_REST		equ	0x0028	; 29829.550 Hz
NOTE_C3			equ	0x2394	; 131.004 Hz
NOTE_D3			equ	0x1fb5	; 146.998 Hz
NOTE_E3			equ	0x1c3f	; 165.009 Hz
NOTE_F3			equ	0x1aa2	; 175.005 Hz
NOTE_G3			equ	0x17c8	; 195.989 Hz
NOTE_A3			equ	0x1530	; 219.982 Hz
NOTE_B3			equ	0x12df	; 246.984 Hz
NOTE_C4			equ	0x11ca	; 262.007 Hz
NOTE_B4			equ	0x0974	; 493.050 Hz
NOTE_C5			equ	0x08e9	; 523.096 Hz
NOTE_D5			equ	0x07f1	; 586.907 Hz
NOTE_E5			equ	0x0713	; 658.853 Hz
NOTE_F5			equ	0x06ad	; 698.176 Hz
NOTE_G5			equ	0x05f2	; 783.957 Hz
NOTE_A5			equ	0x054b	; 880.577 Hz

SOUND_TITLE:
dw	NOTE_E3, 3
dw	NOTE_F3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 7
dw	NOTE_C4, 3
dw	NOTE_G3, 5
dw	NOTE_E3, 3
dw	NOTE_F3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 5
dw	NOTE_F3, 3
dw	NOTE_D3, 8
dw	NOTE_C3, 10
dw	NOTE_REST, 3
dw	NOTE_C4, 3
dw	NOTE_B3, 3
dw	NOTE_A3, 7
dw	NOTE_F3, 3
dw	NOTE_A3, 6
dw	NOTE_C4, 5
dw	NOTE_G3, 8
dw	NOTE_A3, 3
dw	NOTE_G3, 6
dw	NOTE_C4, 3
dw	NOTE_B3, 3
dw	NOTE_A3, 6
dw	NOTE_F3, 5
dw	NOTE_A3, 3
dw	NOTE_C4, 10
dw	NOTE_G3, 10
dw	NOTE_REST, 3
dw	NOTE_E3, 3
dw	NOTE_F3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 7
dw	NOTE_C4, 3
dw	NOTE_G3, 5
dw	NOTE_E3, 3
dw	NOTE_F3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 5
dw	NOTE_A3, 3
dw	NOTE_B3, 8
dw	NOTE_C4, 10
dw	NOTE_REST, 3
dw	SOUND_TERMINATOR, 0

; Some strings are stored in an obfuscated form, with every byte xored with
; 0x25. The xor_encrypt macro obfuscates a literal string in this way.
XOR_ENCRYPTION_KEY	equ	0x25
%macro	xor_encrypt 1
%strlen %%len %1
%assign i 0
%rep %%len
%assign i i+1
%substr c %1 i
	db	c^XOR_ENCRYPTION_KEY
%endrep
%endmacro

STARTUP_NOTICE_TEXT:	xor_encrypt `\
                The Adventures of Captain Comic  --  Revision 2\r\n\
                        Copyright 1988 by Michael Denio\r\n\
\r\n\
  This software is being distributed under the Shareware concept, where you as\r\n\
  the user  are  allowed  to use the program on a "trial" basis.  If you enjoy\r\n\
  playing  Captain  Comic,  you  are encouraged to register yourself as a user\r\n\
  with a $10 to $20 contribution. Registered users will be given access to the\r\n\
  official Captain Comic  question hotline (my home phone number), and will be\r\n\
  the first in line to receive new Comic adventures.\r\n\
\r\n\
        This product is copyrighted material, but may be re-distributed\r\n\
                 by complying to these two simple restrictions:\r\n\
\r\n\
        1. The program and graphics (including world maps) may not be\r\n\
           distributed in any modified form.\r\n\
        2. No form of compensation is be collected from the distribution\r\n\
           of this program, including any disk handling costs or BBS\r\n\
           file club fees.\r\n\
\r\n\
    Questions and contributions can be sent to me at the following address:\r\n\
                                Michael A. Denio\r\n\
                             1420 W. Glen Ave #202\r\n\
                                Peoria, IL 61614\r\n\
\r\n\
      Press \'K\' to define the keyboard  ---  Press any other key to begin.\
\0\0`

; The first byte of SOUND_TITLE_RECAP is simultaneously the terminator for
; STARTUP_NOTICE_TEXT. NOTE_E3 is 0x1c3f, whose little-endian first byte is
; 0x3f, which is the terminator sentinel value that display_xor_decrypt looks
; for.
SOUND_TITLE_RECAP:
dw	NOTE_E3, 6
dw	NOTE_F3, 6
dw	NOTE_G3, 9
dw	NOTE_REST, 1
dw	NOTE_G3, 9
dw	NOTE_REST, 1
dw	NOTE_G3, 9
dw	NOTE_REST, 1
dw	NOTE_G3, 9
dw	NOTE_REST, 1
dw	NOTE_G3, 14
dw	NOTE_C4, 7
dw	NOTE_G3, 18
dw	NOTE_B3, 20
dw	NOTE_C4, 20
dw	SOUND_TERMINATOR, 0

; Unused garbage bytes? They vaguely resemble the format of a sound, a 1193.182
; Hz tone played three times. The final 0x0000, 0x0000 is the same as
; SOUND_TERMINATOR. But 0x0001 for a rest does not match the convention of
; 0x0028 for NOTE_REST used elsewhere in the program.
db	0xe8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00
db	0xe8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00
db	0xe8, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00

; This string (an xor-encrypted empty string) is displayed by
; terminate_program.
TERMINATE_PROGRAM_TEXT:	xor_encrypt `\x1a`
	xor_encrypt `\x1a`	; an unused xor-encrypted empty string

FILENAME_TITLE_GRAPHIC	db	`sys000.ega\0`
FILENAME_UI_GRAPHIC	db	`sys003.ega\0`
FILENAME_STORY_GRAPHIC	db	`sys001.ega\0`
FILENAME_ITEMS_GRAPHIC	db	`sys004.ega\0`
FILENAME_KEYMAP		db	`KEYS.DEF\0`
			db	`File Error\n\r$`	; unused

alignb	16
; This buffer holds the data read from the .TT2 file for the current level.
tileset_buffer:
; http://www.shikadi.net/moddingwiki/Captain_Comic_Tileset_Format#File_format
tileset_last_passable	db	0
			db	0	; unused
			db	0	; unused
tileset_flags		db	0
tileset_graphics	resb	128*128	; space for up to 128 16×16 tile graphics
tileset_graphics_size	equ	$ - tileset_graphics

current_tiles_ptr	dw	0	; points to {pt0,pt1,pt2} + pt.tiles

; http://www.shikadi.net/moddingwiki/Captain_Comic_Map_Format#File_format
struc pt
.width			resw	1
.height			resw	1
.tiles			resb	MAP_WIDTH_TILES*MAP_HEIGHT_TILES
endstruc

pt0:
istruc pt
iend

pt1:
istruc pt
iend

pt2:
istruc pt
iend

current_stage_ptr	dw	0

struc enemy
.y			resb	1
.x			resb	1
.x_vel			resb	1
.y_vel			resb	1
.spawn_timer_and_animation	resb	1	; when the enemy is not spawned, the counter until it spawns; when spawned, the animation counter
.num_animation_frames	resb	1
.behavior		resb	1
.animation_frames_ptr	resw	1
.state			resb	1	; 0 = despawned, 1 = spawned, 2..6 = white spark animation counter, 8..12 = red spark animation counter
.facing			resb	1
.restraint		resb	1	; governs whether the enemy moves every tick or every other tick
endstruc

enemies:
%rep MAX_NUM_ENEMIES
istruc enemy
iend
%endrep

; The number of frames in each of the 4 enemies' animation cycles. The values
; are always 4 for the .SHP files that come with Captain Comic. (Either 4
; distinct frames for ENEMY_HORIZONTAL_SEPARATE, or 3 distinct + 1 duplicate
; for ENEMY_HORIZONTAL_DUPLICATED.) This number of frames only counts one
; facing direction (the number of frames facing left and facing right is the
; same).
enemy_num_animation_frames	resb	4
; Buffers for raw graphics data from .SHP files. There is room for up to 5
; left-facing graphics and 5 right-facing graphics.
enemy_graphics:
enemy0_graphics		resb	10*160
enemy1_graphics		resb	10*160
enemy2_graphics		resb	10*160
enemy3_graphics		resb	10*160
; Pointers to graphics data for each enemy's animation cycle. These arrays
; contain pointers into enemy*_graphics.
enemy_animation_frames:
enemy0_animation_frames	resw	10
enemy1_animation_frames	resw	10
enemy2_animation_frames	resw	10
enemy3_animation_frames	resw	10

; http://www.shikadi.net/moddingwiki/Captain_Comic_Map_Format#Executable_file
struc shp
.num_distinct_frames	resb	1	; the number of animation frames stored in the file (not counting automatically duplicated ones or right-facing ones)
.horizontal		resb	1	; ENEMY_HORIZONTAL_DUPLICATED or ENEMY_HORIZONTAL_SEPARATE
.animation		resb	1	; ENEMY_ANIMATION_LOOP or ENEMY_ANIMATION_ALTERNATE
.filename		resb	14
endstruc

struc door
.y			resb	1
.x			resb	1
.target_level		resb	1
.target_stage		resb	1
endstruc

struc enemy_record
.shp_index		resb	1	; index into the level.shp array, indicating what graphics to use for this enemy, 0xff means this slot is unused
.behavior		resb	1	; ENEMY_BEHAVIOR_* or ENEMY_BEHAVIOR_UNUSED
endstruc

struc stage
.item_type		resb	1
.item_y			resb	1
.item_x			resb	1
.exit_l			resb	1
.exit_r			resb	1
.doors			resb	3*door_size
.enemies		resb	4*enemy_record_size
endstruc

struc level
.tt2_filename		resb	14
.pt0_filename		resb	14
.pt1_filename		resb	14
.pt2_filename		resb	14
.door_tile_ul		resb	1
.door_tile_ur		resb	1
.door_tile_ll		resb	1
.door_tile_lr		resb	1
.shp			resb	4*shp_size
.stages			resb	3*stage_size
endstruc

; This data structure represents the currently loaded level. It is populated by
; copying from the static data structures referenced from LEVEL_DATA_POINTERS.
current_level:
istruc level
iend

LEVEL_DATA_POINTERS:
dw	LEVEL_DATA_LAKE
dw	LEVEL_DATA_FOREST
dw	LEVEL_DATA_SPACE
dw	LEVEL_DATA_BASE
dw	LEVEL_DATA_CAVE
dw	LEVEL_DATA_SHED
dw	LEVEL_DATA_CASTLE
dw	LEVEL_DATA_COMP

; Where to place Comic when entering a new stage or respawning after death. The
; initial value of (14, 12) is used at the start of the game.
comic_y_checkpoint	db	12
comic_x_checkpoint	db	14

alignb	16
GRAPHICS_COMIC:
incbin	"graphics/comic_standing_right_16x32m.ega"
incbin	"graphics/comic_running_1_right_16x32m.ega"
incbin	"graphics/comic_running_2_right_16x32m.ega"
incbin	"graphics/comic_running_3_right_16x32m.ega"
incbin	"graphics/comic_jumping_right_16x32m.ega"
incbin	"graphics/comic_standing_left_16x32m.ega"
incbin	"graphics/comic_running_1_left_16x32m.ega"
incbin	"graphics/comic_running_2_left_16x32m.ega"
incbin	"graphics/comic_running_3_left_16x32m.ega"
incbin	"graphics/comic_jumping_left_16x32m.ega"

GRAPHIC_FIREBALL_0:
incbin	"graphics/fireball_0_16x8m.ega"
GRAPHIC_FIREBALL_1:
incbin	"graphics/fireball_1_16x8m.ega"

GRAPHIC_WHITE_SPARK_0:
incbin	"graphics/white_spark_0_16x16m.ega"
GRAPHIC_WHITE_SPARK_1:
incbin	"graphics/white_spark_1_16x16m.ega"
GRAPHIC_WHITE_SPARK_2:
incbin	"graphics/white_spark_2_16x16m.ega"

GRAPHIC_RED_SPARK_0:
incbin	"graphics/red_spark_0_16x16m.ega"
GRAPHIC_RED_SPARK_1:
incbin	"graphics/red_spark_1_16x16m.ega"
GRAPHIC_RED_SPARK_2:
incbin	"graphics/red_spark_2_16x16m.ega"

ANIMATION_COMIC_DEATH:
incbin	"graphics/comic_death_0_16x32m.ega"
incbin	"graphics/comic_death_1_16x32m.ega"
incbin	"graphics/comic_death_2_16x32m.ega"
incbin	"graphics/comic_death_3_16x32m.ega"
incbin	"graphics/comic_death_4_16x32m.ega"
incbin	"graphics/comic_death_5_16x32m.ega"
incbin	"graphics/comic_death_6_16x32m.ega"
incbin	"graphics/comic_death_7_16x32m.ega"

GRAPHIC_TELEPORT_0:
incbin	"graphics/teleport_0_16x32m.ega"
GRAPHIC_TELEPORT_1:
incbin	"graphics/teleport_1_16x32m.ega"
GRAPHIC_TELEPORT_2:
incbin	"graphics/teleport_2_16x32m.ega"

ANIMATION_MATERIALIZE:
incbin	"graphics/materialize_0_16x32m.ega"
incbin	"graphics/materialize_1_16x32m.ega"
incbin	"graphics/materialize_2_16x32m.ega"
incbin	"graphics/materialize_3_16x32m.ega"
incbin	"graphics/materialize_4_16x32m.ega"
incbin	"graphics/materialize_5_16x32m.ega"
incbin	"graphics/materialize_6_16x32m.ega"
incbin	"graphics/materialize_7_16x32m.ega"
incbin	"graphics/materialize_8_16x32m.ega"
incbin	"graphics/materialize_9_16x32m.ega"
incbin	"graphics/materialize_10_16x32m.ega"
incbin	"graphics/materialize_11_16x32m.ega"

ANIMATION_TABLE_FIREBALL:
dw	GRAPHIC_FIREBALL_0
dw	GRAPHIC_FIREBALL_1

; Entries 0..5 of this table describe the animation of the white spark caused
; by a fireball hitting an enemy. Entries 6..10 describe the animation of the
; red spark caused by an enemy hitting Comic. The tables are indexed by
; enemy.state - 2, when enemy.state >= 2.
ANIMATION_TABLE_SPARKS:
ANIMATION_TABLE_WHITE_SPARK:
dw	GRAPHIC_WHITE_SPARK_0
dw	GRAPHIC_WHITE_SPARK_1
dw	GRAPHIC_WHITE_SPARK_2
dw	GRAPHIC_WHITE_SPARK_1
dw	GRAPHIC_WHITE_SPARK_0
dw	GRAPHIC_WHITE_SPARK_0	; not actually part of the animation; a dummy sentinel slot
ANIMATION_TABLE_RED_SPARK:
dw	GRAPHIC_RED_SPARK_0
dw	GRAPHIC_RED_SPARK_1
dw	GRAPHIC_RED_SPARK_2
dw	GRAPHIC_RED_SPARK_1
dw	GRAPHIC_RED_SPARK_0

ANIMATION_TABLE_TELEPORT:
dw	GRAPHIC_TELEPORT_0
dw	GRAPHIC_TELEPORT_1
dw	GRAPHIC_TELEPORT_2
dw	GRAPHIC_TELEPORT_1
dw	GRAPHIC_TELEPORT_0

; Here starts a block of graphics that each have a counterpart graphic at a
; fixed offset. "Counterpart graphic" means, for example, the odd frame of an
; item animation, or the dark version of the life icon.
GRAPHICS_ITEMS_EVEN_FRAMES:
GRAPHIC_CORKSCREW_EVEN:
incbin	"graphics/corkscrew_even_16x16m.ega"
GRAPHIC_DOOR_KEY_EVEN:
incbin	"graphics/door_key_even_16x16m.ega"
GRAPHIC_BOOTS_EVEN:
incbin	"graphics/boots_even_16x16m.ega"
GRAPHIC_LANTERN_EVEN:
incbin	"graphics/lantern_even_16x16m.ega"
GRAPHIC_TELEPORT_WAND_EVEN:
incbin	"graphics/teleport_wand_even_16x16m.ega"
GRAPHIC_GEMS_EVEN:
incbin	"graphics/gems_even_16x16m.ega"
GRAPHIC_CROWN_EVEN:
incbin	"graphics/crown_even_16x16m.ega"
GRAPHIC_GOLD_EVEN:
incbin	"graphics/gold_even_16x16m.ega"
GRAPHIC_BLASTOLA_COLA_EVEN:
incbin	"graphics/blastola_cola_even_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_1_even_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_2_even_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_3_even_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_4_even_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_5_even_16x16m.ega"
incbin	"graphics/shield_even_16x16m.ega"
GRAPHIC_LIFE_ICON:
incbin	"graphics/life_icon_bright_16x16m.ega"

GRAPHICS_ITEMS_ODD_FRAMES:
incbin	"graphics/corkscrew_odd_16x16m.ega"
incbin	"graphics/door_key_odd_16x16m.ega"
incbin	"graphics/boots_odd_16x16m.ega"
incbin	"graphics/lantern_odd_16x16m.ega"
incbin	"graphics/teleport_wand_odd_16x16m.ega"
incbin	"graphics/gems_odd_16x16m.ega"
incbin	"graphics/crown_odd_16x16m.ega"
incbin	"graphics/gold_odd_16x16m.ega"
incbin	"graphics/blastola_cola_odd_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_1_odd_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_2_odd_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_3_odd_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_4_odd_16x16m.ega"
incbin	"graphics/blastola_cola_inventory_5_odd_16x16m.ega"
incbin	"graphics/shield_odd_16x16m.ega"
incbin	"graphics/life_icon_dark_16x16m.ega"

GRAPHICS_SCORE_DIGITS:
incbin	"graphics/score_digit_0_8x16.ega"
incbin	"graphics/score_digit_1_8x16.ega"
incbin	"graphics/score_digit_2_8x16.ega"
incbin	"graphics/score_digit_3_8x16.ega"
incbin	"graphics/score_digit_4_8x16.ega"
incbin	"graphics/score_digit_5_8x16.ega"
incbin	"graphics/score_digit_6_8x16.ega"
incbin	"graphics/score_digit_7_8x16.ega"
incbin	"graphics/score_digit_8_8x16.ega"
incbin	"graphics/score_digit_9_8x16.ega"

GRAPHIC_METER_EMPTY:
incbin	"graphics/meter_empty_8x16.ega"
GRAPHIC_METER_HALF:
incbin	"graphics/meter_half_8x16.ega"
GRAPHIC_METER_FULL:
incbin	"graphics/meter_full_8x16.ega"

GRAPHIC_PAUSE:
incbin	"graphics/pause_128x48.ega"

GRAPHIC_GAME_OVER:
incbin	"graphics/game_over_128x48.ega"

alignb	16
door_blit_offset	dw	0	; used as an extra pointer parameter by exit_door and enter_door

teleport_animation	db	0
teleport_destination_y	db	0
teleport_destination_x	db	0
teleport_source_y	db	0
teleport_source_x	db	0
teleport_camera_counter	db	0	; how long to move the camera during a teleport (may be less than the teleport duration of 6 ticks)
teleport_camera_vel	dw	0	; the camera moves by this much (signed) while teleport_camera_counter is positive

; Used as parameters to blit_WxH.
blit_WxH_width		dw	0	; byte width, 1/8 of pixel width
blit_WxH_height		dw	0

FILENAME_WIN_GRAPHIC	db	`sys002.ega\0`

SOUND_DOOR:
dw	0x0f00, 1	; 310.724 Hz
dw	0x0d00, 1	; 358.528 Hz
dw	0x0c00, 1	; 388.406 Hz
dw	0x0b00, 1	; 423.715 Hz
dw	0x0a00, 1	; 466.087 Hz
dw	0x0b00, 1	; 423.715 Hz
dw	0x0c00, 1	; 388.406 Hz
dw	0x0d00, 1	; 358.528 Hz
dw	0x0f00, 1	; 310.724 Hz
dw	SOUND_TERMINATOR, 0

SOUND_DEATH:
dw	0x3000, 1	;  97.101 Hz
dw	0x3800, 1	;  83.230 Hz
dw	0x4000, 1	;  72.826 Hz
dw	0x0800, 1	; 582.608 Hz
dw	0x1000, 1	; 291.304 Hz
dw	0x1800, 1	; 194.203 Hz
dw	0x2000, 1	; 145.652 Hz
dw	0x2800, 1	; 116.522 Hz
dw	0x3000, 1	; 97.101 Hz
dw	SOUND_TERMINATOR, 0

SOUND_TELEPORT:
dw	0x2000, 2	; 145.652 Hz
dw	0x1e00, 2	; 155.362 Hz
dw	0x1c00, 2	; 166.460 Hz
dw	0x1a00, 2	; 179.264 Hz
dw	0x1c00, 2	; 166.460 Hz
dw	0x1e00, 2	; 155.362 Hz
dw	0x2000, 2	; 145.652 Hz
dw	SOUND_TERMINATOR, 0

SOUND_SCORE_TALLY:
dw	0x1800, 1	; 194.203 Hz
dw	0x1600, 1	; 211.858 Hz
dw	SOUND_TERMINATOR, 0

; Convert a divisor D into a frequency as 1193182 / D.
SOUND_FREQ_94HZ		equ	0x3195	;  94.003 Hz
SOUND_FREQ_95HZ		equ	0x310f	;  95.006 Hz
SOUND_FREQ_96HZ		equ	0x308c	;  96.008 Hz
SOUND_FREQ_97HZ		equ	0x300c	;  97.007 Hz
SOUND_FREQ_98HZ		equ	0x2f8f	;  98.003 Hz
SOUND_FREQ_99HZ		equ	0x2f14	;  99.003 Hz
SOUND_FREQ_100HZ	equ	0x2e9b	; 100.007 Hz
SOUND_FREQ_105HZ	equ	0x2c63	; 105.006 Hz
SOUND_FREQ_110HZ	equ	0x2a5f	; 110.001 Hz
SOUND_FREQ_125HZ	equ	0x2549	; 125.006 Hz
SOUND_FREQ_130HZ	equ	0x23da	; 130.005 Hz
SOUND_FREQ_146HZ	equ	0x1fec	; 146.009 Hz
SOUND_FREQ_150HZ	equ	0x1f12	; 150.010 Hz
SOUND_FREQ_160HZ	equ	0x1d21	; 160.008 Hz
SOUND_FREQ_200HZ	equ	0x174d	; 200.031 Hz
SOUND_FREQ_210HZ	equ	0x1631	; 210.030 Hz
SOUND_FREQ_220HZ	equ	0x152f	; 220.022 Hz
SOUND_FREQ_230HZ	equ	0x1443	; 230.033 Hz
SOUND_FREQ_240HZ	equ	0x136b	; 240.029 Hz
SOUND_FREQ_250HZ	equ	0x12a4	; 250.038 Hz
SOUND_FREQ_300HZ	equ	0x0f89	; 300.021 Hz
SOUND_FREQ_400HZ	equ	0x0ba6	; 400.128 Hz
SOUND_FREQ_600HZ	equ	0x07c4	; 600.192 Hz
SOUND_FREQ_900HZ	equ	0x052d	; 900.515 Hz

SOUND_MATERIALIZE:
dw	SOUND_FREQ_200HZ, 1
dw	SOUND_FREQ_220HZ, 1
dw	SOUND_FREQ_210HZ, 1
dw	SOUND_FREQ_230HZ, 1
dw	SOUND_FREQ_220HZ, 1
dw	SOUND_FREQ_240HZ, 1
dw	SOUND_FREQ_900HZ, 1
dw	SOUND_FREQ_600HZ, 1
dw	SOUND_FREQ_400HZ, 1
dw	SOUND_FREQ_300HZ, 1
dw	SOUND_FREQ_250HZ, 1
dw	SOUND_FREQ_200HZ, 1
dw	SOUND_FREQ_150HZ, 1
dw	SOUND_FREQ_125HZ, 1
dw	SOUND_FREQ_110HZ, 1
dw	SOUND_FREQ_105HZ, 1
dw	SOUND_FREQ_100HZ, 1
dw	SOUND_FREQ_99HZ, 1
dw	SOUND_FREQ_98HZ, 1
dw	SOUND_FREQ_97HZ, 1
dw	SOUND_FREQ_96HZ, 1
dw	SOUND_FREQ_95HZ, 1
dw	SOUND_FREQ_94HZ, 1
dw	SOUND_TERMINATOR, 0

SOUND_GAME_OVER:
dw	NOTE_B3, 6
dw	NOTE_A3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_B3, 6
dw	NOTE_A3, 3
dw	NOTE_G3, 4
dw	NOTE_REST, 1
dw	NOTE_G3, 4
dw	NOTE_B3, 6
dw	NOTE_A3, 3
dw	NOTE_G3, 4
dw	NOTE_E3, 4
dw	NOTE_F3, 4
dw	NOTE_D3, 5
dw	NOTE_C3, 5
dw	SOUND_TERMINATOR, 0

alignb	16
offscreen_video_buffer_ptr	dw	0	; points to the video buffer not currently onscreen; alternates between 0x0000 and 0x2000
; comic_run_cycle cycles through COMIC_RUNNING_1, COMIC_RUNNING_2,
; COMIC_RUNNING_3, advancing by one step every game tick.
comic_run_cycle		db	0
comic_is_falling_or_jumping	db	0	; 0 when Comic is on the ground, 1 when falling or jumping
comic_is_teleporting	db	0
comic_x_momentum	db	0	; ranges from -5 to +5.
comic_y_vel		db	0	; fixed-point fractional value in units of 1/8 game units (1/16 tile or 1 pixel) per tick
comic_jump_counter	db	0	; a jump stops moving upwards when this counter decrements to 1
			db	0	; unused
comic_facing		db	COMIC_FACING_RIGHT
comic_animation		db	COMIC_STANDING

; How much HP is due to be given the player in the upcoming ticks.
comic_hp_pending_increase	db	MAX_HP	; initially set to fill HP at the start of the game
; After this variable becomes nonzero, it decrements on each tick and the game
; ends when it reaches 1.
win_counter		db	0
; fireball_meter depletes by 1 unit every 2 ticks while the fire key is being
; pressed, and recovers at the same rate when the fire key is not being
; pressed. fireball_meter_counter alternates 2, 1, 2, 1, .... fireball_meter
; decrements when fireball_meter_counter is 2 and increments when
; fireball_meter_counter is 1.
fireball_meter_counter	db	2

SOUND_STAGE_EDGE_TRANSITION:
dw	NOTE_F5, 3
dw	NOTE_A5, 3
dw	NOTE_G5, 5
dw	NOTE_F5, 5
dw	NOTE_E5, 5
dw	NOTE_F5, 5
dw	NOTE_G5, 5
dw	SOUND_TERMINATOR, 0

SOUND_TOO_BAD:
dw	SOUND_FREQ_130HZ, 5
dw	SOUND_FREQ_146HZ, 5
dw	SOUND_FREQ_130HZ, 5
dw	SOUND_FREQ_160HZ, 10
dw	SOUND_TERMINATOR, 0

alignb	16
; http://tasvideos.org/GameResources/DOS/CaptainComic.html#EnemyLogic
enemy_respawn_counter_cycle	db	20	; cycles 20, 40, 60, 80, 100, 20, ...
enemy_respawn_position_cycle	db	PLAYFIELD_WIDTH - 2	; cycles 24, 26, 28, 30, 24, ...
enemy_spawned_this_tick		db	0	; flag to ensure no more than one enemy is spawned per tick
; When inhibit_death_by_enemy_collision is nonzero, Comic will take damage from
; collision with enemies, but will not die, not even when comic_hp is 0. This
; is set during comic_death_animation to prevent enemies from "over-killing"
; Comic while the death animation is playing.
inhibit_death_by_enemy_collision	db	0

struc fireball
.y			resb	1
.x			resb	1
.vel			resb	1
.corkscrew_phase	resb	1
.animation		resb	1
.num_animation_frames	resb	1
endstruc

alignb	16
fireballs:
%rep MAX_NUM_FIREBALLS
istruc fireball
at fireball.y,		db	FIREBALL_DEAD
at fireball.x,		db	FIREBALL_DEAD
at fireball.num_animation_frames,	db	2
iend
%endrep

SOUND_FIRE:
dw	0x2000, 1	; 145.652 Hz
dw	0x1e00, 1	; 155.362 Hz
dw	SOUND_TERMINATOR, 0

SOUND_HIT_ENEMY:
dw	0x0800, 1	;  582.608 Hz
dw	0x0400, 1	; 1165.217 Hz
dw	SOUND_TERMINATOR, 0

alignb	16
item_animation_counter	db	0	; alternates between 0 and 1

SOUND_DAMAGE:
dw	0x3000, 2	; 97.101 Hz
dw	0x3800, 2	; 83.230 Hz
dw	0x4000, 2	; 72.826 Hz
dw	SOUND_TERMINATOR, 0

; The frequencies of SOUND_COLLECT_ITEM don't correspond to any tuning system I
; recognize; the frequencies increase in ratios of roughly a major third (402.6
; cents), a minor third (299.6 cents), and a perfect fourth (481.7 cents),
; spanning just short of an octave from lowest to highest.
SOUND_COLLECT_ITEM:
dw	0x0fda, 2	; 294.032 Hz
dw	0x0c90, 2	; 371.014 Hz
dw	0x0a91, 2	; 441.102 Hz
dw	0x0800, 2	; 582.608 Hz
dw	SOUND_TERMINATOR, 0

SOUND_EXTRA_LIFE:
dw	NOTE_B4, 5
dw	NOTE_D5, 6
dw	NOTE_B4, 2
dw	NOTE_C5, 5
dw	NOTE_D5, 5
dw	SOUND_TERMINATOR, 0

alignb	16
FILENAME_HIGH_SCORES	db	`COMIC.HGH\0`
FILENAME_HIGH_SCORES_GRAPHIC	db	`sys005.ega\0`

struc high_score
.name			resb	14
.score			resb	3
			resb	1	; always 0
endstruc

high_scores:
%rep NUM_HIGH_SCORES
istruc high_score
iend
%endrep

; Temporary space used for inserting a new score into the high scores table.
high_scores_tmp:
%rep NUM_HIGH_SCORES
istruc high_score
iend
%endrep

; This is a buffer structured for the DOS "buffered input" system call (int
; 0x21 with ah=0x0a).
high_score_name_buffer:
high_score_name_buffer_len:
db	13	; number of characters the buffer can hold
high_score_name_buffer_n:
db	0	; after the system call returns, this will be the number of characters read
high_score_name_buffer_text:
resb	13	; the read characters are stored here

STR_GREAT_SCORE		db	"Great Score!$"
STR_ENTER_YOUR_NAME	db	"Enter Your Name: $"
STR_00			db	"00$"
STR_BLANKS		db	"                              $"

; A zero score with a blank name, used to fill uninitialized entries in the
; high score table.
DEFAULT_HIGH_SCORE:
istruc high_score
at high_score.name,	db	"             $"
at high_score.score,	db	0, 0, 0
iend


section level_data
sectalign	16

%include	"levels.asm"


section input_config_strings
sectalign	16

load_fullscreen_graphic_buffer:
	resb	16002

STR_DEFINE_KEYS		db	`\n\n\n\n\n\n\r                                   Define Keys\n$`
STR_MOVE_LEFT		db	`\n\r                                Move Left  : $`
STR_MOVE_RIGHT		db	`\n\r                                Move Right : $`
STR_JUMP		db	`\n\r                                Jump       : $`
STR_FIREBALL		db	`\n\r                                Fireball   : $`
STR_OPEN_DOOR		db	`\n\r                                Open Door  : $`
STR_TELEPORT		db	`\n\r                                Teleport   : $`
STR_THIS_SETUP_OK	db	`\n\n\r                                This setup OK? (y/n)$`
STR_SAVE_SETUP_TO_DISK	db	`\n\r                             Save setup to disk? (y/n)$`

; Indices are off by one: SCANCODE_LABELS[0] is the label for scancode 1.
SCANCODE_LABELS:
	db	"Esc    $"
	db	"1      $"
	db	"2      $"
	db	"3      $"
	db	"4      $"
	db	"5      $"
	db	"6      $"
	db	"7      $"
	db	"8      $"
	db	"9      $"
	db	"0      $"
	db	"-      $"
	db	"=      $"
	db	"Back Sp$"
	db	"Tab    $"
	db	"Q      $"
	db	"W      $"
	db	"E      $"
	db	"R      $"
	db	"T      $"
	db	"Y      $"
	db	"U      $"
	db	"I      $"
	db	"O      $"
	db	"P      $"
	db	"[      $"
	db	"]      $"
	db	"Enter  $"
	db	"Ctrl   $"
	db	"A      $"
	db	"S      $"
	db	"D      $"
	db	"F      $"
	db	"G      $"
	db	"H      $"
	db	"J      $"
	db	"K      $"
	db	"L      $"
	db	";      $"
	db	"'      $"
	db	"`      $"
	db	"L Shift$"
	db	"\      $"
	db	"Z      $"
	db	"X      $"
	db	"C      $"
	db	"V      $"
	db	"B      $"
	db	"N      $"
	db	"M      $"
	db	",      $"
	db	".      $"
	db	"/      $"
	db	"R Shift$"
	db	"*      $"
	db	"Alt    $"
	db	"Space  $"
	db	"Caps   $"
	db	"F1     $"
	db	"F2     $"
	db	"F3     $"
	db	"F4     $"
	db	"F5     $"
	db	"F6     $"
	db	"F7     $"
	db	"F8     $"
	db	"F9     $"
	db	"F10    $"
	db	"NumLock$"
	db	"Scroll $"
	db	"Home   $"
	db	"Up     $"
	db	"PgUp   $"
	db	"- Key  $"
	db	"Left   $"
	db	"5 Key  $"
	db	"Right  $"
	db	"+ Key  $"
	db	"End    $"
	db	"Down   $"
	db	"PgDn   $"
	db	"Ins    $"
	db	"Del    $"

	times	14	db	0


; This sets the init_ss and init_sp fields in the MZ header.
section _ stack
sectalign	16

	resb	2000
