# coder0 + pretrained transformer

`coder0` is Eugene Shelwien's small byte-oriented compressor: an order-9
`mod_ppmd` model (`ppmd2.hpp`) whose per-byte distribution is mixed with a
second byte model by a per-bit-tree-node logistic mixer (`newton.inc`), and
coded with a 32-bit range coder (`sh_v2f.inc`).

This tree replaces the second model — an LSTM trained on the fly — with the
frozen 6M-parameter transformer from cmix's `fx2-cmix-transformer` Hutter-Prize
submission, loaded from the quantized weights file `models/6m-q4-fp32.tfwc2`.
The inference engine is in `tf/` (see `tf/PORTING.md`), the glue is
`transformer.inc`.

## Build

```sh
./build.sh                                   # clang++ (or CXX=g++), -march=haswell
TFDEFS="-DTF_TRAIN=1" ./build.sh             # + online training of the output layer
TFDEFS="-DTF_LOAD_WEIGHTS=0" ./build.sh      # no weights file: initialize in memory
```

Windows: `gc.bat` (clang, the same layout Shelwien's other builds use).

Both build the transformer sources as separate translation units with IEEE
floating point; `coder0.cpp` keeps its `-Ofast` build. `tf/PORTING.md` explains
why that separation is required.

## Use

```sh
./coder0 c <input> <output> [weights.tfwc2]
./coder0 d <input> <output> [weights.tfwc2]
```

Without the fourth argument the weights are looked up as `6m-q4-fp32.tfwc2`
then `models/6m-q4-fp32.tfwc2`. If no weights file is found, or the input's
alphabet does not fit the model's 205 tokens, the transformer is disabled and
coder0 runs on PPMD alone — still lossless, just without the second model.
(A weights file that exists but is corrupt is fatal: the fx2 loader exits the
process rather than reporting an error.) Naming a file that does not exist is
the supported way to ask for that:

```sh
./coder0 c book1000 out.ppmd none
```

## Results

Compressed size in bytes (clang++-18, `-march=haswell`; `log.txt` has the
percentages and the timings):

| input | original | PPMD only | PPMD+LSTM | **PPMD+transformer** |
|---|---|---|---|---|
| `book1000` | 4096 | 1858 | 1821 | **1734** |
| `book1[:16384]` | 16384 | 6312 | 6104 | **6003** |
| `book1[:65536]` | 65536 | 21803 | 20796 | 21085 |
| `book1` | 768771 | 209801 | 194387 | 204669 |

All four round-trip byte-identically (`./t.sh <file>`).

The frozen transformer wins on short inputs and loses to the on-line LSTM past
~30 KB. That is what one should expect: it was trained on WRT-dictionary-encoded
Wikipedia, so a plain-ASCII novel is out of its training distribution, while the
LSTM adapts to the file at hand. It beats PPMD alone on every input.

coder0 sets FTZ and DAZ at startup, which the fx2 kernels were not validated
under; it makes no difference to any of the sizes above, and it is the same on
both sides of the coder either way.

It also costs about 3500 bytes/s (~110 quantized matmuls and a 2.9 MB sequential
weight stream per byte) against the LSTM's ~10^5, and 37 MB of RAM on top of
PPMD's arena.

Compress and decompress with the *same binary*: the coding distribution is
floating point, so a gcc-built and a clang-built coder0 do not produce
interchangeable archives. The transformer is not the reason — with
`-ffp-contract=off` (which `build.sh` and `gc.bat` pass, and which costs clang
nothing because its default contracts nothing here) the two toolchains produce a
*bit-identical model*, verified. What still diverges is `coder0.cpp`'s own float
mixer under `-Ofast`, exactly as it did before this change: build it `-O2
-ffp-contract=off` instead and a gcc and a clang coder0 emit byte-identical
archives (and `book1000` costs one byte more, 1735).

## Switches

`transformer.inc` has two compile-time switches, both settable with `-D`
(`TFDEFS=` in `build.sh`, `set tfdefs=` in `gc.bat`):

* **`TF_FP32`** (default 1) — the weights file stores int4 weights with per-row
  scales and a static scale per activation quantizer. With `TF_FP32=1` every
  weight is dequantized **once, at load** (`w = q * row_scale`) and nothing is
  quantized again: the forward pass is ordinary floating point with no
  activation quantization steps, and every parameter of the model is a plain
  float in one contiguous block (`weights()` / `weight_count()`, 5,897,145
  floats = 22.5 MB) that can be backed up and restored with a `memcpy` —
  which is what training the body needs. `TF_FP32=0` runs the packed int4/int8
  AVX2 engine instead: ~4x faster, but the weights only exist inside its
  arenas. Compression is the same either way (see the table), because the model
  was trained with fake quantization and is at least as good without it.
* **`TF_LOAD_WEIGHTS`** (default 1) — 0 initializes the weights in memory
  instead of reading `6m-q4-fp32.tfwc2`, from `TF_INIT_SEED`. The scheme is the
  reference one, transcribed from the submission's training code
  (`pysrc/model.py`, `pysrc/quantization.py`, `training_recipes/quantize.py`):
  kaiming-uniform weights, `normal_` for the two embeddings, per-row
  `max|w|/7` weight scales, `(2/sqrt(127))*mean|x|` activation scales, and the
  reference's KDA decay-rate and `dt_bias` draws. See `tf/weights_init.cpp`.
  Deterministic, so encoder and decoder build the same model.
* **`TF_TRAIN`** (default 0) — online training, in C++, of what sits on top of
  the frozen blocks. `1` trains the output layer: coder0 keeps an fp32 copy of
  the unembedding (started from the model's), runs it on the transformer's
  final activation and updates it with AdamW from the byte just coded, with the
  gradient taken through the model's logit softcap. `2` additionally runs and
  trains one more block in the model's own MLP shape —
  `h + down(relu(up(rms_norm(h)))²)`, then `rms_norm` — with the backward pass
  for all of it written out in fp32. `TF_LR_X100000` (default 2000) sets the
  rate, `TF_ADAPTER_D` (default 192) the block's hidden width.

Everything trained is position-local, which is what makes a per-byte update
*exact*: there is no recurrence to backpropagate through, so unlike the LSTM
(which could only afford full BPTT every `horizon_` bytes) nothing is
truncated. `tools/gradcheck.cpp` checks the analytic gradients against the
numeric directional derivative of the loss; it agrees to ~1e-5 relative:

```sh
clang++ -O2 -std=c++17 -I. tools/gradcheck.cpp $(ls obj/*.o | grep -v coder0) -o gradcheck && ./gradcheck
```

Every combination round-trips:

| config | book1000 | book1[:16K] | book1[:64K] | book1 |
|---|---|---|---|---|
| PPMD alone | 1858 | 6312 | 21803 | 209801 |
| PPMD + LSTM (the original) | 1821 | 6104 | 20796 | 194387 |
| fp32, frozen (default) | 1735 | 6002 | 21086 | |
| fp32 + `TF_TRAIN=1` | 1761 | 5976 | 20756 | |
| fp32 + `TF_TRAIN=2` | 1765 | 5957 | 20666 | |
| **fp32 + `TF_TRAIN=3`** | **1671** | **5700** | **20038** | **194251** |
| fp32 + fresh init | 2046 | 6680 | 22364 | |
| quantized, frozen | **1734** | 6003 | 21085 | 204669 |
| quantized + `TF_TRAIN=1` | 1761 | 5977 | 20770 | 200299 |
| quantized + `TF_TRAIN=2` | 1766 | 5966 | 20684 | 200026 |
| quantized + fresh init | 2042 | 6672 | 22356 | |

The fp32 and quantized engines land within a byte or two of each other
everywhere; fp32 costs about 4x the time (6.9s vs 1.7s per 4 KB here).

`TF_TRAIN=3` — training the whole model — wins everywhere: against the LSTM it
replaced, −8.2% on book1000, −6.6% at 16 KB, −3.6% at 64 KB and −0.07% on the
whole of book1. The last one matters most: the frozen model lost book1 by
10,282 bytes, and training closes all of it. It costs about 8 ms/byte here
(36s per 4 KB against 6.9s frozen, and 1h47m for book1); `TF_BATCH=4` brings
that to 22s per 4 KB for about 0.7% more output.

### What is not trained, and why

The 12 transformer blocks stay frozen. The engine consumes weights pre-packed
as int4 nibbles with folded per-row scales, behind fused quantization epilogues
and hand-written kernels, so a backward pass through the body needs fp32 master
weights plus a re-pack of the 2.9 MB weight stream after every byte. There is
no C++ backward pass in the submission to reuse either: `cpp_infer/src` is
inference only, and training is PyTorch (Muon for the matrices, AdamW for the
rest). What is here is written from scratch over the model's own operations —
`rms_norm` and its backward, relu-squared, the logit softcap, the residual add
— which are the primitives a deeper port would build on.

## Why the token mapping matters

The transformer has a fixed vocabulary of 205 tokens and was trained on one
specific byte stream: enwik9 after cmix's WRT dictionary transform. A token id
is the *rank* of a byte value among the values occurring in that stream, so
feeding it coder0's own alphabet indices points every embedding at the wrong
symbol.

`transformer.inc` reconstructs the enwik9 vocabulary from the WRT encoder
instead (upper-case letters never occur in it, `encode_text` permutes six byte
ranges, all 128 values >= 0x80 occur), and checks the reconstruction against the
four token ids cmix asserts for the encoded article separator
`"  <page>\n    <title>"`. The derivation is written out in full at the top of
that file. Under it `book1000` codes to 1734 bytes; under coder0's own alphabet
indices, 2140 — worse than not running the model at all (1858). On the first
20000 bytes of `book1` with a flat prior the two mappings measure 3.73 and 7.19
bits/byte, against 8.33 for a deliberately scrambled one.

Upper-case letters and control codes have no token of their own under that
mapping; they get one of the tokens the file's alphabet leaves unclaimed, in
ascending byte order. Feeding them as the WRT capitalization marker plus the
lower-case letter instead was measured to be worth 0.2% and needs two forward
passes per byte, so it is not done.

## Layout

```
coder0.cpp        the compressor (main loop, range coder driver)
transformer.inc   weights loading, byte<->token mapping, context splitting
ppmd2.hpp         mod_ppmd
newton.inc        the logistic mixer
sh_v2f.inc        range coder
utils.inc logf1.inc MOD/  support code
tf/               the transformer inference engine (see tf/PORTING.md)
models/           6m-q4-fp32.tfwc2, the pretrained weights
```
