@echo off

del 1 2

..\timetest mrpc.exe c ..\20000171A.bmp 1
..\timetest mrpc.exe d 1 2

for %%a in (1) do echo %%~za>>log.txt

md5sum ../20000171A.bmp 2
