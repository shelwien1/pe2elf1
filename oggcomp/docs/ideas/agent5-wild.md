# agent5-wild: ratio ideas for oggcomp

Grounding read: `README.md`, `IMPROVEMENTS.md` §1-4, §7, `OPTIVORBIS.md` §3-4,
`IDX/tsvcomp-dig.idx`, `oc_residue.inc`, `tc_fam.inc`, `tc_ptab.inc`,
`oc_floor.inc`, `vb_packet.inc` (`rs_sym`/`rs_part`/`residue`), `oc_frame.inc`.

Three facts from that read that most of these ideas hang on, and that are not
spelled out in the documents:

* **The digit model knows nothing about the floor.** The thirty inputs in
  `digit()` include the partition index `band` and the class `cls`, but no
  value of the floor curve, which is fully decoded (`fl_fy[]`) before any
  residue digit of the packet is coded. 2.4-6.8% of the output is spent on a
  spectral envelope the residue model never looks at.
* **`p.plen[i]`, the exact byte length of every packet on the page, is coded in
  `oc_model::page()` *before* the packets** (`ogg_stream.inc:330` then `:343`),
  and `io_t::pos`/`len` in `vb_packet.inc` make the remaining bit count exactly
  symmetric between encoder and decoder at every codeword boundary. Nothing
  reads it.
* **A context only has to be deterministic on both sides, not
  specification-exact.** Anything computable from already-decoded state --- a
  rendered floor curve, a reconstructed coefficient, a predicted magnitude ---
  is free to be approximate, integer, and cheap. That removes most of the risk
  from the "reconstruct the spectrum" family below.

Reference points used for the gain estimates: music-stereo-q5 is 143,003 bytes
out, digits 94.8% at 1.558 b/digit over 695,824 digits, floor 2.42%, class
2.35%; chirp-stereo-q10 is digits 3.14 b/digit and floor 6.76%;
music-managed-b96 is digits 1.099 b/digit; the zero node alone is 37-59% of the
digit bytes and signs about 28% (≈38 kB of the 143 kB music file) at close to
one bit each.

---

### 1. Predict in the coefficient domain, code in the digit domain

- **Idea:** Keep, per residue and per bin, a reconstruction of the decoded
  spectral magnitude: `X[k] = floorcurve[k] * residue[k]`, where `floorcurve`
  is the piecewise-linear floor1 curve rendered from `fl_fy[]` (a cheap integer
  interpolation over the sorted posts, once per channel per packet). Track a
  two-tap predictor per bin in the log domain --- an EWMA of `log|X[k]|` plus a
  trend --- and at coding time form `pm[k] = predicted|X[k]| /
  floorcurve_now[k]`, the *residue magnitude this packet should have if the
  spectrum is stationary*. Give `digit()` two new variables: `tc_qlog(pm)` and
  the ratio `tc_qlog(t1 * 256 / pm)`. This is `IMPROVEMENTS.md` 4.2 taken all
  the way: 4.2 corrects `t1` by the floor's change, this replaces `t1` with a
  proper predictor in the physical domain and keeps the floor's change as a
  special case.
- **Why it might work:** `t1` --- the same slot one packet ago --- is the
  strongest temporal input the model has, but it lives in a domain the encoder
  renormalises every packet. A sustained note under a swelling envelope gives a
  *constant* `X[k]` and a *changing* residue, so `t1` is systematically wrong
  by exactly the floor's motion, and the model can only average that away. The
  same trick is what MPEG-4 SLS and every scalable-to-lossless audio coder do
  (predict the coefficient, code the difference against a scale), and what
  lepton does when it predicts a DCT coefficient from the neighbouring block's
  *dequantised* value rather than its raw level. Block-size switching is
  handled by keeping separate predictors per `hist.tc_blk`, as `dg_hist`
  already is.
- **Expected gain:** 1-3% of output on tonal music, 0.5-1.5% on mixed music,
  near zero on noise; largest on `sine-stereo-q5`, `chirp-stereo-q10`,
  `sweep-mono-qm1`. Confidence: medium-high that it beats plain `t1`,
  medium that it reaches 1%.
- **Cost:** Moderate. One floor-curve render per channel per packet (O(n/2)
  integer ops, negligible against 30 digits × ~1,200 instructions); two
  `i16`/`u16` arrays the size of `dg_hist`; two more inputs to `tc_make_dig`,
  so ~40 more instructions per digit. No stream-format change beyond `OC_VER`
  and a tuning pass.
- **Risks:** The residue is already a whitened signal, so the headroom may be
  small; the floor is *deliberately* fitted so that the residue is near unit
  variance. `dg_avg` (the decayed per-slot mean) may already be capturing most
  of what a predictor would. Slot-to-bin mapping must be right for type 0 and
  type 2 or the predictor is indexed at the wrong frequency.
- **How to measure it cheaply first:** Log, per digit, `(slot, bin, blk, t1,
  t2, floor_now, floor_prev, digit)` from a release build with a throwaway
  `fprintf`, on music-stereo-q5 and sine-stereo-q5. Offline, fit the geometric
  scale of `|digit|` given `tc_qlog(t1)` alone versus given
  `tc_qlog(t1*floor_prev/floor_now)` versus given the full EWMA predictor, and
  compare the code lengths of the zero node and the magnitude. If the
  floor-corrected version does not beat plain `t1` by 3% of the digit bits in
  that crude estimate, it will not survive `opt.pl`.

### 2. A per-partition hyperprior: one small network per ~30 digits, not per bit

- **Idea:** `IMPROVEMENTS.md` 4.7 rules out a neural mixer because it costs
  10-100× the instructions *per binary decision*. Move the network to the
  partition: a partition is ~30 digits × ~1,200 instructions ≈ 36,000
  instructions, so a 32→16→8 MLP (about 600 multiply-adds, all int16) evaluated
  once in `part_begin` costs under 2% of the partition. Feed it the things that
  are constant over a partition and hard to quantise by hand --- the floor's
  shape across the partition, the class and its two neighbours, the previous
  packet's digit statistics for this partition (count of nonzeros, mean
  magnitude, max), the packet's byte length, the pass, the book's range --- and
  have it emit 4-8 numbers that enter every digit of the partition as extra
  mixer inputs (stretched) and as one or two quantised context axes. Train the
  weights offline by gradient descent on the actual coding loss over a corpus
  and ship them as a table, exactly as `vbooks_gen.inc` ships the codebooks.
- **Why it might work:** This is the hyperprior of learned image compression
  (Ballé/Minnen): a cheap, coarse, per-block latent that conditions a fine
  autoregressive model, which is worth 5-10% there. It is also what `ax` is ---
  a hand-weighted sum of eight neighbours with eight tuned coefficients
  (`avA`..`avX1`) --- generalised from a linear form with 8 hand-chosen inputs
  to a nonlinear form with 32 and learned weights. The `.idx` mechanism can
  only express products of marginal quantisers; a network expresses
  interactions, which is where a plateaued hill-climb usually leaves value.
- **Expected gain:** 0.5-2% of output, roughly uniform across material,
  possibly more on low-bitrate files where the per-partition density varies
  most. Confidence: medium; this is the highest-variance item here.
- **Cost:** Large implementation effort (offline training harness, a fixed-point
  forward pass that both builds reproduce bit-exactly, a weight table in
  `MOD/`). Per-symbol CPU near zero; ~50 kB of weights; `OC_VER` bump. The
  fixed-point determinism requirement is the real work.
- **Risks:** Fixed-point reproducibility across compilers is exactly what
  `./mk.sh check` exists to police, and a network is the worst case for it.
  Overfitting a corpus of 31 files. And the honest risk that a second mixer
  (4.4) plus two or three of the hand-built variables below gets the same
  0.5% for a tenth of the work.
- **How to measure it cheaply first:** Skip the coder entirely. Dump a
  per-partition feature vector and the partition's *actual* digit entropy
  (which the verbose build already computes per stage --- add a per-partition
  accumulator). Train a small regressor offline in Python to predict the
  partition's bits per digit from the features, and compare its residual
  variance with that of the best single hand-quantised predictor. If the
  network explains less than ~15% more of the variance, do not build it.

### 3. Hoist the class map, then give every digit a two-sided spectral context

- **Idea:** In `vb_packet.inc:residue()` the classwords are coded `pv`
  partitions at a time, interleaved with the digits of those partitions. Change
  the *coded* order (not the stream) so that every pass-0 classword of the
  residue is coded first, for all partitions and all channels, before any
  digit. Then every digit can see: the class of the partition below and of the
  partition above, the index of the highest non-silent partition (the packet's
  bandwidth), the count of each class in this packet, and --- for free --- the
  exact number of residue digits left in the packet.
- **Why it might work:** The `.idx` comment says `cls` is the single biggest
  factor and that four buckets of it are worth 4.65%; the class is the
  encoder's own quantised statement of a partition's energy. Today a digit sees
  only its own partition's class and a run length (`crun`, `cm`), which are all
  at pattern zero or nearly so in every index --- a one-sided, time-only view
  of a quantity whose *frequency* neighbours are known and are the informative
  ones. In JPEG terms this is coding the whole block's significance structure
  before its coefficients; in H.264/HEVC terms it is `coded_block_flag` for the
  neighbouring blocks being available before the residual scan. It also makes
  the class stage itself two-sided (a class can be coded knowing the classes of
  the partitions on both sides in the previous packet and below it in this
  one), and the class stage is 1-2.4% of output at 0.08-1.26 bits a value.
- **Expected gain:** 0.5-1.5% of output on music (through the digits), plus
  5-15% of the class stage itself (≈0.1-0.3%). On `music-managed-b96`, where
  two digits in three are zero and the bandwidth moves per packet, the upper
  end. Confidence: medium-high; the mechanism is the same one that makes `cls`
  the strongest variable already.
- **Cost:** Moderate. The encoder must read all pass-0 classwords from the
  packet before the digits (a sequential read it can already do); the decoder
  must buffer the classwords and `bk_put` them in specification order at the
  right points. One array of `u8` classes per residue per packet. Four or five
  new inputs to `tc_make_dig`. Stream-format change: yes (coded order moves),
  `OC_VER` bump.
- **Risks:** The classword is a VQ over `pv` partitions so the grouping is
  already coarse; the neighbour class may be nearly determined by the current
  one, in which case it adds nothing. Buffering interacts with the
  "packet ends inside a field" tolerance in `OPTIVORBIS.md` 3.3 if that is ever
  implemented.
- **How to measure it cheaply first:** No code change needed in the coder ---
  log `(rno, pass, pc, j, cls)` and the per-digit records, then offline compare
  a crude adaptive estimate of the zero node conditioned on `(cls, pq bucket,
  zrun)` against the same plus `(cls_below, cls_above, bandwidth)`. 7.4's
  methodology exactly.

### 4. libvorbis's noise normalisation leaves a countable footprint

- **Idea:** `_vp_noise_normalize()` in libvorbis's `psy.c` does not quantise the
  sub-threshold coefficients independently. Within each aligned block of
  `normal_partition` coefficients (16 or 32, above `normal_start`), it keeps the
  largest values and then spends the block's remaining energy by setting a
  *computed number* of the next-largest positions to ±1 and the rest to 0.
  So the count of `|value| == 1` in an aligned block is a smooth function of
  that block's energy, and the positions are the largest residuals. Add three
  context variables: the running count of `|digit| == 1` so far in the current
  aligned block, the running count of nonzeros, and the position within the
  block --- all in the *coefficient* domain (slot → bin, so correct for type 2
  interleave and type 0 scatter).
- **Why it might work:** This is the packMP3 move: exploit a deterministic
  quirk of the encoder that the format does not require. It predicts precisely
  the structure 7.4 saw and could not name --- "odd lengths dominate on the
  managed file, a zero vector of dimension two ending a run at an even
  boundary" --- and it explains why run lengths lost to per-digit contexts: the
  structure is a *count within a fixed grid*, not a run. On a low-bitrate file
  most nonzero digits are ±1, so this speaks directly to 66% of the digits of
  `music-managed-b96` and 60.7% of the clip set.
- **Expected gain:** 0.5-1.5% of output on low-bitrate and bitrate-managed
  material and on the 1,257-clip set; 0.2-0.6% on `-q 5` music; near zero at
  `-q 10`, where the normalisation region is small. Confidence: medium --- the
  effect is certainly there, its size is not known.
- **Cost:** Small. Three counters reset on a bin-aligned boundary, three inputs
  to `tc_make_dig`. The block size (16 or 32) is not in the stream; try both,
  or add a two-valued parameter the tuner picks, or detect it from the observed
  period of the ±1 pattern. No stream-format change.
- **Risks:** `normal_partition` and `normal_start` vary by psy template and
  quality, and aoTuV changes them; a wrong alignment turns the variable into
  noise (which is cheap --- the pattern goes to zero and the variable costs
  nothing, as the `.idx` header says). The mapping from the digit's coding-order
  slot to a coefficient bin has to be exactly right.
- **How to measure it cheaply first:** From the logged digit stream, compute
  the autocorrelation of the indicator `|digit| == 1` over lags 1..64 within a
  packet, per file. If the encoder grid exists it will show as a periodicity at
  16 or 32. Then measure the conditional entropy of `|digit| == 1` given
  `(position mod 32, count of ±1 so far in this block)` against the
  unconditional one.

### 5. Signs: four bits of sign history per bin is an MDCT phase tracker

- **Idea:** Signs are ~28% of the digit bits and nearly random, ≈38 kB of the
  143 kB music file. `fam_sgn` today sees `t1s`, `p0s`, `q1s` and the magnitude.
  Add a packed *history* variable: the signs of this slot in the last three or
  four packets as a 3- or 4-bit code (a new `u8` array beside `dg_hist`), gated
  by whether those packets had a large magnitude there; plus the sign of the
  adjacent *bin* in this packet (not `q1s`, which for an interleaved residue is
  the other channel), and the sign of the largest neighbour in `ax`'s window.
- **Why it might work:** An MDCT coefficient of a steady sinusoid does not have
  a random sign: the frame-to-frame phase advance is `2*pi*f*H/fs`, so the sign
  at a peak bin follows a periodic pattern whose period is set by the partial's
  offset from the bin centre --- `+ + - -`, `+ - + -`, and so on. The single
  previous sign (`t1s`) cannot see a period-4 rotation; four bits of history
  can, and it costs one table lookup. 4.5 proposes the pair `(t1s, t2s)`; the
  packed 4-bit form is strictly more and no dearer. The adjacent-bin sign is
  the same thing across frequency: the MDCT mainlobe of a partial has a fixed
  sign relation between the two bins straddling it, which is why lepton
  predicts DCT signs from neighbours at all.
- **Expected gain:** 0.3-1.2% of output on music (a 3-5% cut on a quarter of
  the sign bits), several percent on `sine-stereo-q5`, `chirp-stereo-q10` and
  `sweep-mono-qm1`. Confidence: medium-high that it is positive, medium on the
  size.
- **Cost:** Small. One `u8` per history slot (one eighth of `dg_hist`'s
  footprint), two inputs to `tc_make_sgn`. No stream-format change. `sgn_p.inc`
  is already 24% of instructions jointly with `dig_p.inc`, so keep the new axes
  narrow.
- **Risks:** Coupled stereo means the "magnitude" channel carries a spectrum
  sign and the "angle" channel carries something else, so the physics only
  applies to one of the two interleaved channels --- `chn` must be in the sign
  index for it to separate them (today `s_chn: 1!1000000`, so it is, barely).
  Partials that drift in frequency scramble the period within a few frames.
- **How to measure it cheaply first:** Log `(slot, blk, sign, |digit|)`, and
  offline compute the conditional entropy of the sign given the packed 4-bit
  sign history, restricted to slots where `|t1| >= 2`. Anything below 0.97 bits
  there is worth building.

### 6. Replace the 0/1/2/gamma tree with a per-book tree shaped by the prior

- **Idea:** `tc_fam::code()` walks every value as three unary head nodes, a
  unary bit-length, and mantissa bits. But a residue digit is not a general
  magnitude --- it is one of `bk->nv` lattice multiplicands (`bk->mult[]`),
  typically 2 to 33 of them, and the prior at the current trie node
  (`tc_ptab::pw`) already gives their exact weights. Build, once per book per
  trie level (or once per book, from the level-0 weights), a binary tree over
  the `nv` symbols whose splits are as close to 50/50 under the prior as
  possible, and walk that instead. Node ids become tree positions; the counters
  and mixer keep their existing contexts.
- **Why it might work:** For a 9-multiplicand book a digit of magnitude 4 costs
  seven binary decisions today (three heads, two length nodes, a mantissa bit,
  a sign) to carry about two bits of information; a balanced tree costs 3.2.
  Fewer nodes means each node's counter rows carry more counts and adapt
  faster, and splits near p = 0.5 are where the mixer and the APMs have
  leverage --- a chain of near-certain nodes is where a CM model leaks, because
  each one pays its own small modelling error. This is the "code the codeword,
  not the digits" question in its tractable form: the prefix trie is kept
  (so the prior still conditions on the codeword prefix) and only the
  *within-symbol* decomposition changes. At `-q 10`, digits cost 3.14 bits
  each and most have a mantissa, so this is where the waste is.
- **Expected gain:** 0.3-1% of output at `-q 5`, 1-2% at `-q 10` and on
  `chirp-stereo-q10`; plus a large speed win (the decision count per digit
  falls by a third to a half, and `IMPROVEMENTS.md` 3 says the model is the
  time). Confidence: high on speed, medium on ratio.
- **Cost:** Moderate. A per-book tree table in the `tcp_arena` (tiny: `nv`
  nodes), a rewrite of `code()` for the digit family only, and the node
  numbering that `tc_node_len`/`tc_node_man` encode. Stream-format change: yes.
- **Risks:** The tree shape depends on the prior, which depends on the book, so
  two books with the same context index now disagree about what node *k* means
  --- the counter rows become book-dependent and dilute. Mitigation: key the
  tree by depth and branch direction rather than by symbol, or include `bkq` in
  every index (it is already in `a`, `b`, `m`, `s`). Books that miss the prior
  (`pt == nullptr`) need the old path kept.
- **How to measure it cheaply first:** Instrument `code()` to count decisions
  per digit and sum `-log p` per node depth, per file. If the head and length
  nodes together carry less than ~0.15 bits each on average at `-q 10`, they
  are pure overhead and the tree will pay.

### 7. The class was an escape: a running-max constraint per partition

- **Idea:** libvorbis picks a partition's class by walking its candidate books
  in order and taking the first whose range covers every value in the
  partition (`_01class` in `res0.c`). So class *c* implies not only "every
  digit fits book *c*" --- which `bkq` already carries --- but "some digit
  does **not** fit book *c-1*". Both books are in the stream
  (`r->book[c][pass]`, `r->book[c-1][pass]`), so both ranges are derivable. Add
  two variables: `esc` --- has the running max `|digit|` in this partition
  already exceeded the previous class's book range (0/1) --- and
  `left` --- how many digits of the partition remain, quantised.
- **Why it might work:** When `esc == 0` and only three digits remain, one of
  them almost certainly must be large, and the model's zero probability for
  them should collapse. This is exactly CABAC's `last_significant_coeff`
  logic and JPEG's EOB inverted: a hard-ish constraint discovered late in a
  block that the coder should have been carrying all along. It costs nothing to
  compute (a max and a counter in `P`), needs no encoder tables, and is
  invisible to `zrun`, `crun` and `cm`, which are all about *history*, not about
  the partition's own unmet obligation.
- **Expected gain:** 0.3-1% of output. Concentrated in the last third of each
  partition, which is ~30% of digits; if it halves the surprise there on the 10%
  of partitions where the constraint bites, that is ~0.5%. Confidence: medium;
  the rule is real but the book ordering by range is a convention, not a
  requirement.
- **Cost:** Small. Two `u32` in `oc_part`, one comparison per digit, two
  inputs to `tc_make_dig`. No stream-format change.
- **Risks:** Classes need not be ordered by book range, and cascade passes
  complicate the rule (a later pass can carry the overflow instead). aoTuV
  changes `_01class`'s metrics. If the ordering does not hold for a stream, the
  variable is noise and the tuner will zero it, which is cheap.
- **How to measure it cheaply first:** Purely offline from the logged digit
  stream plus the setup: for each partition, record the class, the previous
  class's book range, and the partition's max `|digit|`. If the max exceeds the
  previous class's range in well over half the partitions, the rule holds and
  the context will pay.

### 8. The floor curve as a per-digit context

- **Idea:** Render the floor1 curve from `fl_fy[]` once per channel per packet
  (deterministic integer interpolation; it need not match libvorbis bit for
  bit) and give `digit()` four things it has never had: the floor's value at
  *this digit's own bin* minus the partition's mean floor; the floor's second
  difference across the two posts bracketing this bin (its curvature); the
  distance from this bin to the nearest floor post, normalised by the post
  spacing; and the floor's value relative to the packet's maximum.
- **Why it might work:** Floor1 is a piecewise-linear fit in a *coarsely
  quantised* dB domain (`QUANT[mult-1]` is 256, 128, 86 or 64 levels over the
  whole range) interpolated between posts that can be a hundred bins apart. The
  residue has to absorb that approximation error, and the error is a
  deterministic function of the floor's geometry: largest midway between posts,
  largest where the curve is most convex, zero at a post. That is a systematic,
  *known* bias in the residue's local scale that no current input can see. It
  is the analogue of packMP3's use of the scalefactors to condition the
  big_values region, and of JPEG recompressors conditioning on the quantisation
  table rather than only on neighbouring coefficients. At `-q 10` the floor is
  6.76% of output and 3.23 bits a post, so it is carrying a lot of shape that
  the residue model ignores.
- **Expected gain:** 0.3-1% of output on music, more where the posts are sparse
  relative to the bandwidth (low sample rates, `mono-22k-q4`, `tiny-8k-q0`).
  Confidence: medium.
- **Cost:** Small-to-moderate. A curve render per channel per packet (O(n/2)
  integer ops, ~1% of a packet's model time); three or four inputs to
  `tc_make_dig`; one `i16` array per channel. No stream-format change.
- **Risks:** The residue is by construction whitened by the floor, so the
  headroom may already be spent. The curvature term only exists where posts are
  sparse. Slot→bin mapping again.
- **How to measure it cheaply first:** Log, per digit, the bin, the floor value
  at the bin, the partition's mean floor, and the digit. Offline, bucket by
  (floor-at-bin minus partition mean) and check whether `P(digit == 0)` and
  `E|digit|` actually move across those buckets. If the curves are flat, the
  floor really is doing its job and this idea is dead in one afternoon.

### 9. A learned context quantiser instead of a mixed-radix product

- **Idea:** Every index in `tsvcomp-dig.idx` is a product of per-variable
  threshold patterns --- a rectangular partition of the 30-dimensional input
  space, hill-climbed one pattern at a time. Replace *one* of the counter
  tables (say a new fifth table `E`) with a row index produced by a
  **decision tree over the raw inputs**, trained offline on a corpus with an
  entropy-reduction criterion (CART on the binary outcome of each node, depth
  12-16, 2^14 leaves), shipped as a table of (variable, threshold, children)
  in `MOD/`. Ten to sixteen compares per digit selects the row.
- **Why it might work:** `IMPROVEMENTS.md` 3 says the parameter search is at
  its floor --- 236 pattern moves for 0.05% --- and concludes the remaining
  ratio is in *information the model does not have*. There is a third
  possibility: the information is there but the *shape* of the partition cannot
  express it. A product quantiser cannot represent "`t1` matters only when
  `cls` is 3 and `pq` is high", which is exactly the kind of interaction the
  `.idx` comments keep bumping into ("`cls` beats `band`", "`bkq` measures 1%
  worse than the class it was meant to replace", "`zrun` is worth 3% on one
  file and nothing on another"). Context quantisation by a trained tree is
  standard in CALIC/JPEG-LS-descended coders and in video codec context
  derivation; it is also how a gradient-boosted model would be deployed at this
  instruction budget.
- **Expected gain:** 0.3-1% of output, and --- more valuable --- it gives the
  plateaued tuner somewhere new to go. Confidence: medium.
- **Cost:** Moderate-to-large: an offline training pipeline over logged
  per-digit records, a tree table both builds share (and `./mk.sh check` must
  hold constant), one more counter and one more mixer input. Per-digit CPU:
  ~15 branches, cheap next to the 610 instructions `tc_make_dig` already
  spends. `OC_VER` bump.
- **Risks:** Overfitting 31 files. Branch misprediction on a data-dependent
  tree walk could cost more cycles than the estimate. The tuner and the tree
  will fight over the same information; the tree has to be trained against the
  *residual* of the existing model, not from scratch.
- **How to measure it cheaply first:** Log per-digit records with all thirty
  inputs and the outcome of the zero node only. Train a tree offline on half
  the corpus, evaluate its conditional entropy on the other half, and compare
  with the entropy of the same outcome under the existing index `a`. A tree
  that does not beat index `a` by 5% out-of-sample is not worth wiring.

### 10. An adaptive second codebook prior beside the static one

- **Idea:** `tc_ptab` builds its trie weights from the book's Huffman *lengths*
  --- a fixed statement, made by libvorbis's designers over their training
  corpus, about what an entry is worth. Add a parallel adaptive count array
  over the same trie nodes, updated with each coded digit, producing a second
  prior `prp2` (an eighth mixer input) and a second context axis `pq2`. Weight
  it in by count, and optionally split the counts by a coarse key (block size,
  or pass) so it tracks *local* drift.
- **Why it might work:** The static prior is the model's single most Vorbis-
  specific input and enters the pipeline twice, but it is the *designers'*
  distribution, not this file's --- a book trained on classical will sit in a
  techno stream and misprice every entry by a constant factor. The adaptive
  count fixes the file-level mismatch; splitting by block size fixes the
  transient/steady mismatch that no single set of Huffman lengths can express.
- **Expected gain:** 0.2-0.6% of output. Confidence: medium-low, and here is
  the reason for the caution: `OPTIVORBIS.md` §3/`IMPROVEMENTS.md` 7.2 measured
  that rewriting the book lengths to match the file's own counts moved the
  digits stage by only 0.08%. That is close to an upper bound for the *global*
  version of this idea. The case for it rests entirely on (a) integer Huffman
  lengths quantising the prior coarsely at the tail, and (b) locality, which
  OptiVorbis's single global rewrite cannot capture.
- **Cost:** Small-to-moderate. A `u16` count per trie node and symbol, the same
  shape as `tc_ptab::pw` --- within the 64 MB `TCP_MEMMAX` budget if the
  adaptive table is restricted to levels 0 and 1. One more mixer input, one more
  APM/context axis. No stream-format change beyond `OC_VER`.
- **Risks:** The 0.08% measurement above. Doubling the prior tables costs cache,
  and `tc_ptab.inc` is already 3-4% of instructions.
- **How to measure it cheaply first:** Offline, from the logged `(book,
  trie node, symbol)` stream: compare the code length of the symbols under the
  static weights against an adaptive count model with the static weights as the
  initial state, per file, and again with counts split by block size. The
  split-by-blocksize number is the one that decides it.

### 11. A match model over partitions: loops, silence, repeated phrases

- **Idea:** Hash the digit vector of each coded partition (say the first eight
  digits plus the class and band) into a table of positions in the digit
  history. When a hash hits, follow the matched position forward and feed the
  model, at each digit, the *predicted* digit from the match and a match-length
  confidence --- as a mixer input (stretched log-odds that this digit equals the
  prediction) and as a context axis. The standard PAQ/LZP match model, which
  oggcomp has nothing resembling.
- **Why it might work:** `IMPROVEMENTS.md` 7.3 establishes that the 1,257-clip
  corpus has no repeated packets, and the corpus has none either --- but the
  corpus is 31 synthetic and short files. Real material repeats: a game-audio
  loop repeats *exactly* (same PCM through the same encoder gives the same
  packets), a drum machine repeats bars, an electronic track repeats an
  eight-bar phrase with the same MDCT down to the quantised residue, a podcast
  repeats its intro, and a chained file repeats links. Where a repeat is exact
  the match model takes the digits to near zero bits; where it is approximate
  (same phrase, different mix) it still predicts the zero pattern. The model has
  no other way to reach beyond `t1`/`t2` --- two packets, ~46 ms.
- **Expected gain:** 0.0% on the present corpus and on through-composed music;
  2-20% on loop-based and game audio; a few tenths of a percent on ordinary
  pop with repeated sections. Confidence: high that the gain is bimodal,
  low on the size for any given file. It is the only idea here whose value is
  invisible to the current test set, which is itself the argument for getting a
  corpus that has loops in it.
- **Cost:** Small. One 2^22-entry hash table of `u32` positions, one hash per
  partition (not per digit), one pointer walk per digit, two inputs. Memory a
  few MB against 366 MB of tables. No stream-format change.
- **Risks:** The corpus cannot show it, so `opt.pl` will tune the new variable
  to zero and it will look dead. It needs its own witness file --- a loop
  concatenated to itself, and a real game-audio or electronic track --- added
  to `testfiles/gen.sh` before it can be judged.
- **How to measure it cheaply first:** No coder change at all. Take a real
  loop-based track and a pop track, log the digit stream, and measure the
  fraction of partitions whose first eight digits recur anywhere earlier, and
  the mean continuation length of those matches. If the recurrence rate is
  under 1%, drop it; if it is 10%, build it.

### 12. Feed the model its own recent surprise

- **Idea:** Maintain a running mean of `-log2 p` over the last N binary
  decisions (N ≈ 16 and N ≈ 256, two rates), which both sides compute
  identically from the probability the coder was handed and the bit that came
  out. Quantise into six or eight buckets and use it (a) as a context axis in
  index `a` and `m`, and (b) as the key of a third APM stage on the mixed
  probability.
- **Why it might work:** This is a pure error-feedback loop, and nothing in the
  thirty inputs is a statement about the model's *own state*. When the model has
  been doing badly for the last few dozen decisions --- a transient, an
  instrument entering, a scene change --- its estimates should be pulled toward
  the marginal; when it has been doing well they should be sharpened. That is a
  systematic, context-independent correction that an APM keyed on surprise can
  learn and that no per-variable quantiser can express. Related tricks appear in
  PAQ (the "`APM` on the mixer output keyed by recent match state") and in
  adaptive Rice-parameter selection in FLAC/TTA, which is the same feedback in a
  simpler coder.
- **Expected gain:** 0.2-0.6% of output, most of it at transients, so more on
  percussive and bitrate-managed material. Confidence: medium.
- **Cost:** Very small. Two running accumulators updated in `tc_bit` (a table
  lookup for `log2 p` from the 12-bit probability), one or two context axes, one
  more APM. No stream-format change beyond `OC_VER`. Note that `tc_bit` already
  computes a logarithm under `tc_verbose`; here it must be an integer table so
  both builds agree.
- **Risks:** Self-referential contexts can lock in: a bad estimate makes the
  surprise high, which makes the estimate hedged, which keeps the surprise high.
  Adding a third APM to a pipeline that is already two APMs and a mixer may just
  redistribute the same correction.
- **How to measure it cheaply first:** Log `(p, bit)` for the zero node only,
  per file. Offline, bucket by the running mean surprise of the previous 16
  decisions and check whether the *realised* frequency differs from `p` in a
  monotone way across the buckets. A flat calibration curve means there is
  nothing to correct.

### 13. Trained initial state, keyed by the codebook-table row

- **Idea:** `IMPROVEMENTS.md` 7.3 measures the cold start at 240 kB --- 4% of
  the clip set's output --- for a model that begins every 5 kB stream at zero.
  `vb_dict.inc` already identifies the encoder and its mode exactly: 350 rows
  of `vbooks_gen.inc` cover every corpus file, and the row number is decided
  before a single digit is coded. So ship a *trained initial state per row*:
  run the tuner over a corpus of files that use that row, dump the most-visited
  few thousand rows of counter table `A` and the mixer weights, compress them,
  and load them as the model's starting point when the setup matches that row.
- **Why it might work:** It is the codebook-table trick applied to the model
  rather than to the bits: the observation that made 7.1 worth 5.5% of the
  corpus was that libvorbis writes one of a few hundred fixed setups, and that
  observation is just as true of the *statistics* those setups produce. A
  `-q 4` 44.1 kHz coupled stereo stream from libvorbis 1.3.7 has the same
  residue statistics whoever recorded it. It also generalises `oggdet -S`'s win
  (431 kB) to a single file with no container around it.
- **Expected gain:** 2-4% on the 1,257-clip set and on any short file (which is
  where `-c` loses 431 kB to `-S`); 0.1-0.3% on a 160 kB music file, falling to
  nothing on a long one. Confidence: high on the mechanism (7.3 measured the
  loss), medium on how much of it a static prior recovers.
- **Cost:** Moderate. An offline dump-and-compress step, a table in the binary
  (target: a few hundred kB per row is far too much for 350 rows, so it must be
  a *shared* coarse prior plus a small per-row delta, or priors only for the
  dozen most common rows), and a load step at setup time. `OC_VER` bump; the
  priors become part of what `./mk.sh check` holds constant, as the codebook
  table already is.
- **Risks:** Binary size. Rows the corpus has no training data for. A prior
  that is wrong for an unusual file is worse than zero, so the counters must be
  initialised with a *low* count so the file overrides them within a few
  hundred decisions.
- **How to measure it cheaply first:** Take the clip set, code it solid
  (`oggdet -S`), snapshot the counter tables after the first 100 clips, and
  re-code the remaining 1,157 clips each independently starting from that
  snapshot. Compare against `-c`. That is the whole idea, measured with no new
  format and a debug hook.

### 14. Spend the packet's bit budget, since it has already been paid for

- **Idea:** `oc_model::page()` codes every `p.plen[i]` before the packets of
  that page, and `io_t::pos` is exactly symmetric between encoder and decoder at
  each codeword boundary. So at every partition, both sides know
  `bits_left = plen*8 - pos` and, once the classes are known (idea 3),
  `digits_left`. Give `digit()` the quantised ratio `bits_left / digits_left`
  and, at the tail of a packet, let `bits_left` bound the codeword: entries
  whose `len[e]` exceeds `bits_left + 7` are impossible and their prior weight
  should be redistributed.
- **Why it might work:** This is MP3's `part2_3_length`, which packMP3 uses for
  exactly this. The packet length is a direct statement of the packet's density
  --- on `music-stereo-q5` the packet length has mean 398 bytes and standard
  deviation 138, a 35% swing the model currently has to rediscover from context
  every packet. And the tail bound is a real exclusion, the same kind that makes
  the last coefficients of a JPEG block cheap.
- **Expected gain:** Small, and it is worth stating the ceiling honestly: the
  information a packet length can give about its payload is at most
  `H(plen | history)`, which measures 2.38 bits per packet on
  `music-stereo-q5`, 2.35 on `music-managed-b96`, 3.43 on `chirp-stereo-q10`.
  That is 0.08-0.2% of output on music. On the clip set, where packets are tiny
  and the lacing is 36 kB of 41.9 kB of page bytes, it is up to 0.66%.
  Confidence: high on the ceiling, medium that half of it is reachable.
- **Cost:** Very small: one subtraction per partition, one running digit
  counter, one input. No stream-format change.
- **Risks:** **It is mutually exclusive with `IMPROVEMENTS.md` 7.3's plan** to
  code the payload first and *derive* the lacing, which harvests the same mutual
  information from the other end and was estimated at the same 0.3%/0.66%. The
  two must be compared, not both implemented: this one is far cheaper (no
  read-ahead, no reordering of the container stage), that one is worth slightly
  more because it also removes the 0-7 padding bits' worth of slack. Also, the
  padding bits mean the total is known only to within 7 bits, which blunts the
  tail bound.
- **How to measure it cheaply first:** Already half-done above: `H(plen |
  prev)` per file is a ten-line script and bounds the whole idea. Then, from the
  logged digit stream, check whether the realised bits per digit of a packet
  correlates with `plen / digits` --- a scatter plot and an R² settles whether
  the density signal is real or already captured by `cls`.

---

## Wild cards

**Rewrite `_01class` in reverse and code the classes last.** libvorbis chooses a
partition's class as a deterministic function of the partition's own values
(`_01class` walks candidate books and takes the first that fits). If that
function were reproduced exactly --- and the 350 known encoder rows mean its
parameters are a finite, shippable table, just as the codebooks were --- then
the class would be *derivable* from the digits rather than coded, and the class
stage (1-2.4% of output, 8-21 k values per file) would go to near zero. The
obstacle is circularity: the decoder needs the class to know which book decodes
the digits. There is a way out that is worth thinking about: code the digits of
a partition under a *union* model over all the candidate books of that residue
--- the prior becomes a mixture over classes weighted by what has been decoded
so far --- and recover the class at the end of the partition from the values.
That is a real coding scheme (it is how a decoder with an ambiguous alphabet
works), it is expensive, and it would make the class free. Probably 1-2% of
output for a great deal of pain, and it fails on any encoder whose class rule is
not in the table.

**A harmonic long-term predictor along the frequency axis.** Pitched music has
energy at multiples of f0, so a packet's residue is quasi-periodic *in bin
index* with period `f0 / binwidth`. Estimate that lag once per packet per
channel from the previous packet's decoded nonzero pattern (a cheap
autocorrelation of the binary significance map over 20-200 lags --- a few
thousand popcounts, about 1% of a packet's model cost), then give the digit
model the value at `slot - lag*stride` as a "harmonic neighbour". This is the
long-term/pitch predictor of speech and lossless audio coding transplanted from
the time axis to the frequency axis, and it reaches structure that neither `t1`
(one packet back) nor `q1`/`q2` (one bin back) can see. It would show on solo
instrument and vocal material and on nothing else in the present corpus.

**Checkerboard coding of the codewords in a partition.** Code codewords 0, 2,
4, ... of a partition first, then 1, 3, 5, ..., so that the second half has a
frequency neighbour on *both* sides in the current packet rather than only
below. This is the checkerboard context model of learned image compression,
where it buys most of a full autoregressive context at twice the speed. The
reason it is a wild card and not a main idea: the evidence in
`tsvcomp-dig.idx` says frequency neighbours are weak here --- `a_w1` and
`b_w1` sit at one or two buckets, `a_q2` at two --- because the residue is a
whitened signal by construction. If a cheap offline test showed the *east*
neighbour to be worth much more than the west one (it should not be, but the
asymmetry between "already coded this packet" and "from last packet" muddies
the comparison), this would become a main idea.

**Multi-symbol coding of the digit with a mixed CDF.** Replace the binary chain
for a digit entirely: build a CDF over the `nv` multiplicands as a weighted
mixture of the trie prior, an adaptive per-context frequency table, and a
two-sided geometric whose scale is `ax` --- mix the three in probability space
with weights learned per context --- and code one symbol with a range coder or
rANS. One coder operation and one model evaluation per digit instead of three to
seven. The ratio argument is that a mixture of *distributions* can beat a chain
of mixed binary decisions when the alphabet is small and the shape is known;
the counter-argument is that logistic mixing of binary predictions is the whole
reason this class of model wins, and `SUBSTREAMS.md` already says the coder is
only 4% of the time, so the speed case is weaker than it looks. Worth an offline
comparison on logged data before anyone writes a line of it.

**Ask what the spectrum sounded like.** Every idea above works on the residue or
one step behind it. The complete version is to run a real inverse MDCT and
overlap-add inside the model --- decoding the audio, in floats, deterministically
on both sides --- and predict the next packet's spectrum from the *time-domain*
signal: a sinusoidal model, an LPC fit, a transient detector that fires before
the encoder's block switch does. The cost is a few hundred instructions per
coefficient, ten to fifty times what the model spends per digit today, so at
0.5 MB/s it would be unshippable. But it bounds everything: an offline
experiment that decodes a file to PCM, re-analyses it, and measures how much of
the residue's entropy a perfect spectral predictor removes would say, once and
for all, how much room is left in the 94.8% of the output that residue digits
occupy --- and that number is not in any of the documents.
