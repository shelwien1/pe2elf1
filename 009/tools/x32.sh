#!/usr/bin/env bash
# x32.sh — what the 32-bit build still gets wrong.
#
#     tools/x32.sh
#
# `test.sh` is the project's oracle and it runs one target.  The moment a second
# target exists the claim has a gap in it the size of every pointer in the file.
#
# So this asks `test.sh`'s question of `-m32`: compress each image and compare
# the stream against the reference, then expand it and compare against the
# input.  Byte-identical streams across two pointer widths is a much stronger
# statement than either build alone -- it says the arithmetic that reaches the
# range coder does not depend on how wide an address is, which is exactly what a
# decompilation of 1997 x86 has no reason to guarantee and every reason to
# violate.
#
# This was `x64.sh` and asked the question the other way round, because the
# default target was `-m32` and the references were that build's.  The default
# is x64 now; the references are unchanged, because the two widths produce the
# same fifteen streams, and that fact is the entire subject of this script.  So
# the same instrument now points at the width `test.sh` no longer covers, and
# the sentence it prints -- "N of M cases agree" -- means what it always did.
#
# It also prints what `strict64.log` still holds.  That is a question about the
# *default* build rather than this one, and it is here because a truncation
# count is what says whether a disagreement is likelier a crash or a wrong
# answer.  See tools/ptrwidth.py for the four kinds.
#
# `--high` is gone with the rename, and is not lost: `-DBMF_HIGH_ARENA` puts
# every allocation at an address a four-byte pointer cannot name, and with x64
# as the default that is now
#
#     ./build.sh -DBMF_HIGH_ARENA && ./test.sh
#
# on the ordinary gate, which reports 82 of 83 -- the out-of-memory leg skips
# itself there, because the arena is reserved up front and the ladder stops
# reaching the allocator it is about.  With a *low* heap, which is what x86-64
# Linux hands out for a program this size, truncating a pointer to 32 bits is
# the identity and nothing fails; above 4 GB it faults on the first dereference.
# It is not the default gate: `MAP_FIXED_NOREPLACE` is Linux-only, and a program
# that runs correctly at one address is not a property worth gating on.
set -u
# A job killed by a signal is reported by the shell that waits on it, and
# that shell is this one -- so the redirect has to be on the *loops*
# (`done 2>/dev/null`) and not on the command.  Every line this script
# means to say goes to stdout, so nothing of ours is lost with it.
set +m
cd "$(dirname "$0")/.."

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if ! BMF_OUT="$tmp/bmf32" BMF_BITS=32 ./build.sh >"$tmp/build" 2>&1; then
  echo "the 32-bit build failed:" >&2
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
  timeout 120 "$tmp/bmf32" c "$f" "$tmp/$n.bmf" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    printf '%-9s compress %s\n' "$n" "$(why $rc)"
    bad=$((bad + 1)); continue
  fi
  timeout 120 "$tmp/bmf32" d "$tmp/$n.bmf" "$tmp/$n.out" >/dev/null 2>&1
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
    printf '%-9s round trips, but the stream is not the reference (%s vs %s bytes)\n' \
      "$n" "$(stat -c%s "$tmp/$n.bmf")" "$(stat -c%s "$ref")"
    bad=$((bad + 1)); continue
  fi
  same=$((same + 1))
done 2>/dev/null

# The archive, because walking the members and `bmf_close_archive`'s rewrite of
# a header are paths no single image reaches.  Two members, concatenated: `bmf
# c` creates its output, and this leg used to build the archive by running it
# twice against one path -- which after that change was one member and a leg
# that passed without testing anything.  `test.sh`'s archive leg says the same.
arc=$tmp/arc.bmf
"$tmp/bmf32" c testfiles/t1.bmp "$tmp/arc1.bmf" >/dev/null 2>&1
"$tmp/bmf32" c testfiles/t8g.bmp "$tmp/arc2.bmf" >/dev/null 2>&1
cat "$tmp/arc1.bmf" "$tmp/arc2.bmf" > "$arc"
# Both members expand to the same output name, so what is left there is the
# last one -- which is what `test.sh` compares too.
"$tmp/bmf32" d "$arc" "$tmp/arc.bmp" >/dev/null 2>&1
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
    timeout 120 "$tmp/bmf32" d "$tmp/$m.bmf" "$tmp/$m.out" >/dev/null 2>&1
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

# And the out-of-memory path, which is `test.sh`'s last leg and the one place
# a *failed* allocation is the answer.  It matters more on this target than on
# the default build: the records are bigger there -- five pointers in
# `AltP2Block` and ten in `ModelBlock` -- so the ladder that made an allocation
# fail on one build is not automatically the ladder that makes it fail on the
# other.  That is the whole reason this leg is repeated here.
oombin=$tmp/bmf32
if ( ulimit -v 65536 ) >/dev/null 2>&1; then
  reported=0
  for kb in 6000 8000 10000 12000 16000; do
    rm -f "$tmp/oom.bmf"
    ( ulimit -v $kb; timeout 120 "$oombin" c testfiles/t1.bmp "$tmp/oom.bmf" ) \
        >"$tmp/oom.log" 2>&1
    rc=$?
    case $rc in
      7) grep -q 'Out of memory!' "$tmp/oom.log" || {
           echo "out of mem  -v $kb exits 7 without saying why"; bad=$((bad + 1)); }
         reported=1 ;;
      0|1) ;;
      *) printf 'out of mem  -v %s %s, not 7\n' "$kb" "$(why $rc)"; bad=$((bad + 1)) ;;
    esac
  done
  ran=$((ran + 1))
  if [ "$reported" = 1 ]; then
    same=$((same + 1))
  else
    echo "out of mem  no limit in the ladder made an allocation fail"
    bad=$((bad + 1))
  fi
fi

BMF_STRICT=1 BMF_BITS=64 ./build.sh >/dev/null 2>&1
# `bmf.cpp`, not `subs1.hpp`: the decompilation is in the one file now, and
# asking for the old name ended an otherwise passing run with a traceback.
left=$(python3 tools/ptrwidth.py bmf.cpp | tail -1)

echo
echo "$same of $ran cases agree with the default build"
echo "$left"
exit $((bad ? 1 : 0))
