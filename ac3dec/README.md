# ac3dec

A standalone AC-3 (Dolby Digital, ATSC A/52) and E-AC-3 (Dolby Digital Plus, A/52 Annex E)
decoder. It reads a raw `.ac3` / `.eac3` elementary stream and writes a WAV file. Its output is
FFmpeg's (6.1) bit for bit, in float, where it is run with `-x` and FFmpeg with `-cpuflags 0`; by
default it decodes as A/52 has it where FFmpeg doesn't (below).

## Usage

```
ac3dec [options] input.ac3 output.wav
```

- `-f` writes 32-bit float samples (`WAVE_FORMAT_EXTENSIBLE`, IEEE float); the default is 16-bit
  PCM, rounded as FFmpeg's resampler rounds (`lrintf(x * 32768)`, clipped).
- `-d` doesn't apply the dynamic range control words (`dynrng`). FFmpeg applies them by default
  (`drc_scale` 1), and so does ac3dec.
- `-x` decodes as FFmpeg 6.1 does where it departs from A/52 (see below), for comparisons.
- `-q` turns off the statistics line.

`input` and `output` can be `-` for stdin and stdout. The channels are in WAV's order (L R C LFE
Ls Rs ..., FFmpeg's), with the channel mask of their positions; nothing is downmixed. The first
line printed describes the stream:

```
ss_sp3.ac3: 1501 frames, 1501 superframes, 48000 Hz, 2 channels (mask 0x3), 2305536 samples (48.03 s)
7_pt_1.eac3: 35438 frames, 17719 superframes, 48000 Hz, 8 channels (mask 0x63f), 4536064 samples (94.50 s)
```

Exit codes: 0 OK, 1 usage, 2 the input can't be read, 3 the output can't be written, 5 out of
memory.

## What it decodes

- **AC-3**, bsid 0 to 10: every channel mode (1+1 dual mono, 1/0 to 3/2) with or without the LFE,
  32/44.1/48 kHz (and bsid 9 and 10's half and quarter rates), coupling with phase flags,
  rematrixing, block switching (the 128-point transforms), dither, delta bit allocation, the
  alternate bit stream syntax of bsid 6 (Annex D: xbsi1, xbsi2).
- **E-AC-3**, bsid 11 to 16: frames of 1, 2, 3 or 6 blocks; the frame exponent strategies;
  spectral extension (SPX) with its attenuation; the adaptive hybrid transform (AHT: the vector
  quantisation of hebap 1-7, gain adaptive quantisation with its large mantissas, the 6-point
  IDCT), also of the coupling and LFE channels; dependent substreams, whose channels go where their
  channel map puts them (7.1: an independent 3/2+LFE substream and a dependent one, also behind an
  AC-3 frame as in `00007.ac3`); the mixing, informational and converter metadata, transient
  pre-noise processing and block start information (parsed; decoders may ignore them).
- **Not decoded:** enhanced coupling (`ecplinu`; no sample of it is known, and FFmpeg doesn't
  decode it either), whose blocks are concealed; independent substreams after the first
  (`substreamid` > 0), which are skipped, as FFmpeg skips them; E-AC-3's reduced sample rates
  (`fscod` 3: 24, 22.05 and 16 kHz) are decoded with the bit allocation of AC-3's half rate, which
  A/52 doesn't say is right (FFmpeg refuses them).

A frame is a sync word and a header that parses, where its CRCs hold, the next frame follows it or
the input ends with it; bytes between frames are skipped. A block that doesn't parse is concealed
as FFmpeg conceals it: the last block put out, again, to the end of its frame; so is a last frame
cut short.

## How it works

The decoder is FFmpeg's (`libavcodec/ac3dec.c`, `eac3dec.c`, `ac3.c`), in two parts:

- **The syntax walk** (`Walk`): one function per syntax structure (bsi, audfrm, audblk, the
  mantissas, the AHT data, the trailer), reading each element of A/52 into a record of the frame
  (`Frame`) and keeping what the elements imply as it goes (`Run`: the channels' bands, the
  coupling and SPX band structures, the exponents, the bit allocation pointers). It is written
  against an IO class, so that the same walk can read the bits, write them, or read and write the
  record's values as they are: [ac3tsv](../ac3tsv) and [ac3pack](../ac3pack) use it that way.
- **The arithmetic** (`Decoder`), called after each block: the mantissas dequantised in FFmpeg's
  24-bit fixed point, the dither of zero-bit mantissas from `av_lfg` seeded as FFmpeg seeds it
  (through MD5), coupling and rematrixing in fixed point, the gain to float, SPX, and the IMDCT:
  `av_tx`'s split-radix FFT and MDCT, ported, with its tables, and the KBD window
  (`av_bessel_i0`), so that each float operation is FFmpeg's C code's, in the same order.

## Where it departs from FFmpeg

`-x` does as FFmpeg does in each of these:

- **AHT's VQ table for hebap 4** (A/52 Table E4.4): FFmpeg's (`vq_hebap4` in `eac3_data.c`, in 6.1
  and in master) lacks the table's first row, so that index i decodes as row i + 1, and index 31 as
  zeros. AHT is common in Dolby's own E-AC-3 at the lower bit rates: 8 of the 22 E-AC-3 streams
  here use it (the Dolby kit's 128 and 256 kbps streams, the SPX and AHT samples of
  samples.ffmpeg.org, the dependent substream of `00007.ac3`), and all 8 have hebap-4 vectors,
  2 to 20 of them in each frame that uses AHT. Against the Nero
  E-AC-3 decoder's output that comes with the csi_miami samples, A/52's table brings the decode
  closer in every channel that uses AHT (dither aside, which differs between any two decoders):

  | Stream | Channel | ac3dec | ac3dec -x (FFmpeg) |
  |---|---|---:|---:|
  | `csi_miami_stereo_128_spx` | both | 33.50 dB | 32.18 dB |
  | `csi_miami_5.1_256_spx` | L, R, C | 27.1, 27.0, 26.9 dB | 25.1, 24.3, 26.8 dB |
  | | Ls, Rs | 26.3, 26.3 dB | 23.8, 24.0 dB |

  (SNR of the decode against Nero's, which is 256 samples later.)
- **The overlap where the channels' transforms differ**: FFmpeg's decoder starts with
  `s->downmixed` set (`ac3_downmix()` at init, and again after each dependent frame) although
  nothing is downmixed, so the first block in which one channel switches to short blocks and
  another doesn't "upmixes" the overlap (`ac3_upmix_delay()`): in 2/0 the right channel's overlap is
  replaced by the left's, in the other modes channels' are cleared or moved. That block of those
  channels comes out wrong, once in each of 11 of the streams here and 11 and 12 times in the two
  7.1 ones (there, after dependent frames).
- **E-AC-3's coupling and SPX banding structures**: where a block doesn't send one, A/52 takes the
  default for the first block of the frame that uses coupling (SPX) and the previous block's after
  it; FFmpeg resets to the default only in block 0, and otherwise keeps the last one it read, from
  whatever frame. (In `matrix2_commentary1` this makes the two read different structures; both
  read every frame without error, and decode the same.)
- **E-AC-3's SNR offsets with `snroffststr` > 0**: A/52's Table E1.4 has `snroffste` implicit in
  block 0 and sent in the blocks after; FFmpeg reads it in block 0 and in no other. No stream here
  has `snroffststr` > 0.
- **AHT in use**: a channel can use AHT if its exponents are sent once in the frame (`nchregs` 1);
  FFmpeg checks only that blocks 1-5 reuse them. The two differ only where block 0 reuses
  exponents, which A/52 doesn't allow.

## Verification

The corpus is [samples/](../samples): the eight AC-3 test files, the 14 downloaded E-AC-3 streams
(samples.ffmpeg.org, the Dolby DD+ Online Delivery Kit) and 21 streams made with FFmpeg, 43 in
all, 144 MB, 1.9 hours of audio.

- **Bit for bit:** `ac3dec -x -f` writes the same floats as `ffmpeg -cpuflags 0 -i input -f
  f32le` (FFmpeg 6.1.1, its C code) on all 43: AC-3 in every channel mode FFmpeg encodes, with
  coupling, the alternate bit stream syntax, 32 and 44.1 kHz; E-AC-3 with AHT, SPX, 1/2/3/6
  blocks, 7.1 from dependent substreams, Dolby's Atmos (JOC) and 5.1.4 streams; the junk before
  the first frame of the csi_miami files and the last frame of `casablanca_aht_mono_64` cut short.
- **Against FFmpeg's SIMD code** (a plain `ffmpeg -i input -f s16le`), the 16-bit output of `-x`
  differs by 1 in 0.001-0.013% of the samples (FFmpeg's IMDCT in SSE/AVX rounds otherwise).
- **Without `-x`**, the output differs from FFmpeg's where FFmpeg departs from A/52 (above): in 16
  of the 43 streams, by the hebap-4 vectors (8) and the overlap (13).
- **Damaged input:** 900 damaged streams (bit flips, inserted and deleted bytes, truncation, fake
  sync words, cut from 14 of the streams) under AddressSanitizer and UBSan: no crashes or reports.

Speed: about 20 MB of stream a second (300-1000 times real time) on one core of the VM.

## Build

```
g++ -O2 -ffp-contract=off -o ac3dec ac3dec.cpp
x86_64-w64-mingw32-g++ -O2 -ffp-contract=off -s -static -o ac3dec.exe ac3dec.cpp
```

One file, C++11, no dependencies but the C library. `-ffp-contract=off` keeps the compiler from
fusing multiplies and adds, which would round otherwise than FFmpeg's C code (it matters on
targets with FMA, such as `-march=x86-64-v3`).

## License

The decoder is derived from FFmpeg's libavcodec (the AC-3 and E-AC-3 decoder, its tables) and
libavutil (`av_tx`, `av_lfg`, MD5, `av_bessel_i0`), so `ac3dec.cpp` is licensed under the GNU
Lesser General Public License, version 2.1 or later, not under this repository's MIT license.
