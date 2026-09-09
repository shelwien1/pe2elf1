# The tsvcomp model: design, bugs and room to improve

This describes the model as it stands: 11,355,711 bytes over the 17-file
corpus in 505 MB of declared tables.  Where it started, and what section 7
is a list of, was 11,372,019 bytes in 928 MB.  It is the
companion to two other documents: `BALROGG-MODEL.md` says what balrogg's
model does that this one does not, and what each of those was measured to be
worth here; `IDX/IDX-FORMAT.md` is the specification of the `.idx` files the
contexts are declared in.  Neither is repeated here beyond what is needed to
read the code.

Functions are named rather than line-numbered; everything is in
`tsvcomp.cpp` unless a file is given.  Sizes are what the shipping build
maps; "resident" means what the kernel actually faults in for a file.

Sections 7 and 8 are the working list.  Each entry says what was done and
what it measured, including the ones that measured worse and were reverted --
those are the more useful half, since the next person to have the same idea
should know it has been had.

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

| stage | 05 (105,854 B) | 07 (779,467 B) | what it is |
|---|---|---|---|
| headers | 1.72% | 0.27% | the three Vorbis header packets, record by record |
| pages | 0.31% | 0.37% | Ogg page fields as residuals against their expected values |
| packets | 0.03% | 0.00% | mode, window flags, floor-used flags |
| floor | 1.78% (2.00 b/post) | 5.25% (1.35 b/post) | floor1 posts, coded as folded residuals |
| class | 0.90% (1.18 b/class) | 3.15% (1.19 b/class) | residue partition classes |
| digits | 95.26% (2.63 b/digit) | 90.96% (1.15 b/digit) | residue vector digits, magnitude and sign |

The digit model is the program.  Everything else together is under 10%, and
on the high-bitrate file under 5%.

### The six families

Every value goes through one of six instances of `tc_fam`, a family, and each
family has its own context declarations (`IDX/tsvcomp-<fam>.idx`), its own
tables and its own rates.  What differs between them is only which variables
the contexts are built from and which parts of the cascade they use.

| family | codes | entry | sign | prior | depth | tables (MiB) |
|---|---|---|---|---|---|---|
| dig | residue digit magnitude | `code` | none here | residue book | 29 | 299.4 |
| sgn | residue digit sign | `bit(0)` only | is the sign | residue book, via `sq` | 1 | 23.7 |
| flr | floor posts | `codes` | one saturating counter | subclass book | 29 | 1.7 |
| cls | residue classes | `code` | not used | classbook | 29 | 0.2 |
| aux | page and packet fields | `code` / `codes` | 16 x 13 counters | none | 29 | 0.1 |
| hdr | header records | `codes` | 128 counters, by tag | none | 29 | 12.4 |

Total 505 MiB of address space, of which the corpus touches a small
fraction: the tables are mapped, not allocated, and only the rows a stream
reaches are ever faulted in.

The depth column is how many rows deep a family's tables are per context, and
it is where the cascade's position is carried (section 3).  fam_sgn's 1 is
why it is the cheapest rich model here and can afford context the digit model
cannot.

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

Four counters per decision, on four indices: `A` (dig: 184,320 rows) and
`B` (1,213,056) are two different groupings of the same variables, `C` and
`D` are small (1,080 and 169,344).  In `aux` and `hdr` counter B's rate is 0,
clamped to 1, so B there is a "what happened last time" flag updating at a
fixed 1/3.

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
33/33 for aux and 5/5 for hdr.  Per-file widths were measured and are not
worth a mechanism (section 8, I4).

### The mixer (`cm_mix<7>`)

A logistic mixer with seven inputs: APM1(A), B, C, D, raw A, the codebook
prior (or 0 when there is none, which is an even chance saying nothing), and
a bias of 256.  Weights are 16.16 fixed point stored as the distance from
the mean 1/7, one row of seven per mixer context.  Update is `w += x * err *
lr >> 16` with `err` the 12-bit error against the mixer's own output (not
the final `pf`).  `mb` is a boost for young rows: the rate is multiplied by
`1 + mb / (n + 1)` with `n` the row's u8 visit count, so a fresh row learns
fast and decays to `lr`.  The product is taken in 64 bits, because at the
rates the boost can reach it does not fit in 32 (section 7, B7).  dig has
`lr 11, mb 68`; the others `lr 18..31` with `mb 16` (sgn) or 0.  The
mantissa has its own three-input mixer (`mxm`: counter T, prior, bias) with
its own `lrm`/`mbm`.

### The blend

`bw` is how much of the final answer is the mixer against APM2, out of 16:
11 for dig, 5 for aux, 12 elsewhere.  It is a linear blend in probability
space with one constant per family, not learned and not per context, and two
ways of learning it were tried and are worse (section 8, I1).  What makes it
hard to beat is that an unlearned APM row returns very nearly its own input,
so the blend already degrades to "all mixer" exactly where the APM has
nothing to say.

### Current parameters

| | dig | sgn | flr | cls | aux | hdr |
|---|---|---|---|---|---|---|
| rA rB rC rD | 255 254 16 227 | 254 255 10 11 | 15 75 63 63 | 80 43 63 63 | 22 1 63 63 | 16 1 63 63 |
| rT / rG | 255 / - | - / - | 15 / 15 | 80 / (80) | 22 / 22 | 16 / 16 |
| mwA..D (x64) | 2 | 0 | 0 | 0 | 0 | 0 |
| rS1 rS2 | 7 7 | 7 7 | 6 6 | 5 5 | 6 6 | 1 1 |
| lr mb | 11 68 | 18 16 | 22 0 | 24 0 | 31 0 | 31 0 |
| lrm mbm | 32 68 | - | 22 0 | 24 0 | 31 0 | 31 0 |
| bw | 11 | 12 | 12 | 12 | 5 | 12 |
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

A family's tables are `TC_<fam>_ND` rows deep per context (and `TC_<fam>_MND`
for the mantissa plane), so where a decision sits in the cascade is context
without being a declared variable.  The most any family needs is 29 and 169;
fam_sgn needs one of each and declares that.  There is no sign node: every
family that codes a sign has a counter for it, and `codes` refuses rather
than reaching for a row that does not exist.

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
| n1, w1 | the slot one vector later and one vector earlier, last time, stepping by a whole number of channels so it stays on this one |
| p0 (p0s), pn, ps | the earlier pass's digit at this slot this packet, how many passes hit it, their signed sum |
| cls, bkq | the partition's class, and the class's codebook half-range (log) |
| zrun | zeros in a row in this stream |
| blk | short or long block |
| ax | `tc_qlog` of a weighted mean of `avg` (a 4.12 running mean of the slot's magnitude, decay `avD/256`) and the seven neighbours, weights `avA..avX1`, scale `avC/4096`, summed in 64 bits |
| pq | the prior's P(zero), 32 buckets |
| chn | the slot's channel: its place in the interleave where the residue has one, else the channel the vector is |
| x1 (x1s) | the coupled channel at this very place, one packet ago -- an interleaved residue only |
| crun, cm | how long the class has held, and whether it is what this partition had last packet |
| mg, sq | (sgn only) the magnitude just coded, the prior's sign probability |

What the seven dig indices actually use (buckets per factor, from
`IDX/tsvcomp-dig.idx`):

| index | rows | factors |
|---|---|---|
| A | 184,320 | pass 8, band 16, col 5, bkq 2, cls 6, blk 2, p0s 2, ax 2, pq 3 |
| B | 1,213,056 | pass 3, band 13, col 3, q1 4, bkq 3, cls 2, zrun 3, blk 2, p0 4, pn 2, pq 9 |
| C | 1,080 | pass 2, band 3, ax 5, q1s 2, p0s 2, pq 3, ps 3 |
| D | 169,344 | vpos 2, ax 7, q1s 9, p0s 7, pq 3, ps 4, chn 2, x1s 8 |
| APM1 | 27,648 | pass 2, band 2, t1 3, bkq 3, t1s 2, p0 3, p0s 2, ax 8, ps 2, chn 2, cm 2 |
| APM2 | 5,184 | pass 3, cls 9, q1s 4, ax 6, band 4, chn 2 |
| mixer | 26,880 | pass 2, bkq 2, cls 10, vpos 8, p0 2, pn 3, pq 7, chn 2 |
| mantissa | 280 | pass 4, cls 7, p0 2, chn 5 |

Note what is absent from A and B: `t1`, the same slot a packet ago, is only
in APM1, and the neighbourhood is only present as `ax`.  The optimizer moved
the temporal context out of the direct counters and into the APM and the
`ax` summary; the comment block in the .idx records why `cls` beats `band`
and why `zrun` looked worthless on the file it was first tuned on.

`chn` is at two buckets or none because it was priced: five buckets in every
index, of which a stereo stream reaches two, cost 456 MB and were buying 1459
bytes over four files, where `x1s` buys 3161 for 17 MB.  Sized for a
multichannel stream it would want six, and this is the file that decides
that.

Histories are per residue: `dg_hist`/`dg_hist2` (last two values of a slot,
keyed by block size, pass and coded-vector index), `dg_avg` (the running
mean), `dg_pp`/`dg_ps`/`dg_pn` (this packet's earlier passes, cleared per
residue call), and `dg_q1`/`dg_q2`/`dg_zr` per (rno, pass, channel).  Spans
over 2^17 digits or 2^23 history slots lose the temporal context but stay
codable.

### sgn

Same `tc_dv`, plus `mg` and `sq`.  A (4,082,400 rows): pass 3, cls 6, p0s 7,
mg 5, band 2, pq 5, sq 9, ps 9, x1s 8.  B (1,036,800): pass 3, vpos 2, p0s 8,
q1s 5, q2s 4, band 4, pq 5, sq 9, x1s 6.  The mixer (3,780) carries x1s at
its full 14.  What predicts a sign is the earlier pass's sign at the slot
(`p0s`), the signed sum of passes (`ps`), the prior (`sq`), the digit before
(`q1s`) -- which for interleaved stereo is the other channel at the same
position, this packet -- and `x1s`, which is that same channel a packet ago.
A coupled pair is a magnitude and an angle, and what the angle did here last
time is most of what there is to know about what it does now: `x1s` is worth
3161 bytes over four files, nearly all of it on 00000009.

Four million rows sounds ruinous and is 15.6 MB, because fam_sgn's tables are
one row deep.  That is the whole argument for per-family depth: the same
context in the digit model would be 29 times the size and does not pay.

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
is paid is the pages a stream touches: 505 MiB declared and a small part of
that resident on any corpus file -- a 38.9 GB tuned set once ran in 304 MB.
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
widening has to pay for itself.  It is not decoration.  The same greedy
search over the same axes reached 3,592,483 bytes on four files in 4345 MB
with the price off, and 3,592,689 in 337 MB with it on: 206 bytes for four
gigabytes.

Thresholds (`1!pattern`) map a variable's range onto buckets and saturate;
masks (`&pattern`) select bits; a pattern of all zeros is one bucket and
costs nothing, which is how a variable that did not pay stays visible to
the search.  The search runs on `opt.lst` (05, 00, 03, 07); flr's rates
tuned on the four small files alone cost 560 bytes on the corpus, which is
why a large file has to be in the objective.

---

## 7. Bugs, and what fixing them was worth

None of these broke decoding: encoder and decoder computed the same wrong
thing.  They were modelling defects, wasted memory, or arithmetic that was
only safe by margin.  All are fixed; each says what it measured, because
"correct" and "smaller" turned out to be different questions more than once.

**B1. `chn` was not the channel** for a residue coded per channel, nor for a
stream whose submap interleaves fewer channels than it has.  `tc_part` took
the slot's position modulo the number of channels the stream has, which is
the channel only for an interleave over all of them; for a type-0 or type-1
residue the channel is the vector's own and what the model saw was slot
parity, in seven of its eight indices.  Fixed: the place in the interleave
where there is one, the vector's channel where there is not.  The corpus
cannot show it -- its mono files have one channel and its stereo files
interleave both -- so this is correctness for content not to hand.

**B2. Per-channel histories were keyed by the vector's position** among those
coded, not by the channel.  A residue coded per channel skips the channels
whose floor was not used, so the first time a channel falls silent the next
one's past slides into its slot.  Fixed by keying on the channel.  Again
invisible here, for the same reason.

**B3. `n1` and `w1` stepped one book dimension** along, which stays in the
same channel only when the dimension is a multiple of the interleave.
00000009 codes one-dimensional books over two channels, where every step
crossed.  Fixed to step by the least common multiple -- and that cost 21
bytes, because what the bug had been returning was the coupled channel, and
`t1` already says what this channel held.  A neighbour is worth what it does
not repeat.  So the coupled channel became a variable of its own, `x1`, and
between the two the corpus fell 1115 bytes.  This one is the argument for
measuring a fix rather than trusting it.

**B4. fam_sgn's tables were 29 rows deep for the one row it uses.**  It calls
`bit(0)` and nothing else, so 28 rows in 29 were addressed by nothing, and
the row a context did use sat 116 bytes from the next -- one counter per
cache line, on the model asked once per non-zero digit.  Each family now
declares its own depth.  Byte-identical output, 928 MB down to 711.  The
memory it freed is what let the sign model afford `x1s` later, which is where
the real 3161 bytes came from: the fix that pays is rarely the one that pays
directly.

**B5. `chn` allocated five buckets where two are reachable.**  Priced against
what they buy, even the two lose in five of eight indices.  See section 5.
337 MB, and 588 bytes smaller than not doing it.

**B6. A sign node no family reached.**  `TC_SIGN` was the row a family without
a sign counter would have coded a sign at, and there is no such family.  Gone
with B4; `codes` refuses rather than reaching for it.  fam_cls declared a
sign counter of its own and codes a class through `code`, which never reaches
a sign, so that counter was never read and the rate and bound the .idx
offered for it were two knobs the optimizer could spend moves on and never
move anything.  Both gone, byte for byte.

**B7. The mixer's gradient step could overflow 32 bits.**  `err` is already
scaled by the learning rate and the young-row boost, so at the top of their
ranges any input past a counter at 94% takes the product past 2^31 -- the
fresh, confident, wrong row the boost exists for.  `-fwrapv` kept it
deterministic, so streams decoded and the step merely landed with the wrong
sign.  Taken in 64 bits now, which moves the corpus by 5 bytes: the case is
essentially unreachable at the shipped rates, and reachable exactly where the
search goes.

**B8. `ax` was summed in `int`** from inputs with no bound -- nine terms of
raw digits times a scale the optimizer may take to 1023.  In 64 bits now.

**B9. The granule field had one context axis twice.**  `tc_auxc` already
supplies the field's last value as the second axis, and `tc_page` passed the
same value as the first.  It gets the one before it now, as every other field
does.

**B10. The floor's backward contexts read rows that were not theirs.**  Every
floor context reaching back a packet is keyed by block size and channel, and
neither key says which floor wrote it: a channel whose floor was not used
still holds what it held when it last was, and two modes of one block size
may map a channel onto different floors, whose posts are at different
frequencies.  Each row now records the floor that wrote it and a foreign
history reads as absent.  Two bytes either way on this corpus, where every
packet uses every channel and keeps its floor.

**B11. The class model's "previous class" ran across channels and packets.**
Kept per channel now.  Unobservable here: the mono files have one channel and
the stereo ones interleave into a single classification stream.

**B12. `q1`, `q2` and the zero run ran across packets** -- at the first digit
of a packet they were the last digits of the one before, the top of the
spectrum standing in for the bottom.  Cleared per residue, worth 12 bytes.
`dg_hist` keeping "one packet ago" per block size is by construction and
stays: a short block's slot does not correspond to a long block's.

**B13. The stream header's spare byte** is still spare.  It was to be the
per-file profile of I4, and I4 does not pay.

One bug was introduced and caught here rather than found: the coupled-channel
lookup of B3 is the slot next door, and its guard only checked the slot
itself, so a history row of odd length read one element past its end.  Every
span in the corpus is even.  The whole corpus round-trips, and encode and
decode of a mono and a stereo file are clean under AddressSanitizer and
UndefinedBehaviorSanitizer.

---

## 8. Improvements, and what they measured

Ranked as they were before any of them was tried.  Most of the ones at the
top lost, which is worth more than the ranking was.

**I1. Learn the final blend.**  Tried twice, worse both times.  A two-input
logistic mixer over the mixer and the second APM, at its best rate, costs
1762 bytes over four files; what it learns is a geometric mean of the odds,
which is more confident than either input wherever they agree, and this blend
wants to be less.  Learning `bw` itself per node by gradient is worse still
-- 18,000 bytes -- because the least-squares gradient in probability space is
not the log-loss one, and the weight saturates at whichever end it drifts
towards.  The fixed blend survives because an unlearned APM row returns
almost exactly its own input, so a constant already behaves like "all mixer"
where the APM has nothing to say.

**I2. Weight the APM update by proximity.**  Tried twice, worse both times.
Scaling each point's step by its share of the interpolation costs 2325 bytes:
the step halves while the count ages at full speed, so the rate decays without
the learning.  Updating only the nearer point at full rate is 12 bytes better
on size and 2-9% *slower* to decode, because the branch is unpredictable and
the two points share a cache line anyway.

**I3. Density for sgn and the reachable-bucket fix.**  Done -- B4 and B5.
928 MB to 337 MB: B4 byte for byte, B5 588 bytes to the good.  The decode
speed it was ranked for did not materialise (I10); what it bought instead was
room for I8, which is where the bytes came from.

**I4. A per-file profile in the spare header byte.**  Measured first, and it
does not pay.  Choosing the first APM's width per file, which is the largest
of the per-file gains the earlier measurements claimed, is worth 57 bytes
over four files -- 0.0016%, against a mechanism that needs K trial encodes,
a complete reset of the model between them, and a format byte.  The model
sits at a broad optimum and per-file selection has almost nothing to select.

**I5. Two-rate counters, or a confidence input.**  Tried, worse.  An eighth
mixer input carrying counter A scaled by how much it has seen costs 645
bytes; replacing the raw-A input with it costs 363.

**I6. A fifth, hashed counter.**  Not retried.  It was -0.02% for +8-32%
decode, and nothing since has changed that arithmetic.

**I7. Tune with a large file in the objective and a price on memory.**  Done,
and the price is what made the difference.  A greedy search on the new axes
with memory free arrived at 4345 MB for 3867 bytes -- the corner this tree
built TC_MEMCOST to avoid -- and the same search with the model's address
space charged at 10000 bytes per gigabyte took the axis only where it earns.
Two runs of IDX/opt.pl over all 4190 tunable bits found 25 bytes between
them, which says the scalar parameters are done; the targeted searches on the
new axes found 3161 in a tenth of the time.  New information beats more
search.

**I8. Fix B1 and B2, then give the sign model the coupled channel.**  Done,
and it is the largest single win here: 3161 bytes over four files, 0.03% of
the corpus, for 17 MB.  The channel itself came back afterwards, measured one
index at a time: the sign model's rich counter wants it (976 bytes for 16 MB)
and the digit model's coarse counter does (482 for 134 MB), while the digit
model's fine counter is worse with it (+117) -- it already carries the band,
the column and the class, which on this corpus say much of what the channel
would.  For coupled stereo the channel is which half of the pair a value is,
a magnitude or an angle, and those differ most in their signs.

**I13. Look in the indices nobody had looked in.**  Not on the original list,
and the largest thing left.  Every factor line of tsvcomp-flr.idx and
tsvcomp-cls.idx carried the freeze marker, so IDX/opt.pl had never moved one
of them; the floor model's fourth counter and second APM and both of the
class model's APMs were empty, meaning one row -- a single correction curve
for every decision in a stream, which is the one thing an APM is not for.
The header model's sign-counter index was empty in the same way.  Filling
four of them with variables the .idx comments already named is 738 bytes of
floor, 429 of class and 5.3% of the header stage.  All three files are
unfrozen now.  The lesson is not about those indices: a frozen line is
invisible to the search and to the reader, and nothing recorded why these
were frozen.

**I9. Reset the histories that run past what they describe.**  Done -- B11
and B12.

**I10. Decode speed.**  Prefetching every row a value will read, issued at
select time, is inside the measurement noise.  Decode is unchanged overall,
against where this branch started: 0.95x on 00000005 and 00000003, 1.04x on
00000007, 1.01x on 00000009, 0.97x on 00000008.  The dependent chain through
the cascade is the cost, and neither smaller tables nor prefetch shortens
it.

**I11. Sparse prior tries.**  Done, by truncation rather than by hashing: a
book too deep for a dense trie now keeps the levels that fit instead of
losing the prior entirely.  Byte-identical here, since no book comes near the
cap; forced to a cap of 17 nodes it is 1588 bytes better than refusing.

**I12. The mantissa.**  Not attempted.  It is a few percent of the digit bits
and the work is a table for `tcp_man`, which is a decode-time question, not a
size one.

### What is left

- The rest of tsvcomp-flr.idx and tsvcomp-cls.idx, now that they are
  unfrozen.  Four indices were empty and four things were tried in them; the
  space is not exhausted.
- A residue's floor envelope as a context.  The floor is coded before the
  residue and says what magnitude to expect at each frequency, which is
  physically the right predictor for a digit; `cls` may already carry most of
  it, since the encoder picks the class from the magnitudes.  Untried.
- Anything that shortens the dependent chain per digit, which is what decode
  time is.

---

## 9. Tried, measured, and not worth repeating

From BALROGG-MODEL.md's measurements, this branch's, and the commit history:

- Indirect model (bit-history states + state map): 0.013% for 5-18% decode.
- Match model over the digit stream: worse than the model it was mixed
  into; 77% right, but accuracy alternates with length parity on stereo.
- Hashed fifth counter: -0.02% for +8-32% decode.
- A learned final blend, as a mixer (+1762 bytes) or as a learned `bw`
  (+18,000).
- A proximity-weighted APM update (+2325), or a nearest-point-only one
  (-12 bytes and slower).
- A confidence-scaled counter as an extra mixer input (+645) or in place of
  the raw one (+363).
- Per-file APM widths: 57 bytes over four files, against a format byte and
  K trial encodes.
- Prefetching the rows a value will read: inside the noise.
- Coding page/packet fields as differences: +4,653 bytes; deciding per
  field by a running score: worse than either fixed choice.
- Per-field delta for the header records: -154 bytes, kept.
- Bare threshold `ADD n: v` axes: replaced by threshold lists, which
  saturate and are tunable.
- 64 MB random-access tables (a state map sized to be safe): doubled
  decode time; only cache-resident side tables are affordable.
- Sharing one rate between two components: the optimizer settles it where
  the louder component wants it; every component now has its own.
- Optimizing size with memory free: 4345 MB for 3867 bytes.
- Replacing a bitmask with a threshold list on `p1r`, the header model's
  order-1 byte context: worse in every index tried -- 7968 header bytes
  against the mask's 7963 in the rich counter, 8015 against 8005 in the
  mantissa plane, 7672 against 7643 in the sign counter.  A threshold list
  quantises by magnitude, which is right for a digit or a run length and
  throws away exactly what a byte context is made of.  `p1r` is the only
  variable in the model that is a byte rather than a quantity, and the five
  masks are the five places it appears.  Carrying both forms of it as two
  factors is worse than the mask alone.
