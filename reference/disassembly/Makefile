# https://nasm.us/
NASM ?= nasm
# http://www.delorie.com/djgpp/16bit/djlink/
DJLINK ?= djlink/djlink
GO ?= go

all: R5sw1991.exe
.PHONY: all

djlink/djlink:
	(cd djlink && make djlink.exe) && mv -f djlink/djlink.exe "$@"

%.exe: %.obj
	"$(DJLINK)" -o "$@" $^
	mv "$@" ../original/"$@"

%.com: %.asm
	"$(NASM)" -f bin -o "$@" "$<"

.INTERMEDIATE: %.obj
%.obj: %.asm R3_levels.asm $(GRAPHICS)
	"$(NASM)" -f obj -o "$@" "$<"

.DELETE_ON_ERROR:
