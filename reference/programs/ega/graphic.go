package ega

import (
	"image"
	"image/color"
)

var Palette = color.Palette{
	color.RGBA{0x00, 0x00, 0x00, 255}, /*  0 */
	color.RGBA{0x00, 0x00, 0xaa, 255}, /*  1 */
	color.RGBA{0x00, 0xaa, 0x00, 255}, /*  2 */
	color.RGBA{0x00, 0xaa, 0xaa, 255}, /*  3 */
	color.RGBA{0xaa, 0x00, 0x00, 255}, /*  4 */
	color.RGBA{0xaa, 0x00, 0xaa, 255}, /*  5 */
	color.RGBA{0xaa, 0x55, 0x00, 255}, /*  6 */
	color.RGBA{0xaa, 0xaa, 0xaa, 255}, /*  7 */
	color.RGBA{0x55, 0x55, 0x55, 255}, /*  8 */
	color.RGBA{0x55, 0x55, 0xff, 255}, /*  9 */
	color.RGBA{0x55, 0xff, 0x55, 255}, /* 10 */
	color.RGBA{0x55, 0xff, 0xff, 255}, /* 11 */
	color.RGBA{0xff, 0x55, 0x55, 255}, /* 12 */
	color.RGBA{0xff, 0x55, 0xff, 255}, /* 13 */
	color.RGBA{0xff, 0xff, 0x55, 255}, /* 14 */
	color.RGBA{0xff, 0xff, 0xff, 255}, /* 15 */
}

type Graphic struct {
	Width  int
	Height int
	Planes []byte
}

func GraphicFromPlanes(width, height int, planes []byte) *Graphic {
	return &Graphic{
		Width:  width,
		Height: height,
		Planes: planes,
	}
}

func (g *Graphic) ColorModel() color.Model {
	return Palette
}

func (g *Graphic) Bounds() image.Rectangle {
	return image.Rect(0, 0, g.Width, g.Height)
}

func (g *Graphic) ColorIndexAt(x, y int) uint8 {
	q := (y*g.Width + x) / 8
	r := uint(y*g.Width+x) % 8
	var index uint8
	index |= ((g.Planes[0*g.Height*g.Width/8+q] >> (7 - r)) & 0x01) << 0
	index |= ((g.Planes[1*g.Height*g.Width/8+q] >> (7 - r)) & 0x01) << 1
	index |= ((g.Planes[2*g.Height*g.Width/8+q] >> (7 - r)) & 0x01) << 2
	index |= ((g.Planes[3*g.Height*g.Width/8+q] >> (7 - r)) & 0x01) << 3
	return index
}

func (g *Graphic) At(x, y int) color.Color {
	return Palette[g.ColorIndexAt(x, y)]
}
