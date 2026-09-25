# Alfa Connectome (wundernn.io): local C++ metric/predictor on TSV data

These are local tools for the [Wunder Fund "Alfa Connectome" competition](https://wundernn.io/connectome).
The starter pack is 33.7 GB; these tools work on a few sequences exported to TSV instead.

| File | What |
|---|---|
| `data/seq-<seq_ix>.tsv.xz` | 10 validation sequences (row groups 0–9 of `valid.parquet`), 20,000 rows each; `make data` unpacks them |
| `pred_baseline/seq-<seq_ix>.tsv` | predictions of the starter pack's GRU baseline (`baseline.onnx`) for these sequences |
| `predict.cpp` | predictor template with a dummy model: `./predict data.tsv pred.tsv` |
| `metric.cpp` | Global Weighted Pearson, the competition metric: `./metric data.tsv pred.tsv [data2.tsv pred2.tsv ...]` |
| `quantize.cpp` | data + predictions ⇄ one integer (fixed-point) TSV: `./quantize c seq.tsv pred.tsv int.tsv`, `./quantize d int.tsv seq_out.tsv pred_out.tsv` |
| `tsv.h` | TSV reader and exact float formatting shared by the C++ programs |
| `extract.py` | parquet row groups → TSV (how `data/` was made) |
| `baseline_predict.py` | runs the ONNX baseline on data TSVs (how `pred_baseline/` was made) |
| `validate.py` | checks `./metric` against the official Python scorer (`utils.py`) |
| `fetch_valid.py` | downloads only `valid.parquet` (5.4 GB) out of the starter pack zip |

## Usage

```sh
make            # builds ./predict, ./metric, ./quantize (g++ -O2, plain C++11)
make data       # data/*.tsv.xz -> data/*.tsv (xz -dk)

./predict data/seq-256678.tsv pred.tsv
./metric  data/seq-256678.tsv pred.tsv

# All 10 sequences, pooled into one score like on the platform
# (the platform pools all scored rows; it does not average per-sequence scores):
mkdir -p pred_dummy
for f in data/*.tsv; do ./predict $f pred_dummy/${f#data/}; done
./metric $(for f in data/*.tsv; do echo $f pred_dummy/${f#data/}; done)
./metric $(for f in data/*.tsv; do echo $f pred_baseline/${f#data/}; done)
```

`metric` prints one line per pair (when given several) and a `TOTAL` line.
`-dN` sets the number of decimals.

## File formats

**Data TSV**: a header line, then one line per row. The columns are the parquet columns, in the same order:
`seq_ix step_in_seq need_prediction is_scored <112 features> t0 t1`.
Booleans are written as `0`/`1`. Floats use the shortest text that `strtof` reads back as the
exact original float32 value; `extract.py` re-reads every file and compares it bit-for-bit.
One file can hold several sequences. Models reset their state whenever `seq_ix` changes.

**Prediction TSV**: header `seq_ix step_in_seq t0 t1`, then one line per `need_prediction` row
(steps 99..19999, so 19,901 lines per sequence). `metric` also accepts one line per data row,
in which case it ignores the warm-up lines. It checks that `seq_ix`/`step_in_seq` match the data file
and it rejects non-finite predictions, as the platform does.

**Integer TSV** (`quantize`): the data columns followed by `pred_t0 pred_t1`, one line per data row.
Every float is stored as `round(v * 10^N)`, and the header records the scale in the column name,
e.g. `i0_p0/10000`. `seq_ix`, `step_in_seq`, `need_prediction` and `is_scored` are unchanged.
The prediction columns hold 0 on warm-up rows.

```sh
./quantize c [-dN] seq.tsv pred.tsv int.tsv        # N decimal digits, default 4
./quantize d int.tsv seq_out.tsv pred_out.tsv      # values written as exact decimals (12345/10000 -> 1.2345)
```

Effect on the pooled WP of the 10 sequences:

| digits | baseline preds, targets+preds dequantized | dummy re-run on dequantized data | GRU re-run on dequantized data |
|---|---:|---:|---:|
| original | 0.644815 | 0.440572 | 0.644815 |
| `-d4` | 0.644818 | 0.440568 | 0.644817 |
| `-d3` | 0.644811 | 0.440398 | 0.644829 |
| `-d2` | 0.644673 | 0.438638 | – |

With `-d4` every score moves by 4e-6 or less. The integer file for one sequence is 13.4 MB
at `-d4` and 8.8 MB at `-d2`, against 25 MB for the float TSV. Quantizing a decoded file again
gives the identical integer file.

## Predictor

`predict.cpp` feeds the rows to `Model` one at a time, in file order, the same way the platform calls
`PredictionModel.predict()`. The model is reset at each new `seq_ix`, updated on every row
(warm-up rows included), and its output is written for `need_prediction` rows. Replace
`Model::predict()` with your own model.

- `-m0` (default) dummy: `t0 = i0_p3`, `t1 = -i0_p20`. These are the single features that correlate best
  with the targets on 50 other validation sequences (row groups 10–59).
- `-m1` "same as previous": outputs the previous row's `t0`, `t1`. **This is a leak.** At test time the
  model never receives targets: `DataPoint` carries only `seq_ix`, `step_in_seq`,
  `need_prediction` and the 112 features. This mode only shows how smooth the targets are.

## Scores on these 10 sequences (15,615 scored rows out of 199,010 required)

| predictions | t0 | t1 | WP |
|---|---:|---:|---:|
| baseline GRU (`pred_baseline/`) | 0.616879 | 0.672752 | **0.644815** |
| dummy (`predict -m0`) | 0.468003 | 0.413142 | **0.440572** |
| previous target (`predict -m1`, leak) | 0.991649 | 0.989401 | 0.990525 |

The baseline scores 0.617052 on the full validation set (1,873 sequences). Scores of single sequences
spread widely (dummy: −0.006…0.680, baseline: 0.436…0.872), so 10 sequences give a noisy
estimate.

## Validation against the official scorer

`validate.py` scores each prediction set three ways: with `utils.GlobalAccumulator` (what
`ScorerStepByStep` uses), with `utils.weighted_pearson` on the pooled rows (the reference in
`METRIC.md`), and with `./metric -d15`. It compares both the pooled score and the per-file scores.
It also generates synthetic prediction sets that test clipping, constant predictions (score 0),
perfect and negated predictions, the one-line-per-data-row layout, and rejection of an `inf` prediction.

`pred_dummy/` and `pred_prevtarget/` are the outputs of `./predict -m0` and `./predict -m1`
(see Usage):

```
$ python validate.py --starterpack wnn_connectome_starterpack data pred_baseline pred_dummy pred_prevtarget
pred_baseline    C++ WP=0.644815188  GlobalAccumulator=0.644815188077  reference=0.644815188077  max|diff|=1.2e-14  OK
pred_dummy       C++ WP=0.440572320  GlobalAccumulator=0.440572319826  reference=0.440572319826  max|diff|=1.6e-14  OK
pred_prevtarget  C++ WP=0.990525343  GlobalAccumulator=0.990525342537  reference=0.990525342537  max|diff|=2.9e-14  OK
random_x3        C++ WP=-0.000034116  GlobalAccumulator=-0.000034116198  reference=-0.000034116198  max|diff|=7.0e-16  OK
noisy_target     C++ WP=0.819268944  GlobalAccumulator=0.819268943528  reference=0.819268943528  max|diff|=7.5e-15  OK
constant         C++ WP=0.000000000  GlobalAccumulator=0.000000000000  reference=0.000000000000  max|diff|=0.0e+00  OK
perfect          C++ WP=1.000000000  GlobalAccumulator=1.000000000000  reference=1.000000000000  max|diff|=1.1e-16  OK
negated_x5       C++ WP=-0.909447055  GlobalAccumulator=-0.909447054624  reference=-0.909447054624  max|diff|=1.9e-14  OK
all_rows_layout  C++ WP=0.386902363  GlobalAccumulator=0.386902362700  reference=0.386902362700  max|diff|=3.7e-15  OK
nonfinite        official rejects: True, ./metric rejects: True  OK
ALL OK
```

End-to-end check: running the official `python baseline/solution.py --validation` on a parquet file
built from the same 10 row groups prints `'weighted_pearson': 0.6448151880770624`
(t0 0.6168788460724343, t1 0.6727515300816905, 15,615 selected rows). `./metric` on
`pred_baseline/` gives the same numbers.

### Metric summary (see `METRIC.md` in the starter pack)

For each target separately, over all rows of all sequences with `need_prediction && is_scored`:
`y = clip(t, -2, 2)`, `p = clip(pred, -2, 2)`, weight `w = |y|` (rows with `t = 0` have zero weight), and
the weighted Pearson correlation of `y` and `p` computed in float64 arithmetic from float32 inputs.
A target whose total weight is below 1e-8, or whose weighted std (target or prediction) is at most 1e-8,
scores 0, so constant predictions give 0. The final score is the mean over `t0` and `t1`. Adding a
constant to the predictions or multiplying them by a positive one doesn't change a Pearson correlation,
but the clipping at ±2 is applied before the correlation is computed.

## Getting the starter pack pieces

`validate.py` and `baseline_predict.py` need `utils.py` and `baseline/` from the starter pack.
All of its small files come first in the `.tar.gz`, so the first 3 MB are enough:

```sh
curl --max-filesize 3000001 -r 0-2999999 https://files.wundernn.io/wnn_connectome_starterpack.tar.gz \
  | tar -xz --exclude='*.parquet' --exclude='._*'   # ends with "Unexpected EOF": expected
pip install -r wnn_connectome_starterpack/requirements.txt   # numpy, onnxruntime, pyarrow
```

(`--max-filesize` protects against the CDN occasionally ignoring `Range` on a cold request and
sending the whole file.)

To get the whole validation set (1,873 sequences with targets and `is_scored`) without the 28 GB
`train.parquet`: `python fetch_valid.py valid.parquet`. The zip stores `valid.parquet` as one deflate
stream at a known offset, so the script downloads 5.4 GB with an HTTP range request, inflates it,
and checks the size and CRC. After that, `python extract.py valid.parquet data 10 11 12` exports more
row groups to TSV.

## Data notes (from these sequences)

- Targets are quantized. Within a sequence, every `t0`/`t1` is an integer multiple of one step, like a
  tick count times a scale. The step varies between sequences: these 10 use four of them
  (0.12543313, 0.24132973, 0.46853608, 0.65686768), and each sequence has 9–63 distinct values per target.
  Targets are smooth over time (lag-1 autocorrelation 0.93–0.99). In sequence 256678, `t0` and `t1`
  correlate at −0.67.
- Features look rank-gaussianized: most values fall within ±2.4, and the minimum is −5.1993 = Φ⁻¹(1e-7).
  Price-like columns change rarely: 8 to 1,259 distinct values per 20,000 rows, median ≈ 64.
- `is_scored` selects 7.8% of the required rows here, in short runs (3.7 rows on average). The test
  mask is hidden, so predictions must be good on every row.
- The platform runs `solution.py` on 1 CPU and allows 60 minutes for 1,970 × 20,000 test rows.
  That is about 90 µs per row, Python callback overhead included.
