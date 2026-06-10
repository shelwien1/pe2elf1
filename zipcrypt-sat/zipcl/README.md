# zipcl (vendored, with CBMC model-generation)

`zipcl` is Shelwien's ZipCrypto re-crypt tool, vendored here from the
[`claude/great-ride-ohbguu`](https://github.com/shelwien1/pe2elf1/tree/claude/great-ride-ohbguu)
branch and extended with a `g` mode that emits CBMC key-recovery models (see
[`../zipcrypt.md`](../zipcrypt.md)). `CHANGES.patch` is the diff of that addition
against the upstream branch (`zipcl.cpp`, `zipdump.inc`).

## Build

```sh
./build.sh            # -> ./zipcl   (g++, Linux)
```

## Original modes (raw key = the three 32-bit registers, no password)

```sh
./zipcl d basis.pak out.zip pads a3e30892 f9185194 eb474b09   # decrypt
./zipcl c in.zip out.pak pads a3e30892 f9185194 eb474b09      # encrypt
```

When decrypting it prints, per file, `pad[8..11] crc32 (date|time) uSize cSize`
followed by the first 12 decrypted body bytes as LSB-first bit-strings — the
known-plaintext the SAT models are built from.

## Model-generation mode `g`

```
./zipcl g <archive> <model.cpp> <nunk> <k0> <k1> <k2> [nfiles] [usecheck] [reverse] [deflevel]
```

- `nunk`    — number of low bits of `k0` to leave unknown (the SAT variables).
- `k0 k1 k2`— the known key; its low `nunk` bits are blanked and recovered.
- `nfiles`  — how many archive entries to turn into constraints (default 8).
- `usecheck`— 1 (default) also constrains the CRC check byte; 0 = deflate bits only.
- `reverse` — 0 (default) forward model; 1 = reverse model (cipher run backward
  from the body to the header, recovered init must equal the known key).
- `deflevel`— deflate-header constraints (forward only): 0 (default) BFINAL|BTYPE;
  1 adds the `nlen≤286`/`ndist≤30` validity ranges; 2 also adds the code-length
  code completeness (Kraft==128) — the strong one (see `../zipcrypt.md` §8).

Examples (matching the committed `../models/`):

```sh
./zipcl g basis.pak model8.cpp               8  a3e30892 f9185194 eb474b09 2 1 0 0
./zipcl g basis.pak model16.cpp              16 a3e30892 f9185194 eb474b09 3 1 0 0
./zipcl g basis.pak model8_deflate.cpp       8  a3e30892 f9185194 eb474b09 6 0 0 0
./zipcl g basis.pak model8_deflate_kraft.cpp 8  a3e30892 f9185194 eb474b09 1 0 0 2
./zipcl g basis.pak rev8.cpp                 8  a3e30892 f9185194 eb474b09 1 1 1
./zipcl g basis.pak rev16.cpp                16 a3e30892 f9185194 eb474b09 2 1 1
```

Then solve with [`../solve_zipcrypt.sh`](../solve_zipcrypt.sh).
