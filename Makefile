#  make            balrogg and tsvcomp
#  make clean
#
#  MOD/ ships generated (shipping/Const form) from the six IDX/tsvcomp-*.idx
#  modules; a stale MOD/ compiles fine and codes differently.  ../mk.sh rebuilds it.
CXX       = c++
CXXFLAGS ?= -O2
WARN     ?= -Wall -Wextra
LIBS      = -lm
#  Not optional: lift.inc's integer butterflies rely on wrapping arithmetic.
REQ       = -fwrapv

BLR_INC = common.inc tsv.inc ogg_page.inc vb_info.inc vb_book.inc \
          vb_floor.inc vb_res.inc vb_map.inc vb_setup.inc vb_ctx.inc \
          io.inc source.inc link_walk.inc codec.inc
CMP_INC = common.inc tsv.inc lift.inc floor_db.inc imdct.inc vd_setup.inc \
          sh_v2f.inc cm.inc sh_mapping.inc \
          MOD/tsvcomp-dig_h.inc MOD/tsvcomp-dig_p.inc \
          MOD/tsvcomp-sgn_h.inc MOD/tsvcomp-sgn_p.inc \
          MOD/tsvcomp-flr_h.inc MOD/tsvcomp-flr_p.inc \
          MOD/tsvcomp-cls_h.inc MOD/tsvcomp-cls_p.inc \
          MOD/tsvcomp-aux_h.inc MOD/tsvcomp-aux_p.inc \
          MOD/tsvcomp-hdr_h.inc MOD/tsvcomp-hdr_p.inc

all: balrogg tsvcomp

.flags: FORCE
	@echo '$(CXX)|$(CXXFLAGS)|$(WARN)|$(REQ)|$(LIBS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp
FORCE:

balrogg: main.cpp $(BLR_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ main.cpp

tsvcomp: tsvcomp.cpp $(CMP_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ tsvcomp.cpp $(LIBS)

clean:
	rm -f balrogg tsvcomp .flags .flags.tmp

.PHONY: all clean FORCE
