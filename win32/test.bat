@echo off
rem Self test of ROSE for Windows.  Translates the programs in the examples folder
rem with identityTranslator (a C program using the Windows API, and a C++17 program
rem using the standard library), draws the AST of one with dotGenerator, and, if
rem gcc and g++ are in the PATH, checks that the translated programs behave like
rem the originals.  The files are written to a new folder in the TEMP folder.
rem
rem usage: test.bat [nopause]
setlocal
set "ROSE=%~dp0"
set "BIN=%ROSE%bin"
set "EXAMPLES=%ROSE%examples"
set "WORK=%TEMP%\rose-test-%RANDOM%%RANDOM%"
set "FAILED="

echo ROSE for Windows: self test
echo   ROSE:   %ROSE%
echo   output: %WORK%
echo.
mkdir "%WORK%" 2> nul
if not exist "%WORK%\" goto :cannot_create
cd /d "%WORK%"

echo [1/4] identityTranslator: C with the Windows API (examples\hello.c)
"%BIN%\identityTranslator.exe" -rose:skipfinalCompileStep -c "%EXAMPLES%\hello.c" > hello.log 2>&1
call :check %ERRORLEVEL% rose_hello.c hello.log

echo [2/4] identityTranslator: C++17 with the standard library (examples\shapes.cpp)
"%BIN%\identityTranslator.exe" -std=c++17 -rose:skipfinalCompileStep -c "%EXAMPLES%\shapes.cpp" > shapes.log 2>&1
call :check %ERRORLEVEL% rose_shapes.cpp shapes.log

echo [3/4] dotGenerator: the AST of examples\hello.c as a GraphViz graph
"%BIN%\dotGenerator.exe" "%EXAMPLES%\hello.c" > dot.log 2>&1
call :check %ERRORLEVEL% hello.c.dot dot.log

echo [4/4] the translated programs, compiled with gcc and g++, behave like the originals
gcc --version > nul 2>&1
if errorlevel 1 goto :no_compiler
g++ --version > nul 2>&1
if errorlevel 1 goto :no_compiler
call :compare hello gcc hello.c
call :compare shapes g++ shapes.cpp "-std=c++17"
goto :end

:no_compiler
echo       skipped: gcc and g++ are not in the PATH (see README.txt)
goto :end

:cannot_create
echo cannot create the folder %WORK%
set "FAILED=1"
goto :end

rem :check STATUS FILE LOG -- the step succeeded if STATUS is 0 and FILE exists
:check
if not "%~1"=="0" goto :check_failed
if not exist "%~2" goto :check_failed
echo       ok, wrote %~2
exit /b 0
:check_failed
echo       FAILED (exit status %~1):
type "%~3"
set "FAILED=1"
exit /b 1

rem :compare NAME COMPILER EXAMPLE "OPTIONS" -- builds EXAMPLE with COMPILER and with
rem identityTranslator, runs both programs and compares their output
:compare
%2 %~4 -o %1_original.exe "%EXAMPLES%\%3" > %1_original.log 2>&1
if errorlevel 1 goto :compare_compile_failed
"%BIN%\identityTranslator.exe" %~4 -o %1_rose.exe "%EXAMPLES%\%3" > %1_rose.log 2>&1
if errorlevel 1 goto :compare_translate_failed
%1_original.exe > %1_original.txt 2>&1
%1_rose.exe > %1_rose.txt 2>&1
fc %1_original.txt %1_rose.txt > %1_compare.txt
if errorlevel 1 goto :compare_differ
echo       ok, %1_rose.exe (built by ROSE) prints the same as %1_original.exe:
type %1_rose.txt
exit /b 0
:compare_compile_failed
echo       FAILED to compile %3 with %2:
type %1_original.log
set "FAILED=1"
exit /b 1
:compare_translate_failed
echo       FAILED to translate and compile %3:
type %1_rose.log
set "FAILED=1"
exit /b 1
:compare_differ
echo       FAILED: %1_original.exe and %1_rose.exe print different output:
type %1_compare.txt
set "FAILED=1"
exit /b 1

:end
echo.
set "STATUS=0"
if defined FAILED set "STATUS=1"
if defined FAILED echo Some tests FAILED.
if not defined FAILED echo All tests passed.  The output files are in %WORK%
rem Keep the window open when the script was started from Explorer
if /i not "%~1"=="nopause" echo "%CMDCMDLINE%" | find /i "%~nx0" > nul && pause
exit /b %STATUS%
