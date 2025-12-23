REVS = R1sw1988 R2sw1988 R3sw1989 R4sw1989 R5sw1991

all: $(REVS)

$(REVS):
	@URL_NAME=$(patsubst R1%,%,$@); \
	if [ ! -f $@/COMIC.EXE ]; then \
		echo "Downloading $@..."; \
		mkdir -p $@; \
		if curl -L -f -o $@.zip https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeath$${URL_NAME}michaelA.Denioaction.zip; then \
			echo "Extracting to $@/..."; \
			unzip -o $@.zip -d $@/; \
			rm $@.zip; \
		else \
			echo "Failed to download $@"; \
			rm -f $@.zip; \
			exit 1; \
		fi; \
	else \
		echo "$@/COMIC.EXE already exists, skipping download"; \
	fi