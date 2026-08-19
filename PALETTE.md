# Palettizing a bitmap — coding whole pixels as symbols

Take every distinct pixel in the image, sort them, and code the picture as a
plane of indices into that list. This document is about whether that is a good
idea, when, and how each of its three parts should be built.

It is a companion to `BMG-FORMAT.md`, and every number in it was measured on the
seven corpus files with `bmg` itself, not estimated. Section 10 says exactly how.

The short answer, up front:

* **The palette costs almost nothing when it is small.** A 256-entry palette
  delta-codes to 23 bytes, a 4-entry one to a handful. The information-theoretic
  cost of a *random* set of that size is 558 bytes — real palettes beat that
  bound because they are grids, not random sets: by 24× on `t8p`, 4× on `x_ep`.
* **Do not send the frequency table.** For every image where palettizing pays at
  all, letting an adaptive coder learn the counts is *cheaper* than transmitting
  them — 259 bytes against 308 on `t8p`. And "exclude a colour once its count
  reaches zero" turns out to be adaptive coding wearing a hat: to first order the
  two cost the same thing, and the countdown version also has to pay for the
  counts.
* **The ordering is the whole game.** On `t8p` the best and worst orderings
  differ by **44%** of the compressed size (44 917 against 64 835 bytes). Luma
  ordering — the obvious choice — is one of the *worst*. An optimised linear mix
  found the best one from scratch.
* **Gamma bought nothing** on this corpus, and §5.2 explains why that is not an
  accident — one of the two ways to apply it cannot change a sort order at all.
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
| `x_ep` RGBA | 37 040×4 | 148 160 | **19 856** | 26 288 | 84 570 |
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

---

## 3. Coding the frequency table — and why not to

### 3.1 the counts are only worth sending to a static coder

The counts exist to give the index coder a distribution. An adaptive coder builds
that distribution as it goes and needs no table at all. So the question is not
"how do I compress the counts" but "is a table plus a static coder cheaper than
no table plus an adaptive one".

The classical answer is that adaptive coding costs about `(K−1)/2 · log2 N` bits
more than coding with the true distribution — Rissanen's model cost, one half bit
per free parameter per doubling of the data. An explicit table of `K` positive
counts summing to `N` costs at least `log2 C(N−1, K−1)` bits. Measured:

| file | `K` | count table ≥ | adaptive model cost ≈ | winner |
| --- | ---: | ---: | ---: | --- |
| `x_ci` | 4 | 8 B | 4 B | adaptive, by 4 B |
| `x_ai` | 8 | 18 B | 10 B | adaptive, by 8 B |
| `t8g`/`t8p` | 256 | 308 B | 259 B | adaptive, by 49 B |
| `x_ep` RGB | 6 064 | 6 043 B | 7 240 B | table, by 1 197 B |
| `x_ep` RGBA | 37 040 | 24 643 B | 44 228 B | table, by 19 585 B |

The crossover is where the average count falls to a few hundred:

| `N` | crossover `K` | `K/N` |
| ---: | ---: | ---: |
| 76 800 | 747 | 1/103 |
| 564 000 | 2 035 | 1/277 |
| 4 512 000 | 5 767 | 1/782 |

**Every file where palettizing is worth considering sits on the adaptive side of
that line.** By the time an explicit table would pay, `K/N` is large enough that
the palette itself has already sunk the idea. So: no frequency table.

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
the end and not during a search, so the search needs a proxy. Two natural ones:

* **`Hpair`** — the order-0 entropy of `(rank(a) − rank(b)) mod K` over every
  west/north adjacent pixel pair. Computable from a **co-occurrence table built
  once**, so evaluating a candidate ordering costs `O(nnz)` rather than `O(N)`.
  On `x_ai` that is 64 entries against 9 million: the difference between a
  1 000-candidate search taking a second and taking an hour.
* **`Hmed`** — the order-0 entropy of the MED residual of the reordered index
  plane. Closer to what the coder does, but `O(N)` per candidate.

Measured against reality (section 6.3), both are directionally right over a wide
range and unreliable inside a few percent. The design that follows from that is
the one `bmg` already uses for its predictor choice (`BMG-FORMAT.md` §4.9):
**shortlist with the proxy, then trial-encode the shortlist and keep the
shortest**. Two bits of side information name the winner of four.

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

### 6.6 and the negative control

For the photographic and near-injective files, the index plane's order-0 MED cost
plus the palette, against what `bmg` produces today:

| file | index MED | index bytes | + palette | total | `bmg` today | ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `t24` | 12.82 b/px | 123 036 | 75 959 | 198 995 | 53 718 | **3.7×** |
| `t32` | 7.12 b/px | 68 381 | 105 569 | 173 950 | 53 760 | **3.2×** |
| `x_ep` RGBA | 8.68 b/px | 611 704 | 19 856 | 631 560 | 339 560 | **1.9×** |
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

Two refinements the corpus argues for.

**Palettize colour, not the whole cell, when there is an alpha channel.**
`x_ep`'s 37 040 RGBA pixels are 6 064 RGB colours and an alpha plane; the RGBA
palette costs 19 856 bytes and the RGB one 1 636. Alpha is a different kind of
signal — `bmgstat` reports it as "a real gradient" here, with 77.5% of pixels
fully opaque — and it wants its own plane and its own model, not a seat in a
joint alphabet that it multiplies by six.

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
   Hilbert, greedy RGB tour — then a linear mix optimised against `Hmed` from the
   co-occurrence table. Shortlist four, trial-encode them, send two bits. On `t8p`
   the search reaches the best ordering; on `x_ci` and `x_ai` an exhaustive
   permutation search is affordable and finds 0.20–0.32%.

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

What I would *not* build: gamma in the sort key, until a file turns up whose
palette has genuinely uneven level spacing. `bmgstat`'s tone-curve section is the
test, and it reports γ = 1.000 with rms 0.00 for every component of every file
where palettizing is in play. The one exception is the blue component of `t24`
and `t32` — 64 values, γ = 1.76 — and its rms of 76 says those levels are not on
a power law either, just irregular. They are also the two files a palette cannot
help under any circumstances.

---

## 10. How this was measured

`bmg` at commit-time, release build, on the seven corpus files. Everything here
is reproducible — the scripts are in `palexp/` and need only numpy:

```sh
./mk.sh release
python3 palexp/run.py   testfiles/t8p.bmp   # §2.2, §3, §6.1: orderings, real bytes
python3 palexp/exact.py testfiles/x_ci.bmp  # §6.2, §6.3: every ordering, and the fit
python3 palexp/big.py   testfiles/t24.bmp   # §6.6: the negative control
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
* The one thing measured with a proxy alone is the large-`K` control in §6.6 —
  `bmg` cannot code a 16-bit index plane, so those rows are order-0 MED entropy,
  which §6.3 shows `bmg` beats by about 3%. Even at a 3% discount the verdict is
  a factor of two.
