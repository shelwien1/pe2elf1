# tsvcomp: the targets t0, t1 of a quantize TSV, with everything else as context

`tsvcomp` compresses the target columns `t0` and `t1` of a `connectome/quantize` TSV. The other
columns go to a separate TSV, and the decoder reads them as context. Those columns are the
predictions `pred_t0`/`pred_t1`, `need_prediction` and the features. The model combines
coder0's parts (local copies: `sh_counter.inc`, `sh_mix2.inc`, `sh_SSE.inc`, `config*.hpp`)
with a paq-style block of hashed context models (`paq.inc`). tsvcomp's contexts and knobs are in
`IDX/tc_model-*.idx`. The compressed size measures how much the
predictions and features tell about the targets.

```sh
./gc.sh                                       # shipping build -> ./tsvcomp
./gc.sh tune                                  # tuning build   -> ./tsvcompt
                                              # (MOD/ is left in the tuning state, as gc.bat expects)

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

Every bit goes through

```
p_n = mix2( mix2( C0, C1 ), C2 )      coder0's counters and mixers (IDX-tuned)
p_q = mix2( p_n, paq )                the paq-style block, mixed in by M3
p   = mix2( p_q, SSE(p_q) )           (-DFINAL_MIX=0: p = SSE(p_q))
```

Every stage learns end to end: the gradients are chained back through every path, from the
final mixer through the paq block's mixers into its counters, and into C0/C1/C2. Build with
`-DPAQ=0` to leave the paq block out; that gives exactly the output of the version without it.

### paq block (`paq.inc`), built from the framework's parts

- **22 hashed context models.** Each is a hash table of 2^TB `Counter<CP_H0>` cells
  (`sh_counter.inc`, knobs in `IDX/tc_model-H0.idx`), plus a 32-bit check per slot. The slot of
  (the model's context hash for this value, bit node) is the cell that maps the context's
  history to a probability. A slot whose check doesn't match is re-initialized to the prior and
  taken over.
- **Group mixers.** The 22 logits plus C0/C1/C2's are mixed in 5 groups of 5, each by a
  `MixN<CP_X0,5>` cell (`sh_mixN.inc`, `IDX/tc_model-X0.idx`). A group's cell is selected by
  group × k × stage × exponent step. The groups are target history; levels, the other column
  and change flags; price changes; predictions and a0..a7; and the rest with C0/C1/C2.
- **Final mixer.** A `MixN<CP_X1,5>` cell (`IDX/tc_model-X1.idx`) mixes the group outputs. It is
  selected by k × stage × rows since the last change × change flags.
- **Gain (not convex).** MixN's softmax weights sum to 1, so on their own they can never be more
  confident than the most confident input. Each MixN cell therefore also learns a gain:
  `z = e^g · Σ wᵢ sᵢ + b`. g is a `ParamUpdater` state of its own, with box and rates in the
  bundle (`config_mixN.hpp`: `GON`, `G0`, `Glo`/`Ghi`, `M1g` … `NAGg`). The weights stay a
  softmax, so their conditioning and the Gershgorin step bound are unchanged. `GON = 0` gives the
  convex mixer, byte for byte, and at N = 2 that is still Mix2.

| # | context of the model (with k) | # | context of the model (with k) |
|---|---|---|---|
| 0 | order 0 | 11 | which trade columns changed, which of the first 10 volume columns changed |
| 1 | rows since the last change | 12 | prediction and level, buckets of 512 |
| 2 | last jump | 13 | prediction − level (sign + exponent) × rows since the last change |
| 3 | last two jumps | 14 | change of the prediction × change flags |
| 4 | level | 15 | both predictions, buckets of 1024 |
| 5 | both columns' levels | 16 | which i1 price columns changed |
| 6 | other column's delta × last jump | 17 | a0..a7 >> 12 |
| 7 | change flags × rows since the last change | 18 | directions of the i0 price changes × sign of the last jump × rows since the last change |
| 8 | which i0 price columns changed (22-bit mask) | 19 | which i0 volume columns changed |
| 9 | the directions of those price changes | 20 | price-change mask × last jump |
| 10 | rows since an i0 price column changed × rows since the last change | 21 | price directions × level |

`-DNOCX=<mask>` drops the contexts of the models whose bits are set (for ablations).

### Counter/mixer stages (`IDX/tc_model-*.idx`)

| stage | context |
|---|---|
| C0 | hash(need_prediction, k, bit node, exponent of the previous delta) × rows since column k last changed × side-info change flags |
| C1 | hash(k, bit node, column k's last nonzero delta, the other column's delta) |
| C2 | hash(k, bit node, `pred_tk` and the level in linear buckets of 1024, the change of `pred_tk`) |
| M0, M1 | k × stage × exponent (step) |
| M2, M3 | k × stage × (`pred_tk` − level) bucket |
| X0 | group × k × stage × exponent step |
| X1 | k × stage × rows since the last change × change flags |
| S0 | k × stage × rows since the last change |

C0–C2, M0–M2 and S0 hold the user's tuned knobs. The new modules start as copies: M3 of M2,
H0 of C0, X0/X1 of M0 (plus MixN's XW and gain knobs). Of the few settings tried, `XW = 0`
(diagonal steps) and `TB = 17` did best. The process then needs about 450 MB (TB = 16: 314 MB,
18: 727 MB).

The gain starts at 1.64 and learns slowly: NW and STEP are about 1/250 of the bias's. On the
zeros / GRU corpora:

| gain | zeros | GRU |
|---|---:|---:|
| none (`GON = 0`, convex) | 12,715 | 12,582 |
| learned with the bias's rates | 20,751 | 20,805 |
| learned, rates 1/16 | 13,437 | 13,307 |
| fixed at 1 / 1.64 / 2 / 2.8 / 4 | 12,804 / 12,417 / 12,582 / 13,580 / 15,159 | 12,649 / 12,251 / 12,357 / 13,224 / 14,822 |
| on X0 only / X1 only (fixed 1.64) | 12,386 / 12,380 | 12,274 / 12,260 |
| **learned from 1.64, rates ~1/250** | **12,364** | **12,199** |

A fixed gain of 1 is not byte-identical to `GON = 0`, because the Schraudolph `expf(0)` is not
exactly 1; that is what the switch is for.

## Results (10 connectome sequences, `-d4`; `./mkcorpus.sh` builds them)

The same targets with three kinds of predictions in `pred_t0`/`pred_t1`:

| bytes, t0+t1 of all 10 files | pz: zeros | pd: dummy (`predict -m0`) | pb: GRU baseline |
|---|---:|---:|---:|
| **tsvcomp** (Counter/MixN paq block, MixN with gain) | **12,364** | **10,899** | **12,199** |
| convex MixN (`GON = 0`) | 12,715 | 11,208 | 12,582 |
| previous commit: hand-written paq block (own StateMap-like slots, unconstrained 2-layer mixer) | 11,486 | 10,252 | 11,251 |
| tsvcomp `-DPAQ=0` (tuned counters only) | 21,762 | 16,676 | 20,850 |
| xz -9e, cols.bin | 42,032 | 42,032 | 42,032 |
| coder0, cols.bin | 64,916 | 64,916 | 64,916 |
| raw cols.bin | 1,600,000 | 1,600,000 | 1,600,000 |

The gain takes about 3% off the convex MixN. The framework version is still about 7% behind the
hand-written block, whose knobs were not tuned either. Most of the new modules' knobs are
copies tuned for other roles, so this is the place for `opt.pl`. The Mix2 stages (M0–M3, M1)
are still convex blends.

What the paq block adds, by ablation (`-DNOCX`, measured with the hand-written block of the previous commit whose model 18 used
`is_scored`; that model was replaced because it lost):

| without | zeros | GRU |
|---|---:|---:|
| (nothing) | 11,827 | 11,538 |
| price-change models 8, 9, 10, 20, 21 | 18,856 | 16,285 |
| history models 1–6 | 12,028 | 11,921 |
| prediction models 12–15 | 11,576 | 11,437 |
| model 18 as `is_scored` | 11,616 | 11,317 |

Almost all of the gain comes from which i0 price columns changed, and in which direction. Targets
move when the book moves. The prediction models help with the dummy predictions (−5%) but not
with the GRU ones, as before.

About 1.4 s per file each way. All 30 files round-trip in every mode, and the shipping and
tuning builds produce identical streams. `tsvcomp.exe` in this directory is the previous
version's build: it was not rebuilt here, so rebuild it with `gc.bat`.

## Tuning

```sh
./mkcorpus.sh                 # corpus/ (needs ../connectome built and its data unpacked)
./gc.sh tune
cp tsvcompt tsvcompt.tune
perl IDX/opt.pl opt.lst ./tsvcompt.tune        # or IDX/optv.pl; results in export.!!!
cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done
```

The new modules to tune are H0 (the hashed counters and the table size TB), X0/X1 (the MixN
group and final mixers, including their gain knobs) and M3.
