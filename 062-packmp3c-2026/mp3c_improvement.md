# mp3c: where the bits go, and how to compress better

This document measures what mp3c does with an MP3 now, part by part, and
lists what would make its output smaller, ordered by the expected gain. Most
of the ideas come from aacpack, on branch
`claude/aacpack-lossless-compressor-rlba00`:

- `aac_cm_design.md` is its context mixing (CM) design.
- `aac_cm_design_results.md` gives what each idea measured there.
- Its model is h264p's context mixing, with families of decisions declared
  in IDX (`aacpack_src/IDX/IDX-FORMAT.md`).

Where an idea was cheap to try on mp3c, I tried it on a copy of the sources,
and the numbers below are measured. The remaining numbers are estimates and
are marked as such.

## Summary

- **Current ratios.** mp3rw + mp3c compress what a current encoder writes
  (LAME 3.100, joint stereo) to 88.4%. A set of real-world files, mostly older
  plain-stereo encodes, goes to 76.4%.
- **Values and signs dominate.** 92–96% of the `.c` is the spectral values
  and their signs.
  - The signs code to 97–100% wherever mp3c has no other copy of them to
    look at, as AAC's do in aacpack (99.8%).
  - Side information is 4–8% of the `.c`. Most of it already codes to under
    half its original bits; the scalefactors code to 58–84%.
  - So nearly everything left to gain is in the spectral values.
- **Mixing layer.** An untuned mixing layer, built like aacpack's model, on
  top of mp3c's spectral decisions takes 0.85% (LAME joint stereo) to 1.24%
  (real-world set) off the `.c`.
  - Of its new contexts, most of the gain comes from one: the last value at
    the same place, rescaled by how the quantizer step there changed.
  - A family per decision that replaces mp3c's model and is tuned should do
    better. That is not measured.
- **Bigger gains outside the spectral model.**
  - Movies: carrying the bit reservoir across the video data would save
    5–20% of the file, estimated from coding the audio alone.
  - Repeated material: a match model. Today 40 copies of a file cost 39.3
    times one copy.
- **Small and nearly free.** Using the right scalefactor band table at 48,
  32 and 22.05 kHz saves 0.06–0.45%.

## 1. What mp3c codes, and how

An mp3rw `.dmp` record of a Layer III frame holds the parsed frame:

- the header and side info;
- the scalefactors;
- the 576 quantized values of each granule and channel;
- the bits between one granule's main data and the next (`Garbage`).

mp3c codes the record as binary decisions with a range coder (15-bit
probabilities). The model's parts run in this order (`model__mp3.inc`):

| what | how |
|---|---|
| header fields | adaptive, no context (padding: a periodic model) |
| scfsi | under the channel, the last frame's scfsi of the group, the group before |
| global_gain | the difference to the channel's last one, MSB first |
| scalefac_compress, window flag, block type, preflag, scalefac_scale, count1table | small trees under a few bits of context |
| scalefactors | MPEG-1 long blocks: under the band, the band below, the bit length and the last value coded under those (`rec`, roughly the last granule's value at the band). MPEG-1 short blocks: under the band and scalefac_compress. MPEG-2: under the position only |
| where the values end, where the count1 region starts | two numbers (`ProcessCounts1`), then big_values from them |
| the spectral values | `ModelMDCT`, below |
| region counts, table_select | after the values. table_select is coded under the cheapest table for the region's values (`CalcTableSelect`); with the region counts it costs 18–25% of its bits |
| part2_3_length | computed from the values and the scalefactor lengths, plus a flag for when it differs |
| the bits between granules | order-1 bits; their count follows from part2_3_length |
| main_data_begin | predicted from where the last frame's main data ended |

A spectral value is a series of decisions:

1. zero? (separately for the big_values region and the count1 region)
2. one?, two?, three?, four?
3. unary up to 11
4. two escape flags, then binary digits (modelled below 268, raw above)
5. the sign, last.

**How each decision is coded.** Each decision has:

- two counters under their own contexts;
- an SSE stage on one of them;
- a mixer of the two with a single weight, selected by a third context;
- a byte of the decision's own recent outcomes.

This is declared in `IDX/sh_model.inc` and `IDX/sh_model.idx`.

**The contexts.**

- the place j, and its band;
- the band's scalefactor (`curband`);
- the three values before j in the granule;
- the values at j in the last three granules of the same history (and at
  j-1 in the last one);
- the zero and sign history at j;
- an expected magnitude `l` computed from those and from a fading average
  (`av_MDCT`);
- count1table_select.

The context thresholds are IDX patterns, tuned with `auto_opt2.pl` and
folded into constants (`Const 1`).

**The histories.** The value histories (`m_rDelta[8]`) are kept per block
type and stereo mode:

- In plain L/R stereo both channels share one history, so for the second
  channel the "last value at j" is the first channel's value in this granule.
- In M/S and intensity stereo each channel has its own history.

**Already done well.** mp3c already does several things that aacpack later
found to pay off:

- table_select is coded after the values, from what an encoder would choose
  for them. This is aacpack's "section data after the spectra".
- Fields that follow from the values (part2_3_length, big_values,
  main_data_begin) are coded as a flag that they match the computed value.
- aacpack's per-place fading average is mp3c's `av_MDCT`.

## 2. Where the bits go

I measured this with a copy of mp3c whose range coder charges each
decision's cost to the part of the stream it belongs to. The copy also counts
what each part took in the MP3, from the frame record (appendix). It writes
the same `.c` files as mp3c.

There are two test sets:

- **Real-world:** 9 files, 42.5 MB.
  - 8 MP3s from samples.ffmpeg.org (mp3rw's tests use them) and `cat1.mp3`.
  - Seven are plain L/R stereo at 128–320 kbps, from LAME 3.90–3.96 and
    other encoders.
  - `Silent_Light` is joint stereo; `cat1` is mono at about 80 kbps.
- **LAME joint stereo:** 4 files, 16.4 MB.
  - Two of those tracks (sources at 320 and 256 kbps) decoded and encoded
    again with LAME 3.100 at `-V2` and `-b 128`.
  - Joint stereo is LAME's default, so these look like what current encoders
    write.

All sizes below are in bytes:

| part | real-world: in the mp3 | mp3c | | LAME JS: in the mp3 | mp3c | |
|---|---:|---:|---:|---:|---:|---:|
| spectral values (codes, linbits) | 28344016 | 23186379 | 81.8% | 11103791 | 10172664 | 91.6% |
| their signs | 8825432 | 6553138 | 74.3% | 3246037 | 3219683 | 99.2% |
| scalefactors | 1319961 | 763664 | 57.9% | 1022537 | 855869 | 83.7% |
| global_gain | 270510 | 121907 | 45.1% | 126104 | 68509 | 54.3% |
| table_select, region counts | 685230 | 122486 | 17.9% | 318478 | 80813 | 25.4% |
| other side info | 879018 | 171887 | 19.6% | 410561 | 74033 | 18.0% |
| header, CRC, private bits | 297438 | 2801 | 0.9% | 137926 | 6797 | 4.9% |
| bits between granules | 351206 | 11232 | 3.2% | 18075 | 913 | 5.0% |
| all | 40972810 | 30933554 | 75.5% | 16383508 | 14479301 | 88.4% |

Notes on the table:

- "Spectral values" includes big_values, which mp3c codes along with where
  the values end.
- A file's other bytes (tags, cover art) are in mp3rw's `.out`.

**By stereo mode and channel.** Only the values and their signs are counted
here. "Share" is that channel's part of those bits.

| | real-world: share | values | signs | LAME JS: share | values | signs |
|---|---:|---:|---:|---:|---:|---:|
| mono | 0.4% | 91.5% | 99.7% | | | |
| L/R, first channel | 45.1% | 90.4% | 99.4% | 1.2% | 90.9% | 99.8% |
| L/R, second channel | 45.0% | 70.1% | 43.3% | 1.2% | 86.1% | 71.0% |
| M/S, mid | 6.9% | 92.2% | 99.0% | 64.6% | 90.8% | 99.6% |
| M/S, side | 2.7% | 87.6% | 96.6% | 32.9% | 91.0% | 99.5% |

Per file, as `.c` + `.out`:

| file | | size | `.c` + `.out` | |
|---|---|---:|---:|---:|
| `01_-_Charity_Case` | 320 kbps, L/R | 7699852 | 5797036 | 75.29% |
| `Ed_Rush_-_Sabotage` | 128 kbps, L/R | 7036343 | 4858663 | 69.05% |
| `Silent_Light` | 128 kbps, joint | 4206720 | 3612018 | 85.86% |
| `broken-first-frame` | 192 kbps, L/R | 102400 | 87135 | 85.09% |
| `cat1` | 64–96 kbps, mono | 160456 | 138591 | 86.37% |
| `jpg_in_mp3` | VBR 128–192, L/R | 4524539 | 3713604 | 82.08% |
| `mp3_misidentified` | 256 kbps, L/R | 7016448 | 5292320 | 75.43% |
| `mp3_misidentified_2` | 192 kbps, L/R | 9705037 | 7037717 | 72.52% |
| `mp3_with_embedded_albumart` | 320 kbps, L/R, cut at 2 MB | 2097152 | 1973558 | 94.11% |
| LAME 3.100 `-b 128`, `-V2` (two tracks each) | joint | 16385462 | 14479445 | 86.66–89.03% |

What the tables show:

- **L/R stereo.** Much of what mp3c gains on the real-world set comes from L/R
  stereo. The encoder left the redundancy between the channels in, and the
  shared history finds it.
- **Other channels.** Elsewhere, the values code to about 90% and the signs
  to 97–100%. MDCT signs are close to coin tosses unless a copy of them
  exists.
- **Side information.** Everything from global_gain down is 1.4–1.6% of the
  `.c`. The scalefactors are 2.5–5.9%.

## 3. Changes, by what they would gain

### 3.1 Movies: the bit reservoir across other data (mp3rw)

**The problem.**

- In a movie, each audio frame sits between pieces of video, so for mp3rw
  each frame is a sequence of its own (`README.md`, "Audio between other
  data").
- A frame whose main data starts in the bit reservoir (most frames) cannot
  be parsed there, so its whole body goes to `.out` as it is.

The table compares each file now with the file's audio coded alone plus the
rest of the file:

| file | now | audio alone, coded, + the rest | |
|---|---:|---:|---:|
| `movie_l3.avi` (mp3 128 kbps) | 2571807 | 822235 + 1625603 = 2447838 | −4.8% |
| same, MKV | 2495300 | 822235 + 1547478 = 2369713 | −5.0% |
| same, MP4 | 2524532 | 822235 + 1552289 = 2374524 | −5.9% |
| `lowvideo.avi` (mp3 320 kbps) | 1768585 | 1229802 + 180284 = 1410086 | −20.3% |

Of the 62 KB that `movie_l3.avi`'s `.c` holds now:

- 13.7 KB is the raw part2_3_length of frames that could not be parsed;
- 7.9 KB is its 2295 sequence records.

**The fix.** Keep the sequence going across the other data.

- **Reading (`mp3rw c`).**
  - At the first byte of other data after a Layer III frame, set the end of
    the frames aside: the reservoir is at most 511 bytes of main data, plus
    what follows the last main data, about 2 KB in all.
  - When the next Layer III frame's side info has been read, put that end
    back right above the frame, as if the other data were not there.
    mp3rw already removes headers and side info from between main data the
    same way.
- **Recording.**
  - The other data goes to `.out` as now.
  - A new `.dmp` record says how many bytes of it come before the frame: a
    value with a kind bit, like the sequence records.
- **Writing (`mp3rw d`).**
  - mp3rw writes a frame only once the main data of the frames after it is
    in place (`FrameFlush`). So it has to hold such bytes until the frame
    before them is written.
  - To bound the memory, cap how much other data a sequence carries across;
    past the cap, end the sequence as now.
- **mp3c** codes the new record like the sequence records, which should
  become adaptive (3.7).
- **Format.** This changes the `.dmp` and `.out` formats.

**Layer I/II in containers.**

- mp3rw takes a layer I/II frame only when it sits next to another one.
- In an AVI every frame has video data on both sides, so mp3rw takes 11 of
  `movie_l2.avi`'s 375 Layer II frames.
- Also taking a frame, after other data, whose header matches the last layer
  I/II frame seen would find them. It needs the same new records.

### 3.2 Repeated material: a match model

**The problem.** mp3c learns nothing from exact repeats.

- `cat1_x40.mp3` is 40 copies of `cat1.mp3`.
- It codes to 5423911 bytes, which is 135598 per copy, against 138150 for one
  copy alone.

**aacpack's digest match model.**

- Every value coded (its field and its value) goes into a ring of the last 4M
  tokens.
- The last 4 tokens are hashed to find where they occurred before.
- The token that followed then is a context of the family coding the next
  value. For the spectral values it is used once the match has been right 16
  times.

**What it measured there.**

- a 9.6 MB stream with repeats: from 8.2 MB to 2.9 MB;
- FFmpeg's FATE streams: −8.6%;
- streams without repeats: −0.04% on its dev set.

**When MP3s repeat.**

- concatenated files;
- a track stored twice in one archive;
- loops and digital silence. Here a repeat in the audio codes to the same
  values only if the encoder's reservoir and rate control are also in the
  same state.

For mp3c, the tokens would be each granule's side info fields and its
values.

### 3.3 The spectral values: more contexts, mixed

**The two models.** mp3c mixes two counters per decision. aacpack's family
(h264p's `tc_fam`, `aacpack_src/fam.inc`) has, per decision:

- ten counters, each with two probabilities (an adaptive rate and a fast
  1/8);
- an APM on the first counter;
- an outside prediction and a bias;
- three mixers of 24 inputs, whose weight sets three contexts select;
- a fourth mixer combining those, and an APM after it.

There is a family per kind of value, each with its own contexts and
parameters in an `.idx` file.

**The experiment.** I built a layer on top of mp3c's own decisions. For each
zero, magnitude and sign decision it mixes mp3c's prediction with:

- ten more counters, each with an adaptive rate down to 1/257 and a fast
  rate of 1/16, under hashed contexts;
- a bias.

The mixer's weight sets are selected by:

- the decision;
- the band;
- the last value at j;
- the channel.

An APM follows the mixer and gets a quarter of the weight. Nothing was tuned.

Ten contexts were tried for the zero and magnitude decisions:

1. the place, with the last value at j;
2. the last granule about j (j−1, j, j+1), with the values before j;
3. the zero history at j;
4. the fading average, with the band's scalefactor;
5. the three values before j;
6. the last three granules at j;
7. the rescaled reference (3.4);
8. the distances to where the count1 region and the zeros start;
9. the band's magnitudes so far, and the places left in it;
10. the last granule from j−2 to j+2, and two granules back.

The ablations below show that 8–10 each cost a little, 9 the most. The
results table is for the layer without 9. The sign decisions have six
contexts of their own:

- the reference's sign at j and about it;
- the signs before j;
- the sign history at j;
- the signs at j in the last three granules;
- the place.

**Results.** The change in coded size, with the layer on all three kinds of
decision:

| | zero flags | magnitudes | signs | `.c` |
|---|---:|---:|---:|---:|
| real-world | −1.25% | −1.96% | −0.39% | −1.24% |
| LAME joint stereo | −0.99% | −1.41% | −0.34% | −0.85% |

Ablations on the zero flags (real-world set):

- With contexts 1–6 only: −0.69%.
- With all ten: −1.12%.
- With all ten except the rescaled reference (7): −0.79%.
- With all ten except 9: −1.25% (the table's).
- Without mp3c's own prediction, the ten contexts alone give −0.61%. So
  untuned, they already code the zero flags better than mp3c's tuned model
  alone.
- For the magnitudes, without mp3c's prediction: −0.15%.

**Cost.** The layer is 8.5 times slower than mp3c: 48.4 s against 5.7 s for
a 9.7 MB file. It uses hashed tables and none of aacpack's SIMD or
prefetching.

**Proposal.**

- Bring aacpack's family coder into mp3c: `cm.inc`, `fam.inc`,
  `famcfg.inc`, `mapping.inc` and the generator.
- Make each group of decisions a family:
  - zero in the big_values region;
  - zero in the count1 region;
  - the magnitude cascade, with the decision as the node;
  - the values from 12 up, with tc_fam's integer coder, which codes a length
    and then its digits;
  - the signs.
- At first, give each family mp3c's tuned contexts as indices. The current
  thresholds can be written as threshold mappings in the newer IDX dialect.
  Then add the rescaled references and tune.
- **Expected:** more than the layer gives. A family replaces mp3c's decision
  instead of sitting on top of it, and it gets tuned. In aacpack's
  ablations, each part of the family (the fast counters, the weight-set
  selection, the final APMs) was worth about 0.14% of the total. Not
  measured for mp3c.

### 3.4 Rescaled references

**The formula.** MP3 dequantizes long blocks as

    |x| = |q|^(4/3) · 2^((global_gain − 210)/4) · 2^(−m (sf + preflag·pretab))

where m is 1/2, or 1 with scalefac_scale. Short blocks also have a factor of
2^(−2·subblock_gain) and use the scalefactor of the window. So a value q′
from another granule at the same place, quantized with step S′, predicts

    |q| ≈ |q′| · 2^((3/16)(S′ − S))
    S = global_gain − 210 − 2 (1 + scalefac_scale) (sf + preflag·pretab)

**Why it matters.**

- mp3c's references are the values as they were quantized.
- global_gain moves from granule to granule with the encoder's rate control,
  and the scalefactors move with the masking.

**Measured.** Rescaling the last granule's value at j makes up most of what
the new contexts bring in the layer (3.3): on the zero flags, the four new
contexts take the layer from −0.69% to −1.12%, and without the rescaled
reference it reaches only −0.79%. aacpack rescales the same way:
its `ex`, and its average "as the band's scalefactor now quantizes it".

**Where to apply it.**

- the last granule's values at and about j;
- the other channel's value. In plain stereo the two channels' global gains
  differ;
- `av_MDCT`, kept together with the step it was summed at;
- the expected magnitude `l`.

### 3.5 The other channel in M/S and intensity stereo

**The gap.** In M/S and intensity stereo, the second channel never sees the
first: they have separate histories.

**Measured.** In the layer, I added a context of the first channel's values
at and about j:

| set | side channel's values | `.c` |
|---|---:|---:|
| `Silent_Light` | −0.81% | −0.13% |
| LAME joint stereo (switches between M/S and L/R frame by frame) | | −0.836% instead of −0.849% |

**Verdict.** Worth one try in a family, with the stereo mode of this frame
and of the last one as context. Not more.

### 3.6 The scalefactor band table

**The bug.**

- mp3c's band of a place (`band[]`, `InitBand` in `model_delta.inc`) is the
  44.1 kHz long-block table for every sample rate.
- `curband`, a context of nearly every spectral decision, is the scalefactor
  of that band. At 48 and 32 kHz, and in MPEG-2/2.5, it is mostly another
  band's scalefactor.
- Short blocks get 0 there.

**Measured.** With the frame's own table (`bandInfo[sfreq]`), on LAME
re-encodes of one track:

| rate | `.c` |
|---|---:|
| 22.05 kHz, 64 kbps | −0.45% |
| 48 kHz, 160 kbps | −0.12% |
| 32 kHz, 128 kbps | −0.06% |
| 44.1 kHz, 256 kbps | 0.00% |

Short blocks should get their window's scalefactor.

### 3.7 Side information

**Scalefactors** (2.5–5.9% of the `.c`, at 58–84% of their bits):

- MPEG-1 long blocks are coded under the band, the band below and the bit
  length. Of the last granule there is only `rec`, the last value coded
  under those, and nothing comes from the other channel.
- Short blocks and MPEG-2 have no value context at all.
- aacpack's scalefactor contexts that paid:
  - the last frame's delta and value at the band;
  - the other channel;
  - the change of global_gain;
  - in short windows, the previous group's value.
- An estimate, not measured: a tenth of their cost, 0.3–0.6% of the `.c`.

**global_gain** (0.4–0.5%):

- It is coded as the difference to the channel's last value.
- Candidate contexts: the other channel's difference in this granule, and the
  difference at the last granule.

**Sequence records:**

- The flag that marks a sequence record has a fixed probability: about 15
  bits per record, and its value goes in raw bytes.
- For a plain MP3 that is nothing, but it is 7.9 KB of `movie_l3.avi`'s
  62 KB `.c`. It should be adaptive.

**Frames mp3rw cannot parse** (`p23cleared`):

- Their part2_3_length is stored raw (12 bits per granule), and the rest of
  their side info is coded as usual.
- 3.1 removes most of them.

### 3.8 Layer I/II

mp3c's Layer I/II model is small and hand-made. It codes the streams tried to
44–87%. The same family approach applies: the subband samples play the part
of the spectral values, and the scalefactors repeat across frames. This is
worth it only for Layer II collections (broadcast captures, DVD and VCD
audio).

### 3.9 Tuning with IDX

**The dialects.**

- mp3c's `.idx` files use the older dialect: `Rate` (an index into the rate
  tables), `ADD`, 576-bit masks over the place j, and `Const 1` on most
  lines (the folded state).
- h264p's generator (aacpack's) has neither `Rate`, `ADD` nor `b&`
  (IDX-FORMAT.md §4).
- So either keep mp3c's generator for its present model and write new
  families in h264p's form, or move everything over.

**The loop** (IDX-FORMAT.md §11):

1. A tuning build (`Debug 1, Const 0`), in which `opt.pl` flips the pattern
   bits inside the executable. `auto_opt2.pl` is its ancestor.
2. `import.pl` folds the winners back into the `.idx` sources.
3. A shipping build (`Const 1`), which must write the same bytes as the
   tuning build.

**Faster tuning.**

- aacpack dumps each family's decisions with their inputs once
  (`AACPACK_DUMP`).
- `IDX/famtune.cpp` and `IDX/ftune.py` then replay one family under new
  parameters, which is much faster than compressing again.
- That matters once a decision costs 5–10 times what it costs now.

**The tuning corpus.**

- What mp3c's patterns were tuned on is not in the tree. Their per-place
  masks (`band_Sm0A` cuts j into 198 runs) fit something.
- Tune on a corpus that spans:
  - encoders: LAME versions and presets, FhG, Apple, Xing, Helix;
  - rates: 32–320 kbps, VBR;
  - modes: mono, L/R, joint, intensity;
  - MPEG-1, MPEG-2 and MPEG-2.5.
- Check the result on another corpus.
- Freeze (`!`) what codes to nearly nothing, such as the header fields
  (0.01–0.05% of the `.c`).

### 3.10 Speed and memory

**Speed now.**

- mp3c codes 1.7 MB of MP3 per second (9.7 MB in 5.7 s, one thread of a
  2.1 GHz Xeon).
- The layer of 3.3 is 8.5 times slower.
- aacpack's model, with AVX2 mixers and prefetching, codes 0.37 MB/s.
- aacpack's `-t` splits the stream into parts, with a model each, running on
  threads (MTwrap). That made it 3 times as fast on 4 cores, for +0.13% at
  1 MB parts.

**Keeping a fast mode.** mp3c's present model can stay as a fast level, with
the CM as a second level (aacpack: `levels.inc`, `-9`).

**Memory.** Tables should be mapped (`tc_map`), not allocated, as
IDX-FORMAT.md §9 explains.

## 4. Tried on mp3c, no gain

| idea | `.c`, real-world | note |
|---|---:|---|
| `av_MDCT` fading at every place of every granule, not only where a magnitude of 2 or more is coded | +0.009% to +0.067% (fading 240, 231, 192 /256) | its weights were tuned for the present rule |
| in the layer: the distances to where the count1 region and the zeros start | zero flags −1.18% without, −1.12% with | |
| in the layer: the band's magnitudes so far, and the places left in it | −1.25% without, −1.12% with | aacpack's "places left" paid there |
| in the layer: the last granule from j−2 to j+2, and two granules back | −1.21% without, −1.12% with | |
| the first channel as a context of the second in M/S | LAME set: −0.836% instead of −0.849% | `Silent_Light`: −0.13% (3.5) |

These did not pay in aacpack and likely would not here:

- predicting signs from the last frame (0.2–0.4% of the sign bits);
- coding a band's count of nonzero values first (+9%);
- whole tuples as symbols (+1.4%);
- counters seeded from the Huffman tables;
- scalefactors as a residual against a median predictor (+1.0%).

## 5. Order of work

1. **The band table (3.6).** A few lines; it changes the `.c` format only.
2. **mp3rw (3.1).** The reservoir across other data, and Layer I/II in
   containers. This changes the `.dmp` and `.out` formats.
3. **Spectral families (3.3, 3.4).** The family coder in mp3c, with mp3c's
   contexts and the rescaled references. Measure each step, as
   `aac_cm_design_results.md` does.
4. **The match model (3.2).**
5. **Side information (3.7).**
6. **Tuning and threads (3.9, 3.10).** Tune on a broad corpus, then add
   threads.

Each step changes the `.c` format, and the `.c` has no header today. A magic
and a version byte would let mp3c tell which version wrote a file. The
Layer I/II support already made this branch's `.c` files unreadable by the
original `mp3c.exe`.

## 6. What was done, and what it measured

Within 1.5 times the original model's time (README: Compression):

- **The band table (3.6)**, as proposed.
- **Mixing stages (3.3, 3.4)**, not families: for the time, a stage
  mixes mp3c's two predictions of a bit with four counters of its own, in
  one mixer of 11 inputs (`freq_cm.inc`).
- **Side information (3.7)**: stages at the scalefactors (MPEG-1 long and
  short blocks, MPEG-2), global_gain and the counts.

What the measurements on the way said, on eight files (four of the
real-world set, two LAME 3.100 encodes, and LAME re-encodes of one track at
22.05 and 48 kHz), the time on `mp3_misidentified_2`:

| | `.c` |
| --- | ---: |
| a stage on top of mp3c's own mixer, at the zero flags and \|v\|=1, 2 | −0.59%, 1.5x the time |
| the same in place of mp3c's mixer, weight sets by the stage's own context | −0.52%, 1.24x |
| in place of the mixer, weight sets by mp3c's tuned mixer contexts | −0.62%, 1.29x |
| + \|v\|=3, 4 | −0.68% |
| + the \|v\|=x loop | −0.72% |
| + the signs, with the same counters | −0.55%: worse |
| + stages at the scalefactors | −1.06% |
| + global_gain, the counts | −1.14% |
| + the stages' parameters tuned (opt.pl) | −1.27%, about 1.37x |

- In the first stage, the rescaled reference was worth 0.30 of its 0.55
  points; each of the other three contexts about 0.05.
- mp3c's mixer cost most of what the stage costs: its tuned contexts, its
  stretch and squash tables and its own accesses. Replacing it saved more
  time than the stage takes at those decisions.
- The fast counters were worth 0.13 points, an APM after the mixer 0.04
  (dropped, for the time it took), 16-bit weights cost 0.15, and
  prefetching the stage's rows and weight sets gained nothing measurable.
- At the scalefactors, the value that keeps the band's last quantizer
  step (global_gain, scalefac_scale, preflag) beat the plain change of
  global_gain as a context.

Not done: the reservoir across other data (3.1), the match model (3.2),
threads, and the tuning of the stages' context patterns (their sections
are `Const 0`, ready for opt.pl).

## Appendix: how it was measured

### The accounting

All of this was done on a copy of the tree. The copy writes byte-identical
`.c` files.

- `BProcess` adds −log2 of each decision's probability to the current part
  of the stream. The model sets the part as it goes: in each `Process*`
  function, and in `ModelMDCT` at each decision.
- What each frame took in the MP3 is counted from its record:
  - **Header:** 32 bits, 16 for a CRC, and the private bits.
  - **Side info**, by field:
    - part2_3_length: 12 bits;
    - big_values: 9;
    - global_gain: 8;
    - scalefac_compress: 4 or 9;
    - the window and block fields;
    - table_select: 10 or 15, plus 7 for the region counts;
    - preflag, scalefac_scale, count1table_select;
    - scfsi: 4 per channel;
    - main_data_begin: 9 or 8.
  - **Scalefactors:** the sum of their lengths.
  - **Values:** part2_3_length less the scalefactors, including one sign bit
    per nonzero value.
  - **Bits between granules:** `GrbLen`.
- Frames mp3rw could not parse count only their header and side info. Their
  main data is in the next record's `GrbLen`.
- The split by stereo mode and channel uses the mode of the frame being
  coded.

### The layer

All probabilities are 16-bit probabilities of a 0 bit.

- **Counters:** each with an adaptive rate 1/(n+2), n capped at 255, and
  a fast rate 1/16. Each context has a table of 2^20 counters, indexed by a
  hash of the context.
- **Mixer:** inputs are mp3c's prediction (stretched), the counters' two
  probabilities each, and a bias. mp3c's input starts with weight 1, the
  rest with 0.
  - The learning rate is 6, raised for a weight set's first updates, as
    aacpack's mixer does.
- **APM:** 33 points over the mixer's output (stretched), by decision site
  and j/4. It gets a quarter of the final probability.

A build with the layer decodes what it codes; I checked a full round trip on
four files.

### Builds and files

- g++ 13.3, `-O2`, Linux, one thread.
- mp3rw splits each file first, as in `tp.bat`.
- The LAME files are `lame -V2` and `lame -b 128` (3.100) of
  `01_-_Charity_Case` and `mp3_misidentified`, decoded by ffmpeg.
- The sample rate files (3.6) are `lame --resample 22.05 -b 64`, `--resample
  32 -b 128` and `--resample 48 -b 160` of `mp3_misidentified`.
