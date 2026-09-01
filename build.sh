#!/bin/sh
# Linux/POSIX build of coder0 (the Windows build is gc.bat).
#
# The transformer sources are compiled as their own translation units, WITHOUT
# -ffast-math and without LTO: they need IEEE semantics (the range decoder that
# unpacks the weights recomputes the RoPE tables with a bit-exact port of CUDA's
# sinf/cosf, and the model's quantization steps depend on exact round-to-nearest
# division), while coder0.cpp itself keeps the -Ofast build it has always had.
# -ffp-contract=off costs clang nothing (its default already contracts nothing
# here) and makes a g++ build produce a bit-identical model.
set -e
CXX=${CXX:-clang++}
ARCH=${ARCH:--march=haswell -mtune=haswell}
OUT=${OUT:-coder0}

TFFLAGS="-O3 -std=c++17 -fno-math-errno -ffp-contract=off $ARCH -Wall -Wextra -Wno-unused-parameter"
# Model switches (see the header of transformer.inc):
#   TFDEFS="-DTF_LOAD_WEIGHTS=0"   initialize the weights instead of loading
#   TFDEFS="-DTF_TRAIN=1"          train the output layer online
TFDEFS=${TFDEFS:-}
C0FLAGS="-Ofast -std=c++17 -fomit-frame-pointer -fno-stack-protector $ARCH -Wno-format $TFDEFS"

mkdir -p obj
for f in weights_io weights_io_compressed weights_init qmat_dense qmat_sparse \
         attn kda glue arena_build model_opt; do
  case $f in
    # load-time only: -Os keeps the weights range decoder small
    weights_io_compressed) O="$(echo $TFFLAGS | sed 's/-O3/-Os/')" ;;
    *) O="$TFFLAGS" ;;
  esac
  echo "  CC tf/$f.cpp"
  $CXX $O -c tf/$f.cpp -o obj/$f.o
done

echo "  CC coder0.cpp"
$CXX $C0FLAGS -c coder0.cpp -o obj/coder0.o
echo "  LD $OUT"
$CXX -o $OUT obj/coder0.o obj/weights_io.o obj/weights_io_compressed.o \
    obj/weights_init.o obj/qmat_dense.o obj/qmat_sparse.o obj/attn.o obj/kda.o obj/glue.o \
    obj/arena_build.o obj/model_opt.o -lm
echo "done: $OUT"
