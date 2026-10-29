# H.264 and its counterparts in MPEG-4 Part 2 (m4v, XviD, DivX) and H.263

This document compares two lossless syntax-to-TSV converters:
- **h264p** for H.264 Annex B streams;
- **m4vtsv** (in this directory) for MPEG-4 Part 2 elementary streams.

For each part of H.264 syntax and coding, it lists the counterpart in MPEG-4 Part 2 (as written by
XviD and DivX) and in H.263, from which MPEG-4 Part 2 took its macroblock syntax.

Legend for the tables:

| Mark | Meaning |
|---|---|
| **=** | same thing, possibly with a different value range |
| **≈** | similar purpose but different semantics or granularity |
| **–** | no counterpart |

H.263 annexes are named where an optional H.263 mode is the counterpart. MPEG-4 Part 2 entries
cover the Simple and Advanced Simple profiles: I/P/B/S(GMC) VOPs, quarter-pel, 4MV, interlacing,
MPEG quantisation and video packets. These are what XviD, DivX and libavcodec use. MPEG-4's
"short video header" mode is H.263 baseline syntax.

## 1. The two TSV formats

Both tools remove the entropy coding and keep every syntax element as a value. Both also convert
each unit back and compare it with the original bytes, falling back to hex when they differ. They
differ in how a macroblock is laid out.

| | h264p (H.264) | m4vtsv (MPEG-4 Part 2) |
|---|---|---|
| **Unit** | NAL unit (Annex B): `nal zeros ref_idc type` | start-code chunk (`00 00 01 xx`) |
| **First line** | `h264p 1`, checked by `d` | `# m4vtsv 1` comment |
| **Raw data** | `hex` (payload of a NAL unit), `nalx` (empty or forbidden bit set), `junk` (before the first start code), `eof` (trailing zero bytes) | `H` (non-VOP chunk or junk), `R` (VOP), split into 32 KiB lines |
| **Emulation prevention** | removed, re-inserted by `d`; irregular escaping makes the unit raw | doesn't exist in MPEG-4 |
| **Parameter headers** | one row per syntax element, named as in the spec (`profile_idc 100`); grouped values in one row (`pred_weight_l0`, `mmco`, `cpb`) | VOS/VO/VOL kept as hex (`H`) and parsed only for context; a `#` comment summarises the VOL |
| **Picture header** | slice header rows, again one per element | one positional `V` row per VOP |
| **Macroblock** | several rows: `mb type t8x8 cpred cbp qpd`, then optional `fld`, `pcm`, `ipm`, `sub`, `ref0`, `ref1`, `mvd0`, `mvd1`, and one `res` row per coded block | one row per macroblock (`i`, `p` or `b`) with all fields in columns and six block columns |
| **Skipped macroblocks** | `skip N` run rows | one row per macroblock with `sk=1`; also B macroblocks skipped implicitly |
| **Position** | implicit (`first_mb_in_slice`, skip runs, slice groups, MBAFF pair order) | explicit `x y` columns |
| **Coefficients** | `res cat idx c0 c1 ... cN`: dense values in scan order up to the last nonzero one | `run:level` events, the last one implicitly "last" |
| **Coding choices recorded** | `trail` (unexpected trailing bits), `czw` (cabac_zero_words), `x264_padding` (x264's CABAC flush bit), PCM alignment bits | escape mode `E1`..`E3` when not the default, MCBPC stuffing counts (`st`), video packet stuffing (`K st`), `T` (bits after the last macroblock) |
| **Absent value** | `-` | `-` |

h264p writes the coefficient block `res 2 0 -26 4 4 0 -4 0 -1 0 0 -1 1 0 1` in m4vtsv's form as
`0:-26 0:4 0:4 1:-4 1:-1 2:-1 0:1 1:1`. The run is the number of zeros before a nonzero value, and
the end of the list marks "last". Going the other way, a run of r adds r zeros before the level.

On one 2-second 352x288 clip encoded at about 100 KB both ways, the two formats are close in size:

| Stream | Size | TSV size | Rows | Nonzero coefficients |
|---|---|---|---|---|
| x264 baseline (CAVLC) | 101 KB | 954 KB (9.4x) | 43322 | 122730 |
| x264 high (CABAC, B, 8x8) | 94 KB | 937 KB (10.0x) | 42449 | 129465 |
| XviD (B-frames) | 105 KB | 962 KB (9.2x) | 19544 | 118306 |

Residual rows take 73-75% of h264p's TSV. In m4vtsv the coefficients are part of the macroblock
rows.

## 2. Stream structure and headers

| H.264 | MPEG-4 Part 2 | H.263 |
|---|---|---|
| Annex B start code + NAL header | **=** start codes `00 00 01 xx`; the code value plays the role of nal_unit_type | **≈** PSC, GBSC, slice start codes (Annex K) |
| nal_ref_idc | **≈** implied: I/P/S-VOPs are references, B-VOPs never are | **≈** implied |
| emulation prevention bytes | **–** the VLCs and marker bits are designed not to emulate start codes | **–** |
| SPS | **≈** visual object sequence (`B0`), visual object (`B5`), video object (`00`..`1F`) and VOL (`20`..`2F`) headers | **≈** picture header PTYPE, PLUSPTYPE/OPPTYPE (H.263+) |
| &nbsp;&nbsp;profile_idc, level_idc | **=** profile_and_level_indication (VOS) | **–** (Annex X profiles aren't signalled) |
| &nbsp;&nbsp;pic_width/height_in_mbs, cropping | **≈** video_object_layer_width/height in pixels (13 bits); no cropping | **≈** source format, custom picture size (H.263+) |
| &nbsp;&nbsp;chroma_format_idc, bit depths | **≈** chroma_format (4:2:0 only), not_8_bit/bits_per_pixel (N-bit) | **–** |
| &nbsp;&nbsp;frame_mbs_only, mb_adaptive_frame_field | **≈** interlaced flag (field tools per macroblock) | **–** |
| &nbsp;&nbsp;log2_max_frame_num, POC type | **≈** vop_time_increment_resolution (timing only) | **≈** TR, PCF clock (H.263+) |
| &nbsp;&nbsp;max_num_ref_frames | **–** (P: 1 reference, B: 2) | **≈** Annex N/U |
| &nbsp;&nbsp;seq_scaling_matrix | **=** VOL intra/inter quant matrices (mpeg_quant) | **–** |
| &nbsp;&nbsp;VUI aspect ratio, colour, timing | **=** aspect_ratio_info, video_signal_type (VO), fixed_vop_rate | **≈** custom PAR, PCF (H.263+) |
| &nbsp;&nbsp;direct_8x8_inference | **≈** direct mode uses the co-located 8x8 vectors | **–** |
| PPS | **≈** VOL and VOP header fields | **≈** picture header |
| &nbsp;&nbsp;entropy_coding_mode_flag (CABAC) | **–** always VLC | **≈** Annex E syntax-based arithmetic coding |
| &nbsp;&nbsp;pic_init_qp, chroma_qp_index_offset | **≈** vop_quant (absolute per VOP); chroma uses the same quantiser | **≈** PQUANT; Annex T changes the chroma quantiser |
| &nbsp;&nbsp;num_slice_groups (FMO), ASO | **–** | **≈** Annex K rectangular slices, arbitrary slice order |
| &nbsp;&nbsp;weighted_pred, weighted_bipred | **≈** sprite brightness change (static sprites, not used by XviD) | **–** |
| &nbsp;&nbsp;deblocking_filter_control_present | **–** no in-loop filter (post-filters only) | **≈** Annex J in-loop deblocking |
| &nbsp;&nbsp;constrained_intra_pred | **–** | **–** |
| &nbsp;&nbsp;redundant_pic_cnt | **–** | **–** |
| &nbsp;&nbsp;transform_8x8_mode | **–** always 8x8 DCT | **–** |
| SEI (`sei_message`) | **=** user_data (`B2`): DivX/XviD/Lavc version strings | **≈** PEI/PSUPP (Annex L, W) |
| access unit delimiter | **≈** the VOP start code (`B6`) itself; GOV header (`B3`) | **≈** PSC |
| end of sequence/stream | **=** visual_object_sequence_end_code (`B1`) | **=** EOS |
| filler data, cabac_zero_words | **≈** MCBPC stuffing, stuffing bytes after a VOP, zero bytes before a start code | **≈** PSTUF, MCBPC stuffing |
| IDR picture | **≈** I-VOP after a closed GOV | **≈** INTRA picture |
| data partitioning (NAL types 2-4) | **=** data_partitioned + reversible VLC (m4vtsv keeps such VOPs raw) | **≈** Annex V data-partitioned slices |
| SVC / scalability | **=** VOL scalability (m4vtsv keeps such VOPs raw) | **=** Annex O |

### Encoder fingerprints

Some parts of a stream depend on the encoder rather than on the syntax, and both converters have to
reproduce them:
- **h264p** models x264's CABAC padding bit (`x264_padding`), which follows a fixed bit pattern by
  frame number, and finds x264 by its SEI UUID.
- **MPEG-4** has its own cases. libavcodec picks the shortest escape mode while XviD picks the first
  one available (m4vtsv defaults to XviD's choice). DivX and XviD packed bitstreams add not-coded
  placeholder VOPs. The user data strings identify the encoder and switch decoder workarounds; m4vtsv
  uses them for DivX 5.00 build 413's sprite trajectory quirk.

## 3. Picture and slice header

| H.264 slice header | MPEG-4 Part 2 (`V`, `K`) | H.263 |
|---|---|---|
| first_mb_in_slice | **=** macroblock_number of a video packet (`K mbnum`) | **≈** GN (GOB number), MBA (Annex K slice) |
| slice_type I/P/B | **=** vop_coding_type I/P/B (`V type`) | **≈** I/P picture types; PB-frames (Annex G, M); B pictures (Annex O) |
| slice_type SP/SI | **–** | **–** |
| – | S-VOP (global motion sprite, used by XviD's GMC) | **–** |
| pic_parameter_set_id | **–** | **–** |
| frame_num | **–** (decoding order is the stream order) | **–** |
| pic_order_cnt_lsb, delta_pic_order_cnt | **≈** modulo_time_base + vop_time_increment (`mtb`, `tinc`): display time | **≈** TR |
| idr_pic_id | **–** | **–** |
| direct_spatial_mv_pred_flag | **–** (direct mode is always temporal) | **–** |
| num_ref_idx_active_override, ref_pic_list_modification | **–** | **≈** Annex N reference picture selection, Annex U |
| pred_weight_table | **–** | **–** |
| dec_ref_pic_marking (MMCO) | **–** | **≈** Annex U memory management (H.264's MMCO derives from it) |
| cabac_init_idc | **–** | **–** |
| slice_qp_delta | **≈** vop_quant (`V quant`), quant_scale of video packets (`K quant`): absolute, 1-31 | **≈** PQUANT, GQUANT, SQUANT |
| disable_deblocking_filter_idc, alpha/beta offsets | **–** | **≈** Annex J |
| field_pic_flag, bottom_field_flag (PAFF) | **–** (VOPs are frames; top_field_first is a display flag) | **–** |
| – | vop_coded = 0: a not-coded VOP, "show the previous frame again" | **–** |
| – | vop_rounding_type (`rnd`) | **=** RTYPE (H.263+) |
| – | intra_dc_vlc_thr (`dcthr`) | **–** |
| – | alternate_vertical_scan_flag (`alt`) | **–** |
| – | vop_fcode_forward/backward: motion vector range | **–** (Annex D extends the range) |
| – | header_extension_code: VOP header copy in a video packet (`K hec`) | **≈** GFID, which only signals that the picture header changed |

## 4. Macroblock layer

### Macroblock types

| H.264 mb_type (h264p `mb` field 1) | MPEG-4 Part 2 (m4vtsv `t`) | H.263 |
|---|---|---|
| P_Skip (`skip`) | **≈** not_coded (`p sk=1`). MPEG-4 and H.263 copy with a zero vector; P_Skip uses the predicted vector. In an S-VOP it is GMC. | **≈** COD=1 |
| P_L0_16x16 (0) | **=** inter (`t=0`), MCBPC types 0/1 | **=** INTER, INTER+Q |
| P_L0_L0_16x8 (1), P_L0_L0_8x16 (2) | **–** (field prediction also uses two vectors, but for interleaved fields rather than halves) | **–** |
| P_8x8 (3) with all sub_mb_type 0 (8x8) | **=** inter 4MV (`t=1`), MCBPC types 2/5 | **=** INTER4V, INTER4V+Q (Annex F) |
| P_8x8 with 8x4, 4x8 or 4x4 sub-partitions; P_8x8ref0 (4) | **–** | **–** |
| I_NxN (0), I_16x16 (1-24), I_PCM (25) in I slices; +5 in P, +23 in B | **≈** intra (`i`, `p t=2`), MCBPC types 3/4 | **≈** INTRA, INTRA+Q |
| B_Skip (`skip` in a B slice) | **≈** modb `1` (`b mb=0`): direct, no delta vector, no residual | **≈** PB-frame B-part (Annex G) |
| B_Direct_16x16 (0) | **≈** direct (`b t=0` with `mb` 1 or 2): co-located vectors scaled by time, plus a coded delta vector | **≈** Annex G B-part with MVDB |
| B_L0_16x16 (1) | **=** forward (`b t=3`) | **≈** Annex M, O |
| B_L1_16x16 (2) | **=** backward (`b t=2`) | **≈** Annex M, O |
| B_Bi_16x16 (3) | **=** interpolated (`b t=1`) | **≈** Annex O |
| B 16x8 and 8x16 types (4-21), B_8x8 (22) | **–** (a B-VOP has one vector per direction, or two field vectors) | **–** |
| – | implicitly skipped B macroblock: no bits when the co-located one of the backward reference was skipped (`b sk=1`) | **–** |
| – | GMC macroblock (`mc=1`) | **–** |

### Other macroblock syntax

| H.264 (h264p row) | MPEG-4 Part 2 (m4vtsv field) | H.263 |
|---|---|---|
| mb_skip_run / mb_skip_flag (`skip N`) | **≈** not_coded per macroblock (`sk`) | **≈** COD |
| coded_block_pattern (`mb` cbp: 4 luma bits + chroma 0/1/2 << 4) | **≈** CBPY (4 luma bits) + MCBPC chroma bits (Cb and Cr separately); in m4vtsv implied by blocks that aren't `-`. For I_16x16 it is part of mb_type. | **=** MCBPC + CBPY |
| mb_qp_delta (`mb` qpd, -26..25) | **≈** dquant -2..2 (`dq`), B: dbquant 0/±2 | **≈** DQUANT -2..2; Annex T allows any change |
| transform_size_8x8_flag (`mb` t8x8) | **–** | **–** |
| prev_intra4x4/8x8_pred_mode_flag, rem_intra_pred_mode (`ipm`: -1 or 0-7) | **≈** ac_pred_flag (`ac`): DC/AC prediction in the DCT domain, its direction derived from neighbouring DCs | **≈** Annex I INTRA_MODE: DC, vertical or horizontal prediction of DCT coefficients |
| intra_chroma_pred_mode (`mb` cpred) | **–** (chroma uses the same ac_pred flag) | **≈** Annex I |
| I_16x16 prediction mode (in mb_type) | **–** | **–** |
| pcm_sample (`pcm`) | **–** | **–** |
| mb_field_decoding_flag (`fld`, MBAFF pairs) | **≈** dct_type (`dt`) and field_prediction (`fp`), chosen separately per macroblock | **–** |
| sub_mb_type (`sub`) | **≈** 4MV means four 8x8 parts; no smaller partitions and no B sub-types | **≈** Annex F |
| ref_idx_l0/l1 (`ref0`, `ref1`) | **–** (interlaced field vectors have field selects, `fs`, which choose the reference field) | **≈** Annex N/U |
| mvd_l0/l1 (`mvd0`, `mvd1`) | **=** motion vector differences (`mv`): P/S forward, B forward then backward, direct delta | **=** MVD, MVD2-4 (Annex F), MVDB (Annex G) |
| – | mcsel (`mc`) | **–** |
| – | MCBPC stuffing (`st`) | **=** MCBPC stuffing |
| – | modb (`mb`) | **≈** MODB (Annex G) |

### Motion vectors

The difference values match, but their units and predictors don't:
- **Units:** H.264 vectors are always quarter-pel. MPEG-4 vectors are half-pel, or quarter-pel when
  the VOL sets quarter_sample (XviD's qpel). H.263 vectors are half-pel.
- **Range:** H.264 differences are unbounded (se(v), or UEG3 in CABAC). MPEG-4 and H.263 code them
  modulo a range: MPEG-4 sets it with f_code, and m4vtsv keeps the coded difference before the
  modulo. H.263 widens the range with Annex D.
- **P predictor:**
  - all three use the median of the left, above and above-right vectors;
  - H.264 adds directional rules for 16x8/8x16 partitions, prefers a neighbour with the same
    ref_idx, and predicts P_Skip;
  - MPEG-4 and H.263 Annex F work on 8x8 vectors, with special cases at the first row of a video
    packet or GOB.
- **B predictor:**
  - H.264 uses the same median as in P slices, per list;
  - MPEG-4 B-VOPs predict from the previous macroblock's vector of the same direction, reset to zero
    at the start of every macroblock row and video packet, not from a median.
- **Direct mode:**
  - H.264 temporal direct and MPEG-4 direct both scale the co-located vector by temporal distances
    (POC in H.264, TRB/TRD from the time increments in MPEG-4). MPEG-4 adds a coded delta vector
    and also uses 8x8 vectors when the co-located macroblock had four.
  - H.264 spatial direct has no counterpart.

## 5. Residual data

| H.264 (h264p `res cat`) | MPEG-4 Part 2 | H.263 |
|---|---|---|
| cat 2: luma 4x4 blocks, idx 0-15 | **≈** the four 4x4 blocks of an 8x8 quadrant (idx 4k..4k+3) cover the area of luma block `b`k; MPEG-4 codes one 8x8 DCT block there | **≈** the same 8x8 blocks |
| cat 5: luma 8x8 (High profile, `t8x8=1`), idx 0-3 | **=** luma blocks `b0`..`b3`: 8x8, 64 coefficients | **=** |
| cat 0, 1: Intra16x16 DC (Hadamard over 16 DCs) + 15 AC each | **≈** intra DC coded separately per 8x8 block (`dc|...`), DPCM from neighbours with dc_scaler | **≈** INTRADC: 8-bit fixed length, no prediction |
| cat 3: chroma DC (2x2 Hadamard), idx 0 Cb, 1 Cr | **≈** DC of `b4`/`b5`: separate intra DC, or the first coefficient of the block | **≈** INTRADC |
| cat 4: chroma AC, idx 0-3 Cb, 4-7 Cr | **≈** chroma blocks `b4` (Cb), `b5` (Cr): one 8x8 block per component | **=** |
| cat 6-13: 4:4:4 Cb/Cr coded like luma | **–** | **–** |
| 4x4/8x8 integer transform | **≈** 8x8 DCT (IEEE 1180 accuracy; XviD's IDCT differs slightly) | **=** 8x8 DCT |
| zigzag / field scans | **≈** zigzag; alternate horizontal/vertical scans with AC prediction; alternate vertical scan for interlaced VOPs | **=** zigzag; Annex I scans |
| QP 0-51, step doubling every 6; chroma QP from a table | **≈** quant 1-31 linear: H.263 method (about (2·level + 1)·q) or MPEG method (matrix, mismatch control) | **=** H.263 method |
| scaling lists | **=** quant matrices (MPEG quantisation) | **–** |
| CAVLC: coeff_token by nC, trailing ones, level_prefix/suffix, total_zeros, run_before | **≈** 3-D VLC of (last, run, level) + sign, escape modes 1-3 (`E1`..`E3`), separate intra and inter tables | **≈** TCOEF 3-D VLC, 22-bit escape; Annex S alternative inter VLC |
| CABAC: significance map, levels, context modelling | **–** | **≈** Annex E arithmetic coding (fixed models) |

The coefficient values aren't comparable across the formats without dequantisation. H.264 levels
are quantised with a logarithmic QP and its own transform scale, while MPEG-4 and H.263 levels
multiply a linear quantiser. Only the run/level structure and the scan position carry over directly.

## 6. Prediction and reconstruction tools

These aren't TSV fields but determine what the fields mean.

| H.264 | MPEG-4 Part 2 | H.263 |
|---|---|---|
| spatial intra prediction: 4x4 and 8x8 with 9 modes, 16x16 and chroma with 4 | **≈** DC/AC prediction of DCT coefficients from the left or above block | **≈** Annex I |
| multiple reference pictures, long-term references | **–** (NEWPRED isn't used by XviD) | **≈** Annex N, U |
| partitions 16x16 down to 4x4 | **≈** 16x16, 4x 8x8; field vectors | **≈** 16x16, 8x8 (Annex F) |
| quarter-pel luma (6-tap + bilinear), eighth-pel chroma | **≈** quarter-pel with an 8-tap filter (qpel), or half-pel bilinear | **≈** half-pel bilinear |
| fixed interpolation rounding | **–** rounding alternates per VOP (rounding_type) | **=** RTYPE (H.263+) |
| weighted prediction (explicit or implicit) | **–** | **–** |
| in-loop deblocking | **–** | **≈** Annex J |
| – | global motion compensation (S-VOP, up to 3 warp points) | **–** |
| – | overlapped block MC (obmc_disable is always 1 in practice) | Annex F |
| unrestricted vectors | **=** always | **≈** Annex D |
| MBAFF, PAFF | **≈** per-macroblock field DCT and field prediction | **–** |

## 7. Summary

The H.264 parts with a direct counterpart in MPEG-4 Part 2:
- start-code delimited units;
- I/P/B picture types and B forward/backward/bidirectional 16x16 prediction;
- P 16x16 and 8x8 inter prediction;
- motion vector differences with median prediction in P pictures;
- temporal direct mode;
- coded block patterns;
- per-macroblock quantiser deltas;
- 8x8 coefficient blocks (High profile's 8x8 transform);
- quantisation matrices;
- user data or SEI, and the end of sequence code;
- data partitioning and scalability (MPEG-4 tools XviD doesn't use).

The parts with a partial counterpart:
- skipped macroblocks (zero vector instead of a predicted one);
- intra coding (DCT-domain DC/AC prediction instead of spatial prediction);
- MBAFF (per-macroblock field DCT/prediction);
- slices (video packets);
- timing (time increments instead of POC);
- 4x4 blocks (grouped into 8x8 areas);
- the separate DC transforms (DC prediction of each 8x8 block).

H.264-only:
- CABAC (H.263 has a simpler arithmetic coder, Annex E);
- multiple references, ref_idx and MMCO (H.263 Annex N/U);
- 16x8/8x16 and sub-8x8 partitions;
- B 8x8 sub-types and spatial direct;
- weighted prediction;
- the deblocking filter (H.263 Annex J);
- I_PCM;
- transform size switching;
- 4:2:2/4:4:4 and high bit depths;
- FMO/ASO (H.263 Annex K), redundant slices, SP/SI, PAFF;
- emulation prevention.

MPEG-4/H.263-only:
- GMC S-VOPs;
- not-coded VOPs;
- implicit B skips;
- f_code ranges and modulo vectors;
- rounding type;
- escape modes and MCBPC stuffing;
- intra_dc_vlc_thr and DC prediction;
- the time increment syntax;
- video packet header extensions.

## 8. Using both TSVs together

- **Macroblock positions:** m4vtsv rows carry `x y`. For h264p, count macroblocks from
  first_mb_in_slice, adding `skip N` runs, and follow the slice group map and MBAFF pair order when
  those are used.
- **Coefficients:** convert h264p's dense `res` lists to `run:level` (or the reverse) as in
  section 1. To compare areas, group H.264 luma 4x4 blocks 4k..4k+3 into m4vtsv block `b`k.
- **Types:** use the mapping in section 4. B types differ in numbering: H.264 1/2/3 are forward,
  backward and bidirectional, while MPEG-4 3/2/1 are forward, backward and bidirectional.
- **Motion vectors:** scale MPEG-4 differences by 2 to quarter-pel when quarter_sample is off. Keep
  in mind that MPEG-4 B differences come from a different predictor.
- **Coded block pattern:** h264p stores it in the `mb` row; m4vtsv implies it from blocks written as
  `-`.
- **Order:** both store decoding order. For display order use POC (H.264) or modulo_time_base and
  vop_time_increment (MPEG-4).
