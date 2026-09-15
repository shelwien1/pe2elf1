@echo off
rem
rem build.bat - build fpaq0mw on Windows with MinGW-w64 GCC (or Clang) from a
rem             plain cmd prompt. Needs g++ (or clang++) and perl on PATH,
rem             e.g. from MSYS2 / w64devkit / Strawberry Perl / Git for Windows.
rem
rem   set CXX=clang++          & build.bat     - pick the compiler
rem   set CXXFLAGS=-O3 -ffast-math & build.bat - replace the default flags
rem
rem The three steps mirror the original g.bat:
rem   1. compile the model step (encode_sim) to Intel-syntax assembly,
rem   2. let track.pl insert a journaling call in front of every store,
rem   3. build the executable from the main source plus the instrumented assembly.

setlocal
cd /d "%~dp0"

if "%CXX%"=="" set CXX=g++
if "%CXXFLAGS%"=="" set CXXFLAGS=-O3 -ffast-math -fomit-frame-pointer -fno-rtti -fno-exceptions -fno-stack-protector -march=native
if "%LDFLAGS%"=="" set LDFLAGS=-static -s

del /q coder.s coder1.s fpaq0mw.exe timetest.exe 2>nul

echo [1/3] %CXX% -S -masm=intel -DSIM_FUNC fpaq0mw.cpp -o coder.s
%CXX% %CXXFLAGS% -S -masm=intel -DSIM_FUNC fpaq0mw.cpp -o coder.s || goto :fail

echo [2/3] perl track.pl coder.s coder1.s
perl track.pl coder.s coder1.s || goto :fail

echo [3/3] %CXX% fpaq0mw.cpp coder1.s -o fpaq0mw.exe
%CXX% %CXXFLAGS% fpaq0mw.cpp coder1.s %LDFLAGS% -o fpaq0mw.exe || goto :fail

rem timing helper used by test.bat (optional)
%CXX% -O2 timetest.cpp %LDFLAGS% -o timetest.exe 2>nul || echo note: timetest not built

exit /b 0

:fail
echo build failed
exit /b 1
