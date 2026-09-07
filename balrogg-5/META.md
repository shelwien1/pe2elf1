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
| **meta, with libvorbis** | **13,252,200 (12.25%)** | **1,007,208 (22.96%)** |

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
| Digit corrections | `kd.v`, `kd.i` | 9,583,647 | 72.32% |
| Floor posts | `flr.y`, `flr.d` | 2,293,894 | 17.31% |
| Class corrections | `kc.v`, `kc.i` | 666,361 | 5.03% |
| Per-packet scalars | `pk` | 291,734 | 2.20% |
| Page framing | `page.*` | 202,487 | 1.53% |
| Correction counts | `kn` | 190,658 | 1.44% |
| Floor/residue/mapping setup | `flr.*`, `res.*`, `map.*`, `mode.*` | 11,254 | 0.08% |
| Stream header | `id.*`, `link.*`, `cmt.*`, `pg.pred` | 6,411 | 0.05% |
| Learned models | `cm.*`, `vf.*` | 5,510 | 0.04% |
| **Codebooks** | `cb.*` | **159** | **0.00%** |

22 ffmpeg files:

| part | bytes | share |
|---|---:|---:|
| Floor posts | 445,044 | 44.19% |
| Digit corrections | 424,290 | 42.13% |
| Learned models | 35,899 | 3.56% |
| Correction counts | 30,405 | 3.02% |
| Per-packet scalars | 27,808 | 2.76% |
| Page framing | 15,320 | 1.52% |
| Class corrections | 13,764 | 1.37% |
| Floor/residue/mapping setup | 8,470 | 0.84% |
| Stream header | 5,966 | 0.59% |
| Codebooks | 242 | 0.02% |

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

**The walk's mistakes are predictable from each other, where there are
enough of them.** The digits are not -- the floor has taken the envelope out
and their autocorrelation is +0.009 at lag one -- but the error runs -0.42 at
lag one and +0.32 at lag two, and a digit following a wrong one is wrong 74%
of the time against 11% overall. LPC on that fails, because the error is zero
nine times in ten and a rounded continuous estimate breaks more right answers
than it mends (+40% corrections at order 8). A conditional mode over the two
previous errors, clipped, with a margin favouring zero, works: it needs no
side information at all, since both roles see the same errors in the same
order.

It pays only where the walk is weak. On the ffmpeg files, right 69% to 89% of
the time, it takes 15-29% of the corrections away and 2.21% of the meta; on
the corpus, right 98% or better, it finds nothing and costs a little. One
flag per stream, set by running the model in shadow during the fit pass,
decides -- and on these two corpora it comes out 22 on and 17 off, which is
the whole story.

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

**What predicts a class is the class that partition had last time.** The
metric fit looks at one partition's residue and nothing else, so it cannot
see that a spectral envelope holds still from packet to packet and that
partition *p* tends to want the ladder it wanted before. iczelia's balrogg
conditions its class coder on exactly that (`src/vorbis.h`, `AR_PCLS`), and
the same context works here: an adaptive table over the fit's own guess, the
class this partition had in the previous packet, and the class the previous
partition turned out to be.

Over three files the walk-order predecessor alone takes 4-16% of the class
corrections, the previous packet alone 6-26%, and the two together **20-30%**.
Nothing is carried but a flag. It took class corrections from 7.31% of the
corpus meta to 5.03%, and the meta itself down 2.48%.

The flag has to be scored in a meta pass, not the fit pass: the fit's guess
is only real once the fit has settled, and in the fit pass it is not computed
at all. Scoring it there measured a prediction against a constant zero, which
is how the first attempt at this came out half as good as it should have.

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

## What the format spends on saying which record is which

An audit of the 70 tags a meta can hold found no value that is wrong and none
that is never read back. What it did find was that the *names* were most of
the cost of the small records, and that one of them was saying nothing.

**The tags.** A tag goes out once per row, and the meta changes tag every few
values, so `page.granlo` was costing 19.3 bytes a value and `page.granhi`
14.0 -- against `pk`'s 3.0, which merges. Tag text and row breaks came to
853,594 bytes, **5.63% of the meta**. The meta is read by `tsv2wav` and by
nothing else, so its tags do not have to be the ones balrogg chose: they are
now one or two characters, handed out shortest-first by row count, with the
stream translating on the way out and back. No call site changed, and the tee
still hands over balrogg's names.

**The granule.** It went out as two 32-bit halves, and the high half was zero
on all 3,187 pages -- it stays zero until a stream passes 2^32 samples, which
is twenty-seven hours at 44.1 kHz. It is now one number, and a difference
from the page before: a small number from a small set, 120 distinct values
over 3,187 pages and a third of them exactly 4096, because a page's granule
climbs by the samples its packets put out.

| | before | after | |
|---|---:|---:|---:|
| 17-file corpus | 14,092,627 | 13,589,096 | **-3.57%** |
| 22 ffmpeg files | 1,063,591 | 1,009,013 | **-5.13%** |

Files whose meta is mostly setup gain most -- `00000005` by 15.9%,
`00000003` by 15.3% -- and files that are almost all correction values gain
least, `00000009` by 1.0%, because there the tag was never the problem.

Since a page's granule *is* the samples its packets put out, that difference
can be predicted rather than stored, which is what iczelia's balrogg does --
see TSV2WAV.md 4.8 for the accounting and what it is worth here. It is not
worth much: the residual is narrower than the difference only where a page's
packets are all one block size, so the form is chosen per stream and written
down only when it wins. It takes 189 bytes off the ffmpeg files (-0.019%) and
170 off the corpus (-0.001%), and no file comes out larger.

What is left of the row overhead is 2.99%, spread evenly over the four
per-packet records. Taking more of it means merging rows across packets,
which needs a whole link buffered before anything is written.

Three findings from the audit were left alone. `page.eos` and `aud.wpfix`
carry a value that is redundant with the record's presence, but a TSV record
must have a value, so the saving is two bytes an occurrence -- 693 in all.
And `id.rate`, `id.channels` and `link.frames` are recoverable from the WAV
-- `pcmwin::open` already *checks* the first two against its fmt chunk -- but
they come to 1,824 bytes and removing them would remove that check.

## Cost per value

What a single value occupies, tag text and separator included, over the
17-file corpus.

| tag | bytes | values | bytes/value |
|---|---:|---:|---:|
| `kd.v` | 9,292,192 | 3,998,333 | 2.32 |
| `flr.y` | 1,365,492 | 529,509 | 2.58 |
| `flr.d` | 928,402 | 395,934 | 2.34 |
| `kc.v` | 545,912 | 228,204 | 2.39 |
| `pk` | 291,734 | 109,422 | 2.67 |
| `kd.i` | 291,455 | 113,272 | 2.57 |
| `kn` | 190,658 | 38,546 | 4.95 |
| `page.plen` | 152,715 | 38,096 | 4.01 |
| `kc.i` | 120,449 | 46,976 | 2.56 |
| `page.gran` | 20,978 | 3,008 | 6.97 |

`kn` was 7.93 before its two counts were merged onto one row, and the keep
values fell from 2.67 to 2.15 once they were zigzagged so a minus sign stopped
costing a byte. `page.gran` is still the dearest of these: one row per page
for one number, so it pays a whole record's slack for a value that is usually
two or three digits. The gap-and-run form the keeps use would close it, but a
page's granule is one value, and there is nothing to merge it with.

An earlier version of this table was measured before the tags were shortened
and before the keeps were recoded, and had gone stale against the rest of the
document: it put the class corrections at 1,064,720 bytes where the breakdown
above says 666,361, and it still named `page.granlo`, a tag the format no
longer has. These numbers are from the files the breakdown was measured on,
and add up to it: `kd.v` + `kd.i` to 9,583,647, `flr.y` + `flr.d` to
2,293,894, `kc.v` + `kc.i` to 666,361.

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
