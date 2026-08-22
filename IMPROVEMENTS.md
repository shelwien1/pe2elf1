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
only honest way to settle them is §15.

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

* **The mode search costs what it says it will.** Its predicted size for the
  option it *picks* lands within 0.4 % of what actually ships — `t24` 53 896.0
  estimated vs 53 904 shipped (+0.01 %), `x_ep` 331 852.0 vs 330 640 (−0.37 %).
  So the bits are not going into a mis-priced winner; they are going into the
  models it has to choose between. (This says nothing about the options it
  *rejects*: §6 (b) shows one trial mis-pricing a rejected option by 2×.)
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
proposal, and it is also the enabler for §3, §4 and §10.

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
contexts, which feeds §7.

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

## 6. Orientation: the whole dihedral group, and the trial that already exists

**Now [from the code].** The only geometric transform the encoder considers is a
transpose, applied to the whole image, all planes together
(`transpose_image(img, plane_count)` inside `search_filter`, `codec.inc:932`).
There are eight rigid orientations of a raster — identity, two mirrors, three
rotations, two diagonal flips — and seven of them are never tried. Images of
≤ 4 bpp never reach `search_filter` at all (`code_image_body`, `codec.inc:456`),
so they get no orientation trial whatsoever.

**Measured.** Every test image fed to this build in all eight orientations
(the encoder still runs its own transpose trial on each, so the figures are
already the best of what it would do with that input):

| image | shipped | identity | mirrorX | mirrorY | rot180 | transpose | rot270 | rot90 | antitrans | best |
|---|---|---|---|---|---|---|---|---|---|---|
| `t1` 1 bpp | 2 622 | 2 622 | 2 566 | 2 554 | 2 614 | 2 218 | **2 186** | **2 186** | 2 214 | **−16.6 %** |
| `t24` | 53 924 | 53 924 | 53 044 | 53 004 | 53 948 | 52 620 | **52 464** | 53 124 | 53 264 | **−2.7 %** |
| `t32` | 53 996 | 53 996 | 53 312 | 53 260 | 54 020 | **52 700** | 52 720 | 53 392 | 53 348 | **−2.4 %** |
| `DLRAW` 4 bpp | 225 784 | 225 784 | **224 128** | 224 140 | 225 720 | 225 760 | 224 632 | 224 696 | 225 644 | −0.7 % |
| `t8g` | 42 896 | 42 896 | **42 780** | 43 144 | 42 804 | 42 896 | 43 144 | **42 780** | 42 804 | −0.3 % |
| `t8p` | 43 664 | 43 664 | **43 548** | 43 912 | 43 572 | 43 664 | 43 912 | **43 548** | 43 572 | −0.3 % |
| `x_ai` | 148 780 | 148 780 | **148 564** | 148 984 | 148 684 | 148 780 | 148 984 | **148 564** | 148 684 | −0.1 % |
| `x_ep` | 330 660 | 330 660 | 330 736 | **330 468** | 330 704 | 330 660 | **330 468** | 330 736 | 330 704 | −0.1 % |
| `x_ci` | 633 144 | 633 144 | 633 100 | **632 928** | 633 148 | 633 144 | **632 928** | 633 100 | 633 148 | −0.0 % |
| `f05_200` 1 bpp | 21 286 | **21 286** | 21 538 | 21 346 | 21 442 | 22 678 | 22 882 | 22 826 | 22 658 | 0.0 % |

Three separate findings fall out, and they want three different fixes.

**(a) ≤ 4 bpp images get no orientation trial at all — `t1` loses 16.6 %.** The
short path in `code_image_body` returns before `search_filter` is called, so a
bilevel or 4-bit image is coded in whatever orientation it arrived in. `t1` is
16.6 % cheaper rotated a quarter turn; `DLRAW` 0.7 %; `f05_200` prefers the
identity, so the trial would cost it nothing. This is the largest single number
in this document and the cheapest to collect: give the short path the same
transpose trial the long path has.

**(b) The transpose trial mis-measures on multi-plane images.** For `t24` the
encoder reports the transposed cost as 106 536 bytes and rejects it — but
feeding it the transposed image shows the true cost is 52 852:

```
identity feed:   planar       53896.0 bytes
                 transposed  106536.1 bytes   (abandoned after 1 of 3 planes)
transpose feed:  planar       52852.0 bytes
```

The trial re-codes the transposed planes with the flags and coding order chosen
for the *original* orientation, and abandons the whole trial as soon as one
plane exceeds its original cost by more than 1/256. A plane whose best model in
portrait is not its best model in landscape blows that budget immediately, and
the remaining planes are never measured — the reported figure is then
`best_bits + 1` plus whatever was measured, i.e. meaningless.

The abort fires on single-plane images too — `t8g` reports "abandoned after 0 of
1 planes" — but there its verdict happens to be right on this corpus: feeding
`t8g` or `x_ai` transposed produces exactly the size the encoder already ships.
The measurable damage is on multi-plane images, where the trial *additionally*
reuses a per-plane model choice and coding order fitted to the other
orientation. **Re-running the per-plane search on the transposed image**, or at
least widening the abort budget, is worth 2.4 % on `t24` and `t32` on its own,
before any new orientation is added.

**(c) Beyond transposition, the flips add little.** On multi-plane images the
transpose captures most of the available gain (`t24`: −2.4 % of the −2.7 %;
`t32`: the transpose *is* the best). The mirrors matter for `DLRAW` and for the
≤ 4 bpp class, where they are the entire gain. So the full eight-way search is
worth having but is not where the value is; (a) and (b) are.

**Per-plane orientation [measured].** The planes of one image do prefer
different orientations — `t24`'s three planes coded standalone want identity,
mirrorY (−1.8 %) and transpose (−1.2 %) respectively. But per-plane geometry
forfeits cross-plane references, because a reference only helps if its samples
are co-located with the target's, and that is worth vastly more: `t24`'s three
planes cost 157 980 bytes coded independently against 53 924 with references.
Trading 100 KB of reference gain for 1 KB of orientation gain is not a trade.

There is one exception worth taking. The plane in **coding slot 0 has no
references by construction** — nothing is coded before it. Its reconstruction
can be un-rotated before the other planes reference it, so it can have its own
orientation for free. On `t24` that plane prefers a transpose by 1.2 %, and it
is ~98 % of the file.

**Cost.** (a) is a few lines. (b) is re-running an existing search on transposed
data — encoder time, no format change. The full dihedral search multiplies the
already-expensive mode search by four; a cheap directional statistic (compare
the entropy of horizontal against vertical first differences) can pick the two
or three orientations worth actually trialling.

---
## 7. Bigger and better-shaped context sets

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

## 8. Colour transform: fix the fit, then make it adaptive

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
4. **Make it nonlinear — but not with a fixed gamma.** The freedom that matters
   here is that **the predictor does not have to be invertible**. Only the
   residual coding must be lossless, and it is: the coder subtracts a prediction
   and codes the exact difference mod 256. So the prediction may be computed
   through any curve, at any internal precision, at no cost to losslessness — a
   gamma LUT, a per-channel curve, a fitted monotone map, anything.

   A fixed gamma is not the curve to pick, though. **[measured]**, as a
   MED-then-order-0 cost with the blend weights re-optimised for each exponent:

   | | `t24`, full plane | `x_ep`, 256×256 crop |
   |---|---|---|
   | γ = 0.6 | — | 18 243.6 |
   | **γ = 1.0 (linear, as now)** | **7 447** | **18 228.6** |
   | γ ≈ 1.7 | 7 447 | 18 289.7 |
   | γ = 2.2 | 8 223 | 20 496.5 |
   | γ ≈ 2.7 | 10 468 | 22 351.7 |

   Linear wins on both, and the direction of the loss is the physical one: 8-bit
   image data is *already* gamma-encoded, and that encoding is much of why
   inter-channel correlation is approximately linear in the coded domain.
   Undoing it moves the data away from where a linear blend works. (Caveat: part
   of the γ > 1 penalty is round-trip quantisation through the LUT this
   experiment used; a careful implementation would predict at higher precision
   and round once. Fair enough to say there is no gain to chase, not fair enough
   to put a number on the loss.)

   What is worth trying is a curve **fitted to this image's joint channel
   distribution** and transmitted — a piecewise-linear map with a dozen knots
   costs a few dozen bytes — or, more simply, letting the LMS filter of (3) see
   nonlinear features of the reference samples rather than the samples alone.

**Expected.** (1) costs nothing today on `t24` itself — the slack rule catches the
degenerate fit and `copy first ref` is genuinely the best of the four options
there. What it buys is the case the corpus does not contain: an image where the
fit degenerates *and* no degenerate form is good, where the encoder would ship a
useless transform with nothing to fall back on. Cheap insurance rather than a
measured gain. (2) and (3) **[expected]** 1–4 % on multi-plane photographic
images. (4) **[measured]** nothing for a fixed gamma; **[expected]** small for a
fitted curve, and the freedom it establishes matters more than this particular
use of it.

**Cost.** (1) and (2) are small and self-contained. (3) is a per-pixel filter on
top of the existing transform.

---

## 9. Multiple prediction filters, mixed

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

## 10. A match model for long-range repetition

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

## 11. Better cost estimation

**Now [from the code].** Every decision in `choose_plane_coding` is made on
`estimate_cost` (`planes.inc:5`) — the order-0 empirical entropy
Σ *nᵢ*·log2(*N*/*nᵢ*) of a residual histogram. No adaptation cost, no context, no
relation to the coder that will actually run. The mode search in `search_filter`
does not use it — that one trial-encodes for real — so the two halves of the
encoder judge by different currencies.

**How wrong is it? [measured]** In absolute terms, not very, for a plane like
`t24`'s plane 2:

| | bytes |
|---|---|
| order-0 entropy of the MED residual (what `estimate_cost` measures) | 55 164 |
| the same, as an adaptive order-0 code (adds the learning cost) | 55 445 |
| what the real coder achieves (`p1`, from `-v`) | 53 052 |

The estimator is within 4 % of the truth and the learning cost it ignores is
worth 0.5 %. As a *predictor of size* it is fine.

**Where it fails is ranking [measured].** It is used to choose between options
whose difference lies in structure the order-0 histogram cannot see. The alpha
plane of `x_ep` is a clean inversion — `choose_plane_coding` estimates the
three-weight cross-plane mix at 91 630.8 bytes against 201 224.6 for the best
single-channel copy, a 2.2× preference, fits weights `(-10,-1,-15)` and sets up
a reference; the real trial encodes then price that reference at 69 124 bytes
against 21 652 for coding the plane with no reference at all. The estimator did
not merely misjudge the margin, it recommended a transform the coder rejects.

**A structural inconsistency [from the code].** The four blend options are not
even scored on the same histogram. `residual_bin` (`planes.inc:133`) folds the
fitted blend's residual into **512** bins (`&0x1FF`), while the copy and average
options are binned into **1024** (`&0x3FF` in `cost_candidate`,
`codec.inc:575`). Gradient-domain second differences of two 8-bit planes span
about ±1020, so a *poor* fit wraps and has its entropy measured on merged bins —
which can only lower it. The bias flatters exactly the fits that deserve it
least. Whether it ever changes an outcome is untested; making both sides use the
same range costs nothing either way.

**And naive fixes do not automatically help [measured].** Conditioning the same
estimate on eight buckets of the previous residual's magnitude made it *worse* —
55 629 bytes against 55 164 — because eight contexts' worth of learning cost
outweighed the conditional gain at 76 800 samples. An estimator that adds
context must also count what the context costs, which a plain entropy sum
cannot.

**Change.** In rough order of effort:

1. **Score with an adaptive code, not an entropy.** Sum −log2 *p* under the same
   kind of counter the real coder uses. It counts the learning cost, so adding
   context to the estimate stops being free and the estimate stops preferring
   context sets the coder cannot afford.
2. **Match the estimator's domain to the coder's.** The candidate search works on
   gradient-domain second differences while the coder works on spatial
   residuals of the transformed planes; the two agree only approximately.
3. **Or stop estimating.** The mode search already trial-encodes, and a trial
   encode on a subsampled region — every fourth row, say — costs a fraction of a
   full pass and measures the thing that actually matters. That the encoder
   already has this machinery and does not use it for the transform decisions is
   the real oddity here.

**Expected [expected].** 0–2 % on its own; the estimator mostly gets the answer
right today. Its value is that every *other* proposal in this document is
chosen by it: a mixer, a new orientation, a new colour transform are all only as
good as the thing deciding when to use them. Fix this before adding candidates
for it to rank.

---
## 12. Two-pass parameter transmission

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

## 13. Things that look promising and measured as neutral

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

## 14. Suggested order

Roughly by expected gain per unit of work, and respecting what enables what:

1. **§6 (a)** — give the ≤ 4 bpp short path an orientation trial. A few lines
   for a **measured 16.6 %** on `t1`, and it cannot lose: a trial only adopts an
   orientation that measures cheaper.
2. **§6 (b)** — fix the transpose trial's early abort on multi-plane images.
   **Measured 2.4 %** on `t24` and `t32`, and it is a defect in an existing
   trial rather than a new feature.
3. **§5 step 1** — luminance palette ordering as a *trial*. One extra trial
   encode, one bit on the wire, no risk once it is chosen rather than imposed.
4. **§11 (1)** — score with an adaptive code rather than a plain entropy. Small,
   and everything after this is chosen by the estimator.
5. **§8 (1) and (2)** — robust fit seed, YCoCg-R candidate. Small, contained,
   and the trial machinery already exists to choose between them.
6. **§4** — counter and state representation. Independent of everything else,
   and it frees the memory §7 wants.
7. **Binarisation** — one shared bit decomposition of the residual, every model
   supplying a probability per bit. No compression gain on its own; it is the
   prerequisite for §2, §3 and §10. It replaces each model's symbol coder, so it
   *will* change the stream and cannot be checked for byte-identity; the bar is
   that it stays lossless and lands within a fraction of a per cent of the
   current sizes on every image, before anything is mixed on top of it.
8. **§3** — one APM stage. Immediately measurable once (7) exists.
9. **§2** — the mixer. The big one, and by far the most work.
10. **§10** — match model, as a new input to the mixer.
11. **§6 (c), §7, §9, §12** — the full dihedral search, hashed contexts,
    multi-rate filters, transmitted parameters, in whatever order the
    measurements from (9) suggest.

---

## 15. How to evaluate any of this

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
  arbitrary-palette art. §5 and §10 are aimed squarely at content it does not
  contain, and neither can be judged on it. Before either is implemented the
  corpus needs those classes added. §6 has the same problem from the other
  direction: its largest number, 16.6 %, rests on a single 1 bpp image, and
  `f05_200` — the only other bilevel file — gains nothing. One image is an
  anecdote; that item needs a dozen bilevel and 4-bit images before its size is
  known.
* **Speed is part of the result.** §2 costs roughly 3× on both sides. A change
  that buys 5 % for 3× the decode time is a different product, and the trade
  should be stated in the same table as the ratios.
* **`-v` is the debugging tool.** It already prints every trial's cost and every
  choice (§5.1 of `ALGORITHMS.md`); a mixer should print its weights the same
  way, so that a model earning weight ≈ 0 everywhere shows up immediately
  instead of quietly costing time.
