# coder0 / bmpc / mrpc

`coder0` is an order-1 adaptive arithmetic coder whose counters carry their
own online optimizer: every counter's adaptation rates (`wr0`/`wr1`), its
mixing weight and its stretch gain are themselves parameters, moved each bit
by a damped-Newton `ParamUpdater`.

Two lossless image compressors are built on it, from opposite directions:

* **[bmpc](README-bmpc.md)** — one-pass and adaptive. A 180-tap linear mix
  per pixel and a seven-model context mix per bit, everything learned while
  coding, nothing transmitted.
* **[mrpc](README-mrpc.md)** — two-pass, MRP's scheme extended to colour.
  The image is segmented over a quadtree, a linear predictor is fitted to
  each (class, component), and the whole arrangement is iterated against
  measured code length and then transmitted. Static probability models, no
  `Counter` — which is what lets the optimiser price a region without
  coding it, and leaves the decoder 25× faster than bmpc's.

```
./gc.sh                            # build all three
./t.sh                             # coder0 roundtrip on book1  -> 344899
CODER=./bmpc ./t.sh testfiles/*.bmp
CODER=./mrpc ./t.sh testfiles/*.bmp
```

| | |
|---|---|
| `gc.sh` / `t.sh` | linux ports of `gc.bat` / `t.bat` |
| `coder0.cpp` | the order-1 coder |
| `bmpc.cpp` | the adaptive BMP compressor |
| `mrpc.cpp` | the MRP-scheme RGB/RGBA compressor |
| `sh_common.inc` | rangecoder, IDX runtime, float helpers |
| `sh_counter0.inc` | `ParamUpdater` + `Counter`, shared by both codecs |
| `config.hpp` / `config_b.hpp` | `ParamUpdater` configurations (C0 / B0) |
| `IDX/*.idx` | parameter sources; `MOD/*_h.inc` is generated from them |
| `IDX/opt.pl` | hill-climbs the `!MAP!` patterns inside the binary |
| `sweep.py` / `mrpsweep.sh` | rebuild with `-D` overrides, report corpus totals |
| `tools/` | corpus crops, `#ifndef` wrapping, IDX helpers |

`GCIDX=1 ./gc.sh` regenerates `MOD/` from `IDX/`. See
[IDX-FORMAT.md](IDX-FORMAT.md) for what the `.idx` language is.
