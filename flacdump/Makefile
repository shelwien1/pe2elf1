CXX ?= g++
CXXFLAGS ?= -O2
FLAGS = -std=c++20 -fno-exceptions -fno-rtti -Wall

flacdump: flacdump.cpp
	$(CXX) $(FLAGS) $(CXXFLAGS) -o $@ $<

clean:
	rm -f flacdump
