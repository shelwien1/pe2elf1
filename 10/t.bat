@echo off

del 1 2

dummy.exe c .\book1 1
dummy.exe d 1 2

md5sum 1 ./book1.pmm 2 ./book1

