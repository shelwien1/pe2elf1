@echo off

:goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a 1 >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

del *.exe

rem -ILib3: the codec's RC derives from Coroutine, so common.inc / coro3b.inc /
rem coro3_pin*.inc / coro3_setjmp_*.h have to be reachable.
set incs=-DNDEBUG -DSTRICT -DWIN32 -I. -ILib3

set opts=-fomit-frame-pointer -fstrict-aliasing -ffast-math -fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-rtti -fno-exceptions -fpermissive 

set arch=haswell
set gcc=C:\MinGWG10x\bin\g++.exe -march=%arch% -mtune=%arch%
set path=%gcc%\..\

del *.exe *.o

%gcc% -s -std=gnu++2a -Ofast %incs% %opts% -static xadpcm.cpp -o xadpcm.exe
