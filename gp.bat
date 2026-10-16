@echo off

set path=C:\mingwG20x\bin

g++ -s -static -O2 -std=c++11 h264p_src/cabac.cpp h264p_src/cavlc.cpp h264p_src/h264p.cpp h264p_src/rows.cpp h264p_src/slice.cpp h264p_src/syntax.cpp h264p_src/tables.cpp -o h264p.exe
