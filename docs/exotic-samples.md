# Generating exotic JPEG samples with the JPEG XT reference codec

`pjpg` has to survive the whole of Rec. ITU-T T.81, not just the baseline
subset that `cjpeg`/`libjpeg-turbo` can produce.  Frame types like
lossless-predictive (SOF3), differential/hierarchical (SOF5..SOF7, DHP, EXP)
and the arithmetic variants (SOF9..SOF11) have essentially no mainstream
encoder.  The one implementation that can emit all of them is the JPEG XT
reference codec:

> **https://github.com/thorfdbg/libjpeg** — Thomas Richter's complete
> ISO/IEC 10918-1 implementation with the 18477 (JPEG XT) extensions.

This document records the exact command lines that produce each exotic frame
type, what markers come out, what `pjpg` makes of them, which option
combinations the encoder rejects, and the small source patch that is required
before the encoder will emit SOF3 / SOF11 / JPEG-LS at all.

Everything below was verified against commit `702114c` ("Fixed another
misplaced bracket") of `thorfdbg/libjpeg`.

---

## 1. Building the encoder

```sh
git clone https://github.com/thorfdbg/libjpeg
cd libjpeg
./configure
make -j"$(nproc)"
```

Two things to know before you go looking for a binary that isn't there:

* The build produces a **single** executable called **`jpeg`** in the source
  root.  There is no `cjpeg` and no `djpeg` — this is not libjpeg-turbo and it
  shares no command-line syntax with it.
* `jpeg` is both the encoder and the decoder.  It decodes by default; passing
  `-q`, `-p` or `-ls` switches it into encoding mode.

```sh
./jpeg -q 75 in.ppm out.jpg     # encode
./jpeg out.jpg out.ppm          # decode
./jpeg                          # print the (long) option list
```

The codec only reads and writes PNM/PFM.  It exits with status **0 even when
encoding fails**, and prints the reason to stdout mixed in with the copyright
banner, so scripts must test the output file rather than `$?`:

```sh
./jpeg "$@" in.ppm out.jpg >log 2>&1
[ -s out.jpg ] || { grep -m1 'error -' log; exit 1; }
```

## 2. Test inputs

A 64x48 synthetic image is enough to exercise every frame type and keeps the
samples small.  16-bit and 12-bit inputs are needed for the deep-precision and
refinement cases (PNM stores samples big-endian when `maxval > 255`).

```python
#!/usr/bin/env python3
# mkin.py -- synthetic inputs for the thorfdbg/libjpeg encoder
W, H = 64, 48
def px(x, y):
    return ((x * 4) & 255, (y * 5) & 255, ((x ^ y) * 3) & 255)

rgb8  = bytes(b for y in range(H) for x in range(W) for b in px(x, y))
gray8 = bytes(px(x, y)[0] for y in range(H) for x in range(W))
open("in.ppm", "wb").write(b"P6\n%d %d\n255\n" % (W, H) + rgb8)
open("in.pgm", "wb").write(b"P5\n%d %d\n255\n" % (W, H) + gray8)

rgb16 = b"".join(((v * 257) & 0xFFFF).to_bytes(2, "big")
                 for y in range(H) for x in range(W) for v in px(x, y))
open("in16.ppm", "wb").write(b"P6\n%d %d\n65535\n" % (W, H) + rgb16)

g12 = b"".join(((px(x, y)[0] * 16) & 0x0FFF).to_bytes(2, "big")
               for y in range(H) for x in range(W))
open("in12.pgm", "wb").write(b"P5\n%d %d\n4095\n" % (W, H) + g12)
```

## 3. Recipes

Marker columns below are the raw `!tag=XX!` trace that `pjpg` prints, in
stream order.  `rc` is `pjpg`'s exit status (0 = clean, 1 = errors reported).

Marker legend used throughout:

| | | | | | |
|---|---|---|---|---|---|
| `D8` SOI  | `D9` EOI  | `DA` SOS  | `DB` DQT  | `C4` DHT  | `CC` DAC |
| `DD` DRI  | `DC` DNL  | `DE` DHP  | `DF` EXP  | `F7` SOF55 (JPEG-LS) | `F8` LSE |
| `E0` APP0/JFIF | `E1` APP1/Exif | `EB` APP11/JPEG XT | `EE` APP14/Adobe | | |
| `C0` SOF0 baseline | `C1` SOF1 ext.seq | `C2` SOF2 progressive | `C3` SOF3 lossless | | |
| `C5` SOF5 diff.seq | `C6` SOF6 diff.prog | `C7` SOF7 diff.lossless | | | |
| `C9` SOF9 seq.arith | `CA` SOF10 prog.arith | `CB` SOF11 lossless.arith | | | |

### 3.1 Sequential and progressive DCT (stock encoder)

| Recipe | Frame | Markers | rc |
|---|---|---|---|
| `jpeg -q 75 -bl in.ppm out.jpg` | SOF0 baseline | `D8 E0 DB EB C0 C4 DA D9` | 0 |
| `jpeg -q 75 in.ppm out.jpg` | SOF1 extended sequential *(default!)* | `D8 E0 DB EB C1 C4 DA D9` | 0 |
| `jpeg -q 75 -v in.ppm out.jpg` | SOF2 progressive | `D8 E0 DB EB C2 C4 DA ... D9` | 0 |
| `jpeg -q 75 -a in.ppm out.jpg` | SOF9 sequential, arithmetic | `D8 DB C9 CC DA D9` | 0 |
| `jpeg -q 75 -v -a in.ppm out.jpg` | SOF10 progressive, arithmetic | `D8 DB CA CC DA ... D9` | 0 |
| `jpeg -q 75 -c in.ppm out.jpg` | SOF1, **no** RGB->YCbCr transform | `D8 E1 DB EE EB EB C1 EB C4 DA D9` | 0 |
| `jpeg -q 75 in.pgm out.jpg` | SOF1 grayscale | `D8 E1 DB EE EB EB C1 EB C4 DA D9` | 0 |
| `jpeg -q 75 -n in.ppm out.jpg` | DNL-terminated height | `D8 E0 DB EB C1 C4 DA DC D9` | 0 |
| `jpeg -q 75 -z 4 in.ppm out.jpg` | restart interval 4 MCU | `D8 E0 DB DD EB C1 C4 DA D9` | 0 |
| `jpeg -q 75 -s 1x1,2x2,2x2 in.ppm out.jpg` | 4:2:0 subsampling | `D8 E0 DB EB C1 C4 DA D9` | 0 |
| `jpeg -q 75 -oz -qt 3 -h in.ppm out.jpg` | optimised tables ("JPEG on Steroids") | `D8 E0 DB EB C1 C4 DA D9` | 0 |

Note that the **default is SOF1, not SOF0** — `-bl` is required for a baseline
frame.  Whenever the encoder cannot signal the colour space with a plain JFIF
APP0 (`-c`, or a single-component image) it switches to an Exif APP1 + Adobe
APP14 + JPEG XT APP11 preamble, which is itself a useful marker-parsing test.

### 3.2 Hierarchical / differential frames (stock encoder)

`-y <levels>` builds a hierarchical progression.  Level 0 is the special
"lossless second scan over the DCT residuals" mode; level 1 downscales the
initial scan by a power of two; levels >= 2 add that many refinement stages.

| Recipe | Frames | Markers | rc |
|---|---|---|---|
| `jpeg -q 75 -y 2 -h in.ppm out.jpg` | DHP + SOF1 + EXP + SOF5 | `D8 DB DE C1 C4 DA DF C5 C4 DA D9` | 0 |
| `jpeg -q 75 -y 3 -h in.ppm out.jpg` | DHP + SOF1 + 2x(EXP+SOF5) | `D8 DB DE C1 C4 DA DF C5 C4 DA DF C5 C4 DA D9` | 0 |
| `jpeg -q 75 -y 4 -h in.ppm out.jpg` | DHP + SOF1 + 3x(EXP+SOF5) | `D8 DB DE C1 C4 DA DF C5 C4 DA DF C5 C4 DA DF C5 C4 DA D9` | 0 |
| `jpeg -q 75 -y 1 -h in.ppm out.jpg` | DHP + SOF1 + EXP + **SOF7** (diff. lossless) | `D8 E0 DB EB DE C1 C4 DA DF C7 C4 DA D9` | 0 |
| `jpeg -q 100 -c -y 0 -h in.ppm out.jpg` | DHP + SOF1 + EXP + SOF7, true lossless | `D8 DB EE EB DE C1 C4 DA DF C7 C4 DA D9` | 0 |
| `jpeg -q 75 -y 2 -v -h in.ppm out.jpg` | DHP + SOF2 + EXP + **SOF6** (diff. progressive) | `D8 DB DE C2 ...(10 scans)... DF C6 ...(10 scans)... D9` | 0 |
| `jpeg -q 75 -y 2 -z 4 -h in.ppm out.jpg` | hierarchical + restarts | `D8 DB DD DE C1 C4 DA DF C5 C4 DA D9` | 0 |

`pjpg` resolves the frame geometry correctly across the stage boundary, e.g.
for `-y 2`:

```
Start Of Frame 0xde: width=64, height=48, components=3
  Define Hierarchical Progression, 8-bit, Huffman
Start Of Frame 0xc1: width=32, height=24, components=3
  Sequential frame, 8-bit, Huffman
Expand Reference Component: Eh=1 Ev=1
Start Of Frame 0xc5: width=64, height=48, components=3
  Differential sequential frame, 8-bit, Huffman
```

### 3.3 Lossless DCT and JPEG XT residual streams (stock encoder)

| Recipe | What it is | Markers | rc |
|---|---|---|---|
| `jpeg -q 100 -l -c -h in.ppm out.jpg` | int-to-int lossless DCT, bit-exact | `D8 E1 DB EE EB EB C1 EB C4 DA D9` | 0 |
| `jpeg -q 75 -Q 90 -r -h in.ppm out.jpg` | JPEG XT residual layer | `D8 E1 E0 DB EB EB C1 EB EB C4 DA D9` | 0 |
| `jpeg -q 75 -Q 90 -r -rv -h in.ppm out.jpg` | XT residual, progressive residual | `D8 E1 E0 DB EB EB C1 EB EB C4 DA D9` | 0 |
| `jpeg -q 75 -Q 90 -r -r12 -h in.ppm out.jpg` | 12-bit residual image | `D8 E1 E0 DB EB EB C1 EB EB C4 DA D9` | 0 |
| `jpeg -q 100 -Q 100 -r -rl -c -h in.ppm out.jpg` | XT lossless (residual int-to-int DCT) | `D8 E1 DB EE EB EB C1 EB EB C4 DA D9` | 0 |
| `jpeg -q 90 -Q 100 -r -ro -h in.ppm out.jpg` | near-lossless, spatial residual quantisation | `D8 E1 E0 DB EB EB C1 EB EB C4 DA D9` | 0 |
| `jpeg -q 75 -R 4 -h in16.ppm out.jpg` | 16-bit source, 4 refinement bits hidden in APP11 | `D8 E1 E0 DB EB EB EB C1 EB x11 C4 DA D9` | 0 |
| `jpeg -q 75 -al in.pgm -h in.ppm out.jpg` | alpha channel in a separate XT box | `D8 E1 E0 DB EB EB EB C1 EB EB C4 DA D9` | 0 |

`-R n` hides `n` extra bits of a deep input in JPEG XT APP11 boxes.  It needs
an input deeper than 8 bit; on an 8-bit PPM it fails with
*"error -1028 - can only hide at most the number of extra bits between the
native bit depth of the image and eight bits per pixel"*.

### 3.4 Lossless predictive (SOF3 / SOF11) — **requires the patch in §5**

| Recipe | Frame | Markers | rc |
|---|---|---|---|
| `jpeg -p -h in.ppm out.jpg` | SOF3 lossless, Huffman (YCbCr) | `D8 C3 C4 DA D9` | 0 |
| `jpeg -p -c -h in.ppm out.jpg` | SOF3, **true lossless** (no colour transform) | `D8 EE C3 C4 DA D9` | 0 |
| `jpeg -p -a in.ppm out.jpg` | SOF11 lossless, arithmetic | `D8 CB CC DA D9` | 0 |
| `jpeg -p -c -a in.ppm out.jpg` | SOF11, true lossless | `D8 EE CB CC DA D9` | 0 |
| `jpeg -p -h in.pgm out.jpg` | SOF3 grayscale | `D8 EE C3 C4 DA D9` | 0 |
| `jpeg -p -c -h in12.pgm out.jpg` | SOF3, **12-bit** | `D8 EE C3 C4 DA D9` | 0 |
| `jpeg -p -c -h in16.ppm out.jpg` | SOF3, **16-bit** | `D8 EE C3 C4 DA D9` | 0 |
| `jpeg -p -h -y 2 in.ppm out.jpg` | DHP + SOF3 + EXP + SOF7 | `D8 DE C3 C4 DA DF C7 C4 DA D9` | 0 |

`-c` is what makes it *actually* lossless: without it the RGB->YCbCr transform
runs in integer arithmetic and loses a little.  All of the `-c` variants above
round-trip bit-exact.

`pjpg` decodes the lossless SOS header, including the fact that in a lossless
scan `Ss` is the predictor selector and `Al` is the point transform:

```
Start Of Frame 0xc3: width=64, height=48, components=3
  Lossless frame, 16-bit, Huffman
  predictor=4, point transform=0
```

#### Selecting the predictor and point transform

The patch in §5 adds two environment-variable overrides, because the demo
front-end has no command-line switch for either:

```sh
JPEG_PREDICTOR=1      jpeg -p -c -h in.ppm pred1.jpg   # predictors 1..7
JPEG_POINTTRANSFORM=2 jpeg -p -c -h in.ppm pt2.jpg     # point transform (lossy!)
```

All seven T.81 predictors encode and round-trip bit-exact:

| `JPEG_PREDICTOR` | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| size (bytes) | 4328 | 4615 | 5166 | 2992 | 4001 | 4265 | 4257 |
| round-trip | exact | exact | exact | exact | exact | exact | exact |

A non-zero point transform discards low bits, so those samples are *not*
bit-exact by design — they are still useful for checking that `pjpg` reports
`Al` correctly.

### 3.5 JPEG-LS (SOF55) — **requires the patch in §5**

These are T.87 / ISO 14495-1 streams, a different standard that merely reuses
the JPEG marker framing.

| Recipe | What it is | Markers | rc |
|---|---|---|---|
| `jpeg -ls 0 -c in.ppm out.jpg` | scan-interleaved | `D8 EE F7 DA <desync> D9` | **1** |
| `jpeg -ls 1 -c in.ppm out.jpg` | line-interleaved | `D8 EE F7 DA <desync> D9` | **1** |
| `jpeg -ls 2 -c in.ppm out.jpg` | sample-interleaved | `D8 EE F7 DA <desync> D9` | **1** |
| `jpeg -ls 0 in.pgm out.jpg` | grayscale | `D8 EE F7 DA <desync> D9` | **1** |
| `jpeg -ls 0 -cls in.ppm out.jpg` | part-2 pseudo-RCT, emits an **LSE** (`F8`) marker | `D8 F8 F7 DA <desync> D9` | **1** |

See §6 — this is a genuine, currently-unimplemented gap in `pjpg`, not an
encoder problem: the files decode correctly with the reference decoder.

## 4. Option combinations the encoder rejects

Worth knowing so you don't chase phantom bugs.  All of these print an error and
leave a zero-byte output file while still exiting 0.

| Combination | Message |
|---|---|
| `-y N` without `-h` | `error -1042 - Huffman table is unsuitable for selected coding mode - try to build an optimized Huffman table` |
| `-y N` with `-a` | `error -1034 - arithmetic coding is always adaptive and does not require to measure the statistics` |
| `-p` or `-ls` on the **stock** build | `error -1031 - DQT marker missing, no quantization table defined` |
| `-R n` on an 8-bit input | `error -1028 - can only hide at most the number of extra bits ...` |
| `-p -z N` (restarts in a lossless scan) | `warning -1038 ... found restart marker in the middle of the line, expect corrupt results`, output is empty |
| `-d` (documented as "encode the DC band only") | `unsupported command line switch -d` — listed in the usage text but not implemented |
| `-ls -m N` (near-lossless) | **segfaults** the encoder |

So: **hierarchical always needs `-h`, and hierarchical can never be
arithmetic** in this encoder.

## 5. The patch required for SOF3 / SOF11 / JPEG-LS

Out of the box the demo front-end **cannot produce a single lossless-predictive
or JPEG-LS file**.  Every `-p` and `-ls` invocation dies with

```
writing a JPEG file failed - error -1031 - DQT marker missing, no quantization table defined
```

because the encoder-side table assignment unconditionally requires a DQT, which
a lossless-predictive or JPEG-LS frame legitimately does not have.  Three source
edits fix it.  Apply them, re-run `make`, and keep the stock binary around as
an independent decoder to validate what the patched one writes.

### 5.1 `codestream/tables.cpp` — do not demand a DQT

`Tables::QuantizationTableIndexOf()` is also (ab)used to pick the
component -> table mapping for the Huffman/AC tables, so returning the mapping
is the right answer rather than throwing:

```diff
@@ -1499,8 +1499,11 @@ class QuantizationTable *Tables::FindQuantizationTable(UBYTE idx) const
 // This is for encoder-side quantization table assignment.
 UBYTE Tables::QuantizationTableIndexOf(UBYTE component,bool separatechroma) const
 {
+  // PATCH: lossless-predictive and JPEG-LS frames legitimately have no DQT.
+  // This function is also (ab)used to pick the component->table mapping for
+  // the Huffman/AC tables, so just return the mapping instead of throwing.
   if (m_pQuant == NULL)
-    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::QuantizationTableIndexOf","DQT marker missing, no quantization table defined");
+    return (separatechroma && component > 0)?1:0;
 
   if (separatechroma) {
     if (component == 2 && m_pQuant->hasCompleteTables())
```

### 5.2 `marker/scan.cpp` — let the caller pick the lossless predictor

Without this the predictor is hard-wired and only one of the seven T.81
predictors is ever exercised:

```diff
@@ -744,6 +744,14 @@ void Scan::InstallDefaults(UBYTE depth,ULONG tagoffset,const struct JPG_TagItem
   case ACDifferentialLossless:
     m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_POINTTRANSFORM            ,m_ucLowBit);
     m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_POINTTRANSFORM + tagoffset,m_ucLowBit);
+    // PATCH: allow the caller to select the lossless predictor (Ss field, 1..7).
+    if (type == Lossless || type == ACLossless) {
+      m_ucScanStart  = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_START            ,m_ucScanStart);
+      m_ucScanStart  = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_START + tagoffset,m_ucScanStart);
+      if (m_ucScanStart > 7)
+        JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
+                  "lossless predictor must be between 1 and 7");
+    }
     if (m_ucLowBit >= m_pFrame->PrecisionOf())
       JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
                 "Point transformation removes more bits than available in the source data");
```

### 5.3 `cmd/encodec.cpp` — expose it on the command line

The demo front-end has no switch for either field; environment variables are
the least invasive way in:

```diff
@@ -46,6 +46,7 @@
 */
 
 /// Includes
+#include <stdlib.h>
 #include "cmd/encodec.hpp"
 #include "interface/hooks.hpp"
 #include "interface/tagitem.hpp"
@@ -625,6 +626,11 @@ void EncodeC(const char *source,const char *ldrsource,const char *target,const c
             JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                            ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan7:pscan7),
             JPG_ValueTag((lsmode >= 0)?JPGTAG_SCAN_LS_INTERLEAVING:JPGTAG_TAG_IGNORE,lsmode),
+            // PATCH: env-var overrides for lossless predictor / point transform
+            JPG_ValueTag((lossless && getenv("JPEG_PREDICTOR"))?JPGTAG_SCAN_SPECTRUM_START:JPGTAG_TAG_IGNORE,
+                         getenv("JPEG_PREDICTOR")?atoi(getenv("JPEG_PREDICTOR")):4),
+            JPG_ValueTag(((lossless||lsmode>=0) && getenv("JPEG_POINTTRANSFORM"))?JPGTAG_SCAN_POINTTRANSFORM:JPGTAG_TAG_IGNORE,
+                         getenv("JPEG_POINTTRANSFORM")?atoi(getenv("JPEG_POINTTRANSFORM")):0),
             JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,flt),
             JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,flt),
             JPG_PointerTag(alphain?JPGTAG_ALPHA_TAGLIST:JPGTAG_TAG_IGNORE,alphatags),
```

### 5.4 Why these samples are trustworthy

The patch only removes an encoder-side guard; it does not change the bitstream
format.  Every SOF3/SOF11 file produced by the **patched** encoder was fed back
through the **unpatched, stock** decoder and round-tripped **bit-exact** against
its source PNM:

```
pred_sof3c  (-p -c -h  in.ppm)   BIT-EXACT
pred_sof11c (-p -c -a  in.ppm)   BIT-EXACT
pred16      (-p -c -h in16.ppm)  BIT-EXACT
zl          (-q 100 -l -c -h)    BIT-EXACT
JPEG_PREDICTOR=1..7              all BIT-EXACT
```

So the files are conformant streams, not artefacts of the patch.

## 6. Results against `pjpg`

48 samples come out of the generator in §7:

* **43 parse cleanly** (`rc=0`) — every DCT frame type (SOF0/1/2/9/10), every
  lossless-predictive frame (SOF3/SOF11, Huffman and arithmetic, 8/12/16-bit,
  all seven predictors, point transforms 0..2), every hierarchical/differential
  combination (DHP, EXP, SOF5/SOF6/SOF7), DNL, restarts, subsampling, and the
  whole JPEG XT APP11 residual/alpha family.
* **5 fail** (`rc=1`) — all of them JPEG-LS.

Only three warnings are emitted across the whole set, all of them by design:

| count | warning | why |
|---|---|---|
| 18 | `[tag 00] lossless scan, entropy data not decoded` | `pjpg` implements the DCT entropy decoders, not the lossless-predictive one |
| 2 | `[tag 00] lossless arithmetic scan, entropy data not decoded` | same, for SOF11 |
| 1 | `[tag C1] frame height is 0, expecting DNL` | correct: that is exactly what `-n` produces |

### One bug this exercise found

`pjpg` warned `lossless predictor selector outside 1..7` on every *differential*
lossless scan (SOF7, produced by `-y 0`, `-y 1` and `-p -y 2`).  That was a
false positive: T.81 H.2.2 codes the difference against the upsampled reference
directly and uses no prediction at all, so a differential lossless scan
**must** carry `Ss = 0`.  The reference decoder agrees — `marker/scan.cpp` in
thorfdbg/libjpeg splits the check the same way:

```
case Lossless: case ACLossless:
  if (m_ucScanStart == 0 || m_ucScanStop > 7)     // "must be between 1 and 7"
case DifferentialLossless: case ACDifferentialLossless:
  if (m_ucScanStart != 0)                         // "must be zero"
```

`tag_SOS.inc` now branches on `sof_diff` and still catches both violations
(`Ss != 0` on SOF7, `Ss` outside 1..7 on SOF3).

### The JPEG-LS gap

`pjpg` has no SOF55 (`0xFF F7`) handler and no LSE (`0xFF F8`) handler, so a
JPEG-LS stream is parsed like this:

```
!tag=D8!
!tag=EE!
APP14: Adobe
  Adobe marker: version 100, flags 0x0000/0x0000, transform 0
!tag=F7!
!tag=DA!
Start Of Scan: 1 components; l_tag=5
ERROR [tag DA]: scan before any frame header
!tag=7F!
!tag=7F!
...
parse finished with 3 error(s) and 0 warning(s); first error: scan before any frame header (bad scan header)
```

Two distinct problems, both inherent to T.87 rather than to any particular file:

1. **No frame handler.** SOF55 is skipped as an unknown marker, so no frame
   state exists and the following SOS is rejected.
2. **Different byte stuffing.** T.81 escapes a literal `FF` in entropy-coded
   data as `FF 00`.  T.87 instead uses bit-stuffing: after an `FF`, the next
   byte always has its MSB clear, so `FF 01` .. `FF 7F` are all *data*, not
   markers.  `pjpg`'s marker scanner therefore resynchronises on every such
   pair, which is where the stream of spurious `!tag=7F!` lines comes from.

Fixing this means teaching the outer scanner a JPEG-LS mode (accept SOF55, park
the LSE marker, and switch the `FF`-escape rule for the duration of a T.87
scan).  It is deliberately out of scope here — the samples are recorded so the
gap is reproducible when someone wants to close it.  Note that the failure is
a clean, reported parse error, not a crash or a hang.

## 7. One-shot generator

```sh
#!/usr/bin/env bash
# mkexotic.sh -- regenerate the whole sample set.
#   $STOCK   = stock ./jpeg from thorfdbg/libjpeg
#   $PATCHED = ./jpeg from the same tree with the §5 patch applied
set -u
: "${STOCK:?set STOCK to the stock jpeg binary}"
: "${PATCHED:=$STOCK}"
python3 mkin.py

gen() { # gen <encoder> <name> <args...>
  local e=$1 n=$2; shift 2
  rm -f "$n.jpg"
  "$e" "$@" >"$n.log" 2>&1
  if [ -s "$n.jpg" ]; then printf '  %-14s %7d B\n' "$n" "$(stat -c%s "$n.jpg")"
  else printf '  %-14s FAILED  %s\n' "$n" "$(grep -m1 'error -' "$n.log" | cut -c1-70)"; fi
}

echo "sequential / progressive:"
gen "$STOCK" base_sof0   -q 75 -bl  in.ppm base_sof0.jpg
gen "$STOCK" base_sof1   -q 75      in.ppm base_sof1.jpg
gen "$STOCK" prog_sof2   -q 75 -v   in.ppm prog_sof2.jpg
gen "$STOCK" arith_sof9  -q 75 -a   in.ppm arith_sof9.jpg
gen "$STOCK" aprog_sof10 -q 75 -v -a in.ppm aprog_sof10.jpg
gen "$STOCK" rgb_sof1    -q 75 -c   in.ppm rgb_sof1.jpg
gen "$STOCK" gray_sof1   -q 75      in.pgm gray_sof1.jpg
gen "$STOCK" dnl         -q 75 -n   in.ppm dnl.jpg
gen "$STOCK" rst         -q 75 -z 4 in.ppm rst.jpg
gen "$STOCK" sub411      -q 75 -s 1x1,2x2,2x2 in.ppm sub411.jpg
gen "$STOCK" opt_huff    -q 75 -oz -qt 3 -h in.ppm opt_huff.jpg

echo "hierarchical / differential (-h is mandatory, -a is rejected):"
gen "$STOCK" hier_y2   -q 75 -y 2 -h in.ppm hier_y2.jpg
gen "$STOCK" hier_y3   -q 75 -y 3 -h in.ppm hier_y3.jpg
gen "$STOCK" hier_y4   -q 75 -y 4 -h in.ppm hier_y4.jpg
gen "$STOCK" hier_y1   -q 75 -y 1 -h in.ppm hier_y1.jpg
gen "$STOCK" hier_y0c  -q 100 -c -y 0 -h in.ppm hier_y0c.jpg
gen "$STOCK" hier_prog -q 75 -y 2 -v -h in.ppm hier_prog.jpg
gen "$STOCK" hier_rst  -q 75 -y 2 -z 4 -h in.ppm hier_rst.jpg

echo "lossless DCT and JPEG XT residual:"
gen "$STOCK" dct_lossless -q 100 -l -c -h in.ppm dct_lossless.jpg
gen "$STOCK" xt_resid  -q 75 -Q 90 -r -h in.ppm xt_resid.jpg
gen "$STOCK" xt_rprog  -q 75 -Q 90 -r -rv -h in.ppm xt_rprog.jpg
gen "$STOCK" xt_r12    -q 75 -Q 90 -r -r12 -h in.ppm xt_r12.jpg
gen "$STOCK" xt_rl     -q 100 -Q 100 -r -rl -c -h in.ppm xt_rl.jpg
gen "$STOCK" xt_ro     -q 90 -Q 100 -r -ro -h in.ppm xt_ro.jpg
gen "$STOCK" p16       -q 75 -R 4 -h in16.ppm p16.jpg
gen "$STOCK" alpha     -q 75 -al in.pgm -h in.ppm alpha.jpg

echo "lossless predictive (needs the patched encoder):"
gen "$PATCHED" pred_sof3   -p -h in.ppm pred_sof3.jpg
gen "$PATCHED" pred_sof3c  -p -c -h in.ppm pred_sof3c.jpg
gen "$PATCHED" pred_sof11  -p -a in.ppm pred_sof11.jpg
gen "$PATCHED" pred_sof11c -p -c -a in.ppm pred_sof11c.jpg
gen "$PATCHED" pred_gray   -p -h in.pgm pred_gray.jpg
gen "$PATCHED" pred12      -p -c -h in12.pgm pred12.jpg
gen "$PATCHED" pred16      -p -c -h in16.ppm pred16.jpg
gen "$PATCHED" pred_hier   -p -h -y 2 in.ppm pred_hier.jpg
for p in 1 2 3 4 5 6 7; do
  JPEG_PREDICTOR=$p gen "$PATCHED" "pred_p$p" -p -c -h in.ppm "pred_p$p.jpg"
done
for t in 1 2; do
  JPEG_POINTTRANSFORM=$t gen "$PATCHED" "pred_pt$t" -p -c -h in.ppm "pred_pt$t.jpg"
done

echo "JPEG-LS (needs the patched encoder; pjpg cannot parse these yet):"
gen "$PATCHED" ls0     -ls 0 -c in.ppm ls0.jpg
gen "$PATCHED" ls1     -ls 1 -c in.ppm ls1.jpg
gen "$PATCHED" ls2     -ls 2 -c in.ppm ls2.jpg
gen "$PATCHED" ls_gray -ls 0 in.pgm ls_gray.jpg
gen "$PATCHED" ls_rct  -ls 0 -cls in.ppm ls_rct.jpg
```

Then check them all:

```sh
for f in *.jpg; do ../011_/pjpg "$f" >/dev/null 2>&1 || echo "FAIL $f"; done
```
