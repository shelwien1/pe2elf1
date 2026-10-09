CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

SOURCES = src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h src/lzma.hpp

x64pp: $(SOURCES)
	$(CXX) $(CXXFLAGS) -pthread -o $@ src/x64pp.cpp

# Windows binary with mingw-w64
x64pp.exe: $(SOURCES)
	x86_64-w64-mingw32-g++ -O2 -Wall -Wextra -static -s -o $@ src/x64pp.cpp

clean:
	rm -f x64pp x64pp.exe

.PHONY: clean
