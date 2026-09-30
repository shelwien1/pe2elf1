# dicrank3: dicrank2 with a built-in entropy coder

`dicrank2` (`../dicrank2.cpp`) turns english.dic into small numbers and
bitmaps, and writes them as a text file for cmix to compress. It has no
entropy coder of its own. `dicrank3` uses the same transform, unchanged, but
codes those numbers itself with a context-mixing coder built from the
tsvcomp coder's components. Its contexts are things cmix never sees in the
text file:

- word statistics: case, length, document frequency, count, suffixes;
- candidate scores and the margins between them;
- run statistics: size, recency, typical fit, and how many members share the
  word's suffix, case class, length class and count class.

The side file needs no further compression. `dicrank2` stays as it is.

| english.dic from enwik9 | dicrank2 + cmix | dicrank3 |
|---|---:|---:|
| Explicit words: set | 158 | 40 |
| Explicit words: order | 3,595 | 3,532 |
| Membership of the other words | 2,546 | 1,498 |
| Run numbers | 18,908 | 16,636 |
| Header | (in the file) | 12 |
| **Side file** | **25,112** | **21,718** |

The dicrank2 sections were compressed separately. The whole dicrank2 file,
compressed as one, is 25,112 bytes. The coder's knobs are partly tuned:
with the values copied from tsvcomp, the side file is 22,140 bytes (see
Tuning below).

**The catch.** In fx2-cmix the decoder is part of the program, which is
stored twice. The coder's code costs far more than the 3.4 KB it saves.
[../../docs/english_dic_reconstruction.md](../../docs/english_dic_reconstruction.md)
§9 has the measurements.

## Build and use

```sh
./build.sh                       # dicrank3 and dr3_tune (needs perl for MOD/; MOD/ is also committed)
g++ -O2 -std=c++17 -ffp-contract=off -o dicrank3 dicrank3.cpp    # the same, without perl

./dicrank3 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] [-D DIR] enwik9 english.dic dict3.rank
./dicrank3 d [-v] enwik9 dict3.rank english.dic
```

The options are dicrank2's (see [../dicrank2.md](../dicrank2.md)), plus
`-D DIR`, which dumps the coder's inputs for `dr3_tune`.

- **Self-check.** The encoder decodes its own output and fails unless it
  gets the dictionary back byte for byte.
- **Cost on enwik9.** Encode is about 52 s, plus 50 s for the check. Decode
  is about 50 s, of which the coder takes about 14 s. Decoding peaks at
  1.86 GB.

**Side file.** It is binary, and starts with a header of 12 bytes for
english.dic:

| Field | Size | Contents |
|---|---|---|
| Signature | 1 byte | `D3` |
| Parameters | 7 numbers | counted bytes, context bytes, K, S, runs, membership bitmap length, extra words |
| Extra words | 1 byte per letter, plus 1 | each word's letters, ended by a 0 byte |
| Coded streams | the rest | range coder stream: explicit-word set, membership bitmap, explicit-word order, run numbers |

- **Numbers.** Each number is a varint (7 bits per byte, low bits first,
  top bit set means more follow) of 16·m + e, for the value m·10^e with
  e < 16. So a round value like 10^8, 10^9 or 4000 takes one byte.
- **The explicit-word bitmap's length is not stored.** The decoder reads
  bits until it has seen S explicit words, because the bitmap ends with the
  S-th.
- **english.dic's header,** in hex: `D3 18 19 43 81 31 B1 06 F0 E1 41 00`.
  That is 10^8, 10^9, 4000, 3920, 510, 67,343 and 0 extra words.

## The coder (`dr3_model.inc`)

Every binary decision passes through these stages:

1. 28 hashed contexts, each mapped to a `Counter` cell;
2. 7 `MixN` group mixers;
3. a `MixN` final mixer;
4. an interpolated `SSE` stage;
5. a final `Mix2` of the mixer output and the SSE output.

It is then coded by the binary range coder. As in tsvcomp, every stage
learns end to end: the final error is chained back through the mixers into
the counters.

**How each stream is coded.**

- **The two bitmaps.** One decision per bit. The contexts are the word's
  features and the recent bits.
- **Explicit-word order.** Each next word is sorted among the remaining ones
  by dicrank2's score. Its position is coded as yes/no decisions over the
  first 16 candidates, then as an Elias-gamma escape.
- **Run numbers.** A "new run?" flag comes first. A new run's index among
  the runs not started yet is coded as a bit tree. Otherwise yes/no
  decisions run over all started runs, sorted by dicrank2's centroid score.

**Floating point.** The model computes in floats, so the encoder and decoder
must round every operation the same way.

- `dr3_model.inc` refuses to compile with `-ffast-math`.
- It turns off FMA contraction with pragmas for GCC and clang.
- It requires `FLT_EVAL_METHOD == 0`.
- It sets flush-to-zero and denormals-are-zero while coding.
- Every transcendental function is Schraudolph's bit trick (`schrau.inc`),
  not libm.

**Determinism tests.** These builds all produce the same stream, and a clang
`x86-64-v3` build decodes a gcc build's side file exactly:

- g++ `-O1 -fno-tree-vectorize`, `-O2` and `-O3 -march=native`;
- clang `-O3` and `-O2 -march=x86-64-v3`;
- the IDX tuning build.

Without the pragmas, the FMA builds do not match.

## Tuning with the IDX framework

The knobs are declared in `IDX/dr3-*.idx`:

- H0: the hashed counters, and the table size TB (2^17 cells per context);
- X0, X1: the group and final mixers;
- S0: the SSE stage;
- M1: the final Mix2.

`IDX/idx2inc.pl` generates `MOD/` from them. In the release build every knob
is a constant. In the tuning build every knob is an object that `IDX/opt.pl`
finds and patches in the binary. `dr3_tune` codes the dumped streams in
seconds, instead of recomputing the context vectors for a minute:

```sh
./dicrank3 e -D dump enwik9 english.dic dict3.rank     # dump the coder's inputs
./build.sh tune                                        # -> dr3_tune.tune
printf 'dump/sb\ndump/h\ndump/r\n' > opt.lst          # stream sets, measured as separate "files"
OPT_JOBS=3 perl IDX/opt.pl opt.lst ./dr3_tune.tune ['^H0_']
cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done && cd .. && ./build.sh
```

**Checks.**

- `./dr3_tune c dump/sbhr out` writes exactly dicrank3's stream, i.e. the
  side file without its header.
  - The argument is the dump folder, then the letters of the streams to
    code.
  - `/` and `\` both work as separators.
  - Without a folder, the dump is read from the current one.
- `./dr3_tune d dump/sbhr out` decodes it and checks it against the dump.
- `DR3_NOCX=<stream>:<hex mask>` replaces contexts by a constant, for
  ablations.

## Files

| File | What it is |
|---|---|
| `dicrank3.cpp` | The tool. The transform is dicrank2's code, copied unchanged. |
| `dr3_model.inc` | The coder: its contexts and the four stream coders |
| `dr3_tune.cpp` | Codes dumped streams, for tuning |
| `build.sh` | Regenerates `MOD/` and builds |
| `sh_counter.inc`, `sh_pupdater.inc`, `sh_mix2.inc`, `sh_mixN.inc`, `sh_SSE.inc`, `sh_v2f.inc` (range coder), `sh_mapping.inc`, `schrau.inc`, `config.hpp`, `config_mix2.hpp`, `config_mixN.hpp` | Components of the tsvcomp coder, copied unchanged |
| `IDX/idx2inc.pl`, `IDX/opt.pl`, `IDX/import.pl` | The IDX framework's generator, optimizer and importer, copied unchanged |
| `IDX/dr3-*.idx`, `IDX/dr3-*.inc` | dicrank3's knobs and templates. The knob values started as tsvcomp's. |
| `MOD/` | Generated from `IDX/`, in the release state |
