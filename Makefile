# ccsds -- lossless BMP coder on CCSDS 123.0-R-1

CXX      ?= g++
CXXFLAGS ?= -O2 -g
# NO_COMPUTE_LOCAL keeps the predictor from materialising four whole local
# difference cubes; NDEBUG is what the reference implementation ships with.
DEFS     := -DNDEBUG -DNO_COMPUTE_LOCAL
WARN     := -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
            -Wno-sign-compare -Wno-write-strings -Wno-format
LDLIBS   := -lm

BIN  := ccsds
SRCS := src/ccsds_bmp.cpp \
        src/ccsds123/utils.c \
        src/ccsds123/predictor.c \
        src/ccsds123/entropy_encoder.c \
        src/ccsds123/decoder.c \
        src/ccsds123/unpredict.c
OBJS := $(addsuffix .o,$(basename $(SRCS)))

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEFS) $(WARN) -Isrc -c -o $@ $<

# The reference implementation is C, but it was published to be built with g++
# and the coder links it into one C++ binary, so it is compiled the same way.
%.o: %.c
	$(CXX) $(CXXFLAGS) $(DEFS) $(WARN) -Isrc -x c++ -c -o $@ $<

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
	rm -f $(OBJS) $(BIN)

.PHONY: all check fuzz sancheck clean
