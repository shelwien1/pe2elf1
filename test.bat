@echo off
rem
rem test.bat [program] [file] - the original t.bat with the tools found next to
rem it: compress and decompress a file, append the timings and the compressed
rem size to log.txt (the experiment log), and verify the round trip.
rem Defaults: program fpaq0mw, input ..\book1 (Calgary corpus).

setlocal
cd /d "%~dp0"
set p=%1
if "%p%"=="" set p=fpaq0mw
set f=%2
if "%f%"=="" set f=..\book1

del /q 1 2 2>nul
timetest %p% c %f% 1 || exit /b 1
timetest %p% d 1 2  || exit /b 1

for %%a in (1) do echo %%~za>>log.txt
echo.>>log.txt

for %%a in (1) do echo %%~za
fc /b %f% 2 >nul && echo roundtrip OK || (echo roundtrip FAILED & exit /b 1)
