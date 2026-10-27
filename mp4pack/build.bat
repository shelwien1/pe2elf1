@echo off
rem Build mp4pack.exe and mp4gui.exe on Windows. MinGW-w64 g++ (in PATH):
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
g++ -std=c++11 -O2 -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp -lcomdlg32 -lole32 -lshell32 -lgdi32
rem or clang++ for the MSVC target (wmain needs no -municode there), e.g.
rem clang++ -std=c++17 -O2 -o mp4pack.exe mp4pack.cpp
