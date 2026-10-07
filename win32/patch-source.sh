#!/bin/sh
# Writes a copy of a ROSE source file with the changes of a sed script (one command per line),
# for the Windows build; each command must change the file.
#
# usage: win32/patch-source.sh <source file> <sed script> <output file>
set -eu
src=$1
script=$2
out=$3
while IFS= read -r cmd; do
  case "$cmd" in ''|'#'*) continue ;; esac
  if sed -e "$cmd" "$src" | cmp -s - "$src"; then
    echo "$script: no change in $src by: $cmd" >&2
    exit 1
  fi
done < "$script"
mkdir -p "$(dirname "$out")"
sed -f "$script" "$src" > "$out.tmp"
mv "$out.tmp" "$out"
