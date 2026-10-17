#!/bin/bash
# Round-trip H.264 streams through h264p: dump to TSV (c), restore (d) and
# compare with the original byte by byte.
#
# usage: test/h264p.sh [H264P] [STREAM...]
#
# H264P defaults to ./h264p, STREAMs to all streams under test/ and a
# concatenation of some of them.  The dump is made with -s, so a stream also
# fails when one of its NAL units of a supported type could only be stored
# verbatim.

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

pass=0
fail=0
for s in "$@"; do
  [ -f "$s" ] || continue
  if ! "$bin" c -s "$s" "$tmp/t.tsv" 2> "$tmp/log"; then
    echo "FAIL $s (dump: $(head -1 "$tmp/log"))"
    fail=$((fail + 1))
  elif ! "$bin" d "$tmp/t.tsv" "$tmp/t.264" 2> "$tmp/log"; then
    echo "FAIL $s (restore: $(head -1 "$tmp/log"))"
    fail=$((fail + 1))
  elif ! cmp -s "$s" "$tmp/t.264"; then
    echo "FAIL $s (restored stream differs)"
    fail=$((fail + 1))
  else
    echo "PASS $s ($(wc -l < "$tmp/t.tsv") rows)"
    pass=$((pass + 1))
  fi
done
echo "passed $pass, failed $fail"
[ $fail -eq 0 ]
