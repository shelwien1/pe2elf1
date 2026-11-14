CXX ?= g++
CXXFLAGS ?= -O2
# -ffp-contract=off: the model replays floating point computations of the
# original encoders; results must be identical between c and d runs.
FLAGS = -std=c++20 -fno-exceptions -fno-rtti -ffp-contract=off -Wall

# flacdump.cpp includes the *.inc files (one translation unit)
flacdump: flacdump.cpp $(wildcard *.inc)
	$(CXX) $(FLAGS) $(CXXFLAGS) -o $@ $<

clean:
	rm -f flacdump
