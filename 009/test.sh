#!/bin/sh
# test.sh — the gate: does this build still encode exactly what it used to?
#
#     ./test.sh                      # ./bmf, every image, five minutes a run
#     ./test.sh /tmp/bmf64
#     BMF_IMAGES='t24 t8g' ./test.sh
#     BMF_TIMEOUT=25 ./test.sh       # what tools/struct-sweep.sh runs it with
#
# The corpus is testfiles/: seventeen .bmp inputs and, for each, the stream this
# program is expected to produce for it -- testfiles/ref_<name>.bmf.  A
# reference is not a claim about what BMF.exe emits; there is no wine here to
# ask the donor.  It is what the build produced when the reference was taken,
# which is what a refactoring gate needs: a change that was not meant to move
# the compressed output and moved it has been caught, and one that was meant to
# says so in a commit message and re-takes the references with tools/mkrefs.sh.
#
# `xform1` and `xform2` are the two exceptions, and they are a stronger claim:
# their references were taken from a build of the *original decompilation* --
# the first commit of the tree, before any refactoring round -- because they
# exist to pin a defect a refactoring round introduced, and a reference taken
# from the build under test could not have caught it.  See the header of
# `plane_choose.inc`.  Every other image picks colour transform 0; these
# two are the only ones in the corpus that pick 1 and 2.
#
# Seven legs, because "the streams match" is a much narrower statement than it
# sounds and six kinds of defect live outside it:
#
#   streams       compress each image, compare against its reference.
#   round trip    expand it again, compare against the input.  A stream can
#                 match while the decoder that has to read it does not.
#   reference     expand the *reference* stream instead of the one this build
#                 just wrote, and compare against the input.  The round trip
#                 above feeds the decoder whatever the encoder produced, so a
#                 change that moves both halves the same way passes it; this
#                 leg's input is frozen, so it answers about the decoder alone.
#                 It is the "orig-c -> new-d" direction, and the other one --
#                 new-c fed to the old decoder -- is the streams leg: bytes
#                 equal to the reference are bytes the old decoder already read.
#   output        `bmf c` writes its output rather than growing it.
#   malformed     five broken headers derived from the references.  The answer
#                 here is an exit code, not a stream: 1..8 is the program's own
#                 table (see __exit_402E40), and anything else is a crash.
#   truncated     every reference cut short.  Same question, asked of the one
#                 malformation that needs no knowledge of the format.
#   out of memory the one leg where a *failed* allocation is the right answer.
#                 Under a small enough `ulimit -v`, bmf_new returns null and the
#                 program is expected to say "Out of memory!" and exit 7 rather
#                 than dereference it.
#
# The malformed and truncated inputs are built here from whatever the corpus
# currently is, rather than kept in testfiles/, so that re-taking the references
# re-takes them too.
#
# Two things this leaves to other scripts, both on purpose.  It does not build:
# tools/frame-sweep.sh and tools/struct-sweep.sh run ./build.sh themselves and
# need to tell a build failure from a gate failure.  And it says nothing about
# memory that is read or written out of bounds without changing the answer --
# an exit code cannot see that class.  tools/asan.sh, tools/fuzz.sh and
# tools/hdrscan.sh are what ask it.
#
# The last line of stdout begins PASS or FAILED, and the sweeps read exactly
# that.  A moved stream is reported with the word CHANGED, which is what they
# grep out of the log to say why a candidate was reverted.
set -u
cd "$(dirname "$0")"

BIN=${1:-./bmf}
T=${BMF_TIMEOUT:-300}
[ -x "$BIN" ] || { echo "no $BIN (run ./build.sh)"; echo "FAILED: nothing to test"; exit 2; }

# The binary has to be newer than the source, or this is a gate on the previous
# build.  It has happened three times: twice by rebuilding in one shell while
# this ran in another, and once because `./build.sh 2>&1 | tail -4` reports the
# pipe's exit status and not the compiler's, so a build that failed with an
# undeclared name read as a build that finished with zero warnings -- and the
# 110 checks that followed passed, against the binary from before the edit.
# The message says which file, because "stale" without a name is a hunt.
newer=$(find . -maxdepth 1 \( -name '*.inc' -o -name '*.cpp' \) -newer "$BIN" \
        -printf '%f ' 2>/dev/null)
[ -z "$newer" ] || {
  echo "$BIN is older than: $newer"
  echo "FAILED: the binary does not include the source (run ./build.sh)"
  exit 2
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

ran=0 bad=0
note() { bad=$((bad + 1)); printf '%-9s %s\n' "$1" "$2"; }

# A signal is not an exit code, and "exits 139" is a worse answer than the name
# of the signal: SIGSEGV and SIGFPE are different defects and these legs see
# both.  Anything under 128 is the program's own table.
why() {
  case $1 in
    124) echo "timed out" ;;
    13[4-9]|1[4-9][0-9]) echo "died on SIG$(kill -l $(($1 - 128)) 2>/dev/null || echo "?$1")" ;;
    *)   echo "exits $1" ;;
  esac
}

# `out_<name>.bmp` is not an input.  An RLE-compressed BMP does not come back
# byte for byte -- bmf decodes the runs on the way in and re-encodes them with
# its own run splitting, so the pixels are identical and the file is not -- and
# out_rle4.bmp / out_rle8.bmp are what the decoder is expected to write instead.
# Globbing testfiles/*.bmp picks them up and then looks for a reference stream
# that cannot exist.
images=''
for f in testfiles/*.bmp; do
  n=$(basename "$f" .bmp)
  case $n in out_*) continue ;; esac
  images="$images $n"
done
# BMF_IMAGES restricts the first two legs, and takes either spelling:
# tools/struct-sweep.sh passes `t24.bmp altp1.bmp ...`, a person passes `t24`.
if [ -n "${BMF_IMAGES:-}" ]; then
  want=$(echo "$BMF_IMAGES" | tr ',' ' ' | sed 's/\.bmp//g')
  keep=''
  for n in $images; do
    for w in $want; do [ "$n" = "$w" ] && keep="$keep $n" && break; done
  done
  for w in $want; do
    case " $keep " in *" $w "*) ;; *) note "$w" "named in BMF_IMAGES, not in testfiles/" ;; esac
  done
  images=$keep
fi

# ---- streams, and the round trip ------------------------------------------
# `bmf c` creates its output, so a stale file from an earlier run cannot leak
# into a stream.  It is still removed first: the leg below distinguishes "did
# not write" from "wrote the same bytes", and a file left over from the last
# run makes those two look alike.
streams=0 trips=0 nimg=0
for n in $images; do
  nimg=$((nimg + 1))
  ref=testfiles/ref_$n.bmf
  rm -f "$tmp/$n.bmf" "$tmp/$n.bmp"
  ran=$((ran + 2))
  timeout "$T" "$BIN" c "testfiles/$n.bmp" "$tmp/$n.bmf" >/dev/null 2>&1
  rc=$?
  if [ $rc != 0 ]; then
    note "$n" "compress $(why $rc)"; ran=$((ran - 1)); continue
  fi
  if [ ! -f "$ref" ]; then
    note "$n" "no reference stream ($ref) -- run tools/mkrefs.sh"
  elif ! cmp -s "$tmp/$n.bmf" "$ref"; then
    note "$n" "stream CHANGED ($(stat -c%s "$tmp/$n.bmf") vs $(stat -c%s "$ref") bytes)"
  else
    streams=$((streams + 1))
  fi
  timeout "$T" "$BIN" d "$tmp/$n.bmf" "$tmp/$n.bmp" >/dev/null 2>&1
  rc=$?
  if [ $rc != 0 ]; then
    note "$n" "expand $(why $rc)"; continue
  fi
  want=testfiles/$n.bmp
  [ -f "testfiles/out_$n.bmp" ] && want=testfiles/out_$n.bmp
  if cmp -s "$tmp/$n.bmp" "$want"; then
    trips=$((trips + 1))
  else
    note "$n" "round trip differs from $(basename "$want")"
  fi
done
echo "$streams/$nimg streams match their reference"
echo "$trips/$nimg round trips are byte-identical"

# ---- the reference streams, decoded -----------------------------------------
# Same comparison as the round trip, with the encoder taken out of it.  A
# decoder change is localized here rather than reported as "differs" by a leg
# that used the encoder to build its own input.
refs=0 nref=0
for n in $images; do
  ref=testfiles/ref_$n.bmf
  [ -f "$ref" ] || continue
  nref=$((nref + 1)); ran=$((ran + 1))
  rm -f "$tmp/ref_$n.bmp"
  timeout "$T" "$BIN" d "$ref" "$tmp/ref_$n.bmp" >/dev/null 2>&1
  rc=$?
  if [ $rc != 0 ]; then
    note "$n" "expanding the reference $(why $rc)"; continue
  fi
  want=testfiles/$n.bmp
  [ -f "testfiles/out_$n.bmp" ] && want=testfiles/out_$n.bmp
  if cmp -s "$tmp/ref_$n.bmp" "$want"; then
    refs=$((refs + 1))
  else
    note "$n" "reference stream expands to something other than $(basename "$want")"
  fi
done
echo "$refs/$nref reference streams expand to their image"

# ---- the output file ------------------------------------------------------
# `bmf c` writes its output: the same command twice is the same file, not a
# bigger one.  This is all that is left of an archive leg that compressed two
# images into one file and expanded the last -- there are no archives now, and
# `arc.inc` opens `wb`.
ran=$((ran + 1))
rm -f "$tmp/twice.bmf"
timeout "$T" "$BIN" c testfiles/t1.bmp "$tmp/twice.bmf" >/dev/null 2>&1
once=$(stat -c%s "$tmp/twice.bmf" 2>/dev/null || echo 0)
timeout "$T" "$BIN" c testfiles/t1.bmp "$tmp/twice.bmf" >/dev/null 2>&1
again=$(stat -c%s "$tmp/twice.bmf" 2>/dev/null || echo 0)
if [ "$once" != 0 ] && [ "$once" = "$again" ]; then
  echo "output: compressing twice to one path writes one image ($once bytes)"
else
  note output "compressing twice changed the output: $once then $again bytes"
fi

# ---- malformed headers ----------------------------------------------------
# The same five tools/x64.sh keeps, and for the same reason: each is one field
# of a stream that is otherwise exactly a reference.
#
#   shortlen  data_len halved -- the range decoder runs off the coded buffer
#   len1      the same field taken to 1, which is a four-byte packer read out
#             of a one-byte buffer
#   depthff   depth = 0xFF; six of its bits are the depth, so it says 63, and
#             (63 + 7) >> 3 is eight planes written into five records
#   narrow    a header claiming width 1, where the row-end mirror in
#             alt_p2_d8_decode_body reads a record that a plane narrower than
#             three does not have
#   rawlen    a raw member whose declared length is 100000 bytes
mal=0 nmal=0
if [ -f testfiles/ref_t24.bmf ] && [ -f testfiles/ref_t8g.bmf ]; then
  python3 -c 'import struct, sys
t = sys.argv[1]
r = open("testfiles/ref_t24.bmf", "rb").read()
d = bytearray(r); struct.pack_into("<I", d, 16, struct.unpack_from("<I", d, 16)[0] // 2)
open(t + "/shortlen.bmf", "wb").write(d)
d = bytearray(r); struct.pack_into("<I", d, 16, 1)
open(t + "/len1.bmf", "wb").write(d)
d = bytearray(r); d[14] = 0xFF
open(t + "/depthff.bmf", "wb").write(d)
d = bytearray(open("testfiles/ref_t8g.bmf", "rb").read()); struct.pack_into("<H", d, 4, 1)
open(t + "/narrow.bmf", "wb").write(d)
open(t + "/rawlen.bmf", "wb").write(
    struct.pack("<4sHHHHHBBI", b"\x81\x8a20", 8, 8, 0, 0, 0, 8, 0x04, 100000) + b"\xaa" * 100000)' "$tmp"
  for m in shortlen len1 depthff narrow rawlen; do
    nmal=$((nmal + 1)); ran=$((ran + 1))
    rm -f "$tmp/$m.out"
    ( ulimit -f 262144; timeout "$T" "$BIN" d "$tmp/$m.bmf" "$tmp/$m.out" ) >/dev/null 2>&1
    rc=$?
    if [ "$rc" -ge 1 ] && [ "$rc" -le 8 ]; then
      mal=$((mal + 1))
    else
      note "$m" "malformed input $(why $rc), not a code from the table"
    fi
  done
  echo "$mal/$nmal malformed streams refused with a code from the table"
else
  note malformed "testfiles/ref_t24.bmf and ref_t8g.bmf are what these are made from"
fi

# ---- truncations ----------------------------------------------------------
# Every reference, cut to a half, a quarter and a sixteenth of itself.  A cut
# stream may parse its header and run out part way through, or not parse at
# all; both are fine and 0 is fine -- an empty archive is zero members and zero
# members is success.  What is not fine is a signal.
cut=0 ncut=0
for r in testfiles/ref_*.bmf; do
  [ -f "$r" ] || continue
  n=$(basename "$r" .bmf)
  sz=$(stat -c%s "$r")
  for frac in 2 4 16; do
    ncut=$((ncut + 1)); ran=$((ran + 1))
    head -c $((sz / frac)) "$r" > "$tmp/cut.bmf"
    rm -f "$tmp/cut.out"
    ( ulimit -f 262144; timeout "$T" "$BIN" d "$tmp/cut.bmf" "$tmp/cut.out" ) >/dev/null 2>&1
    rc=$?
    if [ "$rc" -le 8 ]; then
      cut=$((cut + 1))
    else
      note "${n#ref_}" "truncated to 1/$frac $(why $rc)"
    fi
  done
done
echo "$cut/$ncut truncated streams refused without a crash"

# ---- out of memory --------------------------------------------------------
# A ladder rather than one limit: which rung makes an allocation fail depends on
# the pointer width and on what the libc reserves before main, and a single
# number that happens to work on one target is a leg that quietly stops testing
# anything on the other.  Any rung that reports is enough; a rung that succeeds
# is not a failure.  What fails is every rung succeeding -- nothing was tested --
# or a rung dying instead of reporting.
ran=$((ran + 1))
if ( ulimit -v 65536 ) >/dev/null 2>&1; then
  said='' arena=''
  for kb in 4000 6000 8000 10000 12000 16000; do
    rm -f "$tmp/oom.bmf"
    ( ulimit -v $kb; timeout "$T" "$BIN" c testfiles/t1.bmp "$tmp/oom.bmf" ) \
        >"$tmp/oom.log" 2>&1
    rc=$?
    # A -DBMF_HIGH_ARENA build reserves 768 MB up front, so every rung fails
    # inside mmap before the program has allocated anything -- and what this
    # leg is about is bmf_new returning null.  It says so on the way out, which
    # is the only way to tell that binary from a stripped ordinary one; a run
    # that never reaches the allocator has not answered this question, so the
    # leg is skipped with the reason rather than failed.
    if grep -q 'no high arena' "$tmp/oom.log"; then arena=1; break; fi
    case $rc in
      7) if grep -q 'Out of memory!' "$tmp/oom.log"; then
           said=$kb
         else
           note "out of mem" "-v $kb exits 7 without saying why"
         fi ;;
      0|1) ;;
      *) note "out of mem" "-v $kb $(why $rc), not 7" ;;
    esac
  done
  if [ -n "$arena" ]; then
    echo "out of memory: skipped, this build reserves a high arena up front"
    ran=$((ran - 1))
  elif [ -n "$said" ]; then
    echo "out of memory: -v $said exits 7, \"Out of memory!\""
  else
    note "out of mem" "no limit in the ladder made an allocation fail"
  fi
else
  echo "out of memory: skipped, this shell cannot set ulimit -v"
  ran=$((ran - 1))
fi

# ---- the command line -----------------------------------------------------
# Cheap, and it is the one path every other leg avoids: three arguments are
# what the five above always pass.
ran=$((ran + 1))
"$BIN" >"$tmp/usage" 2>&1
rc=$?
if [ "$rc" = 1 ] && grep -q '^Usage: bmf c input.bmp output' "$tmp/usage"; then
  echo "usage: no arguments exits 1 and says how"
else
  note usage "no arguments $(why $rc)"
fi

echo
# `ran` is counted as the legs run rather than computed from the corpus size:
# a line that recomputes what it claims to have counted goes on being confident
# while cases are added under it.
if [ "$bad" = 0 ]; then
  echo "PASS: $ran checks, $nimg images"
else
  echo "FAILED: $bad of $ran checks"
fi
exit $((bad ? 1 : 0))
