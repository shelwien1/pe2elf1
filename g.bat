@echo off

del *.exe

rem The ISPC backend replaces OpenCL: compile the kernel once with ispc.exe,
rem then build with -DRC_OPENCL=0 -DRC_ISPC=1 and rc_kernel_ispc*.obj on the link:
rem   ispc --target=avx512skx-x16,avx2-i32x16 -O2 -DRC_ISPC=1 -DRCNUM=16 ^
rem     -DSCALElog=15 -DhSCALE=16384 -DLOWBYTES=8 -DCODBYTES=4 -DRC_LOWSPLIT=1 ^
rem     -DBLKFULL=65536 -DRC_CL_WORDOUT=1 -DRC_CL_BLOCKREAD=0 -DRC_RANGE64=0 ^
rem     -DRC_RENORM_TAIL=0 rc_kernel.cl -o rc_kernel_ispc.obj -h rc_kernel_ispc.h
rem The -D values must match the RC_* configuration -- see build.sh's mapping.

set incs=-DNDEBUG -DSTRICT -DWIN32 -I. -ILib3

set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-rtti -fno-exceptions -fpermissive -fstrict-aliasing -ftree-vectorize

set gcc=C:\MinGWB10x\bin\g++.exe -m64 -march=skylake
set gcc=C:\MinGWE20x\bin\g++.exe -m64 -march=skylake -mtune=skylake
set path=%gcc%\..\

del *.exe *.o

for /D %%a in (.) do set DIRNAM=%%~na

rem rc_kernel.cl/.inc are generated from rc_kernel.c -- skipped without perl,
rem since the generated file is committed and only the kernel's author needs it.
where perl >nul 2>nul && perl rc_macro.pl rc_kernel.c
where perl >nul 2>nul && perl defines.pl rc_kernel_macro.c
if exist rc_kernel_macro_D.c move /y rc_kernel_macro_D.c rc_kernel.cl >nul
if exist rc_kernel_macro.c del rc_kernel_macro.c
if exist rc_kernel_macro_U.c del rc_kernel_macro_U.c
where perl >nul 2>nul && perl txt2inc.pl -raw rc_kernel.cl rc_kernel.inc

%gcc% -g -std=gnu++1z -O9 -Ofast %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp rc_cl.cpp rc_ispc.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp -o coder.exe %*

