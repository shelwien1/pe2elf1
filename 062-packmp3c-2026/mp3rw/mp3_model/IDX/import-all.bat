@echo off

rar a -m5 -mdg -s -r- -ag idx_ *.idx

set f=%1
if (%f%)==() set f=export.!!!

for %%a in (*.idx) do (
  perl import.pl %%a %f% > %%a.tmp
  move /y %%a.tmp %%a
)
