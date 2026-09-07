@echo off

del 00.meta 00.ogr 00.tsr 00.tsv 00.wav 

.\rogg.exe c 00.ogg 00.tsv
.\tsv2wav.exe c 00.tsv 00.wav 00.meta
.\tsv2wav.exe d 00.wav 00.tsr 00.meta
.\rogg.exe d 00.tsr 00.ogr

md5sum 00.tsv 00.tsr 00.ogg 00.ogr
