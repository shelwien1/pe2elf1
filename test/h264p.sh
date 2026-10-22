#!/bin/bash
# Round-trip H.264 streams through h264p: compress (c) and decompress (d),
# and compare the result with the original byte by byte.
#
# usage: test/h264p.sh [H264P] [STREAM...]
#
# H264P defaults to ./h264p, STREAMs to all streams under test/ and a
# concatenation of some of them.  Compression is run with -s, so a stream
# also fails when one of its NAL units of a supported type could only be
# stored verbatim.  It is tried at each of the model's levels in LEVELS
# (default: all of them, "9 7 5 3 1"), and at the last one once more
# through pipes, where the dry run writes back every NAL unit instead of
# leaving slices to the model pass to check: the output must be the same.
# At that level, last, the stream is compressed on threads (-t, in parts of
# at least 16 KB that start at IDR access units): on 3 threads, on 2
# through pipes, and on 2 with the progress line on (-p), which must end
# done.

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
  why=""
  sizes=""
  for l in $levels; do
    [ -z "$why" ] && why=$(trip "$s" c d "$tmp/t.c" "-$l")
    [ -z "$why" ] && b=$(wc -c < "$tmp/t.c") && packed[$l]=$((${packed[$l]:-0} + b)) && sizes="$sizes, -$l $b"
  done
  if [ -z "$why" ] && [ -n "$l" ]; then
    "$bin" c -s -$l - - < "$s" > "$tmp/p.c" 2> /dev/null && cmp -s "$tmp/t.c" "$tmp/p.c" || why="c through pipes differs"
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
    "$bin" c -s -$l -t 2 -c 16k -p "$s" "$tmp/q.c" 2> "$tmp/log" && "$bin" d "$tmp/q.c" "$tmp/q.264" 2> /dev/null &&
      cmp -s "$s" "$tmp/q.264" || why="c -t -p, d: the stream differs"
    [ -z "$why" ] && ! grep -q "done in" "$tmp/log" && why="c -t -p: no progress line"
  fi
  if [ -n "$why" ]; then
    echo "FAIL $s ($why)"
    fail=$((fail + 1))
  else
    a=$(wc -c < "$s")
    total=$((total + a))
    echo "PASS $s ($a bytes${sizes})"
    pass=$((pass + 1))
  fi
done
echo "passed $pass, failed $fail; $total bytes compressed:$(for l in $levels; do printf ' -%s %s' $l ${packed[$l]:-0}; done)"

# With no streams given, the cases that must fail, and must fail well, and
# the crafted streams of test/crafted (h264p_review.md): each a stream h264p
# once crashed on (poc_wrap), took minutes (fmo_boxout) or gigabytes
# (empty_blocks; kinds, 261 bytes: a slice of each kind of picture at the
# largest size) for, failed as a whole (big_level), mismodelled
# (pps_repeat), read and wrote differently (sei_size) or, with -t, could
# not restore (aud_last: a last part that is only an access unit delimiter)
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
  done
  check "kinds.264: c -9, d in 1.05 GB of address space (1.64 GB unbounded)" sh -c 'ulimit -v 1100000 2>/dev/null; "$1" c -9 "$2" "$3/k.c" 2>/dev/null && "$1" d "$3/k.c" "$3/k.264" && cmp -s "$2" "$3/k.264"' - "$bin" "$here/crafted/kinds.264" "$tmp"
  # no start code at all: -t cuts it at twice the cap (8 KB at -c 1k)
  head -c 50000 /dev/zero | tr '\0' '\377' > "$tmp/ff.264"
  check "50 KB with no start code: c -t 2 -c 1k in parts, d" sh -c '"$1" c -1 -v -t 2 -c 1k "$2/ff.264" "$2/k.c" 2>"$2/log" && grep -q " [2-9] parts" "$2/log" && "$1" d "$2/k.c" "$2/k.264" && cmp -s "$2/ff.264" "$2/k.264"' - "$bin" "$tmp"
  # large parameter sets (random, no zero byte) that no slice uses, then
  # small parts: -t copies none into them (it copied every SPS-like set into
  # every part, 8 times the input here)
  { for b in '\377' '\137' '\177' '\047'; do  # SPS extensions 0, 1, 2 and 3 (ue first)
      printf "\000\000\000\001\015$b"; head -c 40000 /dev/urandom | tr -d '\000' | head -c 30000
    done
    for i in $(seq 60); do printf '\000\000\000\001\014'; head -c 1000 /dev/zero | tr '\0' '\377'; done
  } > "$tmp/psets.264"
  check "large unused parameter sets: c -t 2 -c 1k under twice the input, d" sh -c '"$1" c -1 -t 2 -c 1k "$2/psets.264" "$2/k.c" 2>/dev/null && [ $(wc -c < "$2/k.c") -lt $((2 * $(wc -c < "$2/psets.264"))) ] && "$1" d "$2/k.c" "$2/k.264" && cmp -s "$2/psets.264" "$2/k.264"' - "$bin" "$tmp"
  # an endless run of parameter set versions (two of an SPS extension in
  # 6-byte units, each a new version): -t's address space through 63 MB
  # stays within 128 MB of what 8 MB took (the splitter kept every version:
  # 6 times the input). The least that 8 MB takes is found first.
  printf '\000\000\001\155\377\101\000\000\001\155\377\102' > "$tmp/ab"
  for i in $(seq 17); do cat "$tmp/ab" "$tmp/ab" > "$tmp/ab2" && mv "$tmp/ab2" "$tmp/ab"; done
  lim=
  for l in 150000 175000 200000 225000 250000 275000 300000 350000 400000 500000 700000 1000000; do
    if sh -c 'ulimit -v $1 2>/dev/null || exit 1; for i in 1 2 3 4 5; do cat "$2/ab"; done | "$3" c -1 -t 2 -c 16k - - > /dev/null 2>&1' - $l "$tmp" "$bin"; then lim=$l; break; fi
  done
  check "an endless run of parameter set versions: c -t 2 through 63 MB in what 8 MB took${lim:+ ($lim KB)} and 128 MB" sh -c '[ -z "$1" ] || ulimit -v $(($1 + 131072)); for i in $(seq 40); do cat "$2/ab"; done | "$3" c -1 -t 2 -c 16k - - > /dev/null' - "$lim" "$tmp" "$bin"
  # a run of zero bytes (100 MB, between two access unit delimiters): c and
  # d count them, in the address space that 1 MB takes and 16 MB (c held
  # them); and a NAL unit of 68 MB, past the 64 MB c holds of one: stored
  # as it is read, and restored
  lim=
  for l in 20000 30000 40000 50000 60000 80000 100000 150000 200000 300000 500000; do
    if sh -c 'ulimit -v $1 2>/dev/null || exit 1; { printf "\000\000\000\001\011\360"; head -c 1000000 /dev/zero; printf "\000\000\000\001\011\360"; } | "$2" c -1 - - > /dev/null 2>&1' - $l "$bin"; then lim=$l; break; fi
  done
  { printf '\000\000\000\001\011\360'; head -c 100000000 /dev/zero; printf '\000\000\000\001\011\360'; } > "$tmp/z.264"
  check "100 MB of zero bytes between two NAL units: c and d in what 1 MB took${lim:+ ($lim KB)} and 16 MB" sh -c '[ -z "$1" ] || ulimit -v $(($1 + 16384)); "$2" c -1 "$3/z.264" "$3/z.c" && "$2" d "$3/z.c" "$3/z.d" && cmp -s "$3/z.264" "$3/z.d"' - "$lim" "$bin" "$tmp"
  rm -f "$tmp/z.264" "$tmp/z.d"
  { printf '\000\000\000\001\016'; head -c 68000000 /dev/zero | tr '\0' '\125'; cat "$here/h264p/x264_16x16.264"; } > "$tmp/long.264"
  check "a NAL unit of 68 MB: c, d" sh -c '"$1" c -1 "$2/long.264" "$2/k.c" && "$1" d "$2/k.c" "$2/k.264" && cmp -s "$2/long.264" "$2/k.264"' - "$bin" "$tmp"
  rm -f "$tmp/long.264" "$tmp/k.264"
  check "aud_last.264: c -t 2 -c 1k, d" sh -c '"$1" c -1 -t 2 -c 1k "$2" "$3/k.c" 2>/dev/null && "$1" d "$3/k.c" "$3/k.264" && cmp -s "$2" "$3/k.264"' - "$bin" "$here/crafted/aud_last.264" "$tmp"
  check "sei_size.264: c -s fails (a size past 2^24)" sh -c '! "$1" c -s -1 "$2" "$3/k.c" 2>/dev/null' - "$bin" "$here/crafted/sei_size.264" "$tmp"
  # a slice with a byte changed: stored verbatim, or with -s a failure that
  # leaves no output, on threads too, where it fails rather than hangs
  cp "$here/gen/x264_base.264" "$tmp/bad.264"
  printf '\001' | dd of="$tmp/bad.264" bs=1 seek=5215 conv=notrunc 2>/dev/null
  check "a corrupt slice: c, d" sh -c '"$1" c "$2/bad.264" "$2/k.c" 2>/dev/null && "$1" d "$2/k.c" "$2/k.264" && cmp -s "$2/bad.264" "$2/k.264"' - "$bin" "$tmp"
  for t in 1 2; do
    rm -f "$tmp/k.c"
    check "a corrupt slice: c -s -t $t fails, no output" sh -c '! "$1" c -s -1 -t $3 -c 1k "$2/bad.264" "$2/k.c" 2>/dev/null && [ ! -e "$2/k.c" ]' - "$bin" "$tmp" $t
  done
  cat "$here"/002/*.h264 > "$tmp/big.264"
  printf '\126' | dd of="$tmp/big.264" bs=1 seek=5770 conv=notrunc 2>/dev/null
  check "a corrupt slice early: c -s -t 2 fails, no hang" sh -c 'timeout 300 "$1" c -s -1 -t 2 -c 16k "$2/big.264" "$2/k.c" 2>/dev/null; [ $? -eq 1 ]' - "$bin" "$tmp"
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
  # the last byte gone, or 1 to 3 bytes after the end: d fails (the coder
  # reads exactly the bytes it wrote)
  for x in k m; do
    head -c -1 "$tmp/$x.c" > "$tmp/x1.c"
    check "$x.c with its last byte gone: d fails" sh -c '! "$1" d "$2/x1.c" "$2/k.264" 2>/dev/null' - "$bin" "$tmp"
    for j in '\000' '\377' '\000\377' '\245\245\245'; do
      { cat "$tmp/$x.c"; printf "$j"; } > "$tmp/xj.c"
      check "$x.c with $j after it: d fails" sh -c '! "$1" d "$2/xj.c" "$2/k.264" 2>/dev/null' - "$bin" "$tmp"
    done
  done
  # not h264p's (a stream, nothing, another version's): d fails and leaves
  # no output, not even its temporary file
  : > "$tmp/empty.c"
  printf 'h264c\001\011' > "$tmp/ver.c"
  for x in "$here/gen/x264_base.264" "$tmp/empty.c" "$tmp/ver.c"; do
    check "d of $(basename "$x") fails, no output" sh -c '! "$1" d "$2" "$3/n.264" 2>/dev/null && [ ! -e "$3/n.264" ] && ! ls "$3"/n.264.*.tmp > /dev/null 2>&1' - "$bin" "$x" "$tmp"
  done
  # a part's record of -t that claims 4 GB (its size's high bytes set): d
  # fails as the bytes run out, without taking the 4 GB first
  "$bin" c -1 -t 2 "$here/h264p/x264_16x16.264" "$tmp/m4.c" 2>/dev/null
  printf '\377\377' | dd of="$tmp/m4.c" bs=1 seek=15 conv=notrunc 2>/dev/null
  check "d of a part's record claiming 4 GB fails, in 300 MB" sh -c 'ulimit -v 300000 2>/dev/null; ! "$1" d "$2/m4.c" "$2/k.264" 2>"$2/log" && grep -q "cut short" "$2/log"' - "$bin" "$tmp"
  # d failing to write keeps the old output; a replaced output keeps its
  # mode; an interrupted c leaves no temporary file
  echo old > "$tmp/old.264"
  check "d with a write error keeps the old output" sh -c '(trap "" XFSZ; ulimit -f 8; ! "$1" d "$2/k.c" "$2/old.264" 2>/dev/null) && [ "$(cat "$2/old.264")" = old ]' - "$bin" "$tmp"
  if chmod 600 "$tmp/old.264" 2>/dev/null && [ "$(stat -c %a "$tmp/old.264" 2>/dev/null)" = 600 ]; then
    check "d replacing a file keeps its mode" sh -c '"$1" d "$2/k.c" "$2/old.264" && [ "$(stat -c %a "$2/old.264")" = 600 ]' - "$bin" "$tmp"
  fi
  if command -v timeout > /dev/null; then
    cat "$here"/002/0[0-4].h264 > "$tmp/int.264"
    check "c interrupted leaves no temporary file" sh -c 'timeout -s INT 1 "$1" c -9 "$2/int.264" "$2/int.c"; [ $? -ne 0 ] && ! ls "$2"/int.c* > /dev/null 2>&1' - "$bin" "$tmp"
    # and ended by its progress line's pipe closing (SIGPIPE)
    check "c ended by a closed pipe leaves no temporary file" sh -c '"$1" c -9 -p "$2/int.264" "$2/pp.c" 2>&1 | head -c 1 > /dev/null; ! ls "$2"/pp.c* > /dev/null 2>&1' - "$bin" "$tmp"
  fi
  # /dev/fd/N (/dev/stdout), here through links of our own, as the output:
  # written through the descriptor, where the shell put it (c renamed its
  # output over the file the shell had open, losing what was there and the
  # next call's output, and then renamed one over /dev/stdout itself)
  if [ -e /dev/fd/1 ]; then
    ln -sf /dev/fd/1 "$tmp/o1"
    ln -sf /dev/fd/3 "$tmp/o3"
    check "c and d to /dev/fd/1 and /dev/fd/3 write where redirected (>>)" sh -c '"$1" c -1 "$2" "$3/s.c" && printf old > "$3/so" && { "$1" c -1 "$2" "$3/o1" && "$1" c -1 "$2" "$3/o3" 3>&1 && "$1" d "$3/s.c" "$3/o1"; } >> "$3/so" && { printf old; cat "$3/s.c" "$3/s.c" "$2"; } | cmp -s - "$3/so"' - "$bin" "$here/h264p/x264_16x16.264" "$tmp"
  fi
  # what the video is: the codec tells, the front end says it -- with -p or
  # -v (once for all the parts of -t), with -v where it changes, and
  # nothing on a quiet call
  v16="16x16, High@L1.0, 4:2:0 8-bit, CABAC, progressive, 8x8 transform, weighted P, implicit weighted B, 4 reference frames, 25 fps"
  cat "$here/h264p/x264_16x16.264" "$here/gen/x264_baseline.264" > "$tmp/two.264"
  check "c -p says the video, d -p too" sh -c '"$1" c -1 -p "$2" "$3/v.c" 2>&1 | tr "\r" "\n" | grep -qxF "[enc] video: $4" && "$1" d -p "$3/v.c" "$3/v.264" 2>&1 | tr "\r" "\n" | grep -qxF "[dec] video: $4"' - "$bin" "$here/h264p/x264_16x16.264" "$tmp" "$v16"
  check "c -v says the video and its change; c -t -p once; a quiet c nothing" sh -c '"$1" c -1 -v "$2/two.264" "$2/v.c" 2>"$2/log" && grep -qxF "video: $3" "$2/log" && grep -q "^the video now: 640x352, Constrained Baseline@L3.0, 4:2:0 8-bit, CAVLC" "$2/log" && [ $("$1" c -1 -t 2 -c 1k -p "$2/two.264" "$2/v.c" 2>&1 | tr "\r" "\n" | grep -c "video") -eq 1 ] && "$1" c -1 "$2/two.264" "$2/v.c" 2>"$2/log" && [ ! -s "$2/log" ]' - "$bin" "$tmp" "$v16"
  if [ -w /dev/full ]; then
    check "c and d to a full device fail" sh -c '! "$1" c -1 "$2" - > /dev/full 2>/dev/null && "$1" c -1 "$2" "$3/k.c" && ! "$1" d "$3/k.c" - > /dev/full 2>/dev/null' - "$bin" "$here/h264p/x264_16x16.264" "$tmp"
  fi
  echo "negative and crafted cases: $nfail failed"
  [ $nfail -eq 0 ] || fail=$((fail + 1))
fi
[ $fail -eq 0 ]
