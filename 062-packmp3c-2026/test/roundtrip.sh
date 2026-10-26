#!/bin/sh
# roundtrip.sh BIN files... : each file through BIN c and d with one model, and with
# $OPTS (default -t 3 -c 64k); it must come back, and where $MP3RW names an mp3rw
# build, the outside data must be its .out. Prints a line per file; exit status 1
# if any failed.
BIN=$1; shift
OPTS=${OPTS:--t 3 -c 64k}
T=$(mktemp -d) || exit 1
trap 'rm -rf "$T"' EXIT
bad=0
for f in "$@"; do
  r=ok
  "$BIN" c "$f" "$T/a.pk" "$T/a.out" && "$BIN" d "$T/a.pk" "$T/a.out" "$T/a.bin" && cmp -s "$f" "$T/a.bin" || r=FAIL
  "$BIN" c $OPTS "$f" "$T/b.pk" "$T/b.out" && "$BIN" d "$T/b.pk" "$T/b.out" "$T/b.bin" && cmp -s "$f" "$T/b.bin" || r="$r,FAIL($OPTS)"
  if [ -n "$MP3RW" ]; then
    "$MP3RW" c "$f" "$T/r.dmp" "$T/r.out" >/dev/null 2>&1
    cmp -s "$T/r.out" "$T/a.out" || r="$r,FAIL(.out)"
  fi
  [ "$r" = ok ] || bad=1
  echo "$f: $r ($(wc -c < "$T/a.pk") bytes, $(wc -c < "$T/b.pk") with $OPTS, $(wc -c < "$T/a.out") of outside data)"
done
exit $bad
