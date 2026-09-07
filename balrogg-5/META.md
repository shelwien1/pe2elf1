# What remains in the tsv2wav meta

Measured over 39 files with the libvorbis-assisted build
(`-DBLR_VORBIS -I vorbislib`). Every figure is a byte count of the meta
files actually produced, not an estimate.

Two corpora, because they are shaped very differently and averaging them
hides what each one shows:

* **the 17-file corpus** -- real Ogg Vorbis from six libvorbis releases,
  dominated by two long files that between them are 88% of the bytes;
* **the 22 ffmpeg files** -- six seconds each, written by ffmpeg's own
  encoder (`-c:a vorbis`), made by `psyfit/fftest.sh`. Short files where
  the per-stream fixed costs are still visible.

## Totals

| | 17-file corpus | 22 ffmpeg files |
|---|---:|---:|
| source Ogg | 12,734,656 | 515,872 |
| balrogg TSV | 108,163,951 | 4,386,774 |
| meta, self-contained build | 15,682,793 (14.50%) | 1,748,355 (39.85%) |
| **meta, with libvorbis** | **14,092,491 (13.03%)** | **1,087,653 (24.79%)** |

The percentage is of the TSV, which is what the meta stands in for; the WAV
carries the rest. Reconstruction from WAV plus meta is exact for all 39
files, and each restored TSV rebuilds its original Ogg byte for byte.

The two builds differ only in whether libvorbis is compiled into the same
translation unit. On the ffmpeg files the difference is 34% of the meta,
nearly all of it codebooks; on the 17-file corpus it is 3%, because those
files are long enough that the setup header is rounding error.

## Where the meta goes

17-file corpus:

| part | tags | bytes | share |
|---|---|---:|---:|
| Digit corrections | `kd.v`, `kd.i` | 9,646,221 | 68.45% |
| Floor posts | `flr.y`, `flr.d` | 2,439,670 | 17.31% |
| Class corrections | `kc.v`, `kc.i` | 1,064,720 | 7.56% |
| Page framing | `page.*` | 356,995 | 2.53% |
| Per-packet scalars | `pk` | 328,179 | 2.33% |
| Correction counts | `kn` | 219,820 | 1.56% |
| Floor/residue/mapping setup | `flr.*`, `res.*`, `map.*`, `mode.*` | 22,601 | 0.16% |
| Stream header | `id.*`, `link.*`, `cmt.*` | 8,086 | 0.06% |
| Learned models | `cm.*`, `vf.*` | 5,870 | 0.04% |
| **Codebooks** | `cb.*` | **329** | **0.00%** |

22 ffmpeg files:

| part | bytes | share |
|---|---:|---:|
| Digit corrections | 459,299 | 42.23% |
| Floor posts | 458,948 | 42.20% |
| Learned models | 36,214 | 3.33% |
| Correction counts | 35,023 | 3.22% |
| Per-packet scalars | 31,284 | 2.88% |
| Page framing | 23,000 | 2.11% |
| Class corrections | 18,031 | 1.66% |
| Floor/residue/mapping setup | 17,336 | 1.59% |
| Stream header | 8,056 | 0.74% |
| Codebooks | 462 | 0.04% |

## What each part is, and why it is still there

**Digit corrections -- 68% / 45%.** Where the residue walk read a different
digit from the one the stream had. The recovery MDCTs the WAV it wrote,
divides by the stream's own floor and rounds; a correction is one place where
that rounding landed in the wrong bin.

How often it lands right is now measured directly rather than inferred. On
`ff_48000_q10`, of every digit the walk reads:

| | share of digits |
|---|---:|
| recovered exactly | 89.0% |
| wrong, but within one step of right | 1.7% |
| wrong by a whole step or more | 9.3% |

and of the wrong ones, 37% are in blocks the PCM does not span at all -- a
link edge, where there is nothing to analyse and every digit is a correction
by construction. **That is the ceiling for anything done to the WAV:** at
most about 15% of the corrections are near enough to the boundary that a
better sample could flip them. It is why the encoder-side knobs below are
worth one to nine percent and not more.

**How they are written matters as much as how many there are.** The list is
`(index, value)` pairs and there are three ways to put it down; the packet's
`kn` count says which, in its sign and its low bit, and mode `c` prices all
three:

| form | `kn` | what goes out |
|---|---|---|
| sparse | `n` | the gap to each correction on `kd.i`, then the values on `kd.v` |
| raw | `-2t` | all `t` digits of the packet on `kd.v`, zeros run-coded |
| marked | `-(2t+1)` | the same `t` positions on `kd.v`: right is a zero and joins a run, wrong carries `zig(v)+1` |

The marked form is the sparse list with the indices left out. What separates
two corrections is a run marker rather than a gap, and two corrections side by
side need nothing between them at all -- and corrections cluster, which is why
it wins. It took the corpus down 5.15% on its own and `00000009` by 7.24%;
`kd.i` fell from 4,681,080 bytes to 303,695, and `kc.i` from 342,288 to
50,492. Together with the class values below, the two changes are 6.97% of
the corpus.

Two things that sound right and are not. Storing `want - got` instead of
`want` is **worse** (+4.1%): a correction's true digit is usually zero, and
the difference from a wrong guess is not. And dropping `kd.i` altogether,
keeping only the two block forms, is a mere 0.04% behind keeping all three --
but it regresses badly on packets where the walk is useless, so all three
stay.

Rejected here: zeroing unobservable channels, a deadzone in the digit walk
(monotonically worse from 0.50 to 0.75), and carrying the encoder's lowpass
cutoff. Re-encoding the WAV predicts these *worse* than the current recovery,
because the recovery holds the stream's true floor and a re-encoder has to
guess it.

**Floor posts -- 16% / 40%.** `flr.d` carries the difference from a floor
refitted by libvorbis's own psychoacoustic model, run over the decoded PCM
with the stream's block sizes forced; `flr.y` carries the posts directly
where no encoder setting fits. On the ffmpeg files no setting fits at all --
they were not written by libvorbis -- so all of it is `flr.y`, which is why
this part is 40% there and 16% here.

The ceiling was measured by feeding the ported `floor1_fit` the *true* floor
as its own mask: even a perfect mask gives 2.30 bits against 3.44, so about a
third of this stream is the most any predictor can remove.

**Class corrections -- 7.6% / 1.7%.** Where the fitted classifier chose a
different ladder. libvorbis picks the class from two measures of the
quantized residue against `classmetric1/2`, which are encoder-side and not in
the bitstream, so they are fitted per residue and carried -- a few dozen
integers standing in for tens of thousands of class records. What remains is
largely classes whose ladders are interchangeable, which no predictor can
separate.

These go out through the same three forms as the digits, with one difference.
A digit is signed and is zigzagged so that its sign never costs a byte of its
own and never looks like a run marker; a class is an index into a ladder and
cannot be negative, so zigzagging one would double it for nothing -- class
five would cost the two digits of ten. Skipping it took `kc.v` from 2.97
bytes a value to 2.33, and 99.3% of the values are now a single digit, which
is as far as this can go: a frequency remap has nothing left to save.

**Page framing -- 2%.** `page.plen` dominates: segment lengths are packet
byte sizes and are not derivable without rebuilding the packets. `page.type`,
`page.seq` and `page.serial` are already derived from position and link.

**Per-packet scalars -- 2%.** Mode, next-window flag and the floor-use bits,
packed into one `pk` row per packet.

**Correction counts -- 1.5% / 3.2%.** Two numbers per packet: how many
corrections follow, and which of the three forms above carries them. Merged
onto one row, since a row apiece spent more on tag text than on the values.
This grew when the marked form arrived -- a block form's count is the
packet's whole digit count where the sparse form's is only how many were
wrong -- and it is charged to the forms that cause it, so the choice pays for
its own row.

**Codebooks -- 0.00%.** This was 18.6% when the work started and 2.3% a
little later. libvorbis does not build codebooks when it encodes: `lib/books/`
holds static tables and a setup template points at them, so a stream can name
its set instead of carrying it. What kept it from being zero was that the
*compiled-in* libvorbis is only one encoder among many, and the tables moved
-- revised for 1.1 and again for 1.3, with the managed-bitrate path drawing
on a different residue family in every release. `vbooks_gen.inc` now carries
the sets of 19 libvorbis releases, 15 aoTuV betas and ffmpeg's own encoder --
1346 codebooks in 350 sets, generated by `psyfit/bookdump.sh`. Every file in
both corpora names its set; 329 bytes over 17 files is the index itself.

**Floor/residue/mapping setup, stream header, learned models -- under 2%
together.** Partition sizes, post lists, coupling steps, comments, and the
fitted class metrics and floor quality. The learned models are 3.9% on the
ffmpeg files only because those files are six seconds long; they are a fixed
cost per stream and they replace far more than they cost.

## The WAV is the tool's own output

Everything above is a correction to something read back from the WAV, and the
WAV is not given -- `tsv2wav c` writes it. How a sample is placed between two
integers is therefore an encoder-side choice with no format consequence at
all: mode `d` reads whatever was written and needs to be told nothing.

Three knobs were measured over 26 to 37 files:

| knob | result |
|---|---|
| the lifting's rounding rule (nearest, floor, ceiling, truncate) | within 0.03% of each other; a non-lever |
| the sample split point (floor, nearest, ceiling, and quarter steps) | nearest is best; 0.375 or 0.625 cost 0.4-0.7%, floor or ceiling about 1% |
| first-order error feedback on the quantization | **the one that pays** |

Feeding a fraction `h` of each sample's quantization error into the next
shapes the error spectrum by `1 - h/z`, draining it away from the low
frequencies where most of the digits are. It pays very unevenly, and not with
one sign: a fixed coefficient is wrong, because `h = 0.6` everywhere is 6.3%
worse on the 17-file corpus even though it is 11.5% better on two of its
files. So mode `c` tries four coefficients and keeps the best, which carries
nothing -- mode `d` reads back whatever was written.

| | before | after | |
|---|---:|---:|---:|
| 17-file corpus | 15,191,473 | 15,148,489 | **-0.28%** |
| 22 ffmpeg files | 1,149,911 | 1,117,924 | **-2.78%** |

The corpus figure is small because 88% of its bytes are two long files that
both choose no feedback at all; the files that do move, move a lot --
`00000003` and `0000000F` by 11.5% each, `ff_16000_q10` by 12.9%. Nothing
regresses, since zero feedback is always among the candidates.

Two things that sound like they should matter and do not. Second-order
shaping adds nothing measurable over first-order. And forming the sample
product in double rather than float -- which is the right thing to do, since
a float product of about 32767 carries only nine bits below the point, and
the bits below the point are the whole of the rounding decision -- is worth
0.00% once the sweep is running. It was worth 3.3% on one file *before* the
sweep existed, which is the same thing said differently: both are ways of
nudging where samples land, and the sweep already finds a good nudge.

The sweep costs about 25% more runtime, which is less than it sounds:
scoring a candidate is done with the floor refit switched off, and that refit
is 97% of what mode `c` otherwise spends. Scoring that way was checked over
37 files to pick the same coefficient as scoring the whole meta.

## Cost per value

What a single value occupies, tag text and separator included, over the
17-file corpus.

| tag | bytes | values | bytes/value |
|---|---:|---:|---:|
| `kd.v` | 9,342,526 | 3,998,333 | 2.34 |
| `flr.y` | 1,427,732 | 529,509 | 2.70 |
| `kc.v` | 1,014,228 | 435,082 | 2.33 |
| `flr.d` | 1,011,938 | 395,934 | 2.56 |
| `pk` | 328,179 | 109,422 | 3.00 |
| `kd.i` | 303,695 | 113,272 | 2.68 |
| `kn` | 219,820 | 38,674 | 5.68 |
| `page.plen` | 176,779 | 38,096 | 4.64 |
| `page.granlo` | 58,404 | 3,008 | 19.42 |
| `kc.i` | 50,492 | 11,308 | 4.47 |

`kn` was 7.93 before its two counts were merged onto one row, and the keep
values fell from 2.67 to 2.15 once they were zigzagged so a minus sign stopped
costing a byte. `page.granlo` is one row per page for one number and still
carries the per-record slack that merging fixed elsewhere.

## Notes on method

Every number here is a byte count of files on disk, produced by the build in
this tree. Several earlier figures in this work were wrong because they
compared the wrong things -- coded residuals against absolute posts, a new
build against a stale baseline, or struct-literal text where the numbers were
what mattered -- so each claim above was re-measured against the build that
produced this document.

The corpora are not the ones earlier versions of this document used, so the
shares are not comparable with them file for file. What is comparable is the
direction: codebooks went 18.6% to 2.3% to 0.00%, and digit corrections are
now what is left.

Two files, `00000001` and `0000000B`, come out two bytes larger under the
marked form than without it. The choice is made per packet against a cost
model that cannot see how rows merge across packet boundaries; two bytes in
136,621 is what that blind spot is worth, and it is not worth removing.
