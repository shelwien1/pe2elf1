# aac_cm_design.md: every compression idea, tested in aacpack

Each idea of `aac_cm_design.md` (the aaccm design) was built into aacpack's
model, or for an idea aacpack already had, taken out again (an ablation), and
measured. **SUCCESS** means it makes the output smaller and is now in aacpack
(or was there already and pays); **FAILURE** means it does not pay, and it is
not in aacpack.

## How it was measured

Three sets of streams, compressed with each build:

- **dev** (the decisions): 22 streams, 7.0 MB. 15 are encodes of 30-second
  passages of the two recordings that do not overlap the test excerpts, by
  fdkaac (LC, VBR, HE, HEv2, LD and ELD in LATM, mono), ffmpeg (LC 128 and
  256, PNS/IS/TNS, Main, LTP, mono) and faac (LC, mono). The other 7 are
  synthetic streams from `test/gen.sh`.
- **music** (test): the README's 25 encodes of the two one-minute
  excerpts, 21.3 MB.
- **FATE** (test): FFmpeg's 39 AAC conformance and bug-report streams, 5.0 MB.

The numbers are the change in compressed bytes, as **dev / music / FATE**;
negative means smaller. Each idea was measured against the model as it stood
just before it, so the numbers of the ideas kept add up. An idea is kept
when dev gains and the test sets do not lose. Ideas that gain only where
their feature occurs are kept when nothing else loses. Partway through, the
two faac dev streams turned out to be MP4 files, not ADTS (`faac -w`), and
so had been coded as raw bytes. They were encoded again as ADTS. The
spectral ideas, the scalefactor and section contexts and the forced-decision
skip were measured with the first dev set; the rest with the new one (the
totals below use the new one throughout).

All of it together, against aacpack before this work (stream version 5),
with the same corpora:

| set | before | after | |
|---|---:|---:|---:|
| dev | 6102163 | 6074405 | -0.455% |
| music | 18894842 | 18823379 | -0.378% |
| FATE | 2935803 | 2682960 | -8.612% |

A 9.6 MB stream with repeated material comes to 2909211 bytes instead of
8180991, because of the digest match model (§8). The stream version is 6.
On the way, a bug was found and fixed: in short windows the neighbour sum
`pn` read one place past the window, and in the last window of the last
frame it read past the channel's record. Compression then depended on the
record's layout, though both directions read the same bytes and streams
still decoded.

## §2 Structure and near-determinism

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| no tokens for cb 0/13/14/15 bands, derived counts, the first section opening, closure at max_sfb, ch1's ics_info, ms bits only for msp 1 | SUCCESS | | | | present by construction: the walk codes none of it |
| unary steps above cap(cb) not coded | SUCCESS | +1473 | +5856 | +1087 | present; the numbers are its ablation (unary to 16 everywhere) |
| decisions an n-bit field's range forces are skipped (magnitude past the room, the sign) | SUCCESS | -998 | -2007 | -2576 | new (`codes_in`) |
| alphabet narrowing: cb != 12 | FAILURE | -2 | -3 | -1 | the forced decision cost nothing after a few frames |
| alphabet narrowing: FIXFIX bs_num_env != 3, iid/icc_mode <= 5, ws = 0 for AOT 23 | FAILURE | | | | not built: those fields cost under a byte a stream, and cb != 12 showed the gain is nil |
| window_sequence: last value (state machine) | SUCCESS | | | | present (the field model's difference to the last value, last value as context) |
| window_sequence: other channel's ws, frames since change | FAILURE | -3 | -4 | -1 | |
| window_shape: ws now and before | SUCCESS | -92 | -196 | -26 | new; shape had no ws context (num_windows was 0 when it is coded) |
| common_window: last value, the ws before | FAILURE | 0 | -154 | +1 | the last value is present |
| crc / rdbpos / alignment / fill bits absent, adts cib/cis/nrdb, repeated smc/asc/SBR/PS headers, reserved bits | SUCCESS | | | | present: derived, or the difference to the last value |
| buffer_fullness by the reservoir (deferred, §5.1) | SUCCESS | +1217 | +4555 | +35 | present; numbers: ablation (the last value as base) |
| fill payload count (§5.2) | SUCCESS | -47 | -1 | -1012 | new: the count that keeps the unit's length, or the last count, by the last error |
| useSameStreamMux, bs_header_flag, enable_ps_header: frames since last, last interval | SUCCESS | -367 | -1003 | -38 | new; LATM -125..-257 bytes a stream |
| latmBufferFullness by the reservoir | SUCCESS | -61 | -216 | 0 | new; LATM -28..-77 a stream |
| predictor_reset_group_number = last mod 30 + 1 | FAILURE | +8 | +4 | -8 | |
| SBR leading border = last trailing border | SUCCESS | -94 | -438 | -21 | new |
| first SBR envelope after a header: header-present context for dt/df | SUCCESS | -26 | -87 | -8 | new, with "first envelope" and "freq_res as the reference's" |
| first regular sf = global_gain: first-band context | SUCCESS | -126 | -324 | -153 | new (in the APM); with the last frame's first delta in a counter: +281/+699/+30, not kept |
| max_sfb: history per window class | SUCCESS | -151 | -289 | -184 | new |
| max_sfb: ch1 from ch0 (no common window) | FAILURE | -4 | -3 | 0 | |
| max_sfb: short from the long cutoff | FAILURE | -7 | -5 | -23 | built: it acts once a stream (1-2 bytes); not kept |
| odd/even book as a sparsity hint | SUCCESS | | | | present: the codebook is in every spectral context |
| DSE repeats: match model | SUCCESS | | | | present (the byte model's match) |

## §1.4 Coding order (the design's rejected orders)

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| adts_buffer_fullness after the frame | SUCCESS | | | | present (see §5.1) |
| global_gain after the section data (its coded bands as context) | FAILURE | -78 | -272 | +126 | built (the walk writes a place and patches it) |
| ms_used after the first channel's section data (its codebook as context) | FAILURE | +35 | +259 | -3 | built; only after ch0's sections, since ch0's spectra read ms; without the context +364/+1060/+43 |
| spectral values before the codebooks | FAILURE | | | | not built; bound: spectra with no codebook cost +65352 dev (+1.08%), sections knowing each band's largest magnitude save at most -58188 (-0.96%). Built since, with the section data the encoders would choose as its contexts: it pays (below) |
| spectral order group, band, window, k | SUCCESS | | | | present |

## §3 Baseline

| idea | verdict | notes |
|---|---|---|
| generic compressors on aac2tsv's TSV | SUCCESS | dev streams 7043360 bytes: TSV + xz -9e 8894940, + bzip2 -9 8405470, + PPMd (7z) 7750247; aacpack 6082388. The integrated model wins by far: the generic ones do not even beat the Huffman-coded streams |
| per-token bit accounting | SUCCESS | present (`-v`, by element) |

## §4.0-4.6 Side information

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| element sequence and tags: position, previous element, last frame's | SUCCESS | | | | present (the field's slot is the previous element's) |
| element sequence: chan_config context | FAILURE | 0 | 0 | 0 | |
| scale_factor_grouping: 7 bits MSB-first, by place, last pattern's bit, bit before | SUCCESS | -128 | -274 | -17 | new; faac -0.015% |
| ms_mask_present: last value | SUCCESS | | | | present |
| ms_used: all ones so far, last frame's ms[b+1], ch1 IS last frame | FAILURE | +17 | +39 | -6 | |
| long/short band mapping for last-frame contexts | FAILURE | +1067 | +2716 | +675 | built for sections and scalefactors: worse |
| cb token binarization: zero? special? pair unary, parity | FAILURE | +57 | +699 | +166 | the 4-bit tree stays |
| cb context: band class with the previous section's cb | SUCCESS | -1593 | -5240 | -971 | new |
| cb context: ch0's cb with ms_used | SUCCESS | -1742 | -4180 | -891 | new |
| cb context: band against the last frame's max_sfb | FAILURE | +8 | +9 | +19 | |
| extend-bit binarization of section lengths | SUCCESS | | | | present (aacpack's own) |
| extend context: (cb, band) | SUCCESS | -2977 | -9655 | -1840 | new |
| extend context: ch0's section boundary | SUCCESS | -47 | -441 | -27 | new (marginal) |
| extend context: previous group's boundary | FAILURE | +38 | +23 | +10 | |
| the four section contexts together | | -5500 | -16960 | -3299 | |
| gg ch1: gg_prev(ch1) + change of ch0 | FAILURE | +752 | +2182 | -18 | ch0's gg as base (present) is better |
| gg contexts: last frame without coded bands, ws before | FAILURE | -5 | -118 | -20 | |
| sf: median(P_L, P_T, P_G) primary, coding the residual | FAILURE | +62211 | +186924 | +12580 | +1.0%; the direct delta with every predictor as context (present) is far better |
| sf context: ms_used | SUCCESS | -5001 | -15020 | -222 | new; the largest single side-info gain |
| sf context: bands skipped since the last of the kind | SUCCESS | -331 | -163 | -204 | new |
| sf, short windows: previous group's value (P_W) | SUCCESS | -110 | -230 | -10 | new |
| sf: the five kept together (and ms_used in the mixer select) | | -6147 | -17024 | -779 | |
| sf: huff_sf prior as a fixed input | FAILURE | -12 | -1 | -16 | |
| noise track: first energy against the last frame's at the band | SUCCESS | -251 | -115 | -7 | new (marginal; faac +86); as a context instead: -19/-68/0 |
| IS track: ms_used context | SUCCESS | | | | the sf ms_used context covers intensity bands too |
| TNS: ch1 from ch0's present, coef context coef_res/compress | FAILURE | -6 | -14 | -9 | |
| TNS / LTP / Main fields against the last frame | SUCCESS | | | | present (the field model) |
| payload kind: SBR state of the element | FAILURE | 0 | 0 | +1 | |
| DSE/raw bytes: order-2 context | SUCCESS | 0 | 0 | -2139 | new (hashed 2^22, mapped as touched); MPEG-TS/PS packets -0.8% |

## §4.7 Spectra

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| Z / M_j unary, sign after the magnitude, j in every context | SUCCESS | | | | present |
| A_T: last frame, k-1..k+1 energy | SUCCESS | +8986 | +19984 | +26777 | present; ablation (pf pn ex ds out) |
| A_T as a fading per-line average (mp3c) | SUCCESS | +21615 | +63576 | +31765 | present; ablation |
| A_T2 (two frames back) | FAILURE | +307 | +1747 | -2255 | music worse, FATE better; with an agreement context +202/+1156/-2811 |
| class change mapping (long/short) | FAILURE | -7 | -142 | +133 | faac -0.01% |
| A_W previous short window | SUCCESS | +554 | +2558 | +796 | present; ablation |
| A_C other channel, ms flag | SUCCESS | +1784 | +9176 | +5080 | present; ablation |
| stereo domain conversion (L/R and M/S) | SUCCESS | +4211 | +10064 | +1761 | present; ablation |
| IS bands in history | SUCCESS | -4 | -224 | -112 | new; faac -0.012% |
| PNS bands in history | FAILURE | +33 | +94 | +1 | |
| TNS / LTP / prediction flags | FAILURE | -48 | -208 | -42 | |
| pulses in history | FAILURE | | | | no stream in the corpora has pulse data |
| m1/m2 rescaled across a band boundary | SUCCESS | -1211 | -1368 | -112 | new |
| c1 m1, m2 | SUCCESS | +7914 | +9289 | +40604 | present; ablation |
| c6 band, place in band | SUCCESS | +2575 | +12456 | +3907 | present; ablation |
| c7 places left in the band | SUCCESS | -954 | -1304 | -2253 | new |
| c8 section constraint | SUCCESS | -416 | -1939 | -1758 | new |
| c11 zero run | SUCCESS | -802 | -1063 | -1660 | new |
| c7 + c8 + c11 together | | -1927 | -4016 | -5836 | |
| c12 window in group, group length | FAILURE | -117 | -55 | -43 | |
| c9 E as main context (replacing av in a counter) | FAILURE | +1504 | +6307 | +316 | |
| E in the mixer weight-set selection | SUCCESS | -148 | -2120 | -312 | new (added; replacing av there: +730/+1355/-252) |
| E (mp3c ax analog) in the final APM | SUCCESS | +1338 | +4800 | +1506 | present; ablation |
| final APM(j, m1q, cb) | FAILURE | +2921 | +12021 | +5093 | aacpack's (cb, av, m1) is better |
| S1: the codebook's own probability, given the tuple so far | SUCCESS | -967 | -5458 | -642 | new: a fixed mixer input from the code lengths, sign and escape bits |
| S2: Laplace prior scaled by E | SUCCESS | -230 | -281 | -109 | new, blended with S1 (alone instead of S1: +331/-5/-80) |
| band summary (nonzero count first) | FAILURE | +159142 | +477784 | +88902 | the counts cost 9% of the output; wide odd-book bands only: +18588/+61883/+11704 |
| signs: modelled at all | SUCCESS | +22566 | +37179 | +25840 | present; ablation (p = 1/2) |
| signs: last frame's sign with its magnitude | FAILURE | -22 | +172 | -429 | |
| escape: E as a context | SUCCESS | -166 | -196 | -473 | new |
| escape: n unary + mantissa binarization | FAILURE | +84 | +238 | -1153 | mixed |

## §6-7 SBR and PS

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| header fields against the last header | SUCCESS | | | | present |
| frame class / num_env: core ws | FAILURE | 0 | 0 | 0 | |
| borders: core's transient window | FAILURE | 0 | +1 | -1 | |
| dt/df flags (see §2) | SUCCESS | -26 | -87 | -8 | |
| envelope: running mean of this envelope | SUCCESS | -4 | -37 | -178 | new (tiny) |
| envelope: core's top-band level | SUCCESS | -3 | -10 | -142 | new (tiny); its change instead: +45/+140/+127 |
| envelope ch1: ch0's symbol (uncoupled) | FAILURE | 0 | -1 | -28 | |
| df: the last envelope's shape at the band | SUCCESS | -6 | -29 | -225 | new (tiny) |
| the three envelope contexts together | | -14 | -74 | -528 | |
| residual against a primary (dt: running mean, df: shape) | FAILURE | +576 | +2335 | +1106 | df: +474/+2434/+1263 |
| invf ch1 from ch0; harmonics: frames since set, ch0's bit | FAILURE | -1 | +1 | +3 | harmonics +1/0/-2 |
| PS frame class / num_env from SBR's | FAILURE | 0 | 0 | 0 | |
| PS ICC context IID at (e,b); OPD context IPD | FAILURE | -1 | -2 | -20 | |
| PS IPD/OPD as mod-8 differences | FAILURE | 0 | 0 | 0 | one stream has IPD/OPD, and its values never wrap |

## §8, §11 Generic machinery

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| counters: adaptive rate and a second fast one, both mixer inputs | SUCCESS | +8532 | +5157 | +13363 | present; ablation (no fast one) |
| mixer weight-set selection | SUCCESS | +8297 | +24727 | +4391 | present; ablation |
| mixer learning rate decaying to a floor | SUCCESS | -5416 | -14137 | -1823 | present, but its boost was 4x too strong: quartered (off: -4244/-13301/+972) |
| final APMs | SUCCESS | +8674 | +23744 | +6815 | present; ablation of the final blend (the first APM: +715/+1741/+1159) |
| Huffman priors: SBR/PS tables as fixed inputs | FAILURE | -5 | -11 | -30 | sf: see §4.6; spectra: S1 (a success) |
| Huffman priors: counters seeded from them | FAILURE | +1128 | +1531 | +2688 | spectral counters, plain build |
| indirect contexts | FAILURE | -22 | -36 | -89 | |
| slot match model (fields in a row equal to their last value) | SUCCESS | -150 | -582 | -389 | new |
| digest match model over (id, value) | SUCCESS | -2447 | -4753 | -232582 | new: fields -592/-2612/-4235; + sections and scalefactors -495/-1978/-40559; + spectra -1361/-166/-178151; + SBR/PS +1/+3/-9637 |
| hashed tables with check bits | FAILURE | | | | nothing to check: no model table is hashed (the largest index has 874496 rows, the fields' match context; tables hash past 2^24) |
| whole-file stored fallback | FAILURE | | | | no stream expands; random bytes grow by 21 bytes a MB (plain chunks) |
| IDX declaration of families and contexts | SUCCESS | | | | present |

## The design's other rejected alternatives

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| whole tuples as symbols | FAILURE | +87004 | +214236 | | built as a family of its own (a tree over the tuple index, contexts of the tuple's first place): +1.4% dev, +1.1% music |
| MSB-first binary for small integers | FAILURE | +74 | +317 | +630 | fields of 1-3 bits |

## After the design: the section data after the spectra

An encoder chooses its section data (codebooks and section lengths) from
its quantized values, by rules that can be run again on them. Replayed on
the streams' own values, fdk-aac's (libAACenc `dyn_bits.cpp`: each band
its cheapest codebook, neighbouring sections merged while a merge saves
bits, the best first) gives the stream's sections and codebooks in every
channel-frame of every fdkaac stream (LC, VBR, HE, HEv2, LD, ELD, mono);
faac's (each band the cheaper codebook of the smallest pair that holds its
largest magnitude, equal neighbours one section) in every band of every
faac stream; ffmpeg's trellis (`codebook_trellis_rate()`) in 99.0-99.7% of
bands (its smallest codebook for a band comes from its quantizer search,
before TNS and M/S change the values). So the section data is coded after
the spectra: where it is, each band's kind (no codebook, a spectral one,
noise, intensity), which the scalefactors need; after the spectral values,
where sections start and each coded section's codebook, with the three
encoders' choices as contexts (`cbsel.inc`). The spectra lose their
codebook (+1.1%), the section data shrinks to about its kinds (fdkaac 128
kbit/s: 16516 -> 1274 bytes). It is the default; `-b` codes the section
data before the spectra as before. The stream version is 7 (an order
byte after the level).

| idea | verdict | dev | music | FATE | notes |
|---|---|---:|---:|---:|---|
| section data after the spectra, the encoders' choices as contexts | SUCCESS | -51753 | -143204 | -16649 | dev -0.85%, music -0.76%, FATE -0.62%; per stream -0.05% (ffmpeg 256 kbit/s) to -1.4% (fdkaac LD) |
| a fourth predictor: the cheapest sectioning, and each start's bits against going on | FAILURE | +19 | -120 | -181 | 10-15% slower |
| the spectra's codebook context: the class of the band's largest value in the last frame, rescaled | FAILURE | +270 | +2593 | +2553 | |
| each band's largest magnitude's class coded before its values | FAILURE | +85806 | +251856 | +44648 | the class costs more than the spectra save |

## After the review: stream version 8

The code review (`aacpack_review.md`) found contexts that were not what
they were meant to be (B3-B6: a stale SBR grid, a mean off by one band,
codebooks and window sequences put together) and an element id that
nothing used (A5; removing it shifts the ids the match model's digest
hashes). Fixing them changes what the compressor writes, so the stream
version is 8. Against version 7, with the same corpora:

| set | version 7 | version 8 | | `-b`: version 7 | version 8 | |
|---|---:|---:|---:|---:|---:|---:|
| dev | 6022652 | 6022233 | -0.007% | 6074427 | 6071928 | -0.041% |
| music | 18680175 | 18679079 | -0.006% | 18823404 | 18816150 | -0.039% |
| FATE | 2666311 | 2665965 | -0.013% | 2682999 | 2680087 | -0.109% |

Most of `-b`'s gain is B5 (codebooks 10 and 11 shared every context of
the spectra and the signs); most of the default order's, the
scalefactors' window sequence as a short-window flag (B6).
