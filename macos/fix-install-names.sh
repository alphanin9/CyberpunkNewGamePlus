#!/bin/bash
# Re-sign NewGamePlus.dylib ad hoc. Static lz4 leaves no install name to rewrite.
# If lz4 is a linked dylib, point it at the copy next to the plugin.
# Usage: fix-install-names.sh <path to NewGamePlus.dylib>
set -eu
DYLIB="$1"
old=$(otool -L "$DYLIB" | awk 'NR > 1 && index($1, "liblz4") { print $1; exit }')
if [[ -n "$old" ]]; then
  case "$old" in
    @loader_path/*) echo "fix-install-names: liblz4 already $old" ;;
    *)
      new="@loader_path/$(basename "$old")"
      install_name_tool -change "$old" "$new" "$DYLIB"
      echo "fix-install-names: $old -> $new"
      if ! otool -l "$DYLIB" | grep -A2 LC_RPATH | grep -q '@loader_path'; then
        install_name_tool -add_rpath @loader_path "$DYLIB"
        echo "fix-install-names: rpath @loader_path added"
      fi
      ;;
  esac
else
  echo "fix-install-names: lz4 is not a linked dylib"
fi
codesign -f -s - "$DYLIB"
