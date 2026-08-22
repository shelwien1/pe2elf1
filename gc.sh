#!/bin/sh
# Linux port of gc.bat - build bmf with clang++/lld.
#
# The MSVC-specific parts of gc.bat (-nostdlibinc/-nostdinc++ plus the
# -isystem paths into C:\VC2019, -fms-compatibility/-fms-extensions,
# the WIN32/_CRT_* defines, copying link.exe next to the sources) have no
# Linux counterpart and are dropped; everything else is kept as-is.

set -e

# clang++ from PATH unless overridden, e.g.  CXX=clang++-18 ./gc.sh
gcc=${CXX:-clang++}

incs="-DNDEBUG"

opts="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing
-Wno-ignored-attributes -Wno-expansion-to-defined -fno-stack-clash-protection -flto -fuse-ld=lld"

targ=haswell
arch="-march=$targ -mtune=$targ -m64"

rm -f bmf *.o

# shellcheck disable=SC2086
$gcc -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format $arch $incs $opts -static bmf.cpp -o bmf

rm -f *.o

ls -l bmf
