# ccsds -- lossless BMP coder on CCSDS 123.0-R-1
#
# One translation unit: ccsds_bmp.cpp includes the .inc fragments and the
# CCSDS 123 implementation, so this compiles a single file.

CXX      ?= g++
CXXFLAGS ?= -O2 -g
# NO_COMPUTE_LOCAL keeps the predictor from materialising four whole local
# difference cubes; NDEBUG is what the reference implementation ships with.
DEFS     := -DNDEBUG -DNO_COMPUTE_LOCAL
WARN     := -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
            -Wno-sign-compare -Wno-write-strings -Wno-format
LDLIBS   := -lm

BIN := ccsds
SRC := src/ccsds_bmp.cpp
DEP := $(BIN).d

all: $(BIN)

# -MMD writes the list of everything the file pulled in, the included .c and
# .inc files among them, so editing any of them rebuilds.
$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(DEFS) $(WARN) -MMD -MP -MF $(DEP) -Isrc -o $@ $< $(LDLIBS)

-include $(DEP)

# Round trips the whole corpus under every coder setting.
check: $(BIN)
	./test/roundtrip.sh

# Feeds the decoder damaged streams.  Worth running against a sanitizer build:
#   make clean && make sancheck
fuzz: $(BIN)
	python3 test/fuzz.py

# Both suites over a build with the address and undefined behaviour sanitizers.
sancheck:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"
	./test/roundtrip.sh
	python3 test/fuzz.py
	$(MAKE) clean

clean:
	rm -f $(BIN) $(DEP)

.PHONY: all check fuzz sancheck clean
