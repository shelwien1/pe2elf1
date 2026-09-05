# Linux/ELF build for pjpg (the JPEG structure parser).
#
# The Windows build (gc.bat + t.bat) is unchanged and still authoritative there;
# this Makefile is the Linux equivalent.  See ../README.md for the porting notes.
#
#   make                 build ./pjpg with the default compiler
#   make CXX=clang++     build with clang instead of gcc
#   make test            regression-test against tests/golden.log
#   make corpus          run every bundled jpeg (incl. imagetestsuite) for crashes/hangs
#   make coders          Huffman vs arithmetic must agree on the same image
#   make crosscheck      confirm gcc and clang builds agree byte-for-byte
#   make matrix          build+run the full compiler/flag matrix
#   make clean           remove binaries and generated sources
#
# Knobs:
#   CXX=g++|clang++      compiler                      (default g++)
#   OPT=-O2              optimisation level            (default -O2)
#   CORO=asm|libc        coroutine backend             (default asm)
#   NATIVE=1             add -march=native
#   LTO=1                add -flto
#   STATIC=1             add -static
#   SAN=undefined        add -fsanitize=<...>

CXX      ?= g++
PERL     ?= perl
OPT      ?= -O2
LIB3     ?= ../Lib3
TESTDIR  ?= ../testfiles
BIN      ?= pjpg

# ---------------------------------------------------------------- flags -----
# Kept from gc.bat because they are meaningful on Linux too:
#   -DNDEBUG            release build
#   -Drestrict=__restrict   'restrict' is not a C++ keyword
#   -fomit-frame-pointer -fno-stack-protector -fstrict-aliasing
# Dropped from gc.bat because they are Windows/MSVC-only:
#   -DSTRICT -DWIN32 -D_WIN32 -DWIN32_LEAN_AND_MEAN, every _CRT_*/_SECURE_SCL/
#   _HAS_ITERATOR_DEBUGGING define, -nostdlibinc -nostdinc++, -fms-compatibility,
#   -fms-extensions, -Wmsvc-not-found, -DCOMMON_SKIP_BSF (guards an MSVC-only
#   declaration), and -D__DIRNAM__ (never referenced by any source file).
STD      := -std=c++17
DEFS     := -DNDEBUG -Drestrict=__restrict
INCS     := -I$(LIB3)
CODEGEN  := -fomit-frame-pointer -fno-stack-protector -fno-stack-check -fstrict-aliasing

# CXXFLAGS is yours to override (make CXXFLAGS=-O0 -g). INCS and DEFS are not
# folded into it, so overriding CXXFLAGS cannot accidentally drop the include
# path or the defines the sources need to compile at all.
CXXFLAGS ?= $(STD) $(OPT) $(CODEGEN)
LDFLAGS  ?=
EXTRA    :=
EXTRA_LD :=

# The coroutine saves and restores a live stack frame by hand.  The default
# backend ("asm") uses __builtin_setjmp/__builtin_longjmp on gcc and hand-written
# x86-64 asm on clang, both of which tolerate that.  The portable fallback
# ("libc") uses <setjmp.h>, which on glibc means _FORTIFY_SOURCE redirects
# longjmp to __longjmp_chk -- that aborts with
#   *** longjmp causes uninitialized stack frame ***
# so fortification has to be switched off for that backend.
CORO ?= asm
ifeq ($(CORO),libc)
EXTRA += -DCORO_NOASM=1 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
endif

ifdef NATIVE
EXTRA += -march=native
endif
ifdef LTO
EXTRA    += -flto
EXTRA_LD += -flto
endif
ifdef STATIC
EXTRA_LD += -static
endif
ifdef SAN
EXTRA    += -fsanitize=$(SAN) -g
EXTRA_LD += -fsanitize=$(SAN)
endif

ALL_CXXFLAGS := $(CXXFLAGS) $(INCS) $(DEFS) $(EXTRA)
ALL_LDFLAGS  := $(LDFLAGS) $(EXTRA_LD)

# ------------------------------------------------------------ generation -----
# pjpg0.inc is preprocessed to pjpg0i.inc, then coro_fsm.pl rewrites every
# coro2_get() into a computed-goto resume point, producing pjpg0j.inc.
# Globbed, not listed: pjpg0.inc includes every tag_*.inc, and a hand-kept list
# silently goes stale the moment a parser is added (it had missed three).
TAGS := $(wildcard tag_*.inc)

pjpg0i.inc: pjpg0.inc $(TAGS)
	$(CXX) -x c++ $< -E -P -o $@

pjpg0j.inc: pjpg0i.inc coro_fsm.pl
	$(PERL) coro_fsm.pl $< $@

# ---------------------------------------------------------------- build -----
DEPS := pjpg0j.inc pjpg1.inc cinfo.inc jpeg_marker.inc coro_fp2.inc \
        $(wildcard $(LIB3)/*.inc) $(wildcard $(LIB3)/*.h)

$(BIN): pjpg.cpp $(DEPS)
	$(CXX) $(ALL_CXXFLAGS) $< -o $@ $(ALL_LDFLAGS)

all: $(BIN)
.DEFAULT_GOAL := all

# ---------------------------------------------------------------- tests -----
test: $(BIN)
	@./run-tests.sh check "./$(BIN)" "$(TESTDIR)"

golden: $(BIN)
	@./run-tests.sh golden "./$(BIN)" "$(TESTDIR)"

corpus: $(BIN)
	@./run-tests.sh corpus "./$(BIN)" "$(TESTDIR)"

# Both of these compile pjpg.cpp themselves, so they need the generated source
# to exist -- without the prerequisite they fail on a fresh clone or after
# `make clean`, and crosscheck fails silently.
# Huffman vs arithmetic on losslessly-transcoded pairs of the same image
coders: $(BIN)
	@./run-tests.sh coders "./$(BIN)" "$(TESTDIR)"

crosscheck: pjpg0j.inc
	@./run-tests.sh crosscheck "" "$(TESTDIR)"

matrix: pjpg0j.inc
	@./run-tests.sh matrix "" "$(TESTDIR)"

clean:
	rm -f $(BIN) pjpg0i.inc pjpg0j.inc *.o

.PHONY: all test golden corpus coders crosscheck matrix clean

# The codegen writes fixed filenames into the source directory, and `clean`
# races `all` under -j: `make -j8 clean all` used to print "Nothing to be done"
# and leave no binary.
.NOTPARALLEL:
