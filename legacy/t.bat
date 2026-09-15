@echo off

del 1 2
..\timetest fpaq0mw c ..\book1 1
..\timetest fpaq0mw d 1 2

for %%a in (1) do echo %%~za>>log.txt
echo.>>log.txt

md5sum 2 ../book1

