@echo off

del *.exe

rem -DFILE_API_STD: r045 drives Lib3 filehandles (coro_fhp2.inc); the stdio
rem backend is the one the self-test and the Linux build exercise.
set incs=-DNDEBUG -DSTRICT -DNDEBUG -DWIN32 -DINC_FLEN -DFILE_API_STD -I../Lib3

set opts=-fwhole-program -fstrict-aliasing -fomit-frame-pointer -ffast-math ^
-fno-rtti -fno-exceptions -fpermissive  ^
-fno-stack-protector -fno-stack-check -fno-check-new

:set gcc=C:\MinGW710\bin\g++.exe -m32 
set gcc=C:\MinGW820x\bin\g++.exe
set gcc=C:\MinGW810\bin\g++.exe -m32
set gcc=C:\MinGW810x\bin\g++.exe 
set gcc=C:\MinGW510\bin\g++.exe -m32 -mno-sse
set gcc=C:\MinGW820\bin\g++.exe -m32 -mno-sse
set gcc=C:\MinGW820x\bin\g++.exe -m64 -march=k8 -mno-sse
set gcc=C:\MinGWG20x\bin\g++.exe -m64 -march=k8
set path=%gcc%\..\

del *.exe *.o

%gcc% -std=gnu++1z -O9 -s %incs% %opts% -static cdm.cpp -o cdm.exe

