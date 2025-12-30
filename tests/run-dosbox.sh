#!/usr/bin/env bash
set -euo pipefail

# scripts/run-dosbox.sh
# Start DOSBox-X and mount $PWD/reference/original as C:
# Usage: ./scripts/run-dosbox.sh

BUILD_DIR=$PWD/build
ORIGINAL_DIR="$PWD/reference/original"

if [[ ! -d "$ORIGINAL_DIR" ]]; then
  echo "Error: assets directory not found: $ORIGINAL_DIR" >&2
  exit 1
fi

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "Error: build directory not found: $BUILD_DIR" >&2
  echo "Please build the project first (e.g. 'make build')." >&2
  exit 1
fi

if [[ -f "$ORIGINAL_DIR/COMIC-C.EXE" ]]; then
  echo "Deleteing existing COMIC-C.EXE in $ORIGINAL_DIR..."
  rm "$ORIGINAL_DIR/COMIC-C.EXE"
fi

echo "Copying built COMIC-C.EXE to $ORIGINAL_DIR..."
cp "$BUILD_DIR/COMIC-C.EXE" "$ORIGINAL_DIR/COMIC-C.EXE"

if ! command -v dosbox-x >/dev/null 2>&1; then
  echo "Error: 'dosbox-x' not found in PATH. Install it (e.g. 'brew install dosbox-x')." >&2
  exit 1
fi

# Start DOSBox, mount the assets directory as C: and switch to C:ll
cd tests; exec dosbox-x -conf $PWD/dosbox_deterministic.conf \
  -savedir $PWD/savestates \
  -c "mount c \"$ORIGINAL_DIR\"" -c "c:"
