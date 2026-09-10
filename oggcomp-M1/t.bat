@echo off

del 1 2 >nul 2>&1

..\timetest oggcomp c ..\07.ogg 1

..\timetest oggcomp d 1 2

md5sum ../07.ogg 2

