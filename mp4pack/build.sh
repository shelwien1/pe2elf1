#!/bin/sh
# Cross-build mp4pack.exe with MinGW-w64 (Linux); on Windows the same g++ line
# works with a native MinGW-w64, or clang++ (MSVC target: no -municode needed).
CXX=${CXX:-x86_64-w64-mingw32-g++}
$CXX -std=c++11 -O2 -Wall -s -static -municode -o mp4pack.exe mp4pack.cpp
