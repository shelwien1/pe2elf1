# BMF — possible compression improvements

Where this build could compress better, and what it would cost to get there.
Scope is **algorithmic**: changes to what is modelled and how predictions are
formed and combined, not constant-tuning of the existing models. Bitstream
compatibility is not a constraint — this reconstruction is already its own
format, and nothing below tries to stay readable by the original BMF.

`ALGORITHMS.md` describes what the codec does today; this document assumes it
and refers to it by section. Line references are `file:line` as of this commit.

**How claims are graded.** Compression proposals are easy to write and hard to
believe, so every claim below carries one of:

| tag | meaning |
|---|---|
| **[measured]** | a number produced by running this build; the experiment is described so it can be repeated |
| **[from the code]** | a structural fact, checkable by reading the named function |
| **[expected]** | a judgement from how comparable coders behave; no number was produced here |

Nothing below has been implemented. The expected gains are estimates, and the
only honest way to settle them is §13.

---

## 1. Baseline

`testfiles/`, this build, compress + decompress verified lossless:

| image | geometry | raw | coded | ratio | bpp | model(s) |
|---|---|---|---|---|---|---|
| `f05_200` | 1728×2339×1 | 505 286 | 21 286 | 23.7× | 0.042 | slow |
| `t1` | 320×240×1 | 9 662 | 2 622 | 3.7× | 0.270 | slow |
| `DLRAW` | 816×1200×4 | 489 718 | 225 784 | 2.2× | 1.844 | slow |
| `x_ai` | 2820×1600×8 | 887 278 | 148 780 | 6.0× | 0.264 | slow, transposed |
| `x_ci` | 2820×1600×8 | 3 278 170 | 633 144 | 5.2× | 1.123 | slow |
| `t8g` | 320×240×8 | 77 878 | 42 896 | 1.8× | 4.466 | alt-P2 |
| `t8p` | 320×240×8 | 77 878 | 43 664 | 1.8× | 4.466 | alt-P2 (+768 B palette) |
| `t24` | 320×240×24 | 230 454 | 53 924 | 4.3× | 5.615 | alt-P1 ×2, slow |
| `t32` | 320×240×32 | 307 254 | 53 996 | 5.7× | 5.622 | alt-P1 ×2, slow, alt-P1 |
| `x_ep` | 705×800×32 | 2 256 054 | 330 660 | 6.8× | 4.690 | alt-P2 ×4, joint |

Two facts about the *encoder* frame everything that follows. Both are
**[measured]**, from `bmf c -v` (§5.1 of `ALGORITHMS.md`):

* **The mode search is already accurate.** Its own predicted size lands within
  0.4 % of what actually ships — `t24` 53 896.0 estimated vs 53 904 shipped
  (+0.01 %), `x_ep` 331 852.0 vs 330 640 (−0.37 %). The encoder is not losing
  bits to bad decisions; it is losing them to the models it has to choose
  between.
* **The models it chooses between are close.** On the planes that carry the
  bulk of a file, the second-best model is within a few per cent of the winner:
  `t24` slot 0 — `p1` 53 052 vs `p2` 53 764 (1.3 %); `t8g` — `p2` 43 212 vs
  `p1` 44 048 (1.9 %); `x_ci` — `slow` 633 120 vs `p1` 664 708 (5.0 %); `x_ep`
  slots 0/1/2 — 4.3 %, 8.0 %, 3.7 %. Three models that finish within a few per
  cent of each other are not the same model; they are making largely
  independent mistakes, and the codec discards two of the three answers.

That second observation is the single largest structural opportunity in the
codec, and §2 is about it.

---

## 2. Mix the models instead of choosing one

**Now [from the code].** `search_filter` (`codec.inc:932`) trial-encodes each
plane with up to six flag combinations, keeps the cheapest, and throws the rest
away. Every plane is coded by exactly one of the slow model, alt-P1 or alt-P2.
There is no probability mixing anywhere in the codec — despite the name,
`mixer_fwd` (`counters.inc:183`) sums per-selector offsets to build a *context
index*, not a prediction.

**Change.** Run all three models on every pixel and combine their predictions in
the logistic domain with an adaptive mixer, the way the PAQ/LPAQ family and
every strong modern image coder does:

```
  p_final = squash( Σ  w_i(ctx) · stretch(p_i) )        stretch(p) = ln(p/(1−p))
  w_i    += λ · (bit − p_final) · stretch(p_i)          online gradient step
```

with the weight set selected by a small context (plane index, activity bucket,
bit position). This is strictly more general than selection: a mixer that
learns weight 1 on one model and 0 on the others reproduces today's behaviour,
so the floor is the current result minus the cost of learning the weights.

**What it needs first.** The three models do not currently speak a common
language: the slow model codes a symbol through `SymList`/`FreqRec` rank stages
(`sym_list.inc:72`), alt-P1 codes through 7-slot `CounterNode`s
(`counters.inc:8`), alt-P2 codes a ternary decision from a raw frequency triple
(`P2Freq::code_three_way`, `alt_p2.inc:29`). Mixing needs all of them binarised
onto the same decision tree — one shared binarisation of the residual, with each
model supplying a probability for each bit. That is the real work in this
proposal, and it is also the enabler for §3, §4 and §9.

**Expected [expected].** For three models within 1–8 % of each other, mixing
normally recovers appreciably more than the spread — the models' errors are
correlated but far from identical. A 5–15 % overall gain is the range this kind
of change lands in when the component models are already good; below 5 % would
be a disappointment and above 20 % would be surprising.

**Cost.** All three models run on every pixel, on both sides: roughly 3× the
current encode time for the final pass, and 3× decode, which today is the fast
path. Memory is the sum of the three blocks (alt-P1 alone is 10 MB of
`CounterNode`, `alt_p1.inc:21`) rather than the max. The mode search can then be
cut down or dropped, which claws back most of the encoder cost, since its whole
purpose was to choose between models.

**Risk.** This is a rewrite of the coding layer, not a patch. It should be built
behind a flag with the existing path intact, and the corpus used as a
regression net the whole way.

---

## 3. An SSE / APM stage on the coding probability

**Now [from the code].** alt-P2 has a five-bank bias cascade
(`step_bank`, `alt_p2.inc:324`; `code_banks`, `alt_p2.inc:541`) — but it refines
the *predicted sample value*, feeding a better prediction into context
selection. Once a context is chosen, the symbol is coded straight out of an
adaptive frequency triple with no further refinement. alt-P1 and the slow model
have no equivalent stage at all. Nothing anywhere maps a probability to a better
probability.

**Change.** After the model (or the mixer of §2) produces `p`, refine it through
an adaptive probability map indexed by `(quantised stretch(p), small context)`,
interpolating between adjacent buckets and updating both. Chain two or three
with different contexts — local activity, the previous residual's magnitude
bucket, the plane index.

**Expected [expected].** SSE/APM is the cheapest reliable win in context
modelling: 1–3 % for one stage, a little more for a chain, at a few table
lookups per coded bit. It is worth doing even without §2, and it composes with
it.

**Cost.** Small: a few hundred KB of tables and negligible time. Needs the same
binarisation as §2 to apply to the slow model and alt-P1.

---

## 4. Counters and state representation

**Now [from the code].** `BitCtr` (`counters.inc:73`) is a pair of 16-bit counts
plus a `limit`, halved when the total crosses the limit, with the limit growing
by 64 up to 0x4000 — adaptation slows as a context matures, on a fixed schedule.
`CounterNode` (`counters.inc:8`) is a 7-slot frequency vector, 16 bytes,
629 856 of them per alt-P1 plane. `P2Freq` (`alt_p2.inc:10`) is a `step` plus
three counts.

**Change.** Two independent, well-established upgrades:

* **Dual-rate counters.** Keep two probabilities per context, one fast-adapting
  and one slow, and mix them (or feed both to the §2 mixer). This tracks
  non-stationary regions without giving up precision in stationary ones, which
  is exactly what the fixed `limit` schedule is trying and failing to do with
  one number.
* **State machines instead of counts.** Replace the count pair with an 8-bit
  state (bit history) indexing an adaptive probability table, as in the
  `nex()`/`StateMap` construction. It is smaller per context — 1 byte against
  6 — and better, because the table entry is shared across all contexts in the
  same state and therefore learns far faster.

The memory saved is not incidental: alt-P1's table is 10 MB because each context
costs 16 bytes. At 1–2 bytes per context the same budget buys 5–10× more
contexts, which feeds §6.

**Expected [expected].** 2–5 % combined, and it makes every other model change
cheaper in memory.

**Cost.** Localised — `counters.inc` plus the update sites. Independent of §2.

---

## 5. Palette index reordering

**[measured]** — and nearly free.

**Now [from the code].** The only palette handling is grey-ramp detection
(`bmf_compress`, `bmf.cpp:67`): if the palette is exactly entry *i* =
*i*·(256>>bits) in all three channels, the image is retagged `depth_grey` and the
palette is dropped. Every other palette is stored raw and the indices are coded
as they arrive. But the models predict indices with MED and code residual
*magnitudes* — so how the palette is ordered decides how compressible the index
plane is, and the codec accepts whatever order the file happens to use.

**Measured.** Taking `t8g` (grey ramp, so its indices are already luminance
ordered) and applying a random permutation to the palette:

| | coded |
|---|---|
| `t8g` as shipped | 42 896 |
| palette randomly permuted | 49 324 (+15.0 %) |
| palette then re-sorted by luminance | 42 896 (fully recovered) |

15 % of that file is index ordering, and a sort recovers all of it.

**Change — and it must be a choice, not a rule.** Sorting by luminance is *not*
safe to apply unconditionally. The same experiment run on `t8p`, whose palette is
already an ordered colour ramp, makes things **worse**:

| | coded |
|---|---|
| `t8p` as shipped | 43 664 |
| palette re-sorted by luminance | 49 304 (+12.9 %) |

So the file's own order is sometimes the good one and a blind sort destroys it.
Three steps, in order of effort:

1. **Offer luminance order as a candidate.** Both sides can derive the
   permutation from the palette alone, so the only side information is **one bit**
   saying which order was used. The encoder already trial-encodes six flag
   combinations per plane (`search_planes`, `codec.inc:1241`); adding a seventh
   trial for the reordered index plane is the same machinery. This subsumes the
   grey-ramp special case — a grey ramp is already in luminance order, so its
   permutation is the identity.
2. **Search the ordering.** Luminance is only a proxy for "adjacent indices
   should be adjacent pixels". The real objective is to minimise
   Σ *C(i,j)*·|pos(i)−pos(j)| over the co-occurrence matrix of adjacent pixel
   pairs — a linear-arrangement problem, solvable well enough by spectral
   ordering or simulated annealing. Transmit the winning permutation (256 bytes)
   and let the trial pick between it, luminance and identity.
3. A caution from the experiment: a naive greedy nearest-neighbour chain over the
   co-occurrence matrix produced an ordering *worse* than luminance (49 328 on
   the same file), so step 2 needs a real optimiser, not the first heuristic to
   hand.

**Expected.** Nothing for palettes that are already ordered, which is why step 1
must be a trial rather than a rule. Large for screenshots, maps, scientific
colour maps, GIF-sourced art — anything whose palette order records the order
colours were discovered. **[measured]** 15 % on one adversarial case and −12.9 %
if applied blindly to the wrong one; **[expected]** 5–25 % on real
arbitrary-palette images, 0 % elsewhere once it is a trial.

**Cost.** Step 1 is one extra trial encode of one plane plus a bit on the wire,
and costs nothing at decode. Step 2 adds an encoder-side optimiser and 256 bytes.

---

## 6. Bigger and better-shaped context sets

**Now [from the code].** The slow model interns contexts into fixed direct-mapped
tables — `ctx_id1[192512]`, `ctx_id2[108800]`, `ctx_id3[712000]`
(`model.inc:135`) — via `intern_ctx` (`model.inc:616`), which mints a new ID on
first sight and otherwise reuses the slot. There is no collision handling because
there are no collisions: the signature indexes the table directly, so the context
set is capped by the table's shape, and `kCtxId3Limit = 53248`
(`bmf_util.inc:96`) hard-stops minting. alt-P2's counters are
`5·32768` (`alt_p2.inc:287`) and its frequency cells 15 552 (`alt_p2.inc:334`).
These are 1999 memory budgets.

**Change.** Hashed contexts with checksummed slots, the standard construction:
hash the signature, keep a small tag in the bucket to detect collisions, evict
on the priority the model prefers (usually lowest total count). That decouples
the *shape* of a context from the size of its table, which allows:

* longer spatial contexts (more neighbours, larger quantised gradients);
* contexts the current interning cannot express, e.g. joint contexts over two
  planes' co-located residuals;
* dropping the `kCtxId3Limit` cliff.

**Expected [expected].** 2–6 %, more on large images where the current tables
saturate and less on small ones where they never fill. Combines with §4 —
cheaper counters mean more contexts for the same memory.

**Cost.** Moderate; touches the slow model's context path. Hashing costs a
multiply per context and loses the exactness the current design enjoys.

---

## 7. Colour transform: fix the fit, then make it adaptive

**Now [from the code].** `choose_plane_coding` (`codec.inc:272`) picks which
plane is predicted from the other two, fits two blend weights by closed-form
least squares over gradient-domain second differences (`cost_candidate`,
`codec.inc:575`), refines them by a ±4 coordinate descent, then lets a
degenerate form (copy either reference, or a 64:64 average) take over if it
comes within `min(cost>>7, 0x4000)`. One weight pair serves the whole image.

**Measured.** `bmf c -v` shows the fit failing outright on `t24`:

```
    least-squares fit       65464.0 bytes
    copy first ref          12405.2 bytes  <<
    copy second ref         12532.5 bytes
    average 64:64           15649.8 bytes
    weights (0,0) fitted -> (-5,-5) after coordinate descent
```

The fit lands on `(0,0)` — it predicts *nothing* — so its estimated cost is five
times what simply copying a reference costs. The descent cannot rescue it
because its window is ±4. On `x_ep` the same fit produces `(21,46) → (19,49)`
and beats every degenerate form, so the machinery works in general; on `t24` it
degenerates.

**Change.** Three separable things:

1. **Make the fit robust.** The normal equations are regularised by adding 0.1
   to the determinant and the weights are clamped to [−64, 191]; a proper ridge
   term scaled to the data, and a fallback to the best degenerate form as the
   descent's starting point rather than the fit's output, would stop `(0,0)`
   from ever being the seed. Cheap and strictly safer than what is there.
2. **Add reversible integer lifting as a candidate.** YCoCg-R is exactly
   reversible in integers, costs two adds and two shifts per pixel, and beats a
   fitted linear blend on a large class of photographic content. It slots into
   the existing candidate list and the existing trial machinery picks it.
3. **Make the blend adaptive.** One global weight pair per image is a strong
   assumption; sky and skin want different weights. Either select the weight pair
   by a small local context, or replace the fixed blend with an LMS filter on the
   co-located reference samples — the same machinery alt-P2 already runs
   spatially (`NbRow::predict`, `alt_p2.inc:169`), applied across planes.

**Expected.** (1) costs nothing today on `t24` itself — the slack rule catches the
degenerate fit and `copy first ref` is genuinely the best of the four options
there. What it buys is the case the corpus does not contain: an image where the
fit degenerates *and* no degenerate form is good, where the encoder would ship a
useless transform with nothing to fall back on. Cheap insurance rather than a
measured gain. (2) and (3) **[expected]** 1–4 % on multi-plane photographic
images.

**Cost.** (1) and (2) are small and self-contained. (3) is a per-pixel filter on
top of the existing transform.

---

## 8. Multiple prediction filters, mixed

**Now [from the code].** alt-P2 runs one NLMS filter per context row — 28 float
taps over spatial and cross-plane features (`fill_row_inputs`,
`alt_p2.inc:378`), with 1088 filter rows selected by a 1920-value context
(`seat_nb_row`, `alt_p2.inc:294`) at a single adaptation rate per tap
(`bmf_p2_rate_init`).

**Change.** Run two or three filters over the same features at clearly different
adaptation rates and blend their outputs by inverse recent squared error (or feed
all of them to the §2 mixer). Fast filters track edges and texture changes; slow
filters win on stationary regions; a single rate is a compromise between the two
on every pixel.

**Expected [expected].** 1–3 % on the alt-P2 path — which is where the bits are
on `x_ep` and `t8g`, the two most expensive files in the corpus.

**Cost.** Linear in the number of filters, on the codec's hottest loop
(`walk_bank_bits` and `code_banks` are already 31 % of instructions on `t8g`).
Cheaper than it sounds if the extra filters use fewer taps.

---

## 9. A match model for long-range repetition

**Measured, with a caveat.** Coding a 320×240 photographic plane, then the same
plane again in one image:

| | coded |
|---|---|
| plane once | 52 644 |
| the same plane twice | 69 712 |

The second, *bit-identical* copy costs 17 068 bytes — 32 % of the first. An
explicit match model should code it at a few per cent.

The caveat, also **[measured]**: on strictly periodic content the existing
contexts already do most of the job. A 512×512 image made of one 64×64 random
tile repeated 8×8 costs 7 800 bytes against 4 116 for the tile alone — 1.9×, not
64×, because the context statistics converge to the tile's. So the gap is not
"BMF cannot see repetition"; it is "BMF can only see repetition that its context
memorises, and re-learns everything else from scratch".

**Change.** The standard match model: hash the last *k* coded samples, keep the
most recent position for each hash, and when a candidate match is verified
predict the sample that followed it, with a confidence that grows with the
verified match length. Feed the prediction into the mixer of §2 as one more
model rather than special-casing it.

**Expected [expected].** Near zero on photographs, large on screenshots, maps,
scanned text, textures, and anything with repeated UI or logo elements — the
content classes `f05_200` and `DLRAW` hint at but this corpus does not really
contain.

**Cost.** Small: one hash table, one comparison per pixel. It is the single
cheapest new *model* to add once §2 exists.

---

## 10. Two-pass parameter transmission

**Now [from the code].** Every model constant is compiled in — counter
increments, rescale thresholds, the SSE bank reload counters (`p2_b1_reload`),
the context quantiser edges (`p2_ctx_edges`, `p2_len_edges`,
`p1_level_edges`), the NLMS seeds and rates (`bmf_p2_coef_init`,
`bmf_p2_rate_init`), all in `bmf_tables.inc`. They are one compromise across all
image types.

**Change.** The encoder already runs the full model six or more times per plane
during the search; the information needed to tune those constants for *this*
image is a by-product it currently discards. Pick a handful with real leverage —
the quantiser edges and the adaptation rates are the obvious ones — optimise
them per image against the true coded size, and send them in a header. A few
hundred bits of side information is free at any interesting file size.

**Expected [expected].** 1–3 %, and more on content far from whatever the
constants were tuned on. Note this is *not* parameter tweaking in the sense the
brief excludes: the algorithmic change is making the parameters per-image and
transmitted rather than global and fixed.

**Cost.** Encoder-only complexity plus a header field, but the encoder gets
slower in proportion to how thoroughly it searches.

---

## 11. Things that look promising and measured as neutral

Recorded so they are not proposed again.

* **Sharing model state across planes.** Each plane allocates a fresh block and
  zeroes every counter. Coding `t24`'s three planes as three separate images
  costs 157 980 bytes; stacking them into one image so the model carries over
  costs 159 060 — **0.7 % worse [measured]**. The learning cost of a fresh model
  is already small at this image size, and the seam costs more than the warm
  start saves. Might still pay on many small planes; does not pay here.
* **Region-adaptive model selection.** A 320×480 image whose top half is a
  photograph and bottom half synthetic graphics costs 53 208 bytes as one image
  against 53 024 as two independently coded halves — **0.3 % [measured]**, and
  both halves chose the *same* model anyway. Whole-image model selection is not
  obviously leaving anything on the table for this kind of mixed content.
* **Better mode-search decisions.** The search's own estimate is within 0.4 % of
  the shipped size (§1), so there is essentially nothing to win by making the
  search smarter. Its cost model is fine; its menu is the problem.

---

## 12. Suggested order

Roughly by expected gain per unit of work, and respecting what enables what:

1. **§5 step 1** — luminance palette ordering as a *trial*. One extra trial
   encode, one bit on the wire, no risk once it is chosen rather than imposed,
   and it is the only item here with a measured win.
2. **§7 (1) and (2)** — robust fit seed, YCoCg-R candidate. Small, contained,
   and the trial machinery already exists to choose between them.
3. **§4** — counter and state representation. Independent of everything else,
   and it frees the memory §6 wants.
4. **Binarisation** — one shared bit decomposition of the residual, every model
   supplying a probability per bit. No compression gain on its own; it is the
   prerequisite for §2, §3 and §9. It replaces each model's symbol coder, so it
   *will* change the stream and cannot be checked for byte-identity; the bar is
   that it stays lossless and lands within a fraction of a per cent of the
   current sizes on every image, before anything is mixed on top of it.
5. **§3** — one APM stage. Immediately measurable once (4) exists.
6. **§2** — the mixer. The big one, and by far the most work.
7. **§9** — match model, as a new input to the mixer.
8. **§6, §8, §10** — in whatever order the measurements from (6) suggest.

---

## 13. How to evaluate any of this

The harness that kept this reconstruction honest works just as well for
measuring improvements, with one change of goal: byte-identity stops being the
success criterion and becomes the *baseline* to beat.

* **Losslessness stays absolute.** Every change must round-trip every image in
  the corpus, checked against an independent BMP decoder rather than BMF's own
  reader.
* **Report per-image, not just totals.** A mean over ten files hides a change
  that wins 8 % on photographs and loses 4 % on synthetic images. All ten
  numbers, always.
* **The corpus is too small and too photographic.** `testfiles/` has ten images
  and no screenshots, no maps, no scanned text, no rendered UI, no
  arbitrary-palette art. §5 and §9 are aimed squarely at content it does not
  contain, and neither can be judged on it. Before either is implemented the
  corpus needs those classes added.
* **Speed is part of the result.** §2 costs roughly 3× on both sides. A change
  that buys 5 % for 3× the decode time is a different product, and the trade
  should be stated in the same table as the ratios.
* **`-v` is the debugging tool.** It already prints every trial's cost and every
  choice (§5.1 of `ALGORITHMS.md`); a mixer should print its weights the same
  way, so that a model earning weight ≈ 0 everywhere shows up immediately
  instead of quietly costing time.
