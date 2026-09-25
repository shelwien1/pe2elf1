@echo off

:goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

del *.exe

set incs=-DNDEBUG -DSTRICT -DWIN32 -I../Lib3 -I../pcre2 -DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=8 -DPCRE2_STATIC ^
-I./mim-include -DMI_BUILD_RELEASE -DMI_CMAKE_BUILD_TYPE=release -DMI_STATIC_LIB 

set opts=-fomit-frame-pointer -fstrict-aliasing -ffast-math -fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-rtti -fno-exceptions -fpermissive 

set gcc=C:\MinGWG20x\bin\g++.exe -march=haswell
set path=%gcc%\..\

del *.exe *.o

rem -DMI_MALLOC_OVERRIDE 
rem g++ -march=k8 -O3 -c -DMI_BUILD_RELEASE -DMI_CMAKE_BUILD_TYPE=release -DMI_STATIC_LIB mim-src/static.c -I./mim-include

%gcc% -s -std=gnu++26 -Ofast %incs% %opts% -static coder0.cpp -o coder0.exe

rem mim-src/static.c

