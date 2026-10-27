#!/bin/sh
# run.sh DUMPS_DIR [OUT]
# The checks against the compilers' own detection code (harness.sh builds the
# tools in OUT, default: here): every *CPUID*.txt under DUMPS_DIR (e.g. an
# InstLatx64 checkout) through gcc_harness and llvm_harness, then cmp_gcc and
# cmp_llvm compare their names and features with x86targets.h's.
HERE=$(cd "$(dirname "$0")" && pwd); OUT=${2:-$HERE}; cd "$OUT" || exit 1
find "$1" -name '*CPUID*.txt' -print0 | sort -z > dumps.lst
xargs -0 ./gcc_harness < dumps.lst > gcc_native.tsv
xargs -0 ./llvm_harness < dumps.lst > llvm_native.tsv
echo "== gcc:";  xargs -0 ./cmp_gcc gcc_native.tsv < dumps.lst | tail -12
echo "== llvm:"; xargs -0 ./cmp_llvm llvm_native.tsv < dumps.lst | tail -12
# the -march feature sets against the compilers themselves:
#   ./dumptab > table.tsv
#   python3 ver_clang.py /path/to/clang
#   python3 ver_gcc.py /path/to/gcc        (or: .../gcc/xgcc -B.../gcc/ of a build tree)
