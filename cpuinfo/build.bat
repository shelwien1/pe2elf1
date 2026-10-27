@echo off
rem Build cpuinfo.exe on Windows. MinGW-w64 g++ (in PATH; a 32-bit MinGW makes a 32-bit exe):
g++ -std=c++11 -O2 -s -static -o cpuinfo.exe cpuinfo.cpp
rem or clang++ (MSVC target): clang++ -std=c++14 -O2 -o cpuinfo.exe cpuinfo.cpp
rem or MSVC (2019 tested), from an x64 or x86 developer prompt: cl /O2 /EHsc /MT cpuinfo.cpp
