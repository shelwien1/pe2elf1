@echo off

rem The perl macro pass is gone: rc_macro.pl/defines.pl turned sh_v1xN.inc
rem into sh_v1xN_macro*.inc so the RCNUM lanes could live in local arrays.
rem rc_vec.inc holds RCNUM plain coder objects instead, so there is nothing
rem to generate -- just compile.

del coder.exe

for /D %%a in (.) do set DIRNAM=%%~na

set ICLCFG=icl64.cfg
set ICXCFG=icx64.cfg

set icl=C:\IntelJ2190\bin-intel64\icl2d.bat 
set icl=C:\IntelJl216\bin-cl64\icl2d.bat 
set icl=C:\IntelJl216\bin-ia32\icl2d.bat 
set icl=C:\IntelJl216\bin-intel64\icl2d.bat 
set icl=C:\IntelL1113\bin-intel64\icl2d.bat 
set icl=C:\IntelL1113\bin-cl64\icl2d.bat
set icl=C:\IntelM0316\bin-cl64\icl2d.bat 
set icl=C:\IntelN2400\bin-cl64\icl2d.bat 
set icl=C:\IntelN0320\bin-cl64\icl2d.bat 
set icl=C:\IntelL4924\bin-cl64\icl2d.bat 
set icl=C:\IntelL2317\bin-cl64\icl2d.bat 
set icl=C:\IntelK4912\bin-cl64\icl2d.bat 
set icl=C:\IntelJl216\bin-cl64\icl2d.bat 
set icl=C:\IntelJl216\bin-intel64\icl2d.bat 
set icl=C:\IntelL4924\bin-cl64\icl2d.bat 
set icl=C:\IntelL4924\bin-intel64\icl2d.bat 
set icl=C:\IntelN0320\bin-intel64\icl2d.bat 
set icl=C:\IntelM0123\bin-cl64\icl2d.bat 
set icl=C:\IntelM0316\bin-cl64\icl2d.bat 
set icl=C:\IntelN0320\bin-intel64\icl2d.bat -D__attribute__(x)#
set icl=C:\IntelN2420\bin-cl64\icl2d.bat -mllvm -loopopt=0 /clang:-fno-unsafe-math-optimizations

rem rc_kernel.cl/.inc are generated from rc_kernel.c -- skipped without perl,
rem since the generated file is committed and only the kernel's author needs it.
where perl >nul 2>nul && perl rc_macro.pl rc_kernel.c
where perl >nul 2>nul && perl defines.pl rc_kernel_macro.c
if exist rc_kernel_macro_D.c move /y rc_kernel_macro_D.c rc_kernel.cl >nul
if exist rc_kernel_macro.c del rc_kernel_macro.c
if exist rc_kernel_macro_U.c del rc_kernel_macro_U.c
where perl >nul 2>nul && perl txt2inc.pl -raw rc_kernel.cl rc_kernel.inc

call %icl% "/D__DIRNAM__=%DIRNAM%" coder.cpp FSM.cpp rc_cl.cpp rc_ispc.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp misc/lock_thread.cpp

del *.exp *.obj*  >nul 2>&1
