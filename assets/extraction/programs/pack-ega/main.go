// Converts an image file (PNG or GIF) into Captain Comic EGA format.
//
// http://www.shikadi.net/moddingwiki/Captain_Comic_Image_Format#File_format

package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"image/draw"
	_ "image/gif"
	_ "image/png"
	"io"
	"os"

	"../ega"
)

func chunks(b []byte, n int) [][]byte {
	var result [][]byte
	for len(b) > n {
		result = append(result, b[:n])
		b = b[n:]
	}
	if len(b) > 0 {
		result = append(result, b)
	}
	return result
}

func rleEncode(input []byte) []byte {
	var output bytes.Buffer

	copyStart := 0
	for copyStart < len(input) {
		copyEnd := copyStart
		fillStart := copyStart
		fillEnd := copyStart

		runStart := copyStart
		for runStart < len(input) {
			runLength := 0
			for runStart+runLength < len(input) && input[runStart+runLength] == input[runStart] {
				runLength++
			}
			if runLength < 3 {
				copyEnd += runLength
				fillStart += runLength
				fillEnd += runLength
				runStart += runLength
			} else {
				fillEnd += runLength
				break
			}
		}

		for _, chunk := range chunks(input[copyStart:copyEnd], 125) {
			output.WriteByte(uint8(len(chunk)))
			output.Write(chunk)
		}
		for _, chunk := range chunks(input[fillStart:fillEnd], 127) {
			output.WriteByte(uint8(len(chunk)) | 0x80)
			output.WriteByte(input[fillStart])
		}

		copyStart = fillEnd
	}

	return output.Bytes()
}

func imageToPlanes(im image.PalettedImage) []byte {
	bounds := im.Bounds()
	size := bounds.Size()
	// Each pixel in a plane is 1 bit; 4 planes ⇒ 4 bits per pixel.
	planes := make([]byte, size.X*size.Y/2)
	i := 0
	for plane := 0; plane < 4; plane++ {
		for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
			for x := bounds.Min.X; x < bounds.Max.X; x += 8 {
				var b byte
				for j := 0; j < 8; j++ {
					b |= ((im.ColorIndexAt(x+j, y) >> uint(plane)) & 1) << uint(7-j)
				}
				planes[i] = b
				i++
			}
		}
	}
	if i != len(planes) {
		panic(i)
	}

	return planes
}

func encode(r io.Reader, w io.Writer, rleFlag bool) error {
	// Load the source image.
	src, _, err := image.Decode(r)
	if err != nil {
		return err
	}

	// Create a paletted destination image using the EGA palette.
	im := image.NewPaletted(image.Rect(0, 0, 320, 200), ega.Palette)

	// Blit the source image onto the EGA image. The operation will find the
	// closest color matches in the palette.
	draw.Draw(im, im.Bounds(), src, image.ZP, draw.Src)

	planes := imageToPlanes(im)
	if rleFlag {
		var output bytes.Buffer
		binary.Write(&output, binary.LittleEndian, uint16(8000))
		// RLE encoding units must not cross plane boundaries.
		output.Write(rleEncode(planes[0*8000 : 1*8000]))
		output.Write(rleEncode(planes[1*8000 : 2*8000]))
		output.Write(rleEncode(planes[2*8000 : 3*8000]))
		output.Write(rleEncode(planes[3*8000 : 4*8000]))
		planes = output.Bytes()
	}

	_, err = os.Stdout.Write(planes)
	return err
}

func encodeFile(filename string, w io.Writer, rleFlag bool) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	return encode(f, w, rleFlag)
}

func main() {
	var rleFlag bool

	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [OPTIONS] FILENAME.PNG > out.ega\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.BoolVar(&rleFlag, "rle", true, "RLE-encode output; use -rle=false for Revision 1")
	flag.Parse()
	args := flag.Args()
	if len(args) != 1 {
		flag.Usage()
		os.Exit(1)
	}
	filename := args[0]

	err := encodeFile(filename, os.Stdout, rleFlag)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
