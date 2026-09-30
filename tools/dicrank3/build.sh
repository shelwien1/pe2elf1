#!/bin/sh
# build.sh [release|tune] - builds dicrank3 and its tuning harness.
#
#   ./build.sh         dicrank3 and dr3_tune, every IDX knob folded to a constant
#   ./build.sh tune    dr3_tune.tune: the knobs are live objects that IDX/opt.pl
#                      finds (by their "!MAP!" descriptors) and patches in the binary
#
# MOD/ is generated from IDX/*.idx by IDX/idx2inc.pl (needs perl) and left in
# the release state, which is what a plain "g++ dicrank3.cpp" builds against.
# Both builds code identically: the tuning build evaluates at run time the
# same float expressions the release build folds at compile time.
#
# Tuning (dumps from "dicrank3 e -D dump ..."):
#   ./build.sh tune
#   printf 'dump/sb\ndump/h\ndump/r\n' > opt.lst    # stream sets, coded as separate "files"
#   OPT_JOBS=3 perl IDX/opt.pl opt.lst ./dr3_tune.tune [name-regex]
#   cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done
set -e
cd "$(dirname "$0")"
MODE=${1:-release}
CXX=${CXX:-g++}
FLAGS="-O2 -std=c++17 -ffp-contract=off $CXXFLAGS"

gen() {   # gen NOCONST USENEW
  for f in IDX/*.idx; do
    b=$(basename "$f" .idx)
    IDX_NOCONST=$1 perl IDX/idx2inc.pl "$f" $2 >/dev/null
    mv -f "IDX/${b}_h.inc" "IDX/${b}_p.inc" MOD/
  done
}

case "$MODE" in
  release)
    gen 1 0
    $CXX $FLAGS -o dicrank3 dicrank3.cpp
    $CXX $FLAGS -o dr3_tune dr3_tune.cpp ;;
  tune)
    gen 0 1
    $CXX $FLAGS -o dr3_tune.tune dr3_tune.cpp
    gen 1 0 ;;
  *) echo "usage: $0 [release|tune]" >&2; exit 2 ;;
esac
