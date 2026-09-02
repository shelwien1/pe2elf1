#!/bin/sh
# Linux/POSIX build of the ppmd+LSTM coder0 (the Windows build is gc.bat).
#
# One translation unit: coder0.cpp includes the LSTM (lstm*.inc) and the
# weights codec it shares with the transformer version (../tf/weights_*.inc)
# directly, and every system header lives at the top of coder0.cpp.
set -e
CXX=${CXX:-clang++}
ARCH=${ARCH:--march=haswell -mtune=haswell}
OUT=${OUT:-coder0}

# Model switches (documented at the top of coder0.cpp):
#   LSTMDEFS="-DLSTM_TRAIN=0"           freeze the weights, no online training
#   LSTMDEFS="-DLSTM_SAVE_OPTIMIZER=0"  save the weights without the AdamW state
#   LSTMDEFS="-DLSTM_SAVE_Q4=1"         quantize the saved weights to int4
LSTMDEFS=${LSTMDEFS:-}

$CXX -O3 -std=c++17 -fno-math-errno -ffp-contract=off -fomit-frame-pointer \
     -fno-stack-protector $ARCH -Wno-format $LSTMDEFS coder0.cpp -o "$OUT" -lm
echo "done: $OUT"
