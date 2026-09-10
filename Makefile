#  make            balrogg and oggcomp
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

#  The walk, shared by both; balrogg puts a record-stream sink under it and
#  oggcomp the model.
WALK_INC = common.inc ogg_page.inc vb_info.inc vb_book.inc \
          vb_floor.inc vb_res.inc vb_map.inc vb_setup.inc vb_ctx.inc \
          oc_sink.inc io.inc source.inc link_walk.inc codec.inc
BLR_INC = $(WALK_INC) tsv.inc oc_tsv.inc
CMP_INC = $(WALK_INC) sh_v2f.inc cm.inc sh_mapping.inc tc_prior.inc oc_model.inc \
          MOD/tsvcomp-dig_h.inc MOD/tsvcomp-dig_p.inc \
          MOD/tsvcomp-sgn_h.inc MOD/tsvcomp-sgn_p.inc \
          MOD/tsvcomp-flr_h.inc MOD/tsvcomp-flr_p.inc \
          MOD/tsvcomp-cls_h.inc MOD/tsvcomp-cls_p.inc \
          MOD/tsvcomp-aux_h.inc MOD/tsvcomp-aux_p.inc \
          MOD/tsvcomp-hdr_h.inc MOD/tsvcomp-hdr_p.inc

all: balrogg oggcomp

.flags: FORCE
	@echo '$(CXX)|$(CXXFLAGS)|$(WARN)|$(REQ)|$(LIBS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp
FORCE:

balrogg: main.cpp $(BLR_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ main.cpp

oggcomp: oggcomp.cpp $(CMP_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ oggcomp.cpp $(LIBS)

clean:
	rm -f balrogg oggcomp .flags .flags.tmp

.PHONY: all clean FORCE
