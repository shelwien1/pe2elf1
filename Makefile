# Build the exe2dll / rundll pair.
#
#   make           exe2dll (host build) + rundll.exe (Windows, needs MinGW-w64)
#   make exe2dll   just the converter, built for the host
#   make win       Windows builds of both tools (MinGW-w64)
#   make clean
#
# exe2dll is a build-time tool (pure file surgery, no OS dependency) so a host
# build is fine. rundll is a Windows program and needs a Windows toolchain.

CXX      ?= g++
MINGW    ?= x86_64-w64-mingw32-g++
CXXFLAGS ?= -O2 -Wall

all: exe2dll rundll.exe

exe2dll: exe2dll.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

rundll.exe: rundll.cpp
	$(MINGW) $(CXXFLAGS) -static -o $@ $<

# Optional: build the converter for Windows too.
win: exe2dll.exe rundll.exe
exe2dll.exe: exe2dll.cpp
	$(MINGW) $(CXXFLAGS) -o $@ $<

clean:
	rm -f exe2dll exe2dll.exe rundll.exe

.PHONY: all win clean
