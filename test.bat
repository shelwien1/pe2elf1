@echo off
rem
rem test.bat [program] [file] - the original t.bat with the tools found next to
rem it: compress and decompress a file, verify the round trip, and append the
rem timings and the compressed size to log.txt, the experiment log.
rem Defaults: program fpaq0mw, input ..\book1 (Calgary corpus; see README).

setlocal
cd /d "%~dp0" || (echo test.bat: cannot enter "%~dp0" - run from a local path, not a UNC share & exit /b 1)
set p=%1
if "%p%"=="" set p=fpaq0mw
set f=%2
if "%f%"=="" set f=..\book1

if not exist "%p%.exe" (echo test.bat: %p%.exe not built - run build.bat %p% & exit /b 1)
if not exist "timetest.exe" (echo test.bat: timetest.exe not built - see build.bat output & exit /b 1)
if not exist "%f%" (echo test.bat: %f% not found. book1 is the Calgary corpus file; see README. & exit /b 1)

rem Time into a scratch log, not straight into the experiment log: nothing should
rem reach log.txt until the round trip has verified. A half-entry there is worse
rem than none, since log.pl only closes a record on the size line and would
rem attribute stray timings to whatever ran next.
del /q 1 2 .timetest.log 2>nul
set TIMETEST_LOG=.timetest.log
timetest %p% c %f% 1 || exit /b 1
timetest %p% d 1 2  || exit /b 1

fc /b %f% 2 >nul || (del /q .timetest.log 2>nul & echo roundtrip FAILED - nothing written to log.txt & exit /b 1)

type .timetest.log >> log.txt
for %%a in (1) do echo %%~za>>log.txt
echo.>>log.txt
del /q .timetest.log 2>nul

for %%a in (1) do echo %%~za
echo roundtrip OK
