# CDM2: Contextual Lanes and Non-Contiguous Parsing Over CDM's Block Coder

*Design document. The premise is measured (§2), the parsing algorithm is worked
out (§6), nothing is implemented yet. All measurements in this document were made
with `045-BIJ--v1/cdm` and the `BWTS` tools in this repo, on this machine.*

---

## 1. What this is

CDM's real pipeline is not `file → CDM`. It is

```
    file → bitwise BWTS → CDM
```

and almost all of the compression comes from the transform. Measured on 262 kB of
text:

| pipeline | output | gain |
|---|---|---|
| `cdm` alone | 257 267 | 1.86 % |
| `bwts` (bytewise) + `cdm` | 239 083 | 8.80 % |
| `bwth` (bitwise, MSB-first) + `cdm` | **75 630** | **71.15 %** |

The bitwise BWT sorts bit contexts, which turns *contextual* redundancy into
*local, contiguous* popcount bias — and local contiguous bias is exactly what
CDM's one-dimensional parser and enumerative block coder can exploit. The BWT is
doing the context modelling; CDM is a very good post-coder for its output.

CDM2 replaces that fixed global context grouping with something more flexible.
Instead of one stream, we synthesize many **virtual streams** ("lanes") from the
same input, each holding the bits belonging to one value of a chosen context
function — previous *N* bits, previous byte, bit position within a word, distance
since the last `FF`. Each lane is packed back into bytes and coded with CDM's
block codec. The optimizer chooses, for every bit, which lane codes it.

The resulting parse is a set of `{lane, length}` chunks, exactly as in CDM — but
the chunks are **non-contiguous in file order**. A chunk may hold the bits whose
preceding six bits were `101101`, scattered throughout the file; the bits between
them belong to other lanes. §6 is about how to choose that covering, which is the
hard part and the main subject of this document.

**Measured headroom.** Splitting the *already BWT-transformed* data into
`prev 6 bits` lanes and coding each lane with CDM beats coding the whole thing with
CDM by **19.8 %** on text, 3.3 % on JPEG, 0.5 % on gzip output (§2.3). So there is
real room on top of the existing pipeline, and it is largest where the data is most
compressible.

**What CDM2 is, in one line:** a semi-static context-model coder in which each
context's bits are coded in blocks by exact popcount rather than adaptively, with a
parser choosing both the block boundaries *and* the context order per region.

---

## 2. Measurements

Everything below is reproducible with `tools/ctxscan` (a screening statistic) and
`tools/lanesplit` (the direct test). Read §2.5 before trusting `ctxscan`.

### 2.1 What a chunk can possibly save

For a chunk of *n* bits with *k* zeros, write `d = n/2 − k`. The enumerative code
costs `log2 C(n,k)`, so against *n* raw bits it saves

```
    n − log2 C(n,k)  ≈  ½·log2(π·n/2)  +  2d²/(n·ln2)
```

Verified against exact `log2 C(n,k)` to better than 0.2 bits at n = 2048,
|d| ≤ 160. The two terms behave completely differently:

* **`½·log2(π·n/2)`** — 5.83 bits at n = 2048. It is what you gain from knowing the
  popcount exactly, and it is paid straight back by transmitting the popcount. Net
  ≈ 0. Not a source of compression.
* **`2d²/(n·ln2)`** — the only term that can win. Needs real popcount bias.

Under the null (iid unbiased bits) `E[d²] = n/4`, so pure fluctuation yields
`1/(2·ln2) = 0.721` bits per chunk, independent of *n*. **That 0.72 bits/chunk is
what plain CDM harvests on incompressible input**, and it explains the 0.05–0.5 %
gains reported in `CDM-analysis.md` without appealing to model structure at all.

Setting `2d²/(n·ln2) = H` for header cost *H* gives the bias a chunk needs to pay
for itself: `|d| > √(H·n·ln2/2)`. At n = 2048, H = 10 bits that is |d| > 84 against
a null σ of `√n/2 = 22.6` — about **3.7σ**, which essentially never happens by
chance. Systematic context bias is the only way there, and that is what the BWT and
the lanes both supply.

### 2.2 Baselines: transform and bit order

262 kB text, 187 kB JPEG (concatenated `video-001.*.jpeg`), 262 kB `gzip -9`
output. `bwts` is bytewise BWTS; `bwth`/`bwtl` are bitwise BWTS with MSB-first and
LSB-first bit order.

| input | `cdm` | `bwts`+`cdm` | `bwth`+`cdm` | `bwtl`+`cdm` |
|---|---|---|---|---|
| text 262 144 | 257 267 (1.86 %) | 239 083 (8.80 %) | **75 630 (71.15 %)** | 76 994 (70.63 %) |
| jpeg 186 614 | 184 193 (1.30 %) | 182 515 (2.20 %) | **170 587 (8.59 %)** | 173 895 (6.82 %) |
| gzip 262 144 | 261 911 (0.09 %) | 262 011 (0.05 %) | 261 034 (0.42 %) | **254 245 (3.01 %)** |

Two things to take from this.

**Bitwise ≫ bytewise.** CDM's model is a *bit* popcount model, so it wants bit
contexts grouped, not byte contexts. On text the difference is 8.8 % vs 71.2 %.

**Bit order is a first-class degree of freedom, and it is data-dependent.**
MSB-first wins on text and JPEG; LSB-first wins on gzip output by **7×** (3.01 % vs
0.42 %) — because deflate packs its Huffman codes LSB-first within bytes, so the
LSB-first bit order recovers the format's actual bit sequence. Any tool in this
family should treat LE/BE as a searched parameter, not a build-time constant.

### 2.3 Headroom: lanes on top of the best transform

The direct test — split into lanes, run `cdm` on each, sum the archives. This
already includes each lane's own container overhead, so it is a conservative
measure of the gain (a real CDM2 shares one coder, §7.2).

| input | layout | lanes | `cdm` whole | Σ `cdm`(lane) | ratio |
|---|---|---|---|---|---|
| text→`bwth` | `flat` | 1 | 75 630 | 75 630 | 1.000 |
| | `bitpos mod 8` | 8 | | 126 977 | 1.678 |
| | `prev 1 bit` | 2 | | 63 376 | 0.837 |
| | `prev 3 bits` | 8 | | 61 362 | 0.811 |
| | **`prev 6 bits`** | 64 | | **60 639** | **0.801** |
| | `prev 8 bits` | 256 | | 60 880 | 0.804 |
| | `prev byte` | 256 | | 73 300 | 0.969 |
| jpeg→`bwth` | `prev 3 bits` | 8 | 170 587 | 164 982 | 0.967 |
| | `bitpos mod 8` | 8 | | 183 936 | 1.078 |
| gzip→`bwtl` | `prev 3 bits` | 8 | 254 245 | 253 029 | 0.995 |
| | `bitpos mod 8` | 8 | | 261 696 | 1.029 |

Five findings, and they determine the design:

**(a) History lanes win, by a lot on compressible data.** 19.8 % on text, 3.3 % on
JPEG, 0.5 % on gzip. The gain tracks how much structure is left after the BWT.

**(b) Positional lanes *hurt* on BWT output** — +68 % on text, +7.8 % on JPEG,
+2.9 % on gzip. The BWT has arranged the data so bias is contiguous; striding a
chunk across every 8th bit averages that bias away. So the layout menu **must
include `flat`** and the choice must be per-region.

**(c) There is a dilution optimum at a few kB per lane.** `prev 6 bits` (64 lanes,
4 kB/lane) beats `prev 8 bits` (256 lanes, 1 kB/lane): 0.801 vs 0.804. Lanes want
to be at least a couple of CDM blocks long, which caps the useful lane count at
roughly `filesize / 4 kB`. This is the classic context-dilution tradeoff and is
what makes §6.4's *per-node* order selection worth having.

**(d) The context must match the transform's granularity.** `prev byte` (0.969) is
far worse than `prev 8 bits` (0.804) — the same number of lanes, the same amount of
history, but byte-aligned. After a *bitwise* BWT the meaningful context is the
preceding *bits*.

**(e) Positional lanes do win on non-BWT structured binary.** By the `ctxscan`
screen (proxy, §2.5): float32 array — `bitpos mod 32` at 17.7× `flat`, peaking
exactly at the 4-byte record size; int32 array — `prev 6 bits` at 69× `flat`; ELF
binary — `bitpos mod 64` at 1.24× `flat`. So the positional family earns its place
in the menu, just not on BWT output.

### 2.4 Do not cascade CDM on CDM

`ctxscan` over all 16 contexts on `045-BIJ--v1`'s own archive of the float32 file —
the same file where `bitpos mod 32` screened at 17.7× `flat` on the raw data:

| context | excess bits/chunk on the CDM archive |
|---|---|
| `flat` | 0.014 |
| `bitpos mod 8` / `mod 32` | 0.000 / 0.013 |
| `prev byte` / `prev 6 bits` | −0.025 / 0.006 |
| **all 16 contexts** | **every one inside 2σ of zero** |

Level-1 CDM compressed that file by 0.83 % and destroyed all of the positional
structure doing it: its output is a rangecoder codestream, and enumerative coding
of 2048-bit blocks scrambles bit positions thoroughly. **A second CDM level over a
first CDM's output has nothing to work with.** "Second level" in CDM2 means a
second level of *parsing optimization* applied to the transform output, not a second
pass over compressed data.

### 2.5 Calibrating the screen

`ctxscan` reports `excess = Σ 2(d² − n/4)/(n·ln2)` over fixed 2048-bit chunks, with
a noise floor of `√N` bits for *N* chunks. It is a **ranking** tool, not a
predictor of magnitude:

| input | `ctxscan` ratio, best vs `flat` | actual size ratio (§2.3) |
|---|---|---|
| text→`bwth` | 2.05× | 0.801 (i.e. 19.8 % gain) |
| jpeg→`bwth` | 8.07× | 0.967 |
| gzip→`bwtl` | 15.6× | 0.995 |

The ranking is right — `prev 6 bits`/`prev 3 bits` best, `bitpos mod 8` worst, in
both columns — but the magnitude is overstated by roughly an order of magnitude,
and the overstatement *grows* as the real gain shrinks. Two reasons: the statistic
uses fixed chunk boundaries while CDM chooses them adaptively (so it underrates
`flat` most, where bias is contiguous), and it ignores header cost entirely.

**Use `ctxscan` to prune a 30-entry layout menu to 3 candidates. Use `lanesplit`
plus real `cdm` runs for any number you intend to believe.**

### 2.6 The gate

Before implementing §6, on the actual intended corpus:

1. run `bwth`/`bwtl` and pick the bit order;
2. run `ctxscan` on the transform output and rank the layouts;
3. run `lanesplit` + `cdm` on the top three.

If step 3 shows no layout below 0.99× `cdm` whole, stop — the optimizer has nothing
to work with and CDM2 will lose to `bwth`+`cdm` by the size of its side
information.

---

## 3. Architecture

```
                 ┌───────────────────────────────────────────────┐
   input file ──►│ 0. TRANSFORM (existing)                       │
                 │    bitwise BWTS, bit order LE or BE — searched │
                 └───────────────┬───────────────────────────────┘
                                 ▼
                 ┌───────────────────────────────────────────────┐
                 │ 1. LANE BUILDER                               │
                 │    context tree → lane partition of the bits  │
                 └───────────────┬───────────────────────────────┘
                                 │ lane bits + prefix popcounts
                                 ▼
                 ┌───────────────────────────────────────────────┐
                 │ 2. PRICE ORACLE                               │
                 │    cost(lane, offset, length) in O(1) from     │
                 │    prefix popcounts + CDM's LOG2.FAC table     │
                 └───────────────┬───────────────────────────────┘
                                 ▼
                 ┌───────────────────────────────────────────────┐
                 │ 3. PARSING OPTIMIZER  (§6)                    │
                 │    tree DP over context order  ×  per-lane    │
                 │    contiguous DP over chunk boundaries        │
                 └───────────────┬───────────────────────────────┘
                                 ▼
                 ┌───────────────────────────────────────────────┐
                 │ 4. CONTAINER  (§8)                            │
                 │    split flags, then chunk bodies with CDM's   │
                 │    block codec on one shared rangecoder        │
                 └───────────────────────────────────────────────┘
```

Two departures from the original sketch, both of which remove most of the cost.

**CDM is a codec and a price table, not a running coroutine.** The sketch was to
instantiate many CDM coroutines and feed each a synthesized stream. That works, but
a CDM instance is 23.3 MB (dominated by `tok_array`, 8 MB) and pricing needs no
coder at all: the block price is a closed form (§5.1). Only the final encode runs
the block codec, once. If a coroutine-per-lane design is wanted for prototyping,
note that `045-BIJ--v1`'s `Reset()` makes *sequential* reuse of a single instance
cheap — `Init()` (the ~1 MB price tables) once, `Reset()` per lane — which is the
difference between one 23 MB object and Λ of them.

**CDM2 subsumes CDM's own parser.** The optimizer already fixes every chunk
boundary, so the container encodes chunks directly with CDM's *block* coder
(`process_blklen` plus the enumerative/stored bodies) and never runs CDM's token
DP. Level-1 CDM is recovered exactly as "one lane, `flat`".

---

## 4. Lanes and layouts

### 4.1 Definitions

* Transform output is a bit sequence `b[0..M−1]`, `M = 8·size`.
* A **context variable** is a causal function of the bits before position *i* —
  `b[i−1]`, `b[i−2]`, `(i mod 8)`, the previous byte's popcount, and so on.
* A **layout** is a partition of `[0,M)` induced by fixing a set of context
  variables; its classes are **lanes**. Lane `(q,ℓ)` is the subsequence of bits
  with `q(i) = ℓ`, **in file order**.
* A **chunk** `(q,ℓ,j,L)` is `L` bits of lane `(q,ℓ)` from lane offset `j`. It
  covers `L` file positions, generally non-contiguous.
* A **parse** covers each bit exactly once.

Causality is what lets the decoder derive lane membership (§7). Non-causal contexts
(the *next* byte, say) are inadmissible.

### 4.2 The menu

Ordered by measured value on BWT output (§2.3), which is the opposite of the order
the original sketch suggested.

**Bit-history contexts — the winners.** `prev N bits` for N = 1..8, i.e. lane id is
the value of `b[i−N..i−1]`. 2^N lanes. `prev 6 bits` measured best on text→`bwth`.
Note that `prev N bits` lanes *are* an order-N binary context model; §6.4 is about
choosing N per context rather than globally.

**Products of position and history.** `(i mod 8, b[i−1])` and friends. On
gzip→`bwtl` the screen ranked `bitpos × prev bit` (71.2) above `prev 3 bits` (68.2)
and `prev 6 bits` (68.6) — the only case measured where a product beat pure
history, and plausibly because deflate's bit packing makes position meaningful.

**Byte-derived contexts.** Previous byte (256 lanes), its popcount (9), its high
nibble (16), Gray code of it (256), bucketed delta to the previous byte. All
measured *worse* than same-sized bit-history contexts on bitwise-BWT output
(§2.3(d)); they belong in the menu only for bytewise transforms or untransformed
byte-structured data.

**Positional contexts — for structured binary, not for BWT output.**
`i mod P` for P ∈ {8,16,32,64} and for P from a period scan; `(i/8) mod R` for
record offset. Measured 17.7× `flat` on a float32 array and 1.24× on an ELF binary,
but −68 % on text→`bwth`. Period detection is free: `ctxscan` peaked exactly at
P = 32 for a 4-byte-record file, so scanning `excess(bitpos P)` over a candidate
set and taking the argmax recovers the record length with no autocorrelation
machinery.

**Codec-artifact contexts.** Bucketed distance since the last `FF` or `00` byte,
aimed at Subbotin carry / `FFNum` runs. Measured: nothing above the same-sized
alternatives on any input tried, including on rangecoder output where they were
specifically expected to fire. Keep them in the menu, expect nothing.

**Method variants as extra lanes.** The optimizer does not care *why* two lanes
price differently, so the same lane can appear several times with different coders —
CDM method 1 (enumerative), method 2 (enumerative on Gray), method 0 (stored), or
an adaptive order-0 binary coder for lanes whose bias is strong enough that
enumerative coding is the wrong tool. Each is a separate entry in the chunk
alphabet with its own price function, and §6 is unchanged.

**The stored lane.** Every layout implicitly includes an incompressible lane at
exactly 1 bit per bit plus header. It is always available and it is what bounds
CDM2's worst case (§9).

### 4.3 Why the winning family is the tractable one

`prev 1 bit`, `prev 2 bits`, … `prev 8 bits` are **nested**: the lanes of
`prev 3 bits` refine those of `prev 1 bit`, and every bit belongs to exactly one
lane at each depth. The whole family is a **binary context tree** of depth 8, whose
leaves are the 256 `prev 8 bits` contexts and whose internal nodes are the shorter
histories.

This is not a convenience — it is what makes §6 exactly solvable. A nested family
is *laminar*, and covering a laminar family is a tree DP, not an exact cover. §6.4
is the payoff.

Products extend this: splitting first on `i mod 8` and then on history depth is
still a tree over a fixed variable order, hence still laminar. Laminarity breaks
only when *different variable orders* are mixed in the same region — e.g. `bitpos
mod 32` lanes alongside `prev 6 bits` lanes, whose classes cross-cut. That is the
genuinely hard case, and §6.6 is where it goes.

---

## 5. Pricing

### 5.1 The O(1) oracle

CDM already computes block prices in closed form, and the tables are exactly what
CDM2 needs. Verified on this machine:

* `LOG2.LUT[i]` is `log2(i)` in **.16** fixed point — `LUT[2] = 65536`;
  `LUT[1000]/2^16 = 9.965775` vs `log2 1000 = 9.965784`.
* `LOG2.FAC[i]` is `log2(i!)` in **.12** fixed point — `FAC[10]/2^12 = 21.7910`
  vs `log2 10! = 21.7911`.
* Therefore

  ```
      log2 C(n,k) = (FAC[n] − FAC[k] − FAC[n−k])       in .12 bits
                  = (FAC[n] − FAC[k] − FAC[n−k]) << 4  in .16 bits
  ```

  which is exactly `opt_calc.inc`'s `e = (LOG2.FAC[c0+c1] − LOG2.FAC[c0] −
  LOG2.FAC[c1]) << 4`. Spot-checked against exact `log2 C(n,k)` at (2048,1024),
  (2048,996), (2048,940), (2048,512), (8192,4096), (65536,32768) — agreement to
  ≤ 0.002 bits.
* `LUTsize = 65537`, so **chunks up to 65536 bits (8 KiB) are priceable with the
  existing table** — 32× CDM's own 2048-bit block limit.

So: precompute **prefix popcounts** per lane, and then

```
    body_price(lane, j, L) = log2 C(L, zeros[j+L] − zeros[j])     — two lookups
    chunk_price            = header_price(layout, L) + body_price
```

is O(1) per candidate. For byte-granular lengths CDM's precomputed `e1_price[]` /
`e0_price[]` can be used directly (indexed by block length in bytes and zero count,
with `j = 4(i−1)² + 5(i−1)`).

Keep chunk lengths **byte-granular in lane space**, as CDM does: bit granularity
multiplies the DP's candidate set by 8 for no measured benefit and breaks reuse of
`e1_price`.

### 5.2 Average versus marginal attribution

The original sketch prices a bit as `bit_cl = chunk_cl / chunk_length_in_bits`.
That is the right *total* attribution and the wrong *decision* price, and both are
needed.

**Average is the correct split.** CDM's block code is *exchangeable* — any
permutation of a block's bits with the same popcount gets the identical length,
because the code is `log2 C(n,k)`. Under exchangeability the only symmetric
attribution of the total to individual bits is the uniform one; it is the Shapley
value. So `chunk_cl / n` exactly answers "what share of this chunk's cost does this
bit bear".

**Marginal is the correct price for a move.** The DP asks a different question:
what happens if this bit leaves the chunk. Differencing the closed form,

```
    remove a zero:  log2 C(n,k) − log2 C(n−1,k)   = log2( n / (n−k) )
    remove a one:   log2 C(n,k) − log2 C(n−1,k−1) = log2( n / k )
```

i.e. **the marginal price of a bit is `−log2 p̂`, its self-information under the
chunk's own empirical distribution.** The two differ sharply where it matters: at
k/n = 0.1 the average is 0.469 bits/bit while a one costs 3.32 and a zero 0.152.

Use average for reporting and for the DP objective (where a whole chunk is priced
as a unit anyway); use marginal for reduced costs (§6.6), for local-search move
evaluation (§6.7), and for `λ` initialisation. Using the average there
systematically undervalues moving a minority-value bit out of a skewed chunk, which
is exactly the move that helps.

### 5.3 Header pricing

Do **not** smear the header into per-bit prices. Keep it a per-chunk constant, as
CDM does; that is what makes the DP prefer long chunks and is the whole reason
CDM2 does not degenerate into per-bit context modelling. Measured header cost from
`045-BIJ--v1`'s `TRACE_ON` output: **~0.04 bits for method 0** (`p_maxblk` is
near-certain, so the flag is almost free) and **1.5–15 bits for methods 1/2**,
dominated by coding the popcount.

Charge the side information (§8) inside `header_price`. That is what keeps the
optimizer honest: a parse with a million tiny chunks is then priced correctly and
therefore rejected.

### 5.4 The circularity, and why the tree DP avoids it

Prices are computed from a lane's *full* content. If the optimizer then selects a
subset of a lane, the lane's popcounts change and the prices it optimized against
are not the prices the container pays.

The saving grace: **`log2 C(n,k)` depends only on a chunk's own contents, not on
history.** So if the optimizer only ever takes whole lanes (or whole chunks of a
fixed lane decomposition), the selected chunks' body prices are *exact*. Under
§6.4's tree DP this holds by construction — a node is either coded whole or split
into its children, and either way every chunk that gets priced is a chunk that gets
emitted. **There is no circularity in the recommended design.**

It reappears only in §6.6's free-assignment regime, where the mitigations are
(a) verify-and-iterate — recompute exact prices for the chosen chunks and re-run if
the total moved, which is block-coordinate descent and monotone if moves are
accepted only on exact improvement; and (b) §9's fallback, which makes the
approximation a performance question rather than a correctness one.

---

## 6. Non-contiguous parsing

This is the core of the design.

### 6.1 The problem

With `C` the candidate chunk set and `x_c ∈ {0,1}`:

```
    minimise    Σ_c cost(c)·x_c
    subject to  Σ_{c ∋ i} x_c = 1   for every bit i ∈ [0,M)
```

**Set partitioning** (exact cover), NP-hard in general. CDM's own parser is the
special case where every candidate is a contiguous interval, for which the
constraint matrix is an interval matrix and the problem collapses to the 1-D DP in
`opt_tok.inc`. Lanes destroy that: a chunk of `prev 6 bits` lane 45 covers a set of
positions scattered through the file, interleaved with every other lane.

### 6.2 What structure survives

**Lemma (order preservation).** Every layout in §4.2 fills its lanes in file order:
if `i < i'` and `q(i) = q(i')` then `i` precedes `i'` in lane `q(i)`.

This is forced by causality — a context evaluated at `i` depends only on `b[0..i−1]`,
so a bit's lane is fixed when a left-to-right sweep reaches it, and lanes are only
appended to. Three consequences:

1. **A chunk is lane-contiguous**: it covers *all* of its lane's positions between
   its first and last, and nothing else in that lane.
2. **Chunks of one lane are totally ordered**, and a lane's covered set is always a
   *prefix*. A lane's state is one integer.
3. **A left-to-right sweep is well-defined**: when it reaches an unclaimed position
   `i` and starts a chunk in lane `q(i)`, that chunk takes the next `L` members of
   that lane from `i` on — no earlier chunk of the same lane can have claimed a
   member `≥ i`, since it would have had to claim `i` first.

So the only source of hardness is **cross-lane conflict**: a chunk of lane A begun
at `p < i` may reach past `i` and claim a position that a chunk of lane B begun at
`i` also wants. The exact DP state is therefore the occupancy of a forward window
`[i, i+W)`, with `W` bounding any chunk's reach. For `bitpos 8` with 2048-bit
chunks, `W = 16384` bits: `2^16384` states. **Exact DP over arbitrary layout
mixtures is unreachable**, and no implementation cleverness changes that. The
design's job is to pick the right restriction — and §4.3 already found it.

### 6.3 One layout at a time is separable

If a region is covered by a **single** layout, there is no cross-lane conflict at
all: the lanes partition the region's bits by construction and every chunk lives
inside one lane. The problem decomposes into Λ **independent contiguous 1-D
parses**, each exactly CDM's existing DP:

```
    best[j] = min over L of ( best[j−L] + chunk_price(lane, j−L, L) )
```

Exact, and `O(n·Lmax)` per lane with `Lmax = 256` byte-granular candidates — the
same 256-candidate inner loop CDM already runs. Total work is independent of Λ:
every bit is visited once per layout considered.

Note that the worked example in the original sketch — *"we get L[i] for bit0 of a
byte, and next L[i]−1 bit0's of following bytes are encoded in positional context,
then skipped as we continue to sort input bits"* — **is** this case: a chunk that is
non-contiguous in file order but perfectly contiguous in lane order, inside a single
layout. The expensive freedom is mixing layouts, §6.6.

### 6.4 The context-tree DP — the recommended design

§4.3 established that the winning layouts form a nested family. Exploit it.

Build the **context tree** over a fixed variable order — say `b[i−1]`, `b[i−2]`, …
`b[i−8]`. The root holds all bits; a node at depth *N* holds the bits whose last *N*
context bits match its label; each node's bits are exactly the union of its two
children's. Every bit belongs to exactly one node at each depth.

A parse is then: **choose an antichain of nodes covering all leaves** — equivalently,
for each node, decide *code here* or *split into children*. That is a linear-time
tree DP:

```
    cost(node) =  min(  own_cost(node),                              // code this node's
                                                                     //   bits as one lane
                        split_flag + cost(left) + cost(right)  )     // or recurse

    own_cost(node) = lane_DP( node's bit sequence )                  // §6.3
```

* **Exact.** Not a heuristic, not a relaxation. It finds the optimal antichain.
* **Linear** in the number of nodes, `O(2^{D+1})` for depth *D*, and the total
  `lane_DP` work is `O(D · M · Lmax)` because each depth visits every bit once.
* **Side information is one bit per node** — the split flag. For depth 8 that is at
  most 511 bits for the whole file, and they compress well (splits cluster). There
  is **no per-chunk map at all**; lane membership is derived by the decoder from the
  split flags plus the context variables (§7).
* **No circularity** (§5.4): each node is priced on exactly the bits it would emit.
* **It strictly beats any fixed order.** §2.3 measured `prev 6 bits` at 0.801 and
  `prev 8 bits` at 0.804 — the uniform-order optimum sits between two depths
  because some contexts have enough data to support depth 8 and others do not. The
  tree DP picks per node and dominates both by construction.
* **It solves the dilution problem** (§2.3(c)) automatically: a node splits only
  when its children's chunks pay for themselves, so sparse contexts stay merged.

This is the same pruning DP as context-tree weighting and PPM's exclusion, applied
to a block-enumerative coder instead of an adaptive one, with the block parser
nested inside.

**Product trees.** Splitting first on `i mod 8` and then on history depth is still
a tree over a fixed variable order, so the same DP applies unchanged. Choosing the
variable *order* is a separate, small search — a handful of candidate orders scored
by running the DP — and the DP is cheap enough to run per candidate.

**Per-segment trees.** Page the input (say 64 kB) and let the tree be chosen per
*segment*, a segment being a maximal run of pages sharing a tree. The outer DP over
segments is 1-D:

```
    best[b] = min over a < b, tree T of  best[a] + tree_cost(a,b,T) + switch_cost
```

Cap segment length to bound the work. This is what lets a file that is text in its
first half and a float array in its second get different trees, and it is where the
positional family (§4.2) earns its place.

### 6.5 Pseudocode

```c
// ---- per-lane contiguous parse: CDM's DP with the O(1) price oracle --------
// v.pc[] is the lane's prefix popcount; v.gpc[] the same over Gray-mapped bytes.
price_t lane_parse( Lane& v, Parse* out ) {
  price_t* best = scratch_price(v.nbytes+1);
  int*     from = scratch_int(v.nbytes+1);
  int*     meth = scratch_int(v.nbytes+1);
  best[0] = 0;
  for( int j = 1; j <= v.nbytes; j++ ) {
    best[j] = PRICE_INF;
    for( int L = 1; L <= min(j, MAXBLK); L++ ) {
      int n = 8*L;
      price_t p1 = hdr(1,L) + log2C(n, v.pc [j] - v.pc [j-L]);   // enumerative
      price_t p2 = hdr(2,L) + log2C(n, v.gpc[j] - v.gpc[j-L]);   // enumerative, Gray
      price_t p0 = hdr(0,L) + (n << PRICE_SHIFT);                // stored
      price_t p  = min3(p0,p1,p2);
      if( best[j-L] + p < best[j] )
        best[j] = best[j-L] + p, from[j] = L, meth[j] = argmin3(p0,p1,p2);
    }
  }
  if( out ) backtrack( best, from, meth, out );
  return best[v.nbytes];
}

// ---- context-tree DP: choose the antichain of coded nodes -----------------
// node->bits is materialised lazily: a node's bits are its children's bits
// interleaved in file order, so build depth-first and split on the way down.
price_t tree_cost( Node* nd, int depth, int maxdepth ) {
  price_t here = lane_parse( nd->lane, NULL );          // code this node whole
  if( depth == maxdepth ) { nd->split = 0; return nd->cost = here; }

  split_children( nd );                                 // partition by b[i-depth-1]
  price_t down = SPLIT_FLAG_COST
               + tree_cost( nd->child[0], depth+1, maxdepth )
               + tree_cost( nd->child[1], depth+1, maxdepth );

  if( down < here ) { nd->split = 1; return nd->cost = down; }
  nd->split = 0; free_children( nd );                   // reclaim; they are not used
  return nd->cost = here;
}

// ---- outer DP over segments, each with its own tree ----------------------
void optimize( Segmenting& s ) {
  for( int b = 1; b <= s.npages; b++ ) {
    s.best[b] = PRICE_INF;
    for( int a = max(0, b - SMAX); a < b; a++ )
      for( TreeSpec T : menu_for_pages(a,b) ) {         // pruned by ctxscan, §2.5
        price_t c = s.best[a] + build_and_cost_tree(a,b,T) + SWITCH_COST;
        if( c < s.best[b] ) s.best[b] = c, s.record(b,a,T);
      }
  }
  s.backtrack();
}
```

The one subtlety is that a node's context must be evaluated over the whole prefix,
not just the segment, or the first positions of each segment get a different context
than the decoder will compute. Seed the context from the preceding bytes — which is
what the decoder does naturally.

### 6.6 Mixing non-nested layouts: the genuinely hard case

The tree DP cannot express "these bits go to `bitpos mod 32` lane 5 while their
neighbours go to `prev 6 bits` lane 41", because those classes cross-cut. If
measurement shows such mixing is worth real bits, this is the general version, and
the costs of the freedom are both real:

* **Exact cover** (§6.2) — no exact algorithm.
* **An explicit map.** Membership is no longer derivable, so the container must
  transmit a `{layout, lane, length}` token per chunk. Same shape as CDM's own
  token stream and codable the same way (adaptive, contexted on the previous
  token's layout and the position phase), but now a real cost that §5.3 must charge.

**Lagrangian relaxation** is the principled attack, and it decomposes into problems
CDM already solves. Relax the covering constraint with per-bit multipliers `λ_i`:

```
    L(λ) = Σ_i λ_i + min over chunk sets Σ_c ( cost(c) − Σ_{i∈c} λ_i ) x_c
```

The inner minimisation has no cross-lane coupling — each chunk's reduced cost
depends only on its own bits — so it splits into one problem per lane: *select
disjoint lane-contiguous chunks with negative reduced cost*, which is **CDM's
parser with a per-bit credit**:

```
    best[j] = min( best[j-1],                                     // leave uncovered
                   min over L of best[j-L] + cost(chunk) - Σλ )   // cover
```

Then subgradient ascent, `λ_i += α_t·(1 − coverage_i)`, raising the price of
under-covered bits. `L(λ)` is a valid **lower bound** at every iteration, which is
the real prize: it says how much a heuristic parse is leaving on the table, and
therefore whether §6.6 is worth pursuing at all. The relaxed solution is infeasible
(bits covered twice or not at all), so a **repair** step is needed — sort by reduced
cost, greedily accept non-conflicting chunks, cover the remainder with stored
chunks. Repair quality is what decides whether this beats §6.7; measure both
against the bound.

This is Dantzig–Wolfe decomposition with the lane DPs as pricing subproblems. The
attractive part is that the subproblem is code that already exists.

### 6.7 Greedy peeling and local search

Useful as an initialiser and a refinement.

**Greedy peeling.** Price every candidate chunk; push on a priority queue keyed by
*saving per bit covered*; pop the best, commit if conflict-free, mark its bits
claimed. Committing invalidates overlapping candidates, so re-price lazily: on pop,
verify the chunk is still conflict-free and its price current, else re-price and
re-push. Terminates with everything covered (the stored lane always applies).
`O(|C| log|C|)`. Greedy set-cover behaviour — no guarantee, but a decent warm start.

**Local search.** From any feasible parse, moves that preserve feasibility:
*re-lane* a chunk to another layout covering the same bit set (only where the sets
align, e.g. between a node and its parent); *split/merge/shift* boundaries within
one lane — which is just re-running `lane_parse`, hence exact and free; *escape to
stored* for a chunk whose exact price came out above 1 bit/bit. Evaluate with
**marginal** prices (§5.2) and accept only exact improvements.

### 6.8 Recommended order of work

```
  1. choose the bit order: run bwth and bwtl, keep the smaller       (§2.2)
  2. prune the tree/layout menu with ctxscan per segment             (§2.5)
  3. context-tree DP (§6.4) with the per-lane DP (§6.3) inside  — exact
  4. verify: recompute exact prices for the emitted chunks; by §5.4 they
     must match the DP's prices — assert it
  5. optional: Lagrangian bound (§6.6) to size the remaining gap
  6. optional: §6.6 free assignment, only if step 5 says there is room
  7. compare against bwth+cdm and against stored; emit the best      (§9)
```

Steps 1–4 are the product. Steps 5–6 are research, and step 5 exists specifically
to tell you whether step 6 is worth starting.

---

## 7. Decodability

### 7.1 Decode order

The decoder works in **file order** and maintains the split flags (from the segment
header) and, per coded node, a buffer of decoded lane bits with a read cursor.

At position `i` it walks the context tree from the root using the context variables
computed from `b[0..i−1]` — legal, because contexts are causal and all earlier bits
are decoded — until it reaches a node whose split flag is 0. That node's buffer
supplies the bit, refilled by decoding one more chunk when it runs dry.

No circularity: membership needs only the past; chunk bodies need only the
codestream.

### 7.2 Multiplexing the lane codestreams

The decoder needs chunks in the order its buffers run dry, which is data-dependent.

* **(a) Sections per lane with lengths in the segment header.** Simple; costs one
  length field per coded node and requires buffering a segment. Fine for a first
  implementation.
* **(b) One shared rangecoder, chunks in demand order.** The demand order is a
  deterministic function of the tree and the already-decoded bits, so the encoder
  can *simulate* the decoder and emit chunks in exactly that order. One codestream,
  zero length fields, one chunk of buffering per coded node. This is the right
  design, and it is what makes CDM's self-contained blocks pay off — there is no
  adaptive coder state to keep in sync per lane.
* **(c) Fixed round-robin.** Simple, wastes space whenever lane demand is uneven,
  which it always is.

Recommend (b), with (a) as the fallback. (b) needs chunk lengths known before
emission, which they are — the optimizer fixed them.

### 7.3 Buffer bound

Under (b) the decoder holds at most one outstanding chunk per coded node:
`(#coded nodes) × MAXBLK` bytes. A depth-8 tree fully split is 256 nodes × 256
bytes = 64 kB. Fine, and an argument for capping tree depth.

---

## 8. Container format

```
  file    ::= header segment*
  header  ::= magic "CDM2" version method original_length
  method  ::= 0 stored | 1 bwth+cdm | 2 bwtl+cdm | 3 CDM2          (§9)

  segment ::= bitorder tree_spec split_flags npages chunkstream
  chunkstream ::= chunk*                       // interleaved per §7.2(b)
  chunk   ::= { length, method, popcount } body
```

Chunk header fields are coded exactly as CDM codes them today (`process_blklen` in
`codec.inc`: `p_maxblk` for the length-is-maximal flag, `p_lsb` for the length's
low bit index, `p_cfl` for the method, `p_freq_half`/`p_freq_sign`/`p_mid` for the
popcount). Reusing that codec verbatim means the header cost is already tuned, and
`IDX/` can retune it for CDM2's chunk statistics with no new machinery.

`split_flags` is one bit per internal node in breadth-first order, coded with an
adaptive model contexted on depth and on the parent's flag — splits cluster, so
this should cost well under the 511 raw bits of a depth-8 tree.

For §6.6, `chunk` gains a `{layout, lane}` prefix.

---

## 9. Guarantees and fallbacks

1. **Per chunk**, the stored method costs `n` bits plus header, so no chunk exceeds
   its raw size by more than the header.
2. **Per node**, coding whole is always a candidate, so no subtree is worse than
   its parent coded whole — the tree DP cannot lose to a shallower tree.
3. **Per file**, the encoder computes {stored, `bwth`+`cdm`, `bwtl`+`cdm`, CDM2} and
   writes the smallest, with `method` saying which. Worst case is the header.

Rule 3 makes any pricing approximation a performance question only, and gives a
clean regression test: CDM2's output must be `≤ min(bwth+cdm, bwtl+cdm, raw) +
header` on every input, checkable on a corpus without reference to what the
optimizer decided.

---

## 10. Resource budget

| item | size | note |
|---|---|---|
| transform block | 256 kB–4 MB | BWTS block size; dominates encode time |
| lane bits + prefix popcounts | 3 bytes per input bit, per tree depth | the real cost; page it |
| lane DP arrays | `(lanebytes+1) × 12` | per lane, reused |
| `LOG2` tables | 512 kB | `LUT` + `FAC`; shared |
| CDM block codec | one instance | encode only; no `tok_array` needed |
| decoder buffers | `#coded nodes × 256` bytes | §7.3 |

Note what is absent: the Λ× CDM instances of the original sketch. Pricing is a
table lookup and the final encode needs only CDM's block coder, so CDM2 never holds
more than one CDM-sized object — and that one needs neither `tok_array` (8 MB) nor
`e1_price` (1 MB) if used purely as a block codec.

The prefix-popcount arrays are the memory driver: one `uint32` per lane byte per
depth. At depth 8 over a 4 MB block that is 8 × 4 MB = 32 MB of counters, so page
the input into 64 kB–256 kB segments and process one at a time.

---

## 11. Implementation plan

| # | deliverable | gate |
|---|---|---|
| 0 | `tools/ctxscan`, `tools/lanesplit`, bit-order selection | §2.6: some layout below 0.99× `cdm` whole, else **stop** |
| 1 | price oracle + `lane_parse`, one fixed-depth layout, no container | measured Σ`cdm`(lane) reproduced within 1 % from the oracle alone |
| 2 | context-tree DP (§6.4) + container method 3, §7.2(a) multiplexing | beats `bwth`+`cdm` after side information on the text and JPEG cases |
| 3 | demand-order multiplexing (§7.2b), per-segment trees | removes the length fields; per-segment trees earn their switch cost |
| 4 | positional and product trees | the float32/ELF class beats `bwth`+`cdm` |
| 5 | Lagrangian bound (§6.6) | if the duality gap is small, **stop** — the tree DP was enough |
| 6 | §6.6 free assignment with an explicit map | improves the exact total after charging the map |

Milestone 0 is done (§2). Milestone 2 is the product. Note that step 1's gate is
the one that catches a broken oracle before it silently misleads the DP.

---

## 12. Open questions

1. **How much of the 19.8 % survives the container?** §2.3 summed *separate*
   archives, so it already pays per-lane flushes but not the split flags or the
   length fields. The flags are ~511 bits; §7.2(b) removes the lengths. The margin
   looks comfortable on text and thin on gzip output — milestone 2 settles it.
2. **Is the tree DP better than just using `prev 6 bits` everywhere?** §2.3 says
   the uniform optimum is 0.801 at depth 6 with depth 8 slightly worse, so the
   per-node choice has room between them — but possibly only a fraction of a
   percent. Cheap to measure at milestone 2, and if the answer is "no", the tool
   collapses to `bwth | lanesplit -6 | cdm` and is a filter, not a tool. **Run this
   comparison before building the container.**
3. **Bit order per segment rather than per file?** The 7× gzip result (§2.2) came
   from a whole-file choice. Mixed-format files (a container with both deflate and
   raw sections) would want it per segment, which means the transform, not just the
   parser, becomes segment-local.
4. **Should the popcount be coded jointly across lanes?** Sibling nodes' popcounts
   sum to their parent's, so a parent's popcount plus one child's determines the
   other exactly. Coding popcounts down the tree differentially could recover a
   real share of the `½log2(πn/2)` per chunk currently paid back in full — with 256
   coded nodes that is potentially thousands of bits.
5. **Chunk length ceiling.** `LOG2.FAC` supports 65536-bit chunks (§5.1), 32× CDM's
   current 2048. Longer chunks amortise headers better, and lanes have `1/Λ` as many
   bits each, so this matters more for CDM2 than for CDM. Worth measuring whether
   `MAXBLK` should grow.
6. **Interaction with bijectivity.** `045-BIJ--v1` gives bijective termination and
   BWTS is a bijective transform, so the pair composes. CDM2's split flags and
   method byte reintroduce unreachable encodings; if bijectivity matters, the side
   information must be folded into the arithmetic stream rather than framed.

---

## Appendix A: reproducing §2

```sh
# transform, both bit orders (chunk size in KB for bwth/bwtl, bytes for bwts)
bwth c256 in in.bwth ;  bwtl c256 in in.bwtl ;  bwts c262144 in in.bwts

# screen the layouts (ranking only -- see 2.5)
tools/ctxscan in.bwth 2048

# believe only this: split into lanes and actually code them
tools/lanesplit in.bwth 5 /tmp/lane      # 5 = prev6bit
for f in /tmp/lane.*; do cdm c $f $f.a; done
```

Inputs used in §2.2–2.3:

```sh
head -c 262144 <text corpus>          > txt.bin
cat $GOROOT/src/image/testdata/*.jpeg > jpg.bin      # 186614 bytes
gzip -9 -c <text corpus> | head -c 262144 > gz.bin
python3 -c "import sys,struct,math;sys.stdout.buffer.write(
  b''.join(struct.pack('<f',math.sin(i*0.001)*100) for i in range(250000)))" > float32.bin
```

## Appendix B: symbols

| symbol | meaning |
|---|---|
| `M` | transform output length in bits |
| `q`, Λ | layout (context partition), its lane count |
| `(q,ℓ,j,L)` | chunk: `L` bits of lane `ℓ` from lane offset `j` |
| `n, k, d` | chunk bits, its zero count, `n/2 − k` |
| `H`, `N` | per-chunk header cost in bits; number of chunks |
| `λ_i` | Lagrange multiplier ("price") for bit `i`, §6.6 |
| `D` | context-tree depth |
| `.16` / `.12` | fixed-point formats of `LOG2.LUT` / `LOG2.FAC` |
