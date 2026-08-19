# Palettizing a bitmap — coding whole pixels as symbols

Take every distinct pixel in the image, sort them, and code the picture as a
plane of indices into that list. This document is about whether that is a good
idea, when, and how each of its three parts should be built.

It is a companion to `BMG-FORMAT.md`, and every number in it was measured on the
seven corpus files with `bmg` itself, not estimated. Section 12 says exactly how.

Four independent write-ups of the same idea were reviewed against these
measurements. Where they proposed something testable it was tested; §10 lists
what held up, what did not, and the outright errors — including two of my own,
in §3 and §5.3, which their proposals exposed.

The short answer, up front:

* **The palette costs almost nothing when it is small.** A 256-entry palette
  delta-codes to 23 bytes, a 4-entry one to a handful. The information-theoretic
  cost of a *random* set of that size is 558 bytes — real palettes beat that
  bound because they are grids, not random sets: by 24× on `t8p`, 4× on `x_ep`.
  And the way to get there is the *simplest* one: sort by the packed integer and
  delta-code that integer. Every colour transform and every space-filling sort
  offered by the four reviewed documents is worse, on every file, by 1.2× to 3×
  (§2.5).
* **The frequency table is a wash, so skip it.** A well-coded count table and an
  adaptive coder's learning cost come out within 22 bytes of each other on `t8p`
  — 237 against 259, or 0.05% of the file. And "exclude a colour once its count
  reaches zero" turns out to be adaptive coding wearing a hat: to first order the
  two cost the same thing. §3 also has the one genuinely promising use for the
  counts, which is deciding what belongs in the palette at all.
* **The ordering is the whole game.** On `t8p` the best and worst orderings
  differ by **44%** of the compressed size (44 917 against 64 835 bytes). Luma
  ordering — the obvious choice — is one of the *worst*. An optimised linear mix
  found the best one from scratch.
* **Gamma bought nothing** on this corpus, and §5.2 explains why that is not an
  accident — one of the two ways to apply it cannot change a sort order at all.
  Neither did PCA, the initialiser three of the four documents recommend: it
  lands 22–32% away from the answer on `t8p`, because the orderings that matter
  are lexicographic and PCA cannot express one (§5.5).
* **Which surrogate you optimise depends on how close the candidates are.** Over
  a wide range the residual entropy ranks orderings almost perfectly (+0.99);
  inside two percent it is close to useless (+0.01 to +0.29) and the simple
  sum of adjacency-weighted rank differences the documents propose wins instead
  (+0.56 to +0.66). §5.3.
* **The big win is speed, not size.** Coding `x_ci` as one index plane instead of
  three colour planes is 0.07% smaller but **5.9× faster to encode and 2.3×
  faster to decode**.
* **It is a loss on photographs, by a factor of two to four.** Not marginal — the
  palette alone can exceed the entire compressed file.

---

## 1. The proposal, stated exactly

Let the image have `N` pixels and `K` distinct pixel values. A pixel is the whole
cell — 24 bits of B,G,R or 32 bits of B,G,R,A — treated as one integer.

Palettizing splits the file into three independent things to code:

| part | what it is | size |
| --- | --- | --- |
| **the set** | the `K` distinct pixel values | `K` × 3 or 4 bytes raw |
| **the counts** | how often each occurs | sums to `N` |
| **the index plane** | `N` symbols from an alphabet of `K` | `N ⌈log2 K⌉` bits raw |

The set is a *set*: if the encoder and decoder agree to write it in a canonical
order, its order carries no information. Which means the ordering is not part of
the palette's cost — it is a free parameter, and choosing it well is what
sections 5 and 6 are about.

Here is the corpus, sorted by how much room the idea has to work in:

| file | pixels `N` | distinct `K` | `K/N` | bits/index | `bmg` today |
| --- | ---: | ---: | ---: | ---: | ---: |
| `x_ci.bmp` | 4 512 000 | 4 | 1/1 128 000 | 2.00 | 569 528 |
| `x_ai.bmp` | 4 512 000 | 8 | 1/564 000 | 3.00 | 149 111 |
| `t8g.bmp` | 76 800 | 256 | 1/300 | 8.00 | 44 912 |
| `t8p.bmp` | 76 800 | 256 | 1/300 | 8.00 | 45 026 |
| `x_ep.bmp` | 564 000 | 37 040 | 1/15 | 15.18 | 339 560 |
| `t24.bmp` | 76 800 | 76 312 | 0.993 | 16.22 | 53 718 |
| `t32.bmp` | 76 800 | 76 800 | 1.000 | 16.23 | 53 760 |

`x_ep` is worth a second look: 37 040 distinct RGBA pixels, but only **6 064
distinct RGB** — the alpha channel is what explodes the alphabet. Palettizing
colour and coding alpha separately is a different proposition from palettizing
the whole cell, and section 8 keeps them apart.

`t32` is the pathological end: every one of its 76 800 pixels is distinct, so the
"palette" is a relabelling of the image by itself.

---

## 2. Coding the palette

### 2.1 the sorted list is a set, and that is the whole insight

The natural way to write `K` distinct 24-bit values is `3K` bytes. That is
wasteful twice over: it pays for an order that carries no information, and it
pays full precision for values that are known to be increasing.

Send them in ascending order of the packed integer and the cost has a floor:

    log2 C(2^24, K)  bits

for a set drawn from the 24-bit universe. That is what a *random* set of that
size costs, and it is reachable by binary interpolative coding — recursively send
the median of the remaining range, which needs `log2(range)` bits and halves both
range and count. For 256 colours it is 558 bytes against 768 raw: a 27% saving
and not an exciting one.

### 2.2 what a real palette actually costs

Real palettes are not random sets. Here is the same four ways, measured:

| file | entries | raw | **delta of the packed integer** | per-channel delta | `log2 C(M,K)` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `t8g` | 256×3 | 768 | **4** | 0 | 558 |
| `t8p` | 256×3 | 768 | **23** | 39 | 558 |
| `x_ai` | 8×3 | 24 | **4** | 0 | 22 |
| `x_ep` RGB | 6 064×3 | 18 192 | **1 636** | — | 9 759 |
| `x_ep` RGBA | 37 040×4 | 148 160 | **20 060** | 26 288 | 84 570 |
| `t24` | 76 312×3 | 228 936 | **75 959** | — | 87 946 |
| `t32` | 76 800×4 | 307 200 | **105 569** | — | 165 252 |

Sort ascending, take first differences, code those with an order-0 model. On
`t8p` that is **23 bytes for a 256-entry colour palette** — 24× better than the
combinatorial bound, because the bound is for a random set and this palette is a
regular grid. `t8g`'s identity ramp costs 4 bytes because every difference is 1.

Two conclusions. First, the set-cardinality bound is an upper bound on a
well-built coder, not a target. Second, **the palette is never the reason
palettizing fails** when `K` is small — 23 bytes against a 45 000-byte file is
0.05%.

When `K` is large it is exactly the reason it fails. `t24`'s palette costs 75 959
bytes delta-coded; `bmg`'s entire output for that file is 53 718.

### 2.3 the case worth special-casing: product grids

`t8p`'s palette is not merely regular, it is a **3-3-2 colour cube**: B takes 8
values (0, 36, 72 … 252), G takes the same 8, R takes 4 (0, 85, 170, 255), and
every one of the 8·8·4 = 256 combinations is present. The index is literally
`R_level·64 + G_level·8 + B_level`.

This is extremely common — 3-3-2, the 6-6-6 web palette, the full grid of a
5-6-5 source. Detect it and the palette collapses to three axis lists:

    8 + 8 + 4 = 20 values, about 20 bytes

which is what the 23-byte delta measurement is already finding the hard way. An
explicit product-grid test costs one pass and turns a 768-byte table into three
short ones, and — more usefully — it *tells the coder the palette is a grid*,
which is exactly the structure section 5 needs.

### 2.4 what a good palette coder looks like

Delta-of-packed-integer is strong because it exploits clustering in the top
channel. It is weak because sorting by the packed value orders by R first, so
clustering in G and B is only picked up inside a fixed R.

The generalisation is to code the set as an **octree over the RGB cube**: at each
node send how the remaining count splits between the eight children, arithmetic
coded against the node's capacity. This is interpolative coding done in three
dimensions at once, it degenerates to the `log2 C(M,K)` bound on a random set,
and it captures clustering along every axis rather than one. Equivalently: sort
by Morton-interleaved bits instead of the packed integer, then interpolate.

For a palette that is a product grid, the octree finds it too — every split is
all-or-nothing — so the special case in 2.3 is an optimisation, not a
requirement.

### 2.5 the transforms everyone recommends, measured

All four reviewed documents recommend the same two things before delta-coding a
palette: a decorrelating colour transform (YCoCg-R, or green-guided residuals
`ΔG, ΔR−ΔG, ΔB−ΔG`), and — in two of them — sorting the palette along a
space-filling curve first, on the grounds that a Hilbert order keeps consecutive
palette entries close in RGB.

Both are worse than doing nothing. Order-0 cost of the residual streams, in
bytes, for every combination:

| file | `K` | sort | packed Δ | per-channel Δ | YCoCg-R Δ | green-guided Δ |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `t8p` | 256 | **packed** | **23** | 43 | 61 | 60 |
| | | Morton | — | 110 | 149 | 174 |
| | | Hilbert | — | 119 | 237 | 160 |
| `x_ep` | 37 040 | **packed** | **20 060** | 26 292 | 35 412 | 34 590 |
| | | Morton | — | 47 198 | 48 746 | 48 705 |
| | | Hilbert | — | 25 681 | 27 468 | 27 999 |
| `t24` | 76 312 | **packed** | **75 959** | 82 669 | 170 687 | 99 784 |
| | | Morton | — | 100 047 | 102 936 | 115 802 |
| | | Hilbert | — | 94 422 | 96 384 | 105 233 |
| `t32` | 76 800 | **packed** | **105 569** | 162 706 | 218 319 | 232 066 |
| | | Morton | — | 169 809 | 172 736 | 185 686 |
| | | Hilbert | — | 159 790 | 161 803 | 170 712 |

(`t8g`, `x_ai` and `x_ci` are 4 bytes under every combination and are omitted.)

The winner is the same everywhere and it is the least sophisticated entry in the
table: **sort by the packed integer, delta-code the packed integer.** Not the
channels separately, not a colour transform, not a space-filling order.

The reason the colour transforms lose is that they are solving the wrong problem.
YCoCg-R and green-guided prediction decorrelate the *channels of one pixel*, which
is what you want when coding an image. In a palette sorted by the packed value the
channels are not competing — the high channel is nearly constant along a run and
the low channel does all the moving — and a transform that mixes them destroys
exactly that structure. On `t32` it costs more than twice as much as leaving it
alone.

The reason the space-filling sorts lose is subtler and is worth stating carefully,
because one of the documents asserts that a Hilbert order **guarantees** a bounded
distance between consecutive palette entries, and puts the resulting palette cost
at "3–6 bits per colour component". Neither claim survives contact:

| file | Hilbert: mean, max consecutive L1 | packed: mean, max |
| --- | ---: | ---: |
| `t8p` | 42.8, **121** | 70.2, 589 |
| `x_ep` | 0.25, **40** | 3.3, 124 |
| `t24` | 5.4, **246** | 32.6, 506 |

A Hilbert curve through the *whole* RGB cube takes unit steps, but a palette is a
sparse *subset* of the cube, and consecutive members of a subset can be
arbitrarily far apart along the curve. Nothing is bounded. What is true is that
Hilbert order does cut the mean consecutive distance substantially — 6× on `t24`
— and it still produces a **larger** palette encoding than packed order, on every
file. And `t8p` settles the "3–6 bits per component" figure: packed delta gets
that palette down to 23 bytes for 256 colours, which is 0.24 bits per component.

The lesson is the one the whole document keeps arriving at: minimising distance
in colour space is not the objective. Here it is not even a good proxy for it.

---

## 3. Coding the frequency table — and whether to

### 3.1 the counts are only worth sending to a static coder

The counts exist to give the index coder a distribution. An adaptive coder builds
that distribution as it goes and needs no table at all. So the question is not
"how do I compress the counts" but "is a table plus a static coder cheaper than
no table plus an adaptive one".

The classical answer is that adaptive coding costs about `(K−1)/2 · log2 N` bits
more than coding with the true distribution — Rissanen's model cost, one half bit
per free parameter per doubling of the data. The question is what the table costs
against that.

A first draft of this section put the table's floor at `log2 C(N−1, K−1)`, the
number of compositions of `N` into `K` positive parts, and concluded that adaptive
coding wins outright everywhere palettizing pays. That was wrong, and one of the
reviewed documents pointed at why: the composition count is the cost under a
*uniform* prior over count vectors, and real count vectors are nowhere near
uniform. Coding each count with an order-0 model over the count values — which is
what any real implementation would do — beats it:

| file | `K` | compositions | order-0 over counts | adaptive model cost | winner |
| --- | ---: | ---: | ---: | ---: | --- |
| `x_ci` | 4 | 8 B | **1 B** | 4 B | table, by 3 B |
| `x_ai` | 8 | 18 B | **3 B** | 10 B | table, by 7 B |
| `t8g`/`t8p` | 256 | 308 B | **237 B** | 259 B | table, by 22 B |
| `x_ep` RGB | 6 064 | 6 043 B | — | 7 240 B | table |
| `x_ep` RGBA | 37 040 | 24 643 B | **13 843 B** | 44 228 B | table, by 30 385 B |

So the honest verdict is not "never send counts" but **"it is a wash where it
matters, and not worth the complexity"**: 22 bytes on a 45 000-byte file is
0.05%, and the 259-byte adaptive figure is itself an over-estimate of what an
initial histogram could recover, because `bmg`'s index coder is not a memoryless
`K`-ary model — it is a context-mixing coder whose counters are shared across
contexts and whose learning transient is not the textbook one. Where the table
would clearly pay, at `K/N` above roughly 1/100, palettizing has already lost for
other reasons.

One caveat on the last column: `(K−1)/2 · log2 N` is an asymptotic result for
`N ≫ K` and it stops meaning anything as `K` approaches `N`. On `t24` it reports
77 402 bytes of "model cost" for an image `bmg` codes in 53 718 — which is not a
statement about `bmg`, it is the formula leaving its domain. Read the table only
for the rows where a palette is plausible.

The rows also say something about the images. `t8g`, `t8p`, `x_ai` and `x_ci` have
**no singleton colours at all** — every palette entry is reused, which is what
makes a palette worth having. `x_ep` is 51% singletons and `t24` is 99.4%: at that
point the "frequency table" is mostly the sentence "everything occurs once", which
is why its composition cost collapses to 532 bytes.

### 3.2 exclusion is adaptive coding in disguise

The proposal includes dropping a colour from the alphabet once its count reaches
zero — the alphabet shrinks as the image is coded, and the last pixels are nearly
free. This is real and it works, but it is worth seeing what it *is*.

Given the counts, the exact information content of the sequence is the multinomial
coefficient

    log2 ( N! / Π n_i! )  =  N·H0 − (K−1)/2·log2 N + O(K)

and countdown coding with exclusion achieves it. The saving relative to a static
coder using the exact frequencies is precisely `(K−1)/2·log2 N` — *the same
quantity an adaptive coder pays as model cost*. Countdown coding recovers that
term and then hands it back at the door, because it needs the counts and the
counts cost `log2 C(N−1,K−1)`, which by the table above is larger.

So exclusion-with-counts and plain adaptive coding are the same thing to first
order, and on these files adaptive wins the second-order comparison. The
excluding version is worth building only if the coder is static for some other
reason.

The user-visible version of the idea — "exclude values whose count is zero" as in
*never used at all* — is different and is pure profit: it is the alphabet
reduction, it is free, and it is stronger than it looks because a palette keeps
only realized *combinations*, not realized values per channel. That is §7.1.

### 3.3 the one thing counts are good for

Half of an adaptive coder's model cost is spent in the first fraction of the
image. A **coarse** histogram — 4-bit log counts, `K/2` bytes — used to seed the
counters costs a tenth of the exact table and removes most of the learning
transient. On `t8p` that is 128 bytes to recover a good part of 259. Whether it
nets out is a measurement, not an argument, and it is the only version of "send
the frequency table" I would bother testing.

### 3.4 counts as an admission test: escape coding

There is one use for the counts that has nothing to do with entropy coding, and
it is the most promising untested idea in the reviewed documents: **only admit a
colour to the palette if it repeats**, and code the rest as escapes. A palette is
a dictionary, and a dictionary entry used once has paid for itself and no more.

| file | threshold | palette entries | palette (set bound) | escaped pixels |
| --- | --- | ---: | ---: | ---: |
| `x_ep` | all | 37 040 | 84 570 B | 0 |
| | count ≥ 2 | **18 181** | **43 844 B** | 18 859 (3.3%) |
| | count ≥ 4 | 9 650 | 24 373 B | 38 808 (6.9%) |
| | count ≥ 8 | 5 057 | 13 361 B | 62 249 (11.0%) |
| `t24` | all | 76 312 | 87 946 B | 0 |
| | count ≥ 2 | 488 | 1 007 B | 75 824 (**98.7%**) |

On `x_ep` the trade looks real: half the palette for 3.3% of pixels sent another
way. On `t24` it is decisive in the other direction — the threshold that shrinks
the palette to nothing also escapes almost every pixel, which is just a
restatement of the fact that `t24` has no reusable colours. That is the honest
diagnostic: **escape coding rescues an image only if its palette has a head, and
`t24` is all tail.**

I did not build the escape path, so `x_ep`'s 43 844 bytes is a palette cost with
no matching index-plane measurement. It is the one line item in this document
that is a design sketch rather than a result.

---

## 4. The ordering: the economics first

Sorting the palette by *some* rule and indexing by rank is where the compression
actually happens or does not. Before asking which rule, ask what each rule costs
to communicate, because that changes the answer completely.

The decoder receives the palette as a **set**. Anything it can compute from the
set alone is free.

| class | cost | examples |
| --- | --- | --- |
| **free** | 0 bits | ascending packed integer; luma with fixed weights; Hilbert index in RGB; a greedy TSP through the palette *in RGB space* — the decoder has the set, so it can rerun the same tour |
| **cheap** | a few bytes | a linear mix `w·(R,G,B,A)` with searched weights; add per-channel gammas and it is still under a dozen bytes |
| **expensive** | `log2(K!)` bits | any ordering derived from the image's own statistics — co-occurrence, spectral, a true minimum linear arrangement — because the decoder cannot recompute it before it has the image |

That last row is the same thing as sending the palette *in the order you want to
use it*: a sorted set costs `log2 C(M,K)`, an arbitrary list costs `log2 C(M,K) +
log2(K!)`, and the delta-coding of §2.2 only works on the sorted one. So the
permutation is not a separate transmission, it is what you give up by not sorting.

Measured against the file it would have to earn its keep in:

| `K` | `log2(K!)` | as bytes | share of that file's compressed size |
| ---: | ---: | ---: | ---: |
| 4 (`x_ci`) | 5 bits | <1 | 0.0001% |
| 8 (`x_ai`) | 15 bits | 2 | 0.001% |
| 256 (`t8p`) | 1 684 bits | 210 | **0.47%** |
| 6 064 (`x_ep` RGB) | 67 460 bits | 8 432 | **2.5%** |
| 37 040 (`x_ep` RGBA) | 508 720 bits | 63 590 | 18.7% |

This is worth more than a glance, because it is not what I expected before
measuring it. **An arbitrary permutation is affordable much further up than the
"expensive" label suggests** — under half a percent at 256 colours, two and a
half at six thousand. Nothing in the budget rules out sending a hand-optimised
ordering for a 256-colour image.

What section 6 shows is that the *searching* is the hard part, not the paying.
Both unconstrained orderings I built — a greedy chain and a spectral one — lost
badly to a two-parameter linear mix on `t8p`, and they would have lost even with
their side information waived. The linear family wins there not because it is
cheaper to describe but because it is small enough to search properly.

---

## 5. The ordering: what a linear mix can express

### 5.1 the family is tiny, and that is the point

The sort key `w_R·R + w_G·G + w_B·B` has two degrees of freedom (scale does not
matter). Two colours swap places exactly when `w` crosses the plane
`w·(c_i − c_j) = 0`, so the set of orderings a linear mix can produce is the set
of **cells in an arrangement of `C(K,2)` hyperplanes** — at most `O(K⁴)` of them.

For `K = 256` that is at most `m(m−1)+2 ≈ 1.1·10⁹` orderings, with `m = C(256,2)`
great circles on the sphere of directions, out of `256! ≈ 10⁵⁰⁷`. The linear
family is a restriction of staggering severity.

It is also, on this corpus, enough. Section 6 shows the optimised linear mix
reaching the best measured ordering on `t8p` and `t8g` while every unconstrained
heuristic I tried came in worse — not because the constraint helps, but because a
2-parameter space can be searched *thoroughly* and `K!` cannot.

Two consequences for the search:

* the objective is **piecewise constant** in `w`, so gradients are useless: every
  derivative is zero almost everywhere and undefined on the walls;
* candidate directions, not candidate step sizes, are what to enumerate.

And one consequence for anyone trying to enlarge the family. One of the reviewed
documents proposes a "richer projection"

    s = w0 + w_R·R + w_G·G + w_B·B + w_RG·(R−G) + w_GB·(G−B) + …

This adds **nothing**. `R−G` and `G−B` are linear combinations of `R`, `G`, `B`,
so the whole expression collapses to a plain linear mix with the weights
`(w_R + w_RG, w_G − w_RG + w_GB, w_B − w_GB)`; and `w0`, a constant added to every
key, cannot change a sort order. Sorting by `Y`, `Co`, `Cg` or any other linear
colour space is likewise already inside the family — only the *coordinates* of
the search change, never its reach. Leaving the family requires a genuinely
non-linear term, which per-channel gamma is and these are not.

### 5.2 what gamma does, and what it cannot do

Applying a gamma to the mixed key — `(w·c)^γ` — **cannot change the ordering at
all.** It is a monotone function of the key, and sorting is invariant under
monotone reparameterisation. Only *per-channel* gamma applied before the mix,

    key = w_R·(R/255)^γR + w_G·(G/255)^γG + w_B·(B/255)^γB

changes anything. That distinction is worth stating because the other version
looks equally plausible and is a guaranteed no-op.

Even the per-channel version did nothing measurable here. The search was given
three extra parameters and 400 refinement steps and returned `γ = (1,1,1)` on
`t8g` and `t8p`; on `x_ai` it found a key with a better proxy score that compressed
1 115 bytes **worse**; on `x_ep` it changed the ordering without helping.

The reason is structural. A per-channel gamma is a monotone respacing of one
axis, and it can only change the ordering by changing which channel's steps
dominate. When the palette is a uniform grid — 3-3-2, 5-6-5, an identity ramp,
which is most palettes — the levels are already evenly spaced, so respacing them
is equivalent to rescaling `w`, which the weight search covers already.

Gamma would earn its parameters on a palette whose levels are *unevenly* spaced —
an sRGB-quantised ramp, a palette built by median cut, a gamma-encoded gradient.
`bmgstat`'s "value set and tone curve" section reports exactly that spacing, per
component, with a fitted exponent and its rms. A fitted exponent far from 1.0
*with a small rms* is the case where adding gammas to the sort key is worth the
search time. On the corpus it never happened: every component of every
palette-sized file reports γ = 1.000, rms 0.00, and the one component that
reports γ = 1.76 (`t24`'s blue) has an rms of 76, meaning its 64 levels are
irregular rather than gamma-spaced.

### 5.3 what to optimise against

The objective is "minimise the compressed size of the index plane", and the only
exact way to evaluate it is to compress the index plane. That is affordable at
the end and not during a search, so the search needs a surrogate. Four candidates,
all computable from a **co-occurrence table built once** — the counts `n_ab` of
adjacent pixel pairs with colours `a` and `b` — except the last, which needs a
pass over the image:

| surrogate | | cost per candidate |
| --- | --- | --- |
| `L1` | `Σ n_ab · |π(a) − π(b)|` | `O(nnz)` |
| `log2(1+d)` | `Σ n_ab · log2(1 + |π(a) − π(b)|)` | `O(nnz)` |
| `Hpair` | order-0 entropy of `(π(a) − π(b)) mod K`, weighted by `n_ab` | `O(nnz)` |
| `Hmed` | order-0 entropy of the MED residual of the reordered index plane | `O(N)` |

The co-occurrence table is what makes any of this practical: on `x_ai` it has 64
entries against 9 million adjacent pairs, which is the difference between a
thousand-candidate search taking a second and taking an hour.

I started with `Hmed` and `Hpair` on the reasoning that the coder codes residuals,
so a residual entropy must be the closest thing to its cost. All four reviewed
documents propose `L1` or `log2(1+d)` instead. **They are right, and the
correction is not small.** Rank correlation against real `bmg` output, over every
ordering that could be afforded:

| | `x_ci` (12 orderings, 0.5% spread) | `x_ai` (40, 1.6%) | `t8p` (24, 63%) |
| --- | ---: | ---: | ---: |
| `Hmed` | +0.371 | +0.290 | **+0.989** |
| `Hpair` | +0.287 | +0.008 | **+0.980** |
| `L1` | +0.559 | **+0.663** | +0.750 |
| `log2(1+d)` | **+0.643** | +0.638 | +0.782 |

and, more usefully, what each surrogate's own top pick actually cost:

| | `x_ci` | `x_ai` | `t8p` |
| --- | ---: | ---: | ---: |
| true best | 567 497 | 148 733 | 44 917 |
| best by `Hmed` | 567 497 ✓ | 149 412 | 44 917 ✓ |
| best by `Hpair` | 567 497 ✓ | 150 374 | 44 917 ✓ |
| best by `L1` | 567 497 ✓ | **148 733 ✓** | 44 917 ✓ |
| best by `log2(1+d)` | 567 497 ✓ | **148 733 ✓** | 44 917 ✓ |

The split is clean and it is about *range*, not about which file. Where the
candidates differ by tens of percent (`t8p`) the entropy surrogates are nearly
perfect and the adjacency sums are merely good. Where they differ by one or two
percent (`x_ai`) the entropy surrogates collapse — `Hpair` at +0.008 is
indistinguishable from ranking at random — and the adjacency sums keep working
and find the optimum.

That is not mysterious. `Hmed` and `Hpair` are entropies of a histogram, and a
histogram is insensitive to *which* residuals moved: swapping two bins leaves the
entropy untouched. `L1` is sensitive to magnitude directly, and magnitude is what
a residual ladder charges for. When orderings differ grossly the histogram shape
differs grossly too and either works; when they differ finely, only the one that
counts magnitudes still sees anything.

So: **use `L1` (or `log2(1+d)`, which is marginally better on the narrowest
spread) as the search objective, and keep `Hmed` as a sanity check on the
shortlist.** Then do what `bmg` already does for its predictor choice
(`BMG-FORMAT.md` §4.9) — trial-encode the shortlist and keep the shortest. Two
bits of side information name the winner of four.

One surrogate the documents propose does **not** work: the raw count of index
transitions, `#{(x,y) : I(x,y) ≠ I(x−1,y)}`, suggested as an objective for
graphics. It is invariant under relabelling — every ordering has the same
transition count — so its correlation with output size on `x_ai` is −0.011, which
is what a constant should score. It is a useful thing to *measure* about an image
and useless as an ordering objective.

### 5.4 integer keys, and why they are not optional here

Every one of the reviewed documents writes the sort key in floating point, and
one of them notes in passing that integer arithmetic makes ties deterministic.
For `bmg` it is stronger than a nicety: the whole contract is that a stream is
byte-identical across platforms and compilers (`README.md`, "Windows"), and a
floating-point sort key breaks that the moment two colours land within a rounding
error of each other and a different FPU or a different compiler flag orders them
differently. The decoder would rebuild a different palette and decode garbage.

So the key must be integer, and the tie-break must be total:

```
key(c) = W_R·R + W_G·G + W_B·B  (+ W_A·A)      integer, exact
ties broken by the packed value of c            total, deterministic
```

The good news is that the weights can be tiny. Quantising `t8p`'s searched
`w = (0.9641, 0.2651, 0.0150)` to integers:

| weight precision | integer weights | ordering | `bmg` bytes |
| --- | --- | --- | ---: |
| 4-bit | `(15, 4, 0)` | identical to packed | 44 917 |
| 8-bit | `(255, 70, 4)` | identical to packed | 44 917 |
| 16-bit | `(65535, 18020, 1020)` | identical to packed | 44 917 |

**Four bits per weight is enough** — even with `W_B = 0`, because the tie-break on
the packed value supplies the missing least-significant ordering for free. Three
4-bit weights and a mode byte is under three bytes of side information, and the
sort becomes exact integer arithmetic with no platform dependence anywhere.

### 5.5 PCA as an initialiser: measured, and it does not work

Three of the four documents recommend starting the weight search from a principal
component — either of the colours themselves (frequency-weighted) or of the
spatial colour differences — on the reasoning that the dominant axis of variation
is the axis worth sorting along. It is cheap, it is `3×3`, and it is the first
thing anyone would try.

| file | ordering | `bmg` bytes | |
| --- | --- | ---: | --- |
| `t8p` | packed / searched linear mix | **44 917** | |
| | PCA of the colours, `w = (−0.574, 0.814, 0.091)` | 59 388 | **+32%** |
| | PCA of the spatial differences, `w = (−0.040, 0.007, 0.999)` | 54 655 | **+22%** |
| `t8g`, `x_ai`, `x_ci` | either PCA | identical to packed | — |

On the greyscale files both variants return `(0.577, 0.577, 0.577)` — the grey
axis, exactly right, because there is only one axis. On `t8p` both are badly
wrong.

The reason is worth keeping, because it generalises. The ordering `t8p` wants is
**lexicographic**: R first, then G, then B, which a linear mix expresses with
weights of wildly different magnitudes — the searched answer is a 64 : 18 : 1
ratio. PCA returns a direction of variance, and the variance of a colour cube is
roughly isotropic, so it returns weights of comparable magnitude. **A principal
component cannot be lexicographic.** And quantised colour spaces — 3-3-2, 5-6-5,
the web palette — are exactly the images where palettizing pays and exactly the
ones whose natural ordering is lexicographic.

PCA is not a bad idea in general; it is a bad idea for this. A spherical search
over 768 directions costs 0.4 seconds on `t8p` and finds the answer outright.

---

## 6. What the measurements say

### 6.1 ordering is worth 44%

`t8p`, `K = 256`. Nine orderings, each applied to the palette and the index plane
rewritten, each compressed with `bmg` — real bytes, not estimates:

| ordering | `Hpair` | `Hmed` | **`bmg` bytes** | side info |
| --- | ---: | ---: | ---: | --- |
| packed integer (B \| G<<8 \| R<<16) | 5.1059 | 4.7970 | **44 917** | free |
| optimised linear mix | 5.1059 | 4.7970 | **44 917** | 3 weights |
| optimised linear mix + gammas | 5.1059 | 4.7970 | **44 917** | 6 numbers |
| spectral (Fiedler vector of the co-occurrence graph) | 5.6634 | 5.6792 | 50 877 | permutation |
| greedy chain on co-occurrence | 6.2877 | 6.3439 | 57 209 | permutation |
| greedy nearest-neighbour tour in RGB | 6.4251 | 6.3662 | 58 847 | free |
| frequency, descending | 6.7995 | 6.8415 | 61 545 | counts |
| Hilbert curve in RGB | 6.9547 | 7.0023 | 64 782 | free |
| luma, 0.299/0.587/0.114 | 6.9136 | 6.9678 | 64 835 | free |

Worst over best is **1.44×**. Four things in that table are worth pulling out.

**Luma is the worst ordering tested.** The perceptually obvious choice loses 44%
to the packed integer. `t8p` is a 3-3-2 cube and luma projects that cube onto a
line at an angle that interleaves colours from unrelated parts of it; two
gradients that were adjacent in index space end up hundreds of ranks apart.

**Hilbert is no better.** A space-filling curve preserves *RGB* locality, and RGB
locality is not what this image has. Its structure is the authoring order.

**Frequency ordering is bad** — 61 545 against 44 917, a 37% loss — and it is bad
for a reason that generalises: sorting by count is uncorrelated with sorting by
value, so it deliberately destroys every gradient in the image. It buys a skewed
symbol distribution, which an adaptive coder was going to learn anyway, and pays
for it with the neighbour structure, which the coder cannot recover. The order-0
index entropy is 7.7224 bits/pixel under *every* ordering — reordering cannot
change it — while the MED residual entropy ranges over 4.80 to 7.00. All the
information is in the neighbour structure and frequency ordering throws it away.

**The weight search found the answer from scratch.** It returned
`w = (0.964, 0.265, 0.015)` on `(R,G,B)`. The palette has R in steps of 85, G and
B in steps of 36, so those weights give R a step of 81.9 against G's full range of
66.8, and G a step of 9.5 against B's range of 3.8 — strictly R-major, then G,
then B, which is exactly the 3-3-2 packing the file was authored in. It
reproduced the original index assignment without being told it existed, and hit
the same byte count.

That is the strongest argument for the linear-mix family: **images authored in a
quantised colour space have a lexicographic index order, and a linear mix with
well-separated weights is exactly a lexicographic order.**

`t8g` — the same image with an identity grey ramp instead of a colour cube — is
the degenerate check: packed, luma, Hilbert, greedy TSP and the optimised mix all
coincide at 44 917, because every monotone function of grey level gives the same
ranking. Frequency ordering still costs 61 545.

### 6.2 for a tiny palette, search the permutations

`x_ci` has `K = 4`: twelve orderings up to reversal, so all twelve were compressed.

| ordering | `Hpair` | `Hmed` | `bmg` bytes |
| --- | ---: | ---: | ---: |
| `[0,2,3,1]` best | 1.5085 | 1.2700 | **567 497** |
| `[0,1,2,3]` — what the file uses | 1.5549 | 1.4369 | 569 308 |
| `[2,1,0,3]` worst | 1.5549 | 1.4401 | 570 183 |

Reordering four colours is worth **1 811 bytes, 0.32%**, for one byte of side
information. The spread across all twelve is 0.47%.

`x_ai` has `K = 8`: all 20 160 orderings were ranked by `Hpair`, and 44 spanning
the range were compressed.

| | `bmg` bytes | vs. the file's own order |
| --- | ---: | ---: |
| best measured, `[3,2,0,4,5,1,7,6]` | **148 751** | −304 (−0.20%) |
| the file's own order `[0..7]` | 149 055 | — |
| best by `Hpair` | 149 076 | +21 |
| worst measured | 150 697 | +1 642 |

Two bytes of permutation for 304. Small, but as near to free as makes no
difference, and it composes with everything else.

### 6.3 the proxies are directional, not precise

The full four-surrogate comparison is §5.3; this is the entropy pair alone, which
is what the first draft of this document used throughout:

| file | Spearman(`Hpair`, bytes) | Spearman(`Hmed`, bytes) |
| --- | ---: | ---: |
| `x_ci`, all 12 orderings | +0.434 | +0.371 |
| `x_ai`, 44 orderings | +0.080 | +0.351 |
| `t8p`, 9 orderings spanning 44 917–64 835 | +0.96 | +0.96 |

Over a wide range the proxies rank orderings almost perfectly — the `t8p` table is
monotone in `Hmed` except for its last two rows, and `bmg` comes in 2.5% to 6.7%
under `Hmed × N/8` at every point on it, so the proxy is a usable *upper* bound as
well as a ranking. Inside a couple of percent the ranking is close to noise: on
`x_ai` the best-by-`Hpair` ordering was 325 bytes worse than the best measured,
and `Hpair` ranked candidates barely better than chance.

On both small files the proxy still *picked* the true winner, because the winner
was far enough ahead. That is the pattern to design for: proxy to shortlist,
coder to choose.

It also explains the one place the search actively misfired. On `x_ep` the
optimiser improved `Hpair` from 9.5438 to 9.4353 and made `Hmed` **worse**, 8.6766
to 9.1735 — it optimised a pairwise objective on an image whose structure MED
picks up and pairwise differencing does not. If only one proxy is affordable, use
`Hmed`.

### 6.4 the real prize is speed

`x_ci`, the same 4 512 000 pixels written two ways as uncompressed BMPs so the
RLE structure is out of the comparison:

| | input | `bmg` output | encode | decode |
| --- | ---: | ---: | ---: | ---: |
| 24bpp, three expanded colour planes | 13 536 054 | 569 706 | 27.8 s | 5.5 s |
| 8bpp, one index plane, `K = 4` | 4 513 078 | **569 308** | **4.7 s** | **2.4 s** |

0.07% smaller and **5.9× faster to encode, 2.3× faster to decode**. The size is
a wash because `bmg`'s colour transform already collapses `R == G == B` to one
informative plane — but it spends the time finding that out. The index path codes
one third the samples and, because the encoder's trial loop runs 2 configurations
for one plane instead of 5 for three (`BMG-FORMAT.md` §4.9), it does 2 plane-passes
where the colour path does 15.

For small images this is invisible: `t8p` takes 0.41 s either way, because
allocating and clearing 90–140 MB of model tables dominates a 76 800-pixel image.
The effect is real at scale and only at scale.

### 6.5 the same pixels, indices against colours

`t8p` is an 8-bit file with a colour palette, so `bmg` expands it and codes three
planes (`BMG-FORMAT.md` §3.1). Coding its indices instead:

| | bytes |
| --- | ---: |
| `bmg` on `t8p.bmp` — three expanded colour planes | 45 026 |
| the same indices as an 8bpp plane | 44 917 |
| `t8p`'s own palette, delta-coded (§2.2) | 23–39 |
| **palettized total** | **≈ 44 940–44 956** |

**70 to 86 bytes, about 0.17%.** Modest, and it is the honest figure for what
palettizing an already-paletted file buys: the file was already carrying the
palette, so all that changed is which alphabet the model works in. The size win
is small here; §6.4 is where this change actually pays.

### 6.6 Gray-coded indices lose

One document suggests transforming the index with a Gray code, `g = i XOR (i>>1)`,
so that adjacent indices differ in one bit — attractive if the index coder is a
bit-plane coder, and it notes the idea may hurt an arithmetic residual coder.
Measured against `bmg`, which codes residual magnitude on a ladder rather than
bit planes:

| file | plain | Gray-coded | |
| --- | ---: | ---: | ---: |
| `t8g`, `t8p` | 44 917 | 57 598 | **+28%** |
| `x_ai` | 149 055 | 150 506 | +1.0% |
| `x_ci` | 569 308 | **568 032** | −0.22% |

A heavy loss where the alphabet is large, a small loss at `K = 8`, and a small
*win* at `K = 4` — where a two-bit index is close to being a bit-plane
representation already and there is almost nothing for a magnitude coder to
exploit. The caution in the source was the right one, and the answer for a
magnitude-oriented coder is no.

### 6.7 and the negative control

For the photographic and near-injective files, the index plane's order-0 MED cost
plus the palette, against what `bmg` produces today:

| file | index MED | index bytes | + palette | total | `bmg` today | ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `t24` | 12.82 b/px | 123 036 | 75 959 | 198 995 | 53 718 | **3.7×** |
| `t32` | 7.12 b/px | 68 381 | 105 569 | 173 950 | 53 760 | **3.2×** |
| `x_ep` RGBA | 8.68 b/px | 611 704 | 20 060 | 631 764 | 339 560 | **1.9×** |
| `x_ep` RGB only | 8.15 b/px | 574 335 | 1 636 | 575 971 + alpha | 339 560 | **≥1.7×** |

Not close. On `t32` the palette alone — 105 569 bytes — is twice the entire
compressed file. There is no ordering, no gamma and no clever set coder that
rescues this; the arithmetic is decided before any of them are consulted.

---

## 7. What palettizing buys, and what it costs

### 7.1 the benefits, in the order they matter

**Speed, by the plane count and the trial count.** Section 6.4: 5.9× encode,
2.3× decode. One plane instead of three or four, and — because so many of
`bmg`'s encoder-side searches are per-plane — a smaller trial loop as well.

**The joint alphabet, not the marginal one.** A palette contains only realized
combinations. That is a much stronger statement than per-component alphabet
reduction: `x_ep`'s components use 32, 64, 32 and 256 values (65 536 combinations
if independent) and only 6 064 RGB combinations occur. `bmgstat` reports the
per-component version in its "value set" section and the joint version in
"unique pixels"; palettizing captures the second for free.

**Exact contexts instead of hashed ones.** `bmg` carries two models keyed on the
exact 4- and 8-pixel neighbourhood (`bmg_model.inc` M9 and MA), and on colour
planes those keys must be hashed because the space is `2^32` wide. Over a `K = 8`
alphabet a 4-neighbour context is 4 096 states — a dense table with no collisions
and no wasted counters. This is the benefit most likely to be underestimated:
`bmg` spent real effort on hash collisions (`BMG-FORMAT.md` §4.5) that a small
index alphabet makes disappear.

**Everything cross-plane stops existing.** No colour transform to search, no
plane-order decision, no per-plane predictor trial, no cross-plane context model.
Whatever redundancy those were recovering is already gone — including any exact
functional dependency between components, which `bmgstat`'s derivability section
measures and which a palette removes by construction.

**Match models get whole-pixel symbols.** A repeated colour is a repeated symbol
rather than three separately-matching bytes, so a match model's hit is
unambiguous. On screenshots and UI captures this is most of the file.

**Approximate equality becomes exact equality.** This is the benefit I
under-weighted and one of the reviewed documents puts near the top, rightly. Two
regions with the same shape but different brightness — a gradient at 100…104 and
the same gradient at 150…154 — share no bytes at all in RGB, so no match model and
no LZ can connect them. Under a palette ordered along that gradient both become
some `n … n+4`, and their *residual* sequences become identical. The palette is a
structural normalisation layer: it converts similarity into equality, which is the
only form of similarity a match model can use. `bmg` has a match model
(`bmg_model.inc` M7) that would see this and currently cannot.

**The palette is often already in the file.** For 8-bit BMPs the palette is
carried anyway, and a byte-exact coder has to reproduce it either way — so the
side information is not side information at all.

### 7.2 the costs, in the order they hurt

**Prediction, and this is the one that decides it.** A palette index is a point
on a line; a colour is a point in a cube. Any bijection from an `n×n×n` grid to a
line stretches some grid-adjacent pair to a line distance of order `n²` — no
ordering avoids it, it is a property of the dimensions. A 1-D order can follow one
direction of a 3-D gradient and must break the other two.

This is not theoretical: it is exactly why luma costs 44% on `t8p`. The image has
gradients along all three axes of its cube, luma follows a diagonal, and the two
transverse directions are shredded. The packed order wins because the image was
*authored* along it — and it follows that the best ordering is usually a property
of the file's provenance, not of colour science.

**The palette, once `K` is large.** Section 6.6. It grows as `K` while the index
plane shrinks only as `log K`, so the trade turns over fast: at `K/N = 1/15`
(`x_ep` RGBA) the palette is 6% of the compressed file and the index plane is
already twice too expensive; at `K ≈ N` (`t24`, `t32`) the palette alone is
twice the whole compressed file.

**Wide indices.** Beyond 256 colours the index does not fit in a byte, and a coder
built for byte planes — `bmg` is one — cannot code the plane at all without being
extended. The `x_ep` RGB case (6 064 colours, 12.57 bits) is the interesting one
and it is the one that needs the most new code.

**Sub-byte structure is gone.** `bmg` codes a residual as a ladder of binary
decisions with contexts that mean something about magnitude. On indices those bits
mean whatever the ordering made them mean, which is why a *good* ordering matters
so much more here than a good ordering of anything else in the coder.

---

## 8. Where it pays

From the measurements, as a rule with the corpus attached:

| regime | verdict | corpus |
| --- | --- | --- |
| `K ≤ 16` | **always** — palette a few bytes, a permutation under 6, enumerate the orderings | `x_ci` (4), `x_ai` (8) |
| `16 < K ≤ 256` | **usually** — palette tens of bytes; a permutation is affordable (0.47%) but nothing I built could find a good one, so: searched linear mix | `t8g`, `t8p` (256) |
| `256 < K`, `K/N < 1/100` | **worth trying** — needs a >8-bit index coder; palette still small next to the plane | `x_ep` RGB (6 064, 1/93) |
| `K/N > 1/10` | **no** — the palette approaches the file | `x_ep` RGBA (1/15), `t24`, `t32` |

Two of the reviewed documents give a numerical threshold for abandoning the
scheme: one says disable palette mode when `K/N > 0.3`, the other calls the method
"optimal when `N < H·W/3`" — the same figure. **Both are far too generous.** At
`K/N = 1/15` (`x_ep` as RGBA, well inside their accept region) palettizing already
loses to `bmg` by 1.9×, and at `1/93` (`x_ep` as RGB) it still loses by 1.7×. The
usable threshold on this corpus is nearer `K/N = 1/100`, thirty times stricter
than published. A rule of thumb that accepts `t24` — where the palette alone is
1.4× the entire compressed file — is not a rule worth having.

Two refinements the corpus argues for.

**Palettize colour, not the whole cell, when there is an alpha channel.**
`x_ep`'s 37 040 RGBA pixels are 6 064 RGB colours and an alpha plane; the RGBA
palette costs 20 060 bytes and the RGB one 1 636. Alpha is a different kind of
signal — `bmgstat` reports it as "a real gradient" here, with 77.5% of pixels
fully opaque — and it wants its own plane and its own model, not a seat in a
joint alphabet that it multiplies by six.

One document proposes a middle course: keep alpha in the palette but sort by an
alpha *class* (`A = 0`, `A = 1…254`, `A = 255`) and then by RGB inside each class,
so that unrelated transparent colours do not get dragged into the colour ordering.
Measured on `x_ep`, that is worse than doing nothing:

| ordering of the RGBA palette | `Hmed` | index plane |
| --- | ---: | ---: |
| packed integer — which is already alpha-major | **8.6766** | **611 704 B** |
| alpha class {0, mid, 255}, then RGB | 9.1484 | 644 961 B |
| RGB-major, alpha in the low bits | 10.7693 | 759 236 B |

The proposal is half right and already done: the packed integer puts alpha in the
top byte, so sorting by it *is* "alpha first, then colour", and it is the best of
the three. Coarsening alpha into three classes throws away ordering information
and costs 5%. What actually helps is taking alpha out of the palette altogether.

**Do not decide by rule.** Every threshold above is a shortlisting heuristic. The
proxies rank orderings well over wide ranges and badly over narrow ones (§6.3),
and the same is true of the palettize-or-not decision itself. `bmg` already
settles its predictor choice by encoding both ways and keeping the shorter
(`BMG-FORMAT.md` §4.9); this is the same shape of decision and deserves the same
treatment.

---

## 9. What I would build

In order of return on effort:

1. **Code an 8-bit paletted image as indices whenever `K ≤ 256`, not only when
   the palette is a monotone grey ramp.** `bmg` today takes the index path only
   for grey ramps (`Codec::gray_monotone`) and expands everything else. Extending
   it to any small palette is a small change and it is where the speed in §6.4
   lives. Worth 70–86 bytes on `t8p` and 5.9× encode time on files like `x_ci`.

2. **Search the palette order.** Free orderings first — packed integer, luma,
   Hilbert, greedy RGB tour — then a linear mix with **integer** weights (§5.4)
   optimised against `L1` from the co-occurrence table (§5.3). Shortlist four,
   trial-encode them, send two bits and three 4-bit weights. On `t8p` the search
   reaches the best ordering; on `x_ci` and `x_ai` an exhaustive permutation
   search is affordable and finds 0.20–0.32%.

   The open question this document did not answer: at `K = 256` an arbitrary
   permutation costs 0.47% (§4), and the best free ordering happened to be the
   authoring order, so nothing was left on the table for `t8p`. Whether a real
   minimum-linear-arrangement solver — as opposed to the greedy chain and the
   spectral relaxation tried here, which lost by 27% and 13% — can beat the
   authoring order on a file that does not have one is worth finding out. The
   budget allows it; only the search was missing.

3. **Delta-code the palette, with a product-grid special case.** 23 bytes for 256
   entries. Twenty lines.

4. **Do not send counts.** Section 3. If anything, a 4-bit log histogram to seed
   the counters, and only after measuring.

5. **Only then**, if a >8-bit index plane is wanted, the `x_ep`-RGB case: 6 064
   colours, alpha kept separate, index split across a nibble-and-byte pair of
   context models rather than two byte planes.

What I would *not* build, all of it measured rather than assumed: a colour
transform or a space-filling sort before delta-coding the palette (§2.5, worse on
every file); PCA as the weight initialiser (§5.5, 22–32% off on `t8p`); Gray-coded
indices (§6.6, +28% where the alphabet is large); an alpha-class hierarchy in the
sort key (§8, 5% worse than the packed order it was meant to improve); and gamma
in the sort key, until a file turns up whose palette has genuinely uneven level
spacing. `bmgstat`'s tone-curve section is the
test, and it reports γ = 1.000 with rms 0.00 for every component of every file
where palettizing is in play. The one exception is the blue component of `t24`
and `t32` — 64 values, γ = 1.76 — and its rms of 76 says those levels are not on
a power law either, just irregular. They are also the two files a palette cannot
help under any circumstances.

### 9.1 the three ideas worth building that this document did not test

Three proposals from the reviewed documents are, I think, better than anything
measured here, and all three are too large to have tested in passing. They are
listed as open work, not as results.

**Predict in colour space, then map back to the palette.** The costliest thing
about palettizing is §7.2: a 1-D order cannot preserve 3-D locality, so prediction
gets worse. The proposal sidesteps it entirely. The decoder holds the palette, so
it knows the *colour* of every neighbour — it can run MED in RGB, exactly as a
non-palettized coder does, arrive at a predicted colour `Ĉ`, then binary-search
the sorted palette around `Ĉ` and code the *rank of the true colour among the
candidates near the prediction* rather than a numeric index residual. Prediction
happens in the space where it works; only the symbol is an index. It also makes
the ordering much less critical, since the ordering is then only being used as a
search structure. This is the single strongest idea in the four documents and it
is the one I would build first if the goal were a new codec rather than an
addition to `bmg`.

**Tile-local orderings over one global palette.** A global ordering is one
compromise across an entire image, and §6.1's 44% spread says the compromise is
expensive. Keep one global palette — colours stay deduplicated — and give each
tile its own permutation of the subset it uses. A tile using 500 of 100 000
colours gets 9-bit indices instead of 17, and an ordering fitted to its own
gradients. The cost is per-tile metadata, and the tile size has to be searched.

**Escape coding for the tail.** §3.4 has the numbers and no codec behind them.

---

## 10. The reviewed documents: what held up

Four write-ups of this idea were checked against the measurements. This is the
audit, because "look out for errors" was the brief and because several of the
proposals are right and worth adopting.

### 10.1 right, and adopted

* **`L1` / `log2(1+d)` over the co-occurrence table as the search objective.**
  All four propose it; all four are correct and I was not. It beats the residual
  entropies decisively wherever the candidates are close together (§5.3).
* **Build the co-occurrence table once and search on it**, rather than
  re-scanning the image per candidate. Independently arrived at here; it is what
  makes a thousand-candidate search cost a second.
* **The objective must be the coder's cost, not colour distance** — stated
  plainly in three of the four, and every measurement in §6 agrees.
* **Do not transmit a frequency table to an adaptive coder.** Unanimous, correct.
* **Integer keys with a deterministic tie-break** (one document). For a codec
  whose contract is byte-identical streams across platforms this is not a detail
  (§5.4).
* **Counts coded as an order-0 model over count values**, which is what exposed
  the error in §3.1 below.
* **Escape colours, hierarchical and tile-local palettes, and colour-space
  prediction with palette lookup** — the three best untested ideas, §9.1.
* **`log2 K` is an alphabet bound and says nothing about entropy**; **`K ≈ N`
  is the failure mode**; **the encoder should decide by trying both modes**. All
  correct, all confirmed.

### 10.2 wrong, and refuted by measurement

* **"Apply a reversible colour transform before delta-coding the palette."**
  Proposed by all four. Worse than plain packed-integer deltas on every file,
  by 1.2× to 3× (§2.5).
* **"Sorting the palette by a Hilbert curve *guarantees* consecutive entries are
  close, bounding palette storage to 3–6 bits per component."** The guarantee is
  false — a palette is a sparse subset of the cube and consecutive members can be
  arbitrarily far apart; measured maxima are 121, 40 and 246 (§2.5). The 3–6 bit
  figure is beaten by a factor of 20 by packed deltas, which reach 0.24 bits per
  component on `t8p`.
* **"Disable palette mode when `K/N > 0.3`" / "optimal when `N < H·W/3`."** Thirty
  times too generous; the usable threshold here is nearer `1/100` (§8).
* **PCA as the weight initialiser**, in three of the four. 22–32% off on `t8p`,
  because a principal component cannot express a lexicographic ordering and
  quantised colour spaces are lexicographic (§5.5).
* **"A richer projection `w0 + w_R·R + … + w_RG·(R−G) + w_GB·(G−B)`."** Adds no
  expressive power whatsoever — those terms are linear in `R, G, B` and the
  constant cannot change a sort order (§5.1).
* **Gray-coded indices** as a general improvement. +28% on the 256-colour files
  (§6.6); the source's own caveat was the accurate part.
* **Alpha-class hierarchical ordering.** 5% worse than the packed order it was
  meant to improve, which is already alpha-major (§8).
* **Index-transition count as an ordering objective.** It is invariant under
  relabelling, so it cannot rank orderings at all: measured correlation −0.011
  (§5.3).
* **"Negative weights make the search space larger."** Only by a factor of two,
  and that factor is free: `w` and `−w` give reversed orderings with identical
  residual magnitudes, so the hemisphere suffices. Negative *relative* signs
  between channels are a different matter and are genuinely useful — the searched
  optimum for `x_ep` has two.
* **"A grid search over 100–400 spherical points is guaranteed to find the global
  optimum within the search resolution."** The guarantee is vacuous: the objective
  is piecewise constant over `O(K⁴)` cells — about `10⁹` at `K = 256` (§5.1) — so
  400 samples touch a vanishing fraction of them, and "within the search
  resolution" is doing all the work in that sentence.

### 10.3 not evidence

One document reports a benchmark table — "`64×64`, `N = 4088` unique colours",
optimised weights `(0.6, 0.9, 0.3)`, "4.16% overall bitstream reduction". Its own
listed code generates the test image with `np.random.randint`, i.e. uniform noise,
in which 4 088 of 4 096 pixels are unique and no ordering can help anything. The
numbers are not measurements of the effect they are labelled with. The general
claim they illustrate — that tuned weights beat BT.709 luma — happens to be true,
and §6.1 measures it properly at 44% rather than 4%.

### 10.4 two errors of my own that the review exposed

* **§3.1 originally claimed the frequency table's floor is `log2 C(N−1, K−1)`**
  and concluded that adaptive coding beats it outright. That bound assumes a
  uniform prior over count vectors; an order-0 model over the count *values* is
  substantially cheaper — 237 bytes against 308 on `t8p` — which turns a clear win
  into a wash. The recommendation survives, the argument did not.
* **§5.3 originally recommended `Hmed` as the search objective.** On the files
  where the decision is close, it is barely better than random (§5.3). The
  documents' `L1` is the right choice and it was in front of me.

---

## 11. bmppal: the tool

`bmppal.cpp` is this document as a program. It performs the split of §1 and
nothing else — it does not compress the index image, because that is the coder's
job and the separation is the whole point.

```sh
./mk.sh pal
bmppal c input.bmp out.bmp out.pal [out.frq]
bmppal d out.bmp out.pal [out.frq] restored.bmp
```

| | |
| --- | --- |
| `out.bmp` | the index image, as a plain BMP: 8bpp with a grey ramp while the palette fits in a byte, 24 or 32bpp packed little-endian above that. A ramp of exactly `K` monotone entries, which is what makes `bmg` take its index path (`BMG-FORMAT.md` §3.1) rather than expanding to colour planes. |
| `out.pal` | the palette, the ordering, and the container record. Range coded with `bmg_rc.inc`. Carries the occurrence table too unless `out.frq` is named. |
| `out.frq` | the occurrence table, uncompressed: one little-endian `uint32` per palette entry, in index order. |

**`bmppal d` reproduces the input byte for byte**, not merely pixel for pixel:
header, palette table, row padding, RLE run structure, gaps and trailing bytes.
That contract is what the container record in `out.pal` is for, and it is what
makes the tool comparable with `bmg` rather than with a pixel dumper.

Which of this document it implements:

| | |
| --- | --- |
| §2.1, §2.5 | the palette goes out as a set, ascending, delta-coded on the packed integer — no colour transform, no space-filling sort, because those measured worse |
| §3.2 | the counts are *used*: a pixel's index is its colour's rank among the colours not yet spent, so a colour leaves the alphabet when its count reaches zero. §11.2 |
| §4 | the canonical order is free; anything else costs what it costs |
| §5.3 | the search objective is `L1`, the adjacency-weighted sum of rank differences, over a co-occurrence table built once |
| §5.4 | integer sort keys with a total tie-break on the packed value — verified below to give byte-identical streams on Linux and Windows |
| §6.2 | palettes of eight or fewer are ordered by exhaustive permutation search, and the permutation is transmitted |

A paletted input is handled without decoding it: the symbol is the **stored index
byte**, not the colour, so two palette slots holding the same colour stay two
symbols and the file's own bytes come back. For an RLE8 input that means the run
structure is never touched — only the value bytes inside it are remapped — so
`out.bmp` is still RLE8 and still byte-exactly reversible.

### 11.1 what it does to the corpus

`out.bmp` compressed with `bmg`, plus `out.pal`, against `bmg` on the original.
Every row round-trips through the whole pipeline — `bmppal c`, `bmg c`, `bmg d`,
`bmppal d` — back to the input byte for byte:

| file | `bmg` on the original | `bmg` on `out.bmp` | `out.pal` | total | |
| --- | ---: | ---: | ---: | ---: | ---: |
| `x_ci` | 569 528 | 567 674 | 71 | **567 745** | **−0.31%** |
| `x_ai` | 149 111 | 148 766 | 87 | **148 853** | **−0.17%** |
| `t8p` | 45 026 | 44 967 | 387 | 45 354 | +0.73% |
| `t8g` | 44 912 | 44 967 | 329 | 45 296 | +0.86% |
| `x_ep` | 339 560 | 526 709 | 36 568 | 563 277 | +66% |
| `t24` | 53 718 | 122 422 | 85 281 | 207 703 | +287% |
| `t32` | 53 760 | 70 242 | 170 862 | 241 104 | +348% |

The verdict is §8's, arrived at independently: **it wins on the two files with
tiny palettes and loses on everything else**, and the losses on the last three
match §6.7's proxy prediction of 1.9× / 3.7× / 3.2× at 1.7× / 3.9× / 4.5×.

The ordering search behaves as §6 said it would. On `x_ai` and `x_ci` it takes
the exhaustive permutation, improving `L1` by 28% and 27% and the real output by
0.22% and 0.31% — which is also a reminder that the surrogate is directional and
not proportional. On `t8p` it tries 2 017 integer weight vectors and confirms the
packed order, exactly as §6.1's spherical search did. On the large-`K` files it
does not search at all, by design.

### 11.2 the countdown, and what makes the occurrence table earn its place

The first version of the tool transmitted the counts and never read them. That is
the worst of both worlds — the bytes are spent and nothing is bought — and §3.2
already says what to do about it: **strike a symbol out of the alphabet when its
count runs down to zero**, so the tail of the image codes against a smaller and
smaller alphabet and the last stretch, where one colour is all that is left, is
free.

So a pixel's index is not a fixed number per colour. It is the colour's **rank
among the symbols still alive**, which a Fenwick tree over the live flags answers
and updates in `log K`. The transform is a bijection given the counts — the file
still comes back byte for byte — and it is monotone in the static order, so a
rank only ever drifts downwards and the spatial structure the ordering search
worked for is not scrambled. The decoder now *needs* the occurrence table, which
is the point: it stopped being a note about the image and became half the code.

Two useful properties fell out. A run in an RLE8 stream stays a run, because only
the run's own symbol is being spent and it cannot die before its last pixel, so
the rank cannot move inside a run. And the encoder reports how much alphabet the
coder actually saw, which is the whole of what the exclusion can be worth:

| file | mean alphabet | `bmg` on `out.bmp` with `-x` | with the countdown | |
| --- | ---: | ---: | ---: | ---: |
| `t24` | 50.0% of `K` | 133 920 | **122 422** | **−8.6%** |
| `x_ep` | 59.6% | 534 379 | **526 709** | **−1.4%** |
| `t8g`, `t8p` | 84.1% | 44 912 | 44 967 | +0.1% |
| `x_ai` | 99.9% | 148 699 | 148 766 | +0.05% |
| `x_ci` | 100.0% | 567 716 | **567 674** | −0.01% |
| `t32` | 50.0% | 66 853 | 70 242 | +5.1% |

The diagnostic predicts the sign everywhere except `t32`. Where the alphabet
never collapses — `x_ai` and `x_ci`, whose four and eight colours each occur
hundreds of thousands of times — there is nothing to exclude and the transform is
a no-op that costs a rounding error. Where half the palette dies during the
image, it is worth several percent. `t32` is the exception and an instructive
one: every one of its 76 800 colours is unique, so the alphabet halves, but its
static index already correlates with position (§6.7 noted the same anomaly), and
subtracting the number of already-spent smaller keys destroys more structure than
the shrinking alphabet returns.

Whether it is on by default turns on a question the command line has already
answered. **The occurrence table is transmitted either way** — that is what the
tool is specified to do — so the baseline is "sent and ignored", and against that
baseline the countdown is a win on three files, a wash on three, and a loss on
one:

| file | counts sent and ignored | counts driving the countdown | |
| --- | ---: | ---: | ---: |
| `t24` | 219 202 | **207 703** | **−5.25%** |
| `x_ep` | 570 947 | **563 277** | **−1.34%** |
| `x_ci` | 567 786 | **567 745** | −0.01% |
| `x_ai` | 148 786 | 148 853 | +0.05% |
| `t8g` | 45 241 | 45 296 | +0.12% |
| `t8p` | 45 298 | 45 354 | +0.12% |
| `t32` | 237 715 | 241 104 | +1.43% |

So it is on by default, and `-x` turns it off.

The separate question — whether to send the counts at all — is §3's, and the
answer there has not changed. Priced against *not sending them*, the countdown
only pays where the counts are cheap as well as useful, which on this corpus is
`t24` alone: its table costs 676 bytes because 99.4% of its colours are
singletons and a table that says "everything occurs once" is nearly free, and the
exclusion returns 11 498. `x_ep` has the alphabet collapse but a 13 551-byte
table, so against a no-counts baseline it loses 1.05%. That is §3.2's trade
exactly, now measured against a real coder instead of argued from Stirling.

### 11.3 the container cost, which §6.5 left out

`t8g` and `t8p` are the interesting failures, and they correct this document.

§6.5 estimated that palettizing `t8p` would **gain** 70–86 bytes. The tool
**loses 272**. The estimate priced the colour set at 23–39 bytes (§2.2) and
stopped there. Two things it did not price:

* **the file's own palette table.** `t8p` stores 1 024 bytes of colour table that
  a byte-exact transform must reproduce. Coded inline by `bmg` it is nearly free,
  because the surrounding model has context for it; coded in a separate `.pal`
  stream with nothing around it, it costs about 58 bytes. Splitting a file into
  independent streams costs whatever the streams could have told each other.
* **the occurrence table.** The tool always carries it, because that is what it
  is for. On `t8g` it is **278 of the 328 bytes** — five times the rest of the
  palette put together. Nothing in the restore needs it.

So the corrected figure for "palettize an already-paletted file": the index
coding wins 114 bytes on `t8p`, the palette table costs about 58, and the
occurrence table costs 278 more if you keep it. Take the counts away and the
split is roughly break-even; keep them and it loses. §6.5's number was the
pixels; this is the file.

### 11.4 verification

* Round-trip on the seven corpus files and twelve synthetic edge cases, with the
  countdown on and off and with and without `out.frq` — 1×1,
  single-colour, top-down, `BI_BITFIELDS`, a gap before the pixels, trailing
  bytes, non-zero row padding, a palette with a duplicated colour and junk in the
  unused slots, `K = 1`, RLE8 with EOL/delta/absolute runs, and a `K` large
  enough to force the 24-bit index path — all byte exact, 76 combinations.
* Clean under ASan and UBSan on all of the above.
* Fuzzed both directions under the sanitizers, countdown on and off: mutated
  bitmaps into `c`, where every file accepted has to round-trip exactly — 1 333
  accepted, all exact, 1 167 refused — and corrupted
  `out.bmp`/`out.pal`/`out.frq` into `d`, where nothing may crash — 174
  completed, 2 326 refused, no crashes. An earlier 10 000-run pass over the
  pre-countdown build found two real defects: a corrupt `.pal` could index the
  palette out of bounds, and a malformed input whose pixel bytes point past its
  own palette was accepted by the encoder and then refused by the decoder. Both
  are fixed; the second is now refused up front, with the offending index named.
  The countdown adds a third kind of refusal that costs nothing to get right —
  a rank that names a symbol which is already spent, or an occurrence table the
  index image does not use up exactly, is rejected rather than decoded into
  something plausible.
* Cross-built with MinGW-w64 and run under Wine: `out.bmp`, `out.pal` and
  `out.frq` are **byte-identical** to the Linux build's on every test file, and
  each platform restores the other's output exactly. That is §5.4 earning its
  keep — a floating-point sort key would have made this fail on the first tie.

---

## 12. How this was measured

`bmg` at commit-time, release build, on the seven corpus files. Everything here
is reproducible — the scripts are in `palexp/` and need only numpy:

```sh
./mk.sh release
python3 palexp/run.py   testfiles/t8p.bmp   # §2.2, §3, §6.1: orderings, real bytes
python3 palexp/exact.py testfiles/x_ci.bmp  # §6.2: every ordering, real bytes
python3 palexp/surro.py                     # §5.3: which surrogate predicts the coder
python3 palexp/review.py                    # §2.5, §3.4, §5.4, §5.5, §6.6, §8, §10
python3 palexp/big.py   testfiles/t24.bmp   # §6.7: the negative control
```

* Palettes, counts, co-occurrence tables, orderings and the entropy proxies are
  computed in Python over the images decoded from the BMPs, including the RLE8
  ones.
* **Real byte counts** come from running `bmg c` on a synthetic 8bpp BMP with an
  identity grey-ramp palette whose pixels are the reordered indices. That keeps
  the palette bytes constant across orderings so every difference in output is
  the index plane, and it makes `bmg` take its index path rather than expanding.
  Every such file was decoded again and compared with the original; all
  round-tripped.
* Because those synthetic files are uncompressed while `x_ai` and `x_ci` are
  RLE8, their absolute sizes differ slightly from `README.md`'s table — 569 308
  against 569 528 for `x_ci`. Comparisons in this document are always between two
  synthetic files, never between a synthetic one and the corpus original.
* The weight search used 768 directions on the hemisphere from a spherical
  Fibonacci lattice plus 400 shrinking local steps, scored on `Hpair` from the
  co-occurrence table.
* `x_ci`'s twelve orderings were all compressed. `x_ai`'s 20 160 orderings were
  all scored on `Hpair`; 44 spanning the range were compressed.
* The things measured with a proxy alone are the large-`K` control in §6.7 and
  the alpha-ordering table in §8 — `bmg` cannot code a 16-bit index plane, so
  those rows are order-0 MED entropy, which §6.3 shows `bmg` beats by 2.5–6.7%.
  Even at that discount the §6.7 verdict is a factor of two.
* §3.4's escape-coding table is a palette cost with no index-plane measurement
  behind it; it is the one design sketch in the document.
* The four reviewed write-ups are not in the repository. §10 quotes their claims
  closely enough to check without them, and `palexp/review.py` reproduces every
  number that refutes or confirms one.
