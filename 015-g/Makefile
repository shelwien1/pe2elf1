# Makefile for stegdict / stegchunks / sorted_chunks — Linux port of g.bat
#
#   make              build all three programs
#   make stegdict     dictionary-permutation steganography (Linux port of g.bat)
#   make stegchunks   the sorted_chunks variant (per-chunk line order)
#   make sorted_chunks partition a file into monotone chunks
#   make test         build stegdict, run its round-trip test (t.sh)
#   make testchunks   build stegchunks + sorted_chunks, run t_chunks.sh
#   make clean        remove built binaries and generated *.inc
#
# g.bat builds with MinGW g++ and -D_WIN32; that define is a Windows-target
# artifact and is dropped here so the glibc headers see a real Linux target.
# -static is kept: stegdict/stegchunks are used as their own test payloads and
# must be larger than the dictionary capacity (see t.sh / t_chunks.sh).
#
# usage.inc / usage_chunks.inc hold the help/error strings; they are generated
# from usage*.txt by txt2inc.pl (which also stamps the build date) and #included
# by the respective .cpp.

CXX      ?= g++

INCS     := -DNDEBUG -DSTRICT -ILib3
OPTS     := -fstrict-aliasing -fomit-frame-pointer -ffast-math \
            -fno-rtti -fno-stack-protector -fno-stack-check \
            -fno-check-new -fno-exceptions -fpermissive
ARCH     := -march=k8 -masm=att
# the help/error strings from usage*.inc are printf() format strings (txt2inc
# escapes % -> %% and maps $DATA -> %s), so printing a no-argument one is safe;
# silence Ubuntu g++'s default -Wformat-security on those calls.
CXXFLAGS := -s -Ofast -std=gnu++20 -Wno-format-security $(ARCH) $(INCS) $(OPTS)
LDFLAGS  := -static

.PHONY: all test testchunks clean
all: stegdict stegchunks sorted_chunks

stegdict: stegdict.cpp usage.inc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) stegdict.cpp -o stegdict

stegchunks: stegchunks.cpp usage_chunks.inc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) stegchunks.cpp -o stegchunks

# sorted_chunks is a standalone STL utility; build it with plain flags
# (it needs exceptions/rtti, unlike the steg codec).
sorted_chunks: sorted_chunks.cpp
	$(CXX) -O2 -std=c++17 sorted_chunks.cpp -o sorted_chunks

usage.inc: usage.txt txt2inc.pl
	perl txt2inc.pl usage.txt usage.inc

usage_chunks.inc: usage_chunks.txt txt2inc.pl
	perl txt2inc.pl usage_chunks.txt usage_chunks.inc

test: stegdict
	./t.sh

testchunks: stegchunks sorted_chunks
	./t_chunks.sh

clean:
	rm -f stegdict stegchunks sorted_chunks usage.inc usage_chunks.inc
