# test_vectors/cdd64 — golden XP10 / CDDIP test streams

Snapshot of `project-zipline/dv/CDD_64/tests/` (MIT licensed, Microsoft /
Broadcom). One file triple per test:

    <name>.inbound    AXI4-Stream beat text, the input to `xp10 d`
    <name>.outbound   AXI4-Stream beat text, the expected output
    <name>.config     optional APB config sequence applied before driving

Files are kept as the upstream stores them: Latin-1 with CRLF line endings.
The Makefile's `check` target iconv's them to UTF-8 / LF on the fly.

## Tests (per upstream README)

    cfh_4k                   compound command, 3 frames @ 4 KiB
    cfh_8k                   compound command, 3 frames @ 8 KiB
    gzip                     single-frame GZIP decode
    smoke                    single-frame CDDIP smoke (large, 148 KiB)
    xp10                     single-frame XP10 decode
    xp10_prefix              XP10 with prefix selected by Prefix Attach
    xp10_prefix_w_huffman    XP10 prefix + Huffman
    xp10_user_prefix         XP10 with user-supplied prefix
    xp9                      single-frame XP9 decode
    zlib                     single-frame ZLIB decode

## Running

From the repo root:

    make xp10 RTL_ROOT=.../project-zipline/rtl    # build decoder
    make check                                    # uses test_vectors/cdd64 by default
    make check TDIR=/elsewhere                    # override

Or directly:

    ./xp10 d test_vectors/cdd64/xp10.inbound out.beats \
              test_vectors/cdd64/xp10.config -r
    python3 raw_extract.py test_vectors/cdd64/xp10.outbound > xp10.gold
    cmp out.beats.raw xp10.gold && echo BIT-EXACT
