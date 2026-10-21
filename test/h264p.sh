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
# the same; and at that level by x too (the compressor's coroutine feeding
# the decompressor's, a CoroutinePair), whose output must be the stream.
# At that level, last, the stream is compressed on threads (-t, in parts of
# at least 16 KB that start at IDR access units): by c on 3 threads and d,
# through pipes on 2, and by x on 2 (MTwrap's coder into its decoder); c
# and x there with the progress line on (-p), which must end done.

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
  negative=1  # and the negative and crafted cases below
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
  if [ -z "$why" ] && [ -n "$l" ]; then
    "$bin" x -s -$l "$s" "$tmp/x.264" 2> /dev/null && cmp -s "$s" "$tmp/x.264" || why="x: the stream differs"
  fi
  if [ -z "$why" ] && [ -n "$l" ]; then
    why=$(trip "$s" c d "$tmp/m.c" "-$l -t 3 -c 16k -p")
    [ -z "$why" ] && [ "$(head -c 5 "$tmp/m.c")" != h264m ] && why="c -t: not a stream in parts"
  fi
  if [ -z "$why" ] && [ -n "$l" ]; then
    "$bin" c -s -$l -t 2 -c 16k - - < "$s" 2> /dev/null | "$bin" d - - > "$tmp/p.264" 2> /dev/null &&
      cmp -s "$s" "$tmp/p.264" || why="c -t, d through pipes: the stream differs"
  fi
  if [ -z "$why" ] && [ -n "$l" ]; then
    "$bin" x -s -$l -t 2 -c 16k -p "$s" "$tmp/x.264" 2> "$tmp/log" && cmp -s "$s" "$tmp/x.264" || why="x -t: the stream differs"
    [ -z "$why" ] && ! grep -q "done in" "$tmp/log" && why="x -t -p: no progress line"
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

# With no streams given, the cases that must fail, and must fail well, and
# the crafted streams of test/crafted (h264p_review.md): each a stream h264p
# once crashed on (poc_wrap), took minutes (fmo_boxout) or gigabytes
# (empty_blocks; kinds, 261 bytes: a slice of each kind of picture at the
# largest size, made with r) for, failed as a whole (big_level),
# mismodelled (pps_repeat) or read and wrote differently (sei_size)
if [ -n "${negative:-}" ]; then
  nfail=0
  check() {  # $1 what, then a command that must succeed
    local what=$1
    shift
    if "$@"; then echo "PASS $what"; else echo "FAIL $what"; nfail=$((nfail + 1)); fi
  }
  for s in "$here"/crafted/*.264; do
    n=$(basename "$s")
    check "$n: c -9, d" sh -c 'timeout 300 "$1" c -9 "$2" "$3/k.c" 2>/dev/null && "$1" d "$3/k.c" "$3/k.264" && cmp -s "$2" "$3/k.264"' - "$bin" "$s" "$tmp"
    check "$n: t, r (in 20 s)" sh -c 'timeout 20 "$1" t "$2" "$3/k.tsv" 2>/dev/null && "$1" r "$3/k.tsv" "$3/k.264" && cmp -s "$2" "$3/k.264"' - "$bin" "$s" "$tmp"
  done
  check "kinds.264: c -9, d in 1.05 GB of address space (1.64 GB unbounded)" sh -c 'ulimit -v 1100000 2>/dev/null; "$1" c -9 "$2" "$3/k.c" 2>/dev/null && "$1" d "$3/k.c" "$3/k.264" && cmp -s "$2" "$3/k.264"' - "$bin" "$here/crafted/kinds.264" "$tmp"
  check "sei_size.264: t -s fails (a size past 2^24)" sh -c '! "$1" t -s "$2" "$3/k.tsv" 2>/dev/null' - "$bin" "$here/crafted/sei_size.264" "$tmp"
  # a slice with a byte changed: stored verbatim, or with -s a failure that
  # leaves no output, on threads too; x on threads fails rather than hangs
  cp "$here/gen/x264_base.264" "$tmp/bad.264"
  printf '\001' | dd of="$tmp/bad.264" bs=1 seek=5215 conv=notrunc 2>/dev/null
  check "a corrupt slice: c, d" sh -c '"$1" c "$2/bad.264" "$2/k.c" 2>/dev/null && "$1" d "$2/k.c" "$2/k.264" && cmp -s "$2/bad.264" "$2/k.264"' - "$bin" "$tmp"
  for t in 1 2; do
    rm -f "$tmp/k.c"
    check "a corrupt slice: c -s -t $t fails, no output" sh -c '! "$1" c -s -1 -t $3 -c 1k "$2/bad.264" "$2/k.c" 2>/dev/null && [ ! -e "$2/k.c" ]' - "$bin" "$tmp" $t
  done
  cat "$here"/002/*.h264 > "$tmp/big.264"
  printf '\126' | dd of="$tmp/big.264" bs=1 seek=5770 conv=notrunc 2>/dev/null
  check "a corrupt slice early: x -s -t 2 fails, no hang" sh -c 'timeout 300 "$1" x -s -1 -t 2 -c 16k "$2/big.264" "$2/k.264" 2>/dev/null; [ $? -eq 1 ]' - "$bin" "$tmp"
  # the input and the output the same file, by its name or a link
  cp "$here/gen/x264_base.264" "$tmp/same.264"
  ln -sf "$tmp/same.264" "$tmp/link.264"
  check "c with the input as output fails, the input kept" sh -c '! "$1" c -1 "$2/same.264" "$2/same.264" 2>/dev/null && ! "$1" c -1 "$2/same.264" "$2/link.264" 2>/dev/null && cmp -s "$2/same.264" "$3"' - "$bin" "$tmp" "$here/gen/x264_base.264"
  # a failed c leaves an existing output as it was
  echo old > "$tmp/old.c"
  check "a failed c keeps the old output" sh -c '! "$1" c -s -1 "$2/bad.264" "$2/old.c" 2>/dev/null && [ "$(cat "$2/old.c")" = old ]' - "$bin" "$tmp"
  # cut short: d fails, single model and on threads
  "$bin" c -1 "$here/gen/x264_base.264" "$tmp/k.c" 2>/dev/null && head -c 9000 "$tmp/k.c" > "$tmp/cut.c"
  "$bin" c -1 -t 2 -c 1k "$here/gen/x264_base.264" "$tmp/m.c" 2>/dev/null && head -c 9000 "$tmp/m.c" > "$tmp/mcut.c"
  check "a cut compressed stream: d fails" sh -c '! "$1" d "$2/cut.c" "$2/k.264" 2>/dev/null && ! "$1" d "$2/mcut.c" "$2/k.264" 2>/dev/null' - "$bin" "$tmp"
  if [ -w /dev/full ]; then
    check "t and r to a full device fail" sh -c '! "$1" t "$2" - > /dev/full 2>/dev/null && "$1" t "$2" "$3/k.tsv" && ! "$1" r "$3/k.tsv" - > /dev/full 2>/dev/null' - "$bin" "$here/h264p/x264_16x16.264" "$tmp"
  fi
  echo "negative and crafted cases: $nfail failed"
  [ $nfail -eq 0 ] || fail=$((fail + 1))
fi
[ $fail -eq 0 ]
