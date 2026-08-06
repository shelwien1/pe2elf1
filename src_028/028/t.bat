@echo off

del *_d5c

..\timetest ddsdet.exe cs --all ..\1_shar 1_shar_d5c

..\timetest ddsdet.exe cs --all ..\2_shar 2_shar_d5c

..\timetest dxt5comp.exe c ..\HarpoonGunBarrel_n.DDS HarpoonGunBarrel_n_d5c

rem dir *_d5c >>log.txt

for %%a in (*_d5c) do echo %%~za %%a>>log.txt

