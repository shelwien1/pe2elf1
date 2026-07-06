@echo off

set Isave=%INCLUDE%
set Lsave=%LIB%
set Psave=%PATH%

if (%1)==() (
  set ic_arch=/arch:SSE2
  set gc_arch=-msse2
  set n_arch=SSE2
) else (
  set n_arch=%1
  set ic_arch=%2
  set gc_arch=%3
)

set exe_name=stegdict.exe
del %exe_name%

set ICLCFG=icl64.cfg
rem call C:\IntelH0048\bin-intel64\icl.bat %ic_arch% stegdict.cpp /Fe%exe_name%
rem call C:\IntelH0721\bin-intel64\icl.bat %ic_arch% stegdict.cpp /Fe%exe_name%
call C:\IntelH0721\bin-intel64\icl.bat %ic_arch% stegdict.cpp /Fe%exe_name%

del *.obj

set path=%Psave%
set LIB=%Lsave%
set INCLUDE=%Isave%
