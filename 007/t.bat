@echo off

..\timetest paq8hpc.exe c book1 book1.hpc

..\timetest paq8hpc.exe d book1.hpc book1.unp

md5sum book1 book1.unp

