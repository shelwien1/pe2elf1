CXX      ?= g++
# -mavx2 selects the vector kernels; drop it (or raise it to -march=native) and
# the scalar ones are compiled instead.  -ffp-contract=off keeps the filter
# design arithmetic identical whatever the target: contracting a multiply and an
# add into one FMA rounds differently, which could shift a quantised coefficient.
# It costs nothing measurable, so it is on as insurance.
CXXFLAGS ?= -O2 -std=c++20 -fno-exceptions -fno-rtti -Wall -Wextra \
            -mavx2 -ffp-contract=off -pthread
LDFLAGS  ?=
LDLIBS   ?= -lm -pthread

# The whole program is a single translation unit: one header per module, each
# carrying its own implementation, all included by src/dff2dsf.cpp.
SRC := src/dff2dsf.cpp
DEPS := $(wildcard src/*.h)
BIN := dff2dsf

# Cross build for Windows.  Nothing here is platform specific beyond 64-bit file
# offsets and the clock, both handled in common.h.
WIN_CXX ?= x86_64-w64-mingw32-g++
WIN_BIN := dff2dsf.exe

all: $(BIN)

$(BIN): $(SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

windows: $(WIN_BIN)

$(WIN_BIN): $(SRC) $(DEPS)
	$(WIN_CXX) $(CXXFLAGS) -static $(SRC) -o $@

# The coroutine copies its own stack on every yield, so no frame in the
# conversion may be larger than Coroutine::STKPAD.  The only hit is Lib3's own
# 256 KB pad in call_do_process0, which is what that budget is measured against.
frames: $(SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) -Wframe-larger-than=65536 -c $(SRC) -o /dev/null

clean:
	rm -f $(BIN) $(WIN_BIN) src/*.o

.PHONY: all windows frames clean
