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
checked in generated, so the tree builds without perl; `mk.sh` regenerates it
when perl is present.

## Testing

```sh
./t.sh                       # round-trip and size over testfiles/*.bmp
./t.sh some/other/*.bmp
```

`t.sh` compresses, expands, and `cmp`s against the original. It exits non-zero
if any file fails to round-trip.

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
| `t8g.bmp` | 320×240 grey | 77 878 | 45 859 | 0.589 |
| `t8p.bmp` | 320×240 palette | 77 878 | 45 934 | 0.590 |
| `t24.bmp` | 320×240 RGB | 230 454 | 56 361 | 0.245 |
| `t32.bmp` | 320×240 RGBA | 307 254 | 56 429 | 0.184 |
| `x_ai.bmp` | 2820×1600 grey RLE8 | 887 278 | 152 200 | 0.172 |
| `x_ci.bmp` | 2820×1600 grey RLE8 | 3 278 170 | 574 487 | 0.175 |
| `x_ep.bmp` | 705×800 RGBA | 2 256 054 | 353 074 | 0.157 |
| **total** | | **7 114 966** | **1 284 344** | **0.181** |

About 1.5 million samples a second, and about 60 MB of tables regardless of
image size.

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
* prediction is MED / LOCO-I and nothing else;
* the residual is folded, coded as a literal ladder with a logarithmic escape,
  and every decision is mixed from ten context models in the logistic domain and
  refined by two secondary estimators.

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
| `mk.sh`, `t.sh` | build and test |
