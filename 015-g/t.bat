@echo off

rmdir /S /Q fp_mut fp_unp 
md fp_mut fp_unp 

stegdict.exe c @list1 stegdict.exe

stegdict.exe d @list2 payload

fc /b payload stegdict.exe
