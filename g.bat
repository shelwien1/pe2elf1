@echo off

del *.exe

set incs=-DNDEBUG -DSTRICT -DWIN32 -I. -ILib3

set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-rtti -fno-exceptions -fpermissive -fstrict-aliasing -ftree-vectorize

set gcc=C:\MinGWB10x\bin\g++.exe -m64 -march=skylake
set gcc=C:\MinGWE20x\bin\g++.exe -m64 -march=skylake -mtune=skylake
set path=%gcc%\..\

del *.exe *.o

for /D %%a in (.) do set DIRNAM=%%~na

rem rc_kernel.cl/.inc are generated from rc_kernel0.cl -- skipped without perl,
rem since the generated file is committed and only the kernel's author needs it.
where perl >nul 2>nul && perl rc_macro.pl rc_kernel0.cl
where perl >nul 2>nul && perl defines.pl rc_kernel0_macro.cl
if exist rc_kernel0_macro_D.cl move /y rc_kernel0_macro_D.cl rc_kernel.cl >nul
if exist rc_kernel0_macro.cl del rc_kernel0_macro.cl
if exist rc_kernel0_macro_U.cl del rc_kernel0_macro_U.cl
where perl >nul 2>nul && perl txt2inc.pl -raw rc_kernel.cl rc_kernel.inc

%gcc% -g -std=gnu++1z -O9 -Ofast %incs% %opts% -static "-D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp rc_cl.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp -o coder.exe %*

