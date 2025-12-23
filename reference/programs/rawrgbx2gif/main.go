// Usage: rawrgbx2gif 320 200 input.rawrgbx output.gif

package main

import (
	"bufio"
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/gif"
	"io"
	"os"
	"strconv"

	"../ega"
)

const vgaDelay = 1.0 / (125875.0 / 1796.0)

func loadFrame(r *bufio.Reader, width, height int) (*image.Paletted, error) {
	_, err := r.Peek(1)
	if err != nil {
		return nil, err
	}

	im := image.NewPaletted(image.Rect(0, 0, width, height), ega.Palette)
outer:
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			var R, G, B byte
			R, err = r.ReadByte()
			if err != nil {
				break outer
			}
			G, err = r.ReadByte()
			if err != nil {
				break outer
			}
			B, err = r.ReadByte()
			if err != nil {
				break outer
			}
			_, err = r.ReadByte()
			if err != nil {
				break outer
			}
			im.Set(x, y, color.RGBA{R, G, B, 255})
		}
	}
	if err == io.EOF {
		err = io.ErrUnexpectedEOF
	}
	return im, err
}

func loadFrames(r *bufio.Reader, width, height int) ([]*image.Paletted, error) {
	var frames []*image.Paletted
	for {
		frame, err := loadFrame(r, width, height)
		if err == io.EOF {
			break
		} else if err != nil {
			return frames, err
		}
		frames = append(frames, frame)
	}
	return frames, nil
}

func convertFiles(inputFilename, outputFilename string, width, height int) error {
	inputFile, err := os.Open(inputFilename)
	if err != nil {
		return err
	}
	defer inputFile.Close()

	frames, err := loadFrames(bufio.NewReader(inputFile), width, height)
	if err != nil {
		return err
	}

	var vgaDelay float64 = vgaDelay // shadow const as variable to permit converting to int
	delay := make([]int, len(frames))
	for i := range delay {
		delay[i] = int(100 * vgaDelay)
	}
	g := &gif.GIF{
		Image:     frames,
		Delay:     delay,
		LoopCount: 0,
		Config: image.Config{
			ColorModel: ega.Palette,
			Width:      width,
			Height:     height,
		},
	}

	outputFile, err := os.Create(outputFilename)
	if err != nil {
		return err
	}
	defer outputFile.Close()

	return gif.EncodeAll(outputFile, g)
}

func main() {
	flag.Parse()
	width, err := strconv.Atoi(flag.Arg(0))
	if err != nil {
		panic(err)
	}
	height, err := strconv.Atoi(flag.Arg(1))
	if err != nil {
		panic(err)
	}
	inputFilename := flag.Arg(2)
	outputFilename := flag.Arg(3)

	err = convertFiles(inputFilename, outputFilename, width, height)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
