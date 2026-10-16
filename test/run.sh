#!/bin/bash
# Decode H.264 streams with h264dec and check the output bit-exactly.
#
# usage: test/run.sh [DECODER] [STREAM...]
#
# DECODER defaults to ./h264dec, STREAMs to test/002/*.h264.  The MD5 of the
# decoded YUV is compared with the reference stored in md5.txt next to the
# stream.  A stream without a stored reference is compared with ffmpeg's
# decode of it instead (needs ffmpeg), so any file can be checked.
#
# The references in test/002/md5.txt were made with
#   ffmpeg -i X.h264 -fps_mode passthrough -f rawvideo -pix_fmt yuv420p - | md5sum
# and the JM reference decoder (ldecod) produces identical output for all.
#
# Output is hashed through a pipe, so no multi-GB .yuv files are written.

dec=${1:-./h264dec}
shift
here=$(dirname "$0")
[ $# -eq 0 ] && set -- "$here"/002/*.h264

if [ ! -x "$dec" ]; then
  echo "decoder '$dec' not found, run make first" >&2
  exit 2
fi

log=$(mktemp)
trap 'rm -f "$log"' EXIT

pass=0
fail=0
for s in "$@"; do
  name=$(basename "$s")
  ref=$(awk -v n="$name" '$2 == n { print $1 }' "$(dirname "$s")/md5.txt" 2>/dev/null)
  if [ -z "$ref" ]; then
    if ! command -v ffmpeg > /dev/null; then
      echo "SKIP $s (no reference md5 and no ffmpeg)"
      continue
    fi
    ref=$(ffmpeg -v error -i "$s" -fps_mode passthrough -f rawvideo -pix_fmt yuv420p - | md5sum | cut -c1-32)
  fi
  # fd 3 carries the YUV to md5sum; the decoder's own messages go to the log.
  got=$("$dec" "$s" /dev/fd/3 3>&1 >"$log" 2>&1 | md5sum | cut -c1-32)
  frames=$(awk -F'\t+' '/^Frames:/ { print $2 }' "$log")
  if [ "$got" = "$ref" ]; then
    echo "PASS $s ($frames frames)"
    pass=$((pass + 1))
  else
    echo "FAIL $s ($frames frames) got $got expected $ref"
    fail=$((fail + 1))
  fi
done
echo "passed $pass, failed $fail"
[ $fail -eq 0 ]
