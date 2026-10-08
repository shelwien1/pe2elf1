CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

x64pp: src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h
	$(CXX) $(CXXFLAGS) -o $@ src/x64pp.cpp

# Windows binary with mingw-w64
x64pp.exe: src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h
	x86_64-w64-mingw32-g++ -O2 -Wall -Wextra -static -s -o $@ src/x64pp.cpp

clean:
	rm -f x64pp x64pp.exe

.PHONY: clean
