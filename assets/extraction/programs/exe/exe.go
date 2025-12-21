package exe

import (
	"encoding/binary"
	"io"
	"os"
	"sort"
)

type File struct {
	Header   Header
	Relocs   []Pointer
	Sections []int
	Data     []byte
}

// http://www.delorie.com/djgpp/doc/exe/
type Header struct {
	Magic               [2]byte
	NumBytesInLastBlock uint16
	NumBlocks           uint16
	NumRelocs           uint16
	NumHeaderParagraphs uint16
	MinAlloc            uint16
	MaxAlloc            uint16
	InitSS              uint16
	InitSP              uint16
	Checksum            uint16
	InitIP              uint16
	InitCS              uint16
	RelocOffset         uint16
	OverlayNumber       uint16
}

func (header *Header) DataOffset() int64 {
	return int64(header.NumHeaderParagraphs) * 16
}

func (header *Header) DataLength() int {
	length := int(header.NumBlocks) * 512
	if header.NumBytesInLastBlock != 0 {
		length -= 512 - int(header.NumBytesInLastBlock)
	}
	return length
}

type Pointer struct {
	Offset  uint16
	Segment uint16
}

func (addr Pointer) Linear() int64 {
	return int64(addr.Segment)*16 + int64(addr.Offset)
}

func inferSections(relocs []Pointer, data []byte) []int {
	var sections []int
	// Always at least one section starting at 0000:0000.
	sections = append(sections, 0)
	// Sort and deduplicate the values pointed to by relocations.
loop:
	for _, reloc := range relocs {
		relocLinear := reloc.Linear()
		offset := int(data[relocLinear]) | (int(data[relocLinear+1]) << 8)
		for _, existing := range sections {
			if offset == existing {
				continue loop
			}
		}
		sections = append(sections, offset)
	}
	sort.Ints(sections)
	for i := range sections {
		sections[i] *= 16
	}
	return sections
}

func Read(r io.ReadSeeker) (*File, error) {
	var exe File

	// Read header.
	_, err := r.Seek(0, io.SeekStart)
	if err != nil {
		return nil, err
	}
	err = binary.Read(r, binary.LittleEndian, &exe.Header)
	if err != nil {
		return nil, err
	}

	// Read relocations.
	_, err = r.Seek(int64(exe.Header.RelocOffset), io.SeekStart)
	if err != nil {
		return nil, err
	}
	for i := 0; i < int(exe.Header.NumRelocs); i++ {
		var reloc Pointer
		err = binary.Read(r, binary.LittleEndian, &reloc)
		if err != nil {
			return nil, err
		}
		exe.Relocs = append(exe.Relocs, reloc)
	}

	// Read data.
	_, err = r.Seek(exe.Header.DataOffset(), io.SeekStart)
	if err != nil {
		return nil, err
	}
	exe.Data = make([]byte, exe.Header.DataLength())
	_, err = r.Read(exe.Data)
	if err != nil {
		return nil, err
	}

	// Infer sections based on what values the relocations point to.
	exe.Sections = inferSections(exe.Relocs, exe.Data)

	return &exe, nil
}

func ReadFile(filename string) (*File, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return Read(f)
}
