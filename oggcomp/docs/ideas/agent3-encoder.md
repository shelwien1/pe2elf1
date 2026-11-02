# Agent 3 — the encoder as a model

Angle: almost every Ogg Vorbis file was written by libvorbis or aoTuV.  Their
source is public and deterministic.  `vb_dict.inc` already exploits one
consequence (the codebooks are static tables).  Everything below asks what
*else* is a function of the encoder's code rather than of the audio.

Two things below I checked against the corpus with throwaway scripts before
writing them down (scripts in this directory):

* **libogg's page-boundary rule reproduces every audio page boundary in the
  corpus exactly** — 99/99 boundaries on the files my crude
  "where do the headers end" heuristic got right, including 37/37 on
  `music-stereo-q5`, 22/22 on `music-managed-b96`, 29/29 on
  `chirp-stereo-q10`, 11/11 on `silence-8k-long-qm1` (the 255-lacing branch).
* **Granule positions are exactly the block-size sum** — on `music-stereo-q5`,
  predicted == actual for all 37 non-final pages; the final page is short by
  96 samples (the encoder's trailing truncation), i.e. one free value per link.

Stage costs I am pricing against (`./oggcomp c -v`, release build):

| file | out | headers | pages | packets | floor | class | digits |
|---|---|---|---|---|---|---|---|
| music-stereo-q5 | 143,003 | 0.13% | 0.31% | 0.02% | 2.42% | 2.35% | 94.8% |
| music-managed-b96 | 83,281 | 0.22% | 0.52% | 0.03% | 4.53% | 2.02% | 92.7% |
| noise-stereo-q3 | 24,405 | 0.72% | 0.52% | 0.03% | 2.35% | 2.08% | 94.3% |
| multi6-48k-q4 | 3,316 | 6.4% | 1.6% | 0.10% | 24.3% | 2.5% | 65.2% |
| uncoupled-stereo-q4 | 1,548 | 11.6% | 3.1% | 0.21% | 29.1% | 2.0% | 53.9% |
| mono-22k-q4 | 1,184 | 13.2% | 5.5% | 0.48% | 18.0% | 2.9% | 59.9% |
| tiny-8k-q0 | 192 | 50.9% | 12.7% | 0.81% | 12.8% | 2.6% | 20.0% |

Note the floor is 12–29% of output on every small file, not the "2–8%" the
brief quotes; and pages are 1.6–12.7%.

---

### 1. The rest of the setup packet is a table row too, and so is the vendor string

- **Idea:** `vb_dict.inc` names the codebooks; everything around them is still
  coded field by field.  But the floor records, residue records (including
  `res.cascade` and the `book[class][pass]` matrix), mapping records and mode
  records of a libvorbis stream are not per-file either: `vorbisenc.c`
  assembles them from `_floor_44[]`, `_residue_44_*`, `_mapping_template_*`
  indexed by (channels, sample-rate band, quality), exactly the way it selects
  the books.  Generate the same kind of table `vbooks_gen.inc` was generated
  from — enumerate `oggenc`/`vorbis_encode_init` over its whole parameter
  space per release — but key it **per record**, not per packet: a
  `flr.set` symbol for the whole floor record (the `flr.x` list is the
  expensive part: 20–40 raw `rangebits`-wide values), a `res.set` symbol for
  the whole residue record, a `map.set`/`mode.set` symbol for the rest.  Add a
  built-in list of the ~40 known vendor strings ("Xiph.Org libVorbis I
  20200704 (Reducing Environment)" and friends) so `cmt.byte` becomes one
  symbol for an untagged file, and *condition the vendor symbol on the
  `cb.set` row already chosen* — the row identifies the release, so the two
  are nearly the same variable.
- **Why it might work:** the same argument that already paid off, applied one
  level up.  Nothing in a libvorbis setup header is computed from the audio;
  it is a template lookup.  The corpus proves the header cost is *all*
  first-occurrence cost: `./t.sh` reports that the same file chained to itself
  codes its three headers "for a few bytes more than once" via `oc_hcache`, so
  the 98–211 bytes per file is what a built-in table would replace with a
  symbol.  Per-record keying is what makes it robust the way per-book keying
  made `vb_dict` robust: an aoTuV setup with libvorbis floors still hits on
  the floors.
- **Expected gain:** on small files, most of the header stage: `tiny-8k-q0`
  98 → ~10 bytes (**~46% of the file**), `mono-22k-q4` ~12%,
  `uncoupled-stereo-q4` ~11%, `multi6-48k-q4` ~6%.  On music 0.1%.  On
  `oggdet -c` over the 1,257-clip set, the 197 kB of per-stream setup is
  **3.3% of the archive**.  Corpus total 0.6–1.0%.  Confidence: high on the
  mechanism (it is the proven one), medium on the table's hit rate against
  files in the wild.
- **Cost:** a generator (build every libvorbis/aoTuV release, sweep
  `vorbis_encode_init` over rate × channels × quality, dump the non-book
  sections), a few hundred kB of table, a match-on-bits comparator like
  `vg_pack`.  Zero per-symbol CPU.  Stream format change: yes (`OC_VER`).
- **Risks:** a file whose header was edited (OptiVorbis output, a tag
  rewriter that reflowed the setup) misses — but per-record keying means it
  misses only on the record it touched.  A non-libvorbis encoder (ffmpeg's
  `vorbis_enc` writes its own floor) needs its own rows, which the generator
  gets for free by including ffmpeg.  Table size is the real risk if setups
  are stored whole rather than per record.
- **Measure cheaply:** dump the corpus's setup packets, strip the book
  sections (the `vb_dict` code already locates them), and count distinct
  floor/residue/mapping/mode record blobs across the corpus plus the clip set.
  If the count is small — OPTIVORBIS §3.1 counted 22 distinct whole setups in
  246 kB — the table is worth building.  Separately, instrument `hdr()` to
  split the header stage into id/comment/setup to size the vendor half.

---

### 2. The page layout is libogg's, not the audio's: derive it

- **Idea:** stop coding page structure and re-derive it.  Three parts.
  (a) **Packet lengths** from the payload, as IMPROVEMENTS §7.3 already
  proposes (the decoder knows a packet's byte length once it has decoded its
  fields and the tail flag).  (b) **Page boundaries** from libogg's
  `ogg_stream_flush_i(os,og,0,4096)`: accumulate lacing values and break the
  page after the last completed packet once `acc > 4096` and at least four
  packets are complete, or at 255 lacing values, or at EOS.  (c) **Granule
  positions** from the mode sequence: `gp += (blocksize(i-1)+blocksize(i))/4`
  per packet, exactly as OptiVorbis's `granulator.rs` does.  Code one "the
  prediction held" flag per page and a correction only where it does not.
- **Why it might work:** I measured it.  The boundary rule reproduces every
  audio page boundary in the corpus (99/99, including the 255-lacing branch on
  `silence-8k-long-qm1`), and granule prediction is exact on all 37 non-final
  pages of `music-stereo-q5` — the last page is short by 96 samples, which is
  one value per link.  The `seq` field is already a counter, `serial` is
  constant within a link, and `type` is derived from bos/eos/continuation.  So
  after (a)–(c) the pages stage holds: one flag per page, one granule
  correction per link, and the header pages' layout.
- **Expected gain:** music `440 → ~15` bytes = **0.30%**; `mono-22k-q4`
  **~4.5%**; `tiny-8k-q0` **~10%**; `multi6-48k-q4` ~1.3%; the clip archive's
  41,870-byte pages stage (0.77%) → ~3 kB, **~0.7%**.  Corpus total ~0.4%.
  Confidence: high — this is measured, not estimated.
- **Cost:** the largest restructure here: the packet payload must be coded
  before the page record, which inverts the loop in `ogg_stream.inc` and
  changes what `oc_frame.inc::page` codes.  `MAXPAY`/spill handling and the
  raw-page fallback have to survive it.  Per-symbol CPU: nil (a running
  accumulator).  Stream format change: yes.
- **Risks:** a file repaged by a tool (OptiVorbis's page packer, an Ogg
  muxer, a streaming server that re-chunks) predicts wrong on every page —
  but then it costs one mispredict flag per page plus what the fields cost
  today, which is what it costs today.  Chained and multiplexed files need the
  accumulator per logical stream.  `nfill` is 4096 in libogg but a caller can
  pass another value (`ogg_stream_pageout_fill`); make the threshold a coded
  per-link value with 4096 as the near-certain symbol.
- **Measure cheaply:** the scripts in this directory (`pg2.py`) already do the
  boundary half over any corpus; run them over a few thousand files from the
  wild before committing.  For granule, the same script extended to read the
  mode bit of each packet.

---

### 3. Re-run libvorbis's residue classifier backwards: per-partition statistics as digit context

- **Idea:** `res0.c::_01class` picks the class of a partition as the first `k`
  with `max <= classmetric1[k] && (classmetric2[k] < 0 || ent < classmetric2[k])`,
  where `max` is the largest `|value|` in the partition and `ent` the sum of
  `|value|`; `_2class` (coupled, residue type 2) uses `magmax <= classmetric1[k]
  && angmax <= classmetric2[k]`, the maxima over the magnitude and the angle
  slots separately.  So the class **is** a quantised statistic of the digits.
  The digit model has no per-partition aggregate at all — `q1/q2` are the last
  two digits, `zrun` a run, `ax` a per-slot average, `ps/pn/p0` per-slot
  across passes.  Add, as context: the running `max |digit|` in this partition
  (split by `chn` parity for interleaved residue), the running `sum |digit|`,
  the running count of non-zeros, and the number of slots left — and from
  those the derived **quota state**: "the class says some slot must exceed T,
  and none has yet, and four slots remain".
- **Why it might work:** the class is coded before the digits, so the decoder
  is *given* the constraint for free and then throws it away.  The model
  currently learns only the marginal `P(digit | cls, chn, col)`; what it
  cannot express is the dependence — that having already met the class's lower
  bound relaxes the rest of the partition, and not having met it forces a
  large value into the slots that remain.  The upper half is equally real:
  `ent < classmetric2[k]` with the small integers libvorbis uses (the
  `_residue_44_*` metrics are values like 0,1,2,4,8,16,32 for max and
  0,0,99,4,10,90 for ent) is a hard budget over a 16- or 32-sample partition,
  and once the budget is spent every remaining digit is zero.  Zeros are
  47–66% of digits and the zero decision is 37–59% of the digit bytes, so a
  variable that makes the tail of a partition deterministic lands in the
  right place.  This is also why libvorbis's `_vp_noise_normalize` exists: it
  forces the number of unit-magnitude values in a normalization partition to
  track the partition's energy, which is exactly a count budget.
- **Expected gain:** music 0.3–1.0% of output (the quota alone is worth
  `-log2 P(some slot exceeds T | class)` per partition; at ~21,280 partitions
  and 0.3–0.8 bits each that is 0.8–2.1 kB of 143 kB).  Low-bitrate mono
  files, where `ent` binds hardest, 0.5–1.5% of the digits stage.
  Confidence: medium-high that it pays something, medium on the size — the
  codebook prior and `cls` already carry the marginal.
- **Cost:** small.  Four counters in `oc_model::P`, updated in `digit()`,
  quantised through `tc_qlog`/`tc_sq`, four lines in `IDX/tsvcomp-dig.idx` at
  pattern zero and an `opt.pl` pass.  Per-symbol CPU: a handful of adds on a
  path that already costs 1,260 instructions.  Stream change: yes (any new
  context is).  No table needed for the first cut.
- **Risks:** the aggregate may be redundant with `ax` + `cls` + `col`; the
  budget applies to the *final* value (sum over passes) while passes are coded
  one whole vector at a time, so during pass 0 the constraint is loose —
  though most classes in `_residue_44_*` use a single pass, where it is exact.
  Noise normalization's partition (`vi->normal_partition`, 16 or 32) is not
  the residue partition, so the count budget should be tracked on the
  16-sample grid, not on `psz`.  A non-libvorbis encoder just makes the
  variables uninformative, which costs the tuning pass and nothing else.
- **Measure cheaply:** log `(rno, pass, class, partition, slot, digit)` from a
  debug build over three files, and in python compute the empirical
  conditional entropy of the zero decision with and without
  `(nonzeros-so-far, slots-left, max-so-far)` added to the existing
  `(prior, last-8, class)` context from IMPROVEMENTS §7.4's table.  §7.4's
  harness is exactly the right shape; it found 0.618 bits with prior+last-8+
  class against the model's 0.574, so anything that pulls the *estimate* below
  0.574 is worth building.

---

### 4. The floor's "memo" rule: once one post in a bracketing interval codes 0, the rest do

- **Idea:** `floor1.c::floor1_fit` walks posts in list order and, for post
  `i`, looks up its bracketing *used* neighbours `(ln, hn)` and does
  `if(memo[ln]!=hn){ memo[ln]=hn; ... inspect_error ... }`.  If the interval
  was already inspected and accepted (the earlier post in it was left unused,
  so the neighbours were not updated), the block is skipped entirely, post `i`
  keeps `fit_valueA/B = -200`, `post_Y` returns -1, and `floor1_encode` writes
  `output[i]=predicted|0x8000`, i.e. **the coded value 0**.  Give the floor
  model a context variable that reproduces this: maintain the dynamic
  used-neighbour arrays the way `floor1_fit` does (a post counts as used when
  its coded value is non-zero) and a `memo[]` beside them, and feed
  `first-in-interval / interval-already-accepted / interval-was-split` as a
  three-valued factor.
- **Why it might work:** it is a hard encoder rule, not a statistic, and the
  decompressor can evaluate it exactly from values it has already decoded.
  The model cannot infer it today: posts sharing an interval share `prd`,
  `room`, `hl`, `lov` and `hiv` identically, so the only thing separating the
  first from the rest is `col`, a static frequency rank.  `lov==0 && hiv==0` is
  a partial proxy and some of it is already being picked up that way — which
  is exactly why the `b_lov`/`c_hiv`/`s_lov`/`f_lov` patterns in
  `IDX/tsvcomp-flr.idx` are non-zero.
- **Expected gain:** 5–15% of the floor stage.  Music 0.12–0.36%;
  `uncoupled-stereo-q4` **1.5–4%** of the file; `multi6-48k-q4` 1–3.5%;
  `mono-22k-q4` ~1–2.5%.  Corpus total 0.2–0.5%.  Confidence: high on the
  mechanism (`floor1_fit` is short and I am quoting it), medium on the size
  because of the `lov/hiv` overlap.
- **Cost:** ~20 lines in `oc_floor.inc` mirroring `floor1_fit`'s neighbour
  bookkeeping, one factor line in each index of `tsvcomp-flr.idx`, one
  `opt.pl` pass.  Per-post CPU: a few comparisons on a path that runs once per
  post, not per digit.  Memory: two `u8[VB_MAXPOST]` per channel.  Stream
  change: yes.
- **Risks:** the dynamic neighbour update in `floor1_fit` is guarded by
  `if(ly1>=0 || hy0>=0)` and the `ret0 && ret1` degenerate branch also leaves
  a post unused without updating, so my reconstruction of "used" from
  "coded non-zero" is an approximation — as a *context* that is fine (the
  model eats the exceptions), as a hard rule it would be wrong.  aoTuV
  replaced parts of `floor1_fit`; the variable then simply predicts less well.
  A post that is genuinely used and happens to equal its prediction also codes
  0 and pollutes the "used" test.
- **Measure cheaply:** instrument a debug build to log, per post,
  `(fno, list index, coded value, memo-state)`, then in python measure
  `H(value==0 | memo-state)` against `H(value==0 | col, lov, hiv)` on
  `uncoupled-stereo-q4` and `mono-22k-q4`.  If the memo state is not already
  implied by `(lov,hiv,col)`, build it.

---

### 5. Sub-class resolution: where in its class band was this partition last packet

- **Idea:** the class model's strongest context is `t1`, the same partition's
  class one packet ago.  But the class is a coarse quantisation of
  `(max, ent)` against fixed thresholds, and the decompressor holds the
  previous packet's *actual* decoded values in `hist.dg_hist`.  Recompute
  `max` and `ent` (or `magmax`/`angmax`) over the previous packet's partition
  and feed their position *within* the class band — "last packet sat just
  under the boundary" versus "deep inside" — as a two- or three-bucket factor
  beside `t1`.  The thresholds need not be tabulated: they are identifiable
  online, because for every partition of class `k` the classifier guarantees
  `magmax <= classmetric1[k]` and `magmax > classmetric1[k-1]`, so the running
  minimum and maximum of observed `magmax` per class converge to the
  thresholds within a few dozen packets.
- **Why it might work:** the class is a deterministic function of a continuous
  quantity that moves slowly between packets.  Knowing only the bucket throws
  away the position inside it, which is precisely the information that says
  whether the next packet crosses the boundary.  The class model already
  proves neighbourhood information pays: `tn` (the next partition's class one
  packet ago) and `prev2` were "worth 429 bytes of class over four files".
- **Expected gain:** 8–20% of the class stage.  Music 0.19–0.47%;
  `noise-stereo-q3` ~0.2–0.4%; small mono files almost nothing (class is
  already 0.1–0.2 bits there).  Confidence: medium.
- **Cost:** small — a per-partition scan of the previous packet's history
  (already resident, `P.hist`) once per class symbol, not per digit; two
  factor lines in `tsvcomp-cls.idx`.  Online threshold identification is two
  arrays of 16 ints per residue.  Stream change: yes.
- **Risks:** the previous packet's partition is the same *slot*, not the same
  content, when the block size switches — the history is already keyed by
  `hist.tc_blk` so that is handled, but it halves the amount of history.  The
  thresholds converge slowly on short files, which is where classes are
  cheapest anyway.  aoTuV changes the metrics; online identification handles
  that, a table would not.
- **Measure cheaply:** from the same digit log as idea 3, compute per-class
  histograms of `magmax`/`angmax` — if they are cleanly bounded (they should
  be: the thresholds are hard) the mechanism is confirmed, and the conditional
  entropy of the class given `(t1, position-in-band)` versus given `t1` alone
  is an immediate number.

---

### 6. Do not code decisions the codebook has already decided

- **Idea:** `tc_fam::code` always walks head(0), head(1), head(2), then the
  length nodes, and asks the coder for each.  `tc_ptab` knows the exact symbol
  weights at the current trie node, so it knows when a branch has weight
  exactly zero — but `tcp_p` clamps to `[1, CM_PONE-1]`, so "impossible"
  arrives at the mixer as "very likely" and still costs a coded bit.  Expose a
  `certain(m)` accessor (weight on one side == 0) and skip the decision
  outright on both sides.  Same for `len(k)`, for `man()` and for `sign()`
  when the node holds only one sign of a magnitude.
- **Why it might work:** libvorbis's residue books have small alphabets —
  `{-1,0,1}` for the `_p1_*` family, `{-2..2}` for `_p2_*` — so for most
  non-zero digits at least one node of the walk has only one reachable answer:
  after "not 0" in a ternary book the value *is* 1; after "not 0, not 1, not 2"
  in a 9-value book the length node `k=1` is forced.  That is roughly one
  certain decision per non-zero digit, and 47% of digits on music (66% on the
  managed file) are non-zero.  The books are sparse in the joint sense too:
  `tc_ptab`'s trie is over real codewords, so a partly-decoded vector often
  admits only one continuation.
- **Expected gain:** 0.1–0.3% of output on music (327k certain decisions at
  the 0.005–0.01 bits a near-certain decision costs through a seven-input
  mixer), similar elsewhere, and more on the class stage where the class books
  are genuinely sparse.  Plus **15–25% off encode and decode time**, which
  matters because it is what `opt.pl` costs.  Confidence: medium on the ratio,
  high on the time.
- **Cost:** small and local to `tc_ptab.inc`/`tc_fam.inc`.  Both sides compute
  the prior identically from the setup header, so it is deterministic.  Stream
  change: yes (fewer symbols).
- **Risks:** it removes the counters' chance to correct a wrong prior — if the
  trie's weights are ever zero where the stream is not (a book packed oddly, a
  `lengthlist` with a zero entry the encoder nonetheless used, a stream that
  is not from libvorbis), the decoder desynchronises rather than paying a few
  bits.  It must therefore be an *exact* zero-weight test over the entry
  lengths, and `rs_sym`'s `VB_IF(e >= bk->ent)` guard has to stay.  This is
  the one idea here with a correctness edge; fuzz it (OPTIVORBIS §3.5).
- **Measure cheaply:** in a debug build, count decisions where the node's
  weight on one side is zero, and accumulate `-log2(p)` over exactly those.
  That number is the ratio gain with no implementation at all.

---

### 7. The classifier's thresholds as hard per-slot bounds, split mag/ang

- **Idea:** idea 3's constraint, used as a bound rather than as a soft
  context.  For coupled residue (`_2class`), the class gives `|v| <=
  classmetric1[k]` on every magnitude slot and `|v| <= classmetric2[k]` on
  every angle slot, with `classmetric2` usually much the tighter.  Feed the
  two bounds (or the ratio of the bound to the codebook's own range, `bkq`) as
  a digit context, keyed by `chn` which already carries the interleave parity.
  Get the metrics either from a generated table keyed the way idea 1 keys
  residue records, or by online identification as in idea 5.
- **Why it might work:** the codebook is shared between the magnitude and the
  angle slots of an interleaved partition, so the codebook prior gives both
  the same range; the classifier does not.  `chn` lets the model learn the
  average difference but not the per-class difference, and the classes are
  where the difference lives.
- **Expected gain:** 0.1–0.4% on coupled stereo music, nothing on mono or
  uncoupled files (which go through `_01class` and get the `ent` budget of
  idea 3 instead).  Confidence: medium-low — much of this is already inside
  `cls × chn × bkq`.
- **Cost:** small if the metrics are identified online (two `i32[16]` per
  residue); a table otherwise.  Stream change: yes.
- **Risks:** heavily overlapping with idea 3 and with the existing `cls`/`chn`
  contexts; should be tested *after* 3 and only kept if it adds on top.  If
  `_2class`'s exact form differs from my recollection the variable is just
  noise.
- **Measure cheaply:** from the digit log, tabulate `max |digit|` over
  magnitude slots and over angle slots, per class — two clean staircases
  confirm the mechanism and read the thresholds off directly.

---

### 8. The neighbouring partitions' classes as digit context

- **Idea:** all classes of a residue are coded in pass 0 before any digit, so
  when a partition's digits are coded the classes of *every* partition in the
  packet are already known.  The digit model uses only its own (`cls`).  Add
  the class of the partition below and above in frequency (`cls-1`, `cls+1`)
  and, for the coupled case, the class the other submap gave at the same band.
- **Why it might work:** the classes are a coarse spectrum-energy profile of
  the whole packet, transmitted for free before the digits.  Spectra are
  smooth across partition boundaries, so a rising class to the right says the
  energy inside this partition leans high, which is exactly the `col`
  interaction the model cannot currently form.  The class model already gets
  paid for its neighbour variable `tn`.
- **Expected gain:** 0.1–0.3% of the digits stage on music, more on tonal
  material where the profile is peaky (`chirp-stereo-q10`, `sine-stereo-q5`).
  Confidence: medium.
- **Cost:** trivial — the class array `cl[]` is already materialised in
  `residue()` before the digit loop; two fields in `tc_dv`, two lines per
  index in `tsvcomp-dig.idx`, one `opt.pl` pass.  Per-digit CPU: two loads.
  Stream change: yes.
- **Risks:** more context width on the family that already owns 300 MB of
  tables; `HASHTABLES.md` says the tables are not short of rows, so this
  should be free, but it is the family where that is most likely to bite.
- **Measure cheaply:** the same conditional-entropy harness as idea 3, with
  `cls±1` added to the context list.

---

### 9. A floor alphabet that names the encoder's two attractors

- **Idea:** floor posts are coded as `vd_fold(y, pred, quant)`, so "the post
  is unused" (the common case) is symbol 0 and costs almost nothing.  But
  libvorbis has a *second* attractor: `vorbis_dBquant` clips at 0, so a post
  whose masking curve is below the encoder's dB floor is the absolute value 0
  — and everything above the encoder's lowpass is exactly that.  Its folded
  value is `2*pred-1`, which for `pred` around 50 is a symbol near 100 and
  costs 7–9 bits.  Replace the coded value by a bijection that swaps the
  absolute-zero symbol down next to 0: code `0` for "on the line", `1` for
  "absolute zero", and the remaining folds shifted by one.  Byte-exactness is
  untouched; it is a permutation of the alphabet both sides compute from
  `pred` and `quant`.
- **Why it might work:** the model walks a magnitude as three head nodes then
  a length and mantissa, so a symbol's cost is roughly logarithmic in its
  size.  Moving a frequent value from ~100 to 1 is the difference between
  two decisions and nine.  The transitions into and out of a silent band are
  where this bites: once a run of absolute zeros is established `pred` is 0
  and the fold is 0 already, so it is the first post of each such run that
  costs, and with block switching there is one per packet per band edge.
- **Expected gain:** 0.05–0.2% on music, 1–3% on the small bandlimited files
  where the floor is 12–29% of output (`tiny-8k-q0`'s floor is 6.5 bits a
  post — the raw posts and the big folds).  Confidence: medium-low; depends
  entirely on how often absolute-zero posts occur outside an established run,
  which is a one-hour measurement.
- **Cost:** ~15 lines in `oc_floor.inc`, symmetric on both sides.  No new
  tables, no new context.  Per-post CPU: two comparisons.  Stream change: yes.
- **Risks:** if absolute-zero posts almost always sit inside a run where
  `pred` is already 0, the remap buys nothing and slightly hurts by displacing
  fold value 1.  Encoders that do not clip (or that use `mult` 4, where
  `quant` is 64 and the folds are small anyway) see nothing.
- **Measure cheaply:** log every coded post with its `pred` and `quant`, and
  histogram the folded values against `2*pred-1`.  If a visible spike sits
  there, build it.

---

### 10. A shipped prior keyed by the encoder's identity

- **Idea:** the clip-set measurement says 240 kB — **4% of the archive** — is
  "a model that begins every 5 kB stream at zero and has not settled after 38
  packets".  The plan already calls for a trained prior.  The encoder angle
  is *which* prior: the `cb.set` row chosen by `vb_dict` plus the vendor
  string identify the release, the channel mode and (through the residue
  template) the quality band, so ship a handful of pre-trained counter/weight
  snapshots keyed by that identity and start the digit, class and floor
  families from the matching one instead of from neutral.
- **Why it might work:** the cold-start loss is a function of how far the
  initial state is from the converged one, and the converged state is much
  more a function of the encoder setting (quality, coupling, block sizes) than
  of the music.  The key is available *before* the first audio packet is
  coded, for free, because it is already being coded for the setup header.
- **Expected gain:** on the clip set, a large fraction of the 4% cold-start
  gap for `oggdet -c`; on a single small file, 5–20% (it is the same gap).
  Nothing on music, where the model converges in the first few pages.
  Confidence: medium — the gap is measured, the fraction a static prior
  recovers is not.
- **Cost:** the largest storage cost here: a snapshot per key, and the model
  has 366 MB of tables so only a small, dense subset (the top counter rows,
  the mixer weights, the APMs) can be shipped.  Stream change: yes.  Build
  complexity: the snapshots become part of what `./mk.sh check` must hold
  constant, like `vbooks_gen.inc`.
- **Risks:** overfitting to the training corpus; a wrong key is worse than
  neutral; and a shipped prior interacts with `opt.pl`, which would then be
  tuning against a moving initial condition.  Keep the key coarse (release
  family × channel mode × quality band, a dozen classes at most).
- **Measure cheaply:** take two streams from the same encoder setting, code
  the second with the model as the first left it (that is `oggdet -S`, which
  exists) and compare with `-c`.  The `-S`/`-c` gap of 431 kB on the clip set
  is the upper bound on what any prior can recover; split it by how much
  survives when the two streams share a setting versus not.

---

### 11. The coupled partner across passes

- **Idea:** `_vp_couple` writes the magnitude and angle of a coupled pair into
  adjacent interleaved slots with `mag = the larger of |L|,|R|` and
  `ang = ±(L-R)`, so `|ang|` is bounded in terms of `|mag|` and an angle slot
  whose magnitude partner is zero is itself almost always zero.  Within one
  pass the model sees this through `q1`/`q1s` (the previous digit in the
  partition stream *is* the magnitude slot).  Across passes it does not:
  `ps`/`pn` accumulate at the current slot only.  Add the accumulated value at
  the coupled partner slot — `P.ps[slot - 1]` for an angle slot, `P.ps[slot+1]`
  for a magnitude one — as a context.
- **Why it might work:** the coupling relation holds on the final value, which
  is the sum over passes, not on any single pass's digit.  At pass 1 and
  beyond `q1` carries only the partner's *refinement*, not its magnitude.
- **Expected gain:** 0.1–0.3% on coupled stereo music; nothing on mono,
  uncoupled or multichannel-uncoupled files.  Confidence: medium-low, and it
  overlaps with IMPROVEMENTS §4.1's `x1` work.
- **Cost:** trivial (two loads in `digit()`, one factor line).  Stream change:
  yes.
- **Risks:** most partitions use a single pass, in which case this adds
  nothing over `q1`; the gain is confined to the low bands where cascading
  happens.
- **Measure cheaply:** from the digit log, restrict to pass ≥ 1 of
  interleaved residues and measure the conditional entropy with and without
  the partner's accumulated value.

---

### 12. Simulate the bitrate manager: padding, and packets that end early

- **Idea:** `bitrate.c` runs a reservoir: `vorbis_bitrate_addblock` pads a
  packet with zero bits to hold a minimum bitrate and truncates one
  (`oggpack_writetrunc`) to hold a hard maximum, both as functions of the
  running reservoir state, which is a function of the packet lengths already
  seen and the `id.brmin/brnom/brmax` fields of the identification header.
  The decompressor has all of those.  So (a) predict the pad length rather
  than coding it (`F_PADNZ` and the tail bits), and (b) implement OPTIVORBIS
  §3.3's early-packet-end tolerance, using the reservoir to *predict where* the
  truncation happened rather than coding it.
- **Why it might work:** the reservoir is arithmetic over integers the decoder
  already holds; nothing about the audio enters it except through the packet
  lengths.  `testfiles/minbitrate-pad.ogg` exists precisely because this
  happens in the wild (streaming encodes with `-m`).
- **Expected gain:** near zero on the corpus as it stands — `packets` is
  0.02–0.8% and the two managed corpus files never truncate.  On
  min-bitrate-padded streams (internet radio archives) the padding can be a
  double-digit percentage of the file, and today it is coded as a tail run.
  On truncated streams the gain is not ratio but *scope*: today a page with
  such a packet is coded as bytes, at ~8 bits a byte instead of ~1.5.
  Confidence: low on the corpus, high on the class of files it targets.
- **Cost:** moderate — a faithful re-implementation of `bitrate.c`'s
  arithmetic, which is fiddly, plus the parser change for early termination
  (`bget`'s "packet ends inside a field" refusal in `vb_packet.inc`).  Stream
  change: yes.
- **Risks:** `bitrate.c` changed across releases, and getting it subtly wrong
  is worse than not predicting; it should be a *context* for the pad length,
  never a derivation.  Files from ffmpeg's encoder have no reservoir at all.
- **Measure cheaply:** generate a min-bitrate-padded and a max-bitrate-clipped
  file with `oggenc -m/-M` and look at the `raw` and `packets` stages.  If
  `raw` is non-trivial on the clipped one, §3.3 alone is worth doing.

---

### 13. The block-switch decision is a run, and `wnext` names the next packet's mode

- **Idea:** two small exactnesses in the `packets` stage.  (a) For a
  conforming encoder `next_window_flag` of packet *t* equals the blockflag of
  packet *t+1*'s mode, so the mode of *t+1* is already named by the time it is
  coded — add `oc_wp`/the previous packet's `wnext` to `F_MODE`'s context
  (today it is only `tc_prevmode`).  (b) libvorbis's transient detector
  (`_ve_envelope_search` in `envelope.c`) marks contiguous regions, so the
  blockflag sequence is strongly run-structured; give `F_MODE` a run-length
  context ("how long has this blockflag held") beside the order-1 one.
- **Why it might work:** (a) is a hard identity in every stream libvorbis
  writes and costs a one-line context change; (b) is the same observation that
  makes `crun` pay in the class model.
- **Expected gain:** bounded by the stage: 30 bytes on music (0.02%), 6 bytes
  on `mono-22k-q4` (0.5%), 2 bytes on `tiny-8k-q0` (0.8%).  So: **0.01–0.3%**,
  and it is in the list only because it is nearly free and because the mode
  sequence feeds idea 2's granule prediction, which is not free.
  Confidence: high that it works, high that it is small.
- **Cost:** two context variables in the `aux` family.  Stream change: yes.
- **Risks:** none beyond a wasted tuning slot.
- **Measure cheaply:** `-v`'s `packets` line before and after; it is a
  two-line change.

---

## Wild cards

**Re-run the psychoacoustic spreading function on the decoded floor.**  The
floor is a fit to libvorbis's masking curve, which is the running maximum of a
tone-masking and a noise-masking curve, each produced by convolving the
spectrum with a *fixed* spreading function whose slopes in dB per bark are
constants of the psy template (`_psy_tone_masteratt`, `_vp_noisemask`'s
`noisewindow*` tables).  That puts a hard bound on how fast the rendered floor
may fall with frequency — a masking curve cannot decay faster than the
spreading slope — which the decompressor can evaluate from the post `x` list
and the sample rate.  The floor model predicts a post by linear interpolation
between its bracketing neighbours (`vd_render`); a slope-bounded predictor
would be tighter on the falling edges, which is where the expensive posts are.
The reason I would not bet on it: the bound is on the *masking curve*, and
`floor1_fit` fits a piecewise line to it with its own error tolerance, which
loosens everything by an unknown amount.

**A libvorbis oracle.**  The decompressor can decode the Vorbis stream to PCM.
Feed that PCM back through libvorbis at the same settings — which the setup
header names exactly — and use the encoder's own output as the prediction for
the next packet.  The PCM is not the original (it has been through one
quantisation), so the re-encode will not match; but the *decisions* might: the
block switch, the floor's used/unused pattern, the class of each partition.
Those are coarse decisions about an already-coarse signal, and the whole point
of this document is that they are 5–15% of output on small files.  The reasons
not to bet: it puts a full Vorbis encoder and MDCT into both the compressor and
the decompressor, makes the stream depend bit-for-bit on libvorbis's floating
point, and is two or three orders of magnitude slower than the 0.5 MB/s the
program runs at today.  But a cheap slice of it — re-running only
`_ve_envelope_search`'s transient detector on the decoded PCM to predict the
next packet's blockflag — is integer-ish, fast, and would make idea 13 exact.

**One tuned parameter set per encoder family.**  `mk.sh dll N` already makes a
model a loadable library, and a stream names its model in its header.  So the
machinery exists for "this file was written by libvorbis 1.3.x at q5 stereo,
load the parameter set `opt.pl` tuned on exactly that population".  The
compressor picks the set from the `cb.set` row it is already coding; the
decompressor reads the row before it needs the model.  The cost is N tuning
runs and N copies of `IDX/`, and the risk is that the hill-climb has already
found a set that is near-flat across encoders — the profile in `101ff53` moved
236 patterns for 0.05%, which suggests the surface is flat and per-encoder
specialisation would buy little.  Worth one experiment: tune on
stereo-44.1k-q5 files only and see how much better than the shared set it gets
on held-out files of the same kind.

**Find the point-stereo and noise-normalization boundaries and tell the model
about them.**  Two fixed frequencies per encoder setting change the residue's
statistics abruptly: `vi->normal_start`, below which residue values are plain
roundings and above which `_vp_noise_normalize` forces unit magnitudes, and
`vi->coupling_pointlimit`, above which the angle channel carries a ternary
point-stereo decision rather than a difference.  The digit model sees
frequency only through `tc_qlog(partition)` and `tc_qlog(slot-in-partition)`,
whose buckets are coarse and in the wrong places, so a sharp statistical
boundary is smeared across two buckets.  Both frequencies are template
constants recoverable from the residue record (idea 1's table) or detectable
online as the partition index where the angle-slot alphabet collapses.  I
would not bet on it because the boundaries fall on partition edges by
construction and `band` may already separate them.

**The comment header as a fingerprint of the *muxer*, not the encoder.**  Tag
order, whether `ALBUM` precedes `ARTIST`, whether a `METADATA_BLOCK_PICTURE` is
base64 with or without padding, whether the framing bit is followed by
padding — all of these are fixed per tagging tool (EasyTAG, foobar2000,
picard, ffmpeg).  `oc_tagorder` already pays for a fixed id order; the next
step is a small set of *tool profiles* selected by the first tag seen, each
with its own tag-order prior.  Small money — headers are 0.13% on music — but
on a library of tagged files it is systematic, and the cover-art path
(`oggart.inc`) is where the bytes actually are.
