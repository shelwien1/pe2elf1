#  make                 balrogg and tsv2wav
#  make tsv2wav-vorbis  the libvorbis-assisted tsv2wav (smaller meta, see META.md)
#  make test            round-trip 00.ogg through both tools and compare
#  make test TESTFILES=path/to/dir     round-trip every .ogg in a directory
#
#  CXXFLAGS is yours to set.  The flags the build needs rather than prefers are
#  kept separate, so `make CXXFLAGS='-O0 -g'` cannot silently drop -Wall or
#  -fwrapv; and changing CXXFLAGS or CXX does force a rebuild, which comparing
#  timestamps alone would not.

#  `CXX ?= c++` would not fire: make defines CXX=g++ as a built-in, so ?= sees
#  it as already set and a clang-only host cannot build.  A plain = still loses
#  to `make CXX=clang++` on the command line, which is what we want.
CXX       = c++
CXXFLAGS ?= -O2
WARN     ?= -Wall -Wextra
LIBS      = -lm

#  Not optional, and not CXXFLAGS: the lifting transform in lift.inc is exactly
#  reversible because its butterflies are integer sums and differences, and that
#  stays true when they wrap -- which is what makes a codebook with an enormous
#  float32 delta round-trip bit-exactly today instead of falling apart.  But
#  signed overflow is undefined without this, so the compiler is entitled to
#  assume it cannot happen.  A stream that reaches it is a real, self-consistent
#  Ogg Vorbis file, so the tool has to keep working on one.  -fwrapv makes the
#  behaviour it already relies on the behaviour the standard promises; it
#  changes no output on any file, pathological or not.
REQ       = -fwrapv

TESTFILES ?= 00.ogg

BLR_INC = common.inc tsv.inc ogg_page.inc vb_info.inc vb_book.inc \
          vb_floor.inc vb_res.inc vb_map.inc vb_setup.inc vb_ctx.inc \
          io.inc source.inc link_walk.inc codec.inc
W2V_INC = common.inc tsv.inc lift.inc floor_db.inc imdct.inc vd_setup.inc \
          wav.inc vd_dec.inc vd_ana.inc vd_rec.inc

#  The vorbis build adds these two and drops floor_db.inc, whose table it takes
#  from libvorbis instead (tsv2wav.cpp:99).
VRB_INC = $(filter-out floor_db.inc,$(W2V_INC)) vfloor.inc vbooks.inc
VRB_SRC = $(wildcard vorbislib/*.c vorbislib/*.h vorbislib/vorbis/*.h \
                     vorbislib/ogg/*.h vorbislib/modes/*.h vorbislib/books/*/*.h)

#  -Drestrict and -D_USE_MATH_DEFINES are inert against glibc but are what
#  libvorbis needs on the toolchains that do not define them; they cost nothing
#  here and keep this flag set the same as gc.bat's.  What matters is what is
#  *absent*: no -fpermissive.  See vorbislib/patch.py.
VRB_FLAGS = -std=c++17 -DBLR_VORBIS -D_USE_MATH_DEFINES -Drestrict=__restrict \
            -I vorbislib

all: balrogg tsv2wav

#  make compares timestamps on files, not command lines, so `make CXXFLAGS=-O0`
#  would otherwise leave the -O2 binaries in place and report nothing to do.
#  This records the flags and is newer than the binaries whenever they change.
.flags: FORCE
	@echo '$(CXX)|$(CXXFLAGS)|$(WARN)|$(REQ)|$(VRB_FLAGS)|$(LIBS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp
FORCE:

balrogg: main.cpp $(BLR_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ main.cpp

tsv2wav: tsv2wav.cpp $(W2V_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ tsv2wav.cpp $(LIBS)

tsv2wav-vorbis: tsv2wav.cpp $(VRB_INC) $(VRB_SRC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) $(VRB_FLAGS) -o $@ tsv2wav.cpp $(LIBS)

#  `sh t.sh`, not `./t.sh`: the exec bit does not survive every way this tree
#  gets copied around, and losing it should not break the test target.
test: balrogg tsv2wav
	sh t.sh $(TESTFILES)

test-vorbis: balrogg tsv2wav-vorbis
	TSV2WAV=./tsv2wav-vorbis sh t.sh $(TESTFILES)

clean:
	rm -f balrogg tsv2wav tsv2wav-vorbis .flags .flags.tmp
	rm -f *.tsv *.tsr *.wav *.meta *.ogr

.PHONY: all test test-vorbis clean FORCE
