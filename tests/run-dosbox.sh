#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-R5sw1991}
# scripts/run-dosbox.sh
# Start DOSBox-X and mount $PWD/reference/orig/$VERSION as C:
# Usage: ./scripts/run-dosbox.sh

ASSETS_DIR="$PWD/reference/orig/$VERSION"

if [[ ! -d "$ASSETS_DIR" ]]; then
  echo "Error: assets directory not found: $ASSETS_DIR" >&2
  exit 1
fi

if ! command -v dosbox-x >/dev/null 2>&1; then
  echo "Error: 'dosbox-x' not found in PATH. Install it (e.g. 'brew install dosbox-x')." >&2
  exit 1
fi

# Start DOSBox, mount the assets directory as C: and switch to C:ll
cd tests; exec dosbox-x -conf $PWD/dosbox_deterministic.conf \
  -savedir $PWD/savestates \
  -c "mount c \"$ASSETS_DIR\"" -c "c:"
