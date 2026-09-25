# tsvcomp: the prediction/target columns of a quantize TSV

`tsvcomp` compresses the first two integer columns of a TSV, such as `pred_t0`/`pred_t1` in
`connectome/quantize` output. The remaining columns become a separate TSV, and the decoder uses
them as side information. It is built from coder0's model (`../sh_counter.inc`,
`../sh_mix2.inc`, `../sh_SSE.inc`, `../config*.hpp`). The contexts are tsvcomp's own, declared in
`IDX/tc_model-*.idx`.

```sh
./gc.sh                                       # shipping build -> ./tsvcomp
./gc.sh tune                                  # tuning build   -> ./tsvcompt

./tsvcomp c  input.tsv output.tsv output.rc   # compress; output.tsv = input without the first two columns
./tsvcomp d  output.tsv output.rc restored.tsv
./tsvcomp c  input.tsv output.rc              # compress without writing output.tsv (for IDX/opt.pl)
./tsvcomp tc input.tsv output.tsv cols.tsv    # the two columns as text, no header
./tsvcomp td output.tsv cols.tsv restored.tsv
./tsvcomp bc input.tsv output.tsv cols.bin    # the two columns as int32 LE pairs, no header
./tsvcomp bd output.tsv cols.bin restored.tsv
```

`restored.tsv` is byte-identical to `input.tsv`. The two columns must hold canonical int32
decimals. `cols.tsv`/`cols.bin` keep no header: `td`/`bd` name the columns `pred_t0`/`pred_t1`
and add the scale suffix of the first scaled column in `output.tsv` (e.g. `/10000`), which is
how `quantize` names them. The `.rc` stream stores the two names.

## Model

Each value v of column k is coded row by row, `t0` before `t1`, as a delta
d = v − (previous row's value of column k):

- zero flag `[d == 0]`
- sign
- exponent e = ⌊log2 |d|⌋, in unary
- the e mantissa bits below the leading 1, MSB first, each in the context of the bits above it.
  This is a bit tree, so recurring jump sizes (e.g. multiples of a sequence's target step)
  are learned exactly.

Every bit goes through `p = mix2'( p_m, SSE(p_m) )`, with `p_m = mix2( C0, C1 )`. The
end-to-end gradients are chained through both paths from `p_m`. Build with
`CXXEXTRA=-DFINAL_MIX=0` for coder0's `p = SSE( p_m )` instead.

| stage | context (`IDX/tc_model-*.idx`) |
|---|---|
| C0 | hash(need_prediction, k, bit node, exponent of the previous delta), 14 bits × rows since column k last changed (0/1/2/3+) × side-info change flags (i0 price, i0 volume, i0 trade columns changed since the previous row) |
| C1 | hash(k, bit node, column k's last nonzero delta, the other column's delta as zero/sign/exponent), 19 bits |
| M0, M1 | k × stage (zero / sign / exponent / mantissa) × exponent (step) |
| S0 | k × stage × rows since the last change |

The counter, mixer and SSE knobs are copies of coder0's tuned values; nothing is tuned for
this data yet.

## Results (10 connectome sequences, 20,000 rows each; `./mkcorpus.sh` builds them)

- `qb-*`: `quantize` output with the GRU baseline predictions (`-d4`).
- `qx-*`: the true targets in `pred_t0`/`pred_t1`, with the `t0`/`t1` columns dropped from the
  side columns. This measures how compressible the targets are given the features.

| bytes, all 10 files | qb (GRU predictions) | qx (targets) |
|---|---:|---:|
| **tsvcomp** (mix2(p, SSE(p))) | **694,514** | 27,079 |
| tsvcomp `-DFINAL_MIX=0` (SSE(p)) | 699,971 | **26,877** |
| xz -9e, cols.bin | 817,448 | 42,016 |
| xz -9e, cols.tsv | 925,672 | 51,712 |
| gzip -9, cols.bin | 1,053,843 | 52,398 |
| coder0, cols.bin | 840,343 | 64,316 |
| coder0, cols.tsv | 905,826 | 72,792 |
| raw cols.bin | 3,200,000 | 3,200,000 |

About 1 s per file. All 20 files round-trip, and the shipping and tuning builds produce identical
streams.

## Tuning

```sh
./mkcorpus.sh                 # corpus/ (needs ../connectome built and its data unpacked)
./gc.sh tune
cp tsvcompt tsvcompt.tune
perl ../IDX/opt.pl opt.lst ./tsvcompt.tune     # or ../IDX/optv.pl; results in export.!!!
cd IDX && for f in *.idx; do perl ../../IDX/import.pl $f ../export.!!! > t && mv t $f; done
```
