@echo off

mp3rw0.exe c cat1.mp3 .dmp .out

mp3rw0.exe d .dmp .out .unp

md5sum cat1.mp3 .unp
