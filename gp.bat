@echo off

set path=C:\mingwG20x\bin

g++ -s -static -O2 -march=x86-64-v3 -ffp-contract=off -std=c++11 -fwrapv -pthread h264p_src/h264p.cpp -o h264p.exe
