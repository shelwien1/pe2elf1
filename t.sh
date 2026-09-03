#!/bin/sh
# Round-trip check.  Usage: ./t.sh [file ...]   (default: book1000)
set -e
[ -x ./coder0 ] || ./build.sh
for f in "${@:-book1000}"; do
  ./coder0 c "$f" /tmp/coder0.c
  ./coder0 d /tmp/coder0.c /tmp/coder0.d
  if cmp -s "$f" /tmp/coder0.d; then r="ok"; else r="MISMATCH"; fi
  # gN: exactly N bytes out, the same ones every time, no leftover bit source
  ./coder0 g256 "$f" /tmp/coder0.g1 2>/dev/null
  ./coder0 g256 "$f" /tmp/coder0.g2 2>/dev/null
  [ "$(wc -c < /tmp/coder0.g1)" -eq 256 ] || { echo "$f: g256 did not write 256 bytes"; exit 1; }
  cmp -s /tmp/coder0.g1 /tmp/coder0.g2 || { echo "$f: g256 is not deterministic"; exit 1; }
  [ ! -e /tmp/coder0.g1.rnd ] || { echo "$f: g256 left its bit source behind"; exit 1; }
  printf '%s: %s -> %s bytes  round-trip %s\n' \
      "$f" "$(wc -c < "$f")" "$(wc -c < /tmp/coder0.c)" "$r"
  [ "$r" = ok ]
done
