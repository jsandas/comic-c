// http://www.shikadi.net/moddingwiki/Captain_Comic_Map_Format
// http://www.shikadi.net/moddingwiki/Captain_Comic_Tileset_Format

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
	"image/png"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"../ega"
	"../exe"
)

type Level struct {
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
			SHP      byte
			Behavior byte
		}
	}
	Padding [5]byte
}

// Canonicalize a space- and nul-padded filename as found in Level.
func cleanFilename(buf [14]byte) string {
	return strings.ToLower(strings.TrimRight(string(buf[:]), " \x00"))
}

type Map struct {
	Width  int
	Height int
	Tiles  []uint8
}

type Tile struct {
	IsPassable bool
	Graphic    image.Image
}

type Mask struct {
	w, h   int
	pixels []byte
}

func NewMask(w, h int, pixels []byte) *Mask {
	return &Mask{w, h, pixels}
}

func (g Mask) ColorModel() color.Model {
	return color.RGBAModel
}

func (g Mask) Bounds() image.Rectangle {
	return image.Rect(0, 0, g.w, g.h)
}

func (g Mask) At(x, y int) color.Color {
	width := g.Bounds().Dx()
	q := (y*width + x) / 8
	r := uint((y*width + x) % 8)
	a := (g.pixels[q] >> (7 - r)) & 0x01
	return color.Alpha{(1 - a) * 255}
}

func readTT2(r io.Reader) ([]*Tile, error) {
	var header [4]byte
	_, err := io.ReadFull(r, header[:])
	if err != nil {
		return nil, err
	}

	lastPassable := header[0]

	var tiles []*Tile
	for i := 0; ; i++ {
		planes := make([]byte, 128)
		_, err = io.ReadFull(r, planes[:])
		if err == io.EOF {
			break
		}
		if err != nil {
			return tiles, err
		}
		tiles = append(tiles, &Tile{
			IsPassable: i <= int(lastPassable),
			Graphic:    ega.GraphicFromPlanes(16, 16, planes[:]),
		})
	}

	return tiles, nil
}

func readTT2File(filename string) ([]*Tile, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return readTT2(bufio.NewReader(f))
}

func readPT(r io.Reader) (*Map, error) {
	var width, height uint16
	err := binary.Read(r, binary.LittleEndian, &width)
	if err != nil {
		return nil, err
	}
	err = binary.Read(r, binary.LittleEndian, &height)
	if err != nil {
		return nil, err
	}

	m := Map{
		Width:  int(width),
		Height: int(height),
		Tiles:  make([]uint8, width*height),
	}
	_, err = io.ReadFull(r, m.Tiles)
	if err != nil {
		return &m, err
	}

	// Check for trailing junk.
	var buf [1]byte
	n, err := r.Read(buf[:])
	if n != 0 || err != io.EOF {
		err = fmt.Errorf("trailing junk")
	}

	return &m, nil
}

func readPTFile(filename string) (*Map, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return readPT(bufio.NewReader(f))
}

func findPathCaseInsensitive(dirname, filename string) (string, error) {
	infos, err := ioutil.ReadDir(dirname)
	if err != nil {
		return "", err
	}
	for _, info := range infos {
		if strings.ToLower(filename) == strings.ToLower(info.Name()) {
			return filepath.Join(dirname, info.Name()), nil
		}
	}
	return "", fmt.Errorf("%s not found in %s", filename, dirname)
}

type options struct {
	Solidity bool
	Items    bool
}

func unpackFiles(levelNum, stageNum int, exeFilename, dirname string, options *options, w io.Writer) error {
	var levelData []byte
	exe, err := exe.ReadFile(exeFilename)
	if err == nil {
		levelData = exe.Data[exe.Sections[2]:]
	} else {
		// Hack for KSENI.COM -- it's not an EXE file so it doesn't have
		// sections.
		f, err := os.Open(exeFilename)
		if err != nil {
			return err
		}
		defer f.Close()
		data, err := ioutil.ReadAll(io.LimitReader(f, 0x10000))
		if err != nil {
			return err
		}
		levelData = data[0xf080:]
	}
	// Read levels from EXE.
	var levels [8]Level
	err = binary.Read(bytes.NewReader(levelData), binary.LittleEndian, &levels)
	if err != nil {
		return err
	}
	level := levels[levelNum]
	stage := level.Stages[stageNum]
	tt2Filename, err := findPathCaseInsensitive(dirname, cleanFilename(level.TT2Filename))
	if err != nil {
		return err
	}
	ptFilename, err := findPathCaseInsensitive(dirname, cleanFilename(level.StageFilenames[stageNum]))
	if err != nil {
		return err
	}

	tiles, err := readTT2File(tt2Filename)
	if err != nil {
		return err
	}
	m, err := readPTFile(ptFilename)
	if err != nil {
		return err
	}

	im := image.NewRGBA(image.Rect(0, 0, m.Width*16, m.Height*16))
	for i, tileIndex := range m.Tiles {
		p := image.Point{16 * (i % m.Width), 16 * (i / m.Width)}
		tile := tiles[tileIndex]
		g := tile.Graphic
		if options.Solidity {
			if tile.IsPassable {
				g = image.White
			} else {
				g = image.Black
			}
		}
		draw.Draw(im, image.Rectangle{p, p.Add(tile.Graphic.Bounds().Size())},
			g, tile.Graphic.Bounds().Min, draw.Src)
	}
	if options.Items && options.Solidity {
		for _, door := range stage.Doors {
			if door.X == 0xff && door.Y == 0xff {
				continue
			}
			mask := NewMask(32, 32, doorMask)
			// Draw each of the four 16×16 tiles making up the door,
			// inverting if the tile is not passable.
			for yOff := 0; yOff < 2; yOff += 1 {
				for xOff := 0; xOff < 2; xOff += 1 {
					tile := tiles[m.Tiles[(int(door.Y)/2+yOff)*m.Width+int(door.X)/2+xOff]]
					g := image.Black
					if !tile.IsPassable {
						g = image.White
					}
					p := image.Point{8*int(door.X) + 16*xOff, 8*int(door.Y) + 16*yOff}
					draw.DrawMask(im, image.Rectangle{p, p.Add(image.Pt(16, 16))},
						g, tile.Graphic.Bounds().Min,
						mask, mask.Bounds().Min.Add(image.Pt(16*xOff, 16*yOff)), draw.Over)
				}
			}
		}
	}
	if options.Items {
		itemGraphic := ega.GraphicFromPlanes(16, 16, itemGraphics[int(stage.ItemType)])
		if options.Solidity {
			itemGraphic = ega.GraphicFromPlanes(16, 16, itemGraphicsBW[int(stage.ItemType)])
		}
		itemMask := NewMask(16, 16, itemMasks[int(stage.ItemType)])
		p := image.Point{8 * int(stage.ItemX), 8 * int(stage.ItemY)}
		draw.DrawMask(im, image.Rectangle{p, p.Add(itemGraphic.Bounds().Size())},
			itemGraphic, itemGraphic.Bounds().Min,
			itemMask, itemMask.Bounds().Min, draw.Over)
	}

	return png.Encode(w, im)
}

func main() {
	showSolidity := flag.Bool("solidity", false, "show solidity instead of tile graphics")
	showItems := flag.Bool("items", false, "show item and door graphics")
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s LEVELNUM STAGENUM UNPACKED.EXE DIRNAME > out.png\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.Parse()
	args := flag.Args()
	if len(args) != 4 {
		flag.Usage()
		os.Exit(1)
	}
	levelNum, err := strconv.Atoi(args[0])
	stageNum, err := strconv.Atoi(args[1])
	exeFilename := args[2]
	dirname := args[3]

	options := options{
		Solidity: *showSolidity,
		Items:    *showItems,
	}
	err = unpackFiles(levelNum, stageNum, exeFilename, dirname, &options, os.Stdout)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
