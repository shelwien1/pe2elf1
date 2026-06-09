# Makefile — Project Zipline XP10 <-> Verilator (C++)
#
#   make run       # DECODE: verilate cr_xp10_decomp + build/run C++ TB (xp10_decomp.vcd)
#   make run-enc   # ENCODE: verilate cr_huf_comp (Huffman) + build/run C++ TB (huf_comp.vcd)
#   make lint-lz77 # ENCODE: LZ77 engine (needs >=8GB RAM to elaborate; -Wno-MODDUP)
#   make clean
#
# RTL_ROOT must point at the cloned repo's rtl/ directory.

RTL_ROOT ?= $(CURDIR)/project-zipline/rtl
OVERLAY  := $(CURDIR)/overlay      # patched copies, searched before stock RTL

# -Wno-BLKANDNBLK is SAFE for cr_xp10_decomp (disjoint struct fields).
WAIVERS = -Wno-fatal -Wno-BLKANDNBLK -Wno-UNOPTFLAT -Wno-WIDTH -Wno-WIDTHCONCAT \
          -Wno-CASEINCOMPLETE -Wno-CASEOVERLAP -Wno-UNSIGNED -Wno-CMPCONST \
          -Wno-MULTIDRIVEN -Wno-LATCH -Wno-SELRANGE -Wno-IMPLICIT -Wno-SIDEEFFECT \
          -Wno-ASCRANGE -Wno-ALWCOMBORDER -Wno-COMBDLY -Wno-MODDUP
VFLAGS  = --cc -sv --timing --trace $(WAIVERS) -j 4 -y $(OVERLAY)
# codec: no waveform tracing, optimized -> ~10x faster sim
CVFLAGS = --cc -sv --timing $(WAIVERS) -j 4 -y $(OVERLAY) -CFLAGS "-O2" --x-assign fast --x-initial fast

.PHONY: run run-enc lint-lz77 clean

# ---- DECODE ----
xp10_decomp.f:
	./gen_flags.sh "$(RTL_ROOT)" "$(RTL_ROOT)/cr_cddip/cr_cddip.vlist" $@ \
	    "$(RTL_ROOT)/cr_xp10_decomp/cr_xp10_decomp.v"
run: xp10_decomp.f tb_xp10_decomp.cpp zipline_axi.h
	verilator $(VFLAGS) --exe --build --Mdir obj_dec \
	    --top-module cr_xp10_decomp -f xp10_decomp.f tb_xp10_decomp.cpp -o tb
	./obj_dec/tb

# ---- ENCODE (Huffman entropy stage) ----
huf.f:
	./gen_flags.sh "$(RTL_ROOT)" "$(RTL_ROOT)/cr_cceip_64/cr_cceip_64.vlist" $@ \
	    "$(RTL_ROOT)/cr_huf_comp/cr_huf_comp.v"
run-enc: huf.f tb_huf_comp.cpp zipline_axi.h
	verilator $(VFLAGS) --exe --build --Mdir obj_enc \
	    --top-module cr_huf_comp -f huf.f tb_huf_comp.cpp -o tb
	./obj_enc/tb

# ---- ENCODE (LZ77 match stage) — large; elaborate on a >=8GB host ----
lz77.f:
	./gen_flags.sh "$(RTL_ROOT)" "$(RTL_ROOT)/cr_cceip_64/cr_cceip_64.vlist" $@ \
	    "$(RTL_ROOT)/cr_lz77_comp/cr_lz77_comp.v"
lint-lz77: lz77.f
	verilator --lint-only -sv --timing $(WAIVERS) -y $(OVERLAY) \
	    --top-module cr_lz77_comp -f lz77.f

clean:
	rm -rf obj_* *.f *.vcd ./xp10 ./xp10-enc

# ======================= file codec CLI (xp10) =======================
# DECODE tool — validated bit-exact here against dv/CDD_64/tests.
#   ./xp10 d in.beats out.beats [in.config] [-r]
xp10: cddip.f xp10.cpp
	verilator $(CVFLAGS) --exe --build --Mdir obj_xp10 \
	    --top-module cr_cddip -f cddip.f xp10.cpp -o xp10
	@cp obj_xp10/xp10 ./xp10

cddip.f:
	./gen_flags.sh "$(RTL_ROOT)" "$(RTL_ROOT)/cr_cddip/cr_cddip.vlist" $@ \
	    "$(RTL_ROOT)/cr_cddip/cr_cddip.v"

# ENCODE tool — drives cr_cceip_64; the full compress pipeline (LZ77) needs a
# host with >=16 GB RAM to verilate. Code-complete; validate against
# dv/CCE_64/tests. Same beat/TLV I/O format as decode.
cce.f:
	./gen_flags.sh "$(RTL_ROOT)" "$(RTL_ROOT)/cr_cceip_64/cr_cceip_64.vlist" $@ \
	    "$(RTL_ROOT)/cr_cceip_64/cr_cceip_64.v"
xp10-enc: cce.f xp10.cpp
	verilator $(CVFLAGS) -CFLAGS -DENCODE_BUILD --exe --build --Mdir obj_xp10enc \
	    --top-module cr_cceip_64 -f cce.f xp10.cpp -o xp10-enc
	@cp obj_xp10enc/xp10-enc ./xp10-enc

# Run the full decoder regression against the repo's golden vectors.
# Defaults to the in-tree test_vectors/cdd64/ snapshot so this works without
# the upstream project-zipline clone; override TDIR= to point elsewhere.
TESTS = smoke xp10 gzip zlib xp9 cfh_4k cfh_8k xp10_prefix xp10_prefix_w_huffman xp10_user_prefix
TDIR ?= $(CURDIR)/test_vectors/cdd64
check: xp10
	@for t in $(TESTS); do \
	  iconv -f ISO-8859-1 -t UTF-8 $(TDIR)/$$t.inbound | tr -d '\r' > /tmp/$$t.in; \
	  cfg=; [ -f $(TDIR)/$$t.config ] && { iconv -f ISO-8859-1 -t UTF-8 $(TDIR)/$$t.config|tr -d '\r'>/tmp/$$t.cfg; cfg=/tmp/$$t.cfg; }; \
	  timeout 240 ./xp10 d /tmp/$$t.in /tmp/$$t.ob $$cfg -r >/dev/null 2>&1; \
	  python3 raw_extract.py $(TDIR)/$$t.outbound > /tmp/$$t.gold 2>/dev/null; \
	  if cmp -s /tmp/$$t.ob.raw /tmp/$$t.gold; then echo "  PASS  $$t ($$(wc -c </tmp/$$t.gold) bytes)"; \
	  else echo "  FAIL  $$t"; fi; \
	done
