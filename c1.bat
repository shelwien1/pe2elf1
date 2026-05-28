@echo off

if not exist "%1"  exit

rmdir /S /Q out >nul 2>&1

echo Dump deflate streams
bin\pngdump.exe %1 out\meta

echo Recompress deflate streams
rem for %%a in (out\dump*.bin) do bin\raw2hif c %%a out\%%~na.unp out\%%~na.hif

for %%a in (out\dump*.bin) do (
  bin\preflate_raw -s %%a
  move out\%%~na.bin.u out\%%~na.unp
  move out\%%~na.bin.r out\%%~na.hif
)

echo Convert unpacked dump data to .bmp
bin\png2bmp.exe c out\meta %~n1.bmp

bin\BMF.exe -S -Q9 %~n1.bmp

rar330 m -m5 -mdg -s %~n1.rar %~n1.bmf %~n1.bmp.meta2 out\*.hif

rmdir /S /Q out >nul 2>&1


