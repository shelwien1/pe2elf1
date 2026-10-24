# aac2tsv — lossless AAC ⇄ TSV

`aac2tsv` turns an AAC bitstream into a text file with one syntax structure
per line (tab separated values) and turns that text back into the identical
bitstream. The Huffman layer is removed: quantized spectral values, section
codebooks, scalefactors, TNS/pulse/gain-control data, SBR envelopes and PS
parameters appear as numbers, while everything the decoder can recompute
(frame and element lengths, CRCs, escape and sign bits) is left out.

```
make                                   # c++ -O2 -Wall -o aac2tsv aac2tsv.cpp -lm
./aac2tsv c [options] input.aac output.tsv
./aac2tsv d [options] input.tsv output.aac
```

`-` as input or output means stdin/stdout. One translation unit, C++11 or
later, no dependencies (`x86_64-w64-mingw32-g++ -O2 -static -o aac2tsv.exe
aac2tsv.cpp` for Windows).

| option | |
|---|---|
| `-adts` `-latm` `-adif` | c: only look for this container (default: autodetect) |
| `-asc HEX` | c: the input is back-to-back raw access units with this AudioSpecificConfig (as `aacdec -asc`) |
| `-noverify` | c: skip the per-frame check (about 2.5× faster) |
| `-q` / `-v` / `-vv` | quiet / statistics on stderr / also report SBR payloads kept as hex |

## Lossless by construction

The same code walks the AAC syntax in both directions: in `c` it reads bits
and emits records, in `d` it consumes records and writes bits. In `c` every
frame is also converted back right away (text → records → bits) and compared
with the input, together with the syntax state carried to the next frame;
if anything differs the frame is written as hex instead. Data that is not an
AAC frame (ID3/APE tags, junk, truncated frames) is kept as hex, and a frame
whose syntax breaks somewhere (corrupt stream) keeps the parsed elements and
the rest of the frame as hex. So `d(c(x)) == x` for any input, AAC or not;
the last record holds the size and CRC-32 of the original, which `d` checks.

`d` accepts edited files: lengths, counts, CRCs and block positions are
recomputed, values out of range for their field or codebook are reported with
the line number.

## Coverage

| | |
|---|---|
| containers | ADTS (any number of raw_data_blocks, CRC), LOAS/LATM (AudioSyncStream; one program, one layer, frameLengthType 0, any number of subframes, audioMuxVersion 0 and 1), ADIF, raw access units (`-asc`), AUS1 (ASC + length-prefixed AUs, written by `gen_remux.py`) |
| syntax elements | SCE, CPE, CCE, LFE, DSE, PCE, FIL, END |
| object types | AAC Main (prediction), LC, SSR (gain control), LTP; HE-AAC (SBR, with or without SBR CRC) and HE-AACv2 (PS incl. IPD/OPD); 1024 and 960 sample frames |
| fill payloads | SBR, DRC (`dynamic_range_info`), padding (`EXT_FILL_DATA` A5 bytes, zeros), anything else as hex |
| kept as hex | ER AAC (LC/LD/ELD), USAC and other object types (their access units inside LATM), MP4 and other containers |

The syntax follows the aacdec decoder (FFmpeg 8.1.3 AAC decoder in C++), its
tables are used for the codebooks, scalefactor bands and the SBR frequency
band tables that the SBR syntax depends on.

## TSV format

The first line is `aac2tsv	1` (format version), the last one
`eof	<size>	<crc32>`. A line is a record name followed by tab separated
fields. Fields are decimal integers, hex strings (bytes, or nibbles where noted),
binary strings for bit vectors (`0110`, first bit first), and `-` for an empty
string. Lines starting with `#` are comments. Records appear in bitstream order;
`[...]` marks fields or records that are present only when the syntax has them,
"optional trailing" fields are written only when they are not zero/default.

Values are the syntax element values as coded, with variable length coding
removed: scalefactors, intensity positions and noise energies are the coded
differences (the first noise band its 9-bit value), SBR and PS parameters
the coded deltas (or start values), spectral data the quantized values with
signs and escapes applied.

### File and containers

| record | fields |
|---|---|
| `raw` | hex bytes that are not (parsable) AAC, 32 per line |
| `adtsh` | ADTS fixed header, written when it changes: `id layer protection_absent profile sampling_frequency_index private_bit channel_configuration original_copy home` |
| `adts` | one ADTS frame: `adts_buffer_fullness number_of_raw_data_blocks_in_frame [copyright_identification_bit copyright_identification_start]`; then the raw_data_block(s) |
| `rdbpos` | `raw_data_block_position[1..n]`, only if not the actual block offsets |
| `crc` | `crc_check`, only if it is not the CRC of the frame (regions as in fdk-aac: header, then per element 192 bits from the tag, 128 more bits from the second channel of a CPE, DSE/PCE whole; checked with the fdk-aac decoder); after `adts` for the header CRC, after `end` for the per-block CRCs |
| `tail` | hex bytes after the last raw_data_block of a frame (frame length is derived) |
| `rest` | `nbits hex` — the rest of a frame/payload from a syntax element that does not parse |
| `loas` | one AudioSyncStream frame: `useSameStreamMux [byte_align bits]` (audioMuxLengthBytes derived) |
| `smc` | StreamMuxConfig: `audioMuxVersion [audioMuxVersionA] [taraBufferFullness: bytes value] allStreamsSameTimeFraming numSubFrames numProgram numLayer` |
| `asc` | AudioSpecificConfig: `nbits hex [bytesForValue]` (version 1: nbits is ascLen, fill bits included) |
| `smcx` | `frameLengthType latmBufferFullness otherDataPresent [otherDataLenBits: bytes value] crcCheckPresent [crcCheckSum]` |
| `pay` | one PayloadMux(), a raw_data_block follows (MuxSlotLengthBytes derived) |
| `payraw` | hex payload of an object type that is not parsed |
| `paypad` | `nbits hex` — payload bits after END, if not the zero bits up to the next byte |
| `other` | hex otherDataBits |
| `adif` | `copyright_id_present [copyright_id: 18 nibbles] original_copy home bitstream_type bitrate num_program_config_elements`; then `adifpce` records, then the raw_data_blocks |
| `adifpce` | `[adif_buffer_fullness]` + the fields of `pce` |
| `rawasc` | AudioSpecificConfig given with `-asc` (hex); then the raw_data_blocks |
| `aus1` `au` `autail` | AUS1 header (ASC hex), one access unit, hex bytes after its raw_data_block |

### raw_data_block

| record | fields |
|---|---|
| `sce`, `lfe` | `element_instance_tag`; then an individual channel stream |
| `cpe` | `element_instance_tag common_window [ics_info]`; then `[ms]` and two individual channel streams |
| `ms` | `ms_mask_present [ms_used bits]` |
| `cce` | `element_instance_tag ind_sw_cce_flag num_coupled_elements (cc_target_is_cpe cc_target_tag_select [cc_l·2+cc_r])* cc_domain gain_element_sign gain_element_scale`; then an individual channel stream and a `cgain` per further gain element list |
| `cgain` | `[common_gain_element_present] gain values` (one common value or one per non-zero band) |
| `dse` | `element_instance_tag data_byte_align_flag hex [alignment bits]` |
| `pce` | `element_instance_tag object_type sampling_frequency_index num_front num_side num_back num_lfe num_assoc_data num_valid_cc mono_mixdown_present [tag] stereo_mixdown_present [tag] matrix_mixdown_idx_present [idx pseudo_surround] (is_cpe tag_select)* (lfe/assoc tag_select)* (cc_is_ind_sw tag_select)* comment_hex [alignment bits]` |
| `fil` | fill element, its extension payloads follow (count derived); optional `1` if 14 bytes were written with the escape count |
| `end` | `[byte_alignment bits]` |

### Individual channel stream

| record | fields |
|---|---|
| `ics` | `global_gain [ics_info]` — ics_info (not for a CPE with common window, where it is in `cpe`): `ics_reserved_bit window_sequence window_shape max_sfb` then `scale_factor_grouping` (short) or `predictor_data_present` + Main: `predictor_reset [group] prediction_used bits` / other: `ltp_data_present [ltp_lag ltp_coef ltp_long_used bits]` (twice in a common window CPE) |
| `sec` | one per window group: `(sect_cb sect_len)*` |
| `sf` | one per window group that has coded bands: one value per band with a non-zero codebook |
| `pulse` | `number_pulse pulse_start_sfb (pulse_offset pulse_amp)*` |
| `tns` | one per window: `n_filt [coef_res (length order [direction coef_compress coef*])*]` |
| `gc` | `max_band (adjust_num (alevcode aloccode)*)*` per band and window |
| `spec` | one per window: the quantized values of the window up to `swb_offset[max_sfb]`, 0 in bands without spectral data (codebooks 0, 13–15); left out when no band has spectral data |

### Fill element payloads

| record | fields |
|---|---|
| `fill` | `n [type byte]` — n bytes of padding: extension type, fill nibble 0, n−1 equal bytes (default EXT_FILL_DATA with 0xA5) |
| `ext` | `extension_type hex` — any other payload, its 8n−4 bits as 2n−1 nibbles |
| `drc` | `pce_tag_present [pce_instance_tag reserved] excluded_chns_present [(exclude_mask additional_excluded_chns)*] drc_bands_present [band_incr interpolation_scheme band_top*] prog_ref_level_present [prog_ref_level reserved] (dyn_rng_sgn dyn_rng_ctl)*` |
| `sbr` / `sbrc` | SBR payload (EXT_SBR_DATA / EXT_SBR_DATA_CRC); `sbrc` has the CRC only if it is not the CRC-10 of the payload |
| `sbrh` | `[header]` `bs_amp_res start_freq stop_freq xover_band reserved header_extra_1 header_extra_2 [freq_scale alter_scale noise_bands] [limiter_bands limiter_gains interpol_freq smoothing_mode]` |
| `sbrd` | `bs_data_extra [reserved] [bs_coupling]` (coupling for a CPE); then per channel and in bitstream order: |
| `sgrid` | `frame_class` + FIXFIX: `log2(num_env) freq_res` / FIXVAR: `abs_bord num_rel rel_bord* pointer freq_res bits (last envelope first)` / VARFIX: `abs_bord num_rel rel_bord* pointer freq_res bits` / VARVAR: `abs_bord_0 abs_bord_1 num_rel_0 num_rel_1 rel_bord* pointer freq_res bits` |
| `sdtdf` | `df_env bits, df_noise bits` (1 = time differential) |
| `sinvf` | `invf_mode` per noise band |
| `senv` | one per envelope: start value and frequency deltas, or time deltas |
| `snoise` | one per noise floor: start value and deltas, or time deltas |
| `sharm` | `add_harmonic_flag [add_harmonic bits]` |
| `sbrx` | extended data, `[fill bits]`; `ps` records or `sbrxe id nbits hex` follow |
| `sbrpad` | `nbits hex` — bits after the SBR data, if not the zero bits up to the byte boundary |
| `sbrinit` | at the start of the file: `element_type (0 SCE, 1 CPE, 2 CCE) element_instance_tag bs_amp_res start_freq stop_freq xover_band freq_scale alter_scale noise_bands` — SBR header assumed for the payloads of that element before its first header (see below) |
| `ps` | `enable_ps_header [enable_iid [iid_mode] enable_icc [icc_mode] enable_ext] frame_class num_env_idx [border_position*]` |
| `psiid` `psicc` | one per envelope: `dt values` |
| `psx` | PS extension, `[fill bits]`; `psipd enable_ipdopd reserved_ps` (+ `pspd ipd_dt ipd* opd_dt opd*` per envelope) or `psxe id` follow |

The SBR payload syntax depends on the last SBR header of its element, so a
payload that comes before the first header cannot be parsed (decoders skip
it). When a stream starts like that (cut streams, some conformance streams),
`c` makes a quick parse-only pass, takes the first header of such an element
and states it in a `sbrinit` record; the earlier payloads are then parsed
with it, and any that still do not fit stay `ext` records.

### Example

```
adtsh	0	0	1	1	3	0	2	0	0
adts	2047	0
cpe	0	1	0	2	0	12	27
ms	1	111000100000110111010000110101111110101110111110
ics	118
sec	6	3	2	6	0	3
sec	11	9	0	3
sf	0	2	-3	4	-7	2	0	-1	-5
sf	8	-12	0	0	-12	2	8	-10	7
tns	1	1	9	7	0	1	5	6	0	7	1	0	1
spec	1	2	1	-1	0	1	1	0	1	2	1	-2	0	0	0	1 ...
...
fil
sbr
sbrh	1	10	9	0	0	0	0
sbrd	0
sgrid	0	0	1
...
end
```

## Testing

`make test FILES="..."` converts each file to TSV and back and compares.
Checked bit-exact, with every frame fully parsed: the 15 test streams
(LC, HE, HEv2, LFE), the streams `make_synth.py` generates (22.2 and other
channel configurations, PCE, multi-block ADTS, DSE, DRC, SBR CRC), FFmpeg
encodes (Main with prediction, PNS, intensity stereo, 5.1/7.1 with PCE, 8 to
96 kHz, MPEG-2 ADTS, ID3v2/APE tags), ISO conformance streams remuxed from
FFmpeg's FATE suite to ADTS and LATM (pulse data, coupling channels, AAC Main
and LTP prediction, 960-sample frames, SBR/PS), LATM, ADIF and raw AUs.
Pulse, LTP, SSR gain control and CCE added to real streams decode with aacdec
without errors. The derived ADTS CRCs (single and multi-block frames, block
positions) and SBR CRCs are accepted by the fdk-aac decoder, which rejects
wrong ones. The SBR band tables are computed in double; for every possible
header and sample rate they equal the decoder's float tables, also under
`-ffast-math`. Random data, truncated, spliced and bit-flipped inputs (5600
cases, 800 of them under ASan/UBSan) all round-trip without a single frame
needing the fallback; 1500 mutated TSV files give errors, never crashes.

## License

The syntax tables (`tables.inc`), the SBR band table computation and the
bitstream syntax are taken from aacdec, which is derived from FFmpeg; like
them, aac2tsv is licensed under the GNU Lesser General Public License v2.1
or later.
