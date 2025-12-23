#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-R3sw1989}
# scripts/run-dosbox.sh
# Start DOSBox and mount $PWD/assets/original as C:
# Usage: ./scripts/run-dosbox.sh

ASSETS_DIR="$PWD/reference/orig/$VERSION"

if [[ ! -d "$ASSETS_DIR" ]]; then
  echo "Error: assets directory not found: $ASSETS_DIR" >&2
  exit 1
fi

if ! command -v dosbox >/dev/null 2>&1; then
  echo "Error: 'dosbox' not found in PATH. Install it (e.g. 'brew install dosbox')." >&2
  exit 1
fi

# Start DOSBox, mount the assets directory as C: and switch to C:
exec dosbox -c "mount c \"$ASSETS_DIR\"" -c "c:"
