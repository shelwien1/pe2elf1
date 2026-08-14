#!/bin/sh
# g2.sh - build the mp2<->mp3 lossless converter on Linux (port of g2.bat).

set -e

rm -f mp2 *.o

incs="-DNDEBUG -DSTRICT"

opts="-fstrict-aliasing -fomit-frame-pointer -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions"

gpp="g++ -march=native"

$gpp -s -O2 -std=gnu++17 $incs $opts -static mp2.cpp -o mp2
