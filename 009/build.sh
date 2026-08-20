#!/bin/sh
# build.sh — build `bmf` from bmf.cpp, the way g.bat builds bmf.exe.
#
#     ./build.sh                        # ./bmf: x64, -O2, static, stripped
#     BMF_BITS=32 ./build.sh            # the same source at the other width
#     BMF_OUT=/tmp/bmf BMF_STATIC=0 BMF_GC=0 ./build.sh -fsanitize=address -g -O1
#     BMF_WARN=1   ./build.sh           # warn.log   and the count, no binary
#     BMF_ASSERTS=1 ./build.sh          # the BMF_ASSUME invariants, checked
#     BMF_STRICT=1 ./build.sh           # strict.log and the count, no binary
#     BMF_CONV=1   ./build.sh           # conv.log  and the count, no binary
#     ./build.sh -DBMF_HIGH_ARENA       # the arena leg; see below
#     BMF_GC=list  ./build.sh           # and name the bodies --gc-sections drops
#     BMF_ALIAS=-fstrict-aliasing ./build.sh   # tools/alias.sh's knob; see below
#
# Anything on the command line is appended to the compiler's, which is how
# tools/asan.sh, tools/fuzz.sh and tools/hdrscan.sh ask for their builds.
#
# `-DBMF_HIGH_ARENA` is the one worth naming here, because it is a *leg* and not
# a knob: `memory.inc` compiles a whole bump allocator behind
# `#if UINTPTR_MAX > 0xFFFFFFFFu && defined(BMF_HIGH_ARENA)`, and the default
# build never reaches a line of it.  `bmf_bucket_of`, `bmf_free`, the block
# header and the `#define malloc` pair all live inside it.  Stage 2 Phase 0 asks
# for the leg for exactly that reason: without it, edits to that code are
# unbuilt as well as untested.
#
#     ./build.sh -DBMF_HIGH_ARENA && ./test.sh
#
# `test.sh` answers 110 there rather than 111, and says which check it skipped:
# the out-of-memory leg cannot fire when the arena is reserved up front.
#
# ## what g.bat says, and where this departs from it
#
# g.bat is one command with the Windows parts folded into two variables:
#
#     g++ -march=k8 -mtune=k8 -s -std=gnu++1z -Ofast \
#         -DNDEBUG -DSTRICT -DWIN32 -I../Lib3 \
#         -fomit-frame-pointer -fstrict-aliasing -ffast-math -fno-stack-protector \
#         -fno-stack-check -fno-check-new -fno-rtti -fno-exceptions -fpermissive \
#         -static bmf.cpp -o bmf.exe
#
# `-DSTRICT`, `-DWIN32` and `-I../Lib3` are the Win32 headers' and are dropped;
# bmf.cpp includes nothing but the C++ standard library and the SSE intrinsics.
#
# Two of the rest change the compressed stream, and translating g.bat literally
# produces a binary that round-trips every image and agrees with none of the
# reference encodes.  Each was isolated by building the two variants and running
# `./test.sh` -- one flag apart, everything else held:
#
#     -fstrict-aliasing (g.bat's, and gcc's own default from -O2 up)
#                                     t24, t32 and x_ep differed -- WHEN THAT
#                                     WAS MEASURED.  Not any more: see below.
#     -ffast-math (which -Ofast implies), and specifically its
#     -funsafe-math-optimizations     t8g, t8p and x_ep differ.  Re-measured
#                                     on the current tree: still three, still
#                                     those three.
#
# So `-fno-strict-aliasing` is set explicitly and the optimiser is `-O2`.  This
# is not a guess about which is faithful: it is the flag set that reproduces
# testfiles/ref_*.bmf byte for byte, and it is also what g32.bat and the
# canonical command in tools/unused.py already use.  The float model behind
# t8g/t8p/x_ep is only reassociable if you do not mind the answer changing.
#
# **The aliasing half of that has expired.**  It was true of a tree that read
# objects through pointers of another type; `-Wstrict-aliasing=2` counted 22
# such sites and every one was real.  All 22 are gone -- the p1 symbol coder
# takes a `CounterNode*` instead of the `uint16_t*` eleven callers cast to,
# `mir_top` is a `P2Count*` and a `P2Freq*` instead of one word pointer doing
# both jobs, `(int32_t*)&grid[i]` was cast straight back to `FreqRec*` on its
# next use, the tree nodes in `reduce_alphabet` are the word pair they are
# read as, and the one genuine reinterpretation left goes through `memcpy`.
#
# The flag stays: it is part of the recipe the reference streams were taken
# under, and so is `-O2`.  What changed is that the program no longer *depends*
# on it.  `tools/alias.sh` is the check that keeps that true, and it gates on
# the warning count rather than on the streams -- putting nine of the casts
# back left all seventeen streams matching, because gcc is entitled to exploit
# them and at `-O2` on this code simply does not.  `BMF_ALIAS` is its knob.
#
# `-O3` is *not* one of the causes, which is worth saying because it looks like
# one: `-O3 -fno-strict-aliasing` matches all fifteen.  It is left at `-O2`
# anyway, because that is the level the reference streams were taken at and the
# level every tool in tools/ quotes; the two flags above are the finding.
#
# The third cause is not in g.bat at all -- it is the host.  `-march=native` on
# anything with FMA contracts a multiply and an add in the p2 model and x_ep
# differs.  `-march=k8` is what g.bat pins and what is kept here, and
# `-ffp-contract=off` is added so that a build re-pointed at another `-march`
# cannot move a stream quietly.
#
# ## the target
#
# x64, which is what g.bat builds -- `-march=k8` with no `-m32` is the AMD64
# baseline, and g32.bat is the separate 32-bit script.  Both widths produce the
# fifteen reference streams byte for byte and pass all 83 checks; `BMF_BITS=32`
# is the other one, and tools/x64diff.sh is what asks whether they still agree
# on inputs nobody chose.
#
# On 32-bit, `-mfpmath=sse` is the same concern one register file over: the
# 387's excess precision is not what the model was tuned on, and it is not the
# default on i386 the way SSE is on x64.  g32.bat sets it and so does this.
#
# g++ 13 and clang++ 18 both produce the fifteen reference streams here.
set -eu
cd "$(dirname "$0")"

CXX=${BMF_CXX:-g++}
BITS=${BMF_BITS:-64}
OUT=${BMF_OUT:-./bmf}
SRC=bmf.cpp

case $BITS in
  32) arch='-m32 -march=k8 -mtune=k8 -msse2 -mfpmath=sse' ;;
  64) arch='-m64 -march=k8 -mtune=k8' ;;
  *)  echo "BMF_BITS is 32 or 64, not $BITS" >&2; exit 2 ;;
esac

# The flags g.bat sets that do not change the stream, kept as it has them.
opts='-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-check-new
      -fno-rtti -fno-exceptions'
# Held apart from the rest because BMF_STRICT is defined as the build *without*
# it, and a strict pass that still passes -fpermissive reports zero for the one
# reason that means nothing.  It did, until this line.
permissive='-fpermissive'
# And the three that decide the answer.  See the header.
fidelity="${BMF_ALIAS:--fno-strict-aliasing} -ffp-contract=off"
incs='-DNDEBUG -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0'
# The invariants the source states as `BMF_ASSUME`, checked rather than
# asserted.  Off in the shipping build -- `-DNDEBUG` above already disarms
# `assert`, and these sit in the hottest loops in the program -- and on for a
# leg the gate can run:
#
#     BMF_ASSERTS=1 ./build.sh && ./test.sh
#
# Stage 2 §1.7 asks for `p2_pred`'s `rate in [5,8]` to be "pinned with a debug
# assert rather than re-derived later"; a pin nothing can pull is a comment, so
# this is the build that pulls it.
if [ "${BMF_ASSERTS:-0}" = 1 ]; then
  incs="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -DBMF_ASSERTS"
fi
std='-std=gnu++17'
opt='-O2'

link=''
[ "${BMF_STATIC:-1}" = 0 ] || link="$link -static"
[ "${BMF_STRIP:-1}" = 0 ] || link="$link -s"
# --gc-sections drops the bodies nothing reaches; `BMF_GC=list` names them,
# which is where tools/deadcheck.py's fourth kind is visible from the link.
# ASan needs it off: section GC hides the frames its reports are read from.
case ${BMF_GC:-1} in
  0)    ;;
  list) opts="$opts -ffunction-sections -fdata-sections"
        link="$link -Wl,--gc-sections -Wl,--print-gc-sections" ;;
  *)    opts="$opts -ffunction-sections -fdata-sections"
        link="$link -Wl,--gc-sections" ;;
esac

# `BMF_BITS=32` needs the multilib runtime, which a host that only ever builds
# the default target has no reason to have.  Without it the failure is forty
# lines of `cannot find crti.o` that say nothing about the fix, so ask first and
# name the package.  No silent fall back to the default width: a build that
# quietly became another target is the one thing this script must not do.
#
# The probe links rather than preprocesses.  What is missing on a host without
# multilib is `libc6-dev-i386` -- the startup objects and the 32-bit libraries
# -- and a preprocessor run over an empty file includes nothing, so it succeeds
# exactly where the check is needed and the caller gets the forty lines anyway.
probe32() { echo 'int main(){return 0;}' | $CXX -m32 -x c++ - -o /dev/null; }
if [ "$BITS" = 32 ] && ! probe32 >/dev/null 2>&1; then
  echo "$CXX cannot target -m32 here: install g++-multilib" >&2
  echo "  (the default target is x64 and needs none of it: ./build.sh)" >&2
  exit 2
fi

# The two diagnostic passes.  Both are `-fsyntax-only`, so neither writes a
# binary, and both stamp their log with `cksum` of the source they read: a log's
# line numbers mean nothing against another version of the source and an mtime
# cannot tell you which version that was.  tools/buildlog.py is what checks it.
#
# Every file, not just `$SRC`.  A stamp over bmf.cpp alone said nothing once
# bmf.cpp became an include list -- an edit to `model.inc` leaves it byte for
# byte identical, so the check passed for a log describing the source as it was
# before the edit, which is worse than no check.  `tools/buildlog.py:digest`
# computes this same concatenation and the two have to agree.
stamp() {
  printf '# sources %s\n' \
      "$(cat "$SRC" $(ls *.inc | LC_ALL=C sort) | cksum)" >> "$1"
}

if [ "${BMF_WARN:-0}" = 1 ]; then
  # -fpermissive stays: this pass counts the comparisons whose conversion is
  # invisible, not the conversions themselves.
  #
  # `-Wshadow` is here because the tree has no `this->` left to disambiguate a
  # member from a local of the same name.  A shadow reintroduced later would be
  # silent -- `sym_cache = sym_cache;` compiles -- so the count is the guard.
  # Note for anything that parses this log: gcc quotes the name with the
  # locale's quotation marks, curly ones here, and `-fdiagnostics-plain-output`
  # does not change that.  See tools/unused.py, which accepts both.
  #
  # tools/explicitcmp.py and tools/unused.py read what this writes;
  # tools/resign-drive.sh reads the count off stdout and ratchets on it, so the
  # count is the first line.
  log=warn.log
  set +e
  $CXX $arch $std $opt $incs $opts $permissive $fidelity -fdiagnostics-plain-output \
      -Wsign-compare -Wunused-variable -Wshadow -fsyntax-only "$@" "$SRC" > "$log" 2>&1
  rc=$?
  set -e
  # An **error** is not a warning, and this printed the warning count either
  # way: a build that failed to compile answered `0` and read as clean.  One
  # did, for the length of an edit -- `'val3' was not declared in this scope`
  # under a `0` -- and the only reason it did not survive longer is that
  # somebody opened the log.  The count is the first line because
  # `resign-drive.sh` ratchets on it, so the failure goes above it and the
  # status is what carries it.
  if [ "$rc" != 0 ]; then
    echo "FAILED: $CXX exited $rc; see $log" >&2
    grep -m3 ': error: ' "$log" >&2 || true
  fi
  grep -c ': warning: ' "$log" || true
  stamp "$log"
  exit "$rc"
fi

# Every implicit narrowing the program performs, counted.
#
# This is the *cast audit*'s instrument and not a cleanliness pass.  A narrowing
# in this program is usually the point, and the classes are worth writing down
# once so the next audit does not re-derive them:
#
#   wraparound before a table lookup   `(uint8_t)(sym - pred)` indexing `fold`
#                                      and `fold_hi`, ~40 sites.  The wrap *is*
#                                      the fold: the table has 256 entries and
#                                      the residual is taken modulo 256.
#   the model's truncating update      `(int16_t)` into `P2Count::weighted` and
#                                      `(uint16_t)` into `P2Freq`'s counts.
#                                      Sixteen bits in, sixteen out, and the
#                                      high half the store dropped was never
#                                      read.
#   byte extraction                    `(uint8_t)(low >> 23)` in the range
#                                      coder, and the four in `flush`.
#   context truncation                 `(uint16_t)` feeding `CtxIdx::bit_of`,
#                                      where the cast is what puts the bit at
#                                      15 rather than at 31.
#   float widening                     `(float)` through the NLMS filter.
#
# None of those is going anywhere.  Marking each with an inline `/*wrap*/` was
# considered and declined: forty markers is forty more things to keep true, and
# the count below is one thing that cannot drift.
#
# What the count is for is the other direction.  When a cast is *removed* as
# redundant, the question is whether it was doing something; a cast that was
# doing something leaves a warning behind when it goes.  So the procedure is
# "note the count, remove the cast, compare" -- and having the count in a build
# leg is what makes the comparison a command rather than a remembered number.
#
# It is a ratchet on the same argument as the others here: a *new* narrowing
# that nobody decided on shows up as a count that went up.
if [ "${BMF_CONV:-0}" = 1 ]; then
  log=conv.log
  [ "$BITS" = 32 ] || log=conv64.log
  set +e
  $CXX $arch $std $opt $incs $opts $permissive $fidelity -fdiagnostics-plain-output \
      -Wall -Wextra -Wconversion -fsyntax-only "$@" "$SRC" > "$log" 2>&1
  rc=$?
  set -e
  if [ "$rc" != 0 ]; then
    echo "FAILED: $CXX exited $rc; see $log" >&2
    grep -m3 ': error: ' "$log" >&2 || true
  fi
  grep -c ': warning: ' "$log" || true
  stamp "$log"
  exit 0
fi

if [ "${BMF_STRICT:-0}" = 1 ]; then
  # Without -fpermissive, every conversion the build was letting through is an
  # error, and the list of them is the worklist tools/retype_locals.py and
  # tools/ptrwidth.py work from.  At 64 bits it is a different list -- a cast
  # through int32_t is not a narrowing when a pointer is four bytes wide -- so
  # it gets a different name.
  log=strict.log
  [ "$BITS" = 32 ] || log=strict64.log
  set +e
  $CXX $arch $std $opt $incs $opts $fidelity -fdiagnostics-plain-output \
      -fsyntax-only "$@" "$SRC" > "$log" 2>&1
  set -e
  grep -c ': error: ' "$log" || true
  stamp "$log"
  exit 0
fi

rm -f "$OUT"
# `"$@"` after the defaults so a caller's -O1 or -fno-omit-frame-pointer wins.
$CXX $arch $std $opt $incs $opts $permissive $fidelity $link "$@" "$SRC" -o "$OUT"
