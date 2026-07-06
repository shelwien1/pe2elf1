@echo off

del stegdict.exe

set incs=-DNDEBUG -DSTRICT -ILib3 -D_WIN32

set opts=-fstrict-aliasing -fomit-frame-pointer -ffast-math -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions -fpermissive

set gcc=C:\MinGWG10x\bin\g++.exe -march=k8 -masm=att
set path=%gcc%\..\

perl txt2inc.pl usage.txt usage.inc

%gcc% -s -Ofast -std=gnu++20 %incs% %opts% -static stegdict.cpp -o stegdict.exe
