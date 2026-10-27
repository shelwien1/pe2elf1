@echo off
rem Build mp4pack.exe on Windows. MinGW-w64 g++ (in PATH):
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
rem or clang++ for the MSVC target (wmain needs no -municode there), e.g.
rem clang++ -std=c++17 -O2 -o mp4pack.exe mp4pack.cpp
