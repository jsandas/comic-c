// http://www.shikadi.net/moddingwiki/Captain_Comic_Tileset_Format

package main

import (
	"bufio"
	"flag"
	"fmt"
	"image"
	"image/png"
	"io"
	"os"

	"../ega"
)

func savePNGFile(filename string, im image.Image) error {
	f, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	return png.Encode(f, im)
}

func unpack(r io.Reader, basename string) error {
	var header [4]byte
	_, err := io.ReadFull(r, header[:])
	if err != nil {
		return err
	}

	var planes [128]byte
	for i := 0; ; i++ {
		_, err = io.ReadFull(r, planes[:])
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}
		im := ega.GraphicFromPlanes(16, 16, planes[:])
		filename := fmt.Sprintf("%s-%02x.png", basename, i)
		fmt.Println(filename)
		err = savePNGFile(filename, im)
		if err != nil {
			return err
		}
	}

	return nil
}

func unpackFile(filename, basename string) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer f.Close()
	return unpack(bufio.NewReader(f), basename)
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s FILENAME.TT2 BASENAME\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.Parse()
	args := flag.Args()
	if len(args) != 2 {
		flag.Usage()
		os.Exit(1)
	}
	filename := args[0]
	basename := args[1]

	err := unpackFile(filename, basename)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
