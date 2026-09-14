# Ideas from the signal angle: what a Vorbis decoder knows, and what oggcomp's model does not

Agent 1 of 5.  Angle: the *signal*.  Vorbis is a transform codec; the
compressor models the coded symbols and their symbol-level neighbours,
but never the thing the symbols describe.

## The gap in one sentence

The digit model's thirty context variables (`oc_residue.inc::digit()`,
`IDX/tsvcomp-dig.idx`) are `rno pass band col q1 q2 t1 t2 n1 w1 x1 p0 ps
pn ax zrun cls crun cm bkq chn blk vpos pq` and their signed twins.
**Not one of them is derived from the floor.**  The class model
(`IDX/tsvcomp-cls.idx`) is thinner still -- `rno band prev prev2 t1 tn
blk pq`, eight variables -- and also has no floor.  Yet `payload()` in
`vb_packet.inc` decodes *every channel's floor before any residue*, and
`oc_floor.inc::floor()` leaves this packet's post values in
`hist.fl_yhist` and `fl_cur`, where residue coding could read them for
free.  A residue digit is `round(MDCT / floor)` put through libvorbis's
noise normalisation; the floor is the divisor, and the model is blind to
it.  Every idea below is some way of pointing the model at the signal.

Numbers I measured with `./oggcomp c -v` on this tree (release build):

| file | digits | bits/digit | floor | bits/post | class |
|---|---|---|---|---|---|
| music-stereo-q5 | 94.8% | 1.558 | 2.42% | 1.251 | 2.35% |
| music-managed-b96 | 92.7% | 1.099 | 4.53% | 1.456 | 2.02% |
| chirp-stereo-q10 | 91.7% | 3.142 | 6.76% | 3.233 | 1.04% |
| noise-stereo-q3 | 94.3% | 1.282 | 2.35% | 0.896 | 2.08% |
| sine-stereo-q5 | 67.6% | 1.049 | **23.5%** | 1.063 | 1.92% |
| uncoupled-stereo-q4 | 53.9% | 0.684 | **29.1%** | 1.400 | 2.03% |
| multi6-48k-q4 | 65.2% | 0.955 | **24.3%** | 1.789 | 2.54% |
| sweep-mono-qm1 | 45.3% | 0.870 | **36.5%** | 2.927 | 4.95% |

and on the 1,257-clip set of IMPROVEMENTS 7.3 the floor is 424 kB of
5.44 MB -- **7.8%, at 3.36 bits a post**.  IMPROVEMENTS 4.7 dismisses
the floor as "under 5% of the output on music"; that is true of
`music-stereo-q5` and false of every tonal, small or short file in the
corpus, and false of the clip set, which is what `oggdet` exists for.
So several ideas below are floor ideas.

Block switching, counted by parsing the mode bit out of every audio
packet (`scratchpad/ideas/blk.py`):

| file | packets | short | transitions |
|---|---|---|---|
| music-stereo-q5 | 406 | 70 (17.2%) | 32, in runs of 2/4/5 |
| music-managed-b96 | 403 | 66 (16.4%) | 31 |
| chirp-stereo-q10 | 504 | 502 (99.6%) | 2 |
| noise-stereo-q3 | 96 | 11 (11.5%) | 2 |

So on real music there are ~16 long runs averaging 20 packets and ~16
short runs of 2 to 5, and `hist.dg_hist` is indexed by `tc_blk`: the
first long packet after a short run has a `t1` from 2-5 packets ago,
and the first short packet after a long run has a `t1` from ~20 packets
(0.23 s) ago.  The model is never told.

Budget note: a decision costs ~1,260 instructions and a digit is ~2.5
decisions, so ~3,000 instructions per digit; `tc_make_dig` already
spends ~610 of them turning 30 inputs into 10 indices, about 20 per
input.  Anything costing a handful of operations per digit, or a few
hundred per partition (a partition is 16-32 digits), or a few thousand
per packet (a packet is ~900 digits per channel) is free by comparison.
Nothing below needs more than that except the two wild cards that say so.

---

### 1. The frame-axis resonator: a two-tap MDCT predictor per bin
- **Idea:** For a pure sinusoid at angular frequency w, the MDCT
  coefficient at a fixed bin, read across frames, is itself a sampled
  sinusoid of frequency `w*N` (N = hop).  So `X_m = a*X_{m-1} - X_{m-2}`
  holds **exactly**, with `a = 2*cos(w*N)`, and for a component near bin
  k, `w*N = pi*(k + 1/2 + delta)` so `a = -+2*sin(pi*delta)` -- a number
  in [-2, 2] that depends only on how far the partial sits off the bin
  centre.  Keep one adapted `a_k` per (blk, channel, bin) -- sign-LMS or
  normalised LMS on the reconstructed absolute coefficients -- and give
  the digit model `mp = qlog(|pred|)` and the sign model `mps = sign(pred)`
  and `mpc = |pred|` bucketed, where
  `pred = (a_k * S_{m-1}(k) - S_{m-2}(k)) / floor_now(k)` and
  `S(k) = digit(k) * floor_then(k)` is the reconstructed coefficient in
  absolute units.  This is one multiply-add and one divide (a shift by a
  per-partition exponent) per digit, plus a 3-operation LMS update per bin.
- **Why it might work:** it is the only predictor in the list that
  predicts the *sign*, and libvorbis preserves signs exactly:
  `_vp_noise_normalize` writes `rint(x)` for `|x| >= 0.5` and
  `unitnorm(x)` -- sign times one -- for the small values it keeps as
  energy filler, so the sign of a residue digit is the sign of the MDCT
  coefficient even where its magnitude is pure dither.  Signs are 28% of
  the digit bits (IMPROVEMENTS 4.5) and "nearly random": on
  `music-stereo-q5` that is ~369 k signs costing ~38 kB, 26.5% of the
  file.  For a bin-centred partial `a = 0` and the recursion collapses to
  `X_m = -X_{m-2}`, a period-4 sign pattern -- which is the *mechanism*
  behind 4.5's observation that the (t1, t2) sign pair predicts where
  either alone does not.  4.5 proposes the pair as two context buckets;
  this proposes the actual recursion with its coefficient estimated, which
  also covers the off-centre partials the pair cannot, and yields a
  predicted *magnitude* as a by-product.  Linearity means it survives
  coupling: `_vp_couple` forms the magnitude and angle channels as
  signed linear combinations of two floor-normalised spectra, so both
  obey the same recursion with the same `a_k`.
- **Expected gain:** music 1 to 3.5% of output (say 0.5-1.5% from signs
  and 0.5-2% from the magnitude side); `chirp-stereo-q10`, which is 100%
  short blocks of a swept tone at 3.14 bits a digit, 3 to 8%;
  `sine-stereo-q5` and `sweep-mono-qm1` similar; low-bitrate managed
  files 0.5-1.5% (the phase term is weaker where two digits in three are
  zero, but the floor normalisation still pays); the clip set less,
  because 38-packet streams never let `a_k` converge.  Confidence:
  high that something is there (the recursion is exact, not statistical);
  medium on the size.
- **Cost:** needs the "floor bus" of idea 2 first.  Two i16 per
  (blk, chan, bin) for `S_{m-1}, S_{m-2}` in absolute units and one i16
  for `a_k` -- for a 1024-bin stereo stream about 24 kB, nothing beside
  the 366 MB of tables.  ~10 instructions per digit, ~2% speed.  Two or
  three new `.idx` variables, so `tc_make_dig`/`tc_make_sgn` grow.  No
  stream-format change beyond the usual `OC_VER` bump for a model change.
- **Risks:** at 0/+-1/+-2 quantisation the inputs to the recursion are
  savagely quantised, and `_vp_noise_normalize`'s energy filler puts +-1
  at bins with no partial at all, which will make `a_k` wander; the LMS
  must be gated on the bin actually being tonal (idea 4's confidence
  statistic is the natural gate).  Coupling's per-bin choice of *which*
  channel becomes the magnitude flips the sign when L and R swap
  dominance, which is common in decorrelated stereo.  And on noise-like
  bins the predictor is worthless, which the mixer has to learn.
- **How to measure cheaply:** no model change needed for the first look.
  Instrument the encoder to dump, per digit: `slot`, `blk`, `chan`,
  `digit`, `floor_y` at that bin for this and the two previous packets.
  Offline, fit `a_k` per bin by least squares on the absolute
  coefficients, and score two adaptive estimators of the sign -- one on
  `(sign t1, sign t2)` as 4.5 proposes, one on `sign(pred)` bucketed by
  `|pred|` -- against the model's current 0.82 bits per sign on
  `music-stereo-q5`.  That is a half-day script of exactly the shape
  IMPROVEMENTS 7.4 already used for the zero decision.

### 2. The floor curve as a digit and class context (the "floor bus")
- **Idea:** render this packet's floor to a per-bin array once per
  channel (`vd_render` between the already-decoded posts in
  `fl_fy`, the value being `y*mult`, an index into
  `floor1_inverse_dB_table`, i.e. dB at ~0.541 dB a step), keep last
  packet's array beside it, and expose to `digit()` and `classify()`:
  `fa` the absolute floor level at this bin; `fd` its change since the
  previous packet at this bin; `fs` the local slope; `fc` the local
  second difference (chord-minus-envelope curvature); and `fz` whether
  the floor posts bracketing this bin coded as exactly zero.  Five cheap
  variables and one array per channel.
- **Why it might work:** four separate mechanisms.  (a) *Level.*  Where
  libvorbis's floor sits at the absolute hearing threshold rather than
  on the signal, the band is at the noise floor and the residue is all
  zeros; `band` alone cannot tell "quiet because 18 kHz" from "quiet
  because the passage is quiet".  (b) *Curvature.*  floor1 is piecewise
  linear in dB between posts; the true envelope is not, so the residue
  is systematically large where the chord undershoots and zero where it
  overshoots -- and this is *sub-partition* resolution, which is exactly
  what `col` lacks.  The `.idx` comment says `col` "measured at +0.27%
  alone and worse in combination"; position in a partition is
  meaningless on its own, position relative to the floor's shape is not.
  (c) *Zero posts.*  floor1's fit marks a post unused when the
  interpolated value is good enough, and an unused post codes as 0, which
  is why posts are only 1.25 bits: so the bracketing posts' coded values
  are a free, dense, per-region measure of how hard the encoder had to
  work -- how peaky the spectrum is there.  (d) *It needs no history.*
  Every existing informative variable (`t1 t2 n1 w1 x1 p0 ps ax`) is
  zero on the first packet of a stream.  IMPROVEMENTS 7.3 puts 240 kB,
  4% of the clip corpus, on the model starting cold; floor variables are
  live from packet one.
- **Expected gain:** music 0.5 to 2%; low-bitrate managed 1 to 2.5% (the
  zero decision is 49% of its digit bytes and (a),(b),(c) all aim at
  zero-ness); the clip set 1 to 3% including the cold-start part; tonal
  files more.  Confidence: high that it pays something, medium that it
  pays over 1%.
- **Cost:** the smallest of the top three.  One `vd_render` walk per
  channel per packet -- ~1024 adds against ~900 digits, so under two
  operations a digit amortised -- two i16 arrays of `n` per channel per
  block size (about 16 kB stereo), five new `.idx` lines in `dig` and
  three in `cls`, ~80 instructions per digit through `tc_make_dig`.
  No format change beyond `OC_VER`.  This is also the prerequisite for
  ideas 1, 3, 5, 7, 8 and 13, so it should be built first regardless.
- **Risks:** `cls` is already "the biggest single factor" at 4.65% and is
  chosen by libvorbis *from the magnitudes in the partition*, so it may
  already say most of what the floor level says, at partition
  resolution; the marginal value is then only the sub-partition part and
  the cold-start part.  `pq`, the codebook prior, may likewise have eaten
  the within-vector part.
- **How to measure cheaply:** the floor curve is already decoded, so a
  dry-run instrumented build can dump `(digit, fa, fd, fs, fc, fz, cls,
  band, col)` per digit with a dozen lines in `digit()` and no model
  change at all.  Then measure the conditional entropy of "digit is
  zero" given `cls` alone against `cls` plus each floor variable, on
  `music-stereo-q5`, `music-managed-b96` and the clip set.  If the drop
  is under 2% the whole family is dead and ideas 1, 3, 5, 7, 13 lose
  their best input; that is a one-day answer for a week's work.

### 3. Energy accounting: replicate libvorbis's noise normalisation
- **Idea:** libvorbis's `_vp_noise_normalize` walks each
  `normal_partition` (16 or 32 values) in *descending magnitude* order,
  writes `rint(x)` while `x*x >= 0.25`, then spends a running energy
  accumulator `acc` on +-1 filler values until `acc < normal_thresh`,
  then writes zeros for the rest.  The accumulator is a conservation
  law: the sum of squares of the coded digits in a normalisation block
  approximates the sum of squares of `MDCT/floor` over that block.  The
  decoder can predict that total -- `E = sum over the block of
  (t1[k]*floor_prev[k]/floor_now[k])^2` -- and then maintain, per digit,
  `ebud` = what is left of it and `eper` = `ebud` divided by the slots
  remaining in the block.  Give the digit model both, plus `nzp`, the
  count of non-zeros in the aligned block one packet ago, and `pos16`,
  the position modulo the normalisation block size.
- **Why it might work:** the zero decision alone is 37% of the digit
  bytes on `music-stereo-q5`, 49% on the managed file and 59% on the
  clip set -- the single largest cost centre in the program.  The
  existing zero-ness variables (`zrun`, `ax`, `pq`, `cls`) are all
  *local* or *marginal*; none of them expresses the constraint that
  binds hardest late in a block, which is that the energy is spent.
  `ebud` near zero says every remaining digit is zero with near
  certainty; `eper` large with two slots left says a big digit is
  coming.  This is emphatically not the run-length coding measured out
  in 7.4: it is a per-digit context, seen at every digit (7.4's whole
  argument against run symbols), and it carries information no per-digit
  context in the model carries today.  The energy is also far more
  stable frame to frame than any individual digit, because noise
  normalisation is *designed* to preserve it -- so `E` from the previous
  packet is a good estimate where `t1` bin by bin is noise.
- **Expected gain:** 0.5 to 2% on music, 1 to 3% on low-bitrate and
  managed files where zero-ness dominates, 1 to 2.5% on the clip set.
  Confidence: medium.
- **Cost:** one squared accumulate per digit (the values are tiny; a
  4-entry table), one pass over the previous packet's history per
  partition (16-32 adds, ~1 per digit), four `.idx` variables.  Needs
  the floor bus for the scaling.  No format change.
- **Risks:** `normal_partition`, `normal_start` and `normal_thresh` vary
  by encoder mode and are not in the stream; the block alignment has to
  be guessed or learned (a `pos16` variable at both 16 and 32 lets the
  optimizer pick).  aoTuV and ffmpeg normalise differently or not at
  all.  And 7.4 is a standing warning that the codebook prior may
  already be doing this work: "the codebook prior is already doing what
  those axes would".
- **How to measure cheaply:** from the same per-digit dump, compute for
  each 16- and 32-aligned block the coded sum of squares and check
  frame-to-frame stability after floor scaling; then score an adaptive
  estimator of "digit is zero" given `(cls, pq-bucket, ebud, eper)`
  against one given `(cls, pq-bucket)` alone, in the table format of
  7.4.  If `ebud` does not beat 0.574 / 0.533 / 0.840 bits per digit on
  the three files there, stop.

### 4. A per-bin predictability statistic: online tonality
- **Idea:** keep one byte per (blk, channel, bin): a leaky average of
  the recent prediction error of the temporal predictor, e.g.
  `err = |digit - predicted|` folded into `e_k = (e_k*7 + err*8)/8`.
  Expose `tone = qlog(e_k)` as a context to the digit model, the sign
  model and the class model.  It costs one subtract, one shift and one
  add per digit.
- **Why it might work:** every temporal predictor in the program -- `t1`,
  `t2`, `ax`, and ideas 1 and 3 -- is excellent on tonal bins and worse
  than useless on noise bins, and the model has no way to know which
  kind of bin it is in.  A mixer keyed by `m` can learn a global
  weighting, not a per-bin one.  `tone` is the cheapest possible
  spectral-flatness measure: instead of computing tonality from the
  spectrum, measure whether the spectrum *behaved* predictably here
  lately.  It is also the natural gate for idea 1's LMS update (only
  adapt `a_k` where the bin is tonal) and it turns ideas 1-3 from
  single variables into confidence-weighted ones, which in a
  context-mixing model is usually worth more than the predictor itself.
- **Expected gain:** 0.3 to 1% on music on its own, and it multiplies
  the value of ideas 1-3 -- a predictor the model can learn to distrust
  selectively is worth appreciably more than one it cannot.  Confidence:
  medium-high for the small number, because this is the standard
  SSE/confidence trick and this data has a very strong tonal/noise
  dichotomy.
- **Cost:** trivial.  One u8 per (blk, chan, bin), ~8 kB stereo; three
  instructions per digit; one `.idx` variable in three families.  No
  format change.
- **Risks:** may be largely redundant with `ax` and `cls`, both of which
  correlate with tonality; and with only 0/+-1 digits the error signal
  is coarse.  Cold: useless for the first few packets of a stream.
- **How to measure cheaply:** it is a three-line change to the tuning
  build plus one `.idx` line at pattern zero; `IDX/opt.pl` will then
  place it, which is the repository's own prescribed test ("add the
  variable at pattern zero, let `opt.pl` decide where it goes, read the
  corpus total").  Cheaper still: from the per-digit dump, split the
  digits into quartiles by `e_k` and check whether the conditional
  entropy of the digit given `t1` differs across quartiles.

### 5. Envelope motion compensation for the floor
- **Idea:** the floor model's second-strongest factor is `ep` --
  `vd_fold(hy[p], pred, quant)`, "what floor1 would have coded for the
  previous packet's curve against this packet's prediction", i.e. the
  residual if the envelope had not moved.  Envelopes do move, and for
  glides, vibrato, portamento and sweeps they move *along the frequency
  axis*.  Estimate a per-packet frequency shift `D` (and only then a
  gain) by aligning the previous packet's rendered curve against the
  posts of this packet already decoded, and compute `ep` from the
  *shifted* curve.  Because floor1 posts are coded in list order --
  coarse posts first, `oc_floor::build` computes `rnk` -- the alignment
  can be estimated causally *within the packet*: after the first handful
  of posts, cross-correlate them against the previous curve over
  `D` in a small range and use the winner for the rest of the list.
- **Why it might work:** the bracketing-line prediction `prd` already
  absorbs a uniform *level* change, because it interpolates between this
  packet's own neighbours -- which is why a plain gain term would add
  little.  It absorbs nothing of a frequency translation: a peak that
  moves one post to the right makes two posts jump in opposite
  directions and `ep` is wrong on both.  `chirp-stereo-q10` pays 3.233
  bits a post for 6.76% of its output and `sweep-mono-qm1` 2.927 bits
  for 36.5% of its output, against 1.251 bits on music -- that gap is
  the moving envelope, priced.
- **Expected gain:** `sweep-mono-qm1` 3 to 10% of the file,
  `chirp-stereo-q10` 1 to 3%, `sine-stereo-q5` and the other tonal small
  files 1 to 4%, the clip set 0.3 to 1%, `music-stereo-q5` 0.1 to 0.3%
  (vibrato and portamento are real in music but the floor is only 2.42%
  of it).  Confidence: medium-high on the tonal files, low on music.
- **Cost:** a cross-correlation over ~30 posts and ~16 shifts per
  channel per packet -- 500 operations against a packet's ~900 digits at
  3,000 instructions each, i.e. nothing.  One new floor variable (`ep`
  recomputed, plus `D` itself as a context so the model knows how much
  it trusted the shift).  Two i16 arrays per channel.  No format change.
- **Risks:** with 20-60 posts the correlation is over a very short
  vector and will be noisy; a wrong `D` is worse than no `D`, so the
  shifted `ep` must be offered *beside* the unshifted one rather than
  replacing it, doubling the variable count.  Most real music is not a
  sweep.
- **How to measure cheaply:** offline, from a dump of the post values per
  packet: for each packet compute `sum |fold(shifted prev curve) |`
  over the posts for each `D` in [-8, 8], and report the entropy of the
  folded values at the best `D` against `D = 0`.  Ten lines of script
  over data the encoder can dump with three lines added to `floor()`.

### 6. Signs: the MDCT main lobe across bins, and bin parity
- **Idea:** three additions to `fam_sgn`.  (a) `b1s`, the sign of the
  *same channel's previous bin in this packet*, which is `q1s` for a
  type-0/1 residue and `q2s` for a two-way interleaved type-2 one --
  one semantically correct variable in place of two that mean different
  things in different streams.  (b) `par = k & 1`, the parity of the
  bin index, crossed with `t1s` -- for a partial near bin centre the
  frame-to-frame sign advance is `pi*(k+1/2)`, so the period-4 sign
  pattern runs in opposite directions on even and odd bins and a model
  without parity averages the two away.  (c) `mps`, the predicted sign
  from idea 1, with `mpc` its confidence.
- **Why it might work:** `q1s` is at pattern zero in *every* index of
  `IDX/tsvcomp-sgn.idx` while `q2s` is live in indices `b` and `d`.
  That is the fingerprint of a corpus dominated by coupled stereo, where
  `q1` is the other channel and `q2` is the previous bin: the optimizer
  found the variable that happened to mean "neighbouring bin" for those
  files and left the other dead.  On an uncoupled or multichannel stream
  the meanings swap and both are half-used.  And the MDCT of a windowed
  partial has a main lobe two or three bins wide whose coefficient signs
  are deterministically related, so the neighbouring bin's sign is real
  information in exactly the tonal places where signs are otherwise
  expensive.  Signs are ~26.5% of the output on music.
- **Expected gain:** 0.3 to 1.5% on music; more on tonal files; on
  `uncoupled-stereo-q4` and `multi6-48k-q4` the `b1s` part alone should
  show, and those are the files IMPROVEMENTS 4.1 already says the corpus
  is too thin for.  Confidence: medium-high for (a) which is nearly
  free, medium for (b), tied to idea 1 for (c).
- **Cost:** (a) and (b) are two `.idx` lines and about six instructions
  in `digit()`; (c) rides on idea 1.  No format change.
- **Risks:** `q2s` may already be carrying (a) for every file in the
  corpus, making it a refactor rather than a gain; the parity effect is
  smeared by windowing and by any partial off bin centre; noise
  normalisation's +-1 filler has signs that follow the *pre-quantisation*
  spectrum, which at those amplitudes is close to noise.
- **How to measure cheaply:** from the per-digit dump, tabulate the
  empirical `P(sign | b1s)` and `P(sign | t1s, t2s, parity)` on
  `music-stereo-q5`, `sine-stereo-q5` and `uncoupled-stereo-q4` and
  compare with the model's measured 0.82 bits a sign.  No build needed.

### 7. Give the class model the signal
- **Idea:** the class model has eight variables and no notion of
  amplitude beyond the previous packet's class number.  Add: `fa`/`fd`
  from the floor bus (the absolute level of the floor over this
  partition, and its change since the previous packet); `e1 =
  qlog(sum |digit| )` over this partition one packet ago, which is a
  much finer version of `t1`; and `ec`, the same quantity scaled by the
  floor change -- the predicted energy of *this* partition.  Also the
  class of the same partition in the *other channel* of this packet,
  which for a coupled or correlated pair is already decoded.
- **Why it might work:** libvorbis picks the class from the magnitudes in
  the partition, so the class is very nearly a deterministic function of
  `spectrum / floor` over that partition.  The decoder knows the floor
  exactly and the spectrum approximately (from the previous packet's
  reconstruction), so it can *compute an estimate of the class* rather
  than extrapolating the class number.  A floor that dropped 6 dB while
  the signal held still raises every class in that band, and the present
  model, which sees only `prev` and `t1`, has to relearn that each time.
- **Expected gain:** class is 3,360 bytes (2.35%) on `music-stereo-q5`,
  1,680 (2.02%) on the managed file, 166 kB (3.0%) on the clip set.  A
  10-25% cut of the stage is 0.2 to 0.6% of music, up to 0.75% of the
  clips.  The same variables then also feed the digit model, where the
  leverage is thirty times larger.  Confidence: medium-high that the
  class stage moves, given how thin the variable list is.
- **Cost:** tiny.  Needs the floor bus.  One pass over the previous
  packet's digits per partition, four `.idx` lines.  No format change.
- **Risks:** the class costs only 1.26 bits already and the stage is
  small, so even a good relative win is a small absolute one; and the
  class is not quite deterministic (libvorbis's classifier is a cost
  heuristic over the whole classword, not a per-partition threshold).
- **How to measure cheaply:** dump `(class, fa, fd, e1, ec, prev, t1)`
  per partition -- a few hundred thousand records for the corpus -- and
  measure the conditional entropy of the class under the present
  variable set against the extended one.  An afternoon.

### 8. Predict the floor by re-fitting it to the previous packet's spectrum
- **Idea:** rather than predicting post `p` from the same post one packet
  ago, replicate what floor1's fitter would have done: take the previous
  packet's reconstructed spectrum in absolute units (`digit * floor`),
  run the same sort of least-squares line fit over each floor partition
  that `floor1_fit` runs, and use the resulting quantised post value as
  the prediction `ef`.  Crucially the fitter also decides whether a post
  is *needed*: predict "this post codes exactly 0" when the refit says
  the interpolated value is within tolerance.
- **Why it might work:** the present predictors `ep`, `p1`, `o1`, `fy`
  all carry the previous packet's *floor*, which is the fitter's output.
  The fitter's *input* -- the spectrum -- is also known, and it contains
  the information the quantisation and the "is this post needed" decision
  threw away.  Where the previous packet's residues were large in a band,
  the floor was too low there and the encoder will raise it; where they
  were all zeros, it was too high and it will drop.  That feedback loop
  is a first-order law of how a floor moves and the model cannot see it.
  Posts that code 0 are the majority (1.25 bits a post says so), so
  predicting *which* post codes 0 is most of the floor stage.
- **Expected gain:** 5 to 15% of the floor stage: music 0.12 to 0.36%,
  `sweep-mono-qm1` 2 to 5%, `sine-stereo-q5` 1 to 3%, the clip set 0.4
  to 1.2%, `chirp-stereo-q10` 0.3 to 1%.  Confidence: medium.
- **Cost:** one line fit per floor partition per channel per packet --
  about 30 partitions of 30 bins, 2,000 operations a packet, negligible
  -- plus the previous packet's reconstructed spectrum, which idea 1
  already keeps.  Two floor variables.  No format change.
- **Risks:** libvorbis's fitter works on the *masking curve*, not the
  spectrum, so a plain fit to the reconstruction is a caricature of it;
  the tolerance constant (`twofitatten`) is encoder- and quality-
  dependent.  aoTuV differs.  And the floor is small on the file that
  matters most.
- **How to measure cheaply:** offline from a floor + digit dump: compute
  the refit prediction and report the entropy of the folded post values
  given `(col, ef)` against `(col, ep)`.  If the refit does not beat the
  previous curve, the idea is dead without touching the model.

### 9. Window shape, history age and cross-block-size history
- **Idea:** three small facts the model is not told.  (a) `wprev` and
  `wnext` -- already coded, in `oc_frame.inc::wprev/wnext` -- change the
  analysis window's shape and hence the leakage and the validity of any
  temporal prediction; give them to the digit model as two bits.  (b)
  `age`: how many packets ago this `(blk, pass, channel)` history was
  last written.  Measured above: on `music-stereo-q5` a long packet
  after a short run has a `t1` 2-5 packets old and a short packet after
  a long run one ~20 packets (0.23 s) old.  (c) `t1x`: the *other* block
  size's history at the mapped bin -- short bin `i` covers long bins
  `8i..8i+7`, so `t1x` is their RMS for a short packet and
  `hist[i/8]` for a long one.
- **Why it might work:** `hist.dg_hist` is indexed by `tc_blk`, so the
  two block sizes keep entirely separate histories and neither is told
  how stale it is.  16 of 406 packets on the music file begin a long run
  and 16 begin a short one; those are the packets straddling transients,
  which are the expensive ones.  (a) is nearly free and is a genuine
  signal fact: a long block whose next block is short has a narrowed
  right overlap, so its coefficients are not comparable with the
  previous long block's in the way the model assumes.
- **Expected gain:** 0.2 to 0.8% on music and on the managed file
  (17% and 16% short packets, 32 and 31 transitions); near zero on
  `chirp` (99.6% short, 2 transitions), `sine` and the 8 kHz files.
  Confidence: medium for (a) and (b), lower for (c).
- **Cost:** (a) and (b) are two counters and two `.idx` lines, under
  five instructions per digit.  (c) is one extra load and an index
  mapping, plus keeping the mapping when the interleave stride differs.
  No format change.
- **Risks:** the corpus barely exercises this -- only two files have more
  than a handful of transitions -- so `opt.pl` will fit the patterns to
  two files, exactly the trap IMPROVEMENTS 4.1 warns of for the coupled
  channel.  A transient-rich corpus file is wanted first.
- **How to measure cheaply:** no dump needed for (b): add `age` at
  pattern zero in the tuning build and let `opt.pl` place it; the corpus
  total answers.  For (c), first check offline whether the mapped
  cross-size value correlates with the digit at all.

### 10. The class implies a maximum; track whether it has been spent
- **Idea:** the class is chosen by libvorbis from the magnitudes in the
  partition, so it implies a rough ceiling and a rough floor on what the
  partition contains.  Track, within the partition, `mx` = the largest
  magnitude coded so far and `rem` = the slots left, and give the digit
  model `mx` and `mx` crossed with `rem`.  A class that implies a
  magnitude-3 value, with `mx = 0` and two slots left, makes a large
  digit nearly certain; the same class with `mx = 3` already seen makes
  the rest cheap.
- **Why it might work:** this is a coupon-collector constraint that
  tightens as the partition proceeds, and nothing in the model expresses
  it.  `pq`, the codebook prior, knows only the prefix of the *current
  codeword* (dimension 2 to 8), not the partition; `zrun` counts zeros
  but not magnitudes; `cls` is constant over the partition and so cannot
  express "the class's promise is still outstanding".  It is the same
  shape of argument as idea 3 but about the maximum rather than the
  energy, and it needs no floor and no history, so it works from packet
  one and on any encoder.
- **Expected gain:** 0.3 to 1% of the digits on music; more where
  partitions are long and classes are coarse.  Confidence: medium; the
  mechanism is certain, the magnitude is not, because `cls` at four
  buckets is already worth 4.65% and may have absorbed it.
- **Cost:** one max and one decrement per digit; two `.idx` lines.  No
  format change, no memory.
- **Risks:** libvorbis's classifier is a cost heuristic over the whole
  classword, so the implied ceiling is soft; and for a cascaded residue
  the later passes' classes mean something different.
- **How to measure cheaply:** first establish the premise, which is one
  script over a per-digit dump: for each `(residue, class)` tabulate the
  distribution of the partition's maximum magnitude.  If the class does
  not concentrate it, stop.  If it does, score an estimator of the digit
  given `(cls, pq, mx, rem)` against `(cls, pq)`.

### 11. Harmonic comb tracking
- **Idea:** estimate a fundamental `f0` per packet per channel from the
  previous packet's reconstructed spectrum -- a harmonic product
  spectrum or an autocorrelation of the log magnitude over a
  downsampled (say 128-bin) version, a few thousand operations -- and
  give the digit model `h = distance in bins to the nearest multiple of
  f0`, quantised to a handful of buckets, together with a confidence.
- **Why it might work:** the floor is a *smooth* envelope; it cannot
  represent a harmonic comb, whose teeth are one or two bins wide.  So
  for pitched material the comb lives entirely in the residue -- the
  comb *is* what the residue is for.  `t1` sees the comb only bin by bin
  and only where the previous packet happened to put a non-zero there,
  which noise normalisation makes unreliable; an explicit `f0` pools the
  evidence across all the harmonics and keeps working when the pitch
  glides, which is precisely when `t1` fails.  It also generalises: a
  bin that was zero last frame but sits on a harmonic is a different
  proposition from one in a valley.
- **Expected gain:** 0.5 to 2% on pitched music, zero on
  `noise-stereo-q3`, and possibly large on `sine`/`sweep`/`chirp`.
  Confidence: low-medium -- this is the most speculative of the numbered
  ideas.
- **Cost:** a few thousand operations per packet per channel, which is
  under 1% of a packet's model time; a pitch tracker is 100 lines and
  has to be bit-exact across compilers (integer arithmetic only, no
  `float` -- see `./mk.sh check`'s contract).  Two `.idx` variables.  No
  format change.
- **Risks:** polyphonic music has several `f0`s and the tracker will pick
  one or none; the comb in the *residue* domain is partly flattened
  already because the floor does follow the gross spectral shape;
  octave errors flip the prediction on every other harmonic; and it is
  the one idea here whose implementation is genuinely intricate.
- **How to measure cheaply:** entirely offline.  Run a pitch tracker in
  Python over dumped reconstructed spectra, and measure `P(digit = 0 |
  cls, pq, h)` against `P(digit = 0 | cls, pq)`.  If `h` does not move
  the zero rate on `music-stereo-q5` by several percent, drop it.

### 12. Coupling-aware cross-channel modelling
- **Idea:** for a type-2 interleaved residue, the channels are
  libvorbis's magnitude and angle, not left and right, and their
  relationship is the deterministic square-polar map that
  `_vp_couple` applies and mapping0's inverse undoes.  Give the digit
  model, for an angle slot, `mgv` = the magnitude slot's value *at this
  bin, this packet* -- signed, not log-quantised -- and the derived
  quantity `room = |mgv| - |q2|`; and track a per-bin running estimate
  of the L/R level difference from the floor bus (the two channels have
  *different* floors, so the coupling is between differently scaled
  quantities and the scaling is known).
- **Why it might work:** the map guarantees that after the inverse, one
  output channel equals the magnitude value exactly, so the angle's
  magnitude is bounded by the magnitude channel's in all the usual
  cases; and where `mag = 0` the angle is almost always 0.  `q1`
  supplies the magnitude slot for an angle digit today but only through
  `tc_qlog`, which throws away the sign and the exact value, and the
  bound is a statement about exact values.  Above libvorbis's point-
  stereo limit the angle is forced to a small set, which `band` sees
  only as a frequency.
- **Expected gain:** 0.2 to 0.8% on coupled stereo, which is most files;
  more on `uncoupled-stereo-q4` and `multi6-48k-q4` when combined with
  IMPROVEMENTS 4.1's same-packet `x1`.  Confidence: medium.
- **Cost:** two `.idx` variables, a handful of instructions per digit,
  no memory beyond the floor bus.  No format change.
- **Risks:** `q1` and `q1s` may already carry all of it -- `q1s` is at
  pattern zero in the sign model but live in the digit model's `d` and
  `f` indices, so the optimizer has had the chance.  Which channel
  becomes the magnitude is chosen per bin, so the relation is noisier
  than the algebra suggests.
- **How to measure cheaply:** tabulate the joint distribution of
  (magnitude digit, angle digit) at the same bin from a dump, and
  compare its entropy with the model's current cost for the angle
  digits.  One script, no build.

### 13. A loudness-change and attack detector from the floor alone
- **Idea:** one number per packet per channel: the mean change of the
  rendered floor level since the previous packet, and one more: the
  change concentrated in the top octave.  Give both to the digit, class
  and floor models as `dE` and `dEhi`.
- **Why it might work:** an attack raises the floor everywhere and most
  at high frequencies; a decay lowers it; and in both cases every
  temporal prediction in the model is less trustworthy than usual, which
  the model can only learn if it is told.  `blk` -- short or long block
  -- is the only transient signal the model has today, and it is one bit
  that fires only when libvorbis chose to switch, which on
  `music-stereo-q5` is 32 times in 406 packets.  `dE` is continuous,
  fires on every packet and is available before a single residue digit
  is coded.
- **Expected gain:** 0.1 to 0.5% overall; mostly on dynamic material.
  Confidence: medium, but it is nearly free, so the expected value per
  hour of work is good.
- **Cost:** two accumulations per packet, two `.idx` lines, ~40
  instructions per digit through `tc_make_dig`.  Needs the floor bus.
  No format change.
- **Risks:** small, and likely correlated with `blk` and with `cls`.
- **How to measure cheaply:** add it at pattern zero in the tuning build
  once the floor bus exists and let `opt.pl` place it; it is two lines.

---

## Wild cards

**A real IMDCT, and a forward MDCT of an extrapolated signal.**  The
honest signal-domain answer.  Decode each packet fully -- inverse
floor, IMDCT, window, overlap-add -- keep the reconstructed PCM, and
before coding packet *m* extrapolate the signal forward over the half
window that is not yet known (linear prediction, or sinusoidal
extrapolation of tracked partials), take the forward MDCT of the
extrapolated frame, divide by the known floor, and use the result as a
per-bin prediction of the residue including its sign.  A 2048-point
MDCT by FFT is ~100 k operations per channel per packet against a
packet's ~2.7 M instructions of model, so it is 4% of the time -- it is
*affordable*, which is the surprise.  What kills it is not speed but
determinism: the decoder must compute bit-identical values, so every
operation has to be integer or strictly specified fixed point, across
compilers and platforms, forever, and `./mk.sh check` and the
`OC_VER` contract make that a permanent obligation.  Idea 1 is the
cheap, safe, linear shadow of this; if idea 1 pays well, this is where
the ceiling is.

**Code the digits of a partition in prediction-sorted order.**
`_vp_noise_normalize` processes each normalisation block in descending
order of magnitude and stops emitting when the energy runs out, so the
zeros are *last in that order*, not last in bin order -- which is why
run lengths measured badly in 7.4, the runs being an artefact of the
wrong ordering.  The decoder could sort a partition's slots by predicted
magnitude (from ideas 1-3, all causal) and code the digits in that
order, buffering the partition and assembling the codewords afterwards;
in that order the energy budget of idea 3 becomes monotone and nearly
deterministic.  The obstacle is `tc_ptab`'s codebook prior, which walks
a trie over the codeword prefix in vector order and is the model's
second-biggest factor: reordering breaks it, and a prior over arbitrary
partial assignments is a much harder object.  Worth an estimate before
it is dismissed.

**Replicate the encoder exactly, once you know which encoder it is.**
`vb_dict.inc` already identifies the exact libvorbis build from the
codebook table row -- 350 rows, 35 encoders.  That row also identifies
the *mode setup*: `normal_start`, `normal_partition`, `normal_thresh`,
`twofitatten`, the psychoacoustic tables, the coupling limits, all of
them compile-time constants in that build.  Ship them beside the
codebooks and the decoder can replicate the encoder's deterministic
decisions rather than infer them -- the noise-normalisation accounting
of idea 3 becomes exact, the floor refit of idea 8 becomes libvorbis's
own fitter, and the class model of idea 7 can compute the classifier's
own cost function.  It is a large table and a large commitment to
other people's constants, and it fails silently on anything rewritten
(OptiVorbis) or unknown; but it is the logical end of the codebook
table's own argument.

**Code the next packet's floor before this packet's residue.**  The
floor is a spectral envelope and the residue is what is left of the
spectrum; the envelope of packet *m+1* is a far better statement about
where packet *m*'s signal is going than anything in packet *m-1*.
Packets are ~400 bytes, so buffering one and coding the floors one
packet ahead of the residues is mechanically easy; the model's symbol
order need not be the bitstream's, since the decoder reassembles the
bitstream at the end.  It costs the streaming property that the current
structure has by accident, complicates `oggdet`'s segment framing, and
the gain is unknown -- but the *two-sided* floor context (before and
after) is strictly more information than the one-sided one, and it is
the only idea here that gets information from the future.

**Absolute hearing threshold as a static prior.**  libvorbis's ATH is a
fixed curve in a fixed amplitude scale, and floors rarely go below it.
A 256-entry table of the ATH in floor1 `y*mult` units, indexed by
frequency, would give the floor model a prior for the very first packet
of a stream -- where it has no history at all and currently pays 3.36
bits a post on the clip set against 1.25 on music.  Half a signal idea
and half a trained prior, and it costs 512 bytes.
