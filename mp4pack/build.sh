#!/bin/sh
# Cross-build mp4pack.exe and mp4gui.exe with MinGW-w64 (Linux); on Windows the
# same g++ lines work with a native MinGW-w64, or clang++ (MSVC target: no
# -municode needed).
CXX=${CXX:-x86_64-w64-mingw32-g++}
$CXX -std=c++11 -O2 -Wall -s -static -municode -o mp4pack.exe mp4pack.cpp
$CXX -std=c++11 -O2 -Wall -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp -lcomdlg32 -lole32 -lshell32 -lgdi32
