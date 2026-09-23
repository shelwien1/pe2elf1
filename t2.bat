@echo off

del 1 1a 1b

coder0.exe c book1 1 
coder0.exe c wcc386 1a 
coder0.exe c book1wcc 1b

rem dir 1*>>log.txt

for %%a in (1) do  for %%b in (1a) do  for %%c in (1b) do  echo %%~za         %%~zb         %%~zc // >>log.txt
