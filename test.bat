@echo off
rem
rem test.bat [file] - the original t.bat with the tools found next to it:
rem compress and decompress a file, append the timings and the compressed size
rem to log.txt (the experiment log), and verify the round trip.
rem Default input: ..\book1 (Calgary corpus).

setlocal
cd /d "%~dp0"
set f=%1
if "%f%"=="" set f=..\book1

del /q 1 2 2>nul
timetest fpaq0mw c %f% 1 || exit /b 1
timetest fpaq0mw d 1 2  || exit /b 1

for %%a in (1) do echo %%~za>>log.txt
echo.>>log.txt

for %%a in (1) do echo %%~za
fc /b %f% 2 >nul && echo roundtrip OK || (echo roundtrip FAILED & exit /b 1)
