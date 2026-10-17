@echo off

set path=C:\mingwG20x\bin

g++ -s -static -O2 -std=c++11 -fwrapv h264p_src/h264p.cpp -o h264p.exe
