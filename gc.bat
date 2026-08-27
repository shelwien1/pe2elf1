@echo off

set path=C:\VC\link64
rem C:\VC2019\bin\amd64;
set LIB=C:\VC2019\lib\amd64;C:\VC2019\sdk\Lib\x64;C:\VC2019\ucrt\x64
set INCLUDE=

set incs=-std=c++1y -DSTRICT -DNDEBUG -DWIN32 -I. -ILib3 ^
-D_CRT_SECURE_NO_WARNINGS ^
-D_CRT_SECURE_NO_DEPRECATE ^
-D_CRT_DISABLE_PERFCRIT_LOCKS ^
-D_CRT_NONSTDC_NO_DEPRECATE ^
-D_SECURE_SCL=0 ^
-D_ITERATOR_DEBUG_LEVEL=0 ^
-D_SECURE_SCL_THROWS=0 ^
-D_HAS_ITERATOR_DEBUGGING=0 ^
-DCOMMON_SKIP_BSF ^
-IC:\clangJ104x\lib\clang\19\include ^
-IC:\VC2019\include-no-intrin -IC:\VC2019\sdk\include -IC:\VC2019\ucrt\include ^
-DVC_EXTRALEAN

rem -DWIN32_LEAN_AND_MEAN
rem -DUNICODE -D_UNICODE 

set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing -ffast-math ^
-Wmsvc-not-found -Wno-nonportable-include-path -Wno-pragma-pack -Wno-ignored-pragma-intrinsic ^
-Wno-ignored-attributes -Wno-expansion-to-defined -Wno-deprecated-ofast
rem -fno-stack-clash-protection 

rem -fwhole-program -fno-unsafe-math-optimizations 

set arch=-march=native -mtune=native -fms-compatibility -fms-compatibility-version=19 -fms-extensions -Wno-ignored-attributes -m64
set arch=-march=skylake -mtune=skylake -fms-compatibility -fms-compatibility-version=19 -fms-extensions -Wno-ignored-attributes -m64 -mno-avx2

set gcc=C:\clang900\bin\clang++.exe
set gcc=C:\clang801x\bin\clang++.exe
set gcc=C:\clangB00x\bin\clang++.exe
set gcc=C:\clangJ104x\bin\clang++.exe

del *.exe *.o

rem %gcc% -v -O9 %arch% %incs% %opts% test.cpp -o test.exe

copy /b C:\VC2019\bin\amd64\link.exe .

for /D %%a in (.) do set DIRNAM=%%~na

%gcc% -s -std=c++11 -O9 -Ofast %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp -o coder.exe %*

rem %gcc% -S -fverbose-asm -mllvm --x86-asm-syntax=intel  -std=c++11 -O9 -Ofast %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp

del link.exe

del *.o



