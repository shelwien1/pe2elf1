# aac2tsv - lossless AAC <-> TSV converter (single translation unit, C++11)
#   make                  build ./aac2tsv
#   make test FILES="..." round-trip check (c, d, compare) on the given files
#   make CXX=x86_64-w64-mingw32-g++ LDFLAGS=-static

CXX      ?= c++
CXXFLAGS ?= -O2
LDLIBS   += -lm

DEPS = aac2tsv.cpp $(wildcard *.h *.inc)

all: aac2tsv

aac2tsv: $(DEPS)
	$(CXX) $(CXXFLAGS) -Wall $(LDFLAGS) -o $@ aac2tsv.cpp $(LDLIBS)

test: aac2tsv
	./roundtrip.sh ./aac2tsv $(FILES)

clean:
	rm -f aac2tsv aac2tsv.exe

.PHONY: all test clean
