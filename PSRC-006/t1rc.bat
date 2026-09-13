@echo off

..\timetest psrc.exe cc 1r 1

..\timetest psrc.exe dc 1 2r

for %%a in (1) do echo %%~za>>log.txt

md5sum 1r 2r
