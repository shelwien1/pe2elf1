@echo off
rem Build pngtool.exe with MinGW g++ in one shot from list.txt.
cd /d "%~dp0"
g++ @list.txt -lpthread -static -o pngtool.exe
