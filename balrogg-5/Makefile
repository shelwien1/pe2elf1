#  make                 balrogg and tsv2wav
#  make tsv2wav-vorbis  the libvorbis-assisted tsv2wav (smaller meta, see META.md)
#  make test            round-trip 00.ogg through both tools and compare
#  make test TESTFILES=path/to/dir     round-trip every .ogg in a directory
#
#  CXXFLAGS is yours to set; the flags the build actually needs are kept
#  separate so that `make CXXFLAGS=-O0 -g` does not silently drop -Wall or the
#  -D's the vorbis build cannot compile without.

#  `CXX ?= c++` would not fire: make defines CXX=g++ as a built-in, so ?= sees
#  it as already set and a clang-only host cannot build.  A plain = still loses
#  to `make CXX=clang++` on the command line, which is what we want.
CXX       = c++
CXXFLAGS ?= -O2
WARN     ?= -Wall -Wextra
LIBS      = -lm

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

balrogg: main.cpp $(BLR_INC)
	$(CXX) $(CXXFLAGS) $(WARN) -o $@ main.cpp

tsv2wav: tsv2wav.cpp $(W2V_INC)
	$(CXX) $(CXXFLAGS) $(WARN) -o $@ tsv2wav.cpp $(LIBS)

tsv2wav-vorbis: tsv2wav.cpp $(VRB_INC) $(VRB_SRC)
	$(CXX) $(CXXFLAGS) $(WARN) $(VRB_FLAGS) -o $@ tsv2wav.cpp $(LIBS)

test: balrogg tsv2wav
	./t.sh $(TESTFILES)

test-vorbis: balrogg tsv2wav-vorbis
	TSV2WAV=./tsv2wav-vorbis ./t.sh $(TESTFILES)

clean:
	rm -f balrogg tsv2wav tsv2wav-vorbis
	rm -f 00.tsv 00.tsr 00.wav 00.meta 00.ogr

.PHONY: all test test-vorbis clean
