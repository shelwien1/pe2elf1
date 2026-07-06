@echo off

rem 7z a -mx=9 -m0=lzma:lc=0:lp=4:pb=4:fb=50 1 fp.bin 
rem 7z a -mx=9 -m0=lzma:lc=6:lp=2:pb=2:fb=80 1 fp_tim.bin 
rem 7z a -mx=9 -m0=lzma:lc=8:lp=2:pb=2:fb=40 1 fp_ip.bin 
rem 7z a -mx=9 -m0=lzma:lc=3:lp=0:pb=0:fb=273:mc=999 1 fp_get.txt fp_htp.txt fp_ref.txt fp_url.txt fp_ua.txt 

..\wrapper a fp_tim_ip.bin  fp_tim.bin fp_ip.bin 
..\wrapper a fp_txt.txt     fp_get.txt fp_htp.txt fp_ref.txt fp_url.txt fp_ua.txt 

..\lzma e fp.bin         fp_bin.arc     -lc0 -lp4 -pb4 -fb50
..\lzma e fp_tim_ip.bin  fp_tim_ip.arc  -lc6 -lp2 -pb2 -fb80
..\lzma e fp_txt.txt     fp_txt.arc     -lc3 -lp0 -pb0 -fb273 -mc999

..\wrapper a fp.arc fp_bin.arc fp_tim_ip.arc fp_txt.arc

del fp_bin.arc fp_tim_ip.arc fp_txt.arc fp_tim_ip.bin fp_txt.txt
