@echo off

..\timetest ..\memchk coder0.exe c ../enwik7 1

..\timetest ..\memchk coder0.exe d 1 2

md5sum ../enwik7 2
