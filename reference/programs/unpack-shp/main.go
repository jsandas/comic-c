// http://www.shikadi.net/moddingwiki/Captain_Comic_Sprite_Format

package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"image/gif"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"time"

	"../ega"
	"../exe"
)

const (
	facingRight = 5
	facingLeft  = 0
	facingExtra = -1 // means unreferenced frames at the end of the file
)

// delay for GIF animation
var delayTime time.Duration

type SHP struct {
	NumDistinctFrames byte
	Horizontal        byte
	Animation         byte
	Filename          [14]byte
}

type Level struct {
	TT2Filename    [14]byte
	StageFilenames [3][14]byte
	DoorTiles      [4]byte
	SHPs           [4]SHP
	Stages         [3]struct {
		ItemType     byte
		ItemY, ItemX byte
		ExitL, ExitR byte
		Doors        [3]struct {
			Y, X        byte
			TargetLevel byte
			TargetStage byte
		}
		Enemies [4]struct {
			SHP      byte
			Behavior byte
		}
	}
	Padding [5]byte
}

const (
	shpHorizontalDuplicated = 1
	shpHorizontalSeparate   = 2
	shpAnimationLoop        = 0
	shpAnimationAlternate   = 1
)

// Canonicalize a space- and nul-padded filename as found in Level.
func cleanFilename(buf [14]byte) string {
	return strings.ToLower(strings.TrimRight(string(buf[:]), " \x00"))
}

type Mask []byte

func (g Mask) ColorModel() color.Model {
	return color.RGBAModel
}

func (g Mask) Bounds() image.Rectangle {
	return image.Rect(0, 0, 16, 16)
}

func (g Mask) At(x, y int) color.Color {
	width := g.Bounds().Dx()
	q := (y*width + x) / 8
	r := uint((y*width + x) % 8)
	a := (g[q] >> (7 - r)) & 0x01
	return color.RGBA{0, 0, 0, (1 - a) * 255}
}

func saveGIFFile(w io.Writer, frames []*image.Paletted) error {
	delay := make([]int, len(frames))
	disposal := make([]byte, len(frames))
	for i := range frames {
		delay[i] = int(delayTime / 10 / time.Millisecond)
		disposal[i] = gif.DisposalPrevious
	}
	g := gif.GIF{
		Image:     frames,
		Delay:     delay,
		LoopCount: 0,
		Disposal:  disposal,
		Config: image.Config{
			ColorModel: ega.Palette,
			Width:      frames[0].Bounds().Dx(),
			Height:     frames[0].Bounds().Dy(),
		},
		BackgroundIndex: 0,
	}
	return gif.EncodeAll(w, &g)
}

func readLevelsFromEXE(exeFilename string) ([]Level, error) {
	var levelData []byte
	exeFile, err := exe.ReadFile(exeFilename)
	if err == nil {
		levelData = exeFile.Data[exeFile.Sections[2]:]
	} else {
		// Hack for KSENI.COM -- it's not an EXE file so it doesn't have
		// sections.
		f, err := os.Open(exeFilename)
		if err != nil {
			return nil, err
		}
		defer f.Close()
		data, err := ioutil.ReadAll(io.LimitReader(f, 0x10000))
		if err != nil {
			return nil, err
		}
		levelData = data[0xf080:]
	}
	// Read levels from EXE.
	var levels [8]Level
	err = binary.Read(bytes.NewReader(levelData), binary.LittleEndian, &levels)
	if err != nil {
		return nil, err
	}
	return levels[:], nil
}

func loadFrames(exeFilename, shpFilename string, facing int) ([][]byte, error) {
	levels, err := readLevelsFromEXE(exeFilename)
	f, err := os.Open(shpFilename)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	shpFile := bufio.NewReader(f)

	var info *struct {
		levelNum int
		shpNum   int
		shp      SHP
	}
	// Ensure that all references to shpFilename are consistent as regards
	// their NumDistinctFrames, Horizontal, and Animation fields.
	for levelNum, level := range levels {
		for shpNum, shp := range level.SHPs {
			if cleanFilename(shp.Filename) != strings.ToLower(filepath.Base(shpFilename)) {
				continue
			}
			if info == nil {
				info = &struct {
					levelNum int
					shpNum   int
					shp      SHP
				}{levelNum, shpNum, shp}
			}
			if info.shp != shp {
				fmt.Fprintf(os.Stderr, "warning: %s %s: SHP data mismatch\n", exeFilename, cleanFilename(shp.Filename))
				fmt.Fprintf(os.Stderr, "using    level %d shp %d %+v\n", info.levelNum, info.shpNum, info.shp)
				fmt.Fprintf(os.Stderr, "ignoring level %d shp %d %+v\n", levelNum, shpNum, shp)
			}
		}
	}
	// Ensure that this shpFilename is actually referred to.
	if info == nil {
		return nil, fmt.Errorf("no references to %+q in %s", filepath.Base(shpFilename), exeFilename)
	}
	shp := info.shp

	// See load_shp_file in the disassembly.
	numFrames := int(shp.NumDistinctFrames)
	if shp.Horizontal != shpHorizontalDuplicated {
		// Double the number of frames because we have separate left-
		// and right-facing frames.
		numFrames *= 2
	}
	graphics := make([]byte, 160*numFrames)
	_, err = io.ReadFull(shpFile, graphics)
	if err != nil {
		return nil, err
	}

	var extraFrames [][]byte
	for {
		buf := make([]byte, 160)
		_, err = io.ReadFull(shpFile, buf)
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}
		extraFrames = append(extraFrames, buf)
	}
	if len(extraFrames) > 0 {
		fmt.Fprintf(os.Stderr, "warning: %s: trailing data after reading %d bytes\n", shpFilename, len(graphics))
	}

	var leftFrames [][]byte
	p := 0
	for i := 0; i < int(shp.NumDistinctFrames); i++ {
		leftFrames = append(leftFrames, graphics[p*160:][:160])
		p++
	}
	if shp.Animation != shpAnimationLoop {
		leftFrames = append(leftFrames, leftFrames[len(leftFrames)-2])
	}

	var rightFrames [][]byte
	if shp.Horizontal != shpHorizontalSeparate {
		// Right-facing frames are duplicates of left-facing frames.
		p = 0
	}
	for i := 0; i < int(shp.NumDistinctFrames); i++ {
		rightFrames = append(rightFrames, graphics[p*160:][:160])
		p++
	}
	if shp.Animation != shpAnimationLoop {
		rightFrames = append(rightFrames, rightFrames[len(rightFrames)-2])
	}

	if facing == facingRight {
		return rightFrames, nil
	} else if facing == facingLeft {
		return leftFrames, nil
	} else {
		return extraFrames, nil
	}
}

func unpack(exeFilename, shpFilename string, facing int) error {
	frames, err := loadFrames(exeFilename, shpFilename, facing)
	if err != nil {
		return err
	}
	var images []*image.Paletted
	for _, frame := range frames {
		planes := frame[:128]
		mask := Mask(frame[128:])
		im := ega.GraphicFromPlanes(16, 16, planes)
		palIm := image.NewPaletted(im.Bounds(),
			append(im.ColorModel().(color.Palette), color.Transparent))
		draw.DrawMask(palIm, palIm.Bounds(), im, im.Bounds().Min,
			mask, mask.Bounds().Min, draw.Src)
		images = append(images, palIm)
	}
	return saveGIFFile(os.Stdout, images)
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [OPTIONS] UNPACKED.EXE FILENAME.SHP [left|right] > out.gif\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.DurationVar(&delayTime, "delay", 110*time.Millisecond, "delay for GIF animation, multiple of 10ms")
	flag.Parse()
	args := flag.Args()
	if len(args) != 3 {
		flag.Usage()
		os.Exit(1)
	}
	exeFilename := args[0]
	shpFilename := args[1]
	var facing int
	switch args[2] {
	case "left":
		facing = facingLeft
	case "right":
		facing = facingRight
	case "extra":
		facing = facingExtra
	default:
		fmt.Fprintf(os.Stderr, "unknown \"facing\" value %+q, try \"left\", \"right\", or \"extra\"", args[1])
		os.Exit(1)
	}

	err := unpack(exeFilename, shpFilename, facing)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
