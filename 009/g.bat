@echo off

goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

del *.exe

set incs=-DNDEBUG -DSTRICT -DWIN32 -I../Lib3

set opts=-fomit-frame-pointer -fstrict-aliasing -ffast-math -fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-rtti -fno-exceptions -fpermissive 

set arch=k8
set gcc=C:\MinGWG10x\bin\g++.exe -march=%arch% -mtune=%arch%
set path=%gcc%\..\

del *.exe *.o

%gcc% -s -std=gnu++1z -Ofast %incs% %opts% -static bmf.cpp -o bmf.exe

rem mim-src/static.c

