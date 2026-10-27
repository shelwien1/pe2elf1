#!/bin/sh
# fetch_sources.sh [DIR] -- the LLVM and GCC files gen_targets.py reads, from
# llvm-project main and gcc master, into DIR/llvm and DIR/gcc (default DIR: src)
# Then: python3 gen_targets.py DIR/llvm DIR/gcc x86targets.h
d=${1:-src}
L=https://raw.githubusercontent.com/llvm/llvm-project/main
G=https://raw.githubusercontent.com/gcc-mirror/gcc/master
mkdir -p "$d/llvm" "$d/gcc" || exit 1
for f in llvm/include/llvm/TargetParser/X86TargetParser.def llvm/lib/TargetParser/X86TargetParser.cpp \
         llvm/lib/Target/X86/X86.td llvm/lib/TargetParser/Host.cpp; do
  curl -fsS -o "$d/llvm/$(basename $f)" "$L/$f" || { echo "cannot get $f" >&2; exit 1; }
done
# (Host.cpp, driver-i386.cc and cpuinfo.h are not read by gen_targets.py: they
# are what ClangName()/GccName() in x86targets.h follow, to compare by hand)
for f in gcc/config/i386/i386-isa.def gcc/config/i386/i386.h gcc/common/config/i386/i386-common.cc \
         gcc/config/i386/driver-i386.cc gcc/common/config/i386/cpuinfo.h; do
  curl -fsS -o "$d/gcc/$(basename $f)" "$G/$f" || { echo "cannot get $f" >&2; exit 1; }
done
echo "next: python3 gen_targets.py $d/llvm $d/gcc x86targets.h"
