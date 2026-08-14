@echo off

del chmrec.exe

set incs=-DNDEBUG -DSTRICT -ILib3 -D_WIN32 -Ilibmpg1231 -I. -DWIN32 -DNOXFERMEM -DOPT_GENERIC -DMPG123_INTMAP_H

set opts=-fstrict-aliasing -fomit-frame-pointer -ffast-math -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions -fpermissive

set gcc=C:\MinGWD21x\bin\g++.exe -march=k8
set path=%gcc%\..\

%gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static mp3.cpp -o mp3.exe

