all: rawrgbx2gif unpack-ega unpack-pt unpack-shp unpack-tt2 unpack-game-graphic

# clean:
# 	rm -f $(PROGRAMS) *.o *~

rawrgbx2gif:
	GO111MODULE=off go build -o ./$@/$@ ./$@

unpack-ega:
	GO111MODULE=off go build -o ./$@/$@ ./$@

unpack-pt:
	GO111MODULE=off go build -o ./$@/$@ ./$@

unpack-shp:
	GO111MODULE=off go build -o ./$@/$@ ./$@

unpack-tt2:
	GO111MODULE=off go build -o ./$@/$@ ./$@

unpack-game-graphic:
	GO111MODULE=off go build -o ./$@/$@ ./$@

.PHONY: all rawrgbx2gif unpack-ega unpack-pt unpack-shp unpack-tt2 unpack-game-graphic
