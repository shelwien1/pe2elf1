@echo off
rem Roundtrip check: compress each dump, decompress it again, compare with the
rem original.  "t.bat" tests cat.dmp and path.dmp; "t.bat foo.dmp" tests foo.dmp.

setlocal
set EXE=mp3c.exe
if not exist %EXE% ( echo %EXE% not found - run g.bat / gc.bat / c.bat first & goto :end )

if "%~1"=="" ( set LIST=cat.dmp path.dmp ) else ( set LIST=%* )
for %%f in (%LIST%) do call :one "%%f"
goto :end

:one
if not exist %1 ( echo === %~nx1 : missing, skipped & goto :eof )
echo === %~nx1 ===
%EXE% c %1 "%~n1.c"
if errorlevel 1 ( echo    compress FAILED & goto :eof )
%EXE% d "%~n1.c" "%~n1.unp"
if errorlevel 1 ( echo    decompress FAILED & goto :eof )
fc /b %1 "%~n1.unp" >nul
if errorlevel 1 ( echo    roundtrip MISMATCH ) else ( echo    roundtrip OK )
for %%s in (%1) do echo    original   %%~zs
for %%s in ("%~n1.c") do echo    compressed %%~zs
del "%~n1.unp"
goto :eof

:end
endlocal
