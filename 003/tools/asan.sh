#!/usr/bin/env bash
# asan.sh — run every test image under AddressSanitizer.
#
#     tools/asan.sh              # every image, compress and expand
#     tools/asan.sh testfiles/altp1.bmp
#
# `test.sh` says the fifteen streams are byte-identical.  That is the strongest
# statement this project has about correctness and it is silent about one whole
# class: a body that reads or writes past the end of something and happens not
# to change the answer.  The decompilation is full of candidates -- MSVC laid
# locals next to each other and these bodies knew it, which is why the frames
# exist and why `liftframe.py` has five entries under "tried and reverted".
#
# The one that was here is the reason this file is:
#
#     ERROR: AddressSanitizer: stack-buffer-overflow
#     WRITE of size 16 ... in frame [32, 36) 'g1'
#
# `choose_plane_coding` cleared twelve sixteen-byte chunks from byte 64 to byte
# 255 of a run of three locals that is 120 bytes long.  It happens at the top of
# the body, so what it clobbered was uninitialised, and fifteen byte-identical
# streams said nothing about it for nine rounds.  REFACTORING9.md section 43 has
# the reading and the fix.
#
# Slow (an ASan build plus two runs an image), so it is not in `test.sh` -- the
# same bargain as `sweep.sh`.  What it is *for* is being run after a lift: a
# frame whose members become ordinary locals is exactly the change that turns a
# deliberate walk over neighbours into an overflow.
set -u
cd "$(dirname "$0")/.."

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Dynamic and without --gc-sections: ASan's runtime does not survive `-static`
# here, and section GC hides the frames the report needs.
if ! BMF_OUT="$tmp/bmf" BMF_STATIC=0 BMF_GC=0 ./build.sh \
        -fsanitize=address -fno-omit-frame-pointer -g -O1 >"$tmp/build" 2>&1; then
  echo "the ASan build failed:" >&2
  tail -20 "$tmp/build" >&2
  exit 2
fi

files=("$@")
[ "${#files[@]}" -gt 0 ] || files=(testfiles/*.bmp)

bad=0 ran=0

# Malformed input first, because that is where this instrument earns its keep.
# `test.sh` checks an exit code, so it can only hold on to a defect that changes
# one; the whole point of this class is the ones that do not.  `shortlen.bmf` is
# the example: `data_len` halved, so the range decoder runs off the end of the
# coded buffer -- and the -O2 build reads whatever malloc put there next, keeps
# decoding, and still exits 4, exactly as it does now that the read is bounded.
# Under ASan the two are a heap-buffer-overflow and a clean refusal.  `len1` is
# the same field taken to 1, which is a four-byte packer read out of a one-byte
# buffer and 4 either way for the same reason.  `medflag` is the one flag bit
# that sends `ref_t24` through the MED expander, whose unfolding table was a
# byte short of the codes its own inverse emits (REFACTORING9.md §49) -- the
# fast form of what `tools/hdrscan.sh` finds by walking all 256.  The other two
# here change the exit code, so they are in `test.sh` as well; they are repeated
# because this is where the overflow itself is visible rather than its
# consequence.
#
# Built here rather than kept in testfiles/ so that it is derived from whatever
# the corpus currently is, the way test.sh's truncations are.
if [ "$#" = 0 ] && [ -f testfiles/ref_t24.bmf ]; then
  python3 -c 'import struct
d=bytearray(open("testfiles/ref_t24.bmf","rb").read())
struct.pack_into("<I",d,16,struct.unpack_from("<I",d,16)[0]//2)
open("'"$tmp"'/shortlen.bmf","wb").write(d)
d=bytearray(open("testfiles/ref_t24.bmf","rb").read())
d[14]=0xFF
open("'"$tmp"'/depthff.bmf","wb").write(d)
d=bytearray(open("testfiles/ref_t24.bmf","rb").read())
struct.pack_into("<I",d,16,1)
open("'"$tmp"'/len1.bmf","wb").write(d)
d=bytearray(open("testfiles/ref_t24.bmf","rb").read())
d[15]&=~8
open("'"$tmp"'/medflag.bmf","wb").write(d)
open("'"$tmp"'/rawlen.bmf","wb").write(
    struct.pack("<4sHHHHHBBI",b"\x81\x8a20",8,8,0,0,0,8,0x04,100000)+b"\xaa"*100000)'
  for m in shortlen len1 medflag depthff rawlen; do
    ASAN_OPTIONS=detect_leaks=0 "$tmp/bmf" d "$tmp/$m.bmf" "$tmp/$m.out" \
        >"$tmp/$m.log" 2>&1
    ran=$((ran + 1))
    if grep -q 'ERROR: AddressSanitizer' "$tmp/$m.log"; then
      bad=$((bad + 1))
      printf '%-14s %s\n' "$m" \
        "$(sed 's/^==[0-9]*==//' "$tmp/$m.log" | grep -m1 'ERROR: AddressSanitizer')"
    fi
  done
fi

for f in "${files[@]}"; do
  n=$(basename "$f" .bmp)
  ASAN_OPTIONS=detect_leaks=0 "$tmp/bmf" c "$f" "$tmp/$n.bmf" >"$tmp/$n.c" 2>&1
  ASAN_OPTIONS=detect_leaks=0 "$tmp/bmf" d "$tmp/$n.bmf" "$tmp/$n.bmp" >"$tmp/$n.d" 2>&1
  ran=$((ran + 2))
  hits=$(grep -l 'ERROR: AddressSanitizer' "$tmp/$n.c" "$tmp/$n.d" 2>/dev/null)
  if [ -n "$hits" ]; then
    bad=$((bad + 1))
    printf '%-14s %s\n' "$n" \
      "$(grep -h -m1 'ERROR: AddressSanitizer' $hits | sed 's/^==[0-9]*==//')"
    # The frame the address is in says which body, which the stack trace does
    # not: the build is stripped of symbols by `-s` in `build.sh`.
    grep -h -m1 -A1 'is located in stack of' $hits | sed 's/^/               /'
  fi
done

echo
# `ran` and not `2 * ${#files[@]}`: the malformed pass above adds to it, and a
# line that computes what it claims to have counted goes on being confident
# while cases are added under it -- which is what test.sh's `cuts + 10` did.
if [ "$bad" = 0 ]; then
  echo "no AddressSanitizer report in $ran runs over ${#files[@]} images"
else
  echo "$bad of $ran runs report to AddressSanitizer"
fi
exit $((bad ? 1 : 0))
