# Agent 2 — the modelling machinery

Angle: counters, mixers, APMs, binarization, priors, cold start, ensembles,
indirect contexts, and the tuning process itself.  Grounded in `cm.inc`,
`tc_fam.inc`, `tc_ptab.inc`, `oc_residue.inc`, `IDX/tsvcomp-dig.idx`,
`IDX/opt.pl`.

Standing costs that apply to every idea below and are not repeated in each:
a model change is a new `OC_VER`, a `./mk.sh check`, a `./t.sh`, and a tuning
pass with `IDX/opt.pl`; and the model already spends ~1,200 instructions per
binary decision, of which `cm.inc` is 27% and `tc_make_dig` 24%.

Numbers I measured while reading (release build, `-v`):

| file | digits | bits/digit | digits share |
|---|---|---|---|
| music-stereo-q5 | 695,824 | 1.558 | 94.8% |
| music-managed-b96 | 561,760 | 1.099 | 92.7% |
| chirp-stereo-q10 | 267,696 | **3.142** | 91.7% |
| noise-stereo-q3 | 143,648 | 1.282 | 94.3% |
| multi6-48k-q4 | 18,105 | 0.955 | 65.2% |

`chirp-stereo-q10` is the worst ratio in the corpus (89.1%) and its digits are
3.14 bits each — i.e. dominated by the **length and mantissa** nodes, which get
the *weakest* machinery in the whole model.  That asymmetry shapes several
ideas below.

Table-occupancy arithmetic that also shapes them: on `music-stereo-q5` the
`dig.a` table touches 7,774 rows for 695,824 digits, so the *zero* node of an
`A` row is updated about 90 times on average over a whole 168 kB file, and the
length/mantissa nodes of the same row far fewer.  On a 5 kB clip it is 0 to 2.
**Counter A is a sparse, mostly-cold table even on music.**  Everything that
makes a cold counter useful pays twice: on the clips, and on music.

---

### 1. Cold counters inherit instead of starting at a coin flip
- **Idea:** `cm_cnt::P()` returns `CM_PHALF` when `t == 0`, and `upd()` starts
  its first move from `CM_CONE/2`.  Every first visit to a row of `A`, `B` or
  `D` therefore contributes a dead 0 to the mixer and then walks away from 1/2.
  Replace the seed with something the model already has and that is *already
  good*: the codebook prior for this very node (`prp`, computed one line
  earlier in `code()`), falling back to counter `C`'s probability (`C` is the
  direct, Volume-1080, always-warm table).  Concretely `int pa = a.t ? a.P() :
  (prp >= 0 ? prp : e.P());` and the same seed handed to `upd()` as `cur`.  Add
  one tunable per table choosing prior / coarse-sibling / half, so `opt.pl`
  decides per table.
- **Why it might work:** it is context-tree weighting's core move — a cold
  child inherits its parent — done with two values that are free at the point
  of use.  The prior is a calibrated probability derived from the codebook's
  own Huffman lengths, so it is *much* better than 1/2 for a first visit; and
  `C` carries the coarse statistics of this file.  Today the mixer has to
  compensate with a weight that is shared between cold and warm visits of the
  same weight set, which it cannot do — that is the exact loss.
- **Expected gain:** 0.2–0.6% of output on music (roughly a third of `A`'s
  first-visit mass is dead weight today), 1–3% on the 5 kB clip set, where §7.3
  measured the whole cold-start loss at 4%.  Confidence: high that the sign is
  positive, medium on the size.
- **Cost:** a dozen lines in `cm.inc`/`tc_fam.inc`; `cm_cnt::upd` grows a seed
  argument.  Per-symbol CPU: one extra branch (predictable), no extra memory.
  Stream format: yes (it is a probability change), no layout change.
- **Risks:** the prior may be *worse* than 1/2 on nodes where the codebook is
  systematically miscalibrated (Huffman lengths are rounded, always up for rare
  symbols), and seeding all four counter tables with the same prior makes them
  correlated, which weakens the mixer's ensemble.  Mitigate by seeding only
  `A` from the prior and `B`/`D` from `C`.
- **How to measure it cheaply first:** instrument `bit()` to accumulate
  `-log2` of the coded probability, split by `a.t == 0` vs `a.t > 0`, and
  separately accumulate what the cost *would have been* had the mixer been fed
  `st(prp)` instead of `st(A.P())` in the cold case (a counterfactual on the
  same mixer weights is not exact, but the sign is informative).  Cheaper still:
  log `(prp, a.t, bit)` for a million decisions and compute the empirical
  conditional entropy of `bit` given `prp` restricted to `a.t == 0` — if the
  prior alone beats 1 bit there by a wide margin, the seed is free money.

---

### 2. An indirect model: per-context bit history, globally-shared state map
- **Idea:** add to the `A` row one byte per node holding the last eight
  outcomes at that (context, node) — a shift register, not a probability — and
  a **shared** table `SM[history][node-group][prior-bucket] -> cm_cnt` whose
  prediction becomes an eighth mixer input.  The history byte is per-context
  (sparse, cold); the state map is global (dense, warm after a few hundred
  decisions), so the pair works on the first packet of a 5 kB stream.  The
  richer version is the standard nonstationary 8-bit state machine (lpaq/zpaq's
  `nex()`) instead of a raw shift register.
- **Why it might work:** the counter throws away *order*.  A context whose zero
  decisions have gone `1 0 1 0 1 0 1 0` and one that has gone `1 1 1 1 0 0 0 0`
  have the same `p` and the same `t` and are told apart by nothing in the
  model.  MDCT residues have exactly that structure: a harmonic comb puts
  nonzeros at a periodic spacing along frequency, and a partition's vector
  positions alternate.  §7.4's own evidence points here: "odd lengths dominate
  on the managed file, a zero vector of dimension two ending a run at an even
  boundary" — that is a *pattern*, and the current machinery can only see its
  mean.  Also: the shared state map is the fastest-warming component that could
  be added, which is what the clip set needs.
- **Expected gain:** 0.5–2% of output on music (the generic CM figure for
  adding an indirect model over a direct one), 1–3% on the clips.  Confidence:
  medium-high in the literature, medium here, because the digit alphabet is
  tiny and the prior already carries much of the structure.
- **Cost:** moderate implementation (a new table, a new mixer input, a state
  machine table if the nonstationary form is used).  Memory: 2^17 × 29 bytes =
  3.8 MB for `A`'s history, plus a few hundred kB for the state map — trivial
  next to 366 MB.  Per-symbol CPU: one load, one store, one extra mixer lane:
  perhaps 5% of the decision.  Stream: yes.
- **Risks:** eight bits of history over a context visited 90 times is itself a
  sparse context; the state map's own context (node-group, prior bucket) has to
  be coarse enough to warm up and fine enough to matter.  And the history may
  simply repeat what `q1`, `zrun`, `vpos` already say, in which case the mixer
  learns to ignore it and the whole thing is a 5% speed loss for nothing.
- **How to measure it cheaply first:** the §7.4 methodology exactly — log
  `(index a, node, bit)` for `music-stereo-q5`, `music-managed-b96` and a clip,
  and offline compute (a) `H(bit | context)` with an adaptive counter, (b)
  `H(bit | context, 8-bit history)` with a shared state map.  If (b) is not 3%
  under (a) on the zero node, drop it.  This is a one-evening perl/python job
  on a log and costs no build.

---

### 3. The packet's bit budget as a context
- **Idea:** the page header, with every `p.plen[i]`, is coded **before** the
  payload (`oc_frame.inc`), and `io_t` carries `pos` and `len`, so both sides
  know, at every point in every packet, exactly how many bits are left in it.
  Nothing in the thirty digit variables uses that.  Add three: `plq` =
  `tc_qlog(len*8)` (the packet's size), `blq` = `tc_qlog(len*8 - pos)` (bits
  remaining), and `bpv` = bits remaining divided by residue slots remaining in
  the known geometry, log-quantised.  Compute them once per partition in
  `part_begin`, not per digit.
- **Why it might work:** it is *new information about the future*, not a
  rearrangement of the past.  The packet's length is the total residue energy
  of this packet, known before the first digit; today the model has to infer it
  from `ax`, `t1`, `zrun` as the packet unrolls, which is exactly the "the
  model has not settled" complaint.  Measured on the corpus, packet lengths
  have sd/mean of 0.24–0.41 and 0.8–2.9 bits of entropy in 6%-wide buckets, so
  there is real spread to condition on.  And near the end of a packet the
  budget is a *hard* constraint: the remaining codewords must sum to the
  remaining bits up to ≤7 bits of padding, so the last handful of values in
  every packet become nearly determined.
- **Expected gain:** 0.5–2% of the digit bits on bitrate-managed and variable
  material (`music-managed-b96`, the clips), 0.3–1% on `music-stereo-q5`,
  little on `chirp-stereo-q10` where the packet lengths barely move (1.29 bits
  of entropy).  Confidence: medium-high that it is positive, because the
  information is genuinely absent today.
- **Cost:** small implementation — thread `len*8 - pos` from `io_t::rs_part`
  into `part_begin`, three new `tc_dv` fields, three new `.idx` lines at
  pattern zero.  Per-symbol CPU: near zero (per partition, and `tc_make_dig`
  gains three quantised factors, ~20 instructions per digit).  Memory: the
  index Volumes grow by the buckets the optimizer chooses.  Stream: yes.
- **Risks:** it **collides with the plan's §7.3 item** "code the payload before
  the lacing and derive the lacing from it", which would remove the packet
  length from the decoder's knowledge at payload time and is worth 0.3% on
  music by itself.  These two are mutually exclusive and should be measured
  against each other.  Also, on strict-CBR streams the length carries nothing.
- **How to measure it cheaply first:** no model change needed — dump, per
  packet, its byte length and the count/mean-magnitude of its residue digits
  (an instrumented encoder printing two numbers per packet), then compute the
  mutual information between quantised `plen` and the per-digit zero indicator,
  *conditioned on* `blk` and on a running mean of recent packet lengths (so the
  measurement does not credit information the model already has).  Half a day.

---

### 4. Separate threshold sets for the zero decision and the magnitude tail
- **Idea:** the ten indices of the digit family are computed once per digit and
  the tree node only offsets *within* the selected row, so `a_cls`, `a_band`,
  `a_pq` and the rest are the *same quantisations* for the "is it zero"
  decision, the "is it one" decision, the length nodes and (via `t`/`n`) the
  mantissa.  `opt.pl`'s objective is total bytes, so the patterns are dominated
  by whichever node carries the most bits — the zero node at 37–59% of digit
  bytes on low-rate material, the length/mantissa nodes at `-q 10`.  Emit two
  builders from the `.idx` (one extra word per line saying "zero-node pattern"
  / "tail pattern"), give the tail its own tables, and let the optimizer tune
  each against the traffic that actually uses it.
- **Why it might work:** it is the cheapest way to buy modelling capacity where
  it is presently rationed by an averaging objective.  The evidence that the
  two decisions want different contexts is in the `.idx` comments themselves:
  `zrun` "measures at nothing on a 350 kbit/s mono residue ... on a 96 kbit/s
  stereo one ... worth 3% of the file" — the same tension, one file against
  another, and here it is one *node* against another inside every file.
  `chirp-stereo-q10` at 3.14 bits a digit is a file where the tail is the
  program: only 14% of its digits are zero, so the zero node decides little and
  the length and mantissa nodes decide almost everything.
- **Expected gain:** 0.3–1% on music, 1–3% on `-q 10`-like material (the worst
  file in the corpus), 0.2–0.8% on the clips.  Confidence: medium.
- **Cost:** moderate — `idx2inc.pl` grows a per-line modifier and emits two
  index builders; `tc_fam` grows a second `select()` for the tail; tables
  roughly double for the digit family unless the tail's `h*` are cut (the tail
  sees ~45% of the decisions and can afford fewer rows).  Per-symbol CPU: the
  tail indices need only be built when the digit is nonzero (53% on music, 33%
  on the managed file), so this can be **cheaper** than today on low-rate
  material if `tc_make_dig` is split lazily.  Stream: yes.
- **Risks:** the optimizer's search space roughly doubles, and §3 already says
  "the parameter search is near its floor" — twice as many patterns over the
  same corpus is a fast route to overfitting.  Needs the corpus work of idea 13
  first.
- **How to measure it cheaply first:** before writing the split, run `opt.pl`
  twice on the *current* binary with two different objectives: once minimising
  only the digit-stage bits of the low-rate files, once only those of
  `chirp-stereo-q10`, and compare the two resulting pattern sets.  If they
  disagree materially on `cls`/`band`/`pq` the split has something to win; if
  they converge, it does not.

---

### 5. Put the current node's prior into the APMs, and learn the final blend
- **Idea:** three small changes around the prior.  (a) `s_pq` is all zeroes and
  `f_pq` uses only a handful of buckets — but in any case `pq` is the prior of
  a *zero*, fixed for the whole digit, while `prp` is the prior of the node
  being coded.  Key a third APM (or re-key `ap2`) by `tcp_bucket(prp)` × node,
  a 33×29 table that warms almost instantly.  (b) `mx.set(5, prp >= 0 ?
  cm_stretch(prp) : 0)` makes a *dead* prior indistinguishable from a prior of
  exactly one half — the prior dies after `t->nlev` levels of the trie, and the
  mixer's weight for that lane is shared between the two cases.  Add
  `prp < 0` as a factor of the mixer index `m` (Volume 55440, plenty of room).
  (c) `pf = (bw*pm + (16-bw)*pf + 8) >> 4` with `bw = 11` is a *fixed* 11/16
  blend that damps the second APM to 5/16 everywhere; replace it with a
  two-input logistic mixer keyed by node, which is four multiplies.
- **Why it might work:** the prior is the only genuinely *calibrated* estimate
  in the pipeline, and a table keyed by its bucket is a pure calibration model:
  it learns "when the codebook says 0.7, the truth is 0.66", which transfers
  across books, positions and files and therefore warms in tens of decisions
  rather than hundreds.  §4.6 names (a); (b) is a plain defect and (c) is a
  constant where a learned quantity belongs.
- **Expected gain:** 0.2–0.6% on music, 0.5–1.5% on the clips (all three
  together), of which (b) is the cheapest and probably 0.05–0.2% alone.
  Confidence: high for (b) and (c), medium for (a).
- **Cost:** small.  (a) one more APM: two loads, two updates, one mixer lane —
  perhaps 6% of the decision; memory 33×29×q per context set, negligible.
  (b) one factor in `tc_make_dig`, free.  (c) four multiplies.  Stream: yes.
- **Risks:** a third APM in series with two others over-corrects and the chain
  starts chasing noise; the usual remedy is to mix rather than cascade, which
  costs another lane.  (a) may also just duplicate what `pq` already buys.
- **How to measure it cheaply first:** log `(prp, node, pr_final, bit)` for a
  million decisions and fit a one-dimensional calibration curve of `bit` on
  `tcp_bucket(prp)` conditioned on `pr_final`; the area between that curve and
  the diagonal, integrated against the traffic, is an upper bound on what (a)
  can win, computable in a script with no rebuild.  For (b), simply count what
  fraction of digit decisions have `prp < 0` — if it is under 2% the defect is
  not worth a version.

---

### 6. Give the mantissa path the machinery the head path has
- **Idea:** `bitm()` is one counter table `T` and a three-input mixer (`T`,
  `prp`, bias), with **no APM at all**, against `bit()`'s four counters, two
  APMs and seven-input mixer.  Add to `bitm`: a second counter table under its
  own index, an APM on the mixed probability keyed by a new index, and the
  `prp < 0` and confidence lanes of ideas 5 and 8.  Also look again at the
  length nodes: they go through `bit()` but share `A`..`D` rows and thresholds
  with the zero node (see idea 4).
- **Why it might work:** at `-q 10` digits cost 3.14 bits each and 91.7% of the
  output is digits; a digit of magnitude 4 walks seven binary nodes of which
  three are length and mantissa.  The weakest sub-model in the program is
  carrying the largest share of the bits on exactly the file with the worst
  ratio in the corpus.
- **Expected gain:** 2–4% of `chirp-stereo-q10` (≈89.1% → ≈87.5%), 0.3–0.8% on
  `music-stereo-q5`, near zero on `music-managed-b96` where mantissas are rare.
  Confidence: medium-high — the head path's machinery was presumably added
  because it paid, and there is no reason the mantissa is different in kind.
- **Cost:** moderate.  Memory: `MT` is 169 nodes per row at 2^14 rows = 11 MB;
  a second counter table and an APM at the same size is perhaps +60 MB, which
  is within the existing 366 MB budget's slack.  Per-symbol CPU: mantissa bits
  become as expensive as head bits — on `-q 10` that is a real slowdown (maybe
  15%), on low-rate material nothing.  Stream: yes.
- **Risks:** high-rate Vorbis is a small share of real files, so this improves
  the corpus's worst file and few others; and mantissa bits may genuinely be
  near-random, in which case the extra machinery buys 0.1%.
- **How to measure it cheaply first:** instrument `tc_bits` per node group
  (head 0/1/2, length, mantissa, sign) rather than per stage — a one-line
  change to the existing `-v` accounting — and run the corpus.  That single
  table tells you where every remaining byte is, per file, and settles ideas 4,
  6 and 11 at once.  This is the highest-value hour in the whole list and
  should be done before anything else here.

---

### 7. A signed linear predictor, and the neighbours' signs
- **Idea:** `ax` is a tuned weighted average of eight *magnitudes*
  (`TC_dig_avA..avX1`) — an energy estimate.  There is no signed counterpart.
  Add `pv = (w1·q1 + w2·t1 + w3·t2 + w4·x1 + w5·n1 + w6·w1 + w7·p0) >> 12` with
  its own tuned `Number` weights, quantised through `tc_sq`, as a context for
  *both* the digit family (magnitude prediction) and the sign family.  Add the
  missing signed axes outright: `t2s`, `n1s`, `w1s` (the sign family has
  `t1s`, `q1s`, `q2s`, `p0s`, `x1s` and no neighbour signs at all).
- **Why it might work:** the MDCT of a steady partial has a main lobe whose
  adjacent bins have a fixed sign relation, and successive frames rotate the
  partial's phase at a rate set by its frequency — so `(t1s, t2s)` jointly
  predict where either alone does not (§4.5 says this for the time axis; the
  frequency axis is not in the list at all).  A single signed linear
  combination packs what today is five low-resolution axes into one axis the
  optimizer can quantise finely, which is exactly the trick `ax` already plays
  for magnitudes and which is why `ax` appears in six of the ten indices.
  Signs are 28% of the digit bits and "nearly random" — a 5% dent in a quarter
  of the output is 1.2%.
- **Expected gain:** 0.3–1% of output on tonal material (`sine`, `chirp`,
  music), near zero on `noise-stereo-q3`.  Confidence: medium.
- **Cost:** small — seven multiply-adds per digit (the same shape as `ax`,
  which is already paid), three new `tc_dv` fields, ten new `.idx` lines at
  pattern zero, seven new tunable weights.  Stream: yes.  The seven new
  `Number`s enlarge `opt.pl`'s search noticeably.
- **Risks:** the residue is the spectrum *divided by the floor* and then
  quantised to a handful of levels, which destroys most phase structure; `ax`
  needed tuned weights and got them, but a signed predictor over a 3-to-9-level
  alphabet may quantise to noise.  `avQ1` sits at pattern zero today, which is
  a hint that the immediate neighbour is worth less than one would think.
- **How to measure it cheaply first:** dump the digit stream with slot
  coordinates (already easy — §7.4 did it) and compute, offline, the empirical
  `H(sign | t1s)`, `H(sign | t1s, t2s)`, `H(sign | w1s, n1s)` and
  `H(sign | quantised pv)` for a few weight vectors found by least squares.
  If none of them is 3% under `H(sign)` = 1 bit, stop.

---

### 8. Confidence-aware mixing
- **Idea:** two related changes.  (a) The counters' targets are clamped:
  `mwA = 64·64 = 4096` of `CM_CONE`, so counter `A`'s probability can never
  leave [0.0625, 0.9375] and `st(A)` never exceeds ±693 of the mixer's ±2047
  range.  A deterministic context and a 90%-context therefore hand the mixer
  nearly the same number and are separated only by a weight they share.  Give
  `A` and `B` a **second mixer lane each**, `st(p)·min(t,T)/T`, so the mixer can
  learn a different slope for confident and unconfident counters within one
  weight set (`TC_MIXN` 7 → 9).  (b) Alternatively or additionally, add a
  two-or-three-bucket quantisation of `A.t` as a factor of the mixer's row —
  note this must go inside `bit()`, since `A.t` varies per node while the index
  `m` is built per digit.
- **Why it might work:** it is the textbook fix for exactly the failure the
  occupancy arithmetic above predicts: on music a typical `A` node has been
  updated ~90 times and a large minority far fewer, so cold and warm counters
  are mixed in the same weight set all the time.  The `mw` clamp is a
  regulariser bought at the price of throwing away the distinction between
  "usually" and "always", and the mixer is the right place to buy it back.
- **Expected gain:** 0.2–0.5% on music, 0.5–1.5% on the clips.  Confidence:
  medium-high.  It overlaps idea 1 — do 1 first and re-measure, because a
  counter that starts at the prior is much less often "cold" in the damaging
  sense.
- **Cost:** (a) four extra multiplies in `mix` and four in `upd`, ~3% of the
  decision; no memory, no new tables.  (b) triples the `W` table (45 MB → 135
  MB) and needs `cm_mix::mix` to take a row computed per node.  Stream: yes.
- **Risks:** more lanes means slower weight learning per lane, which hurts the
  very small files the idea is aimed at; and `bw`, `lr` and `mb` already
  implement a warm-up schedule (`lr_eff` is 79 at the first visit of a weight
  set and 10 at the 255th), so some of this is already bought.
- **How to measure it cheaply first:** accumulate coded bits split by
  `min(A.t, 8)` buckets; if the bits-per-decision curve against `A.t` is flat,
  the mixer is already handling it and there is nothing here.  If it is steeply
  worse at `A.t < 4` and that bucket carries 20%+ of the decisions, the idea is
  live.

---

### 9. Trained per-book priors from the codebook table
- **Idea:** §7.1 made every book of every corpus file a row of a fixed table of
  1,346 known libvorbis/aoTuV/ffmpeg books, identified exactly.  The codebook
  prior is currently derived only from the Huffman *lengths* of that book.
  Ship, per table book, a small trained correction measured over a training
  corpus: at minimum one byte of "temperature" applied as `st(prior)·τ`; better,
  a handful of corrections for the head-0, head-1 and length probabilities at
  trie level 0 and 1.  1,346 books × 8 bytes is 11 kB of constants in the
  binary.
- **Why it might work:** Huffman lengths are integers, so the implied
  probabilities are powers of two and are systematically *wrong* in a direction
  that depends on the book's construction — rare symbols always get too much
  probability.  The adaptive counters learn that deviation on a long file and
  cannot on a 5 kB one.  Since the book identity is known exactly and the same
  few hundred books appear in essentially every Vorbis file in the world, a
  correction trained once transfers.  This is the "parameter prior so small
  files start warm" item of the plan, done in the one place where it is small
  enough to ship.
- **Expected gain:** 1–3% on the clip set (the 4% cold-start is mostly the
  digit model's marginals, which is exactly what this supplies), 0.05–0.2% on
  music.  Confidence: medium — it depends on the deviation being consistent
  across encoders and material, which is measurable before anything is built.
- **Cost:** moderate but self-contained: a training harness (encode a corpus,
  accumulate per-book empirical head/length frequencies, emit a generated
  `.inc`), a table lookup at `tc_ptabs::build` time, nothing per symbol.
  Memory: 11 kB.  Stream: yes, and the table is part of the format forever —
  a book whose correction changes is a version bump.
- **Risks:** overfitting to the training corpus's *material* rather than to the
  book: the same book is used at different bitrates on different music.  Guard
  by training only a monotone scalar recalibration, not a full distribution,
  and by holding out files.  Also: this is the idea most likely to be beaten by
  simply doing idea 1 (seed counters from the prior) for a tenth of the work.
- **How to measure it cheaply first:** instrument the encoder to accumulate,
  per book and per trie level, the empirical head-0 frequency and the prior's
  head-0 probability; dump the pairs for the whole corpus and for the clip set.
  If the scatter shows a consistent bias curve across *files* (not just within
  one), the correction is real and its size bounds the gain directly.

---

### 10. A second mixer layer, contexts chosen to be complementary
- **Idea:** stronger than §4.4's "a second mixer averaged in the stretched
  domain".  Run three mixers over the same seven-plus inputs with deliberately
  *different* keys — the present `m`; `tcp_bucket(prp)` × node (prior-shaped);
  and `cls` × `zrun` × node (regime-shaped) — and combine their three outputs
  with a second-layer mixer keyed by node alone, then the APM chain.  The
  second layer is three lanes, so the whole addition is fourteen extra
  multiplies in `mix` and fourteen in `upd`.
- **Why it might work:** one weight set keyed by `m` must serve every regime;
  a small ensemble whose members disagree is the standard next 0.3–0.8% in
  every CM compressor, and the second layer (rather than a fixed average) lets
  the model learn *when* each member is right.  The prior-keyed member also
  warms in tens of decisions, which helps the clips.
- **Expected gain:** 0.3–0.8% on music, similar on the clips.  Confidence:
  medium-high — this is the best-attested item in the list, at the cost of
  being the least imaginative.
- **Cost:** two more weight tables (each `rows × 29 × 7` i32; the prior-keyed
  one is tiny at 33 rows, the regime-keyed one modest) plus a small
  second-layer table.  Per-symbol CPU: ~15% of the decision.  Stream: yes.
- **Risks:** three mixers over the same four counters are highly correlated and
  the ensemble adds little; the second layer then just learns a fixed average
  and the whole thing is 15% slower for 0.05%.  The fix is to make the members
  see *different inputs*, not only different keys — e.g. give the prior-keyed
  member the indirect model of idea 2 and not `D`.
- **How to measure it cheaply first:** log the four counter probabilities, the
  prior, and the bit for a million decisions, and fit the ensemble offline in a
  script (a logistic mixer is twenty lines).  Compare one-mixer and three-mixer
  offline losses on the same log.  No rebuild, and the answer also tells you
  which keys are complementary before you pay for any of them.

---

### 11. Skip the decisions the codebook has already made
- **Idea:** at a prior trie node the weights `s012`/`slen`/`tot` say which
  magnitudes are *possible* — a symbol with zero weight has no codeword with
  this prefix and cannot occur.  `tcp_p` clamps that to [1, 4095] and the
  information is thrown away.  Detect determinism from the raw weights and skip
  the node entirely: if every live symbol at this node has magnitude ≥ 1, do
  not code "is it zero"; if none has magnitude 2, do not code "is it two"; if
  one symbol is left, do not code anything.  Both sides see the same table, so
  this is exact.
- **Why it might work:** two gains.  The direct one is the ~0.00035 bits each
  such decision costs at the 12-bit clamp — small.  The real one is that those
  deterministic outcomes are *updating shared counters*: a book with three
  values (−1, 0, +1) makes node 1 deterministic for every nonzero digit, and
  those updates land in the same hashed rows as genuinely uncertain node-1
  decisions from other books, dragging them.  Removing them cleans the
  statistics.  And it is a real speed win — every skipped node is 1,200
  instructions.
- **Expected gain:** 0.05–0.3% of output; 5–15% of encode/decode time.
  Confidence: high on the speed, low-medium on the ratio.
- **Cost:** small — a per-node precomputed "live mask" alongside `ph`/`pl` in
  `tc_ptab::finish`, three or four `bytes` per node, and three branches in
  `code()`.  Memory: within the existing 64 MB prior budget.  Stream: yes.
- **Risks:** the prior dies after `t->nlev` levels and for books too big to
  table, so the skip is available only sometimes — and a rule that fires
  sometimes makes the counters' population *change*, which invalidates the
  current tuning.  Also a correctness hazard: a skip that the two sides compute
  differently (e.g. through the memory-budget-dependent `nlev`) desynchronises
  the stream.  `tc_ptab`'s layout is already noted as stream-affecting, so the
  discipline exists.
- **How to measure it cheaply first:** count, with a probe in `code()`, how
  many head/length decisions have a one-sided prior weight, per file.  If it is
  under 3% of decisions the idea is speed-only; if it is 15% on low-rate files,
  the counter-cleaning argument is worth testing by simply *not updating* the
  counters on those nodes (a smaller change than skipping the coding) and
  reading the corpus total.

---

### 12. Per-file parameter profiles, chosen by the encoder
- **Idea:** `opt.pl` produces one parameter vector for a corpus that contains a
  168 kB music file and a 2.7 kB one; the counter rates (`rA`=255,
  `rB`=169, `rD`=249), the mixer rate `lr`, the warm-up boost `mb` and the APM
  blend `bw` are a compromise between "settle slowly, you have 700k digits" and
  "settle now, you have 350".  Tune K = 4 to 8 complete profiles on disjoint
  slices of the corpus (long/short, high/low bitrate, tonal/noisy), have the
  encoder try each and write the winner's index in three bits of the header.
- **Why it might work:** it converts a compromise into a choice, at three bits.
  The spread between the corpus's files is enormous — 0.955 bits a digit on
  `multi6-48k-q4` against 3.142 on `chirp-stereo-q10` — and the `.idx` comments
  already name the failure mode: "`zrun` ... measures at nothing on a 350
  kbit/s mono residue ... on a 96 kbit/s stereo one ... worth 3% of the file.
  These patterns are a joint answer over a corpus that has both."  A profile
  index lets the two answers coexist.  It is also the only idea here that helps
  *without* the model getting any smarter.
- **Expected gain:** 0.3–1% averaged over a heterogeneous corpus, 1–2% on the
  files that today lose most to the compromise (the clips, the multichannel
  file), near zero on whatever the current tuning already favours.
  Confidence: medium-high, and it is directly measurable with the existing
  tooling.
- **Cost:** encoder time × K (mitigate by choosing on the first 64 kB and
  committing), decoder time unchanged.  Implementation: the profiles must be
  runtime values in the *shipping* build, which is the opposite of the `Const
  1` design — the cleanest form is K folded constant blocks selected by a
  switch, which `idx2inc.pl` can emit.  Stream: yes, three bits per stream.
  Note the profile index must be per *link*, and `oggdet -S`'s solid mode
  shares one model across streams, so the choice is per archive there.
- **Risks:** K profiles is K times the tuning cost, and `opt.pl` already takes
  a full corpus encode per bit flip.  Also the profiles will overfit their
  slices harder than one profile overfits the whole corpus, so held-out
  measurement is mandatory.
- **How to measure it cheaply first:** no new code at all — build the tuning
  binary, run `opt.pl` separately on (a) the clip set, (b) the two music files,
  (c) `chirp-stereo-q10`, and compare each resulting binary's output on the
  *other* slices.  The gap between "tuned on your slice" and "tuned on the
  union" is exactly the prize, and it costs three tuning runs and no design.

---

### 13. Re-aim the optimizer
- **Idea:** four changes to `IDX/opt.pl` and its corpus, none of which touch
  the model.  (a) Objective: `$t += $s` sums bytes, so `music-stereo-q5`
  (143 kB) outvotes `tiny-8k-q0` (192 bytes) by 700:1; use the sum of
  `bytes/reference_bytes` or of `log(bytes)` so every file gets a vote.
  (b) Corpus: add the 1,257-clip set both chained *and* as `-c` (per-stream
  reset), so cold-start parameters are scored at all — today nothing in the
  corpus punishes a model that needs 40 packets to settle.  (c) Inner loop:
  evaluate on a fixed prefix of each file (say the first 200 packets) for the
  bit-flip sweeps and only confirm improvements on the full corpus; that is a
  5–10× iteration-rate increase on a search described as "near its floor",
  which may mean "out of time" rather than "out of gains".  (d) Held-out set:
  reserve four files, never optimised on, and report their total beside the
  objective; the 0.05% of the last profile is within overfitting distance of
  zero and nobody can currently tell.
- **Why it might work:** every other idea in this list is scored by this
  script, so its noise floor and its biases are everyone's.  §3 concludes "the
  remaining ratio is not in the thresholds" from a search whose objective is
  dominated by one file and whose iteration budget is one full corpus encode
  per bit — both of those are fixable without touching the model.
- **Expected gain:** 0.1–0.3% directly from a better-aimed search; much more
  indirectly, because it decides whether ideas 1–12 measure as real.
  Confidence: high that (a) and (d) change the answers, medium on the size.
- **Cost:** a day of perl.  No per-symbol cost, no stream change.
- **Risks:** (a) changes what "better" means, so the current tuned state is no
  longer the optimum of the new objective and the first re-run will look like a
  regression on the old number.  (c) risks prefix-tuning a model whose
  behaviour on a prefix differs (the tables are cold in a prefix, which is
  arguably a feature here).
- **How to measure it cheaply first:** just compute the alternative objectives
  on the existing `./t.sh -1 -n` table — no run needed — and see whether the
  current parameter set is even near the optimum of the re-weighted objective
  by flipping a dozen patterns by hand.

---

### 14. A packet-level match model
- **Idea:** `t1` and `t2` look exactly one and two packets back at the same
  slot.  Keep instead a ring of the last N (say 64) packets' digit vectors per
  residue and channel — `dg_hist` already holds two, and `oc_hist`'s pool is
  128 MB — and at the start of each packet pick the ring entry that best
  matches the *already coded* part of this packet (the floor posts, the class
  stream, and the first partitions), by L1 distance.  Expose that entry's value
  at the current slot as `m1`/`m1s`, alongside `t1`.
- **Why it might work:** music is periodic at scales the model cannot see —
  a 120 bpm bar is about 90 packets at 23 ms; a sustained note is dozens.  The
  class stream and the floor of the current packet are decoded before its
  residue, so the match can be chosen on genuinely current evidence with no
  side information.  `t1` is the degenerate N = 1 case of this and is one of
  the strongest variables in the model (`a_t1: 1!111`, four buckets, and
  `avT1` at full weight), which is the argument that a *better-chosen* previous
  packet is worth more.
- **Expected gain:** 0.3–1.5% on structured music, near zero on noise and on
  the clips (where §7.3 found 48,624 distinct of 49,314 packets, so no exact
  repeats — but this is a nearest-neighbour, not an exact match).  Confidence:
  low-medium; this is the most speculative idea I would still bet on.
- **Cost:** memory for N packets of residue history (a stereo long block is
  ~2 kB of `i16` per packet per channel, so 64 packets is ~0.5 MB — free);
  a match search once per packet per channel over 64 candidates on a coarse
  signature (say 32 band energies) — microseconds, negligible against 1,200
  instructions × 1,700 digits.  Two new `.idx` variables.  Stream: yes.
- **Risks:** the "best match" is chosen on evidence that is itself noisy, so
  the selected packet may be worse than the immediately previous one; guard by
  including `t1` as a candidate and letting the distance decide, and by giving
  the model the match *quality* as a separate context so it can discount a bad
  match.  Also the match must be computed identically on both sides from
  already-decoded data only — easy to get wrong.
- **How to measure it cheaply first:** entirely offline.  Dump every packet's
  residue vector for `music-stereo-q5`, and for each packet compute
  `H(digit | t1)` and `H(digit | m1)` where `m1` comes from the best of the
  previous 64 packets under a coarse signature.  If the best-of-64 is not
  materially better than the previous packet, the idea is dead for the price of
  a script.

---

## Wild cards

**Prime the model from the codebooks before coding a byte.**  After the setup
header is decoded, both sides know every codebook exactly.  Have both run a
deterministic synthetic warm-up: generate a pseudo-random digit stream from the
codebooks' own entry distributions (a fixed PRNG seeded from the setup's hash),
and run the model's *update* path over it — contexts, counters, mixer weights,
APMs — without coding anything.  No side information, no bytes in the stream,
and the tables arrive at the first real digit already holding the codebook's
marginals and a sensible set of mixer weights.  It is idea 1 and idea 9 taken
to their limit and it costs only CPU, symmetrically on both sides.  The obvious
objection is that a synthetic stream has none of the temporal correlation that
`t1`, `q1` and `ax` carry, so it would warm the marginals and mislead the
conditionals; the obvious answer is to warm only the contexts that do not
depend on history (the prior-keyed ones, `cls`, `band`, `bkq`) and leave the
rest cold, which is a strange and interesting engineering problem.

**Code the codebook entry, with a prior-balanced tree.**  `rs_sym` already has
the entry index `e = bk_get(bk)` in hand at the encoder before it splits it
into digits, so nothing prevents coding the entry as one value.  The current
0/1/2/gamma tree costs up to seven nodes for a magnitude-4 digit out of a
nine-symbol alphabet; a binary tree over the *entry's* value list shaped by the
trie node's own weights (a Huffman tree over the prior) would cost about
`H(prior)` nodes, and each node would sit near 50/50, which is where a mixer
and an APM have the most to do and a 12-bit probability the least to lose.  The
node identity would be "which prior quantile is being split" rather than "which
magnitude", so the counters would become a pure calibration model shared across
every book — warming in tens of decisions rather than hundreds.  Against it:
the per-digit contexts (`t1`, `q1`, `ax`) are defined on *values*, and a tree
whose splits are probability quantiles scrambles their meaning; the honest
experiment is to keep the value tree and add the calibration counters as an
extra mixer lane, which is idea 5(a) in disguise.

**Code under the packet's exact bit budget.**  A Vorbis packet's length is
known to the decoder (idea 3) and the codewords in it must sum to that length
up to at most seven bits of padding.  That is a hard constraint on the joint
distribution of all the digits in the packet, and a model that merely takes
"bits remaining" as a context uses a shadow of it.  The full version is coding
under a total-length constraint — at each step, restrict the alphabet to
symbols whose codeword length leaves a feasible remainder, and renormalise.
For the last few codewords of every packet this collapses to near-certainty,
and one could go further and code the packet's *values* in an order that puts
the most constrained ones last.  It is an exotic and error-prone thing to
implement (feasibility must be computed identically on both sides, over all
remaining partitions and passes, which depends on classes not yet decoded), and
the payoff is concentrated in a handful of values per packet — perhaps 0.2%.
But it is the only idea here that exploits an exact arithmetic identity rather
than a statistical tendency, and exact identities do not need tuning.

**Let the optimizer invent variables.**  `ax` is a hand-designed weighted
average of eight magnitudes whose weights `opt.pl` tunes; it is one of the most
used variables in the model.  Nothing in the design says there should be one of
them.  Add three more slots — `ax2`, `ax3`, `ax4` — each a tuned linear
combination over a *superset* of inputs (magnitudes, signs, the floor level,
the packet budget, the class run), all starting at weight zero, and let the
hill-climb decide what they become.  That is a crude form of learned feature
construction inside a search that today can only move thresholds, and it costs
24 multiply-adds per digit for three features the optimizer might actually use.
The risk is obvious — a 24-dimensional continuous search on top of a
combinatorial one, over a corpus that already flirts with overfitting — but it
is the only proposal here that could find a variable nobody thought of.

**Spend the 1,200 instructions where the bits are.**  Every binary decision in
the digit family gets the identical pipeline: four counters, two APMs, a seven-
input mixer.  But the zero node carries 37–59% of the digit bytes on low-rate
material, while at `-q 10` only 14% of digits are zero and the bits sit in the
length and mantissa nodes — which get a three-input mixer and no APM.  A model that
allocated machinery by measured bit-share — twelve mixer inputs, two layers, an
indirect model and three APMs on the zero node; the present pipeline on the
head tail; something *cheaper* than today on nodes whose prior is already near
certain — would probably cost less total CPU than the flat design and compress
better.  The architecture makes this awkward (one `tc_fam` with one `TC_MIXN`
serves every node), which is precisely why it has not been tried.
