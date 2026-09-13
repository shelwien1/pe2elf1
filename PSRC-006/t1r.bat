@echo off

..\timetest psrc.exe cr d:\tmp5\enwik8 1r

..\timetest psrc.exe dr 1r 2

for %%a in (1r) do echo %%~za>>log.txt

md5sum d:/tmp5/enwik8 2
