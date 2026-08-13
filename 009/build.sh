#!/bin/sh
# build.sh — build `bmf` from bmf.cpp, the way g.bat builds bmf.exe.
#
#     ./build.sh                        # ./bmf: 32-bit, -O2, static, stripped
#     BMF_BITS=64 ./build.sh            # the same source at the other width
#     BMF_OUT=/tmp/bmf BMF_STATIC=0 BMF_GC=0 ./build.sh -fsanitize=address -g -O1
#     BMF_WARN=1   ./build.sh           # warn.log   and the count, no binary
#     BMF_STRICT=1 ./build.sh           # strict.log and the count, no binary
#     BMF_GC=list  ./build.sh           # and name the bodies --gc-sections drops
#
# Anything on the command line is appended to the compiler's, which is how
# tools/asan.sh, tools/fuzz.sh and tools/hdrscan.sh ask for their builds.
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
#                                     t24, t32 and x_ep differ
#     -ffast-math (which -Ofast implies), and specifically its
#     -funsafe-math-optimizations     t8g, t8p and x_ep differ
#
# So `-fno-strict-aliasing` is set explicitly and the optimiser is `-O2`.  This
# is not a guess about which is faithful: it is the flag set that reproduces
# testfiles/ref_*.bmf byte for byte, and it is also what g32.bat and the
# canonical command in tools/unused.py already use.  The decompilation reads
# objects through pointers of another type on purpose -- it is what MSVC emitted
# for 1997 x86 -- and the float model behind t8g/t8p/x_ep is only reassociable
# if you do not mind the answer changing.
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
# On 32-bit, `-mfpmath=sse` is the same concern one register file over: the
# 387's excess precision is not what the model was tuned on.  g32.bat sets it
# and so does this.
#
# g++ 13 and clang++ 18 both produce the fifteen reference streams here.
set -eu
cd "$(dirname "$0")"

CXX=${BMF_CXX:-g++}
BITS=${BMF_BITS:-32}
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
fidelity='-fno-strict-aliasing -ffp-contract=off'
incs='-DNDEBUG -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0'
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

# A 32-bit build needs the multilib runtime, and the failure without it is
# forty lines of `cannot find crti.o` that say nothing about the fix.  Ask
# first, and name the package.  No silent fall back to -m64: the reference
# streams are one target's and a build that quietly became another target is
# the one thing this script must not do.
#
# The probe links rather than preprocesses.  What is missing on a host without
# multilib is `libc6-dev-i386` -- the startup objects and the 32-bit libraries
# -- and a preprocessor run over an empty file includes nothing, so it succeeds
# exactly where the check is needed and the caller gets the forty lines anyway.
probe32() { echo 'int main(){return 0;}' | $CXX -m32 -x c++ - -o /dev/null; }
if [ "$BITS" = 32 ] && ! probe32 >/dev/null 2>&1; then
  echo "$CXX cannot target -m32 here: install g++-multilib" >&2
  echo "  (or BMF_BITS=64 ./build.sh -- the same fifteen streams, four more" >&2
  echo "   bytes a pointer; tools/x64.sh is what checks that claim)" >&2
  exit 2
fi

# The two diagnostic passes.  Both are `-fsyntax-only`, so neither writes a
# binary, and both stamp their log with `cksum` of the source they read: a log's
# line numbers mean nothing against another version of the file and an mtime
# cannot tell you which version that was.  tools/buildlog.py is what checks the
# stamp, and `# subs1.hpp ` is the prefix it looks for -- from when the
# decompilation was a header included by bmf.cpp, and left alone here because
# the tools that read these logs still expect it.
stamp() { printf '# subs1.hpp %s\n' "$(cksum < "$SRC")" >> "$1"; }

if [ "${BMF_WARN:-0}" = 1 ]; then
  # -fpermissive stays: this pass counts the comparisons whose conversion is
  # invisible, not the conversions themselves.  tools/explicitcmp.py and
  # tools/unused.py read what it writes; tools/resign-drive.sh reads the count
  # off stdout and ratchets on it, so the count is the first line.
  log=warn.log
  set +e
  $CXX $arch $std $opt $incs $opts $permissive $fidelity -fdiagnostics-plain-output \
      -Wsign-compare -Wunused-variable -fsyntax-only "$@" "$SRC" > "$log" 2>&1
  set -e
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
