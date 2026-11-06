@echo off
rem Build mp4pack.exe and mp4gui.exe on Windows. MinGW-w64 g++ and windres (in PATH):
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
windres mp4gui.rc -O coff -o mp4gui_rc.o
g++ -std=c++11 -O2 -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp mp4gui_rc.o -lcomdlg32 -lole32 -lshell32 -lgdi32
rem or clang++ for the MSVC target (wmain needs no -municode there), e.g.
rem clang++ -std=c++17 -O2 -o mp4pack.exe mp4pack.cpp
rem (mp4gui's icon there: rc mp4gui.rc, and mp4gui.res on its link line)
