# What remains in the tsv2wav meta

Measured over the 26-file corpus with the libvorbis-assisted build
(`-DBLR_VORBIS -I vorbislib`). Every figure is a byte count of the
meta files actually produced, not an estimate.

## Totals

| | bytes | of TSV |
|---|---:|---:|
| source Ogg | 4623253 | -- |
| balrogg TSV | 41240780 | 100% |
| meta, self-contained build | 3199177 | 7.76% |
| **meta, with libvorbis** | **2553633** | **6.19%** |

The WAV carries the rest. Reconstruction from WAV plus meta is exact for
all 26 files, and each restored TSV rebuilds its original Ogg byte for byte.

## Where the meta goes

| part | tags | bytes | share |
|---|---|---:|---:|
| Floor posts | `flr.y`, `flr.d` | 1165767 | 45.7% |
| Digit corrections | `kd.i`, `kd.v` | 642721 | 25.2% |
| Per-packet scalars | `pk` | 217104 | 8.5% |
| Class corrections | `kc.i`, `kc.v` | 205155 | 8.0% |
| Page framing | `page.*` | 149384 | 5.9% |
| Codebooks | `cb.*` | 59368 | 2.3% |
| Correction counts | `kn` | 54372 | 2.1% |
| Floor/residue/mapping setup | `flr.*`, `res.*`, `map.*`, `mode.*` | 33128 | 1.3% |
| Stream header | `id.*`, `link.*`, `cmt.*` | 19202 | 0.8% |
| Learned models | `cm.*`, `vf.*` | 7146 | 0.3% |
| **total** | | **2553347** | |

## What each part is, and why it is still there

**Floor posts — 45.7%.** The largest item, and the most worked. `flr.d` carries
the difference from a floor refitted by libvorbis's own psychoacoustic model
run over the decoded PCM with the stream's block sizes forced; `flr.y` carries
the posts directly where no encoder setting fits. The refit reproduces about
48% of posts exactly at the matched quality and cuts the stream 30–37% where it
applies, on 12 of 26 files.

What was tried and rejected: predicting the floor from windowed spectrum
statistics (worse than storing it), delta-coding against the previous packet
(5.4% worse), a per-band offset fit over a ported noise mask (5.46 bits against
a 3.37 cost basis), and a neighbour-residual predictor (helps some files, hurts
others, so it is chosen per file). The ceiling was measured by feeding the
ported `floor1_fit` the *true* floor as its own mask: even a perfect mask gives
2.30 bits against 3.44, so about a third of this stream is the most that any
predictor can remove.

**Digit corrections — 25.2%.** Where the residue walk read a different digit
from the one the stream had. The sources are enumerated and mostly inherent:
blocks at a link edge that the PCM does not span, coupled pairs where one
channel has no floor, and type-1 digits placed past the end of a partition.

Rejected here: zeroing unobservable channels (218k → 376k on a six-channel
stream), a deadzone in the digit walk (monotonically worse from 0.50 to 0.75),
and the encoder's own lowpass cutoff (2 bytes, because the residue walk already
stops at `res.end`). Re-encoding the WAV predicts these *worse* than the current
recovery — 95.36% against 97.11% — because the recovery holds the stream's true
floor and a re-encoder has to guess it.

**Per-packet scalars — 8.5%.** Mode, next-window flag and the floor-use bits,
packed into one `pk` row per packet. `aud.wprev` is derived from the previous
block's flag and costs nothing unless the derivation fails.

**Class corrections — 8.0%.** Where the fitted classifier chose a different
ladder. libvorbis picks the class from two measures of the quantized residue
against `classmetric1/2`, which are encoder-side and not in the bitstream, so
they are fitted per residue and carried — a few dozen integers standing in for
tens of thousands of class records. That took corrections from 53% to 14.7% on
the worst file. What remains is largely classes whose ladders are
interchangeable, which no predictor can separate.

**Page framing — 5.9%.** `page.plen` dominates: segment lengths are packet byte
sizes and are not derivable without rebuilding the packets. `page.type`,
`page.seq` and `page.serial` are already derived from position and link. The
granule positions are one row per page for one number each and still carry the
per-record slack that merging fixed elsewhere.

**Codebooks — 2.3%.** Only the two streams whose books libvorbis cannot
reproduce. libvorbis does not build codebooks when it encodes; `lib/books/`
holds static tables and a setup template points at them, so 24 of 26 files
carry a quality index instead of their books. This part was 18.6% before that
change.

**Correction counts — 2.1%.** Two numbers per packet: how many corrections
follow and whether they are sparse or a raw run. Merged onto one row, since a
row apiece spent more on tag text than on the values.

**Floor/residue/mapping setup — 1.3%.** Partition sizes, post lists, coupling
steps. Small and not obviously compressible.

**Stream header — 0.8%.** Identification, comments, link boundaries.

**Learned models — 0.3%.** The fitted class metrics, the floor predictor's
quality, offset and predictor choice, and the codebook index. Together they are
a third of a percent and they replace far more than that.

## Cost per value

What a single value occupies, tag text and separator included. This is where
the recent wins came from: `kn` was 7.93 before its two counts were merged onto
one row, and the keep values fell from 2.67 to 2.15 once they were zigzagged so
a minus sign stopped costing a byte.

| tag | bytes | values | bytes/value |
|---|---:|---:|---:|
| `flr.d` | 688980 | 255188 | 2.70 |
| `flr.y` | 476787 | 177270 | 2.69 |
| `kd.v` | 334586 | 155680 | 2.15 |
| `kd.i` | 308135 | 121125 | 2.54 |
| `pk` | 217104 | 72381 | 3.00 |
| `kc.v` | 110479 | 33886 | 3.26 |
| `page.plen` | 99370 | 24293 | 4.09 |
| `kc.i` | 94676 | 26633 | 3.55 |
| `kn` | 54372 | 14176 | 3.84 |
| `cb.used` | 26402 | 13041 | 2.02 |
| `cb.len` | 24629 | 9116 | 2.70 |
| `page.granlo` | 19451 | 1043 | 18.65 |

## Notes on method

Every number here is a byte count of files on disk. Several earlier figures in
this work were wrong because they compared the wrong things — coded residuals
against absolute posts, or a new build against a stale baseline — so each claim
above was re-measured against the build that produced this document.

The two builds differ only in whether libvorbis is compiled into the same
translation unit. Both reconstruct all 26 files exactly; the self-contained
build is 7.76% and needs no external source.
