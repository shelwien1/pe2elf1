CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

x64pp: src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h
	$(CXX) $(CXXFLAGS) -o $@ src/x64pp.cpp

clean:
	rm -f x64pp

.PHONY: clean
