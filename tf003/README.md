# The Tangelo model in place of PPMD, under a transformer

This is the transformer-based coder with its **context model** replaced: the
thing that hands a distribution over the next byte to the mixer, and to the
transformer as its prior, was PPMD (`ppmd2.hpp`) and is now the Tangelo model
from the rest of this repository, run a byte at a time by journaled speculative
execution over its own bit predictions ([../ALGORITHM.md](../ALGORITHM.md)).

Nothing else moved. The range coder, the binary mixer, the transformer, the
weights format and the main loop are the ones that were here.

## Results

`book1000` is the 4 KB sample that ships with this directory; the 64 KB column
is the first 65 536 bytes of `book1wrt`. Every row round-trips.

| context model | `book1000` | + transformer | `book1wrt`[:64K] | + transformer |
| --- | ---: | ---: | ---: | ---: |
| PPMD | 2 579 | 1 895 | 31 725 | 25 803 |
| Tangelo | **2 298** | **1 807** | **29 425** | **25 396** |
| | −10.9 % | −4.6 % | −7.3 % | −1.6 % |

Two things are worth reading off that.

**Tangelo is the better context model**, by 7-11 % on its own. That is not
surprising - it is a paq/lpaq-class model with 361 MB of state against PPMD's
order-9 - but it is the first time it has been measured here against anything
but itself.

**Most of that gain does not survive the mixer.** With the transformer in the
mix the margin falls to 4.6 % and 1.6 %, because the transformer already knows
much of what Tangelo adds over PPMD. There is also a reason to expect the
remaining margin to be understated: the transformer was *trained* with PPMD's
distribution as its prior (`Transformer::Predict` takes it as `ppmd_probs`), and
it is now being handed a different model's. Retraining it against Tangelo's
priors is the obvious next experiment and is not something this port can do.

The cost is the walk: about 70 µs per byte here, which on the 64 KB file takes
the whole run from 17.6 s to 24.4 s. Tangelo alone goes from 1.7 s (PPMD alone)
to 6.2 s. [../SPEED.md](../SPEED.md) is where that time goes and what has been
done about it.

## Building

```sh
./build.sh                    # Tangelo as the context model (the default)
USE_PPMD=1 ./build.sh         # PPMD instead, for comparison
TF_CHAIN=1 ./build.sh         # a second transformer fed the mix (see below)
```

One translation unit, and the Tangelo half needs no build machinery at all: it
is `tangelo_s`'s route, where the model marks its own writes in its source
(`W(x) = ...`, see [../write.inc](../write.inc)), so there is no second compile
of the model step, no assembly, no perl and no instrumenter. The `track.pl`
route could not have been embedded here without all of that.

`gc.bat` is the original Windows/clang build with the MSVC headers, and takes
the same switches.

```
coder0 c|d <input> <output> [weights_in] [weights_out]
```

Naming a weights file that does not exist runs the context model alone, which is
the "alone" column above. About 380 MB of memory goes to Tangelo, on top of the
transformer's.

## What changed

| | |
| --- | --- |
| [`../tangelo_bm.inc`](../tangelo_bm.inc) | new: Tangelo packaged as `P(next byte)` for a host program that has its own coder |
| `transformer.inc` | `UnifiedModel`'s context model, behind `USE_PPMD` |
| `coder0.cpp` | includes the above; `ppmd_probs_` renamed `ctx_probs_`, `UpdatePPMD` renamed `UpdateCtx` |
| `build.sh` | new: the Linux build |
| `ppmd2.hpp` | unchanged, and still compiled in - `-DUSE_PPMD=1` is a flag, not a fork |
| [`tfwc/`](tfwc/) | new: `3to2`, a converter between the `FX2TFWC2` container this reads and the `FX2TFWC3` one zmix ships; `tch2bin.py`, upstream's PyTorch checkpoints as weight files, quantized or fp32 |
| `tf/fp32_model.inc` | the fp32 engine loads a plain fp32 `.weight` matrix where the file has one instead of `.weight.q` + `.weight.scale` (`tf/PORTING.md`, change 8) |
| `coder0.cpp` | reports each model's own code length before mixing, `coder0: alone, transformer … context model …`, on stderr at the end |

The host's side of it is three lines:

```cpp
tangelo::ByteSource ctx_model_;
ctx_model_.Init(vocab);               // vocab[256]: which bytes can occur
ctx_model_.Update(c);                 // the eight real steps for the coded byte
ctx_model_.Predict(ctx_probs_);       // P(next byte) into 256 floats
```

`Update` then `Predict` is the same shape as PPMD's `ppmd_UpdateByte` then
`ppmd_PrepareByte`, which is why the main loop did not have to change.

## Other weights

`tfwc/` converts between this container (`FX2TFWC2`) and the `FX2TFWC3` one
[zmix](https://zmix.frostbyrne.io/) v1.0 ships, in both directions and without
changing a tensor value. coder0 runs on zmix's retrained
`6m-q4-fp32-t1lambda1` weights with no change at all once they are converted -
1 802 bytes on `book1000` against 1 807 for the gen-7 blob, and 25 333 against
25 396 on the 64 KB slice. `tfwc/README.md` has the numbers and how the
converter was checked against the blobs zmix ships in both containers.

## A second transformer, fed the mix

An idea that had to be tried: take the mixed distribution, mix(context model,
transformer), and hand it to a *second* instance of the transformer as its
prior, with the same weights file. The transformer was trained to correct a
PPMD-quality prior. The mix is a much better prior, so a second instance would
be a learned SSE/APM stage with the whole context at its disposal, for the
price of a second forward pass and no new weights. `-DTF_CHAIN=1`
(`TF_CHAIN=1 ./build.sh`) builds it: a second `Transformer`, a second bank of
mixers after it, and the byte is coded with the second mix. `TF2_WEIGHTS=file`
gives the second instance different weights, `TF2_PRIOR_POW=a` tempers the
prior it is handed (pᵃ, renormalised), and `-DTF_CHAIN=2` feeds it the context
model's distribution instead of the mix - a plain ensemble, which is what a
chain has to beat.

It helps on a small file and hurts on a large one. Same weights in both
instances, Tangelo as the context model unless it says PPMD:

| | `book1000` | `book1wrt`[:64K] | `book1wrt` |
| --- | ---: | ---: | ---: |
| gen-7 | 1 807 | 25 396 | 167 692 |
| gen-7, chained | **1 741** (−3.7 %) | **25 323** (−0.3 %) | 168 476 (+0.5 %) |
| `t1lambda1` | 1 802 | 25 333 | 167 284 |
| `t1lambda1`, chained | | **25 220** (−0.4 %) | 167 977 (+0.4 %) |
| PPMD, gen-7 | 1 895 | 25 803 | |
| PPMD, gen-7, chained | | **25 500** (−1.2 %) | |

The second instance, on its own, is a *worse* model than the first, and gets
worse as the file grows: 3.209 against 3.159 bits/byte at 64 KB, 3.063 against
2.987 on the whole file. It was trained on how far to trust a PPMD prior, and
the mix is sharper than that and keeps getting sharper as Tangelo and the
mixer learn - so it over-trusts, and drifts further from the distribution it
was trained on the longer the file runs. Early on any second opinion helps the
mixers, which are still learning; later the second mixer can only partly
discount an input that is confidently wrong. That the PPMD-prior column gains
the most fits the same reading: its mix is the closest to what the model saw
in training. Tempering the prior does not fix it (64 KB, gen-7: a=0.75 gives
25 314, a=0.5 gives 25 343).

Two more rows at 64 KB say where the gain that does exist comes from. The
ensemble control - the same weights twice, both fed the context model, so the
second instance predicts exactly what the first does - gives 25 375: the second
mixer stage alone is worth 21 bytes. With *different* weights in the second
instance (`TF2_WEIGHTS`, gen-7 first and zmix's `t1lambda1` second) the
ensemble gives 25 228 and the chain 25 161 - better than either model alone
(25 396 and 25 333), the best 64 KB result in this directory. Neither
survives the whole file: 167 638 for the ensemble and 167 796 for the chain,
against 167 284 for `t1lambda1` alone. Past the first hundred KB a second
model costs more than the second mixer stage recovers, whichever prior it is
fed, and the chain is again the worse of the two - its second instance alone
is 3.028 bits/byte where the same weights fed the context model give 2.969.

The cost is a second full transformer pass per byte - 28 s to 53 s on the 64 KB
slice - and it cannot be batched with the first: the second instance's prior
at byte *t* is the first instance's output at byte *t*, so its whole recurrent
state depends on the first's. What would make the chain earn that is the
training-side change the results point at: a second-stage model trained on
the first stage's *mixed* output as its prior, over the same data, rather than
the same weights asked to play both roles. That is a variant of retraining
against the deployed prior (WEIGHTS.md, section 5, item 4), and it is not
something this coder can do.

## Could the weights themselves be tuned for compression?

[WEIGHTS.md](WEIGHTS.md) works through it with this model's numbers, and
measures the one headroom that turned out to be large - the int4 model is
0.78 % behind the fp32 model it was quantized from (section 7). It covers what a
search over ±1 weight moves would cost on a GPU and why a gradient is two
million times cheaper for the same information, how quantized weights are
trained and rounded in practice (QAT, GPTQ), the fact that the container is
110× the compressed data at 64 KB, and the one place - the unembedding - where
an exact code-length search is the right tool and needs no GPU at all.

## Checking it

A model that marks its own writes can miss one, and the failure is silent -
encoder and decoder corrupt the model identically, so the round trip still
passes and only the compressed size suffers. `-DTRACK_VERIFY` is what finds it:

```sh
VERIFY="-DTRACK_VERIFY=32 -DTRACK_VERIFY_EVERY=200" ./build.sh
./coder0 c book1000 /tmp/out
```

It compares every byte of the 360.8 MB of model state across each walk, and
cross-checks the walk's predicted code length for the byte actually coded
against the model's own eight per-bit predictions. Both pass here, and the
compressed output is identical to the ordinary build's.
