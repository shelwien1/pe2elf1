#!/bin/sh
# Linux/gcc build. Pass extra -D flags to override rc_config.inc, e.g.
#   ./build.sh -DRCNUM=8 -DRC_LOWBYTES=6
set -e

CXX=${CXX:-g++}
OPTS="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-rtti -fno-exceptions -fstrict-aliasing"
INCS="-DNDEBUG -I. -ILib3"
DIRNAM=$(basename "$(pwd)")

# The vector coder is generated from rc.inc for the exact configuration on
# this command line: mk_kernel.sh resolves the #if forest with the C
# preprocessor, rc_soa.pl turns the lane state into [RCNUM] arrays, and the
# rc_macro.pl/defines.pl chain makes macros of it. cc and perl, nothing else.
KDEFS=""
for a in "$@"; do case "$a" in
  -DRC_LOWBYTES=*|-DRC_LOWSPLIT=*|-DRC_CODBYTES=*|-DRC_STATS=*) KDEFS="$KDEFS $a";;
  -DRC_RANGE64=1) NOVEC=1;;
esac; done
if [ -z "$NOVEC" ]; then
  CPP="$CXX -E" sh mk_kernel.sh $KDEFS
  CLDEF=""
else
  CLDEF="-DRC_VEC=0"   # RC_RANGE64 keeps the scalar coder
fi
CLLIB=""

$CXX -std=gnu++11 -O3 $INCS $OPTS $CLDEF "-D__DIRNAM__=$DIRNAM" "$@" \
     coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp \
     $CLLIB -o coder

echo "built ./coder${NOVEC:+ (scalar only)}"
