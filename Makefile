CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

all: mtf2ima

mtf2ima: mtf2ima.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test: mtf2ima
	./test/roundtrip.sh $(SAMPLES)

clean:
	rm -f mtf2ima

.PHONY: all test clean
