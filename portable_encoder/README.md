# Portable XP10 encoder (no Verilator install needed)

Self-contained encoder source tree, parallel to `portable_decoder/`. The
RTL has already been translated to C++ by Verilator upstream; you don't
need Verilator here -- but you do need a beefy host to actually compile
the result.

## Why this tree is different from portable_decoder

The decoder ships its 75 `.cpp` files raw in `generated/`. The encoder's
Verilator output is 1803 `.cpp` files totalling ~3.5 GB; two of them are
~890 MB each (Verilator unrolls the LZ77 / Huffman state machines). So
this tree ships a single 25 MB `generated.tar.xz` and unpacks on demand:

    portable_decoder/         portable_encoder/
    -----------------         -----------------
    generated/*.cpp           generated.tar.xz       (25 MB, xz -1)
                              generated.cpp.list     (1803 file manifest)
                              generated.h.list       (52 file manifest)
                              <make unpack>          -> generated/  (~3.5 GB)

## Build

    make unpack             # extract generated/  (~3.5 GB on disk)
    make                    # -> ./xp10-enc (slow, large RAM)
    make clean              # rm .o, xp10-enc, generated/

Defaults to `-O0` because the largest two `.cpp` files OOM g++ at any
optimisation level on machines under ~12 GB RAM. With `-O0 -j2` a
16 GB host should finish in roughly 30-60 minutes; with `-O2 -j4`
expect to need 32 GB+.

This tree was assembled in the same session that built `portable_decoder/`
but the encoder was **not** compiled here -- the elaboration step (the
expensive part Verilator does) succeeded, the g++ compile step exceeded
the sandbox's RAM with `-O2 -j4`. Sources are shipped so a larger host
can pick up the compile.

## What's inside

    xp10.cpp                user CLI driver (same file as decoder; the
                            encode path is gated by -DENCODE_BUILD)
    zipline_axi.h           AXI4-Stream + RBUS field accessors
    generated.tar.xz        xz -1 archive of obj_xp10enc/*.{cpp,h,mk}
                            from `verilator --cc -sv --timing
                            --top-module cr_cceip_64 ...`
    generated.cpp.list      manifest: every .cpp file inside the tarball
    generated.h.list        manifest: every .h file inside the tarball
    verilator_include/      copy of /usr/share/verilator/include from the
                            Verilator 5.020 Debian package
    Makefile                two-step build: unpack -> g++ compile + link

## Provenance

Same Verilator command as the in-tree build at the repo root:

    verilator --cc -sv --timing --x-assign fast --x-initial fast \
              -CFLAGS "-O2" -CFLAGS -DENCODE_BUILD \
              --top-module cr_cceip_64 -f cce.f xp10.cpp -o xp10-enc

Overlay patches applied (see `../overlay/`):

* nx_ram_1r1w_indirect_access.v -- `{field:...}` -> `'{field:...}` (SV apostrophe)
* nx_reg_indirect_access.v      -- BLKLOOPINIT loop -> genvar generate
* cr_prefix_fe_counter.v        -- covergroup hidden behind `\`ifndef VERILATOR`
* cr_prefix_fe_cmpx4.v          -- ditto
* cr_prefix_rec_act.v           -- ditto
* cr_prefix_rec_alu.v           -- ditto
* cr_prefix_rec_do.v            -- ditto
* cr_prefix_rec_us.v            -- ditto
* (plus the existing decoder overlays, all of which apply here too)
