# pdfdump -- single translation unit: src/pdfdump.cpp + src/*.inc
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3
# no exceptions, no RTTI: errors are bool returns (see fail() in src/chan.inc)
NOEXC    := -fno-exceptions -fno-rtti
SRC      := src/pdfdump.cpp $(wildcard src/*.inc)

pdfdump: $(SRC)
	$(CXX) $(CXXFLAGS) $(NOEXC) -pthread -o $@ src/pdfdump.cpp

pdfdump-asan: $(SRC)
	$(CXX) -std=c++17 -O1 -g $(NOEXC) -pthread -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -o $@ src/pdfdump.cpp

# make test CORPUS="dir/*.pdf"
test: pdfdump
	./pdfdump selftest
	tests/roundtrip.sh -b ./pdfdump $(CORPUS) > /dev/null

clean:
	rm -f pdfdump pdfdump-asan

.PHONY: test clean
