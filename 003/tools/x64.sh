#!/usr/bin/env bash
# x64.sh — what the 64-bit build still gets wrong.
#
#     tools/x64.sh
#     tools/x64.sh --high     # and put every allocation above 4 GB
#
# `test.sh` is the project's oracle and it runs one target.  For six rounds
# that was the only target there was, and "fifteen streams byte-identical" was
# the whole claim; the moment a second target exists the claim has a gap in it
# the size of every pointer in the file.
#
# So this asks `test.sh`'s question of `-m64`: compress each image and compare
# the stream against the *32-bit* reference, then expand it and compare against
# the input.  Byte-identical streams across two pointer widths is a much
# stronger statement than either build alone -- it says the arithmetic that
# reaches the range coder does not depend on how wide an address is, which is
# exactly what a decompilation of 1997 x86 has no reason to guarantee and every
# reason to violate.
#
# It also prints what `strict64.log` still holds, because a run that crashes
# and a run that is wrong look the same from here and the truncation count is
# what says which is likelier.  See tools/ptrwidth.py for the four kinds.
#
# `--high` builds with `-DBMF_HIGH_ARENA`, which puts every allocation the
# program makes at an address a four-byte pointer cannot name.  That is the
# check the whole exercise reduces to: with a *low* heap -- which is what x86-64
# Linux hands out for a program this size, and what `bmf.cpp` used to force with
# an arena under 4 GB -- truncating a pointer to 32 bits is the identity and
# nothing fails.  Above 4 GB it faults on the first dereference.  The streams
# come out byte for byte either way, and only the second run proves anything.
#
# It is not the default: `MAP_FIXED_NOREPLACE` is Linux-only, and a program that
# runs correctly at one address is not a property worth gating on.
set -u
# A job killed by a signal is reported by the shell that waits on it, and
# that shell is this one -- so the redirect has to be on the *loops*
# (`done 2>/dev/null`) and not on the command.  Every line this script
# means to say goes to stdout, so nothing of ours is lost with it.
set +m
cd "$(dirname "$0")/.."

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

high=; [ "${1:-}" = --high ] && high=-DBMF_HIGH_ARENA
if ! BMF_OUT="$tmp/bmf64" BMF_BITS=64 ./build.sh $high >"$tmp/build" 2>&1; then
  echo "the 64-bit build failed:" >&2
  grep -m5 'error:' "$tmp/build" >&2
  exit 2
fi

# A signal is not an exit code, and "exits 139" is a worse answer than the
# name of the signal -- SIGSEGV and SIGFPE are different defects and this loop
# sees both.  Anything under 128 is the program's own table (see __exit_402E40).
why() {
  case $1 in
    124)      echo "timed out" ;;
    13[4-9]|1[4-9][0-9]) echo "died on SIG$(kill -l $(($1 - 128)) 2>/dev/null || echo "?$1")" ;;
    *)        echo "exits $1" ;;
  esac
}

# `test.sh`'s corpus, and its exclusion: `out_<name>.bmp` is not an input, it
# is what the decoder is expected to *write* for an input it does not reproduce
# byte for byte.  Globbing `testfiles/*.bmp` picked both of those up and then
# looked for a reference stream that could not exist.
same=0 ran=0 bad=0
for f in $(ls testfiles/*.bmp | grep -v '/out_' | sort); do
  n=$(basename "$f" .bmp)
  ran=$((ran + 1))
  # `$?` after `if ! cmd` is the negation's status and is always 0, which is
  # how the first version of this reported "compress exits 0" under a line of
  # SIGSEGV.  Run it, then read the code.
  timeout 120 "$tmp/bmf64" c "$f" "$tmp/$n.bmf" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    printf '%-9s compress %s\n' "$n" "$(why $rc)"
    bad=$((bad + 1)); continue
  fi
  timeout 120 "$tmp/bmf64" d "$tmp/$n.bmf" "$tmp/$n.out" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    printf '%-9s expand %s\n' "$n" "$(why $rc)"
    bad=$((bad + 1)); continue
  fi
  # An RLE-compressed BMP does not come back byte for byte: BMF decodes the
  # runs on the way in and re-encodes them with its own run splitting, so the
  # pixels are identical and the file is not.  `testfiles/out_<name>.bmp` is
  # what the decoder is expected to write -- the same rule `test.sh` uses, and
  # not having it here made `rle4` and `rle8` look like x64 defects.
  want=$f
  [ -f "testfiles/out_$n.bmp" ] && want="testfiles/out_$n.bmp"
  if ! cmp -s "$tmp/$n.out" "$want"; then
    printf '%-9s round trip differs from %s\n' "$n" "$(basename "$want")"
    bad=$((bad + 1)); continue
  fi
  # `testfiles/ref_<name>.bmf`, which is what `test.sh` calls it.  The first
  # version of this said `testfiles/$n.bmf` and guarded the comparison with
  # `[ -f ]`, so the guard was false for every image and the whole
  # stream check -- the entire reason this script exists -- never ran once.
  # A missing reference is a failure and not a skip, for that exact reason.
  ref=testfiles/ref_$n.bmf
  if [ ! -f "$ref" ]; then
    printf '%-9s no reference stream (%s) -- run tools/mkrefs.sh\n' "$n" "$ref"
    bad=$((bad + 1)); continue
  fi
  if ! cmp -s "$tmp/$n.bmf" "$ref"; then
    printf '%-9s round trips, but the stream is not the 32-bit one (%s vs %s bytes)\n' \
      "$n" "$(stat -c%s "$tmp/$n.bmf")" "$(stat -c%s "$ref")"
    bad=$((bad + 1)); continue
  fi
  same=$((same + 1))
done 2>/dev/null

# The archive, because appending walks the members and `bmf_close_archive`
# rewrites a header -- a path no single image reaches.
arc=$tmp/arc.bmf
"$tmp/bmf64" c testfiles/t1.bmp "$arc" >/dev/null 2>&1
"$tmp/bmf64" c testfiles/t8g.bmp "$arc" >/dev/null 2>&1
# Both members expand to the same output name, so what is left there is the
# last one -- which is what `test.sh` compares too.
"$tmp/bmf64" d "$arc" "$tmp/arc.bmp" >/dev/null 2>&1
ran=$((ran + 1))
if cmp -s "$tmp/arc.bmp" testfiles/t8g.bmp; then
  same=$((same + 1))
else
  echo "archive   the last member is not the input"
  bad=$((bad + 1))
fi

# And the malformed suite, which is where an exit code and not a stream is the
# answer.  `test.sh` builds these from whatever the corpus currently is; the
# five here are the ones `tools/asan.sh` keeps, for the same reason.
if [ -f testfiles/ref_t24.bmf ]; then
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
d=bytearray(open("testfiles/ref_t8g.bmf","rb").read())
struct.pack_into("<H",d,4,1)
open("'"$tmp"'/narrow.bmf","wb").write(d)
open("'"$tmp"'/rawlen.bmf","wb").write(
    struct.pack("<4sHHHHHBBI",b"\x81\x8a20",8,8,0,0,0,8,0x04,100000)+b"\xaa"*100000)'
  for m in shortlen len1 narrow depthff rawlen; do
    timeout 120 "$tmp/bmf64" d "$tmp/$m.bmf" "$tmp/$m.out" >/dev/null 2>&1
    rc=$?
    ran=$((ran + 1))
    # Refused, not crashed: the program's own table is 1..8.
    if [ "$rc" -ge 1 ] && [ "$rc" -le 8 ]; then
      same=$((same + 1))
    else
      printf '%-9s malformed input %s\n' "$m" "$(why $rc)"
      bad=$((bad + 1))
    fi
  done 2>/dev/null
fi

BMF_STRICT=1 BMF_BITS=64 ./build.sh >/dev/null 2>&1
left=$(python3 tools/ptrwidth.py subs1.hpp | tail -1)

echo
echo "$same of $ran cases agree with the 32-bit build${high:+, every allocation above 4 GB}"
echo "$left"
exit $((bad ? 1 : 0))
