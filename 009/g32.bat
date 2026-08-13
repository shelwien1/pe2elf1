@echo off

del *.exe

set incs=-DNDEBUG -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

set opts=-fno-strict-aliasing -fomit-frame-pointer -fpermissive -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions

rem -flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

rem -fprofile-use -fprofile-correction 

:set gcc=C:\MinGW710\bin\g++.exe -m32 
set gcc=C:\MinGW810\bin\g++.exe -m32
set gcc=C:\MinGW810x\bin\g++.exe 
set gcc=C:\MinGW820\bin\g++.exe -march=pentium2
set gcc=C:\cygwin\bin\g++.exe -march=k8
set gcc=C:\msys64\usr\bin\g++.exe -march=k8
set gcc=C:\MinGW820x\bin\g++.exe -march=native -mtune=native
set gcc=C:\MinGWD21x\bin\g++.exe -march=k8
:set gcc=C:\MinGWG00x\bin\g++.exe -march=k8
set gcc=C:\MinGWD20\bin\g++.exe -march=k8 -m32 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing -fpermissive -fno-rtti -fno-exceptions
set path=%gcc%\..\

del *.exe *.o

rem %gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static dummy.cpp -o dummy.exe

%gcc% -s -O2 %incs% -static bmf.cpp -o bmf.exe

