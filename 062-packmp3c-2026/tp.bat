@echo off
rem Full roundtrip: mp3 -> mp3rw c -> mp3c c -> mp3c d -> mp3rw d, then compare
rem the restored mp3 with the original.  Only the mp3 has to come back exactly;
rem mp3c d does not reproduce every byte of the intermediate .dmp.
rem "tp.bat" tests cat1.mp3; "tp.bat a.mp3 b.mp3" tests those.

setlocal
if not exist mp3rw.exe ( echo mp3rw.exe not found - run gc.bat first & goto :end )
if not exist mp3c.exe ( echo mp3c.exe not found - run g.bat / gc.bat first & goto :end )

if "%~1"=="" ( set LIST=cat1.mp3 ) else ( set LIST=%* )
for %%f in (%LIST%) do call :one "%%f"
goto :end

:one
if not exist %1 ( echo === %~nx1 : missing, skipped & goto :eof )
echo === %~nx1 ===
mp3rw.exe c %1 "%~n1.dmp" "%~n1.out"
if errorlevel 1 ( echo    mp3rw c FAILED & goto :eof )
mp3c.exe c "%~n1.dmp" "%~n1.c"
if errorlevel 1 ( echo    mp3c c FAILED & goto :eof )
mp3c.exe d "%~n1.c" "%~n1.dm2"
if errorlevel 1 ( echo    mp3c d FAILED & goto :eof )
mp3rw.exe d "%~n1.dm2" "%~n1.out" "%~n1.unp"
if errorlevel 1 ( echo    mp3rw d FAILED & goto :eof )
fc /b %1 "%~n1.unp" >nul
if errorlevel 1 ( echo    roundtrip MISMATCH ) else ( echo    roundtrip OK )
for %%s in (%1) do echo    mp3  %%~zs
for %%s in ("%~n1.c") do echo    .c   %%~zs
for %%s in ("%~n1.out") do echo    .out %%~zs
del "%~n1.dmp" "%~n1.dm2" "%~n1.unp"
goto :eof

:end
endlocal
