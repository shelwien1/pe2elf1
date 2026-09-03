# CDM -- compressed data model post-coder.  Linux/POSIX build.
#
#   make            build 044-EOF--v4/cdm
#   make test       build, then round-trip the bundled testfile
#   make clean      remove build products
#
# Override the compiler or flags from the environment:
#   make CXX=clang++
#   make CXXARCH=-march=native

CXX      ?= g++
CXXARCH  ?=
OPT      ?= -O3
LDFLAGS  ?=

SRCDIR   := 044-EOF--v4
BIN      := $(SRCDIR)/cdm

CXXFLAGS := -std=gnu++1z $(OPT) $(CXXARCH) -DNDEBUG -I Lib3 \
            -fwhole-program -fstrict-aliasing -fomit-frame-pointer -ffast-math \
            -fno-rtti -fno-exceptions \
            -fno-stack-protector -fno-stack-check -fno-check-new

DEPS := $(wildcard $(SRCDIR)/*.inc) $(wildcard $(SRCDIR)/MOD/*.inc) $(wildcard Lib3/*.inc) $(wildcard Lib3/*.h)

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRCDIR)/cdm.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -s $(LDFLAGS) $< -o $@

test: $(BIN)
	@set -e; \
	tmp=$$(mktemp -d); \
	trap 'rm -rf "$$tmp"' EXIT; \
	./$(BIN) c $(SRCDIR)/testfile "$$tmp/t.cdm" >/dev/null; \
	./$(BIN) d "$$tmp/t.cdm" "$$tmp/t.out" >/dev/null; \
	cmp $(SRCDIR)/testfile "$$tmp/t.out"; \
	printf 'round-trip OK: %s bytes -> %s bytes\n' \
	  "$$(wc -c < $(SRCDIR)/testfile)" "$$(wc -c < "$$tmp/t.cdm")"

clean:
	rm -f $(BIN) $(SRCDIR)/*.o
