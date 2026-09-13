@echo off

rem split pipeline: cr+cc == c, dc+dr == d

psrc.exe cr ..\book1bwt 1r
psrc.exe cc 1r 1

psrc.exe dc 1 2r
psrc.exe dr 2r 2

fc /b 1r 2r
md5sum ../book1bwt 2
