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

set incs=-DNDEBUG -DSTRICT -I../Lib3

set opts=-fstrict-aliasing -fomit-frame-pointer -fpermissive -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions -fno-rtti 
rem -flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections
rem -fprofile-use -fprofile-correction 

:set gcc=C:\MinGW710\bin\g++.exe -m32 
set gcc=C:\MinGW810\bin\g++.exe -m32
set gcc=C:\MinGW810x\bin\g++.exe 
set gcc=C:\MinGW820\bin\g++.exe -march=pentium2
set gcc=C:\cygwin\bin\g++.exe -march=k8
set gcc=C:\msys64\usr\bin\g++.exe -march=k8
set gcc=C:\MinGW820x\bin\g++.exe -march=native -mtune=native
set gcc=C:\MinGWF20x\bin\g++.exe -march=k8
set path=%gcc%\..\

del *.exe *.o

%gcc% -s -Ofast -std=gnu++2a %incs% %opts% -static bcdr5.cpp -o bcdr5.exe

