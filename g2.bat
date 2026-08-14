@echo off

del mp2.exe

set incs=-DNDEBUG -DSTRICT -D_WIN32 -DWIN32

set opts=-fstrict-aliasing -fomit-frame-pointer -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions -fpermissive

set gcc=C:\MinGWD21x\bin\g++.exe -march=k8
set path=%gcc%\..\

%gcc% -s -O2 -std=gnu++17 %incs% %opts% -static mp2.cpp -o mp2.exe
