CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

SOURCES = src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h src/lzma.hpp src/tdelta.h

all: x64pp del_eh

x64pp: $(SOURCES)
	$(CXX) $(CXXFLAGS) -pthread -o $@ src/x64pp.cpp

del_eh: src/del_eh.cpp
	$(CXX) $(CXXFLAGS) -o $@ src/del_eh.cpp

# Windows binaries with mingw-w64
x64pp.exe: $(SOURCES)
	x86_64-w64-mingw32-g++ -O2 -Wall -Wextra -static -s -o $@ src/x64pp.cpp

del_eh.exe: src/del_eh.cpp
	x86_64-w64-mingw32-g++ -O2 -Wall -Wextra -static -s -o $@ src/del_eh.cpp

# test target for del_eh: C++ with try blocks, no exception thrown
hello_eh: tests/hello_eh.cpp
	$(CXX) -O2 -o $@ tests/hello_eh.cpp

hello_eh.exe: tests/hello_eh.cpp
	x86_64-w64-mingw32-g++ -O2 -static -o $@ tests/hello_eh.cpp

test-del-eh: del_eh x64pp
	sh tests/test_del_eh.sh

clean:
	rm -f x64pp x64pp.exe del_eh del_eh.exe hello_eh hello_eh.exe
	rm -rf tests/out

.PHONY: all clean test-del-eh
