#!/bin/sh
# Linux/POSIX counterpart of g.bat.
#
# Differences from the Windows script:
#   -DSTRICT -DWIN32   dropped, they only mean something to <windows.h>
#   -DINC_FLEN         dropped, cdm.cpp already defines it (defining it twice
#                      is just a -Wmacro-redefined warning)
#   -fpermissive       dropped, the dependent-base names are pulled in with
#                      explicit "using" declarations now
#   -O9                -> -O3 (gcc clamps anything above 3 to 3 anyway)
#   -static            optional, see LDFLAGS below
#
# Overridable from the environment, e.g.
#   CXX=clang++ ./g.sh
#   CXXARCH="-march=native" ./g.sh
#   LDFLAGS=-static ./g.sh

set -e

CXX=${CXX:-g++}
CXXARCH=${CXXARCH:-}
LDFLAGS=${LDFLAGS:-}

incs="-DNDEBUG -I../Lib3"

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
