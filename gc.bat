@echo off

goto skip

for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX/idx2inc.pl %%a >nul
    move /y "%%~pna_*.inc" MOD/
  )
)

:skip

rem The macro chain runs the repo's own perl scripts now, not ../Lib3's: the
rem current rc_kernel.c needs the fixed rc_macro.pl (function names must be
rem identifiers) and defines.pl (nothing after the parameter list opens a
rem block) that sit next to it, and txt2inc.pl's non-greedy marker.
perl rc_macro.pl rc_kernel.c
perl defines.pl  rc_kernel_macro.c

if exist rc_kernel_macro_D.c  move /y rc_kernel_macro_D.c rc_kernel.ispc >nul

del rc_kernel_macro.c rc_kernel_macro_U.c

rem ---------------------------------------------------------------------------
rem The coding kernel, compiled into the exe; rc_ispc.cpp is the host side.
rem
rem The -D values must match the RC_* configuration of the C++ build below --
rem these are the defaults (RCNUM=16, BLKSIZE=65536, LOWBYTES=8, CODBYTES=4).
rem RC_LOWSPLIT=1 here is kernel-only and stream-neutral: the split low
rem accumulator measures ~5% faster under ispc while the host keeps the
rem 64-bit one -- see build.sh's mapping for which -DRC_* sets what.
rem Two targets and a dispatcher, so the exe runs on AVX2 and AVX-512 alike.
rem
rem -DRC_DECSPLIT=1 on the C++ line below enables the split decoder (the
rem batch kernel is always in the object); measure it, it is off by default
rem for a reason -- rc_split_ispc_v1.md section 9.2.
rem ---------------------------------------------------------------------------
set ispc=C:\ispc260625\bin\ispc.exe

del rc_kernel_ispc*.obj rc_kernel_ispc*.h

%ispc% --target=avx512skx-x16,avx2-i32x16 --arch=x86-64 -O2 ^
 -DRCNUM=16 -DSCALElog=15 -DhSCALE=16384 ^
 -DLOWBYTES=8 -DCODBYTES=4 -DRC_LOWSPLIT=1 -DBLKFULL=65536 ^
 -DRC_DEV_WORDOUT=1 -DRC_RANGE64=0 -DRC_RENORM_TAIL=0 ^
 rc_kernel.ispc -o rc_kernel_ispc.obj -h rc_kernel_ispc.h

set path=C:\VC\link64
rem C:\VC2019\bin\amd64;
set LIB=C:\VC2019\lib\amd64;C:\VC2019\sdk\Lib\x64;C:\VC2019\ucrt\x64
set INCLUDE=

rem -DUNICODE -D_UNICODE 

rem RC_ISPC_THREAD=0 drops <thread> from rc_ispc.cpp (the worker becomes a
rem synchronous call in DEV_Collect) if the static MSVC STL link objects to it.
set backend=-DRC_ISPC=1

set incs=-std=c++17 -DSTRICT -DNDEBUG -DWIN32 -D_WIN32 -I../Lib3 -Drestrict=__restrict %backend% ^
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

%gcc% -s -std=c++23 -Ofast -O3 -fpermissive -Wno-format %arch% %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp rc_ispc.cpp rc_kernel_ispc.obj rc_kernel_ispc_avx512skx.obj rc_kernel_ispc_avx2.obj -o coder.exe 

rem -o coder.exe -fsanitize=bounds
rem -S -fverbose-asm -mllvm --x86-asm-syntax=intel 

del link.exe

del *.o

