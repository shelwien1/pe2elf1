# HEVC, VVC and ECM ideas for h264p

h264p already models H.264 with tools that VVC and JVET's exploration model
(ECM) introduced: DIMD, template matching, sign prediction, template-ranked
references, statistics that never reset. This note goes through HEVC, VVC
and ECM for what is left, checks each idea against what h264p's families
already see, and measures the cheap ones.

In short: three cheap transfers gain together 3,004 bytes (0.11%), untuned.
These are the VVC coefficient template, HMVP search starts and a CCLM
correction of the intra chroma proxy. VVC's entropy-coding contexts add
about 0.01% each. Substituting another intra proxy (CCLM, IntraTMP) for the
reference one loses. The open bucket is the inter residual: 44% of the
residual bytes, with no estimate in h264p. ECM's OBMC, BDOF and DMVR are
the candidates there, unmeasured (section 5.1).

All figures are level `-9` on `test/002` (7 streams, 3.87 MB of H.264).
Every experiment but the strict IntraTMP one was round-tripped (`d`
restores every stream byte for byte). The default build of the patched
source codes all 7 streams to the baseline's sizes.
The experiments are compile-time switches (`XP_*`, section 7). They are
off by default and leave the shipping build unchanged.

## 1. Sources

| what | version | used for |
|---|---|---|
| FFmpeg `libavcodec/hevc/`, `vvc/`, `cbs_h26[56]*` | `adfe258`, 2026-10-21 | the compact readable decoders: context derivations, CCLM, BDOF, DMVR, HMVP |
| VTM (VVC reference) | 24.0, `051e5e7`, 2026-10-18 | IBC hash search; spec cross-checks |
| HM (HEVC reference) | 18.0, `db12335`, 2026-10-20 | spec cross-checks |
| ECM (beyond VVC) | ECM-20.0, `3391a5f` | `TypeDef.h` lists 500+ tools by JVET document; the decoder-side derivations |
| ITU-T H.265 | 01/2026, 734 pages | 9.3.4.2.4-6 (coded_sub_block_flag, sig_coeff_flag, greater1 ctxSet), 7.3.8.12 and 8.6.6 (cross-component prediction) |
| ITU-T H.266 | 01/2026, 544 pages | 9.3.4.2.7-9 (locSumAbsPass1 template), 8.4.5.2.14 (CCLM), 8.5.2.16 (HMVP), 8.5.3 (DMVR), 8.5.6.5 (BDOF), JCCR |

The VTM, HM and ECM repositories are at `vcgit.hhi.fraunhofer.de` (`jvet/VVCSoftware_VTM`,
`jvet/HM`, `ecm/ECM`). The specifications are free downloads from itu.int:
`https://www.itu.int/rec/T-REC-H.265-202601-I/en` and
`https://www.itu.int/rec/T-REC-H.266-202601-I/en`.

## 2. What h264p already has from them

| VVC / ECM tool | in h264p as |
|---|---|
| DIMD (ECM `ENABLE_DIMD`), location-dependent DIMD (JVET-AC0098) | ipm `dm`/`ds`, at the block's place |
| template matching AMVP/merge refinement (ECM `TM_AMVP`, `TM_MRG`) | mvs `tv`/`tq`/`ta`, the hint |
| TM-based reference reordering (JVET-Z0054) | ref_idx's best-matching reference |
| coefficient sign prediction (ECM `SIGN_PREDICTION`, JVET-Y0141) | sgn `pp`/`mg`, six signs per block, luma and chroma |
| temporal CABAC (JVET-Z0135), per-context windows | statistics never reset; two-rate counters, mixers, APMs |
| MPM lists, TM-sorted MPMs (JVET-AD0085) | actual modes of A, B, C; DIMD; modes ranked in the proxy by SATD |
| CG / sb_coded_flag, last position | count first, then significance with `rem`; CABAC-style sig/last was tried and lost |
| chroma DM (chroma mode from the luma mode) | ipm `mp` = luma_dir() for chroma |
| HEVC QP predictor (left/above average) | qpd `db`, `bc`, `dc`, `cl` |

None of OBMC, BDOF, LIC, CCLM/CCCM, IntraTMP/IBC or TIMD is in h264p.

## 3. Where the bytes are

The model's bytes over the 7 streams:

| element | bytes | share |
|---|---:|---:|
| coeff counts | 413,294 | 15.2% |
| significance | 349,056 | 12.8% |
| levels | 257,872 | 9.5% |
| signs | 205,580 | 7.6% |
| mvd + signs + sub/ref | 427,335 | 15.7% |
| mb_type | 260,564 | 9.6% |
| intra modes | 238,941 | 8.8% |
| skip, end | 218,993 | 8.1% |
| qp delta | 184,163 | 6.8% |
| cbp, 8x8 | 156,468 | 5.8% |

The residual is 45%. The following table splits it by macroblock kind (streams 00, 01, 03, 04 and 06, from cost
accounting per block: `XP_STAT`):

| | 00 | 01 | 03 | 04 | 06 | total |
|---|---:|---:|---:|---:|---:|---:|
| inter luma | 233,143 | 84,094 | 6,868 | 61,856 | 61,025 | 446,986 |
| inter chroma | 24,424 | 7,872 | 31,480 | 8,019 | 9,132 | 80,927 |
| intra luma | 58,005 | 313,377 | 57,392 | 69,644 | 48,846 | 547,264 |
| intra chroma | 19,311 | 57,083 | 21,858 | 7,864 | 9,280 | 115,396 |

The intra residual already has an estimate, the proxy (88 KB of gain). The
inter residual (44% of the residual bytes) has none: its blocks see the
prediction's texture (`pf`, `hv`, `pg`), the edge miss, and in P pictures
the block under the vector. That gap is where ECM's remaining tools point.

## 4. Experiments

Each row is the baseline plus one change, untuned: the IDX masks are those
of the baseline.

| change | from | total | Δ bytes |
|---|---|---:|---:|
| baseline | | 2,717,777 | |
| lvl `lt` over VVC's 5-sample template, Σ min(\|l\|, 4) (was right + below) | H.266 9.3.4.2.7 | 2,717,603 | −174 |
| the same as the gtx offset, Σ(\|l\| − 1) | H.266 9.3.4.2.9 | 2,717,511 | −266 |
| mvd: 2 history (HMVP) vectors as template-search starts | H.266 8.5.2.16 | 2,716,797 | −980 |
| mvd: 4 history vectors | | 2,716,728 | −1,049 |
| intra chroma proxy replaced by CCLM from the decoded luma | H.266 8.4.5.2.14 | 2,726,364 | +8,587 |
| proxy + α·(decoded luma − proxy luma)↓, α fitted on the proxy | CCLM, H.265 8.6.6 | 2,716,728 | −1,049 |
| the same, α fitted on the samples around the macroblock | | 2,716,073 | −1,704 |
| intra proxy from the current picture (IntraTMP, ±32) where its 4-sample template beats the reference proxy's | ECM JVET-V0130 | 2,727,182 | +9,405 |
| the same, ±64 | | 2,729,372 | +11,595 |
| the same, ±32, only where 4 times better | | 2,719,852 | +2,075 |
| inter chroma DC estimated by LIC (template brightness change), through the proxy's contexts | ECM `INTER_LIC` | 2,717,930 | +153 |
| the three gains together (`lt` gtx offset, 4 HMVP, CCLM α around) | | 2,714,773 | −3,004 |

Details: HMVP keeps the last 8 distinct coded vectors per list in the
picture, most recent first, and adds the first N on the partition's
reference to the template search's starts (no new context). CCLM fits α
(and β for the first variant) by least squares, in `double`, clipped to
±4. It uses 2x2-averaged luma, on the row above and the column left of the
macroblock (variant 1 and the last), and is applied where the chroma proxy
is searched again against the decoded luma.

What these show:

- **VVC's entropy-coding contexts are subsumed.** The 5-sample template is
  VVC's main coefficient context. Over h264p's two-sample one it gains
  0.01%. HEVC's ctxSet and VVC's history Rice are of the same kind (section 5.6).
- **A second estimate gains, a substitute loses.** Correcting the proxy's
  chroma by its luma miss gains. Replacing the proxy (by CCLM, by
  IntraTMP) loses. Likely causes: the contexts judge the proxy by its own
  template measure (`pq`, `pxq_`), and the replacement also drops the
  re-searches before later blocks. IntraTMP's 4-sample template was a
  poor judge against the reference proxy's 16-sample, quarter-sample one.
  Even made strict, it lost on 00, 01, 03 and 04.
- **LIC carries nothing here** for inter chroma DC, at least through
  contexts tuned for the intra proxy. Likely because these streams'
  brightness is steady, and x264's weightp would take fades.

### Motion vectors, by the numbers

From an encode-side dump of every partition with an mvd (`XP_MVDUMP`;
763,561 partitions, 371,311 with a nonzero mvd):

- **Candidate hits are common but small.** 23.4% of the nonzero mvds give
  a vector equal to one of VVC's candidates: A, B or C on the same
  reference, col, 5 history entries, or zero. 18.3% equal one that is not
  the template match, and 9.4% only a history entry (streams 00, 01, 03,
  04, 06). But those mvds are small: median |mvd|₁ is 2 quarter samples,
  against 5 for all.
- **The template match already predicts the second component.** Once the
  x component equals some candidate's, y equals that candidate's in 43.4%
  of cases. The template match gets y right in 45.2%, and that is its
  search before x was known; the model searches again after. A merge-style
  candidate context would mostly repeat the hint.
- **The quarter-sample phase is skewed but tells little.** Over nonzero
  components the phase of mv is 0 in 35.7% (1: 23.1%, 2: 18.3%, 3: 22.9%).
  Knowing mvp's phase takes H from 1.954 to 1.925 bits, and for the low
  two bits of |mvd| ≥ 4 from 1.862 to 1.806. At most about 3 KB in all.

## 5. Ideas, ranked

Ordered by expected gain against cost. "Measured" refers to section 4.

### 5.1 An inter proxy (OBMC, BDOF, DMVR)

The inter residual is 528 KB here, and h264p estimates none of it. ECM's
inter tools all compute, at the decoder, a better prediction than the
coded one. Taking (better − coded prediction) through `fquant4`/`fquant8`
gives level estimates, as `proxy_levels()` does for intra blocks. These
can then feed the count, significance, levels and signs.

- **OBMC** (ECM `ENABLE_OBMC`, `InterPrediction::subBlockOBMC`). Blend each
  4x4 boundary block's prediction with the predictions by the
  neighbours' vectors (JEM's weights: 1/4, 1/8, 1/16, 1/32 over the four
  rows or columns into the block). The
  difference is nonzero only where the motion field is discontinuous.
  That is where x264's residual corrects block-edge mismatch. This is the
  candidate for P pictures (60% of CABAC's bits), and for 00 and 04,
  whose residual is mostly inter.
- **BDOF** (H.266 8.5.6.5; FFmpeg `vvc/inter_template.c`
  `derive_bdof_vx_vy`). For a bi-predicted block, solve per 4x4 block for
  (vx, vy) from the two predictions' gradients and their difference. The
  correction ½(vx·(gx0−gx1) + vy·(gy0−gy1)) estimates source − average.
  ECM's `SAMPLE_BASED_BDOF` does it per sample. B residuals are a small
  share in these streams (93% of B macroblocks are skipped).
- **DMVR** (H.266 8.5.3; FFmpeg `vvc/inter.c` `dmvr_mv_refine`). With
  list 0's vector known, search list 1 for min |P0 − P1| (bilateral, the
  block itself, not a template). Use the result as list 1's mvd hint and
  search start. h264p tried the mirrored list 0 vector and a
  bi-predictive template search; bilateral matching on the block was not
  tried.

Each needs its own path, not the intra proxy's. The LIC row above shows
that an inter estimate fed through the intra contexts gains nothing. Give
it a separate `pok` flavour, or variables of its own, so that its trust is
learned apart. Its trust measure would be the template miss of the
corrected prediction against the uncorrected one. h264p's negative results
nearby: "bi-prediction disagreement per block", "the template miss of the
macroblock's motion for the count", "edge-projected levels … as hints for
inter blocks". This is the largest unmeasured bucket, and the riskiest.

### 5.2 Cross-component chroma (CCLM, CCCM, HEVC CCP, JCCR)

**Measured:** −1,704 for the crudest form, a single slope fitted on 16
samples, in floating point, applied to the intra proxy only. The intra
chroma residual is 115 KB, so this is 1.5% of it. Next steps:

- Keep the corrected and the raw chroma proxy as two estimates, not one
  replacing the other.
- Fit a CCCM model in place of the linear one: ECM `JVET_AA0057_CCCM`, a
  7-tap cross-component filter with a nonlinear term, solved on a 6-line
  template. GLM (`JVET_AA0126`) instead models from the luma gradients.
  Both are in `IntraPrediction.cpp`; `xGetLMParameters` is the linear
  case.
- Inter chroma, after HEVC RExt cross-component prediction (H.265
  7.3.8.12, 8.6.6). There the chroma residual is α × the luma residual,
  α ∈ {0, ±1/8 … ±1} per TU. In h264p the luma residual is decoded before
  the chroma. Fit α per picture on the macroblocks so far, and estimate
  the chroma residual from the downsampled luma residual, transformed and
  quantized. Inter chroma DC is 27% of 03's residual.
- JCCR's premise (H.266 `tu_joint_cbcr_residual_flag`,
  `ph_joint_cbcr_sign_flag`) is that Cr ≈ ±Cb, with the sign per picture.
  h264p gives Cr Cb's count (`crcb`) and sign (`scb`). It does not give
  Cb's level or significance at the same position to Cr's `lvl` and `sig`.
  Small.

### 5.3 A second intra estimate for content the reference lacks (IntraTMP, IBC, TIMD)

01 has 313 KB of intra luma residual. Most of it is new slides, which no
reference picture holds. Measured above: using IntraTMP in place of the
reference proxy loses in every form. To use it well:

- **Feed it alongside, not instead.** Make it a second set of estimates
  (levels, count) with its own trust: the IntraTMP template miss against
  the reference proxy's (`pxq_`). Let the mixer weigh the two.
- **Search by hash for exact repeats**, for screen content (00):
  VTM's IBC hash search (`IbcHashMap`) and ECM's IBC/IntraTMP reference
  areas. Text and UI blocks repeat exactly. A hash of the decoded 16x16 or
  8x8 luma, keyed on the template, finds them over the whole picture and
  the DPB at no search cost.
- **TIMD** (ECM `JVET_W0123_TIMD_FUSION`, `IntraPrediction::deriveTimdMode`).
  Predict the macroblock's L-shaped template from the line beyond it with
  each of the nine 4x4 (or four 16x16, four chroma) modes, and rank the
  modes by SATD. That is the proxy's mode ranking without a reference.
  Intra modes are 239 KB, 88 KB of them in 01.

### 5.4 MVD sign by hypothesis cost (ECM JVET-AD0140)

ECM's `InterPrediction::deriveMvdSign` predicts signs this way. Once the
magnitudes are known, it costs the template match of each sign
combination, mvp ± |mvd_x|, ± |mvd_y|, sorts the combinations, and codes
the rank. h264p's mvg sees where the template optimum lies against the
magnitude (`ts`: sign, and equal, less or more). It never costs the two
hypotheses at the coded magnitude. Two changes:

1. Add the cost margin between the + and − hypotheses, as `mg` is for
   coefficient signs.
2. Code both magnitudes before both signs, so that the four combinations
   are costed jointly.

mvd signs are 41 KB, and the mvd suffix bits under them a further share
of the mvd's 338 KB; unmeasured.

### 5.5 Motion candidates and phase (HMVP, AMVR)

**Measured:** HMVP as template-search starts gives −1,049 (0.04%). As contexts, the
statistics in section 4 bound it: hits mostly repeat what the template match
already says. The mvp quarter-sample phase as an mvd context (AMVR's
premise that vectors gather at whole samples) is worth at most about 3 KB.
Both are cheap. Keep HMVP starts; try the phase as a context of the mantissa nodes.

### 5.6 Coefficient context details

Each of these is about 0.01%, measured or by analogy with the measured `lt`:

- The full VVC template for `lt` (measured −266). It could also give
  significance the template sum, by coding the levels in the same reverse
  pass as VVC does (sig, gt1, par, gt3 interleaved). That would change
  h264p's binarization, which the count-first design argues against.
- HEVC ctxSet (H.265 9.3.4.2.6): whether the previous 4x4 group had a
  level > 1, here the previous block of the 8x8 or the macroblock, as a
  `lvl` context.
- VVC's history Rice (`StatCoeff`, FFmpeg `ep_update_hist`): a running
  log2 of the first large remainder per component, for the mantissa
  nodes. It matters at higher rates (06).

## 6. Checked and dropped

| tool | why not |
|---|---|
| sign data hiding, dependent quantization / TCQ, LFNST, MTS, ACT, palette, BDPCM, transform skip | encoder-side syntax with no H.264 counterpart; their premises (parity, mode-dependent transforms) leave nothing an H.264 stream carries |
| PTCC (ECM JVET-AM0056: gt1 before sig where sig is likely) | a binarization reordering for CABAC; a mixing model gains nothing from it |
| SMVD (mvd1 = −mvd0) | tried as the mirrored vector, no gain |
| SbTMVP motion shift, projected motion field | tried ("a motion field projected from the last picture") |
| affine / global motion candidates | tried (global motion of the last picture) |
| GPM split from templates | tried ("a motion split guess from template matching") |
| HEVC/VVC split-flag contexts from neighbour depths | H.264 partitions: "partition shapes of A and B for mb_type", no gain |
| SAO, ALF, LMCS, deblocking changes | in-loop filters; h264p reconstructs as H.264 does |

## 7. Reproducing

The patch `h264p_hevc_vvc_xp.patch` adds the switches to
`h264p_src/model.inc` and `recon.inc`. Each switch is a compile-time
flag, and none changes the default build:

| flag | what |
|---|---|
| `XP_LT=1`/`2` | lvl `lt` over VVC's template (Σ min(\|l\|, 4) / Σ(\|l\| − 1)) |
| `XP_HMVP=N` | N history vectors per list as template-search starts |
| `XP_CCLM=1`/`2`/`3` | the chroma proxy: CCLM / corrected, α on the proxy / corrected, α around |
| `XP_ITMP=R` (`XP_ITMP_K=K`) | IntraTMP proxy within ±R, chosen when K× better |
| `XP_LIC` | LIC estimate of inter chroma DC through the proxy's contexts |
| `XP_STAT` | residual bytes per block kind, intra/inter, with `-v` |
| `XP_MVDUMP` | a line per partition with an mvd on stderr (`c` only): mvp, mv, A, B, C, col, the template match |

Build a single level with
`g++ -O2 -march=x86-64-v3 -ffp-contract=off -std=c++11 -fwrapv -fno-exceptions -fno-rtti -pthread -Ih264p_src/Lib3 -DH264P_ONLY=9 -DXP_…`.
CCLM uses `double`: it is fine for a measurement, but a shipping version
needs integer fits (VVC's are integer, H.266 8.4.5.2.14).
