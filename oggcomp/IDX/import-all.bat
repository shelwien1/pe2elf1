@echo off
rem  Fold an export of opt.pl into a level's .idx files.  Run it in the
rem  level's directory, as  ..\import-all.bat ..\..\export.!!!  in IDX\l9.

rar a -m5 -mdg -s -r- -ag idx_ *.idx

set f=%1
if (%f%)==() set f=export.!!!

for %%a in (*.idx) do (
  perl "%~dp0import.pl" %%a %f% > %%a.tmp
  move /y %%a.tmp %%a
)
