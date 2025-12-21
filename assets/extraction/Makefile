EXTRACTED_DIR = ../extracted
ORIGINAL_DIR = ../original

EXES = unpacked.exe
EGA_PNGS = $(foreach s,sys000 sys001 sys002 sys003 sys004 sys005,$(s).ega.png)
PT_PNGS = $(foreach level,base castle cave comp forest lake shed space,$(foreach n,0 1 2,$(level)$(n).pt.png))
PT_SOLIDITY_PNGS = $(subst pt.png,pt.solidity.png,$(PT_PNGS))
SOUNDS = \
	$(foreach s,title door death teleport unknown materialize gameover transition toobad shoot pop damage itemget wereinthemoney,sound-$(s).wav) \
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
,sprite-$(name).png)
SHP_GIFS = $(foreach facing,left right, \
	$(foreach name,ball bee bird bird2 bug cube fb frog globe rock saucer star1 star2 star3, \
		$(name).shp-$(facing).gif)) 

TILESETS = $(foreach level,base castle cave comp forest lake shed space,$(level).tt2)

all: $(EXES) $(EGA_PNGS) $(PT_PNGS) $(PT_SOLIDITY_PNGS) $(SOUNDS) $(SPRITES) $(SHP_GIFS) $(TILESETS)

# exepack is from https://www.bamsoftware.com/software/exepack/
unpacked.exe: $(ORIGINAL_DIR)/COMIC.EXE
	mkdir -p $(EXTRACTED_DIR)
	exepack -d "$<" "$(EXTRACTED_DIR)/$@"

sys000.ega.png: $(ORIGINAL_DIR)/SYS000.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"
sys001.ega.png: $(ORIGINAL_DIR)/SYS001.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"
sys002.ega.png: $(ORIGINAL_DIR)/SYS002.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"
sys003.ega.png: $(ORIGINAL_DIR)/SYS003.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"
sys004.ega.png: $(ORIGINAL_DIR)/SYS004.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"
sys005.ega.png: $(ORIGINAL_DIR)/SYS005.EGA
	./programs/unpack-ega/unpack-ega $(RLE) "$<" > "$(EXTRACTED_DIR)/$@"

base0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE0.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt 3 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
base1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE1.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt 3 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
base2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE2.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt 3 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE0.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt 6 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE1.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt 6 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE2.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt 6 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE0.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt 4 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE1.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt 4 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE2.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt 4 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP0.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt 7 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP1.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt 7 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP2.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt 7 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST0.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt 1 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST1.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt 1 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST2.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt 1 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE0.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt 0 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE1.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt 0 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE2.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt 0 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED0.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt 5 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED1.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt 5 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED2.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt 5 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space0.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE0.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt 2 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space1.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE1.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt 2 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space2.pt.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE2.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt 2 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"

base0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE0.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 3 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
base1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE1.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 3 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
base2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BASE2.PT $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 3 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE0.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 6 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE1.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 6 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
castle2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CASTLE2.PT $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 6 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE0.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 4 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE1.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 4 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
cave2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CAVE2.PT $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 4 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP0.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 7 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP1.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 7 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
comp2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/COMP2.PT $(ORIGINAL_DIR)/COMP.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 7 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST0.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 1 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST1.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 1 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
forest2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FOREST2.PT $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 1 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE0.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 0 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE1.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 0 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
lake2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/LAKE2.PT $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 0 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED0.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 5 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED1.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 5 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
shed2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SHED2.PT $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 5 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space0.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE0.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 2 0 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space1.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE1.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 2 1 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"
space2.pt.solidity.png: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SPACE2.PT $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-pt/unpack-pt -solidity -items 2 2 "$<" $(ORIGINAL_DIR)/"$*" > "$(EXTRACTED_DIR)/$@"

base.tt2: $(ORIGINAL_DIR)/BASE.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
castle.tt2: $(ORIGINAL_DIR)/CASTLE.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
cave.tt2: $(ORIGINAL_DIR)/CAVE.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
comp.tt2: $(ORIGINAL_DIR)/COMP.TT2
		./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
forest.tt2: $(ORIGINAL_DIR)/FOREST.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
lake.tt2: $(ORIGINAL_DIR)/LAKE.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
shed.tt2: $(ORIGINAL_DIR)/SHED.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"
space.tt2: $(ORIGINAL_DIR)/SPACE.TT2
	./programs/unpack-tt2/unpack-tt2 "$<" "$(EXTRACTED_DIR)/$@"

EXTRACT_SOUND = ./programs/extract-sound "$<" $(OFFSET) "$(EXTRACTED_DIR)/$@"

sound-title.wav: OFFSET = 0x25d2
sound-title.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-start.wav: OFFSET = 0x2bf2
# No R5sw1991 version of this sound.
sound-start.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-unused.wav: OFFSET = 0x2c32
# No R5sw1991 version of this sound.
sound-unused.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-door.wav: OFFSET = 0xf339
sound-door.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-death.wav: OFFSET = 0xf361
sound-death.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-teleport.wav: OFFSET = 0xf389
sound-teleport.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-unknown.wav: OFFSET = 0xf3a9
sound-unknown.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-materialize.wav: OFFSET = 0xf3b5
sound-materialize.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-gameover.wav: OFFSET = 0xf415
sound-gameover.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-transition.wav: OFFSET = 0xf46f
sound-transition.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-toobad.wav: OFFSET = 0xf48f
sound-toobad.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-shoot.wav: OFFSET = 0xf4de
sound-shoot.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-pop.wav: OFFSET = 0xf4ea
sound-pop.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-damage.wav: OFFSET = 0xf501
sound-damage.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-itemget.wav: OFFSET = 0xf511
sound-itemget.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sound-wereinthemoney.wav: OFFSET = 0xf525
sound-wereinthemoney.wav: $(EXTRACTED_DIR)/unpacked.exe
	$(EXTRACT_SOUND)

sprite-%.png: $(EXTRACTED_DIR)/unpacked.exe
	./programs/unpack-game-graphic/unpack-game-graphic $(MASK) "$<" $$(($(SPRITES_START)+$(OFFSET)+512)) "$(DIMENSIONS)" > "$(EXTRACTED_DIR)/$@"

sprite-%.png:   SPRITES_START = 0x9410

sprite-corkscrew_even.png \
sprite-doorkey_even.png \
sprite-boots_even.png \
sprite-lantern_even.png \
sprite-teleportwand_even.png \
sprite-gems_even.png \
sprite-crown_even.png \
sprite-gold_even.png \
sprite-cola_even.png \
sprite-shield_even.png \
sprite-corkscrew_odd.png \
sprite-doorkey_odd.png \
sprite-boots_odd.png \
sprite-lantern_odd.png \
sprite-teleportwand_odd.png \
sprite-gems_odd.png \
sprite-crown_odd.png \
sprite-gold_odd.png \
sprite-cola_odd.png \
sprite-shield_odd.png \
sprite-life_icon_bright.png \
sprite-life_icon_dark.png \
: DIMENSIONS = 16x16

sprite-comic_standing_right.png \
sprite-comic_running_1_right.png \
sprite-comic_running_2_right.png \
sprite-comic_running_3_right.png \
sprite-comic_jumping_right.png \
sprite-comic_standing_left.png \
sprite-comic_running_1_left.png \
sprite-comic_running_2_left.png \
sprite-comic_running_3_left.png \
sprite-comic_jumping_left.png \
sprite-comic_death_0.png \
sprite-comic_death_1.png \
sprite-comic_death_2.png \
sprite-comic_death_3.png \
sprite-comic_death_4.png \
sprite-comic_death_5.png \
sprite-comic_death_6.png \
sprite-comic_death_7.png \
sprite-teleport_0.png \
sprite-teleport_1.png \
sprite-teleport_2.png \
sprite-materialize_0.png \
sprite-materialize_1.png \
sprite-materialize_2.png \
sprite-materialize_3.png \
sprite-materialize_4.png \
sprite-materialize_5.png \
sprite-materialize_6.png \
sprite-materialize_7.png \
sprite-materialize_8.png \
sprite-materialize_9.png \
sprite-materialize_10.png \
sprite-materialize_11.png \
: DIMENSIONS = 16x32

sprite-fireball_0.png \
sprite-fireball_1.png \
: DIMENSIONS = 16x8

sprite-white_spark_0.png \
sprite-white_spark_1.png \
sprite-white_spark_2.png \
sprite-red_spark_0.png \
sprite-red_spark_1.png \
sprite-red_spark_2.png \
: DIMENSIONS = 16x16

sprite-score_digit_0.png \
sprite-score_digit_1.png \
sprite-score_digit_2.png \
sprite-score_digit_3.png \
sprite-score_digit_4.png \
sprite-score_digit_5.png \
sprite-score_digit_6.png \
sprite-score_digit_7.png \
sprite-score_digit_8.png \
sprite-score_digit_9.png \
sprite-meter_empty.png \
sprite-meter_half.png \
sprite-meter_full.png \
: DIMENSIONS = 8x16

sprite-pause.png \
sprite-game_over.png \
: DIMENSIONS = 128x48

sprite-corkscrew_even.png \
sprite-doorkey_even.png \
sprite-boots_even.png \
sprite-lantern_even.png \
sprite-teleportwand_even.png \
sprite-gems_even.png \
sprite-crown_even.png \
sprite-gold_even.png \
sprite-cola_even.png \
sprite-shield_even.png \
sprite-corkscrew_odd.png \
sprite-doorkey_odd.png \
sprite-boots_odd.png \
sprite-lantern_odd.png \
sprite-teleportwand_odd.png \
sprite-gems_odd.png \
sprite-crown_odd.png \
sprite-gold_odd.png \
sprite-cola_odd.png \
sprite-shield_odd.png \
sprite-life_icon_bright.png \
sprite-life_icon_dark.png \
sprite-comic_standing_right.png \
sprite-comic_running_1_right.png \
sprite-comic_running_2_right.png \
sprite-comic_running_3_right.png \
sprite-comic_jumping_right.png \
sprite-comic_standing_left.png \
sprite-comic_running_1_left.png \
sprite-comic_running_2_left.png \
sprite-comic_running_3_left.png \
sprite-comic_jumping_left.png \
sprite-comic_death_0.png \
sprite-comic_death_1.png \
sprite-comic_death_2.png \
sprite-comic_death_3.png \
sprite-comic_death_4.png \
sprite-comic_death_5.png \
sprite-comic_death_6.png \
sprite-comic_death_7.png \
sprite-teleport_0.png \
sprite-teleport_1.png \
sprite-teleport_2.png \
sprite-materialize_0.png \
sprite-materialize_1.png \
sprite-materialize_2.png \
sprite-materialize_3.png \
sprite-materialize_4.png \
sprite-materialize_5.png \
sprite-materialize_6.png \
sprite-materialize_7.png \
sprite-materialize_8.png \
sprite-materialize_9.png \
sprite-materialize_10.png \
sprite-materialize_11.png \
sprite-fireball_0.png \
sprite-fireball_1.png \
sprite-white_spark_0.png \
sprite-white_spark_1.png \
sprite-white_spark_2.png \
sprite-red_spark_0.png \
sprite-red_spark_1.png \
sprite-red_spark_2.png \
: MASK = -mask

sprite-pause.png \
sprite-game_over.png \
sprite-score_digit_0.png \
sprite-score_digit_1.png \
sprite-score_digit_2.png \
sprite-score_digit_3.png \
sprite-score_digit_4.png \
sprite-score_digit_5.png \
sprite-score_digit_6.png \
sprite-score_digit_7.png \
sprite-score_digit_8.png \
sprite-score_digit_9.png \
sprite-meter_empty.png \
sprite-meter_half.png \
sprite-meter_full.png \
: MASK =

sprite-corkscrew_even.png:    OFFSET = 0x2dc4+0x000+160*0
sprite-doorkey_even.png:      OFFSET = 0x2dc4+0x000+160*1
sprite-boots_even.png:        OFFSET = 0x2dc4+0x000+160*2
sprite-lantern_even.png:      OFFSET = 0x2dc4+0x000+160*3
sprite-teleportwand_even.png: OFFSET = 0x2dc4+0x000+160*4
sprite-gems_even.png:         OFFSET = 0x2dc4+0x000+160*5
sprite-crown_even.png:        OFFSET = 0x2dc4+0x000+160*6
sprite-gold_even.png:         OFFSET = 0x2dc4+0x000+160*7
sprite-cola_even.png:         OFFSET = 0x2dc4+0x000+160*8
sprite-shield_even.png:       OFFSET = 0x2dc4+0x000+160*14
sprite-life_icon_bright.png:  OFFSET = 0x2dc4+0x000+160*15
sprite-corkscrew_odd.png:     OFFSET = 0x2dc4+0xa00+160*0
sprite-doorkey_odd.png:       OFFSET = 0x2dc4+0xa00+160*1
sprite-boots_odd.png:         OFFSET = 0x2dc4+0xa00+160*2
sprite-lantern_odd.png:       OFFSET = 0x2dc4+0xa00+160*3
sprite-teleportwand_odd.png:  OFFSET = 0x2dc4+0xa00+160*4
sprite-gems_odd.png:          OFFSET = 0x2dc4+0xa00+160*5
sprite-crown_odd.png:         OFFSET = 0x2dc4+0xa00+160*6
sprite-gold_odd.png:          OFFSET = 0x2dc4+0xa00+160*7
sprite-cola_odd.png:          OFFSET = 0x2dc4+0xa00+160*8
sprite-shield_odd.png:        OFFSET = 0x2dc4+0xa00+160*14
sprite-life_icon_dark.png:    OFFSET = 0x2dc4+0xa00+160*15

sprite-comic_standing_right.png:  OFFSET = 0x0000+320*0
sprite-comic_running_1_right.png: OFFSET = 0x0000+320*1
sprite-comic_running_2_right.png: OFFSET = 0x0000+320*2
sprite-comic_running_3_right.png: OFFSET = 0x0000+320*3
sprite-comic_jumping_right.png:   OFFSET = 0x0000+320*4
sprite-comic_standing_left.png:   OFFSET = 0x0000+320*5
sprite-comic_running_1_left.png:  OFFSET = 0x0000+320*6
sprite-comic_running_2_left.png:  OFFSET = 0x0000+320*7
sprite-comic_running_3_left.png:  OFFSET = 0x0000+320*8
sprite-comic_jumping_left.png:    OFFSET = 0x0000+320*9

sprite-pause.png:     OFFSET = 0x4504
sprite-game_over.png: OFFSET = 0x5104

sprite-comic_death_0.png: OFFSET = 0x10e0+0*320
sprite-comic_death_1.png: OFFSET = 0x10e0+1*320
sprite-comic_death_2.png: OFFSET = 0x10e0+2*320
sprite-comic_death_3.png: OFFSET = 0x10e0+3*320
sprite-comic_death_4.png: OFFSET = 0x10e0+4*320
sprite-comic_death_5.png: OFFSET = 0x10e0+5*320
sprite-comic_death_6.png: OFFSET = 0x10e0+6*320
sprite-comic_death_7.png: OFFSET = 0x10e0+7*320

sprite-teleport_0.png: OFFSET = 0x1ae0+0*320
sprite-teleport_1.png: OFFSET = 0x1ae0+1*320
sprite-teleport_2.png: OFFSET = 0x1ae0+2*320

sprite-materialize_0.png:  OFFSET = 0x1ea0+0*320
sprite-materialize_1.png:  OFFSET = 0x1ea0+1*320
sprite-materialize_2.png:  OFFSET = 0x1ea0+2*320
sprite-materialize_3.png:  OFFSET = 0x1ea0+3*320
sprite-materialize_4.png:  OFFSET = 0x1ea0+4*320
sprite-materialize_5.png:  OFFSET = 0x1ea0+5*320
sprite-materialize_6.png:  OFFSET = 0x1ea0+6*320
sprite-materialize_7.png:  OFFSET = 0x1ea0+7*320
sprite-materialize_8.png:  OFFSET = 0x1ea0+8*320
sprite-materialize_9.png:  OFFSET = 0x1ea0+9*320
sprite-materialize_10.png: OFFSET = 0x1ea0+10*320
sprite-materialize_11.png: OFFSET = 0x1ea0+11*320

sprite-fireball_0.png: OFFSET = 0x0c80+0*80
sprite-fireball_1.png: OFFSET = 0x0c80+1*80

sprite-white_spark_0.png: OFFSET = 0x0d20+0*160
sprite-white_spark_1.png: OFFSET = 0x0d20+1*160
sprite-white_spark_2.png: OFFSET = 0x0d20+2*160

sprite-red_spark_0.png: OFFSET = 0x0f00+0*160
sprite-red_spark_1.png: OFFSET = 0x0f00+1*160
sprite-red_spark_2.png: OFFSET = 0x0f00+2*160

sprite-score_digit_0.png: OFFSET = 0x41c4+0*64
sprite-score_digit_1.png: OFFSET = 0x41c4+1*64
sprite-score_digit_2.png: OFFSET = 0x41c4+2*64
sprite-score_digit_3.png: OFFSET = 0x41c4+3*64
sprite-score_digit_4.png: OFFSET = 0x41c4+4*64
sprite-score_digit_5.png: OFFSET = 0x41c4+5*64
sprite-score_digit_6.png: OFFSET = 0x41c4+6*64
sprite-score_digit_7.png: OFFSET = 0x41c4+7*64
sprite-score_digit_8.png: OFFSET = 0x41c4+8*64
sprite-score_digit_9.png: OFFSET = 0x41c4+9*64

sprite-meter_empty.png: OFFSET = 0x4444
sprite-meter_half.png:  OFFSET = 0x4484
sprite-meter_full.png:  OFFSET = 0x44c4

ball.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BALL.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
ball.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BALL.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
bee.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BEE.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
bee.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BEE.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
bird.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
bird.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
bird.shp-extra.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD.SHP
	./programs/unpack-shp/unpack-shp $^ extra > "$(EXTRACTED_DIR)/$@"
bird2.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD2.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
bird2.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD2.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
bird2.shp-extra.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BIRD2.SHP
	./programs/unpack-shp/unpack-shp $^ extra > "$(EXTRACTED_DIR)/$@"
bug.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BUG.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
bug.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BUG.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
bug.shp-extra.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/BUG.SHP
	./programs/unpack-shp/unpack-shp $^ extra > "$(EXTRACTED_DIR)/$@"
cube.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CUBE.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
cube.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/CUBE.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
fb.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FB.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
fb.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FB.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
frog.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FROG.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
frog.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/FROG.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
globe.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/GLOBE.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
globe.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/GLOBE.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
rock.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/ROCK.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
rock.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/ROCK.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
rock.shp-extra.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/ROCK.SHP
	./programs/unpack-shp/unpack-shp $^ extra > "$(EXTRACTED_DIR)/$@"
saucer.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SAUCER.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
saucer.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/SAUCER.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
star1.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR1.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
star1.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR1.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
star2.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR2.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
star2.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR2.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"
star3.shp-left.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR3.SHP
	./programs/unpack-shp/unpack-shp $^ left > "$(EXTRACTED_DIR)/$@"
star3.shp-right.gif: $(EXTRACTED_DIR)/unpacked.exe $(ORIGINAL_DIR)/STAR3.SHP
	./programs/unpack-shp/unpack-shp $^ right > "$(EXTRACTED_DIR)/$@"

.PHONY: all
.DELETE_ON_ERROR:
