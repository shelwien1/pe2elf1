# bmg — a lossless Grey / RGB / RGBA BMP compressor

`bmg` compresses Windows BMP images without loss and gives back the **exact
bytes** that went in — header, palette, row padding, run-length structure and
trailing bytes included, not just the pixels.

It is built on the `bcdr5` framework: that range coder, that adaptive counter,
and the IDX parameter system, with the image model taken from `ALGORITHM_v2.md`
— the reading of BMF 2.01's three models, its decision layer and its counter
layer.

```
bmg c input.bmp output.bmg      compress
bmg d input.bmg output.bmp      expand
```

## Building

```sh
./mk.sh                # tuning build:   every IDX knob is a live object
./mk.sh release        # shipping build: every IDX knob is a folded literal
./mk.sh check          # prove the two produce identical streams
```

Needs a C++17 compiler and, to regenerate `MOD/` from `IDX/`, perl. `MOD/` is
checked in generated — in its shipping form, so `g++ bmg.cpp -o bmg` on its own
gives the fast build — and `mk.sh` regenerates it when perl is present. A stale
`MOD/` compiles fine and codes differently, so regenerate it after touching an
`.idx`.

## Testing

```sh
./t.sh                       # round-trip and size over testfiles/*.bmp
./t.sh some/other/*.bmp
```

`t.sh` compresses, expands, and `cmp`s against the original. It exits non-zero
if any file fails to round-trip.

## Analysing an image

`bmgstat` is a separate tool that reports what is *in* a bitmap that a
compressor could exploit — and prices each finding in bytes, so the report ends
with a ranked list of things to do rather than a wall of numbers.

```sh
./mk.sh stat
./bmgstat testfiles/x_ep.bmp
./bmgstat -q -t 500 -n 16 some/*.bmp     # looser exception budget, more colours
```

It reads the same depths `bmg` does, expanding 8-bit files through their palette
so the statistics are of colours rather than of indices. What it measures:

* **per component** — min, max, mean, median, standard deviation, distinct
  values, order-0 entropy, and the lattice step if the used values are evenly
  spaced;
* **per bit plane** — the 0/1 balance, with a verdict of *always 0* / *always 1*
  when the minority fits an exception budget (100 ppm by default, `-t`), priced
  at log2(N choose k) — the cost of naming *which* pixels are the exceptions,
  which is the whole cost of a property that holds almost everywhere;
* **the value set** — gaps, a fitted gamma for the spacing of the used values,
  and whether renumbering them 0..n−1 shrinks the MED residual;
* **unique pixels** — distinct pixels and distinct RGB, whether the image is
  palettizable, and the most frequent colours;
* **derivability** — for every target and every subset of the others, the counts
  table the question asks for: how many rows it would have, how many pixels
  disagree with their row's most common value, the conditional entropy H(T|S),
  and whether the table plus its exceptions costs less than the plane does;
* **pairwise tone maps** — H(T|P), the mutual information, the best
  deterministic curve P→T and its residual, and the exponent of the power law
  fitted to the same relation, which is what a gamma correction between two
  channels looks like: two numbers instead of a 256-entry table;
* **exact relations, affine fits, and correlation** — X constant, X == Y,
  X == 255−Y within budget; the best weighted least-squares reading of each
  component from the others and what its residual costs;
* **spatial structure** — neighbour equality, runs, repeated rows, horizontal
  period, MED residual entropies, and the value-domain colour transform,
  reported both per target and as a *realizable* set, since the per-target
  choices can be circular;
* **alpha** — opaque and clear fractions, mask against gradient, whether the
  colour under full transparency is constant, and whether the pixels are
  consistent with premultiplication.

Every byte figure is an upper bound against a stated baseline — the raw bytes,
an order-0 coder on the values, or an order-0 coder on the MED residuals — and
the closing section says which, because the numbers overlap and do not add up.

## What it takes

| depth | |
| --- | --- |
| 8-bit | grey, grey-ramp palette, or colour palette — `BI_RGB` or `BI_RLE8` |
| 24-bit | B,G,R |
| 32-bit | B,G,R,A |

Top-down bitmaps (negative height), oversized DIB headers, gaps between the
header and the pixels, and trailing data are all handled. Anything else — 1-bit,
4-bit, RLE4, bitfield formats, a truncated file, a file that is not a BMP at all
— goes through a general-purpose byte model instead, so `bmg c` never fails and
never grows a file by more than nine bytes.

## Results

Seven images from the BMF distribution, round-trip byte-exact on all of them:

| file | | input | bmg | ratio |
| --- | --- | ---: | ---: | ---: |
| `t8g.bmp` | 320×240 grey | 77 878 | 44 912 | 0.577 |
| `t8p.bmp` | 320×240 palette | 77 878 | 45 026 | 0.578 |
| `t24.bmp` | 320×240 RGB | 230 454 | 53 718 | 0.233 |
| `t32.bmp` | 320×240 RGBA | 307 254 | 53 760 | 0.175 |
| `x_ai.bmp` | 2820×1600 grey RLE8 | 887 278 | 149 111 | 0.168 |
| `x_ci.bmp` | 2820×1600 grey RLE8 | 3 278 170 | 569 528 | 0.174 |
| `x_ep.bmp` | 705×800 RGBA | 2 256 054 | 339 560 | 0.151 |
| **total** | | **7 114 966** | **1 255 615** | **0.176** |

Decoding runs once and is fast — one to two and a half million samples a second.
Encoding runs the model two to six times, because choosing the predictor per
plane means trying it (BMF's `-Q9` pays the same way), so it is two to four
times slower than bmf's encoder while its decoder is about twice as fast as its
own encoder. Tables are 90–140 MB, mostly independent of image size.

### Against BMF 2.01

`bmf.cpp` built with `-Ofast -march=native`:

| file | bmf | bmg | |
| --- | ---: | ---: | ---: |
| `t8g.bmp` | 42 912 | 44 912 | +4.66% |
| `t8p.bmp` | 43 676 | 45 026 | +3.09% |
| `t24.bmp` | 53 924 | 53 718 | **−0.38%** |
| `t32.bmp` | 53 996 | 53 760 | **−0.44%** |
| `x_ai.bmp` | 148 780 | 149 111 | +0.22% |
| `x_ci.bmp` | 633 144 | 569 528 | **−10.05%** |
| `x_ep.bmp` | 330 616 | 339 560 | +2.71% |
| **total** | **1 307 048** | **1 255 615** | **−3.94%** |

bmg is 3.9% smaller overall — and its stream also reproduces the input byte for
byte, which bmf's does not: bmf re-encodes an RLE bitmap's run structure with
its own rules, while bmg reproduces the original stream exactly, and still wins
`x_ci` by ten percent.

Where bmf is ahead is where it reaches for model C, its learned predictor: `t8g`,
`t8p` and `x_ep` all take predictor 2 in its descriptors. `BMG-FORMAT.md` §4.4
and §8.1 are about what that model does, how much of it twenty integer candidate
predictors recover, and what they do not.

Round-trip is verified three ways: `t.sh` over the corpus, twenty synthetic edge
cases, and a fuzz in both directions under ASan and UBSan — corrupted streams
into the decoder and mutated files into the encoder, the latter requiring that
whatever came out decodes back to exactly what went in.

## How it works

`BMG-FORMAT.md` is the full account: the container, the colour transform, the
context models, the residual coder, and — the part worth reading first — the
three things that were built from `ALGORITHM_v2.md`, measured, and then removed
because they did not pay.

The short version:

* the file is coded whole, and the run structure of an RLE8 bitmap is coded
  *against the pixels*, which both halves already have, so preserving it costs
  almost nothing;
* colour planes subtract a fitted, searched combination of the plane that leads
  — in the value domain, before prediction, which measures three times better
  than subtracting the leading plane's residual;
* each plane is predicted either by MED / LOCO-I or by a learned mixture of
  twenty candidate predictors, and which one is decided by **encoding the image
  both ways and keeping the shorter** — no entropy estimate gets that choice
  right, and getting it right per plane is worth 3%;
* the residual is folded, coded as a literal ladder with a logarithmic escape,
  and every decision is mixed from thirteen context models through two layers of
  gated linear mixing and refined by two secondary estimators.

## Tuning

Fifty-one parameters are declared in `IDX/bmg-P0.idx`. The tuning build embeds
each one's bit pattern in the executable; `IDX/opt.pl` flips those bits, re-runs
a corpus and keeps what shrinks the total.

```sh
./mk.sh && cp bmg bmg_opt
perl IDX/opt.pl opt.lst
cd IDX && for f in bmg-*.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done
./mk.sh check
```

`opt.lst` names five small crops covering 8-, 24- and 32-bit images, so no model
drifts while the others are being measured. See `IDX-FORMAT.md` for the format
and `BMG-FORMAT.md` §7 for what each knob does.

## Files

| | |
| --- | --- |
| `bmg.cpp` | container, BMP front end, encoder-side searches, `main` |
| `bmg_rc.inc` | the range coder |
| `bmg_ctr.inc` | counter, logistic domain, mixer, secondary estimator |
| `bmg_model.inc` | the plane model |
| `bmg_bmp.inc` | BMP parsing, the prologue and generic byte models, RLE8 |
| `sh_mapping.inc` | IDX runtime support |
| `IDX/`, `MOD/` | parameter declarations and their generated headers |
| `IDX-FORMAT.md` | the IDX parameter system, as it came with `bcdr5` |
| `bmgstat.cpp` | the image analysis tool, standalone |
| `mk.sh`, `t.sh` | build and test |
