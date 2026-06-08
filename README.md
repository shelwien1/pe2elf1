# Project Zipline XP10 -> C/C++ via Verilator  (+ a working decoder CLI)

Translate the XP10 codec RTL from github.com/opencomputeproject/project-zipline
into cycle-accurate C++ with Verilator, and drive it from a C++ CLI.
Verified with Verilator 5.020 (apt install verilator) + g++.

## The tool

    ./xp10 d input output [config] [-r]      # decode (drives cr_cddip) -- WORKS, bit-exact
    ./xp10 c input output [config] [-r]      # encode (drives cr_cceip_64) -- needs >=16 GB host

I/O is the AXI4-Stream "beat" text format from the repo's dv vectors, one beat
per line:   0x<16 hex tdata>  [SoT|EoT]  0x<2 hex tstrb>     (# = comment).
That is the XP10 on-wire TLV framing; it round-trips and diffs directly against
dv/CDD_64/tests/*.outbound. `-r` also writes the raw decompressed payload bytes
(TLV data frames, type 0x05) to <output>.raw.

    make xp10      RTL_ROOT=.../rtl     # build decoder CLI
    make check     RTL_ROOT=.../rtl     # run the golden-vector regression
    make xp10-enc  RTL_ROOT=.../rtl     # build encoder CLI (large; >=16 GB RAM)

## Decode validation (bit-exact vs the repo's golden vectors)

Payload compared byte-for-byte. The only beat that ever differs is the stats
(type 0x08) counter, which the reference testbench explicitly ignores.

    test                      payload bytes   result
    xp10 / gzip / zlib / xp9        8192       BIT-EXACT
    cfh_4k  (3 frames)             12288       BIT-EXACT
    cfh_8k  (3 frames)             24576       BIT-EXACT
    xp10_prefix                     4096       BIT-EXACT
    xp10_prefix_w_huffman           4096       BIT-EXACT
    xp10_user_prefix                4096       BIT-EXACT
    smoke                         148481       BIT-EXACT

i.e. XP10, GZIP, ZLIB, XP9, compound multi-frame, and all three prefix-attach
modes decode bit-exactly. smoke (148481 bytes / 18570 beats) is also BIT-EXACT.

## How decode works (ported from dv/CDD_64/run/zipline_tb.v)

* Reset rst_n low ~40 cyc with all inputs idle, ob_tready=1, sch_update_tready=1.
* Optional APB config from <name>.config: "r 0xADDR 0xDATA" (read+verify) /
  "w 0xADDR 0xDATA" (write). xp10.config is just a version-register read.
* Feed ib beats honoring ib_tready; tuser bit0=SoT, bit1=EoT; assert ib_tlast on
  the EoT beat of the CQE command frame (tdata[7:0]==0x09).
* Collect ob beats with ob_tready=1; stop at the output CQE completion (tlast).
* Raw payload = bytes of TLV data frames (header type 0x05); other types
  (0x00 request, 0x08 stats, 0x09 completion) are metadata.

## Status of the underlying translations

    engine (top)                role            translates   runs here
    cr_cddip                    decode DMA top  YES          YES (CLI, bit-exact)
    cr_xp10_decomp              XP10 dec core   YES          YES (tb_xp10_decomp)
    cr_huf_comp                 Huffman encode  YES          YES (tb_huf_comp)
    cr_cceip_64                 encode DMA top  YES*         no (OOM: includes LZ77)
    cr_lz77_comp                LZ77 encode     elaborates*  no (OOM ~4 GB)

*Encode elaboration needs >=8-16 GB RAM (the repo's own synth notes assume 16 GB);
it OOM-killed in this 3.9 GB sandbox. The encode CLI is code-complete (same
driver, key_mode=0, identical beat I/O) and builds via `make xp10-enc` on a large
host; validate against dv/CCE_64/tests. Not yet run, so not claimed working.

## Files

    xp10.cpp            the codec CLI (decode + encode paths)
    raw_extract.py      TLV data-frame -> raw payload (used by `make check`)
    gen_flags.sh        VCS .vlist -> Verilator -f flag file (Latin-1 + $RTL_ROOT)
    zipline_axi.h       AXI4-Stream + RBUS field accessors (engine harnesses)
    tb_xp10_decomp.cpp  decoder-core liveness harness
    tb_huf_comp.cpp     Huffman-encoder liveness harness
    Makefile            xp10 / xp10-enc / check / run / run-enc / clean
    overlay/            non-destructive RTL patches, searched before stock via -y

## Overlay (non-destructive RTL patches)

overlay/ holds patched copies; the Makefile adds `-y $(OVERLAY)` BEFORE the stock
dirs so Verilator's library lookup finds them first. Each change is commented.

* cr_mux_32.v -- `wire 0 = t0 | t1;` (wire named 0; output undriven typo) -> `assign o = ...`
* nx_fifo_1r1w_indirect_access_debug_cntrl.v -- `{field:...}` assignment pattern
  missing the SV apostrophe -> `'{field:...}` (the lone cr_cddip blocker).
* cr_huf_comp_is_sorter.v -- BLKLOOPINIT loops -> genvar generate; sym_entry_t
  made `struct packed` (was used in a reduction).
* cr_huf_comp_htb.v -- BLKLOOPINIT loops -> genvar generate; per-element `<= '0` reset.

The decoder otherwise needs only the safe waiver -Wno-BLKANDNBLK (disjoint struct
fields). nx_library RAMs verilate directly (no vendor macros).

## Notes

* .vlist files are Latin-1 with VCS syntax; gen_flags.sh converts + appends
  +libext+.svp so *.svp packages resolve by filename.
* --timing is required (RTL uses event controls).
* LZ77 on a big host also needs -Wno-MODDUP (fwd_therm_gen defined in two files).
