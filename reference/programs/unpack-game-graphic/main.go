// http://www.shikadi.net/moddingwiki/Captain_Comic_Tileset_Format

package main

import (
	"bufio"
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"image/png"
	"io"
	"os"
	"strconv"
	"strings"

	"../ega"
)

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
	return color.RGBA{0, 0, 0, (1 - a) * 255}
}

func unpack(r io.Reader, w, h uint64, useMask bool) error {
	planesSize := w * h / 2
	maskSize := w * h / 8
	if !useMask {
		maskSize = 0
	}
	planes := make([]byte, planesSize)
	_, err := io.ReadFull(r, planes)
	if err != nil {
		return err
	}
	fmt.Fprintf(os.Stderr, "%x\n", planes)
	im := ega.GraphicFromPlanes(int(w), int(h), planes[:])
	var mask image.Image = image.Opaque
	if useMask {
		m := make([]byte, maskSize)
		_, err := io.ReadFull(r, m)
		if err != nil {
			return err
		}
		fmt.Fprintf(os.Stderr, "%x\n", m)
		mask = NewMask(int(w), int(h), m)
	}
	palIm := image.NewPaletted(im.Bounds(),
		append(im.ColorModel().(color.Palette), color.Transparent))
	draw.DrawMask(palIm, palIm.Bounds(), im, im.Bounds().Min,
		mask, mask.Bounds().Min, draw.Src)
	return png.Encode(os.Stdout, palIm)
}

func unpackFile(filename string, offset, w, h uint64, mask bool) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	_, err = f.Seek(int64(offset), io.SeekStart)
	if err != nil {
		return err
	}
	return unpack(bufio.NewReader(f), w, h, mask)
}

func parseWXH(s string) (w, h uint64, err error) {
	parts := strings.SplitN(s, "x", 2)
	w, err = strconv.ParseUint(parts[0], 10, 64)
	if err != nil {
		return
	}
	h, err = strconv.ParseUint(parts[1], 10, 64)
	if err != nil {
		return
	}
	return
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s FILENAME.EXE OFFSET WxH\n", os.Args[0])
		flag.PrintDefaults()
	}
	mask := flag.Bool("mask", false, "trailing mask")
	flag.Parse()
	args := flag.Args()
	if len(args) != 3 {
		flag.Usage()
		os.Exit(1)
	}
	filename := args[0]
	offsetStr := args[1]
	offset, err := strconv.ParseUint(offsetStr, 0, 64)
	if err != nil {
		fmt.Fprintf(os.Stderr, "bad offset %q: %s", offsetStr, err)
		os.Exit(1)
	}
	wxh := args[2]
	w, h, err := parseWXH(wxh)
	if err != nil {
		fmt.Fprintf(os.Stderr, "bad size %q: %s", wxh, err)
		os.Exit(1)
	}

	err = unpackFile(filename, offset, w, h, *mask)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
