@echo off

goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

set path=C:\VC\link64
rem C:\VC2019\bin\amd64;
set LIB=C:\VC2019\lib\amd64;C:\VC2019\sdk\Lib\x64;C:\VC2019\ucrt\x64
set INCLUDE=

rem -DUNICODE -D_UNICODE 

set incs=-std=c++17 -DSTRICT -DNDEBUG -DWIN32 -D_WIN32 -I../Lib3 -Drestrict=__restrict ^
-D_CRT_SECURE_NO_WARNINGS ^
-D_CRT_SECURE_NO_DEPRECATE ^
-D_CRT_DISABLE_PERFCRIT_LOCKS ^
-D_CRT_NONSTDC_NO_DEPRECATE ^
-D_SECURE_SCL=0 ^
-D_ITERATOR_DEBUG_LEVEL=0 ^
-D_SECURE_SCL_THROWS=0 ^
-D_HAS_ITERATOR_DEBUGGING=0 ^
-DCOMMON_SKIP_BSF ^
-DWIN32_LEAN_AND_MEAN ^
-nostdlibinc -nostdinc++ ^
-isystem "C:/clangM10x/lib/clang/22/include" ^
-isystem "C:/VC2019/include" -isystem "C:/VC2019/sdk/include" -isystem "C:/VC2019/ucrt/include"

set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing ^
-Wmsvc-not-found -Wno-nonportable-include-path -Wno-pragma-pack -Wno-ignored-pragma-intrinsic ^
-Wno-ignored-attributes -Wno-expansion-to-defined -fno-stack-clash-protection -flto -fuse-ld=lld

rem -mllvm -inline-threshold=0 -fno-inline-functions

rem -fwhole-program -fno-unsafe-math-optimizations 

set targ=haswell
set arch=-march=%targ% -mtune=%targ% -fms-compatibility -fms-compatibility-version=19 -fms-extensions -Wno-ignored-attributes -m64 
rem -mno-avx2 -mavx512f

set gcc=C:\clang900\bin\clang++.exe
set gcc=C:\clang801x\bin\clang++.exe
set gcc=C:\clangB00x\bin\clang++.exe
set gcc=C:\clangJ104x\bin\clang++.exe -include "C:\VC2019\include\vcruntime_new.h" -IC:\clangJ104x\lib\clang\19\include
set gcc=C:\clangK10x\bin\clang++.exe -include "C:\VC2019\include\vcruntime_new.h" -IC:\clangK10x\lib\clang\20\include
set gcc=C:\clangL15x\bin\clang++.exe -include "C:\VC2019\include\vcruntime_new.h" -IC:\clangL15x\lib\clang\21\include
set gcc=C:\clangM10x\bin\clang++.exe

del *.exe *.o

set VS2017INSTALLDIR=
set VS140COMNTOOLS=

copy /b C:\VC2019\bin\amd64\link.exe .

for /D %%a in (.) do set DIRNAM=%%~na

%gcc% -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format %arch% %incs% %opts% -static mrpc.cpp -o mrpc.exe

rem --- with the encoder's optimisation loop on an OpenCL device --------
rem The headers and the ICD loader import library come from any vendor SDK
rem -- Intel oneAPI, the CUDA toolkit, the AMD APP SDK -- so point these at
rem wherever one of them landed and use the second line instead:
rem
rem set clsdk=C:\Program Files (x86)\Intel\oneAPI\compiler\latest
rem %gcc% -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format %arch% %incs% %opts% ^
rem   -DMRP_OPENCL -I"%clsdk%\include" mrpc.cpp "%clsdk%\lib\OpenCL.lib" -o mrpc.exe

rem -o coder.exe -fsanitize=bounds
rem -S -fverbose-asm -mllvm --x86-asm-syntax=intel 

del link.exe

del *.o



