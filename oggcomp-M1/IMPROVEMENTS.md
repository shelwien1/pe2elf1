# Improving oggcomp: ratio and speed

A review of the sources at commit e744b32, with the measurements that
say where the bytes and the cycles go, and from them what is worth doing
next.  Numbers are from this tree's release build (`g++ -O2 -fwrapv`),
one core of a Xeon at a measured 3.2 GHz; `music-stereo-q5.ogg` is the
file quoted when one file is, `big.ogg` is seven corpus files chained
(1,106,690 bytes).  The coder-specific questions -- several coders,
interleaving -- are in `SUBSTREAMS.md` and only summarised here.
Section 7 is the update at commit fda24ce: the codebook table, which
took the headers out of the small files, and three things measured
since -- OptiVorbis, a set of 1,257 short clips, and run-length coding
of the zeros -- with the plan in section 6 revised to match.

## 1. Where it stands

`./t.sh -1 -n`, 31 files, 711,883 bytes in, 546,567 out (76.8%):

| file | in | out | ratio |
|---|---|---|---|
| music-stereo-q5.ogg | 167,685 | 144,389 | 86.1% |
| music-managed-b96.ogg | 101,549 | 84,604 | 83.3% |
| chirp-stereo-q10.ogg | 130,545 | 116,294 | 89.1% |
| noise-stereo-q3.ogg | 32,036 | 25,745 | 80.4% |
| sine-stereo-q5.ogg | 14,541 | 5,684 | 39.1% |
| multi6-48k-q4.ogg | 13,855 | 6,803 | 49.1% |
| tiny-8k-q0.ogg | 2,763 | 1,346 | 48.7% |

Update at fda24ce: 31 files, 711,883 in, 507,823 out (71.3%), the
difference being the codebook table of section 7.1 -- `tiny-8k-q0.ogg`
is 200 bytes, `multi6-48k-q4.ogg` 3,324, the music file 143,012, and
the three headers are 0.13% of it rather than 1.1%.

On real music the ratio is 83 to 89%, and the whole of that is the
residue: `-v` on the music file puts 93.9% of the output in residue
digits (695,824 values at 1.56 bits each), 2.4% in floor posts (1.25
bits each), 2.3% in residue classes, 1.1% in the three headers, 0.3% in
page headers.  On the small files the headers are half the output,
which is why `oggdet -S` and the header cache exist; that is done and is
not in this document.

Time: 0.29 s to encode the music file and 0.30 s to decode it, 1.49 s
and 1.45 s for `big.ogg` -- 0.5 to 0.75 MB/s, both directions alike.

## 2. What the model is

One decision at a time, everything binary.  A value -- a residue digit,
a floor post, a class, a header field -- is walked as a small tree:
three unary head nodes for 0, 1, 2, then a unary length, then the
mantissa bits (`tc_fam::code`).  Every node of that walk is one binary
decision through the same pipeline (`tc_fam::bit`):

1. four counters from four tables (`A` rich, `B` coarse, `C` direct, `D`),
   each row selected by a context index that a family's `.idx` declares as
   a mixed-radix product of quantised variables;
2. an APM stage on the rich counter's probability, keyed by index `s`;
3. a 7-input mixer -- the APM'd `A`, `B`, `C`, `D`, raw `A`, the
   codebook prior, a bias -- with the weight set keyed by index `m`;
4. a second APM on the mix, keyed by `f`, blended back with the mix by a
   fixed weight;
5. the coder;
6. updates: the mixer's seven weights, both APMs, all four counters.

The mantissa bits take a lighter path (`bitm`): one counter table `T`
and a 3-input mixer with the codebook prior.  Signs are one decision in
their own family with their own contexts (`sgn`).

The **codebook prior** (`tc_ptab.inc`) is what makes this a Vorbis
model rather than a generic one: from a codebook's entry lengths it
builds a trie over codeword prefixes and at every node precomputes the
probability of each head decision, each length and, when small enough
to table, each sign; the mantissa probabilities are found by scanning
the node's symbols.  The prior enters the pipeline twice: as a mixer
input (`prp`) and as a context variable (`pq`, the prior of a zero, in
32 steps).

The **context variables** for a digit (`oc_residue.inc`, `digit()`):
the two digits before it in the partition stream (`q1`, `q2`, signed as
`q1s`, `q2s`), the same slot one and two packets ago (`t1`, `t2`), its
neighbours one packet ago a stride away (`n1`, `w1`), the coupled
channel's slot one packet ago in an interleaved residue (`x1`), what the
earlier passes put at the slot (`p0`, `ps`, `pn`), a weighted average of
all of those (`ax`), the running zero count (`zrun`), the class and how
long it has held (`cls`, `crun`, `cm`), the partition and position
(`band`, `col`, `vpos`), the block size, the residue and pass, the
codebook's range (`bkq`), the channel (`chn`), and the prior (`pq`).
Thirty inputs, quantised through `tc_qlog` or `tc_sq`, and ten indices
built from them by the generated `tc_make_dig`.

Everything in that list is a knob: which variables reach which index and
at what thresholds is the bit pattern beside each line of the `.idx`,
and `IDX/opt.pl` hill-climbs those patterns in the tuning binary over a
corpus.  The tables that result were 5148 MB before the hashed rows and
are 366 MB now, for 0.01% of output.

## 3. What the measurements say

**The parameter search is near its floor.**  The profile in `101ff53`
moved 236 patterns for 0.05% of the corpus; the hashed rows cost 0.01%.
The remaining ratio is not in the thresholds.  It is in information the
model does not have -- a variable that is not in the list -- or in
machinery it does not have -- a second rate, a second mixer.  Any of
those is tested the same way: add the variable at pattern zero, let
`opt.pl` decide where it goes, read the corpus total.

**The output bits per decision are 0.62** (music), and 19% of the
decisions are made at a probability under 1/256 (31% on the
bitrate-managed file, 8.5% on `-q 10`), because 47% of the digits are
zero (66% managed, 14% at `-q 10`).  Those decisions are already nearly
free in bits; they are not free in time.

**The time is the model, not the coder and not memory.**  A decision
costs 1,260 instructions (music) to 1,060 (`big.ogg`) and 380 to 510
cycles, at 2.5 instructions per cycle.  cachegrind counts two or three
L1 misses per decision and next to no last-level misses at either size
(112 K for 1.85 M decisions; 322 K for 12.6 M).  The coder is 4% of the
instructions; with the encoder's `rc_Process` stubbed out, the encoder
runs 10% faster, which is all a coder change can give (`SUBSTREAMS.md`).
By source file, encode (decode is within a point of it):

| where | share | what |
|---|---|---|
| `cm.inc` | 27% (34% at `-q 10`) | counters, two APMs, mixer, mantissa mixer, and all their updates |
| `MOD/tsvcomp-dig_p.inc` + `sgn_p.inc` | 24% (18% at `-q 10`) | `tc_make_dig` and `tc_make_sgn`: ten context indices from thirty inputs, about 610 instructions per digit |
| `tc_fam.inc` | 10% | the tree walk, row selection, prefetches |
| `oc_residue.inc` | 10% | `digit()`: gathering the thirty inputs |
| `tc_base.inc` | 5 to 7% | `tc_qlog`, `tc_sq` |
| `tc_ptab.inc` | 3 to 4% | the codebook prior, mostly the mantissa scan |
| `rc.inc` | 3 to 4% | the coder |
| `vb_packet.inc` | 2 to 3% | the Vorbis parse |

Compilers: clang is 4% faster than gcc on `big.ogg` at `-O2`; `-O3`
and `-march=native` gain nothing on either, and `./mk.sh pgo` was
measured as noise before.

## 4. Ratio

Ordered by expected return per effort.  Each is a model change, so each
means a new `OC_VER`, `./mk.sh check`, `./t.sh`, and a tuning pass; the
gain is what the corpus total says after `opt.pl` has had the new
variable, not before.

### 4.1 The coupled channel for residues that are not interleaved

`x1` -- the other channel at this slot -- exists only for a type-2
residue, where the channels are interleaved in one vector and the value
is one packet old.  A type-0 or type-1 residue with several vectors
(uncoupled stereo, the six vectors of a 5.1 stream) codes channel j
after channel j-1 has been coded *in this packet*, and its value at the
same slot is in `hist.dg_hist` already, this packet's, not last's.
Nothing reads it.  Add `x1` for `R.vch > 1` from channel `R.cidx[jv-1]`
at the same slot, this packet, signed and magnitude, and let the
optimizer find the buckets.

Expected: nothing on coupled stereo, which is most files; several
percent of the digits on uncoupled and multichannel files, whose spectra
are correlated across channels the way the interleaved case already
exploits.  `uncoupled-stereo-q4.ogg` and `multi6-48k-q4.ogg` are the
corpus's witnesses and the corpus needs a longer one of each before this
is tuned, or the patterns will fit those two.

### 4.2 The previous packet, normalised by the floor

`t1` is the residue at this slot one packet ago.  A residue is the
spectrum divided by the floor, so where the spectrum is stationary and
the envelope moves -- a sustained note under a changing level, which is
music -- `t1` is off by the floor's change at that frequency, and the
model sees a shifted value where a scaled one would predict.  The floor
curve of both packets is known to both sides (`fl_fy`, rendered between
posts by `vd_render`): compute the curve's value at the partition's
frequency for this packet and the last, and give the digit model the
difference as a variable (`fd`, quantised in dB steps), and `t1` scaled
by it as another (`t1n`).

Expected: 1 to 3% of the digits on tonal material, nothing on noise; the
sine and sweep files will show it first.  Cost: one interpolation per
partition per channel, cheap next to a digit; two variables, so two
more inputs to `tc_make_dig`.

### 4.3 Two rates per counter

Each context has one counter with one adaptation limit (`rA`..`rD`).  A
fast and a slow counter per context -- the usual pair, one that follows
the last few dozen decisions and one that averages hundreds -- gives the
mixer a way to weight recency by context.  Table `A` is the one to
double: it is the rich one, and the mixer input that matters most.

Expected: 0.3 to 1% of the output, which is what the pair is worth on
most CM models; speed down some 5% for the extra load, update and mixer
input.  Cheap to try: a second `cm_cnt` table wired beside `A`, one more
mixer input, one more rate parameter.

### 4.4 A second mixer

One weight set, keyed by index `m`.  A second mixer over the same inputs
with a different key -- the codebook prior's bucket, or `cls` and `q1`
-- and the two averaged in the stretched domain (or the second fed the
first's output) is the standard next step and usually finds another
0.3 to 0.8%.  Costs a second weight table and seven more multiplies.

### 4.5 Signs

A sign is 28% of the digit bits and nearly random; its contexts are the
signs a pass earlier, of the coupled channel and of the digit before,
and the magnitude.  Two things it does not see: the sign at this slot
*two* packets ago together with one packet ago -- an MDCT coefficient
of a steady tone alternates sign with a period set by the frequency, so
the pair predicts where either alone does not -- and the sign of the
neighbouring slot in this packet (`q1s` is the previous *digit*, which
in a multi-dimension codeword is the neighbouring slot only sometimes).
Both are one variable each.  Expected: a fraction of a percent overall,
more on tonal files.

### 4.6 The prior in the APM

`pq` is the prior of a zero, in 32 steps, used as a context variable.
The prior of the *current node* -- the head node, the length node, the
mantissa bit -- is what `prp` is, and it goes into the mixer as an
input but not into either APM as a key.  An APM keyed by the current
node's prior bucket, on the mixed probability, lets the model correct
the codebook's own opinion where it is systematically off.  Small and
cheap: `tcp_bucket(prp)` is one lookup.

### 4.7 What is not worth it

- **Wider tables.**  The hashed-row measurement (`HASHTABLES.md` §3)
  shows the tables are not short of rows: 2^14 rows per table cost
  0.035%, 2^18 cost 0.004%.  The model is limited by what it is told,
  not by where it keeps it.
- **More thresholds on the present variables.**  The last profile's
  0.05% says so.
- **Floor and class.**  Together under 5% of the output on music, and
  the floor is already at 1.25 bits a post; the ideas in `tsvcomp-flr.idx`
  (`ep`, `prd` in counter `D`) were the ones with room.  Worth a tuning
  pass when the digit changes above force one anyway, not a project.
  *Corrected in `IDEAS.md`: on every small or tonal file the floor is 12
  to 36% of the output, and 7.8% of the clip set at 3.36 bits a post, so
  it is not worth little there; the floor ideas are in `IDEAS.md` 3.1
  and 3.2.*
- **A neural mixer or an LSTM.**  Ten to a hundred times the instruction
  count for the kind of gain a second mixer gets at seven multiplies.
- **Run-length coding of the zeros.**  Measured in 7.4: the zero
  decision is 37 to 59% of the digit bytes, so it is the right place to
  look, but the runs are near-geometric and what structure they have --
  the place in the vector, the codebook's prior -- the per-digit decision
  sees at every digit and a run symbol sees once.  Every run-length
  estimate is 10 to 20% worse than a per-digit one with the same
  information, and the model beats both.

Taken together, 4.1 to 4.6 are a plausible 2 to 4% on music and more on
multichannel and tonal files, against a ratio that is 86% today; a
project of weeks, each step measured on its own.

## 5. Speed

Ordered by expected return.  None of these changes the stream except
where it says so.

### 5.1 The coder's state in registers

`rc` is a static member; between decisions `range`, `low` and `rpre`
are stored and reloaded, and the chain measures 38 cycles in place
against 17 standalone.  Take a local copy of the coder at the start of
`tc_fam::code` and write it back at the end -- one value, three to
eight decisions -- so the chain runs in registers.  Expected: up to 5%
of the encoder and a little less of the decoder; nothing in the stream.

### 5.2 Hoist the partition-constant half of the context hashes

`tc_make_dig` is 610 instructions per digit and half of its inputs do
not change within a partition: `rno`, `pass`, `band`, `cls`, `bkq`,
`blk`, `chn`, `crun`, `cm`, and the codebook-dependent quantities.  The
generated code multiplies all of them out on every digit.  Split each
index into a partition prefix computed once in `part_begin` and a
per-digit suffix -- `idx2inc.pl` knows which variables are which if the
`.idx` says so, a one-word modifier per line -- and the per-digit cost
of the indices drops by roughly the share of constant factors, which by
the patterns is 40 to 60%.  Expected: 8 to 12% of the whole.  Nothing
in the stream: the indices are the same numbers.

### 5.3 Narrower mixer arithmetic

`cm_mix::mix` and `upd` do seven 64-bit multiplies each; the inputs
are 12-bit stretches and the weights 16.16.  Sixteen-bit inputs against
32-bit weights fit a 32-bit product with room, and seven of them pad to
eight lanes of a 32-bit SIMD multiply-add -- `pmulld`/`vpmulld` on
x86-64 -- for both the dot product and the update.  The APMs stay
scalar.  Expected: a third of `cm.inc`'s share, 8 to 10% of the whole,
if the rounding is made identical between the scalar and vector paths
so the stream does not move; it will move otherwise, and then it is a
version bump and a re-tune.

### 5.4 A cheaper path for the certain decisions

A fifth to a third of the decisions are made at under 1/256 and cost
the same 1,260 instructions as the others.  Two ways to spend less on
them, both model changes:

- **A zero-run value.**  When `zrun` passes a threshold, code the length
  of the run of zeros as one value in a small family of its own instead
  of one head decision per zero; the digit model resumes at the first
  nonzero.  *Measured since, in 7.4, and out:* the runs have a mean of
  three, so a run symbol of two or three binary nodes replaces three or
  four decisions and saves little time, and the ratio does not go either
  way by a fraction of a percent -- the run form is 10 to 20% worse on
  the zero pattern, which is 37 to 59% of the digit bytes.
- **A short pipeline when the rich counter is sure.**  When counter `A`
  is at its limit and beyond 1/1024, skip the second APM and the mixer
  update.  Both sides see the same counter, so this is deterministic,
  but it is a change to the probability and so to the stream, and its
  ratio cost is what the corpus says.

### 5.5 The mantissa scan

`tc_pcur::man` finds the mantissa-bit probability by scanning the
node's symbols, up to 64 of them, on every mantissa bit; `sign` does
the same where the sign table was too big to build.  Precompute a
per-node mantissa table the way `pl` is precomputed for lengths, within
the same 64 MB budget, and the 3 to 4% in `tc_ptab.inc` mostly goes.  It
matters most at `-q 10`, where digits are 3 bits each and most have a
mantissa.  Nothing in the stream.

### 5.6 The rest

- **clang** is 4% faster at `-O2` and writes the same bytes.  `mk.sh`
  takes `CXX=clang++` already.
- **`-O3`, `-march=native`, PGO**: measured, noise.
- **Memory** is not the limit at a megabyte of input; it may be at a
  hundred, where the touched rows of the 300 MB digit family outgrow
  the cache.  That is a measurement to make on a long file before
  anything is done about it; huge pages (`-H`) are the first thing to
  try if it turns out to be.
- **Parallelism.**  Within one stream the decoder is one dependency
  chain and the encoder gains nothing from more coders
  (`SUBSTREAMS.md`).  Across streams, `oggdet -c` resets the model per
  stream and could run N streams on N cores, at 366 MB of tables each.
- **The parse** is 2 to 3%; not worth touching.

5.1, 5.2 and 5.5 change nothing in the stream and add up to 20 to 25%;
5.3 is the same again if its rounding is pinned; 5.4 is where a speed
project turns into a model project.

## 6. A plan

1. Speed that changes nothing: coder state in registers (5.1), the
   mantissa table (5.5), then the hoisted indices (5.2).  Each is a
   `./mk.sh check` and a `./t.sh` with identical output; together
   perhaps 25%.
2. The SIMD mixer (5.3), with the scalar path kept and both required
   to agree on the corpus before either ships.
3. Then the ratio, one variable at a time, each with its own tuning
   pass and its own corpus witness: the coupled channel (4.1), the
   floor-normalised history (4.2), the counter pair (4.3), the second
   mixer (4.4), the sign contexts (4.5), the prior in the APM (4.6).
   Each is a version; a version bump is cheap and a wrong stream is not.
4. Not the zero-run value: 7.4 measured it out.  In its place, the
   three things the clip set showed (7.3), each small and each its own
   version: the packet lengths derived from the decoded packet instead
   of coded in the page header, the `-S` archive framed once instead of
   per stream, and the metainfo records batched.
5. A trained prior for the small files (7.3): the tables started from a
   training run instead of from zero, so that a 5 kB stream coded alone
   gets what solid mode gives it.  A model change and a version, and the
   one item here that helps `oggdet -c` more than `-S`.

Before any of the ratio work the corpus wants two more files: a long
uncoupled stereo one and a long 5.1 one, so that 4.1 is tuned on
something it will meet again.

`IDEAS.md` supersedes the ordering above where the two differ: it merges
five brainstorms held after this document was written, ranks twenty
ideas, and puts first the one instrument -- a wider per-digit log and a
per-node byte accounting -- that decides most of them before anything
is built.

## 8. Tested, at d579c50

Twenty ideas were ranked in `IDEAS.md`; eleven were built and measured
against the corpus, the 1,257-clip set and the clock.  Four paid and are
in the tree (stream version 6); seven did not and are recorded here so
that nobody builds them twice.

| | corpus, 35 files | music-stereo-q5 | music-managed-b96 | chirp-stereo-q10 | tiny-8k-q0 | the clip set |
|---|---|---|---|---|---|---|
| before | 509,835 | 143,012 | 83,291 | 114,682 | 200 | 5,443,469 |
| the four changes | 509,072 | 142,730 | 83,150 | 114,572 | 197 | 5,430,527 |
| and retuned | 508,834 | 142,688 | 83,056 | 114,553 | 197 | 5,429,818 |
| | -0.196% | -0.227% | -0.282% | -0.112% | -1.5% | -0.251% |

Decode is unchanged on the music file and 1.05x slower on the clip set;
encode is 1.10x.  On the music file the digits stage went from 1.558 bits
a digit to 1.555, the class stage from 1.263 to 1.255 and the floor from
1.251 to 1.249, so the gain is spread over all three rather than sitting
in one of them.

### 8.1 What paid

**Decisions the codebook has already made are not coded** (`IDEAS.md`
rank 15).  `tcp_p` clamped a zero weight to one part in four thousand,
so a symbol no codeword of the book can reach still cost a bit and, worse,
still updated the counters that genuinely uncertain decisions share.  Two
sentinel values say "certain" instead, and `code()` leaves the head,
length, mantissa and sign decisions the prior has settled out of the
stream entirely.  The largest of the four: -0.05% on music and -0.16% on
the clip set.  The encoder asserts, on every such decision, that the value
agrees with the prior; 500 mutated corpus files and `./t.sh` never fired it.

**Cold counters start from something** (rank 3).  `cm_cnt::P()` returned
one half on a first visit and `upd()` moved away from one half.  A counter
now answers with a seed the caller supplies -- the codebook prior for the
node being coded, or the coarse direct table's answer -- and moves away
from that.  -0.031% of the corpus, and it is what takes `tiny-8k-q0` from
200 bytes to 198.

**The classifier's quota is a digit context** (rank 2).  libvorbis picks a
partition's class as the first whose `classmetric1` bounds the partition's
largest magnitude and whose `classmetric2` bounds the sum of them
(`res0.c`, `_01class` and `_2class`), so the class -- coded before the
digits, and the model's strongest single context -- is a statement about
numbers the model can accumulate as it goes.  Three of them, the running
maximum, the running sum and the slots left, at patterns `IDX/opt.pl`
placed: -0.042% of the corpus, -0.107% of music, -0.113% of the clip set.

**The mantissa plane gets a map** (rank 9).  `bitm()` was one counter and
a three-input mixer against `bit()`'s four counters, two maps and seven
inputs, and it carries most of the bits on the files whose digits are
largest.  An adaptive probability map keyed by the node alone -- 169 of
them, warm within a few hundred decisions -- is -0.015% of the corpus and
-0.04% of `chirp-stereo-q10`.

### 8.2 What did not, and the pattern in it

Seven were built and measured worse than the tree they were added to.  The
figure is the corpus total against the same tree without the change.

| idea | rank | result |
|---|---|---|
| the floor curve as a digit context: level, and departure from the partition's mean | 1 | +125, and +20 on top of the winners |
| the floor's motion since the last packet, as a correction to `t1` | 13 | +455 wide, +66 narrow |
| the noise-normalisation grid: position in a 16-bin block and the unit magnitudes spent in it | 2 | +888 |
| where the neighbouring partitions' classes stand against this one | 4 | +335 |
| a mixer lane saying whether the prior is alive | 3 | part of +883 |
| the second map's blend learned instead of tuned | 3 | part of +883 |
| the sign path given a coarse partner, a mixer and a map | -- | +4,515 |

The pattern is consistent and is the useful finding.  **Every attempt to
add context *width* lost; every fix that added *capacity* or removed
*waste* won.**  A new axis multiplies the index volume of the family it
joins, and the digit family's tables are already visited about ninety
times per row over a whole music file (`HASHTABLES.md` §3 says they are not
short of rows, which is a different thing from being full of counts).  The
estimates in `IDEAS.md` were ten to twenty times optimistic for the
context ideas and roughly right in sign for the machinery ones.

Two caveats on the negatives.  The floor ideas were tested with patterns
set by hand and confirmed narrower by a short `opt.pl` run on three of
them; a full tuning pass over a corpus that contained the clip set might
place them better, and the clip set alone did improve under three of the
four floor variants.  And the sign result is not a verdict on sign
modelling: it was the header family, where the sign context is the tag id,
that collapsed, while music, managed and chirp did not move at all.

### 8.3 The tuner

`IDX/opt.pl` was run three times.  A focused pass over the three new quota
patterns moved an eight-file objective by 0.01% and narrowed two of the
three patterns set by hand.  A broad pass over the rates was abandoned
when it turned out to be tuning the model before the prior skips went in.
The third ran to completion over the digit family's counter, mixer and map
rates and the floor and class families' -- 125 keys, a nine-file objective
including a slice of the clip set -- and moved it 0.039%, which is a third
of everything the four model changes won and more than any single one of
them.  It is in the tree: the counter rates, their targets, the mixer
rates and the second map's blend all moved, and the mantissa counter's
target moved off zero, which is the seeded counters and the new map asking
for different arithmetic than the model was tuned for.  Nothing structural
changed, and `./mk.sh check` agrees over 35 files afterwards.

The lesson for the next person is that a model change is not finished
until the tuner has seen it.  Both runs wanted the key filter
this tree's `opt.pl` does not have, which is why the first attempt matched
nothing: the keys carry a trailing underscore (`TC_dig_rA_`).

`IDEAS.md` rank 17 -- re-aim the optimizer -- is unbuilt and is still the
item that gates every number above.  Its own argument was borne out here:
the patterns tuned on eight files without the clip set made the clip set
worse by 0.04% while making the corpus better.

## 7. Since e744b32

### 7.1 The codebook table

Done, at fda24ce.  libvorbis carries its codebooks as static tables and
writes one of a few hundred fixed sets into every stream, and tsvcomp,
this program's ancestor, had generated the table of them --
`vbooks_gen.inc`, 1,346 books in 350 rows from 35 encoders: every
released libvorbis, the aoTuV betas, ffmpeg's own encoder.  `vb_dict.inc`
packs a table book the way `vorbis_staticbook_pack` does and compares it
with the setup packet bit for bit; the encoder codes which distinct row
the packet's first books are the bits of, then one flag per book, and a
miss is the fields as before.  Nothing is parsed to decide, and a book
from the table is parsed from the packet with nothing listening once the
decoder has written it there, so it is the `vb_book` the stream's bits
would have been.

Every book of every corpus file is in the table.  The three headers of
a corpus file went from 1.1 to 1.6 kB to 100 to 250 bytes:

| file | before | after |
|---|---|---|
| tiny-8k-q0.ogg | 1,346 | 200 |
| mono-22k-q4.ogg | 2,669 | 1,192 |
| multi6-48k-q4.ogg | 6,803 | 3,324 |
| music-stereo-q5.ogg | 144,389 | 143,012 |
| corpus, 31 files | 546,567 (76.8%) | 507,823 (71.3%) |

Two things came with it.  The header tags now take their ids from a
fixed list (`oc_tagorder` in `oc_model.inc`) rather than from first use:
the tag factors are threshold lists over the id, tuned, and the
first-use order was the stream's -- a floor whose subclass books came
before its master book moved every tag after it -- and with most books
no longer coded it would have moved all of them.  And three corpus
files: a setup one book short of a row, one with only its floor books
in a row, and one rewritten by OptiVorbis with no row in it.

### 7.2 OptiVorbis

`OPTIVORBIS.md`.  A sample-lossless optimizer that recomputes the
Huffman codes from the actual symbol counts and drops the unused
entries.  Its rewrite is worth 0.08% to the digits stage here -- the
model codes the symbol, not the codeword -- and its one win over oggcomp,
the smallest files, was the setup header, which 7.1 took.  What
transfers from it: a fuzz harness over `oc_api.h`, and tolerance of an
audio packet that ends inside a field, which the specification allows
and libvorbis's bitrate manager writes under a hard maximum; oggcomp
codes such a page as bytes today.

### 7.3 A set of 1,257 short clips

Mono 44.1 kHz, 4.9 to 10 kB each, all from one libvorbis with one
identification, comment and setup packet between them, serial 1 in
every file, three or four pages, 11 to 118 audio packets.  Headers are
41% of the bytes and the audio is unique: 48,624 distinct of 49,314
packets, and 47 packets share their first eight bytes with a packet in
another file.  A container of exactly the kind `oggdet` is for, and
what it showed is where the per-stream bytes go when the stream is
tiny.

| on the concatenation | bytes | of input |
|---|---|---|
| xz -9e | 6,045,992 | 60.5% |
| oggdet -c | 5,901,788 | 59.0% |
| oggdet -S | 5,470,874 | 54.7% |
| oggdet -S -j | 5,443,523 | 54.4% |
| oggcomp, as one chained file | 5,443,469 | 54.4% |

xz gets its 60% by removing the repeated headers; the audio alone is
5.92 MB.  Solid, the stage table is headers 163 bytes for all 1,257
setups, pages 41,870, floor 424 kB at 3.36 bits a post, class 166 kB,
digits 4.81 MB at 1.43 bits a digit.  Three findings:

- **The model starts cold.**  `-c` is 431 kB behind `-S`: 197 kB of
  setup coded per stream, and 240 kB -- 4% of the output -- for a model
  that begins every 5 kB stream at zero and has not settled after 38
  packets.  On its own audio `-c` gets 96%, `-S` 92%.  This is what the
  trained prior in the plan is for.
- **`-S` loses nothing between streams; its framing costs 27 kB.**
  Each stream segment carries the 7-byte oggcomp header, a 4-byte CRC
  and the coder's last byte, 12 bytes, the same 12 an empty input codes
  to; the metainfo is 4,384 bytes of records in 1,258 chunks, one per
  stream record by design, coded to 8,446 because each chunk pays a CRC
  and a flush for 3.5 bytes of content; the segment varints are 4,523.
  `-j` removes all of it and gives up per-stream addressing.  In solid
  mode one header and one CRC per archive would do, and the records can
  be batched if the chunks are coded by a small model of their own
  rather than the shared one, which must be decoded in file order.
- **The pages stage is the packet lengths.**  33 bytes a file, and the
  lacing values are 5.83 bits of entropy each over 195 distinct lengths,
  36 kB of the 41.9 kB.  The decoder knows a packet's length once it has
  decoded its fields and the tail flag, so coding the payload before the
  lacing and deriving the lacing from it leaves one value per page for
  the boundary.  0.66% here, 0.3% on music, and the granule positions,
  which follow from the block sizes but for the last page, are the rest.

### 7.4 Run-length coding of the zeros

The question was whether to code a run of zero digits as one length,
with zero excluded from the digit that ends it.  Instrumented: the cost
of the first binary node of every digit, the one that says zero or not.

| file | digits | zero | bits per digit on that node | memoryless | of the digits stage |
|---|---|---|---|---|---|
| music-stereo-q5 | 695,824 | 46.9% | 0.574 | 0.997 | 37% |
| music-managed-b96 | 561,760 | 66.5% | 0.533 | 0.920 | 49% |
| noise-stereo-q3 | 143,648 | 56.6% | 0.516 | 0.987 | 40% |
| the clip set | 26,907,520 | 60.7% | 0.840 | 0.962 | 59% |

So the zero pattern is the cost centre of the residue, and on the clips
the model takes 13% off a coin flip for it.  Against that, crude
adaptive estimates over the logged digit stream, per-digit contexts and
run lengths given the same information (bits per digit):

| context | music q5 | managed | clips |
|---|---|---|---|
| last 8 digits zero or not | 0.733 | 0.718 | 0.889 |
| codebook prior, last 8, class | 0.618 | 0.568 | 0.853 |
| run length, order 0 | 0.893 | 0.955 | 0.950 |
| run length given previous run, class, pass, offset | 0.746 | 0.683 | 0.884 |
| run length given previous run, class, pass, prior at start | 0.699 | 0.673 | 0.912 |
| the model | 0.574 | 0.533 | 0.840 |

Run lengths lose 10 to 20% to per-digit contexts on every file, and the
model beats both.  The runs are near-geometric -- on the clips 237k of
one, 116k of two, 65k of three, halving each step -- and what structure
they have is at the place in the vector (odd lengths dominate on the
managed file, a zero vector of dimension two ending a run at an even
boundary) and in the codebook's prior for the next digit, which the
per-digit decision sees at every digit and a run symbol sees once.
Excluding zero from the digit after a run saves the decision the run
symbol has already paid for.

The cheap form tried for real: `zrun`, the zero-run context the digit
model already has at one threshold, widened, and `vpos`, the place in
the vector, enabled, in a release build each:

| build | corpus, 34 files | clips | music q5 | managed |
|---|---|---|---|---|
| current | 501,355 | 5,443,469 | 143,012 | 83,291 |
| zrun widened | 501,369 | 5,437,912 | 143,027 | 83,299 |
| vpos enabled | 501,406 | 5,445,580 | 143,060 | 83,215 |
| both | 501,426 | 5,440,170 | 143,079 | 83,226 |

A tenth of a percent on the clips and noise elsewhere: the codebook
prior is already doing what those axes would.  Building the run form
would mean the encoder reading a partition's codewords ahead, since
`rs_sym` codes each codeword's digits before reading the next, for a
result the estimates put below the model.  The section 5.4 speed case
for it fails on the same numbers: a mean run of three is three or four
decisions replaced by a symbol of two or three nodes.
