@echo off

for %%a in (testfiles\*.bmp) do (

  del 1 2 >nul 2>&1

  echo [%%a]
  bmf c %%a 1
  bmf d 1 2

  md5sum testfiles/%%~nxa 2
  for %%b in (%%a,1) do echo %%b - %%~zb

  md5sum testfiles/%%~nxa 2 >>log.txt
  for %%b in (%%a,1) do echo %%b - %%~zb >>log.txt

  del 1 2 >nul 2>&1

  echo.
  echo.

)
