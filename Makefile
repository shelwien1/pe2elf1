#  make            oggcomp
#  make clean
#
#  MOD/ ships generated (shipping/Const form) from the six IDX/tsvcomp-*.idx
#  modules; a stale MOD/ compiles fine and codes differently.  ../mk.sh rebuilds it.
CXX       = c++
CXXFLAGS ?= -O2
WARN     ?= -Wall -Wextra
LIBS      = -lm
#  Not optional: the model relies on wrapping arithmetic where a mixer
#  update overflows (cm.inc), and the stream must not depend on the compiler.
REQ       = -fwrapv

#  The walk, and the model under it.
WALK_INC = common.inc ogg_page.inc vb_info.inc vb_book.inc \
          vb_floor.inc vb_res.inc vb_map.inc vb_setup.inc vb_ctx.inc \
          oc_sink.inc io.inc source.inc link_walk.inc codec.inc
#  Lib3's coroutine and file API, which oggcomp streams through.
CORO_INC = Lib3/coro3b.inc Lib3/coro3_pin.inc Lib3/coro3_pin_0.inc \
          Lib3/coro3_setjmp_x64.h Lib3/coro3_setjmp_x64d.h Lib3/coro3_setjmp_x32.h \
          Lib3/coro_fhp2.inc Lib3/file_api.inc Lib3/file_api_std.inc
CMP_INC = $(WALK_INC) $(CORO_INC) rc.inc cm.inc sh_mapping.inc tc_prior.inc oc_model.inc \
          MOD/tsvcomp-dig_h.inc MOD/tsvcomp-dig_p.inc \
          MOD/tsvcomp-sgn_h.inc MOD/tsvcomp-sgn_p.inc \
          MOD/tsvcomp-flr_h.inc MOD/tsvcomp-flr_p.inc \
          MOD/tsvcomp-cls_h.inc MOD/tsvcomp-cls_p.inc \
          MOD/tsvcomp-aux_h.inc MOD/tsvcomp-aux_p.inc \
          MOD/tsvcomp-hdr_h.inc MOD/tsvcomp-hdr_p.inc

all: oggcomp

.flags: FORCE
	@echo '$(CXX)|$(CXXFLAGS)|$(WARN)|$(REQ)|$(LIBS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp
FORCE:

oggcomp: oggcomp.cpp $(CMP_INC) .flags
	$(CXX) $(CXXFLAGS) $(WARN) $(REQ) -o $@ oggcomp.cpp $(LIBS)

clean:
	rm -f oggcomp .flags .flags.tmp

.PHONY: all clean FORCE
