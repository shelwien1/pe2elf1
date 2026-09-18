# SSE / APM model component design

*Secondary Symbol Estimation for coder0, built from the coder's own
`ParamUpdater` counters, with every knob in `IDX/sh_model-S0.idx`.*

This note covers what an SSE/APM stage is, what the float design in
`sh_SSE1.inc` does and where it stops, the counter-table design that replaced
it (`sh_SSE2.inc`), how it is parameterized and tuned through the IDX
machinery, how it sits in `coder0.cpp`, and what was measured on `book1`
(text) and `wcc386` (x86 code).

---

## 1. What an SSE stage is

The primary model produces, for every bit, a probability `p_in = P(bit=0)`.
An SSE (Secondary Symbol Estimation; the paq family calls it an APM, Adaptive
Probability Map) is a learned function

```
p_out = SSE[ context ]( p_in )
```

i.e. a table of curves, one per context, each mapping the input probability to
a refined one.  The curve is stored at a few points ("buckets") on the input
axis and interpolated between them.  The stage does two jobs at once:

* **Calibration.**  If the primary model is systematically over- or
  under-confident in some region of `p_in` under some context, the curve
  learns the correction.  This is what a stage indexed by the *same* context
  as the primary model can do, and it is worth little when the primary
  counters are already adaptive (§6.1).
* **Adding information.**  If the row context carries something the primary
  model does not see (here: the bytes before `c1`), the curve for that row
  becomes a conditional model in its own right, with `p_in` as a soft feature
  that tells it where on the axis it is.  This is where the large gains come
  from (§6.2).

Because rows are selected by context and columns by `p_in`, each cell is only
ever asked one question: *given this context and given that the primary
model said roughly this, what is the actual bit frequency?*  A cell is
therefore a binary probability estimator, i.e. a counter.  Everything in the
design follows from taking that literally.

---

## 2. The reference design: `sh_SSE1.inc`

`sh_SSE1.inc` is a float SSE written for a multi-symbol coder (its query
signature works on `freq_ratio` / `remaining`), used as `SSEii<8> SSE[65536]`:
one object per order-1 context, no row hashing.  Per object:

| field | role |
|---|---|
| `sse_mpc[Width][Q]` | the curve: one float probability per bucket |
| `sse_tot[Width][I]` | one count per bucket, drives a `1/N` learning rate, capped at `kMaxCount = 550` |
| `ssebits` | a small bit history selecting one of `1<<Width` sub-rows |

**Quantizer.**  The input `p_raw = 1 - freq_ratio` is warped by a *rational
stretch* `p/(p+K)·(1+K)` with `K = 0.25`, then split uniformly into `Q-1`
intervals.  The warp packs resolution near `p_raw = 0` (i.e. near
`freq_ratio = 1`), which suits a one-sided input; for a symmetric binary
input the logistic stretch `ln(p/(1-p))` is the natural choice, and it is what
`sh_SSE2.inc` uses.

**Prediction.**  Linear interpolation between the two neighbouring curve
points, `P = C0 + sw·(C1-C0)`, clipped to `[eps, 1-eps]`.

**Update.**  The target `P' = P·(1-wr) + bit·wr` is formed with the per-bucket
rate `wr = 1/sse_tot`, and the residual `delta = P' - P` is pushed back into the
two points with the *normalized least-squares* weights `w0/(w0²+w1²)`,
`w1/(w0²+w1²)`: the smallest change to the two points that reproduces the
interpolated target exactly.  That is a sound gradient step on the interpolated
output.

**Where it stops.**

* The cell dynamics are fixed: a count-based `1/N` schedule to a hard cap.
  Nothing per cell can decide that this row is non-stationary and should
  forget faster, or that this bucket is noisy and should move slower.
* The table shape is compile-time (`Width`, `Q`, `I`) and the context is
  hard-wired to 65536 direct rows; there is no hashing and no knob for
  *which* history bits select a row.
* `kMaxCount`, `Wi`, `Ti`, `K_stretch` are literals in the source, outside any
  optimizer's reach.
* The commented-out block at the end of `Update()` shows the author trying
  variants of the split of `delta` between the two points; the design has no
  principled way to learn that split.

---

## 3. The new design: SSE as a table of counters (`sh_SSE2.inc`)

### 3.1 The cell

A cell is a `Counter<CP_S0>` — the same object the order-1 model uses for its
65536 contexts, instantiated on a second parameter bundle:

* `n0`, `n1`: decayed bit counts; `p = n0/(n0+n1)`, mixed with a prior
  (`mwP0`, weight `mw`) and sharpened in the logistic domain by `K`:
  `p_cell = sq( K · st( p·(1-mw) + mwP0·mw ) )`.
* `wr0_state`, `wr1_state`: the two decay rates in the rotated log basis
  `u = (ln wr0 + ln wr1)/2` (memory length), `v = (ln wr0 - ln wr1)/2`
  (hit/miss asymmetry), each driven by a `ParamUpdater` — a diagonal Newton
  step on the coding loss with momentum on the gradient (`D`) and on the
  curvature (`R`), step clipping and a box — coupled through the 2x2 solve
  with the EMA cross term `R_uv`.  The gradients come from RTRL traces
  (`n0_w0 … n1_ww1`, the derivatives of the counts w.r.t. the rates).
* `mw_state`, `k_state`: the same `ParamUpdater` on the prior weight (logit
  coordinate) and on `ln K`.

So a cell is 24 floats (96 bytes) with all three adaptations on, and
learns *its own* forgetting rate, asymmetry, prior weight and confidence.
That is what an SSE bucket needs and what `sh_SSE1.inc`'s fixed `1/N` cannot
give it; §6.3 puts numbers on it.

`Counter<CP>` (coder0.cpp) is the pre-existing counter made a template on the
bundle `CP` so that the two components can carry different constants:
`config.hpp` defines `CP_C0` from the `C0_*` constants and `CP_S0` from the
`S0_*` ones, with identical formulas.  The bundle also carries the adaptation
flags, so cheaper cells are one `-D` away (`S0_ADAPT_WR/MW/K`).

### 3.2 The table

```
rows    : one per row context  (IDX Index Cx x Cx3, §4)
columns : nb buckets on the stretch axis, uniform in [-LIM, +LIM]
cell    : Counter<CP_S0>
```

`rows = min( Cx_Volume·Cx3_Volume, 2^HBITS, 2^SSE_MAXCELLS_LOG / NB )`.  When
the context volume exceeds the rows, the 64-bit context is hashed
(multiplicative, high bits) — so widening a mask in the `.idx` never changes
the memory footprint, only the collision rate.

`NB` is a template parameter of `SSE_Ctr<CP, NB>`: the column stride, the
quantizer scale and the init arrays are compile-time in the build that
ships.  It is still an IDX knob, through the two-version scheme of
`freq_SSE1_dbg.inc` (`SSEjQ`'s `switch(Q)` over `SSEii<Q>`), selected by the
`USE_NEW` macro that `idx2inc.pl` writes into the generated headers:

| build | `USE_NEW` | object | `NB` |
|---|---|---|---|
| shipping (`./gc.sh`) | 0 | `SSE_Ctr<CP_S0, CP_S0::NB>` | literal from the `.idx`, folded |
| tuning (`./gc.sh tune`) | 1 | `SSE_Dyn<CP_S0>` | patchable; `Init()` reads it once and picks the `SSE_Ctr<CP_S0, NB>` instantiation for every `NB` in `[2, SSE_NB_MAX]` |

so `opt.pl` measures exactly the code that ships (checked: a tuning binary
patched to `NB = 6` and a shipping build compiled with `NB = 6` produce the
same 545345 bytes).  The dispatcher is one recursive template plus a small
virtual interface; the shipping build contains neither.  `HBITS`, `LIM`,
`T0`, `W` and the mode flags are bundle constants (`CP_S0::LIM`, …): folded
in the shipping build, runtime in the tuning build, clamped at `Init()`.

### 3.3 Prediction

```
s     = clip( st(p_in), -LIM, +LIM )           st = ln(p/(1-p))
x     = (s + LIM) · (nb-1) / (2·LIM)           bucket coordinate, 0 .. nb-1
j     = floor(x),  wt = x - j                  two neighbouring cells c[j], c[j+1]
p0    = c[j].PredictF(),  p1 = c[j+1].PredictF()
p_sse = ILOG ? sq( st(p0) + wt·(st(p1)-st(p0)) ) : p0 + wt·(p1-p0)
p_out = BLOG ? sq( st(p_in) + (st(p_sse)-st(p_in))·W ) : p_in + (p_sse-p_in)·W
```

`PredictF()` stores the cell's own `pK`, which its update needs.  The final
blend with `p_in` is a fixed weight `W`; with `BLOG=1` it is applied in the
stretch domain, which measured better than the probability domain at every
`W` (§6.4) — the stretch blend keeps the SSE's confident predictions
confident instead of pulling them linearly toward `p_in`.

### 3.4 Update

```
UPD=1:  c[j].C_Update(bit);          c[j+1].C_Update(bit);
UPD=2:  c[j].C_Update(bit, 1-wt);    c[j+1].C_Update(bit, wt);     (each floored at UPMIN)
UPD=0:  (wt<0.5 ? c[j] : c[j+1]).C_Update(bit);
```

`Counter::C_Update(bit, g)` takes the weight of the observation: the loss
gradient is scaled by `g`, and in the count recursion every `wr` becomes
`wr·g` and every injected count `g`, so the RTRL traces remain the exact
derivatives of the weighted recursion (`g=1` is bit-identical to the plain
update).  `UPD=2` is therefore the interpolated update of `sh_SSE1.inc` —
the event is split between the two cells in proportion to their share of
the prediction — without the explicit residual arithmetic: each cell runs
its own Newton step on a fractional event.  It measured best (§6.6);
updating both cells fully is second, the nearer cell alone clearly worst.

### 3.5 Initialization

Column `j` of every row starts as the identity curve at that bucket, with a
chosen mass `T0`: the target `p_j = sq(s_j)` is inverted through the cell's own
output map at the seed `K` and `mw`,

```
p_mix = sq( s_j / K ),  q0 = (p_mix - mwP0·mw) / (1 - mw),  n0 = q0·T0,  n1 = (1-q0)·T0
```

so a fresh row returns `p_in` (times `W`) and costs nothing until it has seen
data.  Small `T0` (about one event) measured best: rows are sparse and the
first observation should count.

### 3.6 Why not a simpler cell

The same table was built with three cell types (all other settings equal, row
context `c2,c1,cxt`, cap 2^24 cells, §6.3):

| cell | bytes | book1 + wcc386 |
|---|---|---|
| plain: fixed `wr`, `mw`, `K` (n0, n1, pK) | 12 | 626021 |
| `mw`, `K` adaptive, fixed `wr` (no RTRL) | 36 | 580559 |
| full `Counter` (u/v, mw, K adaptive) | 96 | 577647 |

Two thirds of the gain of the stage comes from the cells adapting their own
parameters; giving the plain cells eight times the rows (cap 2^27) recovers
almost none of it.  The rate adaptation is worth another ~3K on top of
`mw`/`K`.  Memory per cell is the price and it is the one lever this design
does not try to hide (§7).

---

### 3.7 The output mixer (`sh_mix2.inc`)

The static blend of §3.3 is a fixed point on a line the coder can learn:
`p = sq( w·st(p1) + (1−w)·st(p2) + b )` with `p1` the order-1 prediction and
`p2` the SSE output.  `Mix2<CP_M0>` keeps, per mixer context, the weight
`W` (logit coordinate, `w = sq(W)`, box `±Wclip`) and a stretch-domain
bias `b` (box `±Bclip`) as two `ParamUpdater` states — the diagonal Newton
step of `newton.inc`'s `BinaryMixer`, with the same first/second
derivative chain (`dp/dW = p(1−p)(s1−s2)·w(1−w)`, and its second
derivative including the `w(1−w)(1−2w)` term; `dp/db = p(1−p)`).  Its
context is an IDX index of its own file (`IDX/sh_model-M0.idx`: bits of
`c2`, `c1`, the bit-tree node), and every rate, clip and box is a knob
there with `newton.inc`'s scalings.

Frozen (`NWw = 0`, no bias) with `w0 = 1 − W_sse` it reproduces the static
blend byte for byte (529128), which is the regression check.  Adaptive
(§6.9) it is worth another 2.2%, mostly from its context: the right
blend of "what this byte pair's order-1 counter says" and "what the
order-3 row says" differs a lot between contexts, and the bias corrects
the row's calibration where the cells have not caught up.  With the mixer
in place the SSE's own `W` is seeded to 1 (its output is used as is; the
tuned 0.81 blend measured 600 bytes worse on top of the mixer).

## 4. Parameterization: `IDX/sh_model-S0.idx`

Every number the stage reads lives in one `.idx` file with prefix `S0`, so the
whole stage — geometry, context, and the full counter parameter set — is one
`opt.pl` search space, separate from the order-1 model's `C0` file.

```
Index Cx                      # row context, part 1 (int volume, <= 24 bits)
 c2: c2, &11111111            #   bits of the byte before last
 c1: c1, &00011111            #   bits of the last byte
 cx: cxt, b&00000000          #   bit-tree node (b&: node bits merged relative to the leading 1)
Index Cx3                     # row context, part 2
 c3: c3, &01011111            #   bits of the third-last byte
Number HBITS, NB, LIM         # rows limit, buckets, stretch clip
Number T0, W, ILOG, BLOG      # init mass, blend weight/domain, interpolation domain
Number UPD, UPMIN, QLIN       # update rule (nearer / both / proportional + floor), input domain
Number HW, HMODE              # per-row history sub-rows: width, bits or successes
Number DEG                    # B-spline degree of the interpolation kernel (1..3)
Number DM, DSIM               # delayed-update counter: delay, simulation init (with HMODE 2)
Number P0 … mwXhi             # the cell counter constants, same meaning as C0_*
```

* **Context masks** are IDX bitmask mappings: each `1` keeps a bit of the
  byte, so the optimizer decides how much of `c3`/`c2`/`c1` a row sees.  The
  bit-tree node goes through the incremental mask `b&` (`masking_b` /
  `pmask2`): a `1` at position *k* merges the nodes that differ in the
  *k*-th bit after the leading 1, at every depth, so the optimizer can also
  coarsen the node context (all zeros = every node distinct).  The mixer's
  context (`IDX/sh_model-M0.idx`) uses the same three lines.  Two `Index`
  blocks exist only because the generator's volume is an `int`;
  `coder0.cpp` combines them as `Cx·Cx3_Volume + Cx3` in 64 bits.
* **`Number` knobs** compile to literals in the shipping build
  (`./gc.sh`) and to patchable `!MAP!` objects in the tuning build
  (`./gc.sh tune`); `config.hpp` (with `CP_SSE`) derives the `CP_S0`
  constants — counter floats and the SSE knobs `NB`, `HBITS`, `LIM`, `T0`,
  `W`, `ILOG`, `BLOG`, `UPD`, `UPMIN`, `QLIN`, `HW`, `HMODE`, `DEG`, `DM`,
  `DSIM` — from either.
* **Clamps at the point of use.**  `SSE_Ctr::Init()` clamps `nb` to
  `[2,64]`, `HBITS` to `[8,30]`, `LIM` to `[0.25,16]`, `T0`, `W`, and caps the
  cells at `2^SSE_MAXCELLS_LOG`, so any bit pattern the optimizer visits runs
  (IDX-FORMAT.md §5).
* **Compile-time only**: the adaptation flags `S0_ADAPT_WR/MW/K` (they change
  the cell layout), the cell cap `SSE_MAXCELLS_LOG` and the dispatcher's
  range `SSE_NB_MAX` (16; the measured optimum is 6–8); all can be given on
  the build line (`CXXEXTRA="-DS0_ADAPT_WR=0" ./gc.sh tune`).
* **Frozen lines** (`!` prefix, IDX-FORMAT.md §10) keep knobs the code never
  reads out of the search space: `P0`/`P1` (cells are initialized per bucket)
  and `G1_*` (`grad1_clip` is never used) in the S0 file, and the constants of
  the rejected proposals in the C0 file.  A first `opt.pl` pass spent a
  quarter of its evaluations on them before they were frozen.

### 4.1 Tuning workflow on Linux

```
./gc.sh tune                                        # ./coder0t with live S0_/C0_ knobs
perl IDX/setp.pl coder0t S0_NB=8 S0_c3='&11110000'  # set knobs by patching, no rebuild
./tm.sh coder0t                                     # sizes, files in parallel
OPT_JOBS=2 perl IDX/opt.pl opt.lst ./coder0t '^S0_' # hill-climb the S0 maps only
cd IDX && perl import.pl sh_model-S0.idx ../export.!!! > t && mv t sh_model-S0.idx
./gc.sh && ./t1.sh                                  # shipping build, roundtrip check
```

`setp.pl` performs the same in-place edit of the `"!MAP!name!base\0pattern"`
strings that `opt.pl` does, so a hand experiment measures exactly what the
climb would.  The two builds are byte-identical only with FMA contraction
and reassociation off (`-ffp-contract=off -fno-associative-math` in
`gc.sh`): with knobs folded in one build and runtime in the other, gcc
otherwise contracts or reorders the same float expressions differently
and the streams drift by a few bytes.  `opt.pl` gained an optional map-name regex (third argument) and
`OPT_JOBS` for compressing the corpus files in parallel; it treats a coder
that did not exit cleanly as a failed measurement (a crashed or OOM-killed
run leaves a short output behind, which used to count as an improvement).
`gc.sh` passes
`USE_NEW` (1 for `tune`, 0 otherwise) as the second argument of
`idx2inc.pl`, and leaves `MOD/` in the shipping state after either build.

---

## 5. Integration in `coder0.cpp`

Per bit, in `main()`:

```
p1  = o1[c1][cxt].PredictF();                          // primary, P(bit=0)
cx  = S0_MakeCx(c2, c1, cxt) * S0_Cx3_Volume + S0_MakeCx3(c3);
p2  = sse.Predict( cx, p1, o1reg[c1][cxt] );           // refined P(bit=0)
pf  = mix.Mix( M0_MakeCx(c2, c1, cxt), p1, p2 );       // learned blend (§3.7)
p   = uint( clamp( pf * SCALE ) );
bit = rc.rc_BProcess( p, bit );
o1[c1][cxt].C_Update( bit );   (delayed by DM bits when S0_DM > 0)
sse.Update( bit );
mix.Update( bit );
```

`S0_MakeCx`/`S0_MakeCx3` are generated by `idx2inc.pl` from the `Index` blocks
(`MOD/sh_model-S0_p.inc`).  With the mixer bypassed (`M0_ON = 0`) the final p is the
SSE output with its own blend (529128 at the tuned `W`), and with
`S0_W = 0` as well the coder reproduces the pre-SSE stream byte for byte
(344899 / 309703): the regression checks for the whole path.

---

## 6. Measurements

gcc 13.3, `-O3 -march=haswell -ffast-math`, sizes in bytes, `total` = book1 +
wcc386.  Baseline without SSE: **344899 + 309703 = 654602**.

### 6.1 Row context = the primary model's context (no new information)

Rows `(c1, cxt)`, 24 buckets, `W = 0.75`, cell rate seed 0.05:

| variant | book1 | wcc386 | total |
|---|---|---|---|
| no SSE | 344899 | 309703 | 654602 |
| order-1 SSE | 344968 | 311780 | 656748 |
| order-1 SSE, `W = 0.5` | 344747 | 310146 | 654893 |

Calibrating an already-adaptive counter is not worth the noise of a second
learner.  The stage has to bring context the primary model lacks.

### 6.2 Row context = history the primary model does not see

Same settings, adding bits of `c2` to the row (rows hashed into the cap):

| row context | book1 | wcc386 | total |
|---|---|---|---|
| `c1,cxt` | 344968 | 311780 | 656748 |
| `c2[7:6],c1,cxt` | 325807 | 301459 | 627266 |
| `c2[7:4],c1,cxt` | 319197 | 297080 | 616277 |
| `c2,c1,cxt` | 289098 | 293425 | 582523 |

and, once the stage was set to 8 buckets and stretch blending (§6.4), bits of
`c3`:

| `c3` mask | book1 | wcc386 | total |
|---|---|---|---|
| none | 280297 | 287091 | 567388 |
| `11000000` | 269626 | 287631 | 557257 |
| `11110000` | 265267 | 290896 | 556163 |
| `00011111` | 248078 | 295788 | 543866 |
| `11111111` | 247813 | 296893 | 544706 |

Text keeps gaining from order-3 rows (book1 −28% at full `c3`); the x86 file
prefers the denser order-2 statistics and loses ~10K to the dilution, which
is the one real tension between the two files.  The sum favours most of
`c3`, and the mask is left to the optimizer.

### 6.3 Cell type, buckets and rows

Row context `c2,c1,cxt`, `W = 0.75`, rate seed 0.05:

| cell | cap (cells) | buckets | book1 | wcc386 | total |
|---|---|---|---|---|---|
| full (96 B) | 2^24 | 24 | 289098 | 293425 | 582523 |
| full | 2^24 | 16 | 288117 | 291141 | 579258 |
| full | 2^24 | 12 | 287682 | 289965 | 577647 |
| full | 2^24 | **8** | 287265 | 289264 | **576529** |
| full | 2^24 | 6 | 287195 | 289408 | 576603 |
| full | 2^24 | 33 | 290207 | 295684 | 585891 |
| full | 2^25 | 12 | 287650 | 289436 | 577086 |
| no-RTRL (36 B) | 2^24 | 12 | 290018 | 290541 | 580559 |
| no-RTRL | 2^26 | 12 | 289986 | 289964 | 579950 |
| plain (12 B) | 2^24 | 12 | 326274 | 299747 | 626021 |
| plain | 2^27 | 12 | 326244 | 299198 | 625442 |

Few buckets win twice: each cell sees more events, and the same cell budget
buys more rows.  Doubling the rows beyond ~2M helps little for either cell
type; the adaptation inside the cell is what matters.

Rows versus memory (full cells, 8 buckets, `c2,c1,cxt`):

| `HBITS` | rows | table | book1 | wcc386 | total |
|---|---|---|---|---|---|
| 21 (cap) | 2M | 1.6 GB | 287265 | 289264 | 576529 |
| 19 | 512K | 400 MB | 287374 | 292495 | 579869 |
| 17 | 128K | 100 MB | 287935 | 300221 | 588156 |
| 15 | 32K | 25 MB | 290507 | 309987 | 600494 |

`wcc386` (many distinct byte pairs) is the one that pays for fewer rows;
`book1` barely moves down to 128K rows.

### 6.4 Output blend

8 buckets, `c2,c1,cxt`, rate seed 0.05:

| blend | book1 | wcc386 | total |
|---|---|---|---|
| prob. domain, `W = 0.5` | 299591 | 291921 | 591512 |
| prob. domain, `W = 0.75` | 287265 | 289264 | 576529 |
| prob. domain, `W = 0.875` | 283172 | 289151 | 572323 |
| prob. domain, `W = 0.95` | 281555 | 289531 | 571086 |
| prob. domain, `W = 1.0` | 281052 | 290033 | 571085 |
| **stretch domain, `W = 0.75`** | 281556 | 287303 | 568859 |
| **stretch domain, `W = 0.9`** | 280378 | 288296 | 568674 |

The SSE output should dominate, and in the stretch domain a high `W` costs
`wcc386` nothing.  Interpolating the two cells in the stretch domain
(`ILOG = 1`) is a further −600.

### 6.5 Cell dynamics and update mode

Same base as 6.4 (576529):

| change | total | note |
|---|---|---|
| init mass `T0` 2 → 0.5 / 1 / 4 / 16 | 575977 / 575847 / 578106 / 583102 | first event should count |
| rate seed `wr` 0.05 → 0.02 / 0.1 / 0.2 | 578085 / 575368 / 574491 | cells like fast forgetting too |
| `wr1` = wr + 0.025 / wr − 0.025 | 575211 / 579020 | miss count decays faster |
| `LIM` 8 → 4 / 6 / 10 / 12 | 577404 / 576715 / 576911 / 577409 | |
| update nearer cell only (`UPD = 0`) | 587430 | both cells must learn |

These seeds — 8 buckets, full `c2`, `c3 = 00011111`, `W = 0.9` stretch,
`ILOG = 1`, `T0 = 1`, `wr = 0.2`, `wr1 = +0.025` — gave **543866**; with
the proportional update of §6.6 the values in `IDX/sh_model-S0.idx` before
the optimizer run stand at **542194** (542195 in the tables of §6.6, which
were measured before FMA contraction was switched off).

### 6.6 Update rule, input domain, internal width

Base: the seeds of §6.5 (543866; rows `c3[4:0],c2,c1,cxt`).

**Interpolated (proportional) update** — `UPD=2`, each cell gets the event
with its interpolation weight, floored at `UPMIN`:

| update | book1 | wcc386 | total |
|---|---|---|---|
| both cells, `g=1` (`UPD=1`) | 248078 | 295788 | 543866 |
| proportional, floor 0 | 248282 | 293913 | **542195** |
| proportional, floor 0.25 | 248180 | 294026 | 542206 |
| proportional, floor 0.5 | 248051 | 294433 | 542484 |
| proportional, floor 0.75 | 247934 | 295040 | 542974 |
| nearer cell only (`UPD=0`, §6.5 base) | | | +11K |

The x86 file gains ~1.9K from not training a cell on events that mostly
belong to its neighbour; text prefers slightly heavier updates (the floor
trades one against the other).  With `c3` off the picture is the same
(567388 → 566041).  Proportional with floor 0 is now the seed.

**Input domain** — `QLIN=1` places the buckets uniformly in probability
instead of in `stretch(p)`:

| quantizer | buckets | book1 | wcc386 | total |
|---|---|---|---|---|
| stretch, `LIM=8` | 8 | 248078 | 295788 | 543866 |
| linear | 8 | 248860 | 295956 | 544816 |
| linear | 12 | 250901 | 299099 | 550000 |
| linear | 16 | 252916 | 301306 | 554222 |

Linear buckets waste resolution where the coding cost is (near 0 and 1);
adding buckets to compensate costs rows under the cell cap and loses more.
The stretch axis stays.

**Internal width** — `HW` history bits per row select one of `2^HW`
sub-rows, `HMODE=0` the row's recent bits (`sh_SSE1.inc`'s `ssebits`),
`HMODE=1` its recent SSE successes (the SSE gave the occurring bit
`p > 0.5`):

| rows | `HW` | history | book1 | wcc386 | total |
|---|---|---|---|---|---|
| `c3,c2,c1,cxt` | 0 | | 248078 | 295788 | 543866 |
| | 1 | bits | 253095 | 302129 | 555224 |
| | 2 | bits | 258592 | 308491 | 567083 |
| | 1 | successes | 254258 | 302927 | 557185 |
| | 2 | successes | 260870 | 308971 | 569841 |
| `c2,c1,cxt` | 0 | | 280297 | 287091 | 567388 |
| | 1 | bits | 282113 | 291287 | 573400 |
| | 2 | bits | 284511 | 296205 | 580716 |
| | 1 | successes | 282651 | 293486 | 576137 |
| | 2 | successes | 285513 | 298745 | 584258 |

Both flavours lose, with or without `c3`, and more with each bit.  Part of
that is rows (sub-rows share the cell cap: one history bit halves the
rows, which alone costs ~2K per §6.3), the rest is dilution: the row
already carries the bit-tree node and two or three whole bytes, and a cell
that adapts its own rate and confidence already reacts to a run of
surprises the way a "success" sub-row would, without splitting its
statistics.  The width made sense in `sh_SSE1.inc`, whose rows were plain
order-1 contexts with fixed-rate cells; here it stays a knob, at 0.

**Interpolation degree** — `DEG` selects the uniform B-spline kernel: 1 =
linear over 2 cells (the default), 2 = quadratic over 3, 3 = cubic over 4.
The cells are the spline's control points, the kernel weights are
non-negative and sum to 1, so they serve both the prediction and the
proportional update (they are exactly ∂p_sse/∂cell; a Catmull-Rom cubic,
which passes through the nodes, has negative weights and is not offered).
Rows get one padding cell at each end so no weight is ever clamped.

| kernel | buckets | update | book1 | wcc386 | total |
|---|---|---|---|---|---|
| linear | 8 | proportional | 248282 | 293913 | **542195** |
| quadratic | 8 | proportional | 248853 | 295079 | 543932 |
| cubic | 8 | proportional | 249325 | 296048 | 545373 |
| quadratic | 10 | proportional | 249023 | 294750 | 543773 |
| cubic | 10 | proportional | 249336 | 295206 | 544542 |
| quadratic | 6 | proportional | 248961 | 296509 | 545470 |
| quadratic | 8 | all cells, `g=1` | 248592 | 300748 | 549340 |
| cubic | 8 | all cells, `g=1` | 248922 | 305379 | 554301 |
| quadratic | 8 | proportional, prob. domain | 250036 | 296415 | 546451 |

Every higher degree loses, and adding buckets to give the smoother kernel
back its resolution does not recover it.  The curve an SSE row has to
learn is not smooth in the way a spline assumes: with adaptive cells the
two-cell linear scheme already lets each bucket move on its own, and a
wider kernel spreads every event over cells that mostly belong to other
input probabilities (the same effect that made the full-both update lose
to the proportional one).  Linear stays; `DEG` remains a knob.

### 6.7 The delayed-update counter reading of SSE

A *delayed-update counter* keeps an M-bit register of its most recent bits
(with a leading marker bit, so M+1 bits of state cover the warm-up) and
learns each bit only when it falls out of the register, M steps late.  Its
prediction therefore lags, and a *contextual transformation* indexed by the
register turns the lagging prediction into the current one.  The transform
can be initialized to reproduce the plain counter exactly — apply the M
pending bits to a counter state that would give the delayed prediction —
and then adapt, so the counter's fixed recursion over its last M bits is
replaced by a learned function of those very bits.  That is an SSE whose
row context is the counter's own register and whose input is the delayed
counter; implemented here as `S0_DM` (delay), `S0_HMODE=2` (the order-1
cell's register selects the sub-row) and `S0_DSIM` (simulation vs.
identity init of the sub-rows, `duc_sim()` in coder0.cpp, at the seed
rates and steady-state mass).

As a refinement of the order-1 model alone (rows `c1,cxt` = the counter's
own context, `W=1` so the transform output is used as is; plain counters
654602):

| register | delay | init | `T0` | book1 | wcc386 | total |
|---|---|---|---|---|---|---|
| 1 bit | 0 | — | 1 | 344713 | 309988 | 654701 |
| 1 bit | 1 | simulation | 1 | 344714 | 309531 | 654245 |
| 1 bit | 1 | simulation | 4 | 344777 | 308387 | **653164** |
| 2 bits | 2 | simulation | 1 | 345620 | 310437 | 656057 |
| 2 bits | 2 | identity | 1 | 345495 | 314192 | 659687 |
| 2 bits | 2 | simulation, `W=0.9` | 1 | 345197 | 309835 | 655032 |

The mechanism works as described: the simulation init lands close to the
plain counter (identity init costs 3–9K more) and a one-bit delay with a
learned transform is worth −1438 (−0.2%), all of it on the x86 file.  A
two-bit register already loses: every register value splits the row's
statistics further, and the adaptive counter's own recursion over its
last bits is hard to beat with a table that has to learn it per row.

Inside the full stage (rows `c3[4:0],c2,c1,cxt`) the register costs rows
under the cell cap and dilutes the order-3 statistics; against the plain
SSE at the same row count it loses 11K with one register bit and 20K with
two, delayed or not, and 21K in the best delayed configuration (563590
vs. 542194).  The one table is worth far more spent on context the
counter does not have (§6.2) than on re-learning what the counter does
with the context it has; the delayed reading would be a second, small
stage on the order-1 model, and the knobs stay in the code for that.

### 6.8 Tuned result

One `opt.pl` pass over the S0 file (two climbs on disjoint halves, about
1.3k evaluations of the pair, 7 s each on 4 cores), then the exports
folded into `IDX/sh_model-S0.idx`:

| | no SSE | SSE, seeds (§6.5) | SSE, tuned |
|---|---|---|---|
| book1 | 344899 | 248078 | **241409** (−30.0%) |
| wcc386 | 309703 | 295788 | **287719** (−7.1%) |
| book1 + wcc386 | 654602 | 543866 | **529128** (−19.2%) |
| book1wcc (concatenation) | 656163 | | **531716** (−19.0%) |
| book1 encode / decode | 1.0 s | | 5.7 s / 5.7 s |

All three roundtrips verify (`t1.sh`), and the shipping and tuning builds
produce the same bytes.  What the pass moved, in order of effect:

* the cells' logistic scale `K` (0.74 → 0.87) and the top of its box
  `kMax` (0.94 → 1.50): SSE cells want to be sharper than the order-1
  counters are allowed to be;
* the init mass `T0` (1 → 2.5) and the blend `W` (0.90 → 0.81);
* the row context: `c1` narrowed to its low 5 bits, `c3` widened to
  `01011111` — the optimizer trades resolution in the last byte for a bit
  more of the third-last one, in the same cell budget;
* the K optimizer's curvature clip `G2_k` → 0 and clamp `G4_k` up, the mw
  optimizer's momentum shortened (`M1_m`, `M2_m`), `mwMin` raised;
* the u/v knobs: momentum, `RUinc`/`RVinc` (the Newton damping) down to
  almost nothing, `UVH` (the |v| bound) down, `CXW` up — together worth
  under 500 bytes.

The u/v rate optimizer of the cells matters little once the seeds are
right; `K`, `T0`, `W` and the context masks carry the result.  A second
pass over all S0 knobs jointly and a pass over the C0 knobs (the order-1
model tuned as an SSE *input* rather than as the final predictor) are the
obvious next steps and were started; their outcome is recorded below when
available.

---

### 6.9 Mixer

Seeds of §6.8 (529128), SSE `W = 1`, mixer as in §3.7:

| mixer | context | book1 | wcc386 | total |
|---|---|---|---|---|
| frozen (`NW = 0`), no bias | node | 241409 | 287719 | 529128 |
| adaptive weight, no bias | node | 240043 | 286173 | 526216 |
| weight + bias | node | 239475 | 284879 | 524354 |
| weight + bias, bias rate ×10 | node | 239380 | 284231 | 523611 |
| same | `c1`, node | 237958 | 284860 | 522818 |
| same | `c1`, `c2[7:4]`, node | 236154 | 281205 | **517359** |
| same | `c1`, `c2[7:2]`, node | 236247 | 281192 | 517439 |
| same | `c1`, `c2`, node | 236012 | 282337 | 518349 |
| same | `c1[7:4]`, `c2[7:4]`, node | 237035 | 280488 | 517523 |
| same, SSE `W = 0.81` kept | `c1`, `c2`, node | 236847 | 282141 | 518988 |

The mixer's table is small (2^20 contexts × 36 B = 38 MB) and its cost per
bit negligible next to the SSE cells.

**Weight domain** (`WDOM`): the parametrization `w = f(W)` decides the box
and the shape of the Newton step; only `f'` and `f''` change in the update.
At the seeds above (rates chosen for the logistic domain; the linear and
free forms take a 4× larger effective step, so they were also tried at a
quarter of the rate):

| domain | box | book1 | wcc386 | total |
|---|---|---|---|---|
| logistic `w = sq(W)` | `W` in ±8 | 236097 | 281225 | **517322** |
| linear `w = W` | `w` in [0, 1] | 235951 | 281425 | 517376 |
| linear `w = W` | `w` in [−1, 2] | 236810 | 282882 | 519692 |
| linear, rate ÷4 | `w` in [−1, 2] | 236940 | 282276 | 519216 |
| reciprocal `w = 1/V` | `V` in [0.5, 64] | 237401 | 282759 | 520160 |
| log `w = exp(W)` | `W` in ±8 | 253733 | 298255 | 551988 |
| free `W1·s1 + W2·s2` | each in [−1, 2] | 236809 | 283466 | 520275 |
| free, rate ÷4 | each in [−1, 2] | 236811 | 282389 | 519200 |

Bounded to [0, 1] the domain hardly matters; every form that lets the
weight extrapolate beyond the two inputs, or scale the confidence freely,
loses 2–3K, and the log domain much more.  With per-context Newton steps
and few events per context, the self-damping of the logistic map near
the ends is worth more than the extra freedom.  `WDOM` stays a knob at 0.

## 7. Cost, and how to trade it

| | no SSE | SSE, defaults |
|---|---|---|
| memory | ~6 MB | 2^24 cells × 96 B = 1.6 GB |
| time, book1 | ~1.0 s | ~5.7 s |

The stage does two full `Counter` predictions and updates per bit on top of
the order-1 one, over a table that does not fit any cache.  About 0.7 s of
the run is allocating and identity-initializing the table (row-major; the
remainder is page faulting).  The levers, with their measured cost from §6:

* `S0_HBITS` / `SSE_MAXCELLS_LOG`: rows.  400 MB costs ~3K (`wcc386`),
  100 MB ~12K.
* `S0_NB`: cells per row; 6–8 is the sweet spot anyway.
* `-DS0_ADAPT_WR=0`: 36-byte cells, ~3K worse, ~35% faster.
* `-DS0_ADAPT_MW=0 -DS0_ADAPT_K=0` on top: 12-byte cells, but two thirds of
  the gain is gone; at that point a classic count-rate APM is the better
  design.

---

## 8. Notes and possible extensions

* **One stage, one context.**  The design is deliberately a single table; the
  usual next step is a second stage on another context (e.g. a sparse or
  word-level one) whose outputs are mixed, which would resolve the
  text/binary tension in §6.2 instead of compromising on the `c3` mask.
* **Learned blend.**  `W` is a constant.  A per-row or per-bucket learned
  weight (a two-input mixer in the stretch domain) is the natural
  replacement and would let sparse rows fall back to `p_in` on their own.
* **Cheaper cells.**  The counter stores 24 floats; the traces and optimizer
  states could be halved in width, or the rate optimizer shared per row
  instead of per cell.  Not explored.
* **Quantizer shape.**  The buckets are uniform in the stretch domain.  An IDX
  threshold mapping would make the cut points tunable (the machinery exists
  for that), at the price of a small table for the interpolation weights.
