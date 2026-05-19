@echo off

set gcc=C:\mingw820x\bin\g++.exe

set path=%gcc%\..\

set opt=-Ofast -march=k8 -fpermissive 
rem -Wl,-Ttext-segment=0xF0000000 -Wl,--heap,0xFFF00000 -fno-pie -no-pie "-Wl,--image-base=0x400000"

%gcc% -s %opt% -static dummy.cpp -o dummy.exe
: mprotect.cpp 
