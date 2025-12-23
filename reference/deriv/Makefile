REVS = R1sw1988 R2sw1988 R3sw1989 R4sw1989 R5sw1991

EXES = $(addsuffix /unpacked.exe,$(filter-out R1swh21988 Kseni,$(REVS)))
EGA_PNGS = $(foreach s,sys000 sys001 sys002 sys003 sys004,$(addsuffix /$(s).ega.png,$(REVS))) \
	$(foreach s,sys005,$(addsuffix /$(s).ega.png,$(filter-out R1sw1988 R1swh11988 R1swh21988 Kseni,$(REVS))))
PT_PNGS = $(foreach level,base castle cave comp forest lake shed space,$(foreach n,0 1 2,$(addsuffix /$(level)$(n).pt.png,$(filter-out R1swh21988,$(REVS)))))
PT_SOLIDITY_PNGS = $(subst pt.png,pt.solidity.png,$(PT_PNGS))
SOUNDS = \
	$(foreach s,title door death teleport unknown materialize gameover transition toobad shoot pop damage itemget wereinthemoney,$(addsuffix /sound-$(s).wav,$(filter-out R1swh21988,$(REVS)))) \
	$(foreach s,start unused,$(addsuffix /sound-$(s).wav,$(filter-out R1swh21988 R5sw1991,$(REVS))))
SPRITES = $(foreach name, \
	$(foreach evenodd, even odd, $(addsuffix _$(evenodd), corkscrew doorkey boots lantern teleportwand gems crown gold cola shield)) \
	$(foreach facing, right left, $(addsuffix _$(facing), comic_standing comic_running_1 comic_running_2 comic_running_3 comic_jumping)) \
	$(foreach i,0 1 2 3 4 5 6 7, comic_death_$(i)) \
	$(foreach i,0 1 2, teleport_$(i)) \
	$(foreach i,0 1 2 3 4 5 6 7 8 9 10 11, materialize_$(i)) \
	$(foreach i,0 1, fireball_$(i)) \
	$(foreach i,0 1 2, white_spark_$(i) red_spark_$(i)) \
	$(foreach i,0 1 2 3 4 5 6 7 8 9, score_digit_$(i)) \
	meter_empty meter_half meter_full \
	life_icon_bright life_icon_dark pause game_over \
, $(addsuffix /sprite-$(name).png,$(filter-out R1swh21988,$(REVS))))
SHP_GIFS = $(foreach facing,left right, \
	$(foreach name,ball bee bird bird2 bug cube fb frog globe rock saucer star1 star2 star3, \
		$(addsuffix /$(name).shp-$(facing).gif,$(filter-out R1swh21988,$(REVS))))) \
	Kseni/bird.shp-extra.gif Kseni/bird2.shp-extra.gif Kseni/bug.shp-extra.gif Kseni/rock.shp-extra.gif

TILESETS = $(foreach level,base castle cave comp forest lake shed space,$(addsuffix /$(level).tt2,$(REVS)))

all: create_dirs $(EXES) $(EGA_PNGS) $(PT_PNGS) $(PT_SOLIDITY_PNGS) $(SOUNDS) $(SPRITES) $(SHP_GIFS) $(TILESETS)

create_dirs:
	mkdir -p $(REVS)

%/unpacked.exe: ../orig/%/COMIC.EXE
# exepack is from https://www.bamsoftware.com/software/exepack/
	exepack -d "$<" "$@"
Kseni/unpacked.exe: ../orig/Kseni/KSENI.COM
# KSENI.COM is actually an uncompressed COM file.
	mkdir -p Kseni
	cp -f "$<" "$@"

R1%/sys000.ega.png: RLE = -rle=false
R1%/sys001.ega.png: RLE = -rle=false
R1%/sys002.ega.png: RLE = -rle=false
R1%/sys003.ega.png: RLE = -rle=false
R1%/sys004.ega.png: RLE = -rle=false
Kseni/%.ega.png: RLE = -rle=false
%/sys000.ega.png: ../orig/%/SYS000.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"
%/sys001.ega.png: ../orig/%/SYS001.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"
%/sys002.ega.png: ../orig/%/SYS002.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"
%/sys003.ega.png: ../orig/%/SYS003.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"
%/sys004.ega.png: ../orig/%/SYS004.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"
%/sys005.ega.png: ../orig/%/SYS005.EGA
	../programs/unpack-ega/unpack-ega $(RLE) "$<" > "$@"

%/base0.pt.png: %/unpacked.exe ../orig/%/BASE0.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt 3 0 "$<" ../orig/"$*" > "$@"
%/base1.pt.png: %/unpacked.exe ../orig/%/BASE1.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt 3 1 "$<" ../orig/"$*" > "$@"
%/base2.pt.png: %/unpacked.exe ../orig/%/BASE2.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt 3 2 "$<" ../orig/"$*" > "$@"
%/castle0.pt.png: %/unpacked.exe ../orig/%/CASTLE0.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt 6 0 "$<" ../orig/"$*" > "$@"
%/castle1.pt.png: %/unpacked.exe ../orig/%/CASTLE1.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt 6 1 "$<" ../orig/"$*" > "$@"
%/castle2.pt.png: %/unpacked.exe ../orig/%/CASTLE2.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt 6 2 "$<" ../orig/"$*" > "$@"
%/cave0.pt.png: %/unpacked.exe ../orig/%/CAVE0.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt 4 0 "$<" ../orig/"$*" > "$@"
%/cave1.pt.png: %/unpacked.exe ../orig/%/CAVE1.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt 4 1 "$<" ../orig/"$*" > "$@"
%/cave2.pt.png: %/unpacked.exe ../orig/%/CAVE2.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt 4 2 "$<" ../orig/"$*" > "$@"
%/comp0.pt.png: %/unpacked.exe ../orig/%/COMP0.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt 7 0 "$<" ../orig/"$*" > "$@"
%/comp1.pt.png: %/unpacked.exe ../orig/%/COMP1.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt 7 1 "$<" ../orig/"$*" > "$@"
%/comp2.pt.png: %/unpacked.exe ../orig/%/COMP2.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt 7 2 "$<" ../orig/"$*" > "$@"
%/forest0.pt.png: %/unpacked.exe ../orig/%/FOREST0.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt 1 0 "$<" ../orig/"$*" > "$@"
%/forest1.pt.png: %/unpacked.exe ../orig/%/FOREST1.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt 1 1 "$<" ../orig/"$*" > "$@"
%/forest2.pt.png: %/unpacked.exe ../orig/%/FOREST2.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt 1 2 "$<" ../orig/"$*" > "$@"
%/lake0.pt.png: %/unpacked.exe ../orig/%/LAKE0.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt 0 0 "$<" ../orig/"$*" > "$@"
%/lake1.pt.png: %/unpacked.exe ../orig/%/LAKE1.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt 0 1 "$<" ../orig/"$*" > "$@"
%/lake2.pt.png: %/unpacked.exe ../orig/%/LAKE2.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt 0 2 "$<" ../orig/"$*" > "$@"
%/shed0.pt.png: %/unpacked.exe ../orig/%/SHED0.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt 5 0 "$<" ../orig/"$*" > "$@"
%/shed1.pt.png: %/unpacked.exe ../orig/%/SHED1.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt 5 1 "$<" ../orig/"$*" > "$@"
%/shed2.pt.png: %/unpacked.exe ../orig/%/SHED2.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt 5 2 "$<" ../orig/"$*" > "$@"
%/space0.pt.png: %/unpacked.exe ../orig/%/SPACE0.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt 2 0 "$<" ../orig/"$*" > "$@"
%/space1.pt.png: %/unpacked.exe ../orig/%/SPACE1.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt 2 1 "$<" ../orig/"$*" > "$@"
%/space2.pt.png: %/unpacked.exe ../orig/%/SPACE2.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt 2 2 "$<" ../orig/"$*" > "$@"

%/base0.pt.solidity.png: %/unpacked.exe ../orig/%/BASE0.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 3 0 "$<" ../orig/"$*" > "$@"
%/base1.pt.solidity.png: %/unpacked.exe ../orig/%/BASE1.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 3 1 "$<" ../orig/"$*" > "$@"
%/base2.pt.solidity.png: %/unpacked.exe ../orig/%/BASE2.PT ../orig/%/BASE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 3 2 "$<" ../orig/"$*" > "$@"
%/castle0.pt.solidity.png: %/unpacked.exe ../orig/%/CASTLE0.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 6 0 "$<" ../orig/"$*" > "$@"
%/castle1.pt.solidity.png: %/unpacked.exe ../orig/%/CASTLE1.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 6 1 "$<" ../orig/"$*" > "$@"
%/castle2.pt.solidity.png: %/unpacked.exe ../orig/%/CASTLE2.PT ../orig/%/CASTLE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 6 2 "$<" ../orig/"$*" > "$@"
%/cave0.pt.solidity.png: %/unpacked.exe ../orig/%/CAVE0.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 4 0 "$<" ../orig/"$*" > "$@"
%/cave1.pt.solidity.png: %/unpacked.exe ../orig/%/CAVE1.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 4 1 "$<" ../orig/"$*" > "$@"
%/cave2.pt.solidity.png: %/unpacked.exe ../orig/%/CAVE2.PT ../orig/%/CAVE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 4 2 "$<" ../orig/"$*" > "$@"
%/comp0.pt.solidity.png: %/unpacked.exe ../orig/%/COMP0.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 7 0 "$<" ../orig/"$*" > "$@"
%/comp1.pt.solidity.png: %/unpacked.exe ../orig/%/COMP1.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 7 1 "$<" ../orig/"$*" > "$@"
%/comp2.pt.solidity.png: %/unpacked.exe ../orig/%/COMP2.PT ../orig/%/COMP.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 7 2 "$<" ../orig/"$*" > "$@"
%/forest0.pt.solidity.png: %/unpacked.exe ../orig/%/FOREST0.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 1 0 "$<" ../orig/"$*" > "$@"
%/forest1.pt.solidity.png: %/unpacked.exe ../orig/%/FOREST1.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 1 1 "$<" ../orig/"$*" > "$@"
%/forest2.pt.solidity.png: %/unpacked.exe ../orig/%/FOREST2.PT ../orig/%/FOREST.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 1 2 "$<" ../orig/"$*" > "$@"
%/lake0.pt.solidity.png: %/unpacked.exe ../orig/%/LAKE0.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 0 0 "$<" ../orig/"$*" > "$@"
%/lake1.pt.solidity.png: %/unpacked.exe ../orig/%/LAKE1.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 0 1 "$<" ../orig/"$*" > "$@"
%/lake2.pt.solidity.png: %/unpacked.exe ../orig/%/LAKE2.PT ../orig/%/LAKE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 0 2 "$<" ../orig/"$*" > "$@"
%/shed0.pt.solidity.png: %/unpacked.exe ../orig/%/SHED0.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 5 0 "$<" ../orig/"$*" > "$@"
%/shed1.pt.solidity.png: %/unpacked.exe ../orig/%/SHED1.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 5 1 "$<" ../orig/"$*" > "$@"
%/shed2.pt.solidity.png: %/unpacked.exe ../orig/%/SHED2.PT ../orig/%/SHED.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 5 2 "$<" ../orig/"$*" > "$@"
%/space0.pt.solidity.png: %/unpacked.exe ../orig/%/SPACE0.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 2 0 "$<" ../orig/"$*" > "$@"
%/space1.pt.solidity.png: %/unpacked.exe ../orig/%/SPACE1.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 2 1 "$<" ../orig/"$*" > "$@"
%/space2.pt.solidity.png: %/unpacked.exe ../orig/%/SPACE2.PT ../orig/%/SPACE.TT2
	../programs/unpack-pt/unpack-pt -solidity -items 2 2 "$<" ../orig/"$*" > "$@"

%/base.tt2: ../orig/%/BASE.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/castle.tt2: ../orig/%/CASTLE.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/cave.tt2: ../orig/%/CAVE.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/comp.tt2: ../orig/%/COMP.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/forest.tt2: ../orig/%/FOREST.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/lake.tt2: ../orig/%/LAKE.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/shed.tt2: ../orig/%/SHED.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"
%/space.tt2: ../orig/%/SPACE.TT2
	../programs/unpack-tt2/unpack-tt2 "$<" "$@"

EXTRACT_SOUND = ../programs/extract-sound "$<" $(OFFSET) "$@"

R1sw1988/sound-title.wav: OFFSET = 0x2521
R1swh11988/sound-title.wav: OFFSET = 0x2521
R2sw1988/sound-title.wav: OFFSET = 0x25a2
R3sw1989/sound-title.wav: OFFSET = 0x25d2
R4a1sw1989/sound-title.wav: OFFSET = 0x27a2
R4sw1989/sound-title.wav: OFFSET = 0x27a2
R5sw1991/sound-title.wav: OFFSET = 0x29c4
Kseni/sound-title.wav: OFFSET = 0x2421
%/sound-title.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-start.wav: OFFSET = 0x2665
R1swh11988/sound-start.wav: OFFSET = 0x2665
R2sw1988/sound-start.wav: OFFSET = 0x2bc0
R3sw1989/sound-start.wav: OFFSET = 0x2bf2
R4a1sw1989/sound-start.wav: OFFSET = 0x2daa
R4sw1989/sound-start.wav: OFFSET = 0x2d9e
# No R5sw1991 version of this sound.
Kseni/sound-start.wav: OFFSET = 0x2565
%/sound-start.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-unused.wav: OFFSET = 0x26a5
R1swh11988/sound-unused.wav: OFFSET = 0x26a5
R2sw1988/sound-unused.wav: OFFSET = 0x2c00
R3sw1989/sound-unused.wav: OFFSET = 0x2c32
R4a1sw1989/sound-unused.wav: OFFSET = 0x2dea
R4sw1989/sound-unused.wav: OFFSET = 0x2dde
# No R5sw1991 version of this sound.
Kseni/sound-unused.wav: OFFSET = 0x25a5
%/sound-unused.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-door.wav: OFFSET = 0xeda9
R1swh11988/sound-door.wav: OFFSET = 0xeda9
R2sw1988/sound-door.wav: OFFSET = 0xf309
R3sw1989/sound-door.wav: OFFSET = 0xf339
R4a1sw1989/sound-door.wav: OFFSET = 0xf4f9
R4sw1989/sound-door.wav: OFFSET = 0xf4e9
R5sw1991/sound-door.wav: OFFSET = 0xfc29
Kseni/sound-door.wav: OFFSET = 0xeca9
%/sound-door.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-death.wav: OFFSET = 0xedd1
R1swh11988/sound-death.wav: OFFSET = 0xedd1
R2sw1988/sound-death.wav: OFFSET = 0xf331
R3sw1989/sound-death.wav: OFFSET = 0xf361
R4a1sw1989/sound-death.wav: OFFSET = 0xf521
R4sw1989/sound-death.wav: OFFSET = 0xf511
R5sw1991/sound-death.wav: OFFSET = 0xfc51
Kseni/sound-death.wav: OFFSET = 0xecd1
%/sound-death.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-teleport.wav: OFFSET = 0xedf9
R1swh11988/sound-teleport.wav: OFFSET = 0xedf9
R2sw1988/sound-teleport.wav: OFFSET = 0xf359
R3sw1989/sound-teleport.wav: OFFSET = 0xf389
R4a1sw1989/sound-teleport.wav: OFFSET = 0xf549
R4sw1989/sound-teleport.wav: OFFSET = 0xf539
R5sw1991/sound-teleport.wav: OFFSET = 0xfc79
Kseni/sound-teleport.wav: OFFSET = 0xecf9
%/sound-teleport.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-unknown.wav: OFFSET = 0xee19
R1swh11988/sound-unknown.wav: OFFSET = 0xee19
R2sw1988/sound-unknown.wav: OFFSET = 0xf379
R3sw1989/sound-unknown.wav: OFFSET = 0xf3a9
R4a1sw1989/sound-unknown.wav: OFFSET = 0xf569
R4sw1989/sound-unknown.wav: OFFSET = 0xf559
R5sw1991/sound-unknown.wav: OFFSET = 0xfc99
Kseni/sound-unknown.wav: OFFSET = 0xed19
%/sound-unknown.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-materialize.wav: OFFSET = 0xee25
R1swh11988/sound-materialize.wav: OFFSET = 0xee25
R2sw1988/sound-materialize.wav: OFFSET = 0xf385
R3sw1989/sound-materialize.wav: OFFSET = 0xf3b5
R4a1sw1989/sound-materialize.wav: OFFSET = 0xf575
R4sw1989/sound-materialize.wav: OFFSET = 0xf565
R5sw1991/sound-materialize.wav: OFFSET = 0xfca5
Kseni/sound-materialize.wav: OFFSET = 0xed25
%/sound-materialize.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-gameover.wav: OFFSET = 0xee85
R1swh11988/sound-gameover.wav: OFFSET = 0xee85
R2sw1988/sound-gameover.wav: OFFSET = 0xf3e5
R3sw1989/sound-gameover.wav: OFFSET = 0xf415
R4a1sw1989/sound-gameover.wav: OFFSET = 0xf5d5
R4sw1989/sound-gameover.wav: OFFSET = 0xf5c5
R5sw1991/sound-gameover.wav: OFFSET = 0xfd05
Kseni/sound-gameover.wav: OFFSET = 0xed85
%/sound-gameover.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-transition.wav: OFFSET = 0xeede
R1swh11988/sound-transition.wav: OFFSET = 0xeede
R2sw1988/sound-transition.wav: OFFSET = 0xf43e
R3sw1989/sound-transition.wav: OFFSET = 0xf46f
R4a1sw1989/sound-transition.wav: OFFSET = 0xf62f
R4sw1989/sound-transition.wav: OFFSET = 0xf61f
R5sw1991/sound-transition.wav: OFFSET = 0xfd3f
Kseni/sound-transition.wav: OFFSET = 0xedde
%/sound-transition.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-toobad.wav: OFFSET = 0xeefe
R1swh11988/sound-toobad.wav: OFFSET = 0xeefe
R2sw1988/sound-toobad.wav: OFFSET = 0xf45e
R3sw1989/sound-toobad.wav: OFFSET = 0xf48f
R4a1sw1989/sound-toobad.wav: OFFSET = 0xf64f
R4sw1989/sound-toobad.wav: OFFSET = 0xf63f
R5sw1991/sound-toobad.wav: OFFSET = 0xfd5f
Kseni/sound-toobad.wav: OFFSET = 0xedfe
%/sound-toobad.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-shoot.wav: OFFSET = 0xef4e
R1swh11988/sound-shoot.wav: OFFSET = 0xef4e
R2sw1988/sound-shoot.wav: OFFSET = 0xf4ae
R3sw1989/sound-shoot.wav: OFFSET = 0xf4de
R4a1sw1989/sound-shoot.wav: OFFSET = 0xf69e
R4sw1989/sound-shoot.wav: OFFSET = 0xf68e
R5sw1991/sound-shoot.wav: OFFSET = 0xfdae
Kseni/sound-shoot.wav: OFFSET = 0xee4e
%/sound-shoot.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-pop.wav: OFFSET = 0xef5a
R1swh11988/sound-pop.wav: OFFSET = 0xef5a
R2sw1988/sound-pop.wav: OFFSET = 0xf4ba
R3sw1989/sound-pop.wav: OFFSET = 0xf4ea
R4a1sw1989/sound-pop.wav: OFFSET = 0xf6aa
R4sw1989/sound-pop.wav: OFFSET = 0xf69a
R5sw1991/sound-pop.wav: OFFSET = 0xfdba
Kseni/sound-pop.wav: OFFSET = 0xee5a
%/sound-pop.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-damage.wav: OFFSET = 0xef71
R1swh11988/sound-damage.wav: OFFSET = 0xef71
R2sw1988/sound-damage.wav: OFFSET = 0xf4d1
R3sw1989/sound-damage.wav: OFFSET = 0xf501
R4a1sw1989/sound-damage.wav: OFFSET = 0xf6c1
R4sw1989/sound-damage.wav: OFFSET = 0xf6b1
R5sw1991/sound-damage.wav: OFFSET = 0xfdd1
Kseni/sound-damage.wav: OFFSET = 0xee71
%/sound-damage.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-itemget.wav: OFFSET = 0xef81
R1swh11988/sound-itemget.wav: OFFSET = 0xef81
R2sw1988/sound-itemget.wav: OFFSET = 0xf4e1
R3sw1989/sound-itemget.wav: OFFSET = 0xf511
R4a1sw1989/sound-itemget.wav: OFFSET = 0xf6d1
R4sw1989/sound-itemget.wav: OFFSET = 0xf6c1
R5sw1991/sound-itemget.wav: OFFSET = 0xfde1
Kseni/sound-itemget.wav: OFFSET = 0xee81
%/sound-itemget.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sound-wereinthemoney.wav: OFFSET = 0xef95
R1swh11988/sound-wereinthemoney.wav: OFFSET = 0xef95
R2sw1988/sound-wereinthemoney.wav: OFFSET = 0xf4f5
R3sw1989/sound-wereinthemoney.wav: OFFSET = 0xf525
R4a1sw1989/sound-wereinthemoney.wav: OFFSET = 0xf6e5
R4sw1989/sound-wereinthemoney.wav: OFFSET = 0xf6d5
R5sw1991/sound-wereinthemoney.wav: OFFSET = 0xfdf5
Kseni/sound-wereinthemoney.wav: OFFSET = 0xee95
%/sound-wereinthemoney.wav: %/unpacked.exe
	$(EXTRACT_SOUND)

R1sw1988/sprite-%.png: R1sw1988/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R1swh11988/sprite-%.png: R1swh11988/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R2sw1988/sprite-%.png: R2sw1988/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R3sw1989/sprite-%.png: R3sw1989/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R4sw1989/sprite-%.png: R4sw1989/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R4a1sw1989/sprite-%.png: R4a1sw1989/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
R5sw1991/sprite-%.png: R5sw1991/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"
Kseni/sprite-%.png: Kseni/unpacked.exe
	../programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$@"

R1sw1988/sprite-%.png:   SPRITES_START = 0x8e80
R1swh11988/sprite-%.png: SPRITES_START = 0x8e80
R2sw1988/sprite-%.png:   SPRITES_START = 0x93e0
R3sw1989/sprite-%.png:   SPRITES_START = 0x9410
R4sw1989/sprite-%.png:   SPRITES_START = 0x95c0
R4a1sw1989/sprite-%.png: SPRITES_START = 0x95d0
R5sw1991/sprite-%.png:   SPRITES_START = 0x9d00
Kseni/sprite-%.png:      SPRITES_START = 0x8d80

%/sprite-corkscrew_even.png \
%/sprite-doorkey_even.png \
%/sprite-boots_even.png \
%/sprite-lantern_even.png \
%/sprite-teleportwand_even.png \
%/sprite-gems_even.png \
%/sprite-crown_even.png \
%/sprite-gold_even.png \
%/sprite-cola_even.png \
%/sprite-shield_even.png \
%/sprite-corkscrew_odd.png \
%/sprite-doorkey_odd.png \
%/sprite-boots_odd.png \
%/sprite-lantern_odd.png \
%/sprite-teleportwand_odd.png \
%/sprite-gems_odd.png \
%/sprite-crown_odd.png \
%/sprite-gold_odd.png \
%/sprite-cola_odd.png \
%/sprite-shield_odd.png \
%/sprite-life_icon_bright.png \
%/sprite-life_icon_dark.png \
: DIMENSIONS = 16x16

%/sprite-comic_standing_right.png \
%/sprite-comic_running_1_right.png \
%/sprite-comic_running_2_right.png \
%/sprite-comic_running_3_right.png \
%/sprite-comic_jumping_right.png \
%/sprite-comic_standing_left.png \
%/sprite-comic_running_1_left.png \
%/sprite-comic_running_2_left.png \
%/sprite-comic_running_3_left.png \
%/sprite-comic_jumping_left.png \
%/sprite-comic_death_0.png \
%/sprite-comic_death_1.png \
%/sprite-comic_death_2.png \
%/sprite-comic_death_3.png \
%/sprite-comic_death_4.png \
%/sprite-comic_death_5.png \
%/sprite-comic_death_6.png \
%/sprite-comic_death_7.png \
%/sprite-teleport_0.png \
%/sprite-teleport_1.png \
%/sprite-teleport_2.png \
%/sprite-materialize_0.png \
%/sprite-materialize_1.png \
%/sprite-materialize_2.png \
%/sprite-materialize_3.png \
%/sprite-materialize_4.png \
%/sprite-materialize_5.png \
%/sprite-materialize_6.png \
%/sprite-materialize_7.png \
%/sprite-materialize_8.png \
%/sprite-materialize_9.png \
%/sprite-materialize_10.png \
%/sprite-materialize_11.png \
: DIMENSIONS = 16x32

%/sprite-fireball_0.png \
%/sprite-fireball_1.png \
: DIMENSIONS = 16x8

%/sprite-white_spark_0.png \
%/sprite-white_spark_1.png \
%/sprite-white_spark_2.png \
%/sprite-red_spark_0.png \
%/sprite-red_spark_1.png \
%/sprite-red_spark_2.png \
: DIMENSIONS = 16x16

%/sprite-score_digit_0.png \
%/sprite-score_digit_1.png \
%/sprite-score_digit_2.png \
%/sprite-score_digit_3.png \
%/sprite-score_digit_4.png \
%/sprite-score_digit_5.png \
%/sprite-score_digit_6.png \
%/sprite-score_digit_7.png \
%/sprite-score_digit_8.png \
%/sprite-score_digit_9.png \
%/sprite-meter_empty.png \
%/sprite-meter_half.png \
%/sprite-meter_full.png \
: DIMENSIONS = 8x16

%/sprite-pause.png \
%/sprite-game_over.png \
: DIMENSIONS = 128x48

%/sprite-corkscrew_even.png \
%/sprite-doorkey_even.png \
%/sprite-boots_even.png \
%/sprite-lantern_even.png \
%/sprite-teleportwand_even.png \
%/sprite-gems_even.png \
%/sprite-crown_even.png \
%/sprite-gold_even.png \
%/sprite-cola_even.png \
%/sprite-shield_even.png \
%/sprite-corkscrew_odd.png \
%/sprite-doorkey_odd.png \
%/sprite-boots_odd.png \
%/sprite-lantern_odd.png \
%/sprite-teleportwand_odd.png \
%/sprite-gems_odd.png \
%/sprite-crown_odd.png \
%/sprite-gold_odd.png \
%/sprite-cola_odd.png \
%/sprite-shield_odd.png \
%/sprite-life_icon_bright.png \
%/sprite-life_icon_dark.png \
%/sprite-comic_standing_right.png \
%/sprite-comic_running_1_right.png \
%/sprite-comic_running_2_right.png \
%/sprite-comic_running_3_right.png \
%/sprite-comic_jumping_right.png \
%/sprite-comic_standing_left.png \
%/sprite-comic_running_1_left.png \
%/sprite-comic_running_2_left.png \
%/sprite-comic_running_3_left.png \
%/sprite-comic_jumping_left.png \
%/sprite-comic_death_0.png \
%/sprite-comic_death_1.png \
%/sprite-comic_death_2.png \
%/sprite-comic_death_3.png \
%/sprite-comic_death_4.png \
%/sprite-comic_death_5.png \
%/sprite-comic_death_6.png \
%/sprite-comic_death_7.png \
%/sprite-teleport_0.png \
%/sprite-teleport_1.png \
%/sprite-teleport_2.png \
%/sprite-materialize_0.png \
%/sprite-materialize_1.png \
%/sprite-materialize_2.png \
%/sprite-materialize_3.png \
%/sprite-materialize_4.png \
%/sprite-materialize_5.png \
%/sprite-materialize_6.png \
%/sprite-materialize_7.png \
%/sprite-materialize_8.png \
%/sprite-materialize_9.png \
%/sprite-materialize_10.png \
%/sprite-materialize_11.png \
%/sprite-fireball_0.png \
%/sprite-fireball_1.png \
%/sprite-white_spark_0.png \
%/sprite-white_spark_1.png \
%/sprite-white_spark_2.png \
%/sprite-red_spark_0.png \
%/sprite-red_spark_1.png \
%/sprite-red_spark_2.png \
: MASK = -mask

%/sprite-pause.png \
%/sprite-game_over.png \
%/sprite-score_digit_0.png \
%/sprite-score_digit_1.png \
%/sprite-score_digit_2.png \
%/sprite-score_digit_3.png \
%/sprite-score_digit_4.png \
%/sprite-score_digit_5.png \
%/sprite-score_digit_6.png \
%/sprite-score_digit_7.png \
%/sprite-score_digit_8.png \
%/sprite-score_digit_9.png \
%/sprite-meter_empty.png \
%/sprite-meter_half.png \
%/sprite-meter_full.png \
: MASK =

%/sprite-corkscrew_even.png:    OFFSET = 0x2dc4+0x000+160*0
%/sprite-doorkey_even.png:      OFFSET = 0x2dc4+0x000+160*1
%/sprite-boots_even.png:        OFFSET = 0x2dc4+0x000+160*2
%/sprite-lantern_even.png:      OFFSET = 0x2dc4+0x000+160*3
%/sprite-teleportwand_even.png: OFFSET = 0x2dc4+0x000+160*4
%/sprite-gems_even.png:         OFFSET = 0x2dc4+0x000+160*5
%/sprite-crown_even.png:        OFFSET = 0x2dc4+0x000+160*6
%/sprite-gold_even.png:         OFFSET = 0x2dc4+0x000+160*7
%/sprite-cola_even.png:         OFFSET = 0x2dc4+0x000+160*8
%/sprite-shield_even.png:       OFFSET = 0x2dc4+0x000+160*14
%/sprite-life_icon_bright.png:  OFFSET = 0x2dc4+0x000+160*15
%/sprite-corkscrew_odd.png:     OFFSET = 0x2dc4+0xa00+160*0
%/sprite-doorkey_odd.png:       OFFSET = 0x2dc4+0xa00+160*1
%/sprite-boots_odd.png:         OFFSET = 0x2dc4+0xa00+160*2
%/sprite-lantern_odd.png:       OFFSET = 0x2dc4+0xa00+160*3
%/sprite-teleportwand_odd.png:  OFFSET = 0x2dc4+0xa00+160*4
%/sprite-gems_odd.png:          OFFSET = 0x2dc4+0xa00+160*5
%/sprite-crown_odd.png:         OFFSET = 0x2dc4+0xa00+160*6
%/sprite-gold_odd.png:          OFFSET = 0x2dc4+0xa00+160*7
%/sprite-cola_odd.png:          OFFSET = 0x2dc4+0xa00+160*8
%/sprite-shield_odd.png:        OFFSET = 0x2dc4+0xa00+160*14
%/sprite-life_icon_dark.png:    OFFSET = 0x2dc4+0xa00+160*15

%/sprite-comic_standing_right.png:  OFFSET = 0x0000+320*0
%/sprite-comic_running_1_right.png: OFFSET = 0x0000+320*1
%/sprite-comic_running_2_right.png: OFFSET = 0x0000+320*2
%/sprite-comic_running_3_right.png: OFFSET = 0x0000+320*3
%/sprite-comic_jumping_right.png:   OFFSET = 0x0000+320*4
%/sprite-comic_standing_left.png:   OFFSET = 0x0000+320*5
%/sprite-comic_running_1_left.png:  OFFSET = 0x0000+320*6
%/sprite-comic_running_2_left.png:  OFFSET = 0x0000+320*7
%/sprite-comic_running_3_left.png:  OFFSET = 0x0000+320*8
%/sprite-comic_jumping_left.png:    OFFSET = 0x0000+320*9

%/sprite-pause.png:     OFFSET = 0x4504
%/sprite-game_over.png: OFFSET = 0x5104

%/sprite-comic_death_0.png: OFFSET = 0x10e0+0*320
%/sprite-comic_death_1.png: OFFSET = 0x10e0+1*320
%/sprite-comic_death_2.png: OFFSET = 0x10e0+2*320
%/sprite-comic_death_3.png: OFFSET = 0x10e0+3*320
%/sprite-comic_death_4.png: OFFSET = 0x10e0+4*320
%/sprite-comic_death_5.png: OFFSET = 0x10e0+5*320
%/sprite-comic_death_6.png: OFFSET = 0x10e0+6*320
%/sprite-comic_death_7.png: OFFSET = 0x10e0+7*320

%/sprite-teleport_0.png: OFFSET = 0x1ae0+0*320
%/sprite-teleport_1.png: OFFSET = 0x1ae0+1*320
%/sprite-teleport_2.png: OFFSET = 0x1ae0+2*320

%/sprite-materialize_0.png:  OFFSET = 0x1ea0+0*320
%/sprite-materialize_1.png:  OFFSET = 0x1ea0+1*320
%/sprite-materialize_2.png:  OFFSET = 0x1ea0+2*320
%/sprite-materialize_3.png:  OFFSET = 0x1ea0+3*320
%/sprite-materialize_4.png:  OFFSET = 0x1ea0+4*320
%/sprite-materialize_5.png:  OFFSET = 0x1ea0+5*320
%/sprite-materialize_6.png:  OFFSET = 0x1ea0+6*320
%/sprite-materialize_7.png:  OFFSET = 0x1ea0+7*320
%/sprite-materialize_8.png:  OFFSET = 0x1ea0+8*320
%/sprite-materialize_9.png:  OFFSET = 0x1ea0+9*320
%/sprite-materialize_10.png: OFFSET = 0x1ea0+10*320
%/sprite-materialize_11.png: OFFSET = 0x1ea0+11*320

%/sprite-fireball_0.png: OFFSET = 0x0c80+0*80
%/sprite-fireball_1.png: OFFSET = 0x0c80+1*80

%/sprite-white_spark_0.png: OFFSET = 0x0d20+0*160
%/sprite-white_spark_1.png: OFFSET = 0x0d20+1*160
%/sprite-white_spark_2.png: OFFSET = 0x0d20+2*160

%/sprite-red_spark_0.png: OFFSET = 0x0f00+0*160
%/sprite-red_spark_1.png: OFFSET = 0x0f00+1*160
%/sprite-red_spark_2.png: OFFSET = 0x0f00+2*160

%/sprite-score_digit_0.png: OFFSET = 0x41c4+0*64
%/sprite-score_digit_1.png: OFFSET = 0x41c4+1*64
%/sprite-score_digit_2.png: OFFSET = 0x41c4+2*64
%/sprite-score_digit_3.png: OFFSET = 0x41c4+3*64
%/sprite-score_digit_4.png: OFFSET = 0x41c4+4*64
%/sprite-score_digit_5.png: OFFSET = 0x41c4+5*64
%/sprite-score_digit_6.png: OFFSET = 0x41c4+6*64
%/sprite-score_digit_7.png: OFFSET = 0x41c4+7*64
%/sprite-score_digit_8.png: OFFSET = 0x41c4+8*64
%/sprite-score_digit_9.png: OFFSET = 0x41c4+9*64

%/sprite-meter_empty.png: OFFSET = 0x4444
%/sprite-meter_half.png:  OFFSET = 0x4484
%/sprite-meter_full.png:  OFFSET = 0x44c4

%/ball.shp-left.gif: %/unpacked.exe ../orig/%/BALL.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/ball.shp-right.gif: %/unpacked.exe ../orig/%/BALL.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/bee.shp-left.gif: %/unpacked.exe ../orig/%/BEE.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/bee.shp-right.gif: %/unpacked.exe ../orig/%/BEE.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/bird.shp-left.gif: %/unpacked.exe ../orig/%/BIRD.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/bird.shp-right.gif: %/unpacked.exe ../orig/%/BIRD.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/bird.shp-extra.gif: %/unpacked.exe ../orig/%/BIRD.SHP
	../programs/unpack-shp/unpack-shp $^ extra > "$@"
%/bird2.shp-left.gif: %/unpacked.exe ../orig/%/BIRD2.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/bird2.shp-right.gif: %/unpacked.exe ../orig/%/BIRD2.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/bird2.shp-extra.gif: %/unpacked.exe ../orig/%/BIRD2.SHP
	../programs/unpack-shp/unpack-shp $^ extra > "$@"
%/bug.shp-left.gif: %/unpacked.exe ../orig/%/BUG.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/bug.shp-right.gif: %/unpacked.exe ../orig/%/BUG.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/bug.shp-extra.gif: %/unpacked.exe ../orig/%/BUG.SHP
	../programs/unpack-shp/unpack-shp $^ extra > "$@"
%/cube.shp-left.gif: %/unpacked.exe ../orig/%/CUBE.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/cube.shp-right.gif: %/unpacked.exe ../orig/%/CUBE.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/fb.shp-left.gif: %/unpacked.exe ../orig/%/FB.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/fb.shp-right.gif: %/unpacked.exe ../orig/%/FB.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/frog.shp-left.gif: %/unpacked.exe ../orig/%/FROG.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/frog.shp-right.gif: %/unpacked.exe ../orig/%/FROG.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/globe.shp-left.gif: %/unpacked.exe ../orig/%/GLOBE.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/globe.shp-right.gif: %/unpacked.exe ../orig/%/GLOBE.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/rock.shp-left.gif: %/unpacked.exe ../orig/%/ROCK.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/rock.shp-right.gif: %/unpacked.exe ../orig/%/ROCK.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/rock.shp-extra.gif: %/unpacked.exe ../orig/%/ROCK.SHP
	../programs/unpack-shp/unpack-shp $^ extra > "$@"
%/saucer.shp-left.gif: %/unpacked.exe ../orig/%/SAUCER.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/saucer.shp-right.gif: %/unpacked.exe ../orig/%/SAUCER.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/star1.shp-left.gif: %/unpacked.exe ../orig/%/STAR1.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/star1.shp-right.gif: %/unpacked.exe ../orig/%/STAR1.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/star2.shp-left.gif: %/unpacked.exe ../orig/%/STAR2.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/star2.shp-right.gif: %/unpacked.exe ../orig/%/STAR2.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"
%/star3.shp-left.gif: %/unpacked.exe ../orig/%/STAR3.SHP
	../programs/unpack-shp/unpack-shp $^ left > "$@"
%/star3.shp-right.gif: %/unpacked.exe ../orig/%/STAR3.SHP
	../programs/unpack-shp/unpack-shp $^ right > "$@"

.PHONY: all
.DELETE_ON_ERROR:
