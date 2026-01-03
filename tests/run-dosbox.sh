#!/usr/bin/env bash
set -euo pipefail

# scripts/run-dosbox.sh
# Start DOSBox-X and mount $PWD/reference/original as C:
# Usage: ./scripts/run-dosbox.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

BUILD_DIR=$PWD/build
ORIGINAL_DIR="$PWD/reference/original"

EXE_NAME="COMIC-C.EXE"
if [[ -n "${1-}" ]]; then
  EXE_NAME="$1"
fi

if [[ ! -d "$ORIGINAL_DIR" ]]; then
  echo "Error: assets directory not found: $ORIGINAL_DIR" >&2
  exit 1
fi

if [[ ! -d "$BUILD_DIR" ]]; then
  echo -e "${RED}Error: build directory not found: $BUILD_DIR${RESET}" >&2
  echo -e "${RED}Please build the project first (e.g. 'make build').${RESET}" >&2
  exit 1
fi

if [[ -f "$ORIGINAL_DIR/COMIC-C.EXE" ]]; then
  echo -e "${GREEN}Deleting existing COMIC-C.EXE in $ORIGINAL_DIR...${RESET}"
  rm "$ORIGINAL_DIR/COMIC-C.EXE"
fi

echo -e "${GREEN}Copying built COMIC-C.EXE to $ORIGINAL_DIR...${RESET}"
cp "$BUILD_DIR/COMIC-C.EXE" "$ORIGINAL_DIR/COMIC-C.EXE"

if ! command -v dosbox-x >/dev/null 2>&1; then
  echo -e "${RED}Error: 'dosbox-x' not found in PATH. Install it (e.g. 'brew install dosbox-x').${RESET}" >&2
  exit 1
fi

# Start DOSBox, mount the assets directory as C: and switch to C:
cd tests; exec dosbox-x -conf $PWD/dosbox_deterministic.conf \
  -savedir $PWD/savestates \
  -c "mount c \"$ORIGINAL_DIR\" -freesize 1024" -c "c:" -c "$EXE_NAME"
