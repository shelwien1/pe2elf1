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
x     = (s + LIM) · (NB-1) / (2·LIM)           bucket coordinate, 0 .. NB-1
j     = floor(x),  wt = x - j                  two neighbouring cells c[j], c[j+1]
p0    = c[j].PredictF(),  p1 = c[j+1].PredictF()
p_sse = sq( st(p0) + wt·(st(p1)-st(p0)) )      interpolated in the stretch domain
```

The stage keeps the two cells' own predictions and hands each back to its
`C_Update()` (the cell's former `pK` slot holds its update count, §6.11).
The stage returns `p_sse` as is; blending it with `p_in` is the mixer's job
(§3.7).
Before the mixer existed the stage carried a fixed blend weight `W`, in the
probability or the stretch domain; the stretch domain won at every `W`
(§6.4), and that is the form the mixer generalizes.  Interpolating in the
stretch domain rather than in probability was worth a further 600 bytes.

### 3.4 Update

```
UPD=1:  c[j].C_Update(bit);          c[j+1].C_Update(bit);
UPD=2:  c[j].C_Update(bit, 1-wt);    c[j+1].C_Update(bit, wt);     (each floored at UPMIN)
```

`Counter::C_Update(bit, g)` takes the weight of the observation: the loss
gradient is scaled by `g`, and in the count recursion every `wr` becomes
`wr·g` and every injected count `g`, so the RTRL traces remain the exact
derivatives of the weighted recursion (`g=1` is bit-identical to the plain
update).  `UPD=2` is therefore the interpolated update of `sh_SSE1.inc` —
the event is split between the two cells in proportion to their share of
the prediction — without the explicit residual arithmetic: each cell runs
its own Newton step on a fractional event.  At the first seeds it measured
best (§6.6); after the parameter passes the full update of both cells is
ahead again, and the knob is left to the optimizer.  Updating the nearer
cell alone was clearly worst and is gone from the source.

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
in place the SSE's own blend is gone (its output is used as is; keeping
the tuned 0.81 blend underneath the mixer measured 600 bytes worse).

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
Number T0, ON                 # init mass, stage on/off
Number UPD, UPMIN             # update rule (both cells / proportional + floor)
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
  constants — counter floats and the SSE knobs `ON`, `NB`, `HBITS`, `LIM`,
  `T0`, `UPD`, `UPMIN` — from either.
* **Clamps at the point of use.**  `SSE_Ctr::Init()` clamps `nb` to
  `[2,64]`, `HBITS` to `[8,30]`, `LIM` to `[0.25,16]`, `T0`, `W`, and caps the
  cells at `2^SSE_MAXCELLS_LOG`, so any bit pattern the optimizer visits runs
  (IDX-FORMAT.md §5).
* **Compile-time only**: the adaptation flags `S0_ADAPT_WR/MW/K` (they change
  the cell layout), the cell cap `SSE_MAXCELLS_LOG` and the dispatcher's
  range `SSE_NB_MAX` (16; the measured optimum is 6–8); all can be given on
  the build line (`CXXEXTRA="-DS0_ADAPT_WR=0" ./gc.sh tune`).
* **Storage** is declared in the templates with the IDX `Table()` line
  (`IDX/sh_model-S0.inc`: `Table( SSE_Cell, %M%tbl, sse_table_cells(...) )`,
  likewise `Mix2_Cell` in `sh_model-M0.inc`): in the shipping build the
  generated `S0_T`/`M0_T` hold the cells as fixed array members (static
  storage, 1.7 GB of BSS, no allocation anywhere), in the tuning build as
  pointers allocated by `S0_Init()`/`M0_Init()` (IDX-FORMAT.md §9).  The
  size helpers are constant expressions in the shipping build, runtime
  values in the tuning one; the components receive the table base and the
  row count at `Init()` and own nothing.  `MakeTables` in the templates
  routes the knobs and masks into the `_p.inc` so that the cell types are
  complete before the `_h.inc` declares the arrays.
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
climb would.  `IDX/optv.pl` (same arguments and export format) is the
value-step climb of §6.11: it moves a `Number` knob by ±step of its value,
from a quarter of it down to 1/256, and revisits knobs in the order of
their last gain -- for the 18-20-bit rate knobs that reaches in a dozen
evaluations what the bit flips of `opt.pl` mostly cannot (a high bit is a
huge jump, a low one is noise).  The two builds are byte-identical only with FMA contraction
and the unsafe float algebra off (`-ffp-contract=off
-fno-unsafe-math-optimizations` in `gc.sh`): with knobs folded in one
build and runtime in the other, gcc otherwise contracts, reassociates,
replaces divisions by reciprocals or refactors common terms differently
in the two, and the streams drift by a byte or two (found by tracing the
per-bit probabilities of both builds to the first divergent bit).  The
init-time `logf`/`expf` of knob-derived values go through `rt_logf` /
`rt_expf` for the same reason: the shipping build would otherwise fold
them with MPFR while the tuning build calls libm.  The wrappers take a
`volatile` copy of the argument: a plain static wrapper is inlined and
folded anyway under `-O3 -flto`, which surfaced when a tuner pass moved
`kMax` to 1.0311 (libm's `logf` is an ulp below MPFR's there, so the K box
wall `KYHI` differed and the streams drifted by a byte).  Every knob a
consumer reads must also be clamped to its meaningful range: a pass had
set `C0_leak2` above 1, the RTRL traces of long-lived cells overflowed to
inf, and `clip()` of inf/NaN under `-ffinite-math-only` differs between
the builds.  `-DTRACE_P` writes the per-bit `(p1, p2, pf, p)` of a build
to `$TRACE_P`; comparing two such traces gives the first divergent bit,
and a per-update dump of that cell the field.  `opt.pl` gained an optional map-name regex (third argument) and
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
S0.S0_Init(); M0.M0_Init();                            // tables (tuning build: allocate)
sse.Init( S0.S0_tbl, S0_Cx_Volume * S0_Cx3_Volume );
mix.Init( M0.M0_tbl, M0_Cx_Volume );
...
p1  = o1[c1][cxt].PredictF();                          // primary, P(bit=0)
cx  = S0_MakeCx(c2, c1, cxt) * S0_Cx3_Volume + S0_MakeCx3(c3);
p2  = sse.Predict( cx, p1 );                           // SSE(p1)
pf  = mix.Mix( M0_MakeCx(c2, c1, cxt), p1, p2 );       // learned blend (§3.7)
p   = uint( clamp( pf * SCALE ) );
bit = rc.rc_BProcess( p, bit );
e_f = pf - [bit==0];  w = mix.weight();                // final error (§6.11)
o1[c1][cxt].C_Update( bit, p1, 1, e_f*(w + (1-w)*sse.dz2_dz1()) );
sse.Update( bit, e_f*(1-w) );
mix.Update( bit );
```

`S0_MakeCx`/`S0_MakeCx3` are generated by `idx2inc.pl` from the `Index` blocks
(`MOD/sh_model-S0_p.inc`).  With the mixer bypassed (`M0_ON = 0`) the final p is the
SSE output as is (533676 at the tuned knobs), and with the SSE bypassed as
well (`S0_ON = 0`) the coder reproduces the pre-SSE stream to within one
byte (344899 / 309702 against 309703): the original binary folded
`pK·SCALE` into `SCALE/(1+e^-x)` under `-ffast-math`, one rounding, which
cannot happen once `pK` is materialized as the SSE's input.  Those are the
regression checks for the whole path.

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
order-1 contexts with fixed-rate cells.

*The linear-domain quantizer and the history sub-rows were removed from
the source after these measurements (commit 431db07 is the last with
them); the tables stand as the record.*

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
to the proportional one).  Linear stays; the higher-degree kernels were
removed from the source with the other losing variants (commit 431db07).

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
stage on the order-1 model.  Its code (`DM`, `HMODE 2`, `DSIM`, the
register and `duc_sim`) was removed from the source after these
measurements (last present in commit 431db07).

### 6.8 Tuned result

Three `opt.pl` passes: one over the S0 file (two climbs on disjoint
halves, about 1.3k evaluations of the pair, 7 s each on 4 cores), then,
with the mixer in place, one over the 25 mixer knobs and one over all 74
S0 knobs jointly; each export folded into its `.idx`:

| | no SSE | SSE, seeds (§6.5) | SSE tuned | + mixer, all tuned |
|---|---|---|---|---|
| book1 | 344899 | 248078 | 241409 | **235043** (−31.9%) |
| wcc386 | 309703 | 295788 | 287719 | **280507** (−9.4%) |
| book1 + wcc386 | 654602 | 543866 | 529128 | **515550** (−21.2%) |
| book1wcc (concatenation) | 656163 | | 531716 | **518841** (−20.9%) |
| book1 encode / decode | 1.0 s | | 5.7 s | 5.8 s / 5.7 s |

All three roundtrips verify (`t1.sh`), and the shipping and tuning builds
produce the same bytes.  What the first S0 pass moved, in order of effect:

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
right; `K`, `T0`, the blend and the context masks carry the result.  The
mixer pass (§6.9) and the joint S0 pass on top of it were worth a further
1.2K and 0.6K.  A pass over the C0 knobs (the order-1 model tuned as an
SSE *input* rather than as the final predictor) was tried before the
mixer existed and its changes did not transfer once the mixer was in
place; it remains the obvious next step on the final configuration.

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
the ends is worth more than the extra freedom.  The logistic map is the
one in the source; the other domains were removed (commit 431db07).

**Mixer pass.** One `opt.pl` pass over the 25 mixer knobs (node mask
included) took 517322 → 516174: it widened the mixer's `c2` context to six
bits, raised the bias box and both step clips, and lowered the weight's
curvature clip.

### 6.10 The step rule of `ParamUpdater`

`ParamUpdater` is a diagonal online Newton method: `D = EMA(−∂ln p/∂θ)`,
`R = EMA((∂p/∂θ)²/p² − ∂²p/∂θ²/p)` (the exact Hessian of `−ln p`, not
just the squared gradient), `step = NW·D/(R+inc)`, clipped and boxed.
Muon-style ideas apply to it only in their scalar form — Muon
orthogonalizes a *matrix* momentum, which for independent per-context
scalars collapses to normalized momentum (a step of fixed size along the
momentum, no curvature) — so the rules compared on the mixer were: Newton,
normalized momentum `D/(|D|+inc)`, Adam `D/(√EMA(g²)+inc)`, each also at a
quarter of the rate since their effective step sizes differ, plus a
curvature prior `R0` for the Newton rule (the mixer's contexts see ~10
events on average, so the EMAs barely warm up).  Mixer at the tuned seeds
(515550):

| rule | on | rate | book1 | wcc386 | total |
|---|---|---|---|---|---|
| Newton | — | tuned | 235043 | 280507 | **515550** |
| normalized momentum | weight | tuned / ÷4 | 235752 / 235599 | 281282 / 281399 | 517034 / 516998 |
| normalized momentum | bias | tuned / ÷4 | 245179 / 237946 | 287584 / 284251 | 532763 / 522197 |
| Adam | weight | tuned / ÷4 / ÷2 | 235126 / 235652 / 235309 | 281067 / 281545 / 281277 | 516193 / 517197 / 516586 |
| Adam | bias | tuned / ÷4 | 236817 / 235549 | 281999 / 281969 | 518816 / 517518 |
| Newton, `R0` = 0.25 / 1.0 | weight | tuned | 235157 / 235372 | 280680 / 281107 | 515837 / 516479 |
| Newton, `R0` = 0.25 / 1.0 | bias | tuned | 235105 / 235238 | 280816 / 281375 | 515921 / 516613 |

With an exact second derivative available per parameter, the Newton
step beats both scale-free rules, by a little on the weight and by a lot
on the bias (whose gradient scale varies most across contexts, which is
exactly what the curvature normalizes away), and the EMAs are better
started empty than from a prior.

Three refinements of the Newton rule were then tried on the mixer:

| variant | book1 | wcc386 | total |
|---|---|---|---|
| Newton (above) | 235043 | 280507 | 515550 |
| Nesterov look-ahead (`OPT=1`), weight | 235029 | 280517 | 515546 |
| Nesterov, bias | 234992 | 280181 | 515173 |
| Nesterov, both | 234978 | 280193 | 515171 |
| Nesterov, both, bias rate at its box top | 235025 | 280132 | **515157** |
| 2×2 coupled (weight, bias) solve (`X2`), cross weight 1 / 0.5 / 2 | 235009 / 235009 / 235263 | 280551 / 280629 / 280662 | 515560 / 515638 / 515925 |
| 2×2, det guard 0.1 / 0.4 | 235015 / 235006 | 280559 / 280528 | 515574 / 515534 |
| sign-agreement gain ×1.2 / ×0.6 | 236221 | 281349 | 517570 |
| gain ×1.1 / ×0.8 | 236223 | 281308 | 517531 |
| gain ×1.05 / ×0.95 | 235817 | 280971 | 516788 |
| gain ×1.2 / ×0.6, box [0.125, 4] | 242596 | 286946 | 529542 |

* **Nesterov** (the step is taken from `β·D_new − g` instead of `D_new`;
  `Accum` returns the gradient so nothing is stored) is a small,
  consistent gain, essentially all on the bias, and wants the bias rate a
  little higher.  It is the one refinement kept: the `NAG` switch of a
  `ParamUpdater` config, on for the mixer (`NAGw`, `NAGb`), off for the
  counters.
* **The 2×2 coupled solve** (EMA of the cross curvature of `−ln p`,
  `gW·gb/p² − ∂²p/∂W∂b/p`, then the counters' det-guarded solve with joint
  ray clipping) is neutral: the weight and the bias of a context barely
  interact at these rates.
* **Step-size gain on sign agreement** (a per-state multiplier grown when
  the new gradient agrees with the momentum, shrunk otherwise) loses at
  every setting: the Newton step already normalizes the scale, and a
  gain that reacts to single events only adds noise in contexts that see
  a handful of them.

The counters were never switched away from Newton.  The normalized
momentum, Adam, curvature prior, coupled solve and gain code was removed
after these measurements (last present in commit 9d3e993).

**The offline optimizer** (`opt.pl`) is a different problem: coordinate
descent over bit patterns with one full corpus run per evaluation.  Its
cost is the number of evaluations, and the useful improvements are about
that rather than about the search direction: evaluating several
candidate flips of one knob in parallel and keeping the best (the corpus
runs are independent), climbing values by ±1/±2/±4 steps rather than
single bits (a high bit flip is a huge jump that almost never helps, a
low one is noise), racing — aborting a run as soon as its size at a
checkpoint exceeds the incumbent's — and visiting knobs in the order of
their last measured effect.

### 6.11 Young-cell step schedule, end-to-end gradients (2026-09)

Follow-up to `coder0_optimizer_improvements_v3.md` (its A1, A2, A3, A4, F1,
F4, F13), measured on the fully tuned pipeline (C0 retuned in place):
reference **235391 / 275195 / 512847** (book1 / wcc386 / book1wcc, total
1023433).  Every change is a knob that defaults to bit-identical output in
both builds; deltas are in bytes on the three files, "total" over all
three; the tuning build patched with `setp.pl`.

**Cell age (A1).**  Every `Counter` and `Mix2` cell counts its updates.
The counter keeps it in the former `pK` slot -- `PredictF()` no longer
stores the prediction, the caller passes it back to `C_Update()` -- so the
cells stay 96 B; the mixer cell grows 24 → 28 B.  The step limit becomes
`stepMax·(1 + A/(1 + age/B))`: a fresh cell may move up to (1+A)× the
tuned limit, a settled one is unchanged.  Knobs `AGAm`/`AGAk` (mw, K),
`AGAu` (the u/v ray clip), `AGB` (B, in 1/16 updates) per counter stage,
`AGAw`/`AGAb`/`AGB` on the mixer.  The v3 document had measured the S0
version on Canterbury files; here:

| schedule | book1 | wcc386 | book1wcc | total |
|---|---|---|---|---|
| S0 mw+K, A=1 B=4 | −82 | −8 | −61 | −151 |
| S0 mw+K, A=3 B=4 | −144 | −12 | −114 | −270 |
| S0 mw+K, A=6 B=4 | −152 | −14 | −132 | −298 |
| S0 mw+K, A=3 B=16 | −163 | −16 | −135 | −314 |
| S0 mw only, A=3 B=4 | −119 | −16 | −110 | −245 |
| S0 K only, A=3 B=4 | −33 | −4 | −20 | −57 |
| S0 u/v ray clip only, A=3 B=4 | −97 | +2 | −69 | −164 |
| **S0 mw+K+u/v, A=3 B=16** | −279 | +2 | −195 | **−472** |
| S0 mw+K+u/v, A=3 B=32 / B=64 | −273 / −252 | +7 / +10 | −178 / −152 | −444 / −394 |
| S0 mw+K+u/v, A=6 B=32 | −165 | +13 | −91 | −243 |
| C0 mw+K, A=1 B=4 / A=3 B=4 | −40 / −78 | +54 / +163 | +11 / +91 | +25 / +176 |
| C0 mw / K / u alone, A=1 B=4 | −18 / −20 / −6 | +5 / +49 / +14 | −6 / +20 / +8 | −19 / +49 / +16 |
| M0 weight, A=3 B=4 | −11 | +39 | +29 | +57 |
| **M0 bias, A=3 B=4** | −43 | −217 | −238 | **−498** |
| M0 bias, A=6 B=4 / A=3 B=8 / B=2 / B=1 | −48 / −44 / −41 / −36 | −191 / −217 / −217 / −215 | −209 / −239 / −237 / −234 | −448 / −500 / −495 / −485 |
| S0 (A=3 B=16, three axes) + M0 bias (A=3 B=4) | −323 | −210 | −431 | −964 |

The S0 schedule is a text gain (0.12%) and neutral on x86, the mixer-bias
one an x86 gain (0.08%), and they add.  The order-1 cells and the mixer
weight do not want it (their young cells are rare: 79% of C0 updates hit
cells older than 256 events), and they still do not on the stack below
(C0 mw −24, K +274, u +160, M0 weight +84).

**End-to-end gradients (A2).**  A cell minimizes its own `−ln pK` while the
coder pays `−ln p_final`.  With `e_f = p_final − [bit==0]` the gradient of
the final loss w.r.t. a cell's logit output is `e_f · dz_f/dz_cell`:
`(1−w)·wt_j` for an SSE cell (mixer weight `w` on the order-1 input, `wt_j`
its interpolation share) and `w + (1−w)·(z_c1 − z_c0)·(NB−1)/(2·LIM)` for
the order-1 cell (the second term: its prediction is also the SSE's index
and moves the interpolation point).  `C_Update(bit, pK, g, ef)` takes that
error and scales the parameter gradients by `((1−E2E)·e_own + E2E·ef) /
e_own` -- the gradient direction of the blended objective, the curvature
(and so the Newton normalization) staying the cell's own.  Knobs `C0_E2E`,
`S0_E2E` (/256).

| objective | book1 | wcc386 | book1wcc | total |
|---|---|---|---|---|
| S0 E2E = 0.25 / 0.5 / 1.0 | −13 / +50 / +924 | −39 / −71 / −79 | −85 / −101 / +629 | −137 / −122 / +1474 |
| S0 E2E = 1.0, S0 NW ×2 | +258 | −95 | +66 | +229 |
| C0 E2E = 0.25 / 0.5 | −74 / −148 | −202 / −408 | −286 / −580 | −562 / −1136 |
| **C0 E2E = 1.0** | −339 | −676 | −1146 | **−2161** |
| C0 E2E = 1.0, NWm / NWk / NWu / NWv ×1.5 | −407 / −344 / −400 / −377 | −680 / −718 / −621 / −714 | −1178 / −1216 / −1150 / −1223 | −2265 / −2278 / −2171 / −2314 |
| C0 E2E = 1.0, all four NW ×1.5 / ×2 | −497 / −552 | −714 / −678 | −1313 / −1304 | −2524 / −2534 |
| C0 E2E = 1.0 + S0 E2E = 0.25 / 0.5 | −355 / −295 | −710 / −739 | −1224 / −1232 | −2289 / −2266 |

Training the order-1 cells on the final loss is the largest single item
(0.2%), and it wants the C0 rates about 1.5× higher (the C0 `NW*` and
step patterns were widened by two bits so the tuner can follow).  The SSE
cells gain a little from a quarter of the final error and lose from all of
it: their share `(1−w)·wt` is small, and with the curvature unchanged the
step shrinks with it (raising their rates back does not recover it).

**The stack** and the variants tried on it:

| configuration | book1 | wcc386 | book1wcc | total |
|---|---|---|---|---|
| S0 schedule + M0 bias schedule + C0 E2E 1.0 with NW ×1.5 | −798 | −910 | −1718 | −3426 |
| + S0 E2E = 0.25 | −833 | −971 | −1844 | **−3648** |
| + C0 kMax 0.94 → 1.0 / 1.2 / 1.5 | −892 / −982 / −967 | −992 / −945 / −737 | −1897 / −1830 / −1413 | **−3781** / −3757 / −3117 |
| `E2E_NOCHAIN`: order-1 chain = mixer weight only | −561 | −859 | −1489 | −2909 |
| `E2E_SSEI`: SSE cells on the SSE output's own loss | −823 | −904 | −1745 | −3472 |
| `E2E_HESS`: the E2E error also scales the curvature | −889 | −878 | −1556 | −3323 |
| `UV_NW_AFTER` (F4): NW after the 2×2 solve; alone on the base | −755; +94 | −1009; −33 | −1821; +51 | −3585; +112 |
| `UPD = 2` (proportional SSE update), floor 0 / 0.25 | −365 / −382 | −1043 / −1039 | −1783 / −1728 | −3191 / −3149 |

The SSE-index term of the order-1 chain is worth 740 bytes; the SSE-only
target, the end-to-end curvature (the exact Newton step of the final
loss: it re-inflates the small-share cells' steps), F4 and the
proportional update all lose.  The four variants stay in the source as
compile-time diagnostics (default 0).

**Hypergradient on the rates (A3), rejected.**  A stage-wide multiplier of
NW per axis, moved by `exp(±ρ)` every 2^N updates against the sign of the
summed `dL_{t+1}/dNW = dp_inv_{t+1} · D_t/(R_t+inc)` (the previous step
of each cell, clip and box ignored).  On the −3781 stack: C0 ρ=0.01 N=4096
+1042, ρ=0.03 +1170, N=1024 +1262; S0 ρ=0.01 +861; M0 +8111; at ρ=0.0024
N=65536 it is neutral (C0 +7, S0 +44).  It only hurts as it acts, on both
files, so the code was removed.

**Tuned result.**  `IDX/optv.pl` over the C0/S0/M0 rate, step, momentum,
damping and seed knobs on top of the stack (objective book1 + wcc386 +
book1wcc; the S0/M0 schedule knobs in a pass of their own).  Pass 1 over
79 knobs: 1019646 → 1017882; pass 2 (the biggest gains first, the ceilings
of `C0_M2_k` / `M0_NWb` widened): → 1017100, of which 9 bytes were the
leak-overflow artifact above.  What moved, in order of effect: S0 `T0`
2.0 → 1.3, `M0_W0` 0.47 → 0.59 (the end-to-end-trained order-1 input is
worth more to the mixer), S0 `LIM` 7.25 → 3.1, S0 `E2E` 0.25 → 0.62, C0
`M2_k` from τ = 43 to 2048 events (a decade away from its seed, found by
the wider steps; hence `OPT_SCAN`), `M0_Bclip` 2.0 → 2.6, C0 `kStep`,
S0 `K`, S0 `NWv`.

| | book1 | wcc386 | book1wcc | total |
|---|---|---|---|---|
| 061 baseline (all tuned, C0 in place) | 235391 | 275195 | 512847 | 1023433 |
| A1 + A2 stack, hand seeds | 234499 | 274203 | 510950 | 1019652 |
| + optv.pl passes 1–2 | **234269** | **273275** | **509562** | **1017106** (−0.62%) |

Roundtrips verify and both builds are byte-identical.  Encode time is
unchanged within noise (the chain and the schedule are a few
multiplications per bit).

## 7. Cost, and how to trade it

| | no SSE | SSE, defaults |
|---|---|---|
| memory | ~6 MB | 2^24 cells × 96 B = 1.6 GB |
| time, book1 | ~1.0 s | ~5.7 s |

(The mixer cell is 28 B since §6.11 added its update count.)  The stage
does two full `Counter` predictions and updates per bit on top of the
order-1 one, over a table that does not fit any cache.  About 0.7 s of
the run is identity-initializing the table (row-major; the remainder is
page faulting of the static storage).  The levers, with their measured cost from §6:

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
