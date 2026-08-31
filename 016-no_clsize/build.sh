#!/bin/sh
#
# Linux build of the coder -- the gc.bat equivalent.
#
#   ./build.sh                       clang++, -march=native
#   ./build.sh -DRC_RCNUM=32         extra -D flags (see below)
#   ARCH=skylake-avx512 ./build.sh   pick the -march/-mtune target
#   TUNE=znver5 ./build.sh           schedule for a different core than -march
#   CXX=g++ ./build.sh               build with gcc instead (see below)
#   LTO=0 ./build.sh                 skip -flto
#   STATIC=1 ./build.sh              link -static
#   OUT=coder2 ./build.sh            name the binary
#
# Everything on the command line is passed to BOTH mk_kernel.sh and the
# compile, because the generated coder and the model have to be built from the
# same rc_config.inc: rc_vecD.inc is rc.inc preprocessed against the -D set, so
# a -DRC_LOWBYTES=4 that reached only one of the two would silently produce a
# coder whose lanes disagree about the accumulator width.
#
# clang is the default on purpose. The vector coder is not intrinsics: it is
# clang auto-vectorizing model0.inc's unrolled RCNUM-lane sweep over the local
# arrays rc_vecD.inc declares. gcc compiles the same source correctly -- the
# streams are byte-identical -- but does not vectorize that loop, so it lands
# at about half the encode speed.

set -e
cd "$(dirname "$0")"

CXX=${CXX:-clang++}
ARCH=${ARCH:-native}
# -mtune follows -march unless asked otherwise. Worth asking on AMD: tuning for
# the actual core is reported to buy about 10% encode on Zen, and clang only
# learned znver5 in 19 and znver6 in 21, so the value here outruns what any one
# clang accepts. It is scheduling only -- no ISA -- so a rejected value is a
# warning and the build goes on. On the Intel box this tree is measured on,
# znver tuning COSTS about 4% both ways (see the table below); it is a per-host
# choice, not a default.
TUNE=${TUNE:-$ARCH}
STD=${STD:-c++17}
OUT=${OUT:-coder}
OPT=${OPT:--O3}

if ! command -v "${CXX%% *}" >/dev/null 2>&1; then
  echo "build.sh: $CXX not found -- set CXX to a C++ compiler" >&2
  exit 1
fi
if ! command -v perl >/dev/null 2>&1; then
  echo "build.sh: perl not found -- mk_kernel.sh needs it" >&2
  exit 1
fi

# Does the compiler take this flag? Probed rather than assumed, so a target
# without -march=native (or an older clang without one of the -f options)
# degrades to a slower build instead of a failed one.
try_flag() {
  $CXX "$@" -x c++ -E - </dev/null >/dev/null 2>&1
}

case "$($CXX --version 2>/dev/null | head -1)" in
  *clang*) is_clang=1 ;;
  *) is_clang=0
     echo "build.sh: $CXX is not clang -- the lane sweep will not be vectorized" >&2 ;;
esac

# Measured here, best of 4 passes over enwik8, median of 4-5 rounds, on an
# Intel Cascade Lake Xeon (which is what -march=native resolves to):
#
#   clang 18, -march=native      native   znver2   znver3   znver4  generic
#                       enc       75.57    75.14    71.32    72.77    76.35
#                       dec       46.45    43.95    44.52    44.59    43.71
#
#   clang 20, -march=native      native   znver4   znver5
#                       enc       75.27    69.64    69.24
#                       dec       45.87    44.48    43.08
#
#   clang 20, -march=skylake-avx512      skylake-avx512   znver5
#                                 enc       75.46          70.09
#                                 dec       45.68          44.90
#
# So on THIS core znver tuning costs 6-8% encode, every round, and the two
# tunings that matter are indistinguishable anyway: clang 20 emits a
# byte-identical binary for znver4 and znver5. Reported to buy about 10% on a
# Skylake-X, which is a different core and possibly a different clang, so it
# is a per-host measurement and not a default. clang learned znver5 in 19 and
# znver6 in 21; below that the value is rejected and the build says so.
archflags=
if try_flag "-march=$ARCH"; then
  archflags="-march=$ARCH"
  if try_flag "-mtune=$TUNE"; then
    archflags="$archflags -mtune=$TUNE"
  else
    echo "build.sh: -mtune=$TUNE rejected by $CXX, tuning for $ARCH instead" >&2
    archflags="$archflags -mtune=$ARCH"
  fi
else
  echo "build.sh: -march=$ARCH rejected by $CXX, building without it" >&2
fi

# gc.bat's option set, minus the ones that only mean something in MSVC mode.
opts="-fomit-frame-pointer -fno-stack-protector -fstrict-aliasing"
for f in -fno-stack-clash-protection -fgnu-keywords; do
  try_flag $f && opts="$opts $f"
done

# The name that goes into log.txt's comment column, as in gc.bat.
DIRNAM=$(basename "$PWD")

#--- the generated vector coder ----------------------------------------------
# rc.inc -> rc_vecD.inc, against this build's -D set. Nothing else: the kernel
# is target-independent by construction, so whatever preprocessor runs it is
# free to disagree with the compile about the ISA (see model.inc). Not
# committed: it is derived, and derived differently for every -D set.
CPP="$CXX -E" ./mk_kernel.sh "$@"

# rc_iftree.inc -- the decoder's optional branch-tree byte (RC_DEC_IFTREE).
# Generated unconditionally: it depends on nothing but the tree shape, and
# generating it always keeps the -D set from deciding whether a file exists.
python3 ./mk_iftree.py rc_iftree.inc "${IFTDEPTH:-8}" >/dev/null

#--- compile -----------------------------------------------------------------
src="coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp"

flags="-std=$STD $OPT $archflags -DNDEBUG -I../Lib3 $opts -Wno-format"
flags="$flags -D__DIRNAM__=$DIRNAM"
[ "${STATIC:-0}" = 1 ] && flags="$flags -static"

# -flto+lld is what gc.bat uses. Only worth trying when both halves are there,
# and a failure falls back rather than stopping the build: LTO buys a percent
# or so here, the hot loop is inside one translation unit either way.
lto=
if [ "${LTO:-1}" = 1 ] && [ "$is_clang" = 1 ] && command -v ld.lld >/dev/null 2>&1; then
  lto="-flto -fuse-ld=lld"
fi

rm -f "$OUT"
# The LTO attempt's stderr is held rather than discarded: it is only noise when
# the attempt fails and we are about to retry, and it is the compiler's
# warnings when it succeeds.
err=$(mktemp)
trap 'rm -f "$err"' EXIT
# shellcheck disable=SC2086
if [ -n "$lto" ] && ! $CXX $flags $lto "$@" $src -o "$OUT" 2>"$err"; then
  echo "build.sh: -flto link failed, rebuilding without it" >&2
  lto=
elif [ -s "$err" ]; then
  cat "$err" >&2
fi
# shellcheck disable=SC2086
[ -f "$OUT" ] || $CXX $flags $lto "$@" $src -o "$OUT"

#--- the tag t.sh appends to log.txt -----------------------------------------
# log.txt's lines carry the ISA by hand ("avx512", "avx2"); ask the compiler
# which one this build actually got rather than reading it off ARCH. Named
# after the binary, so two differently-configured builds in one directory do
# not end up sharing one tag.
isa=x86
# shellcheck disable=SC2086
defs=$($CXX $archflags -dM -E -x c++ - </dev/null 2>/dev/null || true)
case "$defs" in *__AVX512F__*) isa=avx512 ;; *__AVX2__*) isa=avx2 ;; *__AVX__*) isa=avx ;; esac
case "$defs" in *__clang__*) fam=clang ;; *__GNUC__*) fam=gcc ;; *) fam=$(basename "${CXX%% *}") ;; esac
ver=$($CXX -dumpversion 2>/dev/null | cut -d. -f1)
cc="$fam${ver:+-$ver}"
printf '%s %s\n' "$cc" "$isa" > "$OUT.tag"

echo "built ./$OUT  [$cc, -march=$ARCH -> $isa${lto:+, lto}]"
