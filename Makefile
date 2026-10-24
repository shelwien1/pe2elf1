# aacpack: a lossless compressor of AAC streams (aacpack_src/).
#
#   make                         optimized build -> ./aacpack
#   make CXXFLAGS="-O0 -g"       debug build
#   make PARCH=                  the plain C++ build (no AVX2), which codes identically
#   make test                    round-trip test/corpus (or FILES="...") through c and d
#
# aacpack is one translation unit (aacpack_src/aacpack.cpp includes the
# rest); it uses neither C++ exceptions nor RTTI nor the C++ library.

CXXFLAGS ?= -O2
# the target: x86-64-v3 (AVX2). PARCH= builds the plain C++ instead, for
# any target, which codes identically
PARCH ?= -march=x86-64-v3 -ffp-contract=off
SRCS := $(wildcard aacpack_src/*.inc aacpack_src/*.cpp aacpack_src/Lib3/* aacpack_src/MOD/*/*.inc)
FILES ?= $(wildcard test/corpus/*)

all: aacpack

aacpack: $(SRCS)
	$(CXX) $(CXXFLAGS) $(PARCH) -std=c++11 -fwrapv -fno-exceptions -fno-rtti -Iaacpack_src/Lib3 -pthread $(LDFLAGS) -o $@ aacpack_src/aacpack.cpp -lm

test: aacpack
	./test/roundtrip.sh ./aacpack $(FILES)

clean:
	rm -f aacpack

.PHONY: all test clean
