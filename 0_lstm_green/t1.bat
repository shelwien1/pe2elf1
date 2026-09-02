@echo off

del 1

..\timetest coder0.exe c ..\book1 1

for %%a in (1) do echo %%~za >>log.txt

echo.>>log.txt


