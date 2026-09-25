# tsvcomp: the targets t0, t1 of a quantize TSV, with everything else as context

`tsvcomp` compresses the target columns `t0` and `t1` of a `connectome/quantize` TSV. The other
columns go to a separate TSV, and the decoder reads them as context. Those columns are the
predictions `pred_t0`/`pred_t1`, `need_prediction` and the features. The model is built from
coder0's parts (`../sh_counter.inc`, `../sh_mix2.inc`, `../sh_SSE.inc`, `../config*.hpp`), with
tsvcomp's own contexts in `IDX/tc_model-*.idx`. The compressed size measures how much the
predictions and features tell about the targets.

```sh
./gc.sh                                       # shipping build -> ./tsvcomp
./gc.sh tune                                  # tuning build   -> ./tsvcompt

./tsvcomp c  input.tsv output.tsv output.rc   # compress; output.tsv = input without t0, t1
./tsvcomp d  output.tsv output.rc restored.tsv
./tsvcomp c  input.tsv output.rc              # compress without writing output.tsv (for IDX/opt.pl)
./tsvcomp tc input.tsv output.tsv cols.tsv    # t0, t1 as text, no header
./tsvcomp td output.tsv cols.tsv restored.tsv
./tsvcomp bc input.tsv output.tsv cols.bin    # t0, t1 as int32 LE pairs, no header
./tsvcomp bd output.tsv cols.bin restored.tsv
```

`t0`/`t1` are found by name; anything after a `/` in the header is the quantize scale.
`restored.tsv` is byte-identical to `input.tsv`, and the targets must be canonical int32
decimals. The `.rc` stream stores the two columns' names and positions. `cols.tsv`/`cols.bin`
store neither, so `td`/`bd` put the targets back as the last two columns, which is where
`quantize` writes them. They are named `t0`/`t1` with the scale suffix of `output.tsv`'s first
scaled column.

## Model

Each target v of column k is coded row by row, `t0` before `t1`, as a delta
d = v − (previous row's value of column k):

- zero flag `[d == 0]`
- sign
- exponent e = ⌊log2 |d|⌋, in unary
- the e mantissa bits below the leading 1, MSB first, each in the context of the bits above it.
  This is a bit tree, so each sequence's jump sizes (multiples of its target step) are learned
  exactly.

Every bit goes through `p_n = mix2( mix2( C0, C1 ), C2 )` and then `p = mix2'( p_n, SSE(p_n) )`.
The end-to-end gradients are chained back through every path. Build with
`CXXEXTRA=-DFINAL_MIX=0` for `p = SSE( p_n )` instead.

| stage | context (`IDX/tc_model-*.idx`) |
|---|---|
| C0 | hash(need_prediction, k, bit node, exponent of the previous delta), 14 bits × rows since column k last changed (0/1/2/3+) × side-info change flags (i0 price, i0 volume, i0 trade columns changed since the previous row) |
| C1 | hash(k, bit node, column k's last nonzero delta, the other column's delta), 19 bits |
| C2 | hash(k, bit node, `pred_tk` and the current target level in linear buckets of 1024 clipped to ±15, the change of `pred_tk` since the previous row), 19 bits |
| M0, M1 | k × stage (zero / sign / exponent / mantissa) × exponent (step) |
| M2 | k × stage × (`pred_tk` − current level) as a sign + exponent bucket |
| S0 | k × stage × rows since the last change |

The counter, mixer and SSE knobs are copies of coder0's tuned values; nothing is tuned for
this data yet. `-DPCX=n` sets the bucket width of C2 to 2^n (0 = sign + exponent of
prediction − level).

## Results (10 connectome sequences, `-d4`; `./mkcorpus.sh` builds them)

The same targets with three kinds of predictions in `pred_t0`/`pred_t1`:

| bytes, t0+t1 of all 10 files | pz: zeros | pd: dummy (`predict -m0`) | pb: GRU baseline |
|---|---:|---:|---:|
| **tsvcomp** (mix2(p, SSE(p))) | 24,586 | 19,859 | 24,914 |
| tsvcomp `-DFINAL_MIX=0` (SSE(p)) | **24,313** | **19,468** | **24,532** |
| xz -9e, cols.bin | 42,032 | 42,032 | 42,032 |
| xz -9e, cols.tsv | 51,828 | 51,828 | 51,828 |
| coder0, cols.bin | 64,916 | 64,916 | 64,916 |
| raw cols.bin | 1,600,000 | 1,600,000 | 1,600,000 |

The dummy prediction, which is the raw features `i0_p3` and `-i0_p20`, saves about 20% over zeros. The GRU
predictions, although they score WP 0.645 against 0.441, add almost nothing in this model. That
held for every prediction bucketing tried: sign + exponent of prediction − level, linear
buckets of 2^10..2^13, and scale-free buckets relative to a running mean. Most of the cost is
in *when* a target changes and by how much. The GRU tracks the target's level, which the
target's own history already gives.

About 1 s per file. All 30 files round-trip in every mode, and the shipping and tuning builds
produce identical streams.

## Tuning

```sh
./mkcorpus.sh                 # corpus/ (needs ../connectome built and its data unpacked)
./gc.sh tune
cp tsvcompt tsvcompt.tune
perl ../IDX/opt.pl opt.lst ./tsvcompt.tune     # or ../IDX/optv.pl; results in export.!!!
cd IDX && for f in *.idx; do perl ../../IDX/import.pl $f ../export.!!! > t && mv t $f; done
```
