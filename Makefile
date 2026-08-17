CXX      ?= g++
# -mavx2 selects the vector kernels; drop it (or raise it to -march=native) and
# the scalar ones are compiled instead.  -ffp-contract=off keeps the filter
# design arithmetic identical whatever the target: contracting a multiply and an
# add into one FMA rounds differently, which could shift a quantised coefficient.
# It costs nothing measurable, so it is on as insurance.
CXXFLAGS ?= -O2 -std=c++20 -fno-exceptions -fno-rtti -Wall -Wextra \
            -mavx2 -ffp-contract=off
LDFLAGS  ?=
LDLIBS   ?= -lm

# The whole program is a single translation unit; the .hpp files are the
# implementation, included by src/dff2dsf.cpp.
SRC := src/dff2dsf.cpp
DEPS := $(wildcard src/*.h) $(wildcard src/*.hpp)
BIN := dff2dsf

# Cross build for Windows.  Nothing here is platform specific beyond 64-bit file
# offsets and the clock, both handled in common.h and dffwrite.hpp.
WIN_CXX ?= x86_64-w64-mingw32-g++
WIN_BIN := dff2dsf.exe

all: $(BIN)

$(BIN): $(SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

windows: $(WIN_BIN)

$(WIN_BIN): $(SRC) $(DEPS)
	$(WIN_CXX) $(CXXFLAGS) -static $(SRC) -o $@

clean:
	rm -f $(BIN) $(WIN_BIN) src/*.o

.PHONY: all windows clean
