# The AC-3 / E-AC-3 corpus

The test streams of [ac3dec](../ac3dec), [ac3tsv](../ac3tsv) and [ac3pack](../ac3pack): the eight
AC-3 test files (`005-ac3.tar.xz`), the public E-AC-3 samples, and streams made with ffmpeg. The
streams themselves are not in the repository; these scripts fetch and make them, and
`samples.md5` has the MD5 of each.

```
./get_samples.sh corpus              # the downloads, checked against samples.md5
./make_samples.sh AC3DIR corpus      # the ffmpeg encodes (ffmpeg 6.1.1 made the ones listed)
```

## AC-3 test files

All 48 kHz stereo (2/0) AC-3, bsid 8, from DVDs: `aamss_pv`, `sd_nced` (448 kbps), `cg_mj6`,
`cg_mj8`, `cg_mj9`, `cg_mjed`, `ss_sp3`, `ss_sp5` (224 kbps); 22 MB, 19463 frames.

## Downloaded E-AC-3

From [samples.ffmpeg.org](https://samples.ffmpeg.org/A-codecs/AC3/eac3/) (demo discs and
broadcasts, 2007-2010) and the [Dolby Digital Plus Online Delivery Kit
v1.5](https://ott.dolby.com/OnDelKits/DDP/Download_15.html) (current Dolby encoder output; four
clear audio tracks of its HLS example, demuxed with `ffmpeg -c:a copy -f eac3`).

| File | Size | Frames | What it has |
|---|---:|---:|---|
| `csi_miami_stereo_128_spx.eac3` | 333664 | 651 | spectral extension (SPX), 2/0; starts with 352 bytes of a cut frame |
| `csi_miami_5.1_256_spx.eac3` | 667490 | 651 | SPX, 3/2+LFE; 866 bytes of a cut frame first |
| `rio_bravo_mono_64_spx.ac3` | 2097408 | 8193 | SPX, 1/0 (E-AC-3 despite the name) |
| `casablanca_aht_mono_64.eac3` | 6013664 | 23491 | adaptive hybrid transform (AHT), 1/0; the last frame cut short |
| `buggy_aht_5.1_384.eac3` | 7014912 | 4567 | AHT, 3/2+LFE |
| `matrix2_commentary1_stereo_192.eac3` | 2191872 | 2854 | coupling, rematrixing, 2/0 |
| `matrix2_english_5.1_640.eac3` | 7306240 | 5708 | 3 blocks a frame, 3/2+LFE |
| `serenity_english_5.1_1536.eac3` | 13108224 | 12801 | 1 block a frame, 1536 kbps, 3/2+LFE |
| `7_pt_1.eac3` | 19632652 | 35438 | 7.1: an independent 3/2+LFE substream and a dependent one, 1 block a frame |
| `00007.ac3` | 1464320 | 440 | 7.1: AC-3 (bsid 6) 3/2+LFE and an E-AC-3 dependent substream |
| `dolby_Holi_en_2ch_128kbps_ddp.eac3` | 1362432 | 2661 | Dolby 2/0 |
| `dolby_Holi_en_6ch_256kbps_ddp.eac3` | 2724864 | 2661 | Dolby 3/2+LFE |
| `dolby_Holi_en_514ch_448kbps_ddp.eac3` | 4766720 | 2660 | Dolby 5.1.4 (3/2+LFE, the height in its metadata) |
| `dolby_Holi_en_6ch_640kbps_ddp_joc.eac3` | 6809600 | 2660 | Dolby Atmos (JOC) over 3/2+LFE |

## Made with ffmpeg

20 s of `ss_sp3.ac3` (stereo) and of `matrix2_english_5.1_640.eac3` (5.1), decoded and encoded
again (`make_samples.sh`). ffmpeg's E-AC-3 is AC-3's coding tools in E-AC-3's syntax (bsid 16, one
independent substream, no AHT, SPX or block switching, the 5-bit frame exponent strategies), with
1, 2, 3 or 6 blocks a frame by the bit rate.

| File | What it has |
|---|---|
| `ff_eac3_st_96k`, `ff_eac3_st_1536k` | 2/0, 6 and 3 blocks a frame |
| `ff_eac3_mono_64k` | 1/0 |
| `ff_eac3_441_st_192k`, `ff_eac3_32_st_128k` | 44.1 and 32 kHz |
| `ff_eac3_51_448k`, `ff_eac3_51_3072k`, `ff_eac3_51_6144k` | 3/2+LFE, 6, 2 and 1 blocks a frame |
| `ff_ac3_mono_96k` | AC-3 1/0 |
| `ff_ac3_441_st_192k`, `ff_ac3_32_st_96k` | 44.1 kHz (frames of 557 and 558 words by turns) and 32 kHz |
| `ff_ac3_st_cpl_96k` | coupling forced |
| `ff_ac3_st_xbsi_256k` | the alternate bit stream syntax (bsid 6, xbsi1 and xbsi2) |
| `ff_ac3_21_192k`, `ff_ac3_30_256k`, `ff_ac3_21b_256k`, `ff_ac3_31_320k`, `ff_ac3_22_320k`, `ff_ac3_50_384k` | 2/0+LFE, 3/0, 2/1, 3/1, 2/2, 3/2 |
| `ff_ac3_51_448k`, `ff_ac3_51_640k` | 3/2+LFE; the second with dialnorm, room type and mixing level set |

Dual mono (1+1) AC-3 is the one channel mode ffmpeg doesn't write; nothing here has it.
