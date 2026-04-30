#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
PRESET="${1:-debug}"

case "$PRESET" in
  debug|release) ;;
  *) echo "Usage: build.sh [debug|release]" >&2; exit 1 ;;
esac

cd "$ROOT_DIR"
cmake --build --preset "$PRESET"
