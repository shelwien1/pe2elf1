#!/bin/sh
# Build cpuinfo: here (Linux; cpuinfo32 needs gcc-multilib), and with
# MinGW-w64 where it is installed (cpuinfo64.exe, cpuinfo32.exe)
CXX=${CXX:-g++}
$CXX -std=c++11 -O2 -Wall -s -o cpuinfo cpuinfo.cpp || exit 1
$CXX -m32 -std=c++11 -O2 -Wall -s -o cpuinfo32 cpuinfo.cpp 2>/dev/null || echo "(no cpuinfo32: no -m32 here)"
if command -v x86_64-w64-mingw32-g++ >/dev/null; then
  x86_64-w64-mingw32-g++ -std=c++11 -O2 -Wall -s -static -o cpuinfo64.exe cpuinfo.cpp || exit 1
fi
if command -v i686-w64-mingw32-g++ >/dev/null; then
  i686-w64-mingw32-g++ -std=c++11 -O2 -Wall -s -static -o cpuinfo32.exe cpuinfo.cpp || exit 1
fi
