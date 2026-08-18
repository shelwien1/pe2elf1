@echo off
rem t.cmd -- round-trip and size report over a corpus.  Usage: t.cmd [files...]
rem With no arguments it uses testfiles\*.bmp.  The Windows counterpart of t.sh:
rem it compresses, expands, and compares against the original, and exits
rem non-zero if any file fails to round-trip.
rem
rem Set BMG to point at a different executable.  The totals are batch integers,
rem so a corpus over two gigabytes will wrap -- report per-file sizes then.
setlocal

if not defined BMG set "BMG=bmg.exe"
if not exist "%BMG%" (
  echo %BMG% not found -- run "mk.cmd release" first, or set BMG to the exe.
  exit /b 1
)

set "FILES=%*"
if not defined FILES set "FILES=testfiles\*.bmp"

set "TDIR=%TEMP%\bmg_t.%RANDOM%%RANDOM%"
mkdir "%TDIR%" 2>nul

set "FAIL=0"
set /a TIN=0, TOUT=0, NF=0

rem Count first, so an empty corpus says so instead of printing a bare header.
for %%f in (%FILES%) do set /a NF+=1
if "%NF%"=="0" (
  echo no test files.  Put BMPs in testfiles\ ^(the seven from the BMF
  echo distribution are what the numbers in README.md are measured on^) or
  echo name them on the command line:  t.cmd path\to\*.bmp
  rmdir /s /q "%TDIR%" 2>nul
  exit /b 1
)

echo file                  input     output    ratio  round-trip
for %%f in (%FILES%) do call :one "%%f"
call :ratio RATIO %TOUT% %TIN%
call :row "TOTAL" "%TIN%" "%TOUT%" "%RATIO%" ""
rmdir /s /q "%TDIR%" 2>nul
exit /b %FAIL%

rem One file: compress, expand, compare, and add to the totals.
:one
set "B=%~nx1"
set /a NF+=1
"%BMG%" c "%~1" "%TDIR%\%B%.bmg" >nul 2>&1
if errorlevel 1 (echo %B%: compress failed & set "FAIL=1" & goto :eof)
"%BMG%" d "%TDIR%\%B%.bmg" "%TDIR%\%B%.out" >nul 2>&1
if errorlevel 1 (echo %B%: expand failed & set "FAIL=1" & goto :eof)
set "I=%~z1"
for %%g in ("%TDIR%\%B%.bmg") do set "O=%%~zg"
rem Size first, then fc /b -- see mk.cmd :cmp1 for why.
set "SO="
for %%g in ("%TDIR%\%B%.out") do set "SO=%%~zg"
set "RT=OK"
if not "%SO%"=="%I%" set "RT=*** MISMATCH ***"
if "%RT%"=="OK" (
  fc /b "%~1" "%TDIR%\%B%.out" >nul 2>&1
  if errorlevel 1 set "RT=*** MISMATCH ***"
)
if not "%RT%"=="OK" set "FAIL=1"
set /a TIN+=I, TOUT+=O
call :ratio R "%O%" "%I%"
call :row "%B%" "%I%" "%O%" "%R%" "%RT%"
goto :eof

rem %1 <- the ratio %2/%3 to three decimals.  Batch has no floating point, so
rem this is per-mille arithmetic with the point written in by hand, rounded to
rem match t.sh.  set /a is 32-bit, so the numerator is scaled down first --
rem three decimals never needed more than six significant digits anyway.
:ratio
set "RN=%~2"
set "RD=%~3"
:ratio_scale
if "%RD%"=="0" goto :ratio_zero
if %RN% LSS 1000000 goto :ratio_do
set /a RN=RN/10
set /a RD=RD/10
goto :ratio_scale
:ratio_do
if "%RD%"=="0" goto :ratio_zero
set /a RQ=(RN*2000/RD+1)/2
set /a RW=RQ/1000
set /a RP=RQ-RW*1000
set "RP=00%RP%"
set "%~1=%RW%.%RP:~-3%"
goto :eof
:ratio_zero
set "%~1=0.000"
goto :eof

rem One report line, columns padded to match t.sh's printf.
:row
set "C1=%~1                    "
set "C2=            %~2"
set "C3=            %~3"
set "C4=        %~4"
if "%~5"=="" (
  echo %C1:~0,16% %C2:~-10% %C3:~-10% %C4:~-7%
) else (
  echo %C1:~0,16% %C2:~-10% %C3:~-10% %C4:~-7%  %~5
)
goto :eof
