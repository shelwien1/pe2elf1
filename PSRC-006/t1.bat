@echo off

..\timetest psrc.exe c d:\tmp5\enwik8 1

..\timetest psrc.exe d 1 2

for %%a in (1) do echo %%~za>>log.txt

md5sum d:/tmp5/enwik8 2
