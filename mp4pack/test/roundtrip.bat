@echo off
rem roundtrip.bat MP4PACK "OPTIONS" FILE...
rem Compresses each file with mp4pack (the tools next to it) and the options,
rem decompresses it and compares; one line per file, in the current directory
rem the archives and the logs. Exit code 1 if any failed.
setlocal
set exe=%~1
set opt=%~2
shift
shift
set fail=0
:next
if "%~1"=="" goto done
del "%~nx1.mpk" "%~nx1.out" 2> nul
"%exe%" c %opt% "%~1" "%~nx1.mpk" > "%~nx1.c.log" 2>&1
set rc=%errorlevel%
"%exe%" d "%~nx1.mpk" "%~nx1.out" > "%~nx1.d.log" 2>&1
set rd=%errorlevel%
set ok=0
if "%rc%%rd%"=="00" fc /b "%~1" "%~nx1.out" > nul 2>&1 && set ok=1
set sz=-
for %%A in ("%~nx1.mpk") do set sz=%%~zA
if %ok%==1 (
  echo ok   %~nx1: %~z1 -^> %sz%
) else (
  echo FAIL %~nx1: c=%rc% d=%rd%
  set fail=1
)
del "%~nx1.out" 2> nul
shift
goto next
:done
exit /b %fail%
