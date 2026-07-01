@echo off

del *.exe

set incs=-DNDEBUG -DSTRICT -ILib3 -D_WIN32

set opts=-fstrict-aliasing -fomit-frame-pointer -ffast-math -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions
rem -fpermissive
rem -flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

rem -fprofile-use -fprofile-correction 

:set gcc=C:\MinGW710\bin\g++.exe -m32 
set gcc=C:\MinGW810\bin\g++.exe -m32
set gcc=C:\MinGW810x\bin\g++.exe 
set gcc=C:\MinGW820\bin\g++.exe -march=pentium2
set gcc=C:\cygwin\bin\g++.exe -march=k8
set gcc=C:\msys64\usr\bin\g++.exe -march=k8
set gcc=C:\MinGW820x\bin\g++.exe -march=native -mtune=native
set gcc=C:\MinGWD20\bin\g++.exe -march=k8
set gcc=C:\MinGWD21x\bin\g++.exe -march=haswell -DWRAPPER_SIMD -masm=att
set gcc=C:\MinGWD21x\bin\g++.exe -march=haswell -DWRAPPER_SIMD -masm=att 
set gcc=C:\MinGWD21x\bin\g++.exe -march=haswell -DWRAPPER_SIMD -masm=att
:set gcc=C:\MinGWG00x\bin\g++.exe -march=k8
set path=%gcc%\..\

del *.exe *.o

%gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static coder0.cpp -o coder0.exe

rem %gcc% -c -Ofast -std=gnu++17 %incs% %opts% -static zstd1/zstd.cpp 
