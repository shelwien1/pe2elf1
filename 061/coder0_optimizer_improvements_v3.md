# coder0_.cpp — optimizer improvement proposals (r3)

Scope: `ParamUpdater`, `Counter<CP>::C_Update` with its RTRL traces, and how `SSE_Ctr` and `Mix2` drive them. r2 merged eight external reviews (ds, gem, grok, gpt, kimi, mmax, qwen, zai), checked their claims against the source, and added measurements from an instrumented build. r3 cross-checks all of that against SSE-DESIGN.md, which already records tests for many of the proposals (§1.6), and adds three measurements it prompted:
- the young-cell step schedule split by stage (§1.4);
- `[[no_unique_address]]` cell sizes (§2.2);
- the C0-versus-S0 tuning status (§0, A0).

My own measurements use Canterbury files with constants tuned on book1/wcc386, so they are indicative; any effect under about 0.1% needs confirming on the tuning corpus. SSE-DESIGN's numbers are on book1 + wcc386.

---

## 0. Summary

- **Many proposals were already tested (SSE-DESIGN.md, §1.6).**
  - Losers against plain Newton: Adam, normalized momentum, and a curvature prior R0 on the mixer. A sign-agreement step gain loses at every setting.
  - Neutral: the mixer's 2×2 coupled (W, b) solve.
  - Nesterov is kept on the mixer only.
  - The proportional SSE update (UPD = 2) won at the seeds and lost after tuning.
  - Also losers: wider SSE interpolation kernels, a linear quantizer, history sub-rows and delayed-update rows.
  - My counter-side tests agree with the mixer-side ones: R0 loses, NAG is neutral.
- **The C0 constants have never been tuned with the SSE and mixer in place.** SSE-DESIGN §6.8 calls this the obvious next step. Inside the pipeline, C0's mw/K adaptation hurts binary, and every C0-side knob I tested either loses on binary or is neutral. A C0 retune on the final configuration (A0) moves to the top. My C0 results are measured away from the pipeline optimum.
- **Young SSE cells want larger step limits.** Applied to S0 only, an age-scheduled stepMax wins on all three files: −366…−393 total, text −0.13…−0.16%, binary −0.08…−0.09% (A1). The same schedule on C0 loses on binary, which fits the point above.
- **The S0 and M0 local knobs are at a local optimum; C0's are not known to be.** R0 floors, NAG and positive-part curvature land within ±0.1% or lose. Bias correction and shared-R warm starts push young-cell steps the same way as R0 floors (which lose on both counters and mixer), so they are predicted to lose.
- **The o1 cells optimize a proxy.** Their own code length is 24–62% above the final output, yet the mixer weight on them is 0.36–0.44. A0 is the hyperparameter-level fix; per-event end-to-end training (A2) is the online one.
- **Most review errors come from stale header notes or constants the reviewers didn't read.**
  - C0's `G2_v` is 4 (clip 0.125), so the d²p and ww-trace channels are live.
  - The `stP_min` gate skips near p = 0.5, not at the extremes, and practically never fires.
  - NAG is wired only for mw/K.
  - With UPD = 1 shipped, g ≡ 1 everywhere.
- **Housekeeping.** `gc.sh` already builds with `-ffp-contract=off -fno-unsafe-math-optimizations` (SSE-DESIGN §4.1); my stream-drift measurement reproduces the reason. `[[no_unique_address]]` on the Counter members gives the 36 B / 12 B cells SSE-DESIGN intends (currently 44 / 20 B), which saves 134 MB in either reduced-cell configuration.

---

## 1. Measurements

### 1.1 Setup

gcc `-O2 -march=native -ffp-contract=off`. Files: Canterbury alice29.txt (152089 bytes), lcet10.txt (426754), mapsdatazrh (285886, binary). Base output: 43488 / 113593 / 175575 bytes. Decode verified for the base and the PCURV variant.

The instrumented build adds counters and a per-cell `age` field without changing the arithmetic, and produces byte-identical output under `-ffp-contract=off`. Ablations are not retuned: the frozen seeds are the ones tuned *with* adaptation on, so "adaptation off" deltas are upper bounds on its value.

### 1.2 Where the bytes are

Ideal code length per stage, from the model probabilities, in bytes:

| file | o1 alone | SSE output | final | mixer weight on o1 |
|---|---|---|---|---|
| alice29 | 66039 | 44828 | 43484 | 0.437 |
| lcet10 | 184176 | 116151 | 113589 | — |
| mapsdatazrh | 217396 | 181518 | 175571 | 0.361 |

Ablations (Δ bytes: alice / lcet10 / maps, and the total):

| disabled | alice | lcet10 | maps | total |
|---|---|---|---|---|
| all C0 adaptation | +1321 | +2532 | +932 | +4785 |
| C0 wr (u,v) | +318 | +621 | +829 | +1768 |
| C0 mw | +372 | +549 | −192 | +729 |
| C0 K | +211 | +304 | −333 | +182 |
| all S0 adaptation | +820 | +3174 | +1429 | +5423 |
| S0 wr | +192 | +571 | +223 | +986 |
| S0 mw | +120 | +509 | +241 | +870 |
| S0 K | +214 | +735 | +564 | +1513 |

SSE-DESIGN §3.6/§7 has the corresponding book1 + wcc386 numbers:
- Plain 12-byte cells lose two thirds of the stage's gain (626021 vs 577647, measured before the mixer existed).
- `S0_ADAPT_WR=0` costs ~3K (0.6%) and runs ~35% faster.

My all-S0-off delta (0.8–2.8%) is far smaller than the pre-mixer 8.4%. Likely causes: the mixer now absorbs part of it, and the frozen seeds are the adapted-tuning ones.

### 1.3 Per-axis optimizer behaviour

Values are alice / maps. "Inc<0" is the share of updates where the R increment `g² − clip(d²p)` is negative. "Clip" is the share of steps limited by stepMax; for u/v it is the ray clip.

| axis | R = 0 | R at cap | inc<0 | d²p clip binds | clip, all | clip, age 1–15 | clip, age 256+ | at box |
|---|---|---|---|---|---|---|---|---|
| C0 mw | 3.6 / 8.6 | 0 / 0 | 40 / 56 | 0 / 0 | 8.2 / 21 | 84–95 / 73–87 | 1.2 / 1.1 | 2.4 lo / 0 |
| C0 K | 0 / 0 | 4.4 / 4.4 | 0 / 0 | — | 38 / 14 | 65–90 / 22–73 | 36 / 4.4 | 44 (41 hi) / 12 |
| C0 u,v | 0 / 0 | u 0.2 / 2.0 | v 16 / 28 | v 6 / 34 | ray 5.9 / 29 | ray 61–92 / 56–82 | ray 0.1 / 0.2 | u 0.2 / 3.3 |
| S0 mw | 0.5 / 0.4 | 0 / 0 | 24 / 14 | 0 / 0 | 55 / 83 | 96–100 / 95–100 | 1.2 / 1.0 | 0 |
| S0 K | 0 / 0 | 0 / 0 | 4.4 / 4.0 | 92 / 97 | 14 / 32 | 9–63 / 11–40 | 0 / 0 | 0 |
| S0 u,v | 0 / 0 | 0 / 0 | u 19 / 17 | u 23 / 19 | ray 25 / 18 | ray 19–36 / 27–47 | ray 1.5 / 1.7 | 0.1 |
| M0 W | 1.0 / 12.1 | 4.8 / 0.5 | 22 / 30 | 0.7 / 0.4 | 3.3 / 16 | — | — | 0 |
| M0 b | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 1.1 / 23 | — | — | 9.8 / 2.9 |

Structure counters:

| | C0 alice / maps | S0 alice / maps |
|---|---|---|
| updates at age 0 | 0.2% / 2.8% | 7.2% / 44.7% |
| updates at age < 16 | 2.6% / 22% | 40% / 76% (lcet10: 26%) |
| updates at age 256+ | 79% / 37% (lcet10: 89%) | 12% / 4.5% |
| 2×2 det guard trips | 0.6% / 0.1% | 0.2% / 0.0% |
| DIAMP clamp active | 16.8% / 21.9% | 2.4% / 2.0% |
| mean R: u / v | 4.0 / 31 (cap 48 / 1856) | 1.6 / 2.5 |

### 1.4 Knob tests

Δ bytes, alice / lcet10 / maps (total). Rows touching C0 are measured with C0 constants that were not tuned for the pipeline (A0).

| change | Δ | verdict |
|---|---|---|
| R0_k = 0.25 / 1 / 4 (C0+S0) | +70/+200/+177 … +242/+645/+425 (+447 … +1312) | monotone loss |
| R0_m = 0.25 / 1 (C0+S0) | +472 / +733 total | loss |
| R0_k = 1, C0 only | +31 / +37 / −41 (+27) | neutral |
| positive-part R increment, all axes | +11 / +9 / −3 (+17) | neutral |
| positive-part R increment, C0 mw only | −3 / −7 / −4 (−14) | noise |
| NAG on C0 mw/K | +3 / +2 / +68 (+73) | loss |
| NAG on S0 mw/K | −1 / −1 / −2 (−4) | noise |
| C0 kMax 0.94 → 1.0 / 1.2 / 1.5 / 2.0 | text −13…−41, binary +50…+413 (+20 … +395) | trade-off, net loss |
| young-cell stepMax ×(1 + A/(1+age/B)), mw/K, **both stages**, A=1 B=4 | −62 / −131 / +82 (−111) | text win |
| same, both stages, A=3 B=4 | −110 / −229 / +232 (−107) | text win, binary loss |
| same, both stages, A=1 B=16 | −87 / −178 / +155 (−110) | text win |
| same, both stages, A=−0.5 B=4 (smaller cold steps) | +68 / +150 / +89 (+307) | loss |
| **C0 only**, A=1 / A=3 (B=4) | −28/−33/+177 (+116) · −53/−62/+382 (+267) | binary loss |
| **S0 only**, A=1 B=4 | −35 / −98 / −95 (−228) | win on all |
| **S0 only**, A=3 B=4 | −58 / −167 / −152 (−377) | win on all |
| **S0 only**, A=6 B=4 | −59 / −171 / −136 (−366) | win on all |
| **S0 only**, A=3 B=16 | −63 / −181 / −149 (−393) | win on all |

### 1.5 Determinism

Two builds with identical coder arithmetic (reference, plus reference with counters) under `-O2 -march=native -ffast-math` produced different streams of the same size on all three files. With `-ffp-contract=off` the streams were identical (checked on alice29). SSE-DESIGN §4.1 records the same effect between the shipping and tuning builds (a byte or two of drift), and `gc.sh` already sets `-ffp-contract=off -fno-unsafe-math-optimizations`. What remains open is cross-toolchain libm differences, which only matter if streams must decode across builds (F5).

### 1.6 Results already recorded in SSE-DESIGN.md

SSE-DESIGN numbers are on book1 + wcc386 against the stated reference. Mixer references: 515550 for the step rules (§6.10), 517322 for the weight domain (§6.9).

| test (SSE-DESIGN §) | Δ | answers |
|---|---|---|
| mixer 2×2 (W,b) solve, cross weight 1 / 0.5 / 2; det guard 0.1 / 0.4 (§6.10) | +10 / +88 / +375; +24 / −16 | qwen §3.10.1, grok §3.4, r2 B2: neutral |
| Adam `D/(√EMA(g²)+inc)` on mixer W / b, best of 2–3 rates (§6.10) | W +643, b +1968 | ds §5.1, kimi §1.3, qwen §3.14.1, mmax §3.2/§4.4, r1 OPT_ALPHA: sqrt normalization loses to Newton |
| normalized momentum `D/(abs(D)+inc)`, best rate (§6.10) | W +1448, b +6647 | qwen §3.14.3 (sign-type updates) |
| Newton with curvature prior R0 = 0.25 / 1.0 (§6.10) | W +287 / +929, b +371 / +1063 | gpt §17, mmax §2.3, zai §7, r1 F7; agrees with §1.4 on the counters |
| Nesterov on mixer W / b / both / both + bias rate at box top (§6.10) | −4 / −377 / −379 / −393 | kept on the mixer; counters: §1.4 (neutral) |
| step gain on sign agreement, 3 settings; with box (§6.10) | +1238 … +2020; +13992 | qwen §3.11, gpt §10; zai §3.3's coherence damping is a cousin |
| mixer weight domain: linear [0,1] / [−1,2] / reciprocal / log / free (§6.9) | +54 / +2370 / +2838 / +34666 / +2953 | weak evidence against the link-function swaps in ds §6.1 and grok §3.1 |
| UPD = 2 proportional update, floor 0 / 0.25 / 0.5 / 0.75, at seeds (§6.6) | −1671 / −1660 / −1382 / −892; UPD = 1 ahead again after tuning (§3.4) | gpt §3.1, kimi §7, r1 SSEI; g-aware decay itself is untested |
| nearer cell only, UPD = 0 (§6.5/§6.6) | +10901 | bounds qwen §3.7.3's stochastic single-cell update |
| quadratic / cubic B-spline SSE kernels, 8 buckets; with g = 1 (§6.6) | +1737 / +3178; +7145 / +12106 | qwen §3.7.2: wider kernels lose |
| linear-probability quantizer, 8 / 12 / 16 buckets (§6.6) | +950 / +6134 / +10356 | qwen §3.7.1 (fixed shape; tunable cut points still untested) |
| history / success sub-rows, HW = 1–2 (§6.6) | +6012 … +25975 | — |
| delayed-update counter rows (§6.7) | −1438 as an o1-only stage; +11K … +21K inside the SSE | — |
| cell type: plain 12 B / no-RTRL 36 B / full 96 B (§3.6) | 626021 / 580559 / 577647 | qwen §3.7.4 and §3.8.3, r1 §3.8 |
| rows: HBITS 21 → 19 / 17 / 15; cap 2^25 vs 2^24 (§6.3) | +3340 / +11627 / +23965; −561 | bounds the value of hash-collision fixes (mmax §2.5, qwen §3.9) |
| T0, wr and LIM seeds (§6.5) | T0 ≈ 1 best at seeds (tuned later to 2.5; the file has 2.0); faster forgetting better | background for B3 (seeds) and A1 |

---

## 2. The optimizer as it runs

Per parameter θ ∈ {u, v, x = logit-coordinate of mw, y = ln K}, per cell:

```
g   = -dL/dθ          = sign·dpK/dθ / p_t                  (dp_inv)
inc =  g² - clip(sign·d²pK/dθ² / p_t, G2)                  (exact d²L/dθ² when unclipped)
D   = mD·D - g ;   D = clip(D, G3)                         unnormalized EMA: no (1-m) factor
R   = mR·R + inc ; R = clamp(R, 0, G4)
θ  -= clip(NW·D/(R+inc_damp), stepMax) ; box
```

u/v go through `Accum` and then a 2×2 solve with `R_uv = mR_u·R_uv + CXW·(g_u·g_v/p_t² − XHW·h_x)`, a det guard, a joint ray clip, and DIAMP. mw/K go through `Update()`.

**Why the d²p term is mostly noise (Bartlett identity, from zai §1).** Under the model's own predictive distribution, E[∂²p_y/∂θ² / p_y] = Σ_y ∂²p_y/∂θ² = ∂²(Σ_y p_y)/∂θ² = 0. The d²p term therefore has zero mean for a calibrated cell and contributes only variance, plus sign flips (§1.3: increments are negative on 16–56% of updates where it is live). E[R-increment] equals the Fisher information either way. This explains the tuned state: G2 = 0 on C0 u/K and S0 v, small clips elsewhere, and C0 `XHW` = −0.30, which weights the exact cross term *against* its own sign. SSE-DESIGN §6.10 describes R as the EMA of the exact Hessian; that holds only where G2 > 0.

**Implicit anneal.** Because D and R are unnormalized, a cell's step is roughly NW·ḡ/h̄ while age < τ_D. It then falls as ~τ_D/age until age reaches τ_R or R reaches its cap, then stays constant. Relative to a normalized (bias-corrected) updater with the same steady state, cold steps are larger by τ_R/τ_D: u 138, v ~10⁴, mw 41, K 2.75 (C0), with R0 damping u/v at the start. The measurements say the tuner relies on this:
- young mw/K cells run at stepMax (§1.3);
- damping them loses, on the counters (§1.4) and on the mixer (SSE-DESIGN §6.10, "the EMAs are better started empty than from a prior");
- widening the S0 limit wins (A1).

### 2.1 Effective regimes (C0 unless noted)

| param | τ_D | τ_R | NW | inc | stepMax | G2 | R cap | R0 | regime (measured) |
|---|---|---|---|---|---|---|---|---|---|
| x (mw) | 32 | 1298 | 0.0311 | 3.2e-3 | 0.0312 | 1.03 | off | 0 | cold: sign-SGD at stepMax; warm: Newton with live, noisy d²p; R = 0 on 3.6–8.6% |
| y (ln K) | 16 | 44 | 0.0078 | 1.2e-5 | 0.0154 | 0 | 5 | 0 | empirical-Fisher RMS; at age 1, D/R = 1/g, so clipped for 2.4e-5 < abs(g) < 0.51; clipped 36% even at age 256+ on text; at the upper wall (K = 0.939) 41% on text |
| u | 32 | 4424 | 0.0318 | 0.025 | 0.0137 | 0 | 48 | 0.58 | growing sum of g² (AdaGrad-like); cap rarely binds (0.2–2%) |
| v | 32 | 3.4e5 | 0.0312 | 0.080 | 0.0073 | **0.125** | 1856 | 0.66 | sum of g² plus a live d²p term (clip binds 6–34%); ±UV_VH box inactive |

S0: mw and K are sign-SGD while cold (mw is clipped 95–100% at ages 0–15). S0 K's d²p clip (0.031) binds on 92–97% of updates, so it acts as a ±0.031 sign term. S0 u has a live d²p term (G2 = 0.031); S0 v has G2 = 0; XHC = 0 kills the cross channel. S0's K box was raised by the tuner (kMax 1.63, K seed 1.02; SSE-DESIGN §6.8: "SSE cells want to be sharper than the order-1 counters are allowed to be").

### 2.2 Memory (compiled `sizeof`)

| config | Counter bytes now | with `[[no_unique_address]]` | SSE table (16.8M cells) now → fixed |
|---|---|---|---|
| shipped (all adaptive) | 96 | 96 | 1.61 GB |
| S0_ADAPT_WR = 0 | 44 | 36 | 0.74 → 0.60 GB |
| S0 no adaptation | 20 | 12 | 0.34 → 0.20 GB |

The o1 table is 6.3 MB; the mixer is 1.04M × 24 B = 25 MB. Dropping all S0 adaptation saves 1.27 GB and costs 0.8–2.8%; dropping only S0 wr saves 0.87 GB and costs 0.13–0.50% (§1.2; SSE-DESIGN §7: ~3K, 35% faster).

---

## 3. Code-level findings

**F1. Three stale header notes.**
- (a) "with C0_G2_u = C0_G2_v = 0 … all n?_ww? traces stay dead code": in this file C0_G2_v = 4 (clip 0.125) and S0_G2_u = 1 (0.031). The d²p channel is therefore live through `h_d`, and in C0 the ww traces also feed `h_x` (XHC = 100).
- (b) The LOGWR block says "no cross-Hessian/2x2 — diagonal Newton only", but the UV2X2 solve is unconditional.
- (c) The 050c results (344956 / 310144 / 656681) are o1-only, from before the SSE and mixer stages. With both stages bypassed, the current coder gives 344899 / 309702 (SSE-DESIGN §5).

Deleting the ww traces requires XHW·XHC → 0 *and* G2_v → 0 (C0), plus G2_u → 0 (S0); test that as one toggle.

**F2. Dead code and knobs (source hygiene only).** `grad1_clip` (all G1_* constants), `hbeta` and `efw` (declared, never defined), the `inv_pq` and `sc` arguments, and `Config_V::minVal/maxVal` are dead. A grep finds 24 IDX constants that are never read: C0_LWw/LWk/LWm, AWF, AWFo, HB_u/v/m/k, EFW_u/v/m/k, LKA, LKB, LKlo, LKhi, SMWm, SMWk, SM0, MPW, CMK, MKDET and BWT. SSE-DESIGN §4 says the rejected-proposal constants in the C0 file, S0's G1_* and P0/P1 are frozen (`!` lines), so opt.pl no longer spends evaluations on them; r2 wrongly said it did. What remains is dead source.

**F3. Redundant transcendentals.** There are 11 libm calls per o1 bit. Five of C_Update's seven recompute values that already exist: the pre-step wr pair equals the previous post-step pair, and `sq(mw)`, `exp(K)` and `st(p_mix)` equal PredictF's values. Caching them costs +12–20 B per o1 cell [mmax §2.1, ds §8.3].

**F4. NW sits inside the 2×2 RHS.** `s = H⁻¹·diag(NW)·D` scales the coupling by NW_v/NW_u (0.98). Apply NW after the solve.

**F5. Determinism.** The build flags are already handled in `gc.sh` (§1.5). Streams that must decode across toolchains would additionally need a polynomial exp2/log2 on the per-bit path, because libm differs between platforms [zai §9, kimi §7].

**F6. R collapse on mw is real but small.** R = 0 on 3.6–8.6% of C0 mw updates, and 40–56% of increments are negative. Clamping the increment at 0 is worth −14 B.

**F7. R0 is hard-wired to 0 for mw/K, and that is correct.** R0 > 0 loses on the counters (§1.4) and on the mixer (SSE-DESIGN §6.10).

**F8. NAG is wired only in `Update()`.** mw/K honor `Config_*::NAG`; u/v go through `Accum`, the 2×2 solve and `Apply`, which ignore it. NAG on u/v needs code: use `Dn = mD·D − dp_inv` in `b_u`/`b_v`. Measured on mw/K: +73 (C0), −4 (S0). On the mixer it was kept (−393, mostly on the bias).

**F9. The 2×2 is not PSD by construction.** R_uv decays with U's momentum (τ 4424) while R_v uses 3.4e5, and R_uv is uncapped while R_u and R_v are capped. The det guard absorbs this and trips on only 0–0.6% of updates.

**F10. The K gate `abs(stP) >= stP_min` is effectively off.** stP_min = 1/32768 ≈ 3e-5, and the gate skips near p_mix ≈ 0.5, not at the extremes.

**F11. The v box never binds.** UV_VH = 5.88, but DIAMP limits abs(v) ≤ (UVHI − UVLO)/2 = 2.29.

**F12. C0's K sits at kMax (0.94) on 41% of text updates.** S0's kMax was raised to 1.63 by its in-pipeline tuning; C0's was never re-examined in the pipeline. My C0 kMax sweep is a trade-off (text −13…−41, binary +50…+413) at the pipeline-untuned C0 constants. Leave it to A0.

**F13. Empty adaptation members cost 8 B per cell.** `ParamUpdater<0,…>` and `RTRLState<0>` occupy 1 byte each plus padding. Marking the five `Counter` members `[[no_unique_address]]` gives the 36 B / 12 B cells SSE-DESIGN §3.6/§7 describes, saving 134 MB of SSE table in either reduced configuration. GCC and Clang accept it in C++17 mode; MSVC needs `[[msvc::no_unique_address]]`. The layout change leaves the arithmetic untouched.

---

## 4. Proposals

Tags in brackets name the review(s) that proposed each idea or a variant. r1 and r2 are my earlier revisions. SSE-DESIGN references are to SSE-DESIGN.md.

### 4.1 Tier A — backed by measurements

**A0. Retune the C0 knobs in the full pipeline (SSE-DESIGN §6.8's "obvious next step").**
The C0 constants were tuned with the order-1 model as the final predictor. A pre-mixer C0 pass as an "SSE input" did not transfer once the mixer existed, so no C0 pass has run on the final configuration. Evidence that it matters:
- C0 mw/K adaptation hurts binary in the pipeline (§1.2).
- A1's C0 part loses on binary while its S0 part wins everywhere (§1.4).
- C0 K is pinned at 0.94 on 41% of text updates, while S0 was free to move to 1.63 (F12).
- The o1 output is a proxy: the mixer gives it only 36–44% weight, and it also indexes the SSE.

Run it with A4's tuner changes, and include C0's u/v machinery (CXW, XHW, UVDET, G2_v, leaks), which was tuned on o1 alone. This is the hyperparameter-level form of A2 and should come first.

**A1. Age-scheduled step limit for S0 mw/K (measured win on all three files).**
`stepMax_eff = stepMax·(1 + A/(1 + age/B))`, applied to S0 cells only. Measured totals: −228 (A=1, B=4), −377 (A=3, B=4), −366 (A=6, B=4), −393 (A=3, B=16). Text gains −0.13…−0.16% and binary −0.08…−0.09%, a plateau around A ≈ 3–6. Add S0_AGA and S0_AGB as IDX knobs, then let an S0 pass tune them together with the S0 mw/K rates; the tuner already drove RUinc/RVinc "down to almost nothing" (SSE-DESIGN §6.8), consistent with young cells wanting big steps.

The C0 part loses on binary (+116/+267); re-test it after A0.

State needed: an 8-bit saturating age per cell. It fits in the `pK` slot if the caller passes pK in (`SSE_Ctr` keeps c[0] and c[1]'s predictions in its per-query state), so cells stay 96 B. u/v are unaffected here: their steps are already ray-limited, so a separate schedule on `iStepU/V` would be the next test.

This runs opposite to the cold-damping proposals (gpt §5/§14, qwen §3.2, ds §5.3, mmax §3.1), which the data rejects.

**A2. End-to-end objective for the cells (r1 §3.2; derivative forms from gpt §19).**
Cells minimize their own −ln p while the coder pays the final one. In the logit domain, with e = p − [bit = 0]:
```
e_f        = p_f − [bit=0]
dz_f/dz_1  = w + (1−w)·(z_c1 − z_c0)·qscale     (second term 0 outside ±LIM)
dz_f/dz_c0 = (1−w)(1−wt),   dz_f/dz_c1 = (1−w)·wt
e_j        = (1−β)·(pK_j − [bit=0]) + β·e_f·dz_f/dz_j
g = e_j·z',   H = p(1−p)·z'² + e_j·z''         (z = K·st(p_mix); u/K keep only e²z'²)
```
Add an `AccumZ(e, z', z'', p(1−p))` path. β_C0 and β_S0 are knobs. SSEI is β = 1 for the SSE cells against the interpolated SSE loss only; unlike UPD = 2 it scales the error, not the event weight. Risk: z_1 is SSE's index. Run after A0, which captures the hyperparameter part of the same effect.

**A3. Per-file global rate adaptation (r1 §3.7) [ds §9.1, kimi §1.2, grok §4, qwen §3.17.2].**
Text and binary want opposite settings in several places:
- C0 mw/K adaptation helps text and hurts binary (§1.2);
- the SSE c3 mask (SSE-DESIGN §6.2);
- C0 cold steps (§1.4).

Use a one-step hypergradient per stage and axis: `A_acc += (dL/dθ)·D/(R+inc)` (D and R taken before `Accum`), with `NW ← NW·exp(ρ·sign(A_acc))` every N events, boxed to [NW₀/4, 4·NW₀]. Keep it global; per-cell gains are noise, as the sign-agreement gain showed (SSE-DESIGN §6.10). Caveat: OPT_HBETA was "file-antagonistic". Accept only on held-out files.

**A4. Tuner (opt.pl) changes.**
SSE-DESIGN §6.10 already lists:
- evaluating several candidate flips of a knob in parallel;
- ±1/±2/±4 value steps instead of bit flips;
- racing (abort a run once it exceeds the incumbent at a checkpoint);
- visiting knobs in the order of their last measured effect.

Dead constants are already frozen, OPT_JOBS parallelism exists, and gc.sh fixes the float flags. New here:
- Search M1/M2 as log τ (zai §5.2, gpt §16). A unit step of M2_v = 25 moves τ by 4%, while a unit step of M1_m moves it by 0.0004%, and ±1/2/4 value steps do not fix that asymmetry.
- Use grouped moves: Nelder–Mead or CMA-ES over each axis's (M1, M2, NW, inc, stepMax) and A1's knobs [zai §11, gem].
- Keep one text and one binary file in the objective, and accept on a held-out file.
- Caveat for racing: knobs with τ ≥ 10⁴ (M2_u, M2_v) cannot rank on an early checkpoint.
- Bclip: the mixer pass raised the bias box once (SSE-DESIGN §6.9), yet b still sits at ±Bclip on 10% of text updates (§1.3). Include it in the next M0 pass.

### 4.2 Tier B — not yet tested

| id | proposal | notes |
|---|---|---|
| B1 | g-aware EMA decay `m^g ≈ 1 − (1−m)g` for fractional events [gpt §3.1]; SSEI | UPD = 2 was tested: −1671 at the seeds, behind UPD = 1 after tuning (§1.6). What is untested is whether letting a fractional event advance the EMAs by fractional time flips that result. Without UPD = 2 (and g ≠ 1) it is a no-op. |
| B2 | ~~mixer 2×2 (W,b)~~ | tested neutral (SSE-DESIGN §6.10); moved to Tier C |
| B3 | RTRL leak tied to the cell's memory: `leak = 1 − λ·wr` (r1) or tied to mass [kimi §5.2] | Tuned leaks: C0 0.934 / 0.9986, S0 0.983 / 0.874 (kimi's "~0.998 / ~0.990" are the commented-out defaults). At the wr floor, C0's leak cuts the trace horizon 18×. Unless OPT_ALEAK was exactly this, it is not a retest. |
| B4 | Seeds: per-column SSE seeds (r1 CSEED); per-context C0 u-seed LUT [zai §7] | Only global seeds were tested (SSE-DESIGN §6.5); 26–76% of S0 updates hit cells younger than 16. The LUT has overfit risk; accept only on concat or held-out data. |
| B5 | Parent init on first touch (r1 PINIT); count backoff [qwen §3.16.2] | Use A1's age == 0 as the first-touch test. |
| B6 | Saturation handling: decay D at the wall [zai §3.1], projected step [zai §4.1], soft barrier [zai §4.2, qwen §3.6, kimi §4.3] | K is at its wall 44% / 12% of the time. Do this after A0, which may move C0's wall. Fix zai's barrier formula (it gives lo + 0.63δ at the join). |
| B7 | Shared statistics across cells [r1 SHR, ds §3.3, kimi §3.2, zai §2.3, grok §4] | SSE-DESIGN §8 lists "rate optimizer shared per row" as an unexplored memory saving. Its young-cell effect (warm denominators) is what R0 > 0 does, which loses on counters and mixer. Test only variants that leave young-cell steps alone: share after age > N, or kimi's D-prior toward the row mean. |
| B8 | Shrink values toward their seeds [qwen §3.4, mmax §3.6/§5.3, corrected] | Implement as a value leak toward the seed, not as `+λ` in R (that is R0 damping). Mind the sign and the absence of a 1/p_t factor (§5.3). |
| B9 | Winsorized R increment, `min(g², c·max(R, inc))` [zai §2.1, r1 §3.8] | The surprise spike is (pK/(1−pK))² times the typical g²; K's cap already absorbs it. Low priority. |
| B10 | Per-cell adaptive mwP0 [kimi §4.2] | +3 floats per cell; untested; low priority. |

### 4.3 Tier C — measured negative, already tested, or no-ops

| proposal | source | verdict |
|---|---|---|
| R0 > 0 / curvature prior | r1 F7, zai §7, mmax §2.3, gpt §17 | counters +447 … +1312 (§1.4); mixer +287 … +1063 (SSE-DESIGN §6.10) |
| Adam, sqrt normalization, normalized momentum | ds §5.1, kimi §1.3, qwen §3.14, mmax §3.2/§4.4, r1 OPT_ALPHA | lose to Newton on the mixer: +643 … +6647 (SSE-DESIGN §6.10) |
| sign-agreement step gain, stability score, per-cell adaptive gain | qwen §3.11, gpt §10, zai §3.3 | +1238 … +2020 on the mixer (SSE-DESIGN §6.10); the OPT_AWOSC family (+3768) on counters |
| mixer 2×2 (W,b) coupled solve | qwen §3.10.1, grok §3.4, r2 B2 | neutral (+10 … +375, det-guard variants −16 / +24) |
| Adam-style bias correction or W-normalized moments | gpt §3.2, mmax §3.1, qwen §3.2, ds §5.3, zai §2.4 | shrinks cold steps by τ_R/τ_D (§2); same direction as R0 > 0 and A < 0, which both lose |
| NAG on the counters | ds §5.4, kimi §1.4, zai §5.1, r1 | +73 C0, −4 S0; u/v need code (F8). Kept on the mixer. |
| positive-part R, Rpos/Rneg, Fisher when h ≤ 0 | r1 §3.5, gpt §12, ds §2.1 | best is −14 |
| wider SSE interpolation kernels | qwen §3.7.2 | +1578 … +12106 (SSE-DESIGN §6.6) |
| stochastic single-cell SSE update | qwen §3.7.3 | nearer-cell-only is +10901 (SSE-DESIGN §6.5); proportional beats it at the seeds |
| link-function swaps for weights | ds §6.1, grok §3.1 | the mixer's weight-domain test: bounded forms tie, unbounded ones lose 2–35K (SSE-DESIGN §6.9) |
| "switch to Fisher/Gauss-Newton curvature" | ds §2.1, gpt §4.1, qwen §3.1, grok §2.2 | the empirical Fisher is already the setting wherever G2 = 0; the expected Fisher is OPT_EFISH (EFW_* tuned to 0); G2 → 0 on mw/v is a single tuner move |
| exact 2×2 via h01 traces | ds §2.2 (its #1), kimi §2.1 | adds zero-mean noise (Bartlett); tuned XHW < 0 argues against it; the guard trips ≤ 0.6%; needs 2 extra traces |
| learned rotation angle | ds §6.2, grok §3.1 | a full 2×2 Newton step is rotation-invariant |
| (mw,K) coupling, joint (mw,K) ray clip | ds §3.1, mmax §3.7, kimi §1.5, zai §6 | = OPT_MKCPL and OPT_MKRAY (+131) |
| relative stepMax in log coordinates; relative D clip | ds §4.3–4.4, gpt §13 | a log-coordinate origin is arbitrary; D_clip never binds on counters |
| reset at the file boundary | ds §7.3, mmax §4.8, qwen §3.12 | state is fresh at f_pos = 0; inside the concat benchmark the coder cannot see the boundary |
| double-precision moments | ds §8.5, zai §9, qwen §3.18.1 | no statistical effect; the output is deterministic |
| secant curvature, curvature variance, control variate, label smoothing, surprise-weighted g, "hex SSE" | gpt §11/§9, mmax §3.3/§5.6, qwen §3.15.3, kimi §3.1 | noise-dominated, objective-changing, or wrong (§5.3) |

### 4.4 Cost (state and time)

- **C0. `[[no_unique_address]]` on the Counter members** (F13): −8 B per reduced cell, free.
- **C1. Cache transcendentals** (F3): 5 of 7 per C_Update, o1 only.
- **C2. Polynomial exp2/log2**: only if streams must cross toolchains (F5).
- **C3. 64-byte full cell.** SSE-DESIGN §8 lists "halved width" as unexplored. Prerequisites: F1's ww-deletion toggle passes, and pK comes from the caller (it can also carry A1's age). Layout: 6 values + 4 first-order traces + 4 R (f32) + 4 D (f16) = 64 B. R_uv does not fit, so drop it (UV2X2 was worth −74 at 050c) or share R per row (B7). D is safe in f16 (τ ≤ 32); R is not, because with τ_R ≥ 1300 the per-step decay is below the f16 ulp and R freezes (qwen §3.8.2 proposes f16 R). SSE table 1.61 → 1.07 GB.
- **C4. Skip steps 1–3 when the gradient bound is small** (r1 §4.4, zai §9). Gate on abs(e)·abs(z'); confident-correct events still carry gradient.
- **C5. SoA 4-lane ParamUpdater** (r1; ds §8.4).
- **C6. SSE memory ladder.** 1.61 GB full → 0.60 GB without wr adaptation (after C0; ~3K worse, 35% faster per SSE-DESIGN §7) → 0.20 GB with none (two thirds of the stage's gain gone). Row count: 400 MB costs ~3K (SSE-DESIGN §6.3). Lazy allocation (qwen §3.8.1) avoids little, since 45% of binary SSE updates hit age-0 cells; a hot/cold split (qwen §3.8.4) gains little, because C_Update touches every field.

### 4.5 Model-side (out of scope, with prior results)

- **Hash collisions:** doubling the rows (2^25 cap) gained only −561 (SSE-DESIGN §6.3), which bounds what check bits or probing can win [mmax §2.5, qwen §3.9].
- **Interpolation kernels:** wider kernels, the linear quantizer and history sub-rows all lose (§1.6). Tunable quantizer cut points are untested (SSE-DESIGN §8; qwen §3.7.1).
- **Second SSE stage:** a second stage on another context would resolve the text/binary tension of the c3 mask (SSE-DESIGN §8).
- **Beta-prior or backoff counts** [qwen §3.16].

---

## 5. Error register

### 5.1 Errors in r1

| r1 location | r1 said | correct |
|---|---|---|
| §1.1 table, v row | G2 = 0 | G2 = 0.125 (C0_G2_v = 4); d²p clip binds 6–34%; increments negative 16–28% |
| §1.1 consequence 1 | d²p is off on u, v and y; only mw uses it | off on C0 u/K and S0 v; live on C0 mw and v, S0 mw, u and K |
| F1 | ww traces are live only through h_x | they are also live through v's d²p (C0) and u's (S0) |
| §1.1 u row | AdaGrad with cap; cap reached within 10²–10³ events | the cap binds on 0.2–2% of updates; mean R is 4–10 against a cap of 48 |
| §1.1 K row | cold bang-bang because R ≈ 0 and inc ≈ 0 | at age 1, R = g², so step = NW/abs(g), clipped for 2.4e-5 < abs(g) < 0.51. K also clips 36% at age 256+ (text). r1 also missed cold mw |
| §1.2 memory | S0_ADAPT_WR = 0 → 60 B / 1.0 GB; none → 12 B / 201 MB | 44 B / 0.74 GB and 20 B / 0.34 GB as compiled; 36 / 12 B with F13 |
| §1.1 point 3 | a typical SSE cell sees O(1–5) updates | 40% / 26% / 76% of S0 updates hit cells younger than 16 (alice / lcet10 / maps) |
| §3.8 | implied SSE adaptation may be near-worthless while cold | worth 0.8–2.8% here; SSE-DESIGN §3.6 had already shown it carries two thirds of the stage's gain |
| §3.1 SHR | top proposal; mass-based warm-up gate | mass starts at 2.1 / 2.0 and saturates near 1/wr; the main effect is predicted negative by the R0 tests |
| F6 | illustrative magnitudes | those were invented; measured R = 0 on 3.6–8.6%, and the fix is worth −14 B |
| F7 | expose R0_m / R0_k | tested: monotone loss (and on the mixer, SSE-DESIGN §6.10) |
| §3.8 | NAG is a Config field for all four axes | only mw/K are wired (F8) |
| §3.8 | OPT_ALPHA (sqrt normalization) as untested | Adam-style sqrt normalization already lost on the mixer (SSE-DESIGN §6.10) |
| §4.3 | "R_v := R_u, the July 'shared h00+h11' design" | the July design shared the d²p input h_d; R_u and R_v were always separate |
| §3.8 | surprise spike 10⁵–10⁶ × g² | (pK/(1−pK))²: 10⁴ at pK = 0.99 |
| page header | baseline 344956 / 310144 / 656681 | 050c o1-only numbers (F1c) |
| F3 | 4 of 7 cacheable | 5 of 7 |

### 5.2 Errors in r2 (found against SSE-DESIGN.md and the stage split)

| r2 location | r2 said | correct |
|---|---|---|
| §0, §1.4 | "the local knobs are at a local optimum" | shown for S0/M0 only; the C0 constants were never tuned in the pipeline (A0) |
| A1 | shared A/B; the binary loss is a trade-off | the binary loss is entirely C0's; S0-only wins on all three files (§1.4) |
| F5 | "fix: build with `-ffp-contract=off`" | already in `gc.sh` together with `-fno-unsafe-math-optimizations` (SSE-DESIGN §4.1) |
| F2 | dead knobs waste opt.pl evaluations | they are frozen in the IDX (SSE-DESIGN §4); only the source is dead |
| B2 | mixer 2×2 untested | tested neutral (SSE-DESIGN §6.10) |
| B1 | "retest UPD = 2" framed as new | UPD = 2 was tested (−1671 at seeds, behind after tuning); only g-aware decay is new |
| §4.5 | wider SSE kernels listed as open | tested negative (SSE-DESIGN §6.6) |
| §2.2 | 44 B / 20 B presented as the cost of reduced cells | the intended 36 / 12 B needs `[[no_unique_address]]` (F13) |
| §5.3 | credited qwen's S0 simplification test as answered by r2's §1.2 | already measured on the tuning corpus in SSE-DESIGN §3.6/§7 |

### 5.3 Errors in the reviews

**A. Stale header notes taken as fact.** kimi §2.2/§5.1 (recommends pruning the ww traces as "safe"), qwen §1.4/§3.13, gpt §24.3, mmax §2.3 and zai §12 treat "G2_u = G2_v = 0 ⇒ ww traces dead" as current (see F1). zai §1 read G2_v = 4 correctly but kept "G2 > 0 revival" in its falsified list.

**B. Proposals that already exist in the code.**
- ds §2.3, qwen §3.5 and gpt §7.1 propose separate u/v curvature and a rank-1 cross term. R_u and R_v are already separate EMAs of g_u² and g_v², and R_uv already carries CXW·g_u·g_v.
- ds §2.1, gpt §4.1, qwen §3.1 and grok present "Fisher curvature" as new. The empirical Fisher is what runs wherever G2 = 0; the expected Fisher is OPT_EFISH.

**C. Rotation algebra.** ds §2.2 writes h_uu = (h00 + 2h01 + h11)/4 and similar. With ln wr0 = u + v and ln wr1 = u − v, there is no factor 1/4: h_uu = h00 + 2h01 + h11, h_vv = h00 − 2h01 + h11 and h_uv = h00 − h11. The code's `h_x` comment and gpt §7.1 agree. ds's claim that the exact 2×2 would remove R_uv and UVDET also fails: per-sample Hessians are indefinite, so an EMA and a PD guard are still needed. kimi §2.1 mixes (wr0, wr1) and (u, v) coordinates.

**D. NAG "flip four constants, zero code"** (ds §5.4, zai §5.1): see F8.

**E. stP_min gate direction** (kimi §4.1 and §7, mmax §2.2): see F10.

**F. Leak values** (kimi §5.2): "~0.998 / ~0.990" are the commented-out defaults.

**G. Bias-correction direction** (ds §5.3 "early steps tiny", mmax §2.1 "under-step", qwen §3.2 "faster adaptation"): wrong direction; see §2. mmax also gives momentum_R(mw) as 0.975 (it is 0.99923) and says R starts biased upward (it starts at 0).

**H. No-ops at UPD = 1.** gpt §3.1 (its top priority) and kimi §7 (its #1 "bugfix") act only when g ≠ 1. The UPMIN floor is deliberate, and per-cell RTRL is exact for any g.

**I. Prior negatives misread.**
- kimi §1.5: its "direction-preserving rescale" is RAYCL = OPT_MKRAY (+131).
- ds §3.1: says MKCPL used a global constant. It used per-cell realized/EFISH cross terms with CMK as the dose.
- mmax §3.7: expects "hundreds of bytes" from (mw,K) coupling, against the measured ≤ noise.
- mmax §2.4: has OPT_ALEAK's files reversed.
- mmax §5.5: calls OPT_BIASC's "+43..+294" a gain; it is a loss.

**J. mmax, other errors.**
- **Vanishing mixer gradient:** the loss is already log loss, and ∂L/∂z = p − y does not vanish on errors.
- **"Copy-paste bug" in Config_V's box:** the fields are dead (F2, F11).
- **v box "half the range":** backwards; the box is 2.57× the half-range.
- **R_uv from the Hessian at the seed:** all traces are 0 at init.
- **Update n0/n1 before the parameters:** reordering would bias the gradient.
- **Run DIAMP first:** a no-op.
- **PredictF cache from "triangle-cell symmetry":** false.
- **"9 trace floats = 4 channels":** false.
- **Control variate that subtracts the gradient's running mean:** removes the signal.
- **sqrt form "has the right scaling":** Newton's D/R is loss-scale invariant; the sqrt form is not.
- **mw, K "convex in their coordinates":** σ is not convex.
- **"Hex SSE":** no basis.

**K. kimi, other errors.**
- **R at cap "gradient-independent":** it becomes curvature-independent; the step is still ∝ D.
- **"~100 ms per tuner iteration":** SSE-DESIGN §6.8 reports 7 s per evaluation pair on 4 cores.
- **Surprise-weighted g:** changes the objective and, since g scales the count recursion, the model.

**L. qwen, other errors.**
- **Same-observation step check (§3.3; ds §4.2):** a gradient step on the observed bit almost always lowers that bit's loss, so the test carries no information.
- **Seed prior (§3.4):** `D −= λ(val − seed)/p_t` has the wrong sign, and a prior's gradient takes no 1/p_t factor.
- **Double precision to "reduce noise" (§3.18.1):** the output is deterministic.
- **Stochastic cell selection (§3.7.3):** needs a deterministic PRNG, which the review does not say.

**M. zai, other errors.**
- **"K has a soft boundary":** KLOG has a hard y box.
- **Barrier formula:** discontinuous at the join.
- **FTZ as a hidden leak:** traces never approach 1e-38.
- **"1/√t" decay:** the implicit schedule is constant up to τ_D, then ~τ_D/t.
- **"OPT_G1 clipped dp into D":** unstated in the source.

**N. gpt:** the §6.2 boundary distance `min(x−hi, lo−x)` is negative inside the box. §11's secant curvature from single-event gradients is noise-dominated (gpt flags this itself).

**O. ds:** §9.3 reads "relative deltas only" as the tuner's objective; the note refers to absolute sizes differing across compilers. §8.1 "assert byte-identity across `-ffast-math` on/off" cannot hold; gc.sh fixes the flags instead.

**P. grok and gem.** grok's "re-orthogonalize RTRL state" does not apply to scalar traces, it counts `grad1_clip` as an active clip, and its Fisher should be p(1−p)·z'z'ᵀ. gem does not analyze this program; its "up to 10×" and "up to 50%" figures are unsourced. Its multi-fidelity and CMA-ES ideas apply to opt.pl only (A4; SSE-DESIGN §6.10 already lists racing).

**Already tested, so retests rather than errors:** qwen §3.10.1 and grok §3.4 (mixer 2×2), ds §5.1, kimi §1.3 and qwen §3.14 (Adam), qwen §3.7.2 (wider kernels), qwen §3.11 and gpt §10 (sign gain), gpt §17, zai §7 and mmax §2.3 (R0 prior), qwen §3.7.4 (SSE cell simplification). See §1.6.

### 5.4 Notes on SSE-DESIGN.md versus the current source

- §3.6/§7 give 36 B / 12 B for the reduced cells; compiled they are 44 / 20 B until F13 is applied.
- §6.9 gives the mixer table as "2^20 contexts × 36 B = 38 MB"; the current `Mix2_Cell` is 24 B over 1044480 contexts, 25 MB (the removed variants' fields are gone).
- §6.10 describes R as the EMA of the exact Hessian of −ln p. On axes with G2 = 0 (C0 u/K, S0 v) the d²p term is clipped to zero, so R is the squared gradient there.
- §4 says `SSE_Ctr::Init()` clamps nb to [2, 64] and clamps `W`. The code clamps to [2, SSE_NB_MAX = 16], and `W` no longer exists since the mixer replaced it.

### 5.5 What the reviews got right

- **zai:** the only review that read the G2 values; the Bartlett/Fisher identity; the dead-constant list; the stale LOGWR comment; the ESS parametrization; auditing the g call sites.
- **gpt:** correct half-lives; R_uv inheriting U's decay; leakage making the traces approximate; the logit-domain derivatives (A2); the counterfactual step-ratio diagnostic.
- **ds, mmax:** the redundant expf/mws computations (F3).
- **kimi:** the D-prior toward the row mean (B7) and grouped tuner moves.
- **qwen:** a thorough instrumentation list.

Accuracy against the source: zai > gpt > ds > qwen > grok (few code-specific claims) > kimi > mmax. gem is off-topic.

---

## 6. Diagnostics still to run

1. A0 itself: a C0 opt.pl pass on the final configuration, on book1 / wcc386 / book1_wcc386.
2. A1's S0 schedule on the tuning corpus, then inside an S0 pass.
3. §1.2–1.4 repeated on the tuning corpus, where Canterbury effects ≤ 0.1% may not transfer.
4. The applied/raw step ratio per axis [gpt §26], split into ray, stepMax and box limits.
5. The correlation of D_u·D_x with D_u·D_y, before any (u,v)×(mw,K) coupling.

## 7. Suggested order

| stage | items | decides |
|---|---|---|
| 0 | F1 header fixes, F2 source cleanup, F13 `[[no_unique_address]]` | clean source; free memory |
| 1 | A1 for S0 (S0_AGA / S0_AGB knobs) inside an S0 opt.pl pass | the measured young-cell win on the tuning corpus |
| 2 | A0 C0 pass in the full pipeline, using A4's tuner changes | C0 as an SSE input and mixer operand; C0's kMax |
| 3 | A1 for C0, re-measured after A0 | whether the C0 part survives the retune |
| 4 | A2 E2E (β_C0, β_S0), SSEI | the online objective |
| 5 | A3 global hypergradient on held-out data | per-file adaptivity |
| 6 | B1 (with UPD = 2), B3, B4, B5, B6, B7 (age-gated only) | smaller structural items |
| 7 | C1, C3–C5 as needed | cost |

Protocol as before: every toggle defaults to bit-identical under gc.sh's flags; report book1 / wcc386 / book1_wcc386 deltas with decode verification; give isolated and on-stack numbers.

## 8. Relation to already-measured negatives

| prior result | proposals that retest it |
|---|---|
| OPT_EFISH (subsumed by MWLGT; EFW_* = 0) | the "Fisher curvature" proposals from qwen, gpt, ds and grok |
| OPT_MKCPL (all 4 variants ≈ noise) | ds §3.1, mmax §3.7, zai §6 |
| OPT_MKRAY (+131) | kimi §1.5 |
| OPT_AWOSC (+3768); sign-agreement gain on the mixer (+1238 … +2020) | qwen §3.11, gpt §10, zai §3.3 |
| OPT_SMASS (+1349, stale mass) | file-boundary resets |
| OPT_ALEAK (book1-only, concat worse) | B3 (retest only if ALEAK was leak = f(wr)) |
| OPT_HBETA (file-antagonistic) | A3 (retest only if HBETA was a global per-type rate) |
| OPT_G1 (+366) | B9 clips only the R increment |
| "G2 > 0 ww revival +110..+155" | superseded: the current constants have G2_v = 4 (C0) and G2_u = 1 (S0) |
| mixer R0 prior, Adam, normalized momentum, 2×2 (SSE-DESIGN §6.10) | R0 floors, bias correction, OPT_ALPHA, mixer 2×2 |
| UPD = 2 behind after tuning; UPD = 0 +10.9K (SSE-DESIGN §3.4, §6.5–6.6) | B1, stochastic single-cell update |
| wider kernels, linear quantizer, sub-rows, delayed-update rows (SSE-DESIGN §6.6–6.7) | qwen §3.7 |
