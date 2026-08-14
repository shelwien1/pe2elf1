#!/bin/sh
# g.sh - build lzx on Linux (port of g.bat).

set -e

rm -f mp3 *.o

incs="-DNDEBUG -DSTRICT"

opts="-fstrict-aliasing -fomit-frame-pointer -ffast-math -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions"

gpp="g++ -march=native"

$gpp -s -Ofast -std=gnu++17 $incs $opts -static mp3.cpp -o mp3

