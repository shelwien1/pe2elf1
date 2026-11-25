# pdfdump -- single translation unit: src/pdfdump.cpp + src/*.inc
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2
SRC      := src/pdfdump.cpp $(wildcard src/*.inc)

pdfdump: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ src/pdfdump.cpp

pdfdump-asan: $(SRC)
	$(CXX) -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -o $@ src/pdfdump.cpp

# make test CORPUS="dir/*.pdf"
test: pdfdump
	./pdfdump selftest
	tests/roundtrip.sh -b ./pdfdump $(CORPUS) > /dev/null

clean:
	rm -f pdfdump pdfdump-asan

.PHONY: test clean
