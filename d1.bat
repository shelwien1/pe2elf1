@echo off

if not exist "%1"  exit

rmdir /S /Q out >nul 2>&1

rar330 x -o+ %1

del %~n1.bmp
bin\BMF.exe -O%~n1.bmp %~n1.bmf

bin\png2bmp.exe d %~n1.bmp out\meta

rem for %%a in (out\dump*.hif) do bin\raw2hif d out\%%~na.unp %%a out\%%~na.bin

for %%a in (out\dump*.hif) do (
  move out\%%~na.unp out\%%~na.bin.u
  move out\%%~na.hif out\%%~na.bin.r
  bin\preflate_raw -r out\%%~na.bin
)

bin\pngdump.exe out\meta %~n1.png

del DLRAW1.bmf DLRAW1.bmp.meta2 >nul 2>&1

rem rmdir /S /Q out >nul 2>&1
