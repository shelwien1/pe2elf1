# coder0 / bmpc

`coder0` is an order-1 adaptive arithmetic coder whose counters carry their
own online optimizer: every counter's adaptation rates (`wr0`/`wr1`), its
mixing weight and its stretch gain are themselves parameters, moved each bit
by a damped-Newton `ParamUpdater`. `bmpc` is a lossless BMP compressor built
on the same machinery — see **[README-bmpc.md](README-bmpc.md)**.

```
./gc.sh                            # build both (clang++ if present, else g++)
./t.sh                             # coder0 roundtrip on book1  -> 344899
CODER=./bmpc ./t.sh testfiles/*.bmp
```

| | |
|---|---|
| `gc.sh` / `t.sh` | linux ports of `gc.bat` / `t.bat` |
| `coder0.cpp` | the order-1 coder |
| `bmpc.cpp` | the BMP compressor |
| `sh_common.inc` | rangecoder, IDX runtime, float helpers |
| `sh_counter0.inc` | `ParamUpdater` + `Counter`, shared by both codecs |
| `config.hpp` / `config_b.hpp` | `ParamUpdater` configurations (C0 / B0) |
| `IDX/*.idx` | parameter sources; `MOD/*_h.inc` is generated from them |
| `IDX/opt.pl` | hill-climbs the `!MAP!` patterns inside the binary |
| `sweep.py` | rebuilds with `-D` overrides, reports corpus totals |
| `tools/` | corpus crops, `#ifndef` wrapping, IDX helpers |

`GCIDX=1 ./gc.sh` regenerates `MOD/` from `IDX/`. See
[IDX-FORMAT.md](IDX-FORMAT.md) for what the `.idx` language is.
