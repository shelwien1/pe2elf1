# CDM -- compressed data model post-coder.  Linux/POSIX build.
#
#   make                 build both revisions
#   make r044 / r045     build one
#   make test            round-trip both on the bundled testfile
#   make selftest        r045's exhaustive forward/backward suite (~20 s)
#   make ab              size comparison of the two revisions
#   make clean
#
# Override the compiler or flags from the environment:
#   make CXX=clang++
#   make CXXARCH=-march=native

CXX      ?= g++
CXXARCH  ?=
OPT      ?= -O3
LDFLAGS  ?=

R044     := 044-EOF--v4
R045     := 045-BIJ--v1

COMMON   := -std=gnu++1z $(OPT) $(CXXARCH) -DNDEBUG -I Lib3 \
            -fwhole-program -fstrict-aliasing -fomit-frame-pointer -ffast-math \
            -fno-rtti -fno-exceptions \
            -fno-stack-protector -fno-stack-check -fno-check-new

# r045 uses Lib3's filehandle (coro_fhp2.inc), so it needs the std file backend.
CXXFLAGS_044 := $(COMMON)
CXXFLAGS_045 := $(COMMON) -DFILE_API_STD

LIBDEPS  := $(wildcard Lib3/*.inc) $(wildcard Lib3/*.h)
DEPS_044 := $(wildcard $(R044)/*.inc) $(wildcard $(R044)/MOD/*.inc) $(LIBDEPS)
DEPS_045 := $(wildcard $(R045)/*.inc) $(wildcard $(R045)/MOD/*.inc) $(LIBDEPS)

.PHONY: all r044 r045 test selftest ab clean

all: r044 r045
r044: $(R044)/cdm
r045: $(R045)/cdm

$(R044)/cdm: $(R044)/cdm.cpp $(DEPS_044)
	$(CXX) $(CXXFLAGS_044) -s $(LDFLAGS) $< -o $@

$(R045)/cdm: $(R045)/cdm.cpp $(DEPS_045)
	$(CXX) $(CXXFLAGS_045) -s $(LDFLAGS) $< -o $@

test: all
	@set -e; \
	tmp=$$(mktemp -d); trap 'rm -rf "$$tmp"' EXIT; \
	for v in $(R044) $(R045); do \
	  ./$$v/cdm c $(R044)/testfile "$$tmp/t.cdm" >/dev/null; \
	  ./$$v/cdm d "$$tmp/t.cdm" "$$tmp/t.out" >/dev/null; \
	  cmp $(R044)/testfile "$$tmp/t.out"; \
	  printf 'round-trip OK  %-14s %s -> %s bytes\n' "$$v" \
	    "$$(wc -c < $(R044)/testfile)" "$$(wc -c < "$$tmp/t.cdm")"; \
	done

selftest: r045
	./$(R045)/cdm t

ab: all
	@set -e; \
	tmp=$$(mktemp -d); trap 'rm -rf "$$tmp"' EXIT; \
	printf '%-14s %10s %10s %10s %7s\n' file input r044 r045 delta; \
	: > "$$tmp/empty"; printf 'a' > "$$tmp/one"; \
	head -c 200000 /dev/urandom > "$$tmp/rnd"; \
	head -c 200000 /dev/zero   > "$$tmp/zero"; \
	for f in "$$tmp/empty" "$$tmp/one" "$$tmp/zero" "$$tmp/rnd" $(R044)/testfile; do \
	  ./$(R044)/cdm c "$$f" "$$tmp/a4" >/dev/null; \
	  ./$(R045)/cdm c "$$f" "$$tmp/a5" >/dev/null; \
	  ./$(R045)/cdm d "$$tmp/a5" "$$tmp/d5" >/dev/null; cmp "$$f" "$$tmp/d5"; \
	  s4=$$(wc -c < "$$tmp/a4"); s5=$$(wc -c < "$$tmp/a5"); \
	  printf '%-14s %10s %10s %10s %7s\n' "$$(basename $$f)" "$$(wc -c < $$f)" "$$s4" "$$s5" "$$((s5-s4))"; \
	done

clean:
	rm -f $(R044)/cdm $(R045)/cdm $(R044)/*.o $(R045)/*.o
