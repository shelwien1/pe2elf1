#!/bin/sh
# Round-trip check.  Usage: ./t.sh [file ...]   (default: book1000)
set -e
[ -x ./coder0 ] || ./build.sh
for f in "${@:-book1000}"; do
  ./coder0 c "$f" /tmp/coder0.c
  ./coder0 d /tmp/coder0.c /tmp/coder0.d
  if cmp -s "$f" /tmp/coder0.d; then r="ok"; else r="MISMATCH"; fi
  printf '%s: %s -> %s bytes  round-trip %s\n' \
      "$f" "$(wc -c < "$f")" "$(wc -c < /tmp/coder0.c)" "$r"
  [ "$r" = ok ]
done
