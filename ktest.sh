#!/bin/sh
# ktest.sh -- build and run the NLMS kernel differential test at several ISA
# levels.  The runtime-dispatch build exercises the function-pointer path; the
# -march ones exercise the inlined path, which is a different code generation of
# the same intrinsics.
set -e
for a in "" "-msse4.1" "-mavx2" "-march=native"; do
  g++ -std=gnu++20 -O2 -fno-exceptions -fno-rtti $a -I. ktest.cpp -o _ktest
  printf "%-16s " "${a:-portable}"
  ./_ktest
done
rm -f _ktest
