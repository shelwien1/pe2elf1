# `3to2` — FX2TFWC2 ⟷ FX2TFWC3 weight containers

```
3to2 c <in.tfwc3> <out.tfwc2>     v3 -> v2, which is what coder0 reads
3to2 d <in.tfwc2> <out.tfwc3>     v2 -> v3, the way back
./build.sh                        # no dependencies
```

`coder0` reads `FX2TFWC2`. [zmix](https://zmix.frostbyrne.io/) v1.0 ships its
retrained transformer weights as `FX2TFWC3` only, and its source package
contains the v3 **decoder** but not the encoder (`experiments/gen7-weights-v3/
wcode3.py` is referenced, not included). So this is the encoder for both
containers, written from that decoder, whose header comment specifies the
format exactly.

**No tensor value changes in either direction.** The v3 container can carry a
`DT_F32` tensor as bfloat16 (`ENC_BF16_F32`) — that rounding is the model
exporter's business, not a converter's. Going v2 → v3 a float tensor is written
as bfloat16 only when every value in it already *is* one, which is exactly true
of a file this tool produced from a v3 one and essentially never otherwise. So
`c` then `d` gives back the bytes it started from, and an upstream v2 blob keeps
every bit it had.

Every conversion checks itself: the file just written is decoded again and
compared tensor by tensor with what came out of the input.

## What the two containers are

Both are one adaptive range-coded stream (LZMA-style, 11-bit probabilities,
shift 5). v3 is v2 with three changes, all in the zmix reader's header comment:

| | v2 | v3 |
| --- | --- | --- |
| int4 weights | 11-bit binary tree | adaptive 15-symbol frequency counts, one model per weight *class* (a byte in the metadata) |
| names | length + order-2 character model | skeleton (each digit run → one `0x00`) front-coded against an earlier skeleton, plus LEB128 varints; shape dims are varints too |
| raw f32 | `ENC_PLANE4` | `ENC_BF16_F32` available — lossy, and the reason the two blobs below are not bit-identical |

`rope.sin` and `rope.cos` are in neither: the loader recomputes them from
`rope.inv_freq` with a bit-exact port of CUDA's `sinf`/`cosf`, so 32 MB of table
never reaches the file. This tool passes the encoding byte across and never
needs that port — which is why keeping the tensors in their original file order
matters, and why `Weights` here is a list and not the reader's `unordered_map`.

## Checked against the shipped blobs

`zmix-src.tar.gz` v1.0 carries the same model in both containers, which is a
ground truth to test against. Comparisons are decodes by **zmix's own reader**
(`third_party/fx2_transformer/weights_io_compressed.cpp`, `-DFX2_WEIGHTS_V3=1`),
not by this tool.

| check | result |
| --- | --- |
| `3to2 c 6m-q4-fp32.tfwc3` vs the source `.tfwc3` | **PASS** — 0 of 434 tensors differ |
| the same output vs upstream `6m-q4-fp32.tfwc2` | 88 tensors, 28 280 values, max \|Δ\| 0.0097 — *exactly* the 88 raw trained f32 tensors zmix documents as bf16-carried |
| `c` then `d` then `c` | byte-identical |
| `3to2 d` output vs `wcode3.py`'s `6m-q4-fp32.tfwc3` | identical tensors, **identical size** (2 840 417 B), different stream |

That last row is worth a word: the two encoders reach the same size to the byte
but not the same bytes. A range-coded stream diverges from the first differing
decision and never resyncs, so "same size, different content" means the choices
this encoder makes (which earlier skeleton to front-code against, which class id
goes to which tensor) are as cheap as `wcode3.py`'s without being the same ones.
Reproducing that blob bit-for-bit would need the script itself.

## Sizes

| file | bytes | sha256 |
| --- | ---: | --- |
| `6m-q4-fp32.tfwc2` (upstream, in this repo) | 2 930 652 | `7f4db6c8…` |
| `6m-q4-fp32.tfwc3` (zmix) | 2 840 417 | `c8919e6e…` |
| ↳ `3to2 c` → `.tfwc2` | 2 874 742 | `e07c1dda…` |
| **`6m-q4-fp32-t1lambda1.tfwc3`** (zmix, retrained) | 2 815 630 | `8e8ef3ac…` |
| ↳ **`3to2 c` → `.tfwc2`** | **2 845 074** | `d1ac7095…` |

The converted v2 files are *smaller* than the upstream v2 blob even though v2 is
the fatter container, because their 88 raw f32 tensors have 16 zero bits each
after the bf16 rounding and the plane models eat that for free.

## `tch2bin.py` — a checkpoint as a weights file

```
python3 tch2bin.py <ckpt.tch> <out.bin>            quantized, like the shipped blobs
python3 tch2bin.py <ckpt.tch> <out.bin> --fp32     the 111 matrices left as fp32
```

Reads upstream's PyTorch checkpoints (a `torch.save` zip) with no torch, only
numpy, and writes the uncompressed `FX2TFW01` container every loader in `../tf/`
accepts. Quantized output applies upstream's own quantizer with the checkpoint's
learned row scales; from `models/6m-q4-fp32.tch` in
[fx2-cmix-transformer-v1](https://github.com/astOwOlfo/fx2-cmix-transformer-v1)
it reproduces `6m-q4-fp32.tfwc2` with 0 mismatches in every tensor. `--fp32` is
for a checkpoint from before quantization-aware training (`models/6m.tch` has
no scales), and needs the `TF_FP32=1` engine, whose loader takes a plain
`.weight` where it finds one. `../WEIGHTS.md` section 7 is what it was built to
measure.

## Getting the weights

They are not in this repository; both are GPL-3 assets of other projects.

```sh
curl -LO https://zmix.frostbyrne.io/v1.0/zmix-src.tar.gz    # 13 122 268 B
tar xzf zmix-src.tar.gz
./build.sh
./3to2 c zmix-src/assets/6m-q4-fp32-t1lambda1.tfwc3 ../6m-q4-fp32-t1lambda1.tfwc2
cd .. && ./coder0 c book1000 /tmp/out 6m-q4-fp32-t1lambda1.tfwc2
```

## Does coder0 work with the retrained weights?

Yes, with no change to `coder0` at all — it loads, compresses and decompresses.
The blob is a different model (408 of 434 tensors differ from gen-7) but the
same architecture, the same 205-token vocabulary and the same tensor names, so
`Transformer::SelfCheck` and `BuildMap` are satisfied. Round trips verified.

| weights | `book1000` (4 KB) | `book1wrt`[:64K] |
| --- | ---: | ---: |
| `6m-q4-fp32.tfwc2` (upstream gen-7) | 1 807 | 25 396 |
| `6m-q4-fp32.tfwc3` → `.tfwc2` (same model, bf16-rounded) | 1 806 | 25 404 |
| **`6m-q4-fp32-t1lambda1.tfwc2`** (zmix retrained) | **1 802** | **25 333** |

Both effects are small and both have the sign zmix reports. The bf16 rounding
costs 8 bytes on the 64 KB file (+0.03 %), which is its "+0.00004 nats/token".
The retrained weights save 63 bytes (−0.25 %) — zmix measured −10 318 B at its
20 MB tier, and −0.25 % is the same order once the fixed cost is out of it.

Note that zmix trains these weights against its own λ\* objective, whose point is
a *cheaper container* at equal cross-entropy; the compression gain above is
measured through coder0's mixer and PPMD/Tangelo prior, which is not the pipeline
they were tuned in. Reading more into 63 bytes than that would be a mistake.

## Licence

This file is derived from GPL-3 sources and is therefore **GPL-3**, unlike the
rest of this repository (MIT):

* *fx2-cmix-transformer* (Vladimir Ivanov) — `weights_io.h`,
  `weights_io_compressed.cpp`: the FX2TFWC1/2 container, the range coder and its
  model set.
* *zmix* v1.0, `third_party/fx2_transformer/weights_io_compressed.cpp` — the
  FX2TFWC3 reader, whose header comment specifies the format this implements the
  writer for.

`../tf/` already vendors the first of those, so the directory was GPL-3 before
this was added; nothing outside `tf003/` is affected.
