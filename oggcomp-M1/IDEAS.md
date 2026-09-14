# Ideas for oggcomp's ratio: five brainstorms, merged

Five agents were asked, at high temperature, to invent ways of
improving the compression ratio, each from a different angle: the signal
(what a decoder knows of the spectrum), the modelling machinery, the
encoder's determinism, structure and cross-stream information, and
anything from other fields.  Their reports are in `ideas/` as written,
with their own measurements; this document is the merge.  Every
mechanism below that could be checked against the code or against
libvorbis's source was checked, and the annotations say which.  The
estimates are the agents', bracketed; where the record says an estimate
is optimistic, it says so.  Nothing here is implemented.

The numbers everything is priced against (release build at commit
d579c50, `./oggcomp c -v`):

| file | out | headers | pages | floor | class | digits | bits per digit |
|---|---|---|---|---|---|---|---|
| music-stereo-q5 | 143,003 | 0.13% | 0.31% | 2.42% | 2.35% | 94.8% | 1.558 |
| music-managed-b96 | 83,281 | 0.22% | 0.52% | 4.53% | 2.02% | 92.7% | 1.099 |
| chirp-stereo-q10 | 114,673 | 0.16% | 0.35% | 6.76% | 1.04% | 91.7% | 3.142 |
| multi6-48k-q4 | 3,316 | 6.4% | 1.6% | 24.3% | 2.5% | 65.2% | 0.955 |
| uncoupled-stereo-q4 | 1,548 | 11.6% | 3.1% | 29.1% | 2.0% | 53.9% | 0.684 |
| sweep-mono-qm1 | 1,897 | 8.95% | 3.99% | 36.5% | 4.95% | 45.3% | 0.870 |
| tiny-8k-q0 | 192 | 50.9% | 12.7% | 12.8% | 2.6% | 20.0% | 0.875 |
| the 1,257-clip set, solid | 5,443,469 | 0.00% | 0.77% | 7.8% | 3.0% | 88.3% | 1.430 |

Two corrections to earlier documents fall out of that table.  The floor
is not "under 5% of the output" in general: it is 12 to 36% on every
small or tonal file and 7.8% on the clip set, at 3.36 bits a post there
against 1.25 on music.  And the pages stage is 1.6 to 12.7% on the small
files.  `IMPROVEMENTS.md` 4.7 dismissed both from the music file alone.

## 1. What the five agents agreed on without being asked

Four things were found independently by three or more of them, and all
four were confirmed in the code.

**The digit model never sees the floor.**  `digit()` in oc_residue.inc
builds ten context indices from thirty inputs and not one is derived
from the floor curve, though every channel's floor is decoded before any
residue digit of the packet and its posts are in `fl_fy`.  A residue is
the spectrum divided by the floor; the model is blind to the divisor.
The class model, eight variables, has no floor either.

**Structure is coded in the wrong order.**  Classes are coded a
classword at a time, `pv` partitions per codeword, interleaved with the
digits of those partitions (vb_packet.inc `residue()`), so a digit sees
its own class and nothing of the classes above it.  Packet lengths are
coded in the page header before the payload (oc_frame.inc `page()`),
and the decoder could derive them from the payload instead.  The page
boundaries and granule positions follow libogg's flush rule and the
block-size sum, and are coded as if they were free.

**The model starts cold and stays sparse.**  A counter's first visit
returns one half (`cm_cnt::P()` at `t == 0`), the codebook prior is
handed to the mixer as zero when it is dead (`mx.set(5, prp >= 0 ? … :
0)`), and on the music file the `A` table touches 7,774 rows for
695,824 digits, so the zero node of a row is updated about ninety times
in a whole file and a handful of times in a 5 kB clip.  The clip set's
4% cold-start loss is the visible end of this.

**libvorbis's rules are computable from decoded data.**  Checked in
psy.c, res0.c and floor1.c: noise normalisation sorts the sub-threshold
values of a 16 or 32 coefficient block by magnitude and promotes them to
±1 in that order while an energy accumulator holds, preserving the sign;
the residue classifier takes the first class whose `classmetric1`
bounds the partition's largest magnitude and whose `classmetric2` bounds
its sum (`_01class`) or the angle channel's maximum (`_2class`); and the
floor fitter's memo skips every later post of a bracketing interval once
one has been left unused, so those posts code zero.  None of the three
is a context in the model today.

## 2. The ideas, merged and ranked

Ranked by expected gain times confidence, divided by cost; the agent
numbering is `report.idea`.  Gains are of the output, on music unless
said.

| rank | idea | from | stage | estimate | cost | verdict |
|---|---|---|---|---|---|---|
| 1 | the floor bus: floor-derived contexts for digits and classes | 1.2, 5.8, 1.7 | digits, class | 0.5–2% music, 1–3% clips and small | small | build first; it is the prerequisite for a third of the list |
| 2 | encoder-rule contexts: noise-norm grid counts, classifier quota, floor memo | 5.4, 1.3, 3.3, 5.7, 1.10, 3.4 | digits, floor | 0.5–2% music, 1–3% low bitrate; floor memo 1.5–4% on small files | small each | mechanisms verified in libvorbis; measure each on the 7.4 harness |
| 3 | cold counters seeded from the prior; the dead-prior factor; a learned APM blend | 2.1, 2.5, 2.8 | all | 0.2–0.6% music, 1–3% clips | a dozen lines | cheapest model change with a real mechanism |
| 4 | hoist all classes ahead of all digits, then forward class context and the packet's digit count | 4.2, 5.3, 3.8 | digits, class | 0.3–1.5% | moderate, format | the one reorder that attacks the 95% |
| 5 | derive the page layout: lengths from the payload, boundaries from the flush rule, granules from block sizes | 3.2, 4.4, plan 6.4 | pages | 0.3% music, 0.7% clips, 4–10% small | large restructure, format | rule predicts 99 of 119 corpus boundaries; a flag per page covers the rest |
| 6 | signs: phase history per bin, the previous bin's sign, the two-tap resonator | 5.5, 1.6, 1.1, 2.7, plan 4.5 | signs (26% of music) | 0.3–1.2% music, more on tonal | small to moderate | `q1s` is dead in every sign index while `q2s` is live: a real hole |
| 7 | trained prior keyed by the codebook-table row | 5.13, 3.10, 2.9, plan 6.5 | all | 2–4% clips and small files, 0.1% music | moderate, table | the row is known before the first digit |
| 8 | setup records and vendor strings as table rows | 3.1 | headers | 46% of tiny-8k-q0, 3.3% of a `-c` clip archive, 0.1% music | generator + table | the codebook table's argument, one level up |
| 9 | the mantissa path gets the head path's machinery; thresholds split by node | 2.6, 2.4 | digits at high rate | 2–4% on chirp-q10, 0.3–0.8% music | moderate | the weakest sub-model carries the worst file |
| 10 | an indirect model: per-context bit history, shared state map | 2.2 | zero node | 0.5–2% | moderate | the standard next component of a CM coder; 7.4's "odd runs" are its evidence |
| 11 | container: order streams by similarity; frame solid archives once; carry `dg_avg` | 4.3, 4.5, 4.11 | archives | 0.5–2%, 0.4–1%, 0.2–0.8% | low, archive format | measured by the agent on synthetic containers |
| 12 | container: field-level comment cache, parsed tags, a match model on the byte path | 4.1, 4.12, 4.8 | headers, raw | 40–70% on tagged albums, 20–45% on text-heavy containers, 0 on music | moderate | oggcomp loses to xz by 3.6x on a re-tagged album today |
| 13 | predict in the coefficient domain: floor times residue, EWMA and trend per bin | 5.1, plan 4.2 | digits | 1–3% tonal, 0.5–1.5% music | moderate | needs the floor bus; overlaps 6 |
| 14 | a container prelude or grouped solid, so `-c` gets `-S`'s ratio | 4.7 | archives | 2–4% of a `-c` clip archive | high | measure the ceiling first: code `-S` twice on one instance |
| 15 | skip decisions the codebook prior has already decided | 2.11, 3.6 | digits | 0.1–0.3%, and 15–25% of the time | small, correctness edge | the ratio part is counter hygiene; fuzz it |
| 16 | a per-book symbol tree shaped by the prior | 5.6 | digits | 0.3–1% music, 1–2% at q10, plus speed | moderate, format | fewer, better-balanced nodes |
| 17 | re-aim the optimizer: per-file objective, held-out set, the clip set in the corpus | 2.13 | all | gates every number above | a day of perl | do it before measuring anything else |
| 18 | a second mixer layer over complementary keys; a surprise-keyed APM | 2.10, 5.12 | all | 0.3–0.8% | moderate | the best-attested and least imaginative item |
| 19 | floor: envelope motion compensation; a refit predictor; the absolute-zero attractor | 1.5, 1.8, 3.9 | floor | 3–10% of sweep, 1–4% of tonal small files, 0.1–0.4% music | small to moderate | the floor is a third of the small files |
| 20 | per-serial page state; window shape and history age; the coupled partner across passes | 4.10, 1.9, 3.11 | pages, digits | 1–3% of multiplexed files; 0.2–0.8% music; 0.1–0.3% | trivial | small, certain, cheap |

Three pairs are mutually exclusive or overlap enough to be one decision:

- **The packet's bit budget as a context** (2.3, 5.14) against **deriving
  the lacing from the payload** (5).  The budget needs the length before
  the payload; derivation removes it.  The budget's ceiling is the mutual
  information between a packet's length and its content, which the wild
  agent measured at 2.4 bits per packet on music, 0.1 to 0.2% of output;
  derivation takes the whole pages stage.  Derive.
- **Cold counters seeded from the prior** (3) against **trained per-book
  corrections** (2.9) and **a trained initial state** (7): the first is a
  tenth of the work of the others and should be measured before either.
- **The resonator** (1.1) against **the coefficient-domain predictor**
  (13): the same reconstruction, one predicting sign and magnitude from a
  two-tap recursion, the other magnitude from a trend.  Build the floor
  bus, dump the reconstructed spectrum, and score both offline.

Two families the reports circled and the record already closes: run-
length coding of zeros (`IMPROVEMENTS.md` 7.4) and any two-pass scheme
that codes the zero map before the magnitudes, which breaks the codebook
prior's trie (4's first wild card says so itself).  Huffman
re-optimisation and wider hash tables are likewise done.

## 3. The ideas by theme

### 3.1 The signal

The floor bus (1.2) renders each channel's floor once per packet, in the
dB units floor1 uses, and exposes at each digit the level at its bin,
the change since the previous packet, the local slope, the curvature,
and whether the bracketing posts coded zero.  Four mechanisms: a band at
the hearing threshold is all zeros whatever `band` says; floor1 is
piecewise linear in dB and the residue is systematically large where the
chord undershoots the envelope, at sub-partition resolution, which is
what `col` lacked; the posts' zeros are a free measure of how hard the
encoder worked there; and it needs no history, so it is live on packet
one, which is where the clip set loses.  Cost is one render per channel
per packet and about 80 instructions per digit through `tc_make_dig`.

On that bus: a two-tap resonator per bin (1.1), exact for a sinusoid,
`X_m = a X_{m-1} − X_{m-2}` with `a` set by the partial's offset from the
bin centre, predicting the sign as well as the magnitude; a per-bin
tonality byte (1.4), the leaky mean of the predictor's recent error, so
the mixer can trust temporal predictors where they work; a coefficient-
domain EWMA and trend (5.1) as the general form of the plan's floor-
normalised history; and window shape and history age (1.9): the two
block sizes keep separate histories and neither is told that after a
short run the long history is twenty packets old.

For the floor itself: motion compensation (1.5), an estimated frequency
shift of the previous curve before `ep` is computed, aimed at sweeps
and vibrato where the floor is 36% of the file; a refit predictor (1.8)
that runs a line fit over the previous packet's reconstructed spectrum
the way floor1's fitter does; the absolute-zero attractor (3.9), a
permutation of the folded alphabet so a post clipped to zero costs two
decisions rather than nine; and an ATH prior for the first packet.

The honest caution, from agent 1 itself: the residue is quantised to a
handful of levels after division by the floor, and `cls` and `pq` may
already carry most of what the level says at partition resolution.  The
first-look dump decides that in a day.

### 3.2 The encoder's rules

Three deterministic rules, each verified in libvorbis and each absent
from the model.

*Noise normalisation* (5.4, 1.3): within each aligned block of
`normal_partition` coefficients above `normal_start`, the number of ±1
values is a function of the block's energy and their positions are the
largest residuals.  Contexts: position within the block, the count of
±1 and of nonzeros so far in it, and an energy budget carried from the
previous packet.  It is the structure 7.4 saw as "odd run lengths" and
could not name: a count within a grid, not a run.  The block size is not
in the stream; try 16 and 32 and let the optimizer pick.

*The classifier* (3.3, 5.7, 1.10, 3.5, 3.7): class `k` means the
partition's maximum is within `classmetric1[k]` and its sum within
`classmetric2[k]`, and also that some digit did not fit class `k − 1`.
Contexts: the running maximum, sum and nonzero count in the partition,
the slots left, and the derived quota state, "the class promises a
value above `T`, none has come, four slots remain".  This is CABAC's
last-significant-coefficient logic, and it lands on the zero decision,
which is 37 to 59% of the digit bytes.  Sub-class resolution (3.5) adds
where in its band the partition sat last packet; the thresholds are
identifiable online from decoded data.

*The floor memo* (3.4): floor1_fit's `if(memo[ln] != hn)` means that
once a post in a bracketing interval is left unused, every later post in
that interval codes zero.  Posts sharing an interval share every current
floor context, so the model cannot tell the first from the rest.  About
twenty lines mirroring the fitter's neighbour bookkeeping.

*The setup records* (3.1): floors, residues, mappings and modes are
template lookups in vorbisenc.c exactly as the books are, so the rest of
the setup packet is a table row too, keyed per record, and the vendor
string is one of forty.  Tiny-8k-q0 would go from 98 header bytes to
about 10.

*The page layout* (3.2, 4.4): libogg's flush rule and the block-size sum
reproduce most page boundaries and every non-final granule; code a
"prediction held" flag per page and a correction where it did not.

Everything in this theme degrades gracefully on a non-libvorbis stream:
the variable is noise, the optimizer zeroes it, and it costs the tuning
slot and nothing else.

### 3.3 The machinery

Agent 2 read the coder and found four defects and several missing
components, ordered by evidence.

- A cold counter returns one half; seed it from the codebook prior for
  the same node, or from the direct table `C` (2.1).  `s_pq` is all
  zeroes; a dead prior reaches the mixer as exactly one half; the final
  APM blend is a fixed 11/16 where a two-input mixer belongs (2.5).
  Counter targets are clamped to [0.0625, 0.9375], so a deterministic
  context and a 90% context hand the mixer nearly the same number (2.8).
- The mantissa path is one counter and a three-input mixer with no APM,
  against the head path's four counters, two APMs and seven inputs; on
  chirp-q10 at 3.14 bits a digit the tail is the program (2.6).  The ten
  indices are built once per digit, so the zero node and the tail share
  every threshold (2.4).
- An indirect model, an 8-bit outcome history per context with a shared
  state map (2.2), sees the order of outcomes that a counter discards.
- A second mixer layer over complementary keys (2.10), a surprise-keyed
  APM (5.12), a learned decision-tree context quantiser (5.9), a
  packet-level match model over the last 64 packets (2.14), and per-file
  parameter profiles chosen by the encoder in three bits (2.12).
- The optimizer (2.13): its objective sums bytes, so the music file
  outvotes the smallest by 700 to 1; there is no held-out set; the clip
  set is not in the corpus.  "The search is near its floor" cannot
  currently be distinguished from "out of time".  Fix this before
  measuring anything else.

Two speed items double as ratio items: skipping decisions the prior has
already decided (2.11, 3.6), whose real gain is that deterministic
outcomes stop updating shared counters, and a per-book symbol tree
balanced by the prior (5.6), which cuts the decisions per digit by a
third to a half.  The first has a correctness edge, a zero-weight test
that both sides must compute identically, and wants the fuzz harness of
`OPTIVORBIS.md` 3.5.

### 3.4 Structure and containers

Within a stream: hoist every class of a residue ahead of every digit
(4.2, 5.3), which the decoder can undo, then give digits the classes
above and below, the packet's bandwidth and the exact digit count
remaining.  Column-major order within a partition (4.6) gives half the
digits a neighbour on the right while keeping the prior's trie, at the
price of `q1`'s meaning.  Hoisting a page's floors ahead of its residues
(4.9) gives the digit model the next packet's floor as well as the last.

Across streams, agent 4 measured on synthetic containers: grouping
streams by parameters before solid coding is worth 0.65 to 1.48%
(4.3); solid framing costs 12 bytes a stream and the metainfo 2.5 times
its content (4.5), which is the 27 kB of `IMPROVEMENTS.md` 7.3; the
model carries statistics between streams but no content, so a gain,
fade or trim variant costs what an unrelated stream costs (its M4); a
comment header differing in one byte from one seen before re-codes
every byte of it at 5.2 bits, and xz beats oggcomp 3.6x on a re-tagged
album (4.1); and gap bytes in a text-heavy container code at 4.4 bits a
byte where a match model would find every repeat (4.8).  The prelude
(4.7) would give `-c` most of `-S`'s ratio while keeping random access,
and its ceiling is measurable today by coding a container solid twice
on one instance.

### 3.5 Cold start

The clip set loses 4% to a model that begins every 5 kB stream at zero.
Four answers of increasing cost: seed counters from the prior (2.1);
ship a small trained correction per table book (2.9); ship a trained
initial state keyed by the codebook-table row, which names the encoder
and its mode before the first digit (5.13, 3.10); carry the decayed
averages across solid segments (4.11).  The first is an afternoon, and
the measurement that bounds all four exists: `oggdet -S` against `-c`.

## 4. The order of experiments

The reports converge on one instrument.  Extend the 7.4 dump, the
per-digit log the run-length question was settled with, to carry the
floor level at the digit's bin, the neighbouring classes, the
partition's running maximum, sum and slots left, the noise-norm grid
position, the sign history at the slot, and the packet's bit budget;
and split the `-v` accounting by node rather than by stage, so the zero,
head, length, mantissa and sign nodes each report their bytes.  One log
and one table answer ranks 1, 2, 4, 6, 9, 10 and 13 before any model is
built, in the same afternoon-per-question style that closed 7.4.

Then, in order:

1. Re-aim the optimizer (17): per-file objective, a held-out set, the
   clip set in the corpus.  Every later number depends on it.
2. The cheap machinery fixes (3): counter seeding, the dead-prior
   factor, the learned blend.  Each is a version and a corpus total.
3. The floor bus (1) and the encoder-rule contexts (2), each at pattern
   zero, each placed by `opt.pl`.
4. The structural version (4, 5, 8, 11): classes hoisted, page layout
   derived, setup records in the table, solid archives framed once and
   streams ordered.  These change the format and should ship as one.
5. Signs (6) and the coefficient-domain predictors (13), once the floor
   bus exists to feed them.
6. The trained prior (7), measured against what seeding (3) already
   recovered.
7. The mantissa path and the split thresholds (9), the indirect model
   (10), the second layer (18).
8. The container items that need real material (12, 14): a tagged
   album and a text-heavy archive belong in the corpus before they are
   judged.

What the corpus needs for any of this to be measured honestly, from
three reports independently: a long uncoupled stereo file and a long
5.1 file (already in `IMPROVEMENTS.md`), a transient-rich file, a
loop-based or repeated-phrase file, a tagged album with art, and the
clip set itself.
