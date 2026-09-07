CXX      ?= c++
CXXFLAGS ?= -O2 -Wall -Wextra
LIBS      = -lm

BLR_INC = common.inc tsv.inc ogg_page.inc vb_info.inc vb_book.inc \
          vb_floor.inc vb_res.inc vb_map.inc vb_setup.inc vb_ctx.inc \
          io.inc source.inc link_walk.inc codec.inc
W2V_INC = common.inc tsv.inc lift.inc floor_db.inc imdct.inc vd_setup.inc \
          wav.inc vd_dec.inc vd_ana.inc vd_rec.inc

all: balrogg tsv2wav

balrogg: main.cpp $(BLR_INC)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

tsv2wav: tsv2wav.cpp $(W2V_INC)
	$(CXX) $(CXXFLAGS) -o $@ tsv2wav.cpp $(LIBS)

clean:
	rm -f balrogg tsv2wav

.PHONY: all clean
