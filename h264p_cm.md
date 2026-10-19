# Compressing h264p dumps with context mixing

(This is the model of level `-9`, the strongest; `h264p_levels.md` has the
lighter levels, which leave parts of it out.)

`h264p c` turns an H.264 stream into rows of syntax element values, and
`h264p d` turns the rows back into the identical stream. A compressor for
the TSV files is therefore an H.264 recompressor, and its model has the same
job as paq8's JpegModel: follow the syntax, keep the decoded state of the
neighbouring blocks and predict each value from it.

This note covers four things:

- what such a model has to track;
- which contexts are worth trying for each field of the TSV;
- which values follow from others, and what repeats;
- where the bits are, measured on the x264 streams `test/002/00-04.h264`
  (3.6 MB, CABAC, 4:2:0, progressive).

## 1. What to beat

The baseline is the CABAC-coded stream, not a general-purpose compressor run
on the TSV:

| | MBs | pictures | H.264 | TSV | TSV, xz -9e | TSV, bzip2 -9 |
|---|---|---|---:|---:|---:|---:|
| 00 | 60×34 | 1867 | 815,140 | 11,630,192 | 1,200,428 | 1,157,014 |
| 01 | 30×54 | 1072 | 1,095,103 | 15,092,142 | 1,641,224 | 1,681,585 |
| 02 | 16×18 | 2 | 28,143 | 299,502 | 45,220 | 42,510 |
| 03 | 45×80 | 907 | 1,181,369 | 20,691,032 | 1,654,508 | 1,543,530 |
| 04 | 29×57 | 253 | 484,238 | 6,953,351 | 808,132 | 754,802 |
| all | | 4101 | 3,603,993 | 54,666,219 | 5,349,512 | 5,179,441 |

General-purpose compressors end up 44-48% above CABAC: they see digits,
not macroblocks.

### Where CABAC spends its bits

These figures come from h264p's CABAC decoder. Each bin was costed at
−log2 p from its context state just before decoding, each bypass bin at 1
bit, and the terminating bins (end_of_slice_flag, I_PCM) by estimate. The
total comes within 0.2% of the actual size of the slice data.
Slice data is 98.3% of these streams and slice headers 0.66%. Over all five
streams:

| syntax element | share of bits | bits/bin |
|---|---:|---:|
| **residual** | **43.6%** | |
| significant_coeff_flag | 11.9% | 0.74 |
| coeff_sign_flag (bypass) | 9.3% | 1 |
| coded_block_flag | 7.2% | 0.82 |
| last_significant_coeff_flag | 6.7% | 0.73 |
| coeff_abs_level_minus1, first bin | 5.2% | 0.56 |
| coeff_abs_level_minus1, other bins | 3.3% | 0.85 |
| **macroblock type** | **19.8%** | |
| mb_type | 10.3% | 0.53 |
| mb_skip_flag | 9.2% | 0.29 |
| end_of_slice_flag | 0.3% | 0.01 |
| **motion** | **14.3%** | |
| mvd, context-coded bins | 7.9% | 0.64 |
| mvd, bypass bins (suffix, sign) | 4.8% | 1 |
| ref_idx | 1.4% | 0.73 |
| sub_mb_type | 0.1% | 0.44 |
| **intra prediction** | **8.5%** | |
| prev_intra_pred_mode_flag | 3.2% | 0.83 |
| rem_intra_pred_mode | 3.2% | 0.95 |
| intra_chroma_pred_mode | 2.2% | 0.74 |
| **mb_qp_delta** | **7.7%** | 0.61 |
| **coded_block_pattern** (luma 3.8%, chroma 2.0%) | **5.8%** | 0.39 |
| transform_size_8x8_flag | 0.3% | 0.61 |

By picture type, P pictures take 60% of the bits, B pictures 23% and
I pictures 17%. 78% of the macroblocks in P pictures are skipped, and 93% in
B pictures.

The residual is under half of the bits, where in a JPEG file the
coefficients are nearly everything. The macroblock side information
(types, skips, motion, intra modes, QP) matters as much. 14% of the bits are
bypass bins coded at exactly 1 bit (signs, mvd suffixes): whatever a model
predicts there is pure gain.

The residual by block type (bits include the coded_block_flag of the blocks
that turned out empty):

| block (ctxBlockCat) | nonzero blocks | coefficients | share of bits | bits/coefficient |
|---|---:|---:|---:|---:|
| luma 4×4 (2) | 613,605 | 1,577,944 | 26.6% | 4.8 |
| chroma DC (3) | 378,961 | 466,819 | 6.4% | 3.9 |
| luma 8×8 (5) | 121,963 | 288,702 | 4.7% | 4.7 |
| Intra16x16 DC (0) | 59,733 | 152,889 | 2.9% | 5.3 |
| chroma AC (4) | 55,411 | 95,763 | 1.9% | 5.5 |
| Intra16x16 AC (1) | 24,933 | 52,943 | 1.0% | 5.6 |

These are low-rate streams (about 440 bytes per picture in 00). At higher
rates the residual share grows.

## 2. JpegModel, packJPG, Lepton, and what is different here

paq8's JpegModel compresses baseline JPEG without transforming it. It
decodes the Huffman codes while the scan is being coded, so for every bit it
knows the component, the block, the zigzag position and the bits of the
current code so far. Its contexts are hashes of those, combined with values
already decoded:

- the previous coefficients of the block, and the sum of their magnitudes;
- the coefficients at the same position in the blocks to the left and
  above, and predictions of the current coefficient from those blocks;
- the run/size of the previous code;
- the quantizer.

Small contexts such as the zigzag position and the code length so far select
the mixer weight sets, and an APM chain refines the result.

packJPG and Lepton decode the coefficients first and code them with their
own binarization. Lepton codes the number of nonzero coefficients of a block
first, from the counts of the neighbouring blocks. Then it codes each
coefficient's magnitude class, mantissa bits and sign. The contexts include
the remaining count and the neighbours' magnitudes at the same position, and
the edge row/column and the DC are predicted from the pixels of the
neighbouring blocks.

An h264p model sits between the two. Like packJPG and Lepton, it gets values
whose entropy coding is already undone, so it chooses its own binarization.
Like JpegModel, it has to follow the syntax to know what each value is and
where its neighbours are. The concepts map like this:

| JpegModel | h264p TSV model |
|---|---|
| headers: SOF (size, sampling), DQT, DHT | SPS, PPS and slice header rows (section 3.1) |
| MCU position, component | macroblock address, block index, ctxBlockCat |
| zigzag index, (u, v) | scan position, mapped to (u, v) with the standard's frame or field scan |
| bits of the Huffman code so far | bin index in our own binarization |
| previous coefficients, sum of magnitudes | levels and nonzero count so far, last position |
| same position in the left/above block | same position in the 4×4/8×8 neighbours A and B (6.4.11) |
| quantization table | QP, scaling matrices |
| — | previous pictures: co-located macroblocks and blocks |
| — | side information: 56% of the bits |

There are also real differences from JPEG:

- **The residual is a prediction error.** The encoder has already predicted
  each block from its neighbours (intra prediction) or from other pictures
  (motion compensation), so DC and edge continuity, Lepton's best tricks,
  have less to work with. They still give the signs away: once a block's
  magnitudes are known, the sign combination whose reconstruction continues
  the decoded samples around the block best is right for 68% of the signs
  it predicts (section 4.8). Correlations between neighbouring blocks remain
  too.
- **CABAC is a much stronger baseline than Huffman.** JPEG recompressors
  gain 20-25%. CABAC already adapts per syntax element with neighbour
  contexts. The single contexts measured below save between 1% and 18% of
  an element's bits. But CABAC forgets: it starts over at every slice
  (every picture here), and that alone costs it 2.2% of the stream
  (section 3.4).
- **About twenty kinds of values instead of one alphabet.** The mixer
  selection has to start with the element and bin. I, P and B pictures have
  very different statistics.
- **A time axis.** The same macroblock in the previous picture is a strong
  neighbour, for the skip flags above all (section 4.2).

## 3. Model structure

### 3.1 From rows to values

Parse the rows into values and model the values, not the text. Decimal
digits are a poor binarization: "-1", "1" and "12" share no structure a
bitwise model can use. A paq-style byte model with field-aware contexts
(row name, field index, previous fields) works on the text as a quick
prototype, but parsing is what pays.

h264p writes the TSV deterministically: decimal integers, `-` for absent
values, lowercase hex, tabs and `\n`. So the decoder can regenerate the text
from the values. `h264p d` also accepts comments, blank lines and CRLF;
input that isn't h264p's own output needs an escape to a generic model.

**What the model must track to know which slice data values come next:**

- **SPS:** `chroma_format_idc` (with `separate_colour_plane_flag`, giving
  ChromaArrayType), bit depths (PCM sample size, QP range), picture size in
  MBs, `frame_mbs_only_flag`, `mb_adaptive_frame_field_flag`,
  `direct_8x8_inference_flag`.
- **PPS:** `transform_8x8_mode_flag`, the default `num_ref_idx_l0/l1`,
  `constrained_intra_pred_flag`, `pic_init_qp`, `entropy_coding_mode_flag`
  and the slice group map.
- **Slice header:** `first_mb_in_slice`, `slice_type`, `field_pic_flag`,
  `num_ref_idx_active_override`, `slice_qp_delta` and
  `direct_spatial_mv_pred_flag`.

The macroblock address follows from `first_mb_in_slice` (×2 with MBAFF), the
`skip N` rows and the `mb` rows. FMO changes the next address; it can be
left to a fallback.

**The rows of a coded macroblock and what is implied:**

- `mb T X C P Q`: mb_type, then four fields whose presence is implied by T
  and the state. X (transform_size_8x8_flag) needs transform_8x8_mode_flag;
  then it is present for I_NxN, and for an inter MB with luma cbp and no
  partition smaller than 8×8 (direct partitions count as 8×8 only with
  direct_8x8_inference_flag). C (intra_chroma_pred_mode) is present for
  intra MBs other than I_PCM, with 4:2:0 or 4:2:2. P (cbp) is present unless
  the MB is Intra16x16 or I_PCM. Q (mb_qp_delta) is present if cbp ≠ 0 or the
  MB is Intra16x16. Never code a `-`.
- `fld`: present where MBAFF codes the flag (top MB of a pair, or the bottom
  one after a skipped top).
- `ipm`: 16 values, or 4 with X = 1.
- `sub`: 4 values.
- `ref0`/`ref1`: one value per partition using the list, only when
  num_ref_idx_active > 1 (or for a field MB in MBAFF). P_8x8ref0 has none.
- `mvd0`/`mvd1`: X/Y pairs per partition or sub-partition, none for direct.
- `res`: rows in coding order:
  1. Intra16x16 DC;
  2. the luma blocks of each 8×8 with its cbp bit set;
  3. Cb and Cr likewise for 4:4:4;
  4. chroma DC (Cb, Cr) if chroma cbp ≥ 1;
  5. chroma AC (Cb blocks, then Cr) if chroma cbp = 2.

  Only nonzero blocks have a row, so the model codes a "present" flag for
  every block the cbp allows. Each row holds coefficients in scan order up to
  the last nonzero one. The AC blocks (CAT 1, 4, 7, 11) start at scan
  position 1.

With CABAC (except in 4:4:4), an 8×8-transformed block whose cbp bit is
set always has a row, because CABAC has no coded_block_flag for it. With
CAVLC it may not. Otherwise the TSV of a CAVLC stream looks like that of a
CABAC stream; only the baseline is about 10-15% weaker.

Header rows (SPS, PPS, SEI, AUD and slice headers) are 0.8% of the stream
and need little modelling. Their presence is conditional on earlier values
too.

### 3.2 State

**For each macroblock of the current picture, keep:**

- skip, class (skip, direct, 16×16, 16×8, 8×16, 8×8, I_NxN, I16, PCM), and
  the full mb_type and sub_mb_types;
- field flag, QP, cbp, transform 8×8;
- the 16 intra 4×4/8×8 modes (actual modes, not the TSV's MPM-relative form)
  and the chroma mode;
- ref_idx[2][4] and mvd[2][16][2] as signed values;
- nonzero counts per 4×4 block for Y, Cb and Cr, the coefficients of each
  4×4/8×8 block, and the chroma DC values.

**For temporal contexts, keep the same arrays for previous pictures.** In
the experiments below, "col" is the MB at the same address in the previous
picture with the same slice type and the same nal_ref_idc ≠ 0 status (P
uses the last P, a reference B the last reference B). Keeping the pictures
by POC also makes the real temporal neighbours available, such as the
co-located MB in the list-1 reference that temporal direct uses.

**Neighbours:** A, B, C, D at MB level and at 4×4 level (6.4.11 and 6.4.12,
with table 6-4 for MBAFF; `nb_loc` in `h264p_src/slice.inc` implements it).
Unlike CABAC, the model may use MBs of other slices of the same picture: its
decoder knows them.

### 3.3 Binarization

| value | binarization |
|---|---|
| skip runs | one flag per MB, as CABAC (mb_skip_flag) |
| end of slice | one flag per MB; certain at the last MB of the picture |
| mb_type | a tree: intra? → partition shape → prediction lists per partition; for intra, I_NxN / I16 / PCM, then I16's prediction mode, chroma cbp and luma cbp (0 or 15), which mb_type encodes |
| X, flags | one bin |
| C | truncated unary 0-3 |
| P | four luma bits in 8×8 order, then chroma 0/1/2 as two bins |
| Q, mvd components | zero flag, sign, magnitude: unary for small values, then exponent (unary) and mantissa bits |
| ipm | "mode = MPM" flag, then the mode (or rem) |
| sub | P: tree over 0-3; B: direct? → lists → shape |
| ref_idx | truncated unary |
| coefficient block | present flag, then CABAC-style significance/last map and levels in reverse scan order, or count first (below) |

Count first: code the number of nonzero coefficients (CAVLC's TotalCoeff,
Lepton's num_nonzeros) with the neighbours' counts as context. Then code the
positions with "nonzeros remaining" as context, and the levels after that.
CABAC's significance map approximates this with position and last flags.
Both are worth trying, and the count is a good context for everything after
it.

### 3.4 Components

- **Context maps:** hashed contexts with bit histories, as in paq8, plus
  direct counter tables for small contexts. The CABAC context of each bin is
  a good first input.
- **Indirect contexts:** as in paq8, map a context to what followed it the
  last few times (say, the last two modes chosen after this pair of
  neighbour modes) and use that history as a further context.
- **A decoder in the loop:** reconstructing the pictures while the values
  are decoded gives contexts from the samples: the edge directions around an
  intra block (section 4.4), the signs of a block's coefficients (section
  4.8) and the pixel matches of section 7. The first two see the current
  picture as intra prediction does, before deblocking.
- **Mixer:** weight sets selected by element × bin index, and per element a
  small context: scan position for significance, gt1/eq1 counts for levels,
  the mvd magnitude class. Use two layers with several selectors, as in
  paq8px.
- **APM:** an APM/SSE stage indexed by element, bin and one strong context.
- **Counters:** statistics shift at scene cuts and between picture types.
  Separate the contexts by slice type (and nal_ref_idc), and mix fast and
  slow counters.
- **Priors:** CABAC's initialization (m, n) at the slice QP gives every
  context a sensible starting probability. A model starting at p = 0.5 pays
  for that on short streams.
- **Memory:** CABAC restarts its contexts at every slice, from the tables at
  the slice QP; in these streams that is every picture. Its own contexts and
  state machine, carried over instead from the previous slice of the same
  type, spend 2.6% less on the context-coded bins: 0.63 Mbit, 2.2% of the
  stream (00: 4.4% of its context-coded bits, 01: 1.2%, 03: 3.3%, 04: 1.0%;
  02 has one slice of each type). Most of it is in
  last_significant_coeff_flag (0.13 Mbit, −6.6%), mb_type (−2.8%),
  coded_block_flag (−2.9%), mvd (−2.5%) and mb_qp_delta (−2.4%). Any model
  that keeps its statistics gets this. The experiments of appendix A never
  reset either, so their gains come on top of it.
- **Speed:** CABAC codes about 15.5 bins per stream byte. Skip and
  end_of_slice flags are a third of the bins but 9.5% of the bits, so model
  them cheaply.

## 4. Contexts by field

For each element: what CABAC uses (the baseline to reproduce first), then
what to add. "Measured" figures come from the single-context experiments in
appendix A, over all five streams. They show what one context adds on its
own, with no mixing unless stated.

### 4.1 Stream and header rows

- `nal Z REF TYPE`: Z (the zero bytes before the start code) depends on the
  NAL type and its position in the access unit. TYPE and REF follow the GOP
  structure. An order-2 context over NAL types plus a match model covers
  them.
- SPS, PPS and AUD repeat (01 has 37 copies of its SPS and 1,072
  identical AUDs): use "same row in the previous NAL of this type" as
  context, or a match model.
- x264's version SEI (user data unregistered: a UUID, then the version and
  the options as text) is 667 bytes in 02, 2.4% of that stream, and 747 in
  04, coded as is. The TSV has it in hex: decode it and use a text model.
  zlib alone takes its text to 432 bytes.
- Slice header fields:
  - context: the same field in the previous slice header of the same slice
    type and nal_ref_idc;
  - frame_num increases by 1 after each reference picture;
  - pic_order_cnt_lsb follows the GOP/pyramid pattern;
  - slice_qp_delta depends on the slice type and the rate control;
  - pred_weight rows repeat their structure from picture to picture.
- `x264_padding`, `trail`, `czw`, `junk`, `hex`, `eof` are rare: code them
  literally.

### 4.2 Skip (`skip N`)

- **Baseline:** convert runs to per-MB flags. CABAC uses ctxIdxInc =
  (A available and coded) + (B available and coded). Coding the run
  lengths as numbers instead would lose the contexts of each MB's position
  (col, the 3×3 count), which make the gains below.
- **Add:**
  - co-located skip;
  - the number of skipped MBs in the 3×3 co-located window;
  - C and D (above right, above left), which are already coded;
  - the length of the current run;
  - slice QP and pyramid level;
  - for B pictures, the co-located MB in the list-1 reference;
  - with a decoder in the loop, how much the area changed between the two
    previous pictures (not measured).
- **Measured:** A and B as three states (unavailable, skipped, coded): −1.5%.
  Adding col: −6.8%. Adding the 3×3 count: −8.3% (00: −29%, 03: −1.3%).
- **End of slice:** there is no row; the slice ends where the next `nal`
  row starts. Code an "end" flag after each MB. It is certain at the last MB
  of a picture, and slice sizes are regular otherwise. CABAC spends 0.3% of
  the bits on it.

### 4.3 The mb row

**mb_type (T), 10.3% of the bits.**

- **CABAC:** only the first bin uses neighbours. In I slices: A/B not I_NxN.
  In B slices: A/B not skip or direct. In P slices: nothing. The other bins
  have fixed contexts.
- **Add:**
  - the classes of A, B, C and D;
  - the co-located class (as a separate input; as a single context it diluted
    the A/B context);
  - the number of skipped neighbours, QP, pyramid level.
  - For I16, its parts: the prediction mode (from the neighbours' I16 modes
    and boundary 4×4 modes), and chroma and luma cbp (from the neighbours'
    cbp).
- **Measured:** classes of A and B: −8.5% against an order-0 model per slice
  type. Adding col: −7.6%, worse (dilution). Adding the length of the skip
  run before the MB: −7.7%, worse too; the classes of A and B already tell
  it.

**transform_size_8x8_flag (X), 0.3%.**

- **CABAC:** the A/B flags.
- **Add:** class, luma cbp (for inter MBs it comes after cbp), QP and
  co-located flag.

**intra_chroma_pred_mode (C), 2.2%.**

- **CABAC:** first bin from A/B (intra, not PCM, mode ≠ 0); the rest fixed.
- **Add:**
  - the actual modes of A and B;
  - the luma prediction. Note the numbering differs: chroma uses 0 DC,
    1 horizontal, 2 vertical, 3 plane, while I16 uses 0 vertical,
    1 horizontal, 2 DC, 3 plane. For I_NxN, use the dominant direction of
    the 4×4 modes.
  - the co-located mode.

**coded_block_pattern (P), 5.8%.**

- **CABAC:** each luma bit from the bits of the 8×8 blocks to the left and
  above (inside the MB or in A/B); chroma from A/B chroma cbp ≠ 0, then = 2.
- **Add:**
  - the MB's own prediction, which the syntax codes before cbp: partition,
    ref_idx, and the mvd magnitudes of each 8×8 (zero motion in a static
    area makes a zero cbp likely);
  - the nonzero counts of the adjacent neighbour blocks rather than one cbp
    bit, the co-located cbp, QP, the intra modes, and the bits already set.
  - For chroma: luma popcount, chroma mode, co-located chroma cbp.

**mb_qp_delta (Q), 7.7%.** x264's adaptive quantization and macroblock
tree give each MB its own QP, which makes this element expensive.

- **CABAC:** the first bin from "previous MB in decoding order had a nonzero
  delta"; the other bins fixed.
- **The delta is relative to QP_pred**, the QP of the previous MB in decoding
  order. An MB without a delta (skipped, or cbp = 0) inherits QP_pred. So a
  delta after a run of such MBs covers several MBs' worth of adaptive-quant
  variation.
- **Add:**
  - the QP of A, B and col relative to QP_pred (the QP field is smooth in
    space and time, the delta isn't);
  - the distance to the last MB that coded a delta;
  - this MB's class and cbp, which come before Q;
  - slice type and slice QP.
- **Measured:** col QP: −1.3%. Col and above: −1.6%. Cbp and class: −2.8%.
  The previous delta's value: −0.1%; the last nonzero delta's: −0.7%.
- **Open question:** the remaining structure is the adaptive-quant offset
  field, which comes from source variance. The best proxies are probably the
  residual energy and the intra modes of the MB. Coding Q after the
  residual, which section 5 allows, would make the residual usable as
  context.

**fld (MBAFF).**

- **CABAC:** the flags of the left and above pairs.
- **Add:** the co-located pair's flag, and the neighbours' vertical motion.

### 4.4 ipm (intra 4×4/8×8 modes), 6.3%

CABAC uses one context for the flag and one for all three rem bits, so this
is nearly unmodelled. The TSV value is relative to the MPM =
min(mode A, mode B) (8.3.1.1), so reconstruct the actual modes:

- A neighbour that isn't available (or is inter with
  constrained_intra_pred) makes the MPM DC (2).
- A neighbour MB that isn't I_NxN (I16, PCM, inter, skipped) counts as DC.
- For a 4×4 block, an I8x8 neighbour gives the mode of its 8×8 block. For
  an 8×8 block, an I4x4 neighbour gives its 4×4 block 1 (left) or 2 (above)
  of the adjacent 8×8 block (8.3.2.1).

Then code the actual mode with contexts:

- the actual modes of A and B (I16 neighbours' real modes are worth more
  than the forced DC);
- the previous block's mode and the block index;
- 4×4 versus 8×8, co-located modes, QP;
- the geometry of the nine directions: 0 vertical, 1 horizontal, 2 DC,
  3 diagonal down-left, 4 diagonal down-right, 5 vertical-right,
  6 horizontal-down, 7 vertical-left, 8 horizontal-up.

**Measured:** the actual mode with (mode A, mode B) as context: −9.8%
(03: −15.7%) against the TSV's flag/rem with CABAC's contexts.

**With a decoder in the loop**, the decoded samples around the block show
which way its edges run. Make a histogram of the gradient directions there,
as DIMD in JVET's ECM does: a 3×3 Sobel along the second row above and the
second column to the left, as far as they are decoded, each gradient voting
for the nearest of the eight directional modes with weight |Gx| + |Gy|. The
strongest direction and the total gradient strength (DC is likely where it
is low) are the contexts.

**Measured:** the MPM, the strongest direction and the strength as one
context: −14.5% against CABAC, −5.2% against the modes of A and B. Mixed in
a logistic mixer with the modes of A and B and with the two strongest
directions: −19.0%, where mixing two contexts without the samples (the modes
of A and B, and those with the previous block's mode) gets −11.6%. Mixer
against mixer, the samples are worth −9.0% on 00, −7.1% on 01, −10.6% on 03
and −6.1% on 04. The same should work for the Intra16x16 and chroma modes
(not measured).

### 4.5 sub (sub_mb_type), 0.1%

Use the previous sub types in the MB and the neighbours' partition shapes.
In B slices, direct sub-MBs go together with direct neighbours.

### 4.6 ref0, ref1 (ref_idx), 1.4%

- **CABAC:** first bin from A/B ref_idx > 0 (adjusted for field/frame
  neighbours in MBAFF); the other bins fixed.
- **Add:** the actual refs of A, B and C, the previous partition's ref in
  the MB, the other list's ref, the co-located ref, and
  num_ref_idx_active. With several references, a binarization that asks
  "same as A?" and "same as B?" first may fit better than truncated unary
  (not measured). With x264's weightp, the list contains duplicates of the
  same picture with other weights.

### 4.7 mvd0, mvd1, 12.7%

- **CABAC:**
  - first bin from the sum of |mvd| of A and B: < 3, 3-32, > 32;
  - prefix bins 1-8 by index;
  - UEG3 suffix and sign in bypass;
  - separate contexts for X and Y.
- **Add:**
  - |mvd| of A and B separately, in finer classes;
  - the other component of the partition (Y after X);
  - partition size and shape, ref_idx and list;
  - for bi-predicted partitions, the other list's mvd;
  - QP (the encoder's λ grows with QP and favours small mvd);
  - for the signs: the signs of A and B and of the other component.
- **Measured:**
  - finer |A| and |B|: −0.7%;
  - adding the other component: −1.4%;
  - adding the co-located mvd: −1.0%, worse;
  - signs: −5.2% of the sign bits.
- **The next step is motion vector reconstruction.** The mvd is the
  innovation over the spatial prediction mvp, which is the median or
  directional rule of 8.4.1.3, plus P_Skip and B direct. That is why
  co-located mvd doesn't help, and why the neighbours' mvds serve as
  contexts (magnitudes, signs) but not as a predictor to subtract, such as
  their median. With mv = mvp + mvd reconstructed, the
  co-located and neighbouring *vectors* give a predicted mvd (mv_col − mvp)
  to use as context, and the quarter-pel phase of mvp can condition the low
  bits. h264p's walker only keeps |mvd| for CABAC, so this needs a good part
  of a decoder's motion vector prediction. It is the obvious target for the
  largest side-information element.

### 4.8 res (coefficients), 43.6%

**Present flag (coded_block_flag), 7.2%.**

- **CABAC:** the flags of blocks A and B. An unavailable neighbour counts as
  1 for an intra MB and 0 for an inter MB; a skipped neighbour, or one with
  its cbp bit off, counts as 0.
- **Add:**
  - nonzero counts of A and B;
  - intra/inter, QP, the block's intra mode, the mvd of its partition;
  - the number of nonzero blocks so far in the 8×8 and the position in it.
    In these streams x264 never sets a cbp bit for an empty 8×8 (271,116
    bits checked; Intra16x16, whose luma cbp is all or nothing, aside), so
    the fourth block is certain after three empty ones;
  - for chroma DC, Cr given Cb, plus the chroma cbp value. In these
    streams every chroma cbp of 1 has a nonzero DC block, and every 2 a
    nonzero AC block;
  - with a decoder in the loop, the gradients of the block's prediction:
    edges in a motion-compensated block turn small motion errors into
    residual (not measured).
- **Measured:** counts of A and B: −1.2%. Adding intra and QP/6: −1.4%. Cr
  chroma DC given Cb: **−18.3%**.

**Significance map, 18.6% (sig 11.9% + last 6.7%).**

- **CABAC:** scan position for 4×4 blocks; for 8×8 blocks, the standard's
  position maps (frame or field); per ctxBlockCat.
- **Add:**
  - nC = (nA + nB + 1) / 2 as in CAVLC;
  - the number of significant coefficients so far, or the number remaining
    with count first;
  - the same-position coefficients of A and B;
  - the block's intra mode (directional prediction leaves directional
    residual spectra) or its partition and motion;
  - (u, v) from the scan table instead of the scan index for cross-category
    sharing.
- **Measured (luma 4×4):** position and nC: −1.9%. Adding the count so far:
  −2.7%.

**Levels, 8.6%.**

- **CABAC:** first bin from the number of earlier levels > 1 and = 1 (coded
  in reverse scan order); then unary to 14 with an Exp-Golomb bypass suffix.
- **Add:** nC and position, the same-position magnitudes of A and B, the
  sum of magnitudes so far (JpegModel's ssum), QP.
- **Measured (luma 4×4):** nC and position: −1.2%.

**Signs, 9.3% (bypass in CABAC).**

- **Add:**
  - the signs at the same scan position in A and B, and the scan position;
  - for Cr, the sign of Cb at the same position.
- **Measured:** luma 4×4 signs: **−7.5%** (00: −11.6%, 04: −17.1%). Chroma
  DC signs of Cr given Cb: **−15.8%**.
- Low-frequency prediction errors are spatially coherent, for example a
  gradient the prediction missed. The two chroma residuals of a region are
  correlated through its colour. Signs are a tenth of the stream and CABAC
  doesn't model them at all, so even simple contexts recover part of that.
- **With a decoder in the loop, predict them.** Code a block's magnitudes
  first, then its signs. The decoder knows the block's prediction and the
  decoded samples above and to the left of it. Take the first six nonzero
  coefficients in scan order (the other signs go first, at 1 bit each), try
  their 64 sign combinations, and score each by how well the reconstructed
  block continues its surroundings: Σ|2R₁ − R₂ − X| along the top and left
  edges, where R₁ and R₂ are the two decoded rows or columns next to the
  block and X its own edge samples (the cost of ECM's sign prediction).
  Then code whether each of these signs agrees with the cheapest
  combination, with the coefficient's rank and the cost margin (the
  cheapest combination with this sign flipped, minus the cheapest) as
  context.
- **Measured:** the cheapest combination has the right sign for 68% of the
  1.42 million signs it covers (90% of the signs of CAT 2 blocks). That
  saves 12.8% of the luma 4×4 sign bits, and 15.6% together with the signs
  of A and B (00: −16.6%, 01: −12.4%, 03: −27.4%, 04: −22.0%). 8×8,
  Intra16x16 and chroma blocks can be done the same way (not measured).
- The trailing ones (CAVLC's: up to three ±1 at the end of a block) have
  60% of the luma 4×4 signs and compress like the others: 0.91 bits per
  sign with the signs of A and B, 0.85 with the prediction as well.

**Per block type:**

- **Intra16x16 DC (CAT 0):** the Hadamard transform of the 16 DC values. Use
  the I16 DC blocks of A and B at the same positions, and the I16 prediction
  mode.
- **Luma 8×8 (CAT 5):** CABAC's position maps as the base. Use the same
  position in the neighbouring 8×8 blocks.
- **Chroma AC (CAT 4):** Cr given Cb for the same block, plus the luma
  energy of the MB.
- **4:4:4 (CAT 6-13):** Cb and Cr given the luma block at the same position,
  since all three planes show the same edges.

### 4.9 pcm

pcm rows are rare: x264 uses I_PCM only where it is cheaper, such as noise
at QP 0. An image model with left and above samples is enough. The A field
is 0 in x264 streams (`x264_padding`).

## 5. Coding order

The model may code the values in any order its decoder can follow. The
decoder can buffer a macroblock, or a picture, before writing rows.

- **Within a macroblock:** the bitstream order (mb_type, prediction, cbp,
  the transform flag of inter MBs, QP, residual) lets cbp use the motion and
  QP use the cbp. Moving Q after the residual would trade the QP as a
  residual context for the residual as a QP context. Sign prediction
  (section 4.8) codes a block's signs after its magnitudes.
- **Per picture, in two passes:** code the side information of all MBs
  first, then the residual. The residual contexts can then use the modes and
  motion of all eight neighbours, not only the causal ones.
- **Across slices:** neighbours in other slices are usable (section 3.2).

## 6. Values that follow from others

Some values in the TSV follow from earlier ones: the standard requires
them, or x264 never does otherwise. A model should compute the first kind,
and give the second kind a probability close to 1 while keeping an escape
for other encoders. The stream pays for all of them: CABAC's contexts don't
know the rules, and header fields are coded in full. The figures below are
measured on 00-04.

**Required by the standard:**

| value | follows from | CABAC/header bits |
|---|---|---:|
| end_of_slice_flag | the slice's macroblock count: one decision per slice ("ends at the last MB of the picture") instead of one per MB | 97,896 |
| skip flags after the last coded MB | a "last coded MB of the slice" decision, when the slice runs to the end of the picture (3,732 slices, 1.55 million MBs) | 60,471 |
| frame_num | 0 at an IDR picture, else the frame_num of the previous reference picture + 1, mod MaxFrameNum (held in all 4,101 pictures). It doesn't hold with frame_num gaps or after an MMCO 5, and both fields of a frame share it | 16,404 |
| first_mb_in_slice | 0 for the first slice of a picture, else where the previous slice ended; not with FMO or arbitrary slice order | 4,101 |
| intra 4×4/8×8 modes on the picture border | modes that need missing samples aren't allowed (8.3.1.2, 8.3.2.2): DC only in the corner; horizontal, DC or horizontal-up on the top edge; vertical, DC, diagonal down-left or vertical-left on the left edge (11,779 blocks, no exception) | at most 8,191 |
| Intra16x16 and chroma modes on the border | likewise 1 or 2 of the 4 modes (14,331 and 18,977 MBs) | |

Outside the slice data, the NAL framing (start codes, zero bytes and NAL
header bytes: 26,625 bytes) and the emulation prevention bytes (4,469
bytes) are 0.86% of the stream. h264p regenerates them, and the only
framing value in the TSV, Z, is 1 in every `nal` row of these streams.
x264's own Annex B output in `test/gen` has the zero_byte exactly where
B.1.2 requires it: before SPS, PPS and the first NAL unit of an access unit
(all 5,178 NAL units of its 33 x264 streams). There Z follows from the NAL
type and the access unit boundaries.

**x264's habits (no exception in these streams):**

| value | rule | CABAC/header bits |
|---|---|---:|
| coded_block_flag of the last block of a group | a set cbp bit always has a nonzero block. So the flag is implied for: the 4th luma 4×4 block of an 8×8 after three empty ones (16,874 cases), Cr DC after an empty Cb DC with chroma cbp 1 (104,844), the last chroma AC block (1,240), the last Intra16x16 AC block (938) | 117,379 |
| coded_block_pattern of B_Direct_16x16 | never 0 (that is B_Skip), so chroma ≠ 0 when the luma bits are 0 (66,513 MBs) | 63,709 |
| sub_mb_type | never a sub-8×8 B partition, nor four direct sub-MBs. P sub-partitions are also always 8×8 in these streams; that follows from the encoder's partition settings | P: 16,513 |
| inter residual | decimation (below) | as a context |
| constant slice header fields | pic_parameter_set_id, the deblocking fields, cabac_init_idc and direct_spatial_mv_pred_flag never change within a stream | 23,218 |
| nal_ref_idc, idr_pic_id | nal_ref_idc is 3 for IDR, SPS and PPS, 2 for reference slices and 0 otherwise; idr_pic_id alternates 0, 1 | 156 |

**Decimation.** x264 zeroes the luma of an inter 8×8 block (with 4×4
transforms) whose coefficients score under 4. It also zeroes the AC of an
inter chroma plane scoring under 7. For each coefficient of magnitude 1,
the score adds 3, 2, 2, 1, 1, 1 or 0 by the number of zeros before it (back
to the previous nonzero coefficient); any larger level makes it 9. No coded
inter 8×8 block (139,832) or chroma plane (3,724) breaks the rule. With
CABAC and trellis, x264 doesn't apply it to 8×8 transforms (1 exception in
32,212). A model can keep the score of the 8×8 so far: while it is under 4,
more coefficients must follow.

**Options.** x264's version SEI lists its options. It is present in 02, 04
and the x264 streams of `test/gen`, but not in 00, 01 or 03. In 02's and
04's, `analyse` (0x3:0x113, 0x1:0x111) rules out sub-8×8 partitions, and
`decimate=1` turns on the rule above. `8x8dct` (0 in 04's: 04 has no 8×8
transform), `bframes`, `b_pyramid`, `direct`, `weightp` and `trellis` say
what else can occur. Without the SEI, a model learns the same from the
stream.

**Total:** about 0.41 Mbit of CABAC and header bits, 1.4% of the stream,
are spent on values these rules determine. CABAC pays 1.29 bits for each
implied 4th luma flag and 4.32 bits for each implied last Intra16x16 AC
flag.

## 7. Duplicates

**NAL units.** 01, 03 and 04 repeat their SPS and PPS (70 copies), 01
sends 1,072 identical AUDs, and 70 slices equal an earlier one byte for
byte. In all that is 10.8 KB (0.3%). As rows they cost next to nothing
once the model compares a NAL unit with the previous one of its type.

**Whole pictures.** 146 pictures are entirely skipped (99 in 00, 47 in
01). CABAC codes them in 11,675 bits, plus their headers. 20 decoded frames
equal the frame before them, and 19 of them are all-skip pictures (the
20th has one coded MB). x264 codes a repeated frame as skips, so
whole-frame dedup only saves those skip flags: one decision per picture
instead of one per MB.

**Macroblock syntax.** 69% of the coded macroblocks repeat an earlier one
exactly (all rows, same absolute QP). Most of them are cheap, 8.8 bits on
average:

| the coded MB repeats | MBs | bits | share of coded-MB bits |
|---|---:|---:|---:|
| the previous coded MB | 305,683 | 1,029,798 | 3.8% |
| the co-located MB of the previous picture of the same type | 50,536 | 233,558 | 0.9% |
| any earlier MB, for MBs over 30 bits | 6,545 | 443,451 | 1.6% |

Half of the expensive repeats are 01's caption, whose MBs come back
unchanged in its keyframes: 617 intra MBs (219,295 bits) repeat an MB of an
earlier keyframe at the same position, mostly in MB rows 14-16. A match
input over whole MBs catches these. Candidates are the previous MB, the
co-located MB of the previous picture of the same type, and the co-located
MB of the previous I picture.

**Residual blocks.** Counted by blocks, repeats are everywhere: 64-95% of
the nonzero blocks of 00, 01, 03 and 04 equal one of the last 256 distinct
blocks of their category, and they hold 27-71% of the coefficient bits. But
most of them have one or two coefficients, which take few values and cost
little to code anyway. Blocks with five or more coefficients repeat one of
the last 256 in 0.9-1.6% of cases (03: 13%). In 01 a quarter of them repeat
a block further back, and half of those are its caption, at the same
position in an earlier keyframe, which the macroblock match above already
finds. As a substitution, with a match flag per block and the rank in an
Elias-gamma code, a block dictionary costs more than it saves: it loses 1.0%
of the coefficient bits on 00, 8.3% on 03 and 8.4% on 04, and gains 0.2% on
01.

**Decoded pixels.** h264dec decoded each stream. Every macroblock (16×16
luma and both 8×8 chroma blocks) was compared, by 64-bit hashes, with three
candidates: the co-located MB of the previous picture in decoding order, the
co-located MBs of all older pictures, and all earlier MBs anywhere. These
are the CABAC bits spent on coded (not skipped) MBs whose pixels already
existed:

| stream | content | co-located, previous picture | co-located, older picture | elsewhere | of these, flat MBs | share of coded-MB bits |
|---|---|---:|---:|---:|---:|---:|
| 00 | screen recording of a web UI | 71,921 | 70,996 | 56,948 | 70,472 | 3.3% |
| 01 | slideshow, caption over black bars | 131,655 | 109,968 | 9,402 | 59,882 | 3.0% |
| 03 | dark phone video with a watermark | 650,078 | 2,102,181 | 1,044,659 | 2,577,899 | 43.5% |
| 04 | street footage, caption bar | 3,727 | 1,608 | 1,334 | 3,393 | 0.2% |

In 03, 43% of the coded-MB bits go to macroblocks whose pixels the decoder
already had, 2.6 Mbit of them to flat dark blocks (luma 16-31) whose colour
was already there. x264 didn't copy them; the source may lie outside its
reference pictures, or the MB is intra. A recompressor can, but only with a
decoder in the loop. It needs the prediction (intra from the reconstructed
neighbours, inter from the reference pictures) to turn "this MB will look
like block X" into predicted coefficients, by transforming and quantizing
X − prediction at the MB's QP. That is JpegModel's adv_pred with whole
earlier blocks as candidates. For flat blocks a cheaper context does part
of the job: the colours of flat MBs seen before at the same position.
h264dec decodes progressive 4:2:0 8-bit streams and could serve as that
decoder.

In 00 and 01 the static content is mostly skipped or repeats as syntax.
In moving camera video like 04 there is nothing to find.

## 8. Method

1. **Parse** the TSV into values, regenerate it, and compare: the text must
   come back identical, or the input goes to the escape path. Check the
   whole chain with `h264p d` and `cmp`.
2. **Reproduce CABAC first:** CABAC's binarization and contexts with the
   model's counters, element by element. The result should land near the
   stream size, a little below it because the model doesn't restart at each
   slice (section 3.4). With the simple counters of appendix A, the skip
   flags cost 2.51 Mbit against CABAC's 2.59. A large miss means the
   parsing or the neighbour state is wrong.
3. **Add contexts element by element**, and measure each element against
   the table in section 1. The experiments in appendix A were made with
   `tools/ctxexp.py`, which parses the TSV, keeps the per-picture state and
   codes chosen decisions under several context sets. It is a starting point
   for trying more.
4. **Test on all the test streams.** Tune for progressive 4:2:0 8-bit x264,
   but keep everything lossless on all of `test/002`, `test/gen` and
   `test/h264p` (CAVLC, interlaced, 4:2:2, 4:4:4, high bit depth, PCM, FMO).

h264p itself is a good host for the model. `SliceCoder` already walks the
syntax in both directions, for CAVLC and for CABAC, with the neighbour state
a model needs (`MbState`, `nb_loc`). A third backend that codes the same
values with the model would leave the TSV as a view for debugging: the
compressed file could decode straight to H.264.

### Order of work

1. Statistics kept across slices (2.2% of the stream, section 3.4) and the
   rules of section 6 (1.4%).
2. Coefficients (43.6%): signs from A/B and Cb, Cr given Cb, nC and the
   count so far, same-position neighbours.
3. Skip and mb_type (19.8%): temporal and C/D contexts, and the neighbours'
   classes.
4. mvd (12.7%): neighbour magnitudes and signs, then motion vector
   reconstruction.
5. Intra modes (8.5%): actual modes of A and B.
6. mb_qp_delta (7.7%) and cbp (5.8%).
7. A decoder in the loop (section 3.4): sign prediction and the edge
   directions of intra blocks, 0.22 Mbit (0.75%) more as single contexts,
   and the pixel matches of section 7 for content like 03.

The best single context of each element in appendix A saves 0.93 Mbit
in total, 3.3% of the slice data, before any mixing, and 1.15 Mbit (4.0%)
with the two that need a decoder in the loop, on top of the 0.63 Mbit from
not restarting at each slice. They leave out mb_type, cbp, chroma AC, the
8×8 and Intra16x16 blocks and motion vector reconstruction.

## 9. As implemented

`h264p c` and `d` (h264p_src/model.inc, recon.inc, fam.inc, IDX/) follow
sections 3-8 with these changes and additions.

**Structure.** The model codes the syntax straight from `SliceCoder`, no
TSV in between (section 8's "third backend"). Fifteen families (nal, hdr,
skp, mbt, cbp, ipm, mvs, qpd, cbf, sig, lvl, sgn, mvg, qpg, pcm) are oggcomp's
`tc_fam`, declared in IDX: each decision is predicted by ten counters
(A-E, H-L, one index each, each counter with an adaptive-rate and a fast
fixed-rate probability), an APM (index s), three mixers selected by indices m, w and
s, a final mixer and a second APM (index f); where CABAC codes the element,
the probability of CABAC's own context state is one more mixer input.
Where the model has a guess at a number (a hint: a level, a count, an mvd,
a QP delta), each decision of the number's cascade also sees what the
hint's own cascade decides there while the two agree, or which side of the
hint the number went, through counters of its own (and the final mixer is
selected by that state too). The signs of mvd and mb_qp_delta are
decisions of families of their own (mvg, qpg), coded once the magnitude is
known. end_of_slice_flag, after a slice of its kind that was the whole
picture, is an adaptive counter's of its own by the slice's kind, not a
decision of the skip family: 0 there all but for sure, it cost as much
time as the skip flag. Coefficients are coded
count first (the number of nonzero coefficients, then the significance of
each position knowing how many are left, the levels backwards, the
signs).

**A decoder in the loop** (recon.inc): the model reconstructs the pictures
of 8-bit 4:2:0 frame streams without scaling matrices (POC, reference
lists and marking, weighted prediction, quarter-sample motion compensation,
intra prediction, dequantization and transforms, deblocking), block by
block as the residual arrives. Contexts from the samples, in the order of
their gain on test/002:

| context | element | gain |
|---|---|---:|
| intra residual estimated from the reference: the block where the neighbours' motion (refined by template matching) puts the macroblock, less the prediction, transformed and quantized, in quarter levels | count, significance, levels, signs, intra cbp | 88 KB |
| texture of the prediction; sign prediction (the first signs searched for the best continuity of the block with its surroundings, section 7) | coefficients, cbp | 75 KB* |
| maps of the last picture (energy, change), DIMD | most | 44 KB* |
| skip prediction: how its luma continues the samples above and left (edge miss), how its motion predicts them (template matching), in B slices how far its two predictions are apart and which of them (or their average) predicts the samples around better; the best template match | skip, mb_type | 36 KB |
| the intra estimate's reference block matched by the macroblocks above, left and above right whole (a 16-sample template, where a vector to code does better with 2; the search starting from the neighbours' proxies, their vectors and the temporal one), and searched again before each later luma block of an Intra4x4/8x8 macroblock (the blocks decoded added to the template) and before the chroma (against the luma, decoded by then) | count, significance, levels, signs | 18 KB |
| motion vector by template matching search, the vector zero, col's mvd | mvd | 17 KB |
| hints: the intra estimate's level and count, the template match's mvd, the adaptive quantization line's QP delta | levels, count, mvd, qp delta | 13 KB |
| edge miss of the prediction, edge-projected level estimates | coefficients | 13 KB* |
| the sign of mb_qp_delta (its own family, mixed) by the line's delta, col's and B's QP, against the magnitude | qp delta | 16 KB |
| x264's adaptive quantization: a per-picture line of QP against log2 energy, from the energy of the macroblock's prediction (of an intra macroblock's proxy, on a line of its own); a picture's first macroblocks on the last picture's slope | qp delta | 13 KB |
| intra modes ranked in the proxy by SATD: how much worse the most probable mode does, in x264's lambda; the best two other modes and the lead of the first; the best Intra16x16 and chroma modes | intra modes, mb_type | 9 KB |
| the sign of mvd (its own family, mixed) by the template match's vector, the zero vector's, col's, against the magnitude; the neighbours' vectors' signs | mvd signs | 5 KB |
| the texture of the prediction and its edge miss against the quantizer's step | count, significance, cbp | 3 KB |
| which way the prediction's texture runs (log2 of its differences along rows over those along columns); for inter significance, the prediction's own level at the position | significance, count, levels | 13 KB |
| the texture of the reference at the predicted vector across the component, against the step (where it is flat, a search gains nothing) | mvd, ref_idx | 8 KB |
| the texture of the skip prediction against the step | skip, mb_type | 3 KB |
| B: the same macroblock in list 1's reference (skipped, class, the skipped ones around) | skip, mb_type | 4 KB |
| what x264's decimation still requires of a coded 8x8 block after its first 4x4 blocks | count | 1 KB |
| inter signs: the sign each coefficient would have were the residual a small shift of the prediction (the levels of the prediction's gradients), the shift fitted by least squares to the coefficients whose signs are known | signs | 1 KB |
| for mb_qp_delta, how much the macroblock changed between the last two pictures (where pictures stay, x264's macroblock tree lowers the QP) | qp delta | 0.4 KB |
| how far the mvds of the neighbours were from their template matches' (size; direction for the sign) | mvd, mvd signs | 0.3 KB |
| for mb_qp_delta, encoders other than x264: the slice's QP; the class of the energy (whose QP is no line of it: flat, then rising); the QP of the picture's last macroblock of the same kind and energy class, and of B, C and col where they are of the same kind | qp delta | 5.3 KB |
| B: the vector temporal direct would give, as the template search's candidate and the temporal context; col's vector scaled by the POC distance it spanned, the neighbours' on other references scaled to the partition's | mvd, mvd signs | 3.9 KB |
| the template search's second best start | mvd, mvd signs | 0.2 KB |
| a 4:2:0 chroma block's count by the coefficients of the luma under it; a chroma or Intra16x16 AC block's by its DC (from the DC levels' transform) | count | 1.4 KB |
| how far to trust the intra estimate: the template match of its vector, then how far the luma decoded so far is from it | coefficients, intra modes | 9 KB |
| the best intra mode in the reference | intra modes | 4 KB |
| P pictures: the coefficients of the block the vector points at in the reference (as it was: the last picture of a kind, found by POC), moved and still blocks apart; for cbp, the 4x4 blocks with coefficients under it | coefficients, cbp | 2 KB |
| ref_idx: the reference (of the first four, by its candidates alone) the template matches best | ref_idx | 1 KB |

Without the decoder: the significance of the eight positions before (12 KB
with the frequencies left of and above; the same position in the blocks to
the left and above, 1 KB), the last levels and signs coded in the block and
the higher-frequency levels (4 KB), a second, fast probability in each
counter (12 KB), a third mixer (4 KB), counters seven to ten (19 KB;
four to six were 16 KB) and, for the top six mantissa bits of a number
(but a level's or a PCM sample's), nodes by the bits above them (2 KB).
In the headers: each value against the last of its element at the same
place in its group and row in a run of it (pred_weight_l0's per
reference), frame_num and pic_order_cnt_lsb modulo their range, and
whether the slice before was a reference for frame_num (1.1 KB; 2.4 KB
on the 95 streams); the bytes nothing else models (x264's SEI) with a
match against a built-in dictionary of x264's user data, its options
with their defaults (0.6 KB: 02's 667 bytes of it take 84).

\* measured when added, with four counters and before the later contexts.

**Results** (test/002, against the CABAC cost of each element):

| element | CABAC | model | |
|---|---:|---:|---:|
| skip, end | 336,175 | 221,412 | −34.1% |
| mb_type | 365,120 | 259,426 | −28.9% |
| cbp, transform_size_8x8_flag | 216,100 | 153,390 | −29.0% |
| intra modes | 299,858 | 231,869 | −22.7% |
| sub_mb_type, ref_idx, mvd | 504,865 | 404,213 | −19.9% |
| mb_qp_delta | 271,297 | 182,415 | −32.8% |
| coded_block_flag, significance (count, significance) | 910,721 | 696,918 | −23.5% |
| levels | 302,611 | 219,815 | −27.4% |
| signs | 329,382 | 182,124 | −44.7% |
| macroblock syntax | 3,536,129 | 2,551,582 | −27.8% |
| whole streams (H.264 against compressed) | 3,603,993 | 2,556,839 | −29.1% |

The model tables take 226 MB.

**Tried, no gain:** the CABAC-style significance/last binarization instead
of the count; the mirrored list 0 vector for list 1 mvds (as a context, and
as where list 1's search starts); a motion split guess from template
matching for mb_type, and the spread of the neighbours' vectors (better on
test/002, as much worse on the other test streams); the second reference's
agreement for P skip, and the skip flags of the last picture where the
skip vector points; the intra proxy from the best of several references,
or averaged over two; its brightness corrected by the samples around; its
best mode with the decoder's edges (the proxy's own surroundings do much
better); a wider search for it; thicker templates for the other searches;
bi-prediction disagreement per block for B residuals; the neighbours'
distance from the adaptive
quantization line; the reference's QP where a block's motion points;
richer contexts for the mantissa (better on test/002, worse on the rest;
the bits above a mantissa bit as its node do better);
edge-projected levels, or the levels of the block the vector points at, as
hints for inter blocks; histories of skip flags and mb_qp_delta; a final
mixer selected by context (by the hint's state it helps); longer
significance histories (12, hashed); for mb_type and
transform_size_8x8_flag, the proxy's summed best Intra4x4, Intra8x8 and
Intra16x16 costs (little for their size); the template match's quantized
difference from the skip prediction for P skip; the misses of skip and
mb_type against the quantizer's step; CAVLC's nC as the count's hint;
the vector's fractional phase and bi-prediction for the residual; the
diagonal direction of the prediction's texture; the prediction's own level
for the levels; chroma AC decimation; more unary decisions before the
length for mvd (5 or 8, both worse); the template miss of the macroblock's
motion for the count; list 1's co-located macroblock in place of the last
B picture's (better on test/002, worse on the generated streams); twice
the rows for every hashed table (71 bytes); for skip, the picture's skip
rate so far; for mvd, a motion field projected from the last picture
(it seldom finds what col's vector does not), and the same scaled
candidates for the searches of ref_idx and of skip (79 bytes, slower);
the run of unchanged QPs and the place in the row for mb_qp_delta; the
luma under a chroma block for its significance and levels, and Cb's
count (rather than whether it has one) for Cr's; ranking the intra
remainder by the proxy (the contexts do better than its 2.9 bits); the
partition shapes of A and B for mb_type; a bi-predictive template
search for list 1's vector (x264's comes from its own search); the
global motion of the last picture as a candidate; the relative
template miss of the skip motion; for B slices, the block under the
vector from its reference (the same macroblock of the last B picture
does better); in the levels, the shift fitted to the signs (signs coded
with their levels).

## Appendix A: single-context experiments

`tools/ctxexp.py` on the five TSV files, with each stream starting from
empty statistics. Every variant codes the same values as binary decisions,
with one adaptive probability per context. The update is
p += (bit − p)/(n + 1.5), with the count n capped at 255, and there is no
mixing, except in the rows marked "mix": they mix the predictions of their
contexts in the logistic domain, with one weight set per tree node and
transform size (learning rate 0.02). The rows marked "decoder" use the
decoded samples (appendix B). The first variant of each element is the
baseline; changes are relative to it.

The experiments use these simplifications:

- neighbours across slice boundaries are allowed (these streams have one
  slice per picture);
- "col" is the MB at the same address in the previous picture of the same
  slice type and reference status;
- residual experiments cover luma 4×4 blocks (CAT 1 and 2) and chroma DC;
- mvd is coded as zero flag, unary to 8 and an Exp-Golomb escape, not as
  CABAC's UEG3.

| element | decisions | context | bits | change |
|---|---:|---|---:|---:|
| skip flag | 9,035,778 | CABAC: A, B coded | 2,510,505 | |
| | | A, B (unavailable/skipped/coded) | 2,472,517 | −1.5% |
| | | + col | 2,339,929 | −6.8% |
| | | + skipped count in col 3×3 | 2,302,598 | −8.3% |
| mb_type (6-bit tree) | 7,445,082 | slice type | 2,813,104 | |
| | | + classes of A, B | 2,574,117 | −8.5% |
| | | + class of col | 2,599,495 | −7.6% |
| | | classes of A, B + length of the skip run before the MB | 2,595,419 | −7.7% |
| intra modes | 2,011,675 | CABAC: flag, rem bins | 1,799,758 | |
| | | actual mode \| modes of A, B | 1,623,770 | −9.8% |
| | | + previous block's mode | 1,640,590 | −8.8% |
| | | decoder: actual mode \| MPM, strongest gradient direction, gradient strength | 1,538,893 | −14.5% |
| | | decoder, mix of: modes of A, B; the context above; the two strongest directions and the strength | 1,458,566 | −19.0% |
| | | mix of: modes of A, B; modes of A, B and previous block's mode | 1,590,331 | −11.6% |
| mb_qp_delta (unary bins) | 3,564,756 | CABAC: previous delta ≠ 0 | 2,010,315 | |
| | | + QP of col − QP_pred | 1,983,929 | −1.3% |
| | | + QP of B − QP_pred | 1,978,407 | −1.6% |
| | | CABAC + cbp, class | 1,954,551 | −2.8% |
| | | previous delta (−3..3) instead of ≠ 0 | 2,009,078 | −0.1% |
| | | CABAC + last nonzero delta (−3..3) | 1,995,655 | −0.7% |
| coded_block_flag, luma 4×4 | 1,220,368 | CABAC: flags of A, B | 1,097,707 | |
| | | counts of A, B | 1,085,011 | −1.2% |
| | | + intra, QP/6 | 1,082,072 | −1.4% |
| sig/last, luma 4×4 | 4,272,001 | CABAC: position | 3,453,856 | |
| | | + nC | 3,388,279 | −1.9% |
| | | + count so far | 3,361,084 | −2.7% |
| levels, luma 4×4 | 2,477,696 | CABAC: gt1/eq1 counts | 1,717,081 | |
| | | + nC, position | 1,695,992 | −1.2% |
| signs, luma 4×4 | 1,630,887 | none (bypass) | 1,630,887 | |
| | | signs at the same position in A, B | 1,508,517 | −7.5% |
| | | decoder: sign of the best-continuing combination; rank, cost margin | 1,421,657 | −12.8% |
| | | + signs at the same position in A, B | 1,377,024 | −15.6% |
| coded_block_flag, chroma DC | 611,888 | CABAC: flags of A, B | 523,907 | |
| | | + Cb's flag for Cr | 428,274 | −18.3% |
| signs, chroma DC | 466,819 | none (bypass) | 466,819 | |
| | | Cr: Cb's sign at the same position | 393,146 | −15.8% |
| mvd | 3,654,514 | CABAC: \|A\| + \|B\| class | 3,367,988 | |
| | | \|A\|, \|B\| separately | 3,344,483 | −0.7% |
| | | + other component | 3,322,203 | −1.4% |
| | | + col | 3,333,190 | −1.0% |
| signs, mvd | 458,508 | none (bypass) | 458,508 | |
| | | signs of A, B, other component | 434,585 | −5.2% |

The gains vary with content. For the skip flags with all contexts, they were
−29% on 00, −10% on 01, −1.3% on 03 and −6.6% on 04. For the luma signs:
−11.6%, −3.7%, −5.2% and −17.1%. On 02, which has only two pictures, larger
context sets lose to the learning cost; mixing avoids that.

## Appendix B: how the other figures were measured

The CABAC bits come from the same instrumented decoder as section 1. It also
costed each macroblock and each residual block, and tagged the bins whose
value a rule gives. For section 3.4 it costed every context-coded bin a
second time, with context states that are carried over from the previous
slice of the same type instead of initialized. Python scripts walked the TSV
files to check the rules and find repeated macroblocks and blocks, and
compared the decoded macroblocks from h264dec's output by 64-bit hashes of
their samples. A rule counts as holding when it had no exception in the five
streams; for x264's habits that says nothing about other encoders.

The "decoder" experiments used a copy of h264dec with two probes. One
writes the gradient histogram around each Intra4x4/8x8 block just before
the block is predicted. The other writes the predicted signs of each
nonzero luma 4×4 block (Intra4x4 MBs, and inter MBs without the 8×8
transform) just before its residual is added. At those points the current
picture is decoded up to the block and not yet deblocked, as a decoder in
the loop would have it. The probes left the decoded output unchanged (same
MD5), and their blocks matched the TSV's one for one.

None of these tools is in the repository.
