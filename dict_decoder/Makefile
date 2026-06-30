CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++11

# Single self-contained translation unit: the DictDecoder class and the
# driver both live in decode.cpp.
decode: decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ decode.cpp

# Decode the bundled sample and check it is bit-identical to book1.
.PHONY: test
test: decode
	./decode testdata/dict testdata/book1p testdata/out
	cmp testdata/out testdata/book1 && echo "OK: output identical to book1"

.PHONY: clean
clean:
	rm -f decode testdata/out
