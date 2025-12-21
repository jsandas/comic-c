package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

const levelSize = 208

type levelInfo struct {
	TT2Filename    [14]byte
	StageFilenames [3][14]byte
	DoorTiles      [4]byte
	SHPs           [4]struct {
		NumDistinctFrames byte
		Horizontal        byte
		Animation         byte
		Filename          [14]byte
	}
	Stages [3]struct {
		ItemType     byte
		ItemY, ItemX byte
		ExitL, ExitR byte
		Doors        [3]struct {
			Y, X        byte
			TargetLevel byte
			TargetStage byte
		}
		Enemies [4]struct {
			SHPIndex byte
			Behavior byte
		}
	}
	Padding [5]byte
}

func enemyHorizontalConstant(c uint8) string {
	switch c {
	case 1:
		return "ENEMY_HORIZONTAL_DUPLICATED"
	case 2:
		return "ENEMY_HORIZONTAL_SEPARATE"
	default:
		panic(c)
	}
}

func enemyAnimationConstant(c uint8) string {
	switch c {
	case 0:
		return "ENEMY_ANIMATION_LOOP"
	case 1:
		return "ENEMY_ANIMATION_ALTERNATE"
	default:
		panic(c)
	}
}

func enemyBehaviorConstant(c uint8) string {
	var parts []string
	switch c & 0x7f {
	case 1:
		parts = append(parts, "ENEMY_BEHAVIOR_BOUNCE")
	case 2:
		parts = append(parts, "ENEMY_BEHAVIOR_LEAP")
	case 3:
		parts = append(parts, "ENEMY_BEHAVIOR_ROLL")
	case 4:
		parts = append(parts, "ENEMY_BEHAVIOR_SEEK")
	case 5:
		parts = append(parts, "ENEMY_BEHAVIOR_SHY")
	default:
		panic(c)
	}
	if c&0x80 != 0 {
		parts = append(parts, "ENEMY_BEHAVIOR_FAST")
	}
	return strings.Join(parts, " | ")
}

func itemTypeConstant(itemType uint8) string {
	switch itemType {
	case 0:
		return "ITEM_CORKSCREW"
	case 1:
		return "ITEM_DOOR_KEY"
	case 2:
		return "ITEM_BOOTS"
	case 3:
		return "ITEM_LANTERN"
	case 4:
		return "ITEM_TELEPORT_WAND"
	case 5:
		return "ITEM_GEMS"
	case 6:
		return "ITEM_CROWN"
	case 7:
		return "ITEM_GOLD"
	case 8:
		return "ITEM_BLASTOLA_COLA"
	case 14:
		return "ITEM_SHIELD"
	default:
		panic(itemType)
	}
}

func gen(f io.ReadSeeker, mapData int64) error {
	_, err := f.Seek(mapData, io.SeekStart)
	if err != nil {
		return err
	}

	var levels [8]levelInfo
	err = binary.Read(f, binary.LittleEndian, &levels)
	if err != nil {
		return err
	}

	for _, level := range levels {
		levelName := level.TT2Filename[:bytes.IndexByte(level.TT2Filename[:], '.')]
		fmt.Printf("\n")
		fmt.Printf("LEVEL_DATA_%s:\n", bytes.ToUpper(levelName))
		fmt.Printf("istruc level\n")
		fmt.Printf("at level.tt2_filename,\tdb\t\"%s\"\n", level.TT2Filename[:13])
		fmt.Printf("at level.pt0_filename,\tdb\t\"%s\"\n", level.StageFilenames[0][:13])
		fmt.Printf("at level.pt1_filename,\tdb\t\"%s\"\n", level.StageFilenames[1][:13])
		fmt.Printf("at level.pt2_filename,\tdb\t\"%s\"\n", level.StageFilenames[2][:13])
		fmt.Printf("at level.door_tile_ul,\tdb\t%d\n", level.DoorTiles[0])
		fmt.Printf("at level.door_tile_ur,\tdb\t%d\n", level.DoorTiles[1])
		fmt.Printf("at level.door_tile_ll,\tdb\t%d\n", level.DoorTiles[2])
		fmt.Printf("at level.door_tile_lr,\tdb\t%d\n", level.DoorTiles[3])
		for shp_i, shp := range level.SHPs {
			if shp.NumDistinctFrames == 0 {
				fmt.Printf("at level.shp + %d*shp_size + shp.num_distinct_frames,\tdb\tSHP_UNUSED\n", shp_i)
				continue
			}
			fmt.Printf("at level.shp + %d*shp_size + shp.num_distinct_frames,\tdb\t%d\n", shp_i, shp.NumDistinctFrames)
			fmt.Printf("at level.shp + %d*shp_size + shp.horizontal,\t\tdb\t%s\n", shp_i, enemyHorizontalConstant(shp.Horizontal))
			fmt.Printf("at level.shp + %d*shp_size + shp.animation,\t\tdb\t%s\n", shp_i, enemyAnimationConstant(shp.Animation))
			fmt.Printf("at level.shp + %d*shp_size + shp.filename,\t\tdb\t\"%s\"\n", shp_i, shp.Filename[:13])
		}
		for stage_i, stage := range level.Stages {
			fmt.Printf("; %s%d\n", levelName, stage_i)
			fmt.Printf("at level.stages + %d*stage_size + stage.item_type,\tdb\t%s\n", stage_i, itemTypeConstant(stage.ItemType))
			fmt.Printf("at level.stages + %d*stage_size + stage.item_y,\t\tdb\t%d\n", stage_i, stage.ItemY)
			fmt.Printf("at level.stages + %d*stage_size + stage.item_x,\t\tdb\t%d\n", stage_i, stage.ItemX)
			if stage.ExitL == 0xff {
				fmt.Printf("at level.stages + %d*stage_size + stage.exit_l,\t\tdb\tEXIT_UNUSED\n", stage_i)
			} else {
				fmt.Printf("at level.stages + %d*stage_size + stage.exit_l,\t\tdb\t%d\n", stage_i, stage.ExitL)
			}
			if stage.ExitR == 0xff {
				fmt.Printf("at level.stages + %d*stage_size + stage.exit_r,\t\tdb\tEXIT_UNUSED\n", stage_i)
			} else {
				fmt.Printf("at level.stages + %d*stage_size + stage.exit_r,\t\tdb\t%d\n", stage_i, stage.ExitR)
			}
			for door_i, door := range stage.Doors {
				if door.Y == 0xff && door.X == 0xff {
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.y,\t\tdb\tDOOR_UNUSED\n", stage_i, door_i)
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.x,\t\tdb\tDOOR_UNUSED\n", stage_i, door_i)
				} else {
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.y,\t\tdb\t%d\n", stage_i, door_i, door.Y)
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.x,\t\tdb\t%d\n", stage_i, door_i, door.X)
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.target_level,\tdb\t%d\n", stage_i, door_i, door.TargetLevel)
					fmt.Printf("at level.stages + %d*stage_size + stage.doors + %d*door_size + door.target_stage,\tdb\t%d\n", stage_i, door_i, door.TargetStage)
				}
			}
			for enemy_i, enemy := range stage.Enemies {
				if enemy.Behavior == 127 {
					fmt.Printf("at level.stages + %d*stage_size + stage.enemies + %d*enemy_record_size + enemy_record.behavior,\tdb\tENEMY_BEHAVIOR_UNUSED\n", stage_i, enemy_i)
				} else {
					fmt.Printf("at level.stages + %d*stage_size + stage.enemies + %d*enemy_record_size + enemy_record.shp_index,\tdb\t%d\n", stage_i, enemy_i, enemy.SHPIndex)
					fmt.Printf("at level.stages + %d*stage_size + stage.enemies + %d*enemy_record_size + enemy_record.behavior,\tdb\t%s\n", stage_i, enemy_i, enemyBehaviorConstant(enemy.Behavior))
				}
			}
		}

		fmt.Printf("iend\n")
		fmt.Printf("resb %d-($-LEVEL_DATA_%s)\t; padding\n", levelSize, bytes.ToUpper(levelName))
	}

	return nil
}

func genFile(filename string, mapData int64) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	return gen(f, mapData)
}

func main() {
	flag.Parse()
	if flag.NArg() != 2 {
		fmt.Fprintf(os.Stderr, "need a filename and offset\n")
		os.Exit(1)
	}
	filename := flag.Arg(0)
	mapData, err := strconv.ParseInt(flag.Arg(1), 10, 64)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	err = genFile(filename, mapData)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
