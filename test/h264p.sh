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
# only be stored verbatim.  Compression is tried at each of the model's
# levels in LEVELS (default: all of them, "9 7 5 3 1"), and at the last one
# once more through pipes, where the dry run writes back every NAL unit
# instead of leaving slices to the model pass to check: the output must be
# the same.

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
  # and one that changes the picture size, chroma format, bit depth, slice
  # groups and field coding from stream to stream
  cat "$here"/gen/x264_base.264 "$here"/h264p/x264_422.264 "$here"/gen/jm_direct_t_noinfer_cavlc.264 \
    "$here"/h264p/jm_fmo3.264 "$here"/gen/jm_hier_b.264 "$here"/h264p/x264_420_10.264 "$here"/h264p/jm_paff.264 \
    > "$tmp/mixed.264"
  set -- "$here"/002/*.h264 "$here"/gen/*.264 "$here"/h264p/*.264 "$tmp/concat.264" "$tmp/mixed.264"
fi

# round trip: $1 stream, $2/$3 the commands there and back, $4 the
# intermediate file, $5 more options there; prints the reason of a failure
trip() {
  if ! "$bin" "$2" -s $5 "$1" "$4" 2> "$tmp/log"; then
    echo "$2: $(head -1 "$tmp/log")"
  elif ! "$bin" "$3" "$4" "$tmp/t.264" 2> "$tmp/log"; then
    echo "$3: $(head -1 "$tmp/log")"
  elif ! cmp -s "$1" "$tmp/t.264"; then
    echo "$3: the stream differs"
  fi
}

levels=${LEVELS:-9 7 5 3 1}
pass=0
fail=0
total=0
declare -A packed
for s in "$@"; do
  [ -f "$s" ] || continue
  why=$(trip "$s" t r "$tmp/t.tsv")
  sizes=""
  for l in $levels; do
    [ -z "$why" ] && why=$(trip "$s" c d "$tmp/t.c" "-$l")
    [ -z "$why" ] && b=$(wc -c < "$tmp/t.c") && packed[$l]=$((${packed[$l]:-0} + b)) && sizes="$sizes, -$l $b"
  done
  if [ -z "$why" ] && [ -n "$l" ]; then
    "$bin" c -s -$l - - < "$s" > "$tmp/p.c" 2> /dev/null && cmp -s "$tmp/t.c" "$tmp/p.c" || why="c through pipes differs"
  fi
  if [ -n "$why" ]; then
    echo "FAIL $s ($why)"
    fail=$((fail + 1))
  else
    a=$(wc -c < "$s")
    total=$((total + a))
    echo "PASS $s ($(wc -l < "$tmp/t.tsv") rows; $a bytes${sizes})"
    pass=$((pass + 1))
  fi
done
echo "passed $pass, failed $fail; $total bytes compressed:$(for l in $levels; do printf ' -%s %s' $l ${packed[$l]:-0}; done)"
[ $fail -eq 0 ]
