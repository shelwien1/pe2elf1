# The tsvcomp model: design, bugs and room to improve

This describes the model as it stands at the tree that produced the
`tsvcompG` column of `log.txt`: 11,372,019 bytes over the 17-file corpus,
1.57% under balrogg 1.3, decoding at 1.1x to 1.9x the Rev F time.  It is the
companion to two other documents: `BALROGG-MODEL.md` says what balrogg's
model does that this one does not, and what each of those was measured to be
worth here; `IDX/IDX-FORMAT.md` is the specification of the `.idx` files the
contexts are declared in.  Neither is repeated here beyond what is needed to
read the code.

Functions are named rather than line-numbered; everything is in
`tsvcomp.cpp` unless a file is given.  Sizes are what the shipping build
maps; "resident" means what the kernel actually faults in for a file.

---

## 1. What is coded

tsvcomp does not see Ogg.  `balrogg` (in this tree, only the transform half
of upstream balrogg) takes a Vorbis stream apart into a record stream, and
`tsvcomp c` walks that record stream in the same order the Vorbis decoder
would, asking each value from the input and handing it to a model.  `tsvcomp
d` runs the identical walk with the roles reversed: each value is answered by
the model and written out.  The walk is in `tc_walk` and the functions it
calls (`tc_page`, `tc_header`, `tc_audio`, `tc_payload`, `tc_residue`,
`tc_part`), and because the shape of what comes next depends on values
already coded, the two directions cannot drift.

Where the bits go, on a 350 kbit/s mono file and a 96 kbit/s stereo one:

| stage | 05 (105,952 B) | 07 (780,525 B) | what it is |
|---|---|---|---|
| headers | 1.80% | 0.29% | the three Vorbis header packets, record by record |
| pages | 0.31% | 0.37% | Ogg page fields as residuals against their expected values |
| packets | 0.03% | 0.00% | mode, window flags, floor-used flags |
| floor | 1.78% (2.01 b/post) | 5.30% (1.37 b/post) | floor1 posts, coded as folded residuals |
| class | 0.91% (1.19 b/class) | 3.18% (1.21 b/class) | residue partition classes |
| digits | 95.17% (2.63 b/digit) | 90.86% (1.15 b/digit) | residue vector digits, magnitude and sign |

The digit model is the program.  Everything else together is under 10%, and
on the high-bitrate file under 5%.

### The six families

Every value goes through one of six instances of `tc_fam`, a family, and each
family has its own context declarations (`IDX/tsvcomp-<fam>.idx`), its own
tables and its own rates.  What differs between them is only which variables
the contexts are built from and which parts of the cascade they use.

| family | codes | entry | sign | prior | tables (MiB) |
|---|---|---|---|---|---|
| dig | residue digit magnitude | `code` | none here | residue book | 714.4 |
| sgn | residue digit sign | `bit(0)` only | is the sign | residue book, via `sq` | 199.9 |
| flr | floor posts | `codes` | one saturating counter | subclass book | 1.8 |
| cls | residue classes | `code` | not used | classbook | 0.2 |
| aux | page and packet fields | `code` / `codes` | 16 x 13 counters | none | 0.1 |
| hdr | header records | `codes` | 128 counters, by tag | none | 12.4 |

Total 928.8 MiB of address space, of which the corpus touches about 57 MB.

---

## 2. One binary decision

Everything the model does is binary.  A value becomes a sequence of yes/no
decisions (section 3), and each decision goes through the same six-line
pipeline in `tc_fam::bit`:

```
p1  = APM1[ s ][ A.p ]                    counter A, corrected by the first APM
pm  = mixer[ m ]( st(p1), st(B.p), st(C.p), st(D.p), st(A.p), st(prior), 256 )
pf2 = APM2[ f ][ pm ]                     the mixer output, corrected again
pf  = ( bw * pm + (16 - bw) * pf2 + 8 ) >> 4
code the bit at pf; then update mixer, both APMs and the four counters
```

`st` is the stretch (log-odds) of a 12-bit probability, and every probability
is P(bit = 0).  `A`..`D`, `s`, `f`, `m` are rows chosen by seven different
context indices computed once per value (`tc_make_<fam>`), each then offset
by the position of the decision in the cascade (the node).  So a family has
seven ways of grouping the same variables, and the .idx decides them all.

### Counters (`cm_cnt`, `cm.inc`)

A counter is a 16-bit probability and a 16-bit count.  Update is the running
mean `p += (target - p) / (t + 2)` with `t` incremented first and capped at
the family's rate `lim`, so a fresh context moves at 1/3 on its first visit
and settles at `1/(lim + 2)`.  All-zero storage means "never visited" and
reads as 1/2, which is what lets the tables stay unfilled.  `mw` bounds the
target away from 0 and 1 (`mw x 64` out of 65536) so a context that has seen
one outcome does not claim certainty.  The first update at 1/3 rather than
1/2 amounts to a fixed prior of one pseudo-count per side.

Four counters per decision, on four indices: `A` (dig: 921,600 rows) and
`B` (2.4 M rows) are two different groupings of the same variables, `C`
and `D` are small (5,400 and 52,920).  In `aux` and `hdr` counter B's rate
is 0, clamped to 1, so B there is a "what happened last time" flag updating
at a fixed 1/3.

### APMs (`cm_apm`)

An APM is a curve with `q` points over the stretch axis, one curve per
context row.  The input probability is stretched, the two bracketing points
are interpolated with a 7-bit weight, and the answer is what that
confidence has actually meant in this context.  Stored per point is the
distance from the identity curve, so an untouched row is an unlearned APM.
Each point carries its own u8 count and updates at `1/(n + 2)` until `n`
reaches `2^rate - 2` (255 at rate 8 and above), which is a counter's rule
applied to an APM.  Both bracketing points are updated with the same step
whatever the interpolation weight was.

APM1 corrects counter A before the mixer; APM2 corrects the mixer's output.
Their widths are parameters: `qs`/`qf` are 33/65 for dig, 49/65 for sgn,
33/33 for aux and 5/5 for hdr, and they are why APM1 is 195.8 MiB in dig,
second only to counter B.

### The mixer (`cm_mix<7>`)

A logistic mixer with seven inputs: APM1(A), B, C, D, raw A, the codebook
prior (or 0 when there is none, which is an even chance saying nothing), and
a bias of 256.  Weights are 16.16 fixed point stored as the distance from
the mean 1/7, one row of seven per mixer context.  Update is `w += x * err *
lr >> 16` with `err` the 12-bit error against the mixer's own output (not
the final `pf`).  `mb` is a boost for young rows: the rate is multiplied by
`1 + mb / (n + 1)` with `n` the row's u8 visit count, so a fresh row learns
fast and decays to `lr`.  dig has `lr 11, mb 68`; the others `lr 18..31`
with `mb 16` (sgn) or 0.  The mantissa has its own three-input mixer
(`mxm`: counter T, prior, bias) with its own `lrm`/`mbm`.

### The blend

`bw` is how much of the final answer is the mixer against APM2, out of 16:
11 for dig, 12 elsewhere.  It is a linear blend in probability space with
one constant per family, not learned and not per context.

### Current parameters

| | dig | sgn | flr | cls | aux | hdr |
|---|---|---|---|---|---|---|
| rA rB rC rD | 255 254 16 227 | 254 255 10 11 | 15 75 63 63 | 80 43 63 63 | 22 1 63 63 | 16 1 63 63 |
| rT / rG | 255 / - | - / - | 15 / 15 | 80 / (80) | 22 / 22 | 16 / 16 |
| mwA..D (x64) | 2 | 0 | 0 | 0 | 0 | 0 |
| rS1 rS2 | 7 7 | 7 7 | 6 6 | 5 5 | 6 6 | 1 1 |
| lr mb | 11 68 | 18 16 | 22 0 | 24 0 | 31 0 | 31 0 |
| lrm mbm | 32 68 | - | 22 0 | 24 0 | 31 0 | 31 0 |
| bw | 11 | 12 | 12 | 12 | 12 | 12 |
| qs qf | 33 65 | 49 65 | 33 65 | 33 65 | 33 33 | 5 5 |

An APM rate of `r` means a point's count stops at `2^r - 2`; 8 and above
are all 255, so 8..15 are the same setting.

---

## 3. A value into decisions: the cascade

`tc_fam::code` turns a non-negative integer into bits without knowing its
range:

```
node 0   is it 0 ?                         yes -> done
node 1   is it 1 ?                         yes -> done
node 2   is it 2 ?                         yes -> done
u = x - 2  (so u >= 1);  nb = bit length of u
node 3+k is nb == k + 1 ?   for k = 0, 1, ...   (unary; k >= 25 share node 28)
then the nb - 1 bits of u below its leading one, most significant first,
each under mantissa node 13 * min(nb, 12) + min(p, 12)   (169 nodes)
```

Every table is `TC_NODE = 30` rows deep per context (or `TC_MNODE = 169` for
the mantissa plane), so where a decision sits in the cascade is context
without being a declared variable.  Node 29 is `TC_SIGN`, the sign coded
through the full pipeline; no family reaches it any more (section 7, B6).

Small values, which is nearly all of them, cost one to three decisions.  A
value of 3 costs four (three ladder steps and `nb == 1`) and no mantissa.
Bit lengths up to 62 are admitted.

**Mantissa bits** (`bitm`) skip the pipeline: one counter `T` on the
mantissa index, and if the codebook prior can speak to the bit, the
three-input mixer above.  What is left after the length is close to
uniform, and a stage that cannot pay for itself is not run four million
times.

**Signs** are coded three different ways:

- A residue digit's sign is a family of its own, `sgn`: the digit's
  magnitude is coded by `dig`, then `tc_part` builds sgn's contexts (which
  include the magnitude and the prior's opinion of the sign) and calls
  `fam_sgn.bit(0, ..)`, the whole pipeline at node 0.  The prior's sign
  probability goes in twice, as the mixer input and as the axis `sq`.
- flr, aux and hdr code through `codes`, which codes the magnitude and then
  the sign with one bare counter `G` under a small context of its own: flr
  has one counter (posts are never negative; it saturates), aux has 16 x 13
  (a threshold of the previous value by field), hdr has one per tag.
- `cls` never codes a sign; it calls `code`.

---

## 4. The codebook prior (`tc_prior.inc`)

A residue digit is one base-`nsym` place of a codebook entry, and the entry
was a Huffman codeword whose length `L` the Vorbis file declares.  A
complete Huffman code is a probability model: `2^-L` per entry.  The prior
is the conditional form of it: per book, a trie of prefixes (`nsym^k` nodes
at level `k`, up to 65,536 nodes, 16 levels and 256 symbols per book, 64 MiB
across all books), and per node the sums the cascade asks for, precomputed:
the weight of `|v| = 0, 1, 2` (`s012`), of each bit length of `|v| - 2`
(`slen`), and per symbol (`pw`).  A node's weights are normalised to
`2^20` and every reachable child keeps at least one unit.

The cursor (`tc_pcur`) restarts at every vector (`vpos == 0`) and steps one
level per digit; a digit the grid does not name turns it off for the rest
of the vector.  What the cascade gets, per decision, is `prp`: P(bit = 0)
from the weights on either side of that decision (`tcp_head`, `tcp_len`,
`tcp_sign`, `tcp_man`), or -1 when there is nothing to say.  `tcp_sign` and
`tcp_man` walk the alphabet at coding time (at most 64 symbols for the
mantissa); the rest is a table read.

The prior is also an axis: `tcp_bucket` puts its log-odds into 32 steps,
keeping 0 for "no prior", and that is `pq` (P(digit = 0)) in dig and `sq`
(P(sign)) in sgn, and the `pri`/`pq` axes of flr and cls.  Classbooks get the
same treatment per classword (most significant place first), floor posts per
subclass book (the post's value selects the subclass, `brm` maps symbol to
branch).  A book too big to tabulate, or whose lengths did not survive
reading, has `ok = 0` and every consumer falls back to no prior.

The corpus's largest trie has 156 nodes, so the bounds are never near.

---

## 5. Contexts and histories

### dig

`tc_part` builds `tc_dv` once per digit; every field is a small integer
after `tc_qlog` (exact to 3, then two buckets per octave) or `tc_sq` (the
same, signed):

| variable | meaning |
|---|---|
| rno, pass | which residue, which of the 8 cascade passes |
| band, col, vpos | partition index (log), position in the partition (log), position in the vector |
| q1, q2 (q1s, q2s) | the two digits before this one in the same (rno, pass, channel) stream, persisting across packets |
| t1, t2 (t1s) | the same slot the last two times this (block size, pass, channel) was coded |
| n1, w1 | the slot one vector later and one vector earlier, last time |
| p0 (p0s), pn, ps | the earlier pass's digit at this slot this packet, how many passes hit it, their signed sum |
| cls, bkq | the partition's class, and the class's codebook half-range (log) |
| zrun | zeros in a row in this stream |
| blk | short or long block |
| ax | `tc_qlog` of a weighted mean of `avg` (a 4.12 running mean of the slot's magnitude, decay `avD/256`) and the six neighbours, weights `avA..avQ2`, scale `avC/4096` |
| pq | the prior's P(zero), 32 buckets |
| chn | `slot mod su.ch` (meant to be the channel of an interleaved type-2 slot) |
| crun, cm | how long the class has held, and whether it is what this partition had last packet |
| mg, sq | (sgn only) the magnitude just coded, the prior's sign probability |

What the seven dig indices actually use (buckets per factor, from
`IDX/tsvcomp-dig.idx`):

| index | rows | factors |
|---|---|---|
| A | 921,600 | pass 8, band 16, col 5, bkq 2, cls 6, blk 2, p0s 2, ax 2, pq 3, chn 5 |
| B | 2,426,112 | pass 3, band 13, col 3, q1 4, bkq 3, cls 2, zrun 3, blk 2, p0 4, pn 2, pq 9, chn 2 |
| C | 5,400 | pass 2, band 3, ax 5, q1s 2, p0s 2, pq 3, ps 3, chn 5 |
| D | 52,920 | vpos 2, ax 7, q1s 9, p0s 7, pq 3, ps 4, chn 5 |
| APM1 | 69,120 | pass 2, band 2, t1 3, bkq 3, t1s 2, p0 3, p0s 2, ax 8, ps 2, chn 5, cm 2 |
| APM2 | 12,960 | pass 3, cls 9, q1s 4, ax 6, band 4, chn 5 |
| mixer | 67,200 | pass 2, bkq 2, cls 10, vpos 8, p0 2, pn 3, pq 7, chn 5 |
| mantissa | 280 | pass 4, cls 7, p0 2, chn 5 |

Note what is absent from A and B: `t1`, the same slot a packet ago, is only
in APM1, and the neighbourhood is only present as `ax`.  The optimizer moved
the temporal context out of the direct counters and into the APM and the
`ax` summary; the comment block in the .idx records why `cls` beats `band`
and why `zrun` looked worthless on the file it was first tuned on.

Histories are per residue: `dg_hist`/`dg_hist2` (last two values of a slot,
keyed by block size, pass and coded-vector index), `dg_avg` (the running
mean), `dg_pp`/`dg_ps`/`dg_pn` (this packet's earlier passes, cleared per
residue call), and `dg_q1`/`dg_q2`/`dg_zr` per (rno, pass, channel).  Spans
over 2^17 digits or 2^23 history slots lose the temporal context but stay
codable.

### sgn

Same `tc_dv`, plus `mg` and `sq`.  A (510,300 rows): pass 3, cls 6, p0s 7,
mg 5, band 2, pq 5, sq 9, ps 9.  B (172,800): pass 3, vpos 2, p0s 8, q1s 5,
q2s 4, band 4, pq 5, sq 9.  What predicts a sign is the earlier pass's sign
at the slot (`p0s`), the signed sum of passes (`ps`), the prior (`sq`) and,
for interleaved stereo, the digit before (`q1s`), which is the other
channel's digit at the same position.

### flr

Posts are coded in list order, not X order, so that the prediction is
available: `prd` is `render_point` between the two bracketing neighbours,
`room`/`hl` the space either side of it, `lov`/`hiv` the neighbours' own
residuals, `p1` this post's residual last packet, `fy` last packet's curve
value, `ep` what floor1 would have coded for last packet's curve against
this packet's prediction (the residual if the envelope had not moved),
`o1`/`od` the channel before's residual and curve difference, `col` the
post's rank in X order (a 48-wide threshold list, 34 buckets in A), `pri`
the subclass-book prior.  A is `fno 2, col 34, fy 2, hl 3`; B is `col 2, p1
13, fy 2, blk 2, lov 9, ep 12`; the mixer is `col 16, p1 2, pri 9`.  All
flr factors are frozen (`!`).

### cls

`prev` (the class before, in coding order, carried across channels and
packets), `t1` (the same partition last packet), `tn` (the next partition
last packet), `prev2`, `band`, `rno`, `blk`, `pq`.  Cheap: 800 rows in A.

### aux

Thirteen fields (`F_MORE .. F_USED`), each coded against its own history:
`fld` is a dense factor, `p1` is whatever the caller passes as context
(usually the previous value quantised, sometimes an expectation such as
`want * 4 + prevtype`), `p2` is the field's last value quantised.  Page
fields are coded as residuals against what position implies (type, serial,
sequence, granule against the previous, packet length against the
previous).

### hdr

Each header record under its tag (a dense factor, up to 65 buckets), the
previous value under that tag whole (`p1r`, 8 bits, mask-selected) and
quantised (`p1`, `p2`), and the run of the same tag (`pos`).  The value
coded is the difference from the previous value under the tag
(BALROGG-MODEL.md sec.7); that same difference on the aux fields costs
4,653 bytes and is not done.

---

## 6. Tables, memory, build and tuning

Each family's `.idx` declares its factors; `IDX/idx2inc.pl` turns them into
`MOD/tsvcomp-<fam>_h.inc` (the table struct, with every `_Volume` a
product of the factor sizes via saturating `tc_vmul`) and `_p.inc` (the
index builder).  The tables of a family are one struct mapped with
`MAP_NORESERVE` (`tc_map`), so what is declared is address space and what
is paid is the pages a stream touches: 929 MiB declared, about 57 MB
resident on any corpus file, and a 38.9 GB tuned set once ran in 304 MB.
`TC_MEMCAP` (64 GB) bounds the declaration; `tc_vfits`/`tc_ifits` check
each component's index against the width it can actually address, so an
oversized .idx is refused rather than wrapped.

The tuning build (`./mk.sh`) keeps every parameter a live `mapping` object
with a `!MAP!` marker so `IDX/opt.pl` can drive the binary; the shipping
build (`./mk.sh release`) folds them, and `./mk.sh check` proves the two
code identically.  `t-guard.sh` checks that a wide index codes with low
residency and that an oversized one is refused.

Widening a context is free to an optimizer whose objective is file size, so
`-DTC_MEMCOST` (tuning builds only) appends `TC_MEMCOST` bytes of 0xFF per
GiB of declared tables: memory has a price, 10 kB per GB by default, and a
widening has to pay for itself.

Thresholds (`1!pattern`) map a variable's range onto buckets and saturate;
masks (`&pattern`) select bits; a pattern of all zeros is one bucket and
costs nothing, which is how a variable that did not pay stays visible to
the search.  The search runs on `opt.lst` (05, 00, 03, 07); flr's rates
tuned on the four small files alone cost 560 bytes on the corpus, which is
why a large file has to be in the objective.

---

## 7. Bugs

None of these breaks decoding: encoder and decoder compute the same wrong
thing.  They are modelling defects, wasted memory, or arithmetic that is
only safe by margin.  Ordered by how much they are likely to matter.

**B1. `chn` is not the channel for type-0/1 residues, nor for more than one
submap.**  `tc_part` sets `d.chn = (pc * psz + i) % su.ch` for every
residue.  For a type-2 residue with a single submap that is the slot's
channel.  For type 0 and 1 the channel is `j` and `chn` is slot parity,
which A, C, D, APM1, APM2, the mixer and the mantissa all split on.  For a
type-2 residue in a stream with several submaps the modulus should be the
submap's channel count, not the stream's.  Fix: pass the residue type and
`nch` into `tc_part`; `chn = type == 2 ? slot % nch : j`.

**B2. Digit and class histories are keyed by coded-vector index, not by
channel.**  For type 0/1 residues `tc_residue` iterates `j` over the
non-zero channels only, so `j` is "the j-th channel that was coded", and
`dg_hist`, `dg_pp/ps/pn`, `dg_q1/q2/zr` and `cl_hist` are all indexed by it.
When a channel falls silent for a packet the other channel's history slides
into its slot.  Rare in coupled stereo (the coupling step forces both
channels non-zero together) but wrong in general.  Fix: map `j` to the true
channel through the `nz` array before keying.

**B3. `n1` and `w1` stride by `dim`, which crosses channels on interleaved
residues.**  `hist[slot +- dim]` is "the same place one vector along" only
when the vector belongs to one channel; on a type-2 residue whose `dim` is
not a multiple of the channel count it is a different channel's slot.  The
comment on `vpos` admits the same.  Every `n1`/`w1` pattern is at zero, so
today it is only what `ax` folds in (`avN1 5, avW1 3`).  Fix: stride by
`dim * nch` for type 2, or by the channel-aligned distance.

**B4. sgn's tables are 30 rows deep for one row used.**  sgn only ever
calls `bit(0)`, but its A..D, APM and mixer tables are laid out `TC_NODE`
deep per context like every family's.  Of its 199.9 MiB, 29/30 is never
addressed; worse, the row a context does use is 120 bytes from the next, so
sgn's counters sit one per cache line instead of sixteen, and sgn is asked
once per non-zero digit.  The same applies to its two APM tables (each
context's `qs`/`qf` points are 30x apart).  Fix: a per-family depth (1 for
sgn) in the Table lines and in `select`.  Decode speed is the expected win,
compression unchanged.

**B5. `chn` allocates five buckets where two are reachable.**  Every dig
index and most sgn ones carry `chn` at `1!1010101` (5 buckets) although
`chn` is 0 or 1 on every stereo file and 0 on mono.  The unreachable
buckets cost no resident memory, but they multiply every `_Size` by 2.5 and
so the rent `TC_MEMCOST` charges, and they inflate `tc_ifits` headroom for
nothing.  dig A would be 368,640 rows rather than 921,600 with identical
output.  Fix: a pattern of `1!1` (or a clamp in `tc_part`).

**B6. Dead code and dead knobs.**  `TC_SIGN` (node 29) is unreachable:
every family that codes signs has a `G` counter, so the `else` branch in
`codes` never runs and the 30th row of every table in every family is
address space for nothing.  `cls` never calls `codes`, so its `G` table,
`rG`, `mwG` and `Index g` are dead; its `Number rG` and `mwG` are tunable
knobs that move nothing.  APM rates 8..15 are one setting.  sgn's `MT`/`WM`
tables (mantissa) exist and are never read.

**B7. The mixer's gradient step can overflow 32 bits.**  `cm_mix::upd`
computes `err = (target - pr) * lr'` and then `x[i] * err` in `int`.  With
dig's `lr 11, mb 68`, a fresh row has `lr' = 11 * 69 = 759`, `|err|` up to
3.1 million, and any input past `|x| = 690` (a counter at about 94%)
multiplies past 2^31 when the row is wrong by the full range.  At pattern
extremes (`lr 64, mb 255`) any `|x| > 32` overflows.  `-fwrapv` makes the
wrap deterministic, so streams still decode, but the step lands with a
wrong sign and magnitude.  Measured: computing the product in `i64` changes
the corpus by +5 bytes (07, 08 and 0A one byte smaller, 09 eight bytes
larger), so at the shipped rates the case is essentially never reached; it
is a hazard for the search, which does visit the extremes, and for any
retune that raises `mb`.  Fix: `(i32) (((i64) x[i] * err) >> 16)`, and
clamp the weights while there.

**B8. `ax` is computed in `int` from unbounded inputs.**  `ex` sums `avg >>
4` and six neighbours times weights, then `ex * avC` is shifted.  `avg`
tops out near 10^6 and the history values are i16, so at current weights
the product stays under 4.5 x 10^8; but `q1`/`q2` are the raw `i32` digits,
unclamped, and at pattern extremes (`avC 1023`, all weights 7) the product
passes 2^31 on legal Vorbis values.  `-fwrapv` again keeps it deterministic;
the context is then noise for that slot.  Fix: compute in `i64`, clamp the
inputs to what `avg` already clamps to (4095).

**B9. The granule field's two context axes are the same axis.**  `tc_page`
codes `F_GRAN` through `tc_auxc(F_GRAN, tc_qlog(tc_last[F_GRAN]), ..)`, and
`tc_auxc` itself supplies `tc_qlog(tc_last[fld])` as the other axis, so
`p1 == p2` for that field.  The intended context was presumably the
difference two back, as `tc_aux` supplies for the fields that go through
it.

**B10. Floor's "channel before" context can be stale.**  `o1`/`od` read the
previous channel's `fl_hist` row for this block size, which that channel
updated this packet only if its floor was used; when it was not, the row is
from some earlier packet.  `fl_hist` is also shared between floor
configurations that different modes may map onto the same channel and
block size, so `hp[p]` can be a post from another floor's layout.  Both are
small on the corpus.

**B11. `cl_last` carries across channels and packets.**  The class model's
strongest axis, `prev`, at the first partition of channel 1 is the last
class of channel 0, and at the first partition of a packet it is the last
class of the previous packet's last channel.  Whether resetting or keying
by channel is better was not measured.

**B12. "One packet ago" is "last time this (block size, pass, channel) was
coded".**  `dg_hist` is keyed by `tc_blk`, so for a stream alternating
short and long blocks `t1` may be several packets old, and `dg_hist2` older
still.  This is by construction (a short block's slot does not correspond
to a long block's), but the variable documentation says "one packet ago".

**B13. The stream header has a spare byte nobody reads.**  `tc_walk` writes
`h[1] = 0` and the decoder ignores it.  It is the natural place for a
per-file profile (section 8, I4) and costs nothing until then.

---

## 8. Points for improvement

Ranked by expected value per unit of decode time, with what has been
measured where it has been.

**I1. Learn the final blend.**  `bw` is one constant per family, a linear
blend in probability space.  Replacing it with a second small mixer (inputs:
`st(pm)`, `st(pf2)`, bias, under a cheap context such as the node) is one
more 3-input mix per bit, and it is how every paq-family coder ends its
pipeline.  Cheap; untested here.

**I2. Weight the APM update by proximity.**  Both bracketing points get the
full step regardless of the 7-bit interpolation weight `w`.  Updating with
`(128 - w)` and `w` scaled rates, or only the nearer point, is the standard
form and costs nothing.  Untested.

**I3. Density for sgn (B4) and the reachable-bucket fix (B5).**  Both are
free in compression and are the cheapest decode-time win available; sgn's
tables become 6.7 MiB and its counters cache-resident.  The decode budget
this buys back is what I6 needs.

**I4. A per-file profile in the spare header byte.**  BALROGG-MODEL.md sec.4
measured per-file APM widths at 0.05% and scalar rates at 0.01%.  A byte
selecting one of a few width/rate sets (chosen by trial encodes, as balrogg
does with three bytes) is the way to take it without a per-file search
inside the format.

**I5. Two-rate counters, or a confidence input.**  A counter gives the
mixer one number.  Giving it two, a fast and a slow probability for the
same context, or the stretch and a count-scaled confidence, is the cheapest
way to let the mixer tell a settled context from a young one.  The
indirect (bit-history) model was measured at 0.013% for 5-18% decode
(BALROGG-MODEL.md sec.1) and is not the way to get this.

**I6. A fifth, hashed counter.**  Measured at -0.02% for +8-32% decode
(sec.2).  Worth taking only once I3 has bought the time back.

**I7. Tune with a large file in the objective, and with `TC_MEMCOST` on.**
`opt.lst` has one large file (07); flr's rates show what tuning on the
small ones alone does (+560 bytes), and the 38.9 GB set shows what tuning
without a price on memory does.  Both are one-line changes to how the
search is run, not to the model.

**I8. Fix B1 and B2, then give the sign model the coupled channel's sign
explicitly.**  Today `q1s` stands in for it on interleaved stereo.  A
proper channel axis plus the other channel's digit at the same slot (from
`dg_pp` of the other channel, this pass) is the context the sgn comment
describes and the code does not quite build.

**I9. Reset or key `dg_q1/q2/zr` and `cl_last` per packet or channel
(B11, B12).**  Cheap experiments; the current carry-over may be neutral or
may be worth a few hundred bytes on the stereo files.

**I10. Decode speed on the digit path.**  Six dependent cache misses per
digit (A, B, C, D, APM1 row, mixer row) with nothing to overlap them against,
since the next digit's contexts depend on this one.  What can be done: keep a
row's node 0..2 counters in one cache line (they are, at 4 bytes each),
prefetch the next partition's `B` row when the class is known (the class is
coded before the digits), and SIMD the 7-input dot product.  balrogg's
SSE2/AVX2 kernels (sec.10) are the reference.

**I11. Sparse prior tries.**  `nbr^k` nodes per level hits `TCP_NODEMAX`
at `nbr = 16, k = 4`, after which a book simply has no prior.  No corpus
book comes near it (156 nodes), but a stream with a large residue book
loses the prior exactly where it is worth most.  A hash of the prefix, or
a trie of only the reachable prefixes (which is what a Huffman code's
prefix set is), removes the cliff.

**I12. The mantissa.**  A single counter plus the prior, on a 280-row
index.  The prior's `tcp_man` walks the alphabet per bit; a per-node table
of the first mantissa bit (the one most often asked) would make it a read.
Compression value unknown and small: mantissa bits are a few percent of
the digit bits.

---

## 9. Tried, measured, and not worth repeating

From BALROGG-MODEL.md's "what these are worth here" and the commit history:

- Indirect model (bit-history states + state map): 0.013% for 5-18% decode.
- Match model over the digit stream: worse than the model it was mixed
  into; 77% right, but accuracy alternates with length parity on stereo.
- Hashed fifth counter: -0.02% for +8-32% decode.
- Coding page/packet fields as differences: +4,653 bytes; deciding per
  field by a running score: worse than either fixed choice.
- Per-field delta for the header records: -154 bytes, kept.
- Bare threshold `ADD n: v` axes: replaced by threshold lists, which
  saturate and are tunable.
- 64 MB random-access tables (a state map sized to be safe): doubled
  decode time; only cache-resident side tables are affordable.
- Sharing one rate between two components: the optimizer settles it where
  the louder component wants it; every component now has its own.
