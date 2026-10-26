@echo off

del mp3c.exe

set incs=-DNDEBUG -DSTRICT -ILib3 -D_WIN32 -DWIN32 

set opts=-fstrict-aliasing -fomit-frame-pointer -ffast-math -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions -fpermissive

set gcc=C:\MinGWG20x\bin\g++.exe -march=k8
set gcc=C:\MinGW630x\bin\g++.exe -march=k8
set gcc=C:\MinGWA21x\bin\g++.exe -march=k8
set path=%gcc%\..\

rem %gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static mp3.cpp -o mp3.exe

rem %gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static mp3fr/mp3frames.cpp -o mp3fr.exe

%gcc% -s -Ofast -std=gnu++11 %incs% %opts% -static -Imp3rw -DINC_LOG2I mp3c.cpp -o mp3c.exe

rem mp3pack: mp3rw and mp3c's model in aacpack's front end (mk.sh mp3pack's flags)
%gcc% -s -O2 -std=gnu++11 %incs% %opts% -fwrapv -Wno-invalid-offsetof -static -I. -Imp3rw mp3pack.cpp -o mp3pack.exe

rem %gcc% -s -Ofast -std=gnu++17 %incs% %opts% -static -fno-fast-math pcm-distance.cpp -o pcm-distance.exe

