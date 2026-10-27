#!/bin/sh
# Build the Linux mp4pack (the same mp4pack.cpp, with winapi_posix.h for the
# Win32 functions it uses). Put it next to the tools converted by
# tools_linux.sh.
CXX=${CXX:-g++}
$CXX -std=c++11 -O2 -Wall -s -pthread -o mp4pack mp4pack.cpp
