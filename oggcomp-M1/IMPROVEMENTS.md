# Improving oggcomp: ratio and speed

A review of the sources at commit e744b32, with the measurements that
say where the bytes and the cycles go, and from them what is worth doing
next.  Numbers are from this tree's release build (`g++ -O2 -fwrapv`),
one core of a Xeon at a measured 3.2 GHz; `music-stereo-q5.ogg` is the
file quoted when one file is, `big.ogg` is seven corpus files chained
(1,106,690 bytes).  The coder-specific questions -- several coders,
interleaving -- are in `SUBSTREAMS.md` and only summarised here.

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
- **A neural mixer or an LSTM.**  Ten to a hundred times the instruction
  count for the kind of gain a second mixer gets at seven multiplies.

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
  nonzero.  On the managed file two digits in three are zero and most of
  those are in runs.  Expected: 15 to 30% fewer decisions on low-bitrate
  and stereo files, none on `-q 10`; the ratio can go either way by a
  fraction of a percent, since the run's contexts are coarser than the
  digits' but its symbol is one.
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
4. The zero-run value (5.4) last, once the digit model has settled,
   since it changes what a digit's contexts see.

Before any of the ratio work the corpus wants two more files: a long
uncoupled stereo one and a long 5.1 one, so that 4.1 is tuned on
something it will meet again.
