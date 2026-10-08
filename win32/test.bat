@echo off
rem Self test of ROSE for Windows.  Translates the programs in the examples folder
rem with identityTranslator (a C program using the Windows API, and a C++17 program
rem using the standard library), draws the AST of one with dotGenerator, changes
rem a copy of examples\refactor.cpp with the refactoring tools (rose-ren, rose-m2g
rem and rose-using), and, if gcc and g++ are in the PATH, checks that the
rem translated and changed programs behave like the originals.  The files are
rem written to a new folder in the TEMP folder.
rem
rem usage: test.bat [nopause]
setlocal
set "ROSE=%~dp0"
set "BIN=%ROSE%bin"
set "EXAMPLES=%ROSE%examples"
set "WORK=%TEMP%\rose-test-%RANDOM%%RANDOM%"
set "FAILED="

rem Paths are echoed through FOR variables, so that they may contain an ampersand
echo ROSE for Windows: self test
for %%i in ("%ROSE%") do echo   ROSE:   %%~i
for %%i in ("%WORK%") do echo   output: %%~i
echo.
mkdir "%WORK%" 2> nul
if not exist "%WORK%\" goto :cannot_create
cd /d "%WORK%"

echo [1/6] identityTranslator: C with the Windows API (examples\hello.c)
"%BIN%\identityTranslator.exe" -rose:skipfinalCompileStep -c "%EXAMPLES%\hello.c" > hello.log 2>&1
call :check %ERRORLEVEL% rose_hello.c hello.log

echo [2/6] identityTranslator: C++17 with the standard library (examples\shapes.cpp)
"%BIN%\identityTranslator.exe" -std=c++17 -rose:skipfinalCompileStep -c "%EXAMPLES%\shapes.cpp" > shapes.log 2>&1
call :check %ERRORLEVEL% rose_shapes.cpp shapes.log

echo [3/6] dotGenerator: the AST of examples\hello.c as a GraphViz graph
"%BIN%\dotGenerator.exe" "%EXAMPLES%\hello.c" > dot.log 2>&1
call :check %ERRORLEVEL% hello.c.dot dot.log

echo [4/6] the refactoring tools, on refactor.cpp (a copy of examples\refactor.cpp)
copy "%EXAMPLES%\refactor.cpp" refactor.cpp > nul
echo       rose-ren refactor.cpp balance
"%BIN%\rose-ren.exe" refactor.cpp balance > rename_list.txt 2> rename_list.log
call :check_tool %ERRORLEVEL% rename_list.txt "Account::balance" rename_list.log
if not errorlevel 1 for /f "delims=" %%l in (rename_list.txt) do echo         %%l
echo       rose-ren refactor.cpp balance:1 getBalance
"%BIN%\rose-ren.exe" refactor.cpp balance:1 getBalance > rename.log 2>&1
call :check_tool %ERRORLEVEL% refactor.cpp "account.getBalance()" rename.log
echo       rose-m2g refactor.cpp Account::deposit
"%BIN%\rose-m2g.exe" refactor.cpp Account::deposit > m2g.log 2>&1
call :check_tool %ERRORLEVEL% refactor.cpp "deposit(&account, stack.top())" m2g.log
echo       rose-using refactor.cpp
"%BIN%\rose-using.exe" refactor.cpp > using.log 2>&1
call :check_tool %ERRORLEVEL% refactor.cpp "using Container<T>::items;" using.log

gcc --version > nul 2>&1
if errorlevel 1 goto :no_compiler
g++ --version > nul 2>&1
if errorlevel 1 goto :no_compiler
echo [5/6] the translated programs, compiled with gcc and g++, behave like the originals
call :compare hello gcc hello.c
call :compare shapes g++ shapes.cpp "-std=c++17"
echo [6/6] the changed refactor.cpp, compiled with g++, prints what the original prints
echo       (compiled with Visual C++: g++ does not compile the original)
call :refactored
goto :end

:no_compiler
echo [5/6] skipped: gcc and g++ are not in the PATH (see README.txt)
echo [6/6] skipped: gcc and g++ are not in the PATH
goto :end

:cannot_create
for %%i in ("%WORK%") do echo cannot create the folder %%~i
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

rem :check_tool STATUS FILE TEXT LOG -- the tool succeeded if STATUS is 0 and FILE
rem contains TEXT
:check_tool
if not "%~1"=="0" goto :check_tool_failed
findstr /l /c:"%~3" "%~2" > nul 2>&1
if errorlevel 1 goto :check_tool_failed
echo         ok
exit /b 0
:check_tool_failed
echo         FAILED (exit status %~1):
type "%~4"
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

rem :refactored -- builds the changed refactor.cpp with g++ and compares its output
rem with that of the original program
:refactored
g++ -o refactor.exe refactor.cpp > refactor_compile.log 2>&1
if errorlevel 1 goto :refactored_compile_failed
refactor.exe > refactor.txt 2>&1
> refactor_expected.txt echo depth 2, top 2, balance 102
fc refactor_expected.txt refactor.txt > refactor_compare.txt
if errorlevel 1 goto :refactored_differ
echo       ok, refactor.exe prints:
type refactor.txt
exit /b 0
:refactored_compile_failed
echo       FAILED to compile the changed refactor.cpp with g++:
type refactor_compile.log
set "FAILED=1"
exit /b 1
:refactored_differ
echo       FAILED: refactor.exe prints something else:
type refactor_compare.txt
set "FAILED=1"
exit /b 1

:end
echo.
set "STATUS=0"
if defined FAILED set "STATUS=1"
if defined FAILED echo Some tests FAILED.
if not defined FAILED for %%i in ("%WORK%") do echo All tests passed.  The output files are in %%~i
rem Keep the window open when the script was started from Explorer
if /i not "%~1"=="nopause" echo "%CMDCMDLINE%" | find /i "%~nx0" > nul && pause
exit /b %STATUS%
