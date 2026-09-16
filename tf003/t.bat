@echo off

..\timetest ..\memchk coder0.exe c book1000 1

..\timetest ..\memchk coder0.exe d 1 2

md5sum book1000 2
