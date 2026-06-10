@echo off

del 1 2 3

zipcl.exe d basis.pak 1 2 a3e30892 f9185194 eb474b09

zipcl.exe c 1 3 2 a3e30892 f9185194 eb474b09

unzip -t 1

md5sum basis.pak 3

