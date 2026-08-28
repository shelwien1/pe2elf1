#!/bin/sh
# Linux/gcc build. Pass extra -D flags to override rc_config.inc, e.g.
#   ./build.sh -DRCNUM=8 -DRC_LOWBYTES=6
set -e

CXX=${CXX:-g++}
OPTS="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-rtti -fno-exceptions -fstrict-aliasing"
INCS="-DNDEBUG -I. -ILib3"
DIRNAM=$(basename "$(pwd)")

# rc_kernel.ispc is generated from rc_kernel.c -- the kernel is kept as C so
# an editor can highlight it. The generated file is committed, so perl is only
# needed by whoever edits the kernel.
if [ rc_kernel.c -nt rc_kernel.ispc ]; then
  if command -v perl >/dev/null 2>&1; then
    ./mk_kernel.sh
  else
    echo "warning: rc_kernel.c is newer than rc_kernel.ispc, and perl is missing" >&2
  fi
fi

# Device path: the ispc-compiled coding kernel. Built whenever an ispc
# compiler is around (ISPC=path says where, ISPC=0 refuses it); without one
# the coder builds host-only and -C is simply always in effect.
#
# One explicit target, no dispatcher: ISPC_TARGET picks it, and the binary
# runs only on machines that have that ISA.
#   ISPC_TARGET=avx512skx-x16   AVX-512 (the default)
#   ISPC_TARGET=avx2-i32x16     AVX2
if [ -z "$ISPC" ]; then
  if command -v ispc >/dev/null 2>&1; then ISPC=ispc; else ISPC=0; fi
fi
ISPC_TARGET=${ISPC_TARGET:-avx512skx-x16}

if [ "$ISPC" != 0 ]; then
  # The kernel constants are compile-time on both sides, so the RC_*
  # overrides on our command line are mapped to the kernel's own names here,
  # with the rc_config.inc defaults for the rest.
  #
  # K_LS starts at 1, not the host's 0: the split low is stream-neutral and
  # the ispc kernel measures ~5% faster with it (the 64-bit accumulator costs
  # two zmm per varying). An explicit -DRC_LOWSPLIT still sets both sides.
  K_RCNUM=16 K_BLK=65536 K_LB=8 K_CB=4 K_LS=1 K_RT=0 K_R64=0 K_WO=-1
  for a in "$@"; do case "$a" in
    -DRC_RCNUM=*)       K_RCNUM=${a#*=};;
    -DRC_BLKSIZE=*)     K_BLK=${a#*=};;
    -DRC_LOWBYTES=*)    K_LB=${a#*=};;
    -DRC_CODBYTES=*)    K_CB=${a#*=};;
    -DRC_LOWSPLIT=*)    K_LS=${a#*=};;
    -DRC_RENORM_TAIL=*) K_RT=${a#*=};;
    -DRC_RANGE64=*)     K_R64=${a#*=};;
    -DRC_DEV_WORDOUT=*) K_WO=${a#*=};;
  esac; done
  # -1 = auto; under ispc the word path is the one that vectorises
  [ "$K_WO" = "-1" ] && K_WO=1

  rm -f rc_kernel_ispc*.o
  "$ISPC" --target=$ISPC_TARGET --arch=x86-64 -O2 --pic \
    -DRCNUM=$K_RCNUM -DSCALElog=15 -DhSCALE=16384 \
    -DLOWBYTES=$K_LB -DCODBYTES=$K_CB -DRC_LOWSPLIT=$K_LS -DBLKFULL=$K_BLK \
    -DRC_DEV_WORDOUT=$K_WO -DRC_RANGE64=$K_R64 -DRC_RENORM_TAIL=$K_RT \
    rc_kernel.ispc -o rc_kernel_ispc.o -h rc_kernel_ispc.h

  CLDEF="-DRC_ISPC=1 -DRC_ISPC_TARGET=$ISPC_TARGET"
  CLLIB="rc_kernel_ispc.o"
  BACKEND="ispc/$ISPC_TARGET"
else
  CLDEF="-DRC_ISPC=0"; CLLIB=""
  BACKEND=host-only
fi

$CXX -std=gnu++11 -O3 $INCS $OPTS $CLDEF "-D__DIRNAM__=$DIRNAM" "$@" \
     coder.cpp FSM.cpp rc_ispc.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp \
     $CLLIB -o coder

echo "built ./coder (backend=$BACKEND)"
