# Linux/ELF build for pjpg (the JPEG structure parser).
#
# The Windows build (gc.bat + t.bat) is unchanged and still authoritative there;
# this Makefile is the Linux equivalent.  See ../README.md for the porting notes.
#
#   make                 build ./pjpg with the default compiler
#   make CXX=clang++     build with clang instead of gcc
#   make test            regression-test against tests/golden.log
#   make corpus          run every bundled jpeg (incl. imagetestsuite) for crashes/hangs
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

CXXFLAGS ?= $(STD) $(OPT) $(INCS) $(DEFS) $(CODEGEN)
LDFLAGS  ?=

# The coroutine saves and restores a live stack frame by hand.  The default
# backend ("asm") uses __builtin_setjmp/__builtin_longjmp on gcc and hand-written
# x86-64 asm on clang, both of which tolerate that.  The portable fallback
# ("libc") uses <setjmp.h>, which on glibc means _FORTIFY_SOURCE redirects
# longjmp to __longjmp_chk -- that aborts with
#   *** longjmp causes uninitialized stack frame ***
# so fortification has to be switched off for that backend.
CORO ?= asm
ifeq ($(CORO),libc)
CXXFLAGS += -DCORO_NOASM=1 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
endif

ifdef NATIVE
CXXFLAGS += -march=native
endif
ifdef LTO
CXXFLAGS += -flto
LDFLAGS  += -flto
endif
ifdef STATIC
LDFLAGS  += -static
endif
ifdef SAN
CXXFLAGS += -fsanitize=$(SAN) -g
LDFLAGS  += -fsanitize=$(SAN)
endif

# ------------------------------------------------------------ generation -----
# pjpg0.inc is preprocessed to pjpg0i.inc, then coro_fsm.pl rewrites every
# coro2_get() into a computed-goto resume point, producing pjpg0j.inc.
TAGS := tag_APP0.inc tag_APP1.inc tag_DHT.inc tag_DQT.inc tag_DRI.inc \
        tag_SOF.inc tag_SOS.inc

pjpg0i.inc: pjpg0.inc $(TAGS)
	$(CXX) -x c++ $< -E -P -o $@

pjpg0j.inc: pjpg0i.inc coro_fsm.pl
	$(PERL) coro_fsm.pl $< $@

# ---------------------------------------------------------------- build -----
DEPS := pjpg0j.inc pjpg1.inc cinfo.inc jpeg_marker.inc coro_fp2.inc \
        $(wildcard $(LIB3)/*.inc) $(wildcard $(LIB3)/*.h)

$(BIN): pjpg.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

all: $(BIN)
.DEFAULT_GOAL := all

# ---------------------------------------------------------------- tests -----
test: $(BIN)
	@./run-tests.sh check "./$(BIN)" "$(TESTDIR)"

golden: $(BIN)
	@./run-tests.sh golden "./$(BIN)" "$(TESTDIR)"

corpus: $(BIN)
	@./run-tests.sh corpus "./$(BIN)" "$(TESTDIR)"

crosscheck:
	@./run-tests.sh crosscheck "" "$(TESTDIR)"

matrix:
	@./run-tests.sh matrix "" "$(TESTDIR)"

clean:
	rm -f $(BIN) pjpg0i.inc pjpg0j.inc *.o

.PHONY: all test golden corpus crosscheck matrix clean
