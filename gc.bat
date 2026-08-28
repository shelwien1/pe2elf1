@echo off

goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

rem The vector coder is generated from rc.inc for this configuration: clang's
rem preprocessor resolves the #if forest (RC_VECOUT=1 picks the vector-shape
rem coder bodies), rc_soa.pl turns the lane state into [RCNUM] arrays, and
rem the rc_macro.pl/defines.pl chain makes macros of it, included inside
rem do_process -- sh_v1xN's arrangement, one source. Pass the same -DRC_*
rem here as on the compile line below (RC_LOWBYTES/RC_LOWSPLIT/RC_CODBYTES).
set cpp=C:\clangN10x\bin\clang++.exe -E -P -x c++

%cpp% -DRC_VECOUT=1 -DRC_CARRYLESS=1 -imacros rc_config.inc rc.inc > rc_kernel0.c
perl rc_soa.pl rc_kernel0.c
perl rc_macro.pl rc_kernel1.c
perl defines.pl  rc_kernel1_macro.c

if exist rc_kernel1_macro_D.c  move /y rc_kernel1_macro_D.c rc_vecD.inc >nul

del rc_kernel1_macro.c rc_kernel1_macro_U.c

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
-isystem "C:/clangN10x/lib/clang/23/include" ^
-isystem "C:/VC2019/include" -isystem "C:/VC2019/sdk/include" -isystem "C:/VC2019/ucrt/include" ^
-isystem "D:/IntelSWTools4/compiler/2025.1/include"

rem -isystem "C:/clangM10x/lib/clang/22/include" ^


set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing ^
-Wmsvc-not-found -Wno-nonportable-include-path -Wno-pragma-pack -Wno-ignored-pragma-intrinsic ^
-Wno-ignored-attributes -Wno-expansion-to-defined -fno-stack-clash-protection -flto -fuse-ld=lld

rem -isystem "C:/Program Files (x86)/AMD APP/include"

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
set gcc=C:\clangN10x\bin\clang++.exe

del *.exe *.o

set VS2017INSTALLDIR=
set VS140COMNTOOLS=

copy /b C:\VC2019\bin\amd64\link.exe .

for /D %%a in (.) do set DIRNAM=%%~na

rem %gcc% -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format %arch% %incs% %opts% -static mrpc.cpp mrpc_lib.cpp OpenCL.lib -o mrpc.exe

%gcc% -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format %arch% %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp -o coder.exe 

rem -o coder.exe -fsanitize=bounds
rem -S -fverbose-asm -mllvm --x86-asm-syntax=intel 

del link.exe

del *.o

