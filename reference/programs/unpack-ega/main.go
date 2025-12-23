// http://www.shikadi.net/moddingwiki/Captain_Comic_Image_Format#File_format

package main

import (
	"bufio"
	"encoding/binary"
	"flag"
	"fmt"
	"image/png"
	"io"
	"os"

	"../ega"
)

var (
	errRLEDecodingMissingRepeat = fmt.Errorf("EOF while looking for repeat byte")
	errRLEDecodingShortVerbatim = fmt.Errorf("EOF in the middle of a verbatim block")
)

type rleReader struct {
	r             *bufio.Reader
	verbatimCount int
	repeatCount   int
	repeatValue   uint8
}

func newRLEReader(r *bufio.Reader) *rleReader {
	return &rleReader{r: r}
}

func (r *rleReader) Read(p []byte) (int, error) {
	if len(p) == 0 {
		return 0, nil
	}

	for r.verbatimCount == 0 && r.repeatCount == 0 {
		c, err := r.r.ReadByte()
		if err != nil {
			// EOF here is really EOF, not a decoding error.
			return 0, err
		}
		if c < 128 {
			r.verbatimCount = int(c)
		} else {
			d, err := r.r.ReadByte()
			if err == io.EOF {
				err = errRLEDecodingMissingRepeat
			}
			if err != nil {
				return 0, err
			}
			r.repeatCount = int(c) - 128
			r.repeatValue = d
		}
	}

	if r.verbatimCount > 0 {
		toRead := r.verbatimCount
		if toRead > len(p) {
			toRead = len(p)
		}
		n, err := r.r.Read(p[:toRead])
		r.verbatimCount -= n
		return n, err
	}

	toRepeat := r.repeatCount
	if toRepeat > len(p) {
		toRepeat = len(p)
	}
	for i := 0; i < toRepeat; i++ {
		p[i] = r.repeatValue
	}
	r.repeatCount -= toRepeat
	return toRepeat, nil
}

func unpack(r io.Reader, w io.Writer, rleFlag bool) error {
	br := bufio.NewReader(r)

	var planeSize uint16
	if rleFlag {
		err := binary.Read(br, binary.LittleEndian, &planeSize)
		if err != nil {
			return err
		}
		if planeSize != 8000 {
			return fmt.Errorf("got plane size %d, expected %d", planeSize, 8000)
		}
	} else {
		planeSize = 8000
	}
	planes := make([]byte, 4*int(planeSize))
	for i := 0; i < 4; i++ {
		var pr io.Reader
		if rleFlag {
			pr = newRLEReader(br)
		} else {
			pr = br
		}
		_, err := io.ReadFull(pr, planes[i*int(planeSize):(i+1)*int(planeSize)])
		if err != nil {
			return err
		}
	}

	// Check for trailing junk.
	n, err := br.ReadByte()
	if n != 0 || err != io.EOF {
		err = fmt.Errorf("trailing junk")
	}

	return png.Encode(w, ega.GraphicFromPlanes(320, 200, planes))
}

func unpackFile(filename string, w io.Writer, rleFlag bool) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	return unpack(f, w, rleFlag)
}

func main() {
	var rleFlag bool

	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [OPTIONS] FILENAME.EGA > out.png\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.BoolVar(&rleFlag, "rle", true, "assume RLE format; use -rle=false for Revision 1")
	flag.Parse()
	args := flag.Args()
	if len(args) != 1 {
		flag.Usage()
		os.Exit(1)
	}
	filename := args[0]

	err := unpackFile(filename, os.Stdout, rleFlag)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
