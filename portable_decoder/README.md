# Portable XP10 decoder (no Verilator install needed)

This tree is a self-contained snapshot of `xp10` (decode side) that builds
with only a C++17 compiler. The RTL has already been translated to C++ by
`verilator --cc -sv --timing` upstream; you don't need Verilator here.

## Build

    make            # -> ./xp10
    make clean

The Makefile invokes only `$(CXX)` (defaults to `g++`) and links with
`-pthread -latomic`. No `verilator` binary is touched.

## Run

Same CLI as the in-tree build at the repo root:

    ./xp10 d input.beats output.beats [input.config] [-r]

I/O is the AXI4-Stream beat text format used by the project-zipline
`dv/CDD_64/tests/*.inbound|outbound` vectors. `-r` also writes the raw
decompressed payload bytes to `<output>.raw`.

## What's inside

    xp10.cpp                user CLI driver (decode path)
    zipline_axi.h           AXI4-Stream + RBUS field accessors
    generated/              Verilator output (`--cc` top: cr_cddip) -- 75 files,
                            the C++ that `obj_xp10/` would contain after
                            `verilator ... -f cddip.f xp10.cpp`
    verilator_include/      copy of /usr/share/verilator/include from the
                            Verilator 5.020 Debian package; only verilated.cpp
                            and verilated_threads.cpp are actually linked,
                            the rest are header dependencies of those two

## Provenance

Generated with: Verilator 5.020 (Debian 5.020-1), top module `cr_cddip`,
flags `--cc -sv --timing -CFLAGS "-O2" --x-assign fast --x-initial fast`
plus the overlay patches from `../overlay/`. Validates bit-exact against
all 10 `dv/CDD_64/tests/*.outbound` golden vectors (see top-level README).

## Limitations

* This is a one-shot snapshot — if you change the RTL, the overlay, or the
  Verilator flags, you have to re-elaborate with Verilator and regenerate
  `generated/`. The build here only re-runs g++.
* Decoder only. The encoder elaboration is much larger and lives in its
  own tree (see top-level README for status).
