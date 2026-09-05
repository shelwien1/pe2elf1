#!/usr/bin/env bash
# Linux equivalent of gc.bat -- one-shot build, no make required.
# gc.bat itself is left alone; it is still the Windows build.
set -e
cd "$(dirname "$0")"

CXX=${CXX:-g++}
OPT=${OPT:--O2}

# Windows-only flags from gc.bat that are deliberately NOT carried over:
#   -DSTRICT -DWIN32 -D_WIN32 -DWIN32_LEAN_AND_MEAN, the _CRT_*/_SECURE_SCL/
#   _HAS_ITERATOR_DEBUGGING/_ITERATOR_DEBUG_LEVEL defines, -nostdlibinc,
#   -nostdinc++, -fms-compatibility*, -fms-extensions, -Wmsvc-not-found,
#   -DCOMMON_SKIP_BSF, -D__DIRNAM__ (unreferenced), and the -isystem paths.
incs="-std=c++17 -DNDEBUG -I../Lib3 -Drestrict=__restrict"
opts="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing"

rm -f pjpg pjpg0i.inc pjpg0j.inc *.o

# codegen: preprocess, then turn coro2_get() calls into computed-goto resume points
"$CXX" -x c++ pjpg0.inc -E -P -o pjpg0i.inc
perl coro_fsm.pl

"$CXX" $OPT $incs $opts -Wno-format pjpg.cpp -o pjpg
rm -f *.o
echo "built ./pjpg with $CXX $OPT"
