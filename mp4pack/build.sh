#!/bin/sh
# Cross-build mp4pack.exe and mp4gui.exe with MinGW-w64 (Linux); on Windows the
# same g++ lines work with a native MinGW-w64, or clang++ (MSVC target: no
# -municode needed). mp4gui's icon: mp4gui.rc (mp4gui.ico), by windres.
CXX=${CXX:-x86_64-w64-mingw32-g++}
WINDRES=${WINDRES:-x86_64-w64-mingw32-windres}
$CXX -std=c++11 -O2 -Wall -s -static -municode -o mp4pack.exe mp4pack.cpp
$WINDRES mp4gui.rc -O coff -o mp4gui_rc.o
$CXX -std=c++11 -O2 -Wall -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp mp4gui_rc.o -lcomdlg32 -lole32 -lshell32 -lgdi32
