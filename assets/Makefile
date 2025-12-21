VERSION="R3sw1989"
ARCHIVE_URL=https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeath${VERSION}michaelA.Denioaction.zip

get_original:
	mkdir -p original
	echo "Downloading ${ARCHIVE_URL} via curl..."
	curl -L -o original/${VERSION}.zip ${ARCHIVE_URL}
	
	echo "Extracting archive to original/${VERSION}..."
	unzip -o "original/${VERSION}.zip" -d "original/"

programs:
	$(MAKE) -C extraction/programs

extraction:
	$(MAKE) -C extraction

# disassembly:
# 	$(MAKE) -C disassembly

all: programs extraction

.PHONY: all programs extraction
