#!/bin/bash
# Round-trip H.264 streams through h264p, both ways it has: compress (c) and
# decompress (d), and dump to TSV (t) and restore (r); compare each result
# with the original byte by byte.
#
# usage: test/h264p.sh [H264P] [STREAM...]
#
# H264P defaults to ./h264p, STREAMs to all streams under test/ and a
# concatenation of some of them.  Compression and the dump are run with -s,
# so a stream also fails when one of its NAL units of a supported type could
# only be stored verbatim.

bin=${1:-./h264p}
shift
here=$(dirname "$0")

if [ ! -x "$bin" ]; then
  echo "h264p '$bin' not found, run make first" >&2
  exit 2
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ $# -eq 0 ]; then
  # and a concatenation, in which x264's padding bits change phase and stop
  cat "$here"/gen/x264_base.264 "$here"/gen/jm_hier_b.264 "$here"/002/02.h264 "$here"/gen/x264_slices4.264 \
    > "$tmp/concat.264"
  set -- "$here"/002/*.h264 "$here"/gen/*.264 "$here"/h264p/*.264 "$tmp/concat.264"
fi

# round trip: $1 stream, $2/$3 the commands there and back, $4 the
# intermediate file; prints the reason of a failure
trip() {
  if ! "$bin" "$2" -s "$1" "$4" 2> "$tmp/log"; then
    echo "$2: $(head -1 "$tmp/log")"
  elif ! "$bin" "$3" "$4" "$tmp/t.264" 2> "$tmp/log"; then
    echo "$3: $(head -1 "$tmp/log")"
  elif ! cmp -s "$1" "$tmp/t.264"; then
    echo "$3: the stream differs"
  fi
}

pass=0
fail=0
total=0
packed=0
for s in "$@"; do
  [ -f "$s" ] || continue
  why=$(trip "$s" t r "$tmp/t.tsv")
  [ -z "$why" ] && why=$(trip "$s" c d "$tmp/t.c")
  if [ -n "$why" ]; then
    echo "FAIL $s ($why)"
    fail=$((fail + 1))
  else
    a=$(wc -c < "$s")
    b=$(wc -c < "$tmp/t.c")
    total=$((total + a))
    packed=$((packed + b))
    echo "PASS $s ($(wc -l < "$tmp/t.tsv") rows; $a -> $b bytes)"
    pass=$((pass + 1))
  fi
done
echo "passed $pass, failed $fail; compressed $total -> $packed bytes"
[ $fail -eq 0 ]
