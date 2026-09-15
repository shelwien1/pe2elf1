@echo off
rem
rem build.bat [program...] - build on Windows with MinGW-w64 GCC (or Clang) from
rem a plain cmd prompt. Needs g++ (or clang++) and perl on PATH, e.g. from
rem MSYS2 / w64devkit / Strawberry Perl / Git for Windows.
rem Programs default to "fpaq0mw tangelo_w"; each is built from <program>.cpp.
rem
rem   build.bat tangelo_w                   - just one
rem   set CXX=clang++          & build.bat  - pick the compiler
rem   set CXXFLAGS=-O3 -ffast-math & build.bat - replace the default flags
rem
rem The three steps per program mirror the original g.bat:
rem   1. compile the model step (encode_sim) to Intel-syntax assembly,
rem   2. let track.pl insert a journaling call in front of every store,
rem   3. build the executable from the main source plus the instrumented assembly.

setlocal
cd /d "%~dp0"

if "%CXX%"=="" set CXX=g++
if "%CXXFLAGS%"=="" set CXXFLAGS=-O3 -ffast-math -fomit-frame-pointer -fno-rtti -fno-exceptions -fno-stack-protector -march=native
if "%LDFLAGS%"=="" set LDFLAGS=-static -s

rem Win64 has no red zone, so step 1 needs nothing extra there; keeping the flag
rem makes the two build scripts produce the same code.
if "%SIMFLAGS%"=="" set SIMFLAGS=-mno-red-zone

set progs=%*
if "%progs%"=="" set progs=fpaq0mw tangelo_w

for %%p in (%progs%) do call :build %%p || exit /b 1

%CXX% -O2 timetest.cpp %LDFLAGS% -o timetest.exe 2>nul || echo note: timetest not built
exit /b 0

:build
set p=%1
del /q coder-%p%.s coder1-%p%.s %p%.exe 2>nul

echo [%p% 1/3] %CXX% -S -masm=intel -DSIM_FUNC %p%.cpp -o coder-%p%.s
%CXX% %CXXFLAGS% %SIMFLAGS% -S -masm=intel -DSIM_FUNC %p%.cpp -o coder-%p%.s || goto :fail

echo [%p% 2/3] perl track.pl coder-%p%.s coder1-%p%.s
perl track.pl coder-%p%.s coder1-%p%.s || goto :fail

echo [%p% 3/3] %CXX% %p%.cpp coder1-%p%.s -o %p%.exe
%CXX% %CXXFLAGS% %p%.cpp coder1-%p%.s %LDFLAGS% -o %p%.exe || goto :fail
exit /b 0

:fail
echo build failed: %p%
exit /b 1
