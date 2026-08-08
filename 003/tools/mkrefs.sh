#!/usr/bin/env bash
# mkrefs.sh — regenerate testfiles/ref_<name>.bmf, the streams test.sh compares
# against.
#
#   tools/mkrefs.sh [./bmf]
#
# Run this only when a change is *meant* to move the compressed output, and say
# in the commit message why.  Every other time, a stream that no longer matches
# its reference is the gate doing its job.
#
# The references are what the current build produces, not what BMF.exe produces
# — there is no wine here to run the donor.  They lock in the behaviour the
# decompilation had when they were made, which is what a refactoring gate needs;
# they do not independently prove that behaviour is the donor's.
set -eu
cd "$(dirname "$0")/.."

BIN=${1:-./bmf}
[ -x "$BIN" ] || { echo "no $BIN (run ./build.sh)"; exit 1; }

n=0
for img in testfiles/*.bmp; do
  ref="testfiles/ref_$(basename "$img" .bmp).bmf"
  rm -f "$ref"                       # bmf opens its output "w+b"; be explicit
  "$BIN" c "$img" "$ref" >/dev/null || { echo "compress failed: $img"; exit 1; }
  printf '%-28s %8s bytes\n' "$ref" "$(stat -c%s "$ref")"
  n=$((n + 1))
done
echo "$n reference streams written"
