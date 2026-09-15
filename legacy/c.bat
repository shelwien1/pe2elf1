@echo off                          

del *.exe

set ICLCFG=icl64.cfg
set ICXCFG=icx64.cfg

set icl=C:\IntelJ1144\bin-intel64\icl2a.bat 
set icl=C:\IntelJ0117\bin-intel64\icl2a.bat 
set icl=C:\IntelJ2190\bin-intel64\icl2a.bat 
set icl=C:\IntelN2400\bin-cl64\icl2d.bat

call %icl% /DSIM_FUNC fpaq0mw.cpp /c /clang:-emit-llvm -o coder.bc

C:\mingwD20x\bin\llc.exe --x86-asm-syntax=intel coder.bc
del coder.bc

C:\cygwin\bin\perl.exe 1a.pl

call %icl% -mllvm --x86-asm-syntax=intel fpaq0mw.cpp coder1.s

del *.obj *.exp
