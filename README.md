# mtf2ima

Lossless structural repack between **MT Framework ADPCM** (Capcom's
`adpcm_ima_mtf`, as carried in `.fwse` files) and **IMA-ADPCM WAV**.

A companion to `ima2ms` in the same style: the tool moves the 4-bit code
stream from one container into the other, byte for byte, and puts everything
that cannot be derived from the codes into a small side file, so that
`c` followed by `d` reproduces the input exactly.

```
mtf2ima c|d [-q] [-bN] input output metainfo.bin

  c = repack into the other format (.fwse <-> IMA wav, detected from the
      input's magic)
  d = rebuild the original file from output + metainfo.bin

  -q  quiet
  -bN IMA block size in bytes for c (default 2048), rounded down to
      4*channels*(groups+1)
```

## Why the two formats line up

Both spend one 4-bit code per sample and share the same 89-entry step table,
so the code streams are interchangeable through the bijection that preserves
the order of the reconstruction levels. MTF codes are already in level order
— its delta is `step*(2n-15)`, rising monotonically from `n = 0` to `n = 15` —
while IMA codes are sign-magnitude:

```
MTF n  ->  IMA c = (n >= 8) ? n - 8 : (8 | (7 - n))
```

This pair fits together much more closely than IMA and MS do:

* **The ladders are the same rungs.** For `n >= 8` the MTF delta is
  `step*(2m+1)` with `m = n-8`; the IMA delta for magnitude `m` is
  `step*(2m+1)/8` exactly (that is what the shift-and-add ladder computes, up
  to its truncation). One is the other scaled by 8.
* **The step-index tables agree rung for rung.** ffmpeg's
  `mtf_index_table = {8,6,4,2,-1,…,-1,2,4,6,8}` indexed by the code is IMA's
  `{-1,-1,-1,-1,2,4,6,8}` indexed by magnitude, under the map above, and both
  clamp to `0..88`. So the repacked file does not merely carry the codes, it
  reproduces the source's **step-index trajectory sample for sample**.

What does not survive is the recursion. IMA integrates, MTF leaks:

```
IMA:  p = clip16 (p + delta)
MTF:  p = clip16 ((p + 8*delta) >> 4)
```

MTF is very nearly a memoryless adaptive quantiser — the old predictor comes
back attenuated by 16 — so the two reconstructions diverge from the first
sample however the scales are matched, and no fixed code map can prevent it.
The output is a conformant IMA-ADPCM WAV (correct `fmt` chunk, valid block
geometry, decoded by ordinary decoders) but it is **not the same audio**:
measured against the source it comes out at r ≈ −0.22 on average over the
sample set, |r| ≤ 0.6. The file is a container for the code stream; restoring
the audio means restoring the source file, which is what `d` does.

## Geometry

Both formats pack two codes of one channel per byte, so a chunk of the MTF
stream and the payload of one IMA block hold the same codes in the same number
of bytes. Only the packing order differs: MTF puts the earlier code in the
high nibble and interleaves channels every byte, IMA puts it in the low nibble
and interleaves every 4 bytes. Each IMA block costs `4*channels` bytes of
header on top.

* **MTF → IMA** — the block header has no counterpart in the source, so it is
  synthesised from the MTF decoder state at the block boundary: the step index
  is *exactly* right (the trajectories agree) and the predictor is the MTF
  one. Being a function of the codes it costs nothing in the metainfo — `d`
  ignores block headers entirely.
* **IMA → MTF** — the block header has nowhere to go, so it goes to the
  metainfo: predictor and step index as residuals against the previous block's,
  and the "reserved" byte as an exception list.

The source container header (the 1 KiB FWSE header, or the RIFF header) is
stored run-length coded, with the fields that are functions of the payload —
FWSE file size and duration — blanked out and rebuilt. The target's header is
generated. A CRC-32 of the source is verified after restoring; a CRC-32 of the
target sample stream detects a mismatched pair. The forward direction runs its
own inverse in memory before writing anything.

## Build

```
make
```

## Verification

`test/roundtrip.sh [-v] file…|dir` repacks and restores every file and checks
that the restore is byte-identical and that ffmpeg accepts the repack.

`test/mtf_check.py orig.fwse packed.wav [orig.pcm]` is the stronger check: it
reads the codes back out of the produced IMA wav, maps them to MT Framework
codes, runs ffmpeg's `adpcm_ima_mtf` reconstruction over them and compares the
result with what ffmpeg itself decodes the original `.fwse` to. It also checks
that every IMA block header holds the MTF state at that code position, and
reports the correlation between the two decodes.

Against the 408-file MT Framework sample set (mono, 48 kHz, 9.6 MB of payload)
and ffmpeg 6.1.1:

```
408 ok, 0 failed; 10098116 bytes in, 9725140 repacked, 84107 metainfo
408/408 files: codes recovered from the IMA wav, MTF-decoded == ffmpeg,
               all block headers == MTF state
```

Metainfo runs about 200 bytes per file, nearly all of it the opaque middle of
the FWSE header. The repack is smaller than the source overall because the
1 KiB FWSE header moves to the side file and the IMA block headers cost less
than it.

Also verified: both directions on 1- and 2-channel data, ffmpeg-encoded IMA
wavs at block sizes 1024/2048/4096 repacked to `.fwse` and decoded back by
ffmpeg, and a 116-case matrix of edge geometries (empty payload, 1–15 byte
payloads, odd lengths, payloads not a multiple of the group size, runt final
blocks, minimum-size blocks, non-multiple-of-`4*channels` block sizes).
