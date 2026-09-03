#!/bin/sh
# Linux/POSIX build for r045 (bijective coder over CoroutinePairs).
#
# Differs from 044-EOF--v4/g.sh only by -DFILE_API_STD: r045 uses Lib3's
# filehandle (coro_fhp2.inc) rather than raw FILE*, and file_api.inc would
# otherwise pick the Win32 backend on Windows.
#
#   CXX=clang++ ./g.sh        CXXARCH="-march=native" ./g.sh        LDFLAGS=-static ./g.sh

set -e

CXX=${CXX:-g++}
CXXARCH=${CXXARCH:-}
LDFLAGS=${LDFLAGS:-}

incs="-DNDEBUG -DFILE_API_STD -I../Lib3"

# clang has no -fwhole-program and warns about it; -flto is its equivalent here.
case "$($CXX --version 2>&1 | head -1)" in
  *clang*) WHOLE="-flto" ;;
  *)       WHOLE="-fwhole-program" ;;
esac

opts="$WHOLE -fstrict-aliasing -fomit-frame-pointer -ffast-math
-fno-rtti -fno-exceptions
-fno-stack-protector -fno-stack-check -fno-check-new"

rm -f cdm

# shellcheck disable=SC2086
$CXX -std=gnu++1z -O3 -s $CXXARCH $incs $opts $LDFLAGS cdm.cpp -o cdm

ls -l cdm
