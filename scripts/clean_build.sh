#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
PRESET="${1:-debug}"

case "$PRESET" in
  debug|release|all) ;;
  *) echo "Usage: clean_build.sh [debug|release|all]" >&2; exit 1 ;;
esac

if [ "$PRESET" = "all" ]; then
  set -- debug release
else
  set -- "$PRESET"
fi

for name do
  path="$ROOT_DIR/build/$name"
  if [ -d "$path" ]; then
    rm -rf "$path"
  fi
done
