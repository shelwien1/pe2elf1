#!/bin/sh
# sqtest.sh -- build and run the squash/stretch accuracy test across the legal
# ST_SCALE range.  ST_SCALE is a compile-time constant (it comes from IDX), so
# sweeping it means rebuilding, the same way ktest.sh rebuilds per ISA level.
#
# 256 is what ships; the ends of the clamp and a non-power-of-two in the middle
# are there because opt.pl is free to move the parameter and the table
# construction must hold for wherever it lands, not just for the tuned value.
set -e
for s in 16 64 100 256 300 512 1024 2048; do
  g++ -std=gnu++20 -O2 -fno-exceptions -fno-rtti -DSQ_ST_SCALE=$s -I. sqtest.cpp -o _sqtest
  ./_sqtest
done
rm -f _sqtest
