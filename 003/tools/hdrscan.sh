#!/usr/bin/env bash
# hdrscan.sh — every value of the member header's one-byte fields, under ASan.
#
#     tools/hdrscan.sh                      # every reference stream
#     tools/hdrscan.sh testfiles/ref_t1.bmf
#
# `tools/fuzz.sh` samples.  This enumerates, over the part of the input small
# enough to enumerate: a `.bmf` member header is four bytes of magic and then
# the sixteen-byte BmfImage `compress_image` wrote, and two of its fields --
# the depth at +14 and the flag byte at +15 -- are one byte each.  512 runs a
# stream covers both completely, which is a claim no amount of fuzzing can
# make: *every* value has been tried, not enough of them to feel confident.
#
# It is worth having because both fields turned out to be believed, and the two
# defects are the kind that hide from sampling in different ways:
#
#   depth   six of its bits are the depth, so it can say 63, and
#           `(63 + 7) >> 3` is eight planes written into the five records
#           `plane_desc` holds.  One value in 256, and a global-buffer-overflow.
#   flags   bit 1 asks `expand_image` to deinterleave -- a walk of
#           `width * height * plane_count` bytes that is only the buffer's size
#           when the image is a byte per plane per pixel.  For a packed image
#           it is up to eight times it.  Eleven values in 256 reach it, and
#           every one of them needs bit 5 clear as well, which is why a random
#           byte finds it about once in a hundred tries and a scan finds it
#           eleven times in five hundred.
#
# `REFACTORING9.md` §48.  Slow (an ASan build plus 512 runs a stream), so it is
# not in `test.sh` -- the same bargain as `sweep.sh` and `asan.sh`.
set -u
cd "$(dirname "$0")/.."

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if ! BMF_OUT="$tmp/bmf" BMF_STATIC=0 BMF_GC=0 ./build.sh \
        -fsanitize=address -fno-omit-frame-pointer -g -O1 >"$tmp/build" 2>&1; then
  echo "the ASan build failed:" >&2
  tail -20 "$tmp/build" >&2
  exit 2
fi

files=("$@")
[ "${#files[@]}" -gt 0 ] || files=(testfiles/ref_*.bmf)

bad=0 ran=0
for f in "${files[@]}"; do
  n=$(basename "$f" .bmf)
  for off in 14 15; do
    field=depth; [ "$off" = 15 ] && field=flags
    v=0
    while [ "$v" -lt 256 ]; do
      python3 -c 'import sys
d=bytearray(open(sys.argv[1],"rb").read()); d[int(sys.argv[2])]=int(sys.argv[3])
open(sys.argv[4],"wb").write(d)' "$f" "$off" "$v" "$tmp/m.bmf"
      ( ulimit -f 262144 -v 2097152
        ASAN_OPTIONS=detect_leaks=0 "$tmp/bmf" d "$tmp/m.bmf" "$tmp/o.bmp" ) \
          >"$tmp/log" 2>&1
      rc=$?
      ran=$((ran + 1))
      rm -f "$tmp/o.bmp"
      msg=$(grep -m1 'ERROR: AddressSanitizer' "$tmp/log" 2>/dev/null |
            sed 's/^==[0-9]*==ERROR: AddressSanitizer: //; s/ on address.*//; s/ at pc.*//')
      case $msg in
        # Same reasoning as fuzz.sh: an allocator message is as often the
        # ulimit above as it is the program, and neither is decided by reading
        # the message.
        ''|*'out of memory'*|*'exceeds maximum'*|*'allocation size'*) ;;
        *) bad=$((bad + 1))
           printf '%-14s %-6s=%-3d %s\n' "$n" "$field" "$v" "$msg" ;;
      esac
      [ "$rc" -le 8 ] || [ -n "$msg" ] ||
        { bad=$((bad + 1)); printf '%-14s %-6s=%-3d exited %d\n' "$n" "$field" "$v" "$rc"; }
      v=$((v + 1))
    done
  done
done

echo
if [ "$bad" = 0 ]; then
  echo "no report in $ran runs: both one-byte header fields, every value, ${#files[@]} streams"
else
  echo "$bad of $ran runs report"
fi
exit $((bad ? 1 : 0))
