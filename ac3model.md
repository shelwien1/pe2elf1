# Lossless compression of AC-3 and E-AC-3

What in an AC-3 or E-AC-3 stream a lossless compressor can leave out because it can be computed,
what it can predict, and from which contexts: measured on the 43 streams of [samples/](samples)
with `ac3stat` ([ac3tsv/ac3stat.cpp](ac3tsv/ac3stat.cpp), which walks the frames with the syntax of
[ac3dec](ac3dec) and [ac3tsv](ac3tsv)). [ac3pack](ac3pack) is the compressor built on it.

AC-3 has no entropy coding at all: every element is a fixed-length field, the mantissas uniform
codes (with three or two of the small ones packed into one code), the exponents base-5 groups of
three deltas. All of its compression is in the quantisation: the exponents are a 6 dB envelope of
the spectrum, and an integer psychoacoustic model, run identically by the encoder and the
decoder on that envelope, gives each coefficient its number of bits (the bap). So a lossless
recompressor has two things to work with: the redundancy the fixed-length codes leave, and the
context the bit allocation hands it for free.

## Where the bits go

By `ac3stat`, in four groups of the corpus (shares of all the bits of the frames):

| | AC-3 test files (DVD, 2/0) | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby |
|---|---:|---:|---:|---:|
| frames | 19463 | 7867 | 9742 | 105435 |
| sync word, CRCs | 0.53% | 0.54% | 0.14% | 0.58% |
| header (bsi, audfrm) | 0.42% | 0.41% | 0.34% | 2.00% |
| block side information | 1.22% | 1.34% | 0.14% | 2.72% |
| coupling and SPX: strategy, coordinates | 0.00% | 1.18% | 0.10% | 1.04% |
| bit allocation parameters, SNR offsets, delta | 0.53% | 0.71% | 0.01% | 0.47% |
| exponents | 11.36% | 19.38% | 9.36% | 27.83% |
| mantissas (AHT's included) | 84.02% | 75.42% | 67.80% | 64.45% |
| skip fields (in the side information) | 0.21% | 0 | 0 | 2.09% |
| after the last block (unused bits, auxiliary data) | 1.93% | 1.02% | 22.11% | 0.92% |

("E-AC-3 by Dolby": the samples.ffmpeg.org streams, 2007-2010 encodes, and the Dolby DD+ kit's.)
The mantissas are most of every stream; then the exponents, much more of them in Dolby's E-AC-3
(D15 strategies, sent often) than in the DVD AC-3. FFmpeg's E-AC-3 at 1536-6144 kbps leaves a
fifth of its frames unused.

## What can be computed

These cost nothing where they hold, which in the corpus is everywhere:

- **The sync word** (16 bits a frame).
- **The CRCs.** AC-3's `crc1` is the word that makes the CRC of the first 5/8 of the frame zero
  (CRC-16, x^16 + x^15 + x^2 + 1, from bit 16; the word sits at the start of what it covers, so it
  is the CRC of the rest times x^-(n + 16) modulo the polynomial), `crc2` the CRC of the last
  3/8; E-AC-3's one CRC covers the whole frame. They hold in 100% of the frames of all 43 streams:
  a flag each, and the word only where it doesn't hold. (`crcrsv`, the bit before `crc2`, which an
  encoder may flip so that `crc2` isn't the sync word, is a field and stays.)
- **The frame's size and where its parts are**: the frame size follows from `frmsizecod` and
  `fscod` (AC-3) or `frmsiz` (E-AC-3); the number of exponent groups, the bins of each channel, the
  coupling and SPX bands from the strategies; the bit allocation pointers from the exponents and
  the parameters. None of this is in the bits, and a parser has it anyway: the bap is the
  context every mantissa model starts from.
- **The elements that are implied**: E-AC-3's defaults where a syntax flag is off (`blksw` 0,
  `dithflag` 1, the bit allocation parameters, fast gains 4), the coordinates' exist flags of a
  first block, the exponent strategies of each block from E-AC-3's 5-bit frame strategies, the
  rematrixing band count from the coupling or SPX start. A parser that writes the bits back
  regenerates them.

## What is nearly free

Things an adaptive model predicts at a small fraction of their bits:

- **The header.** The bsi of a frame is the bsi of the substream's frame before in 99.5% of the
  DVD AC-3 frames and 99.2% of FFmpeg's. In E-AC-3 75% of the frames repeat the one before, and
  what changes is regular: `convsync` alternates in streams of 2 or 3 blocks a frame (frames sets of
  6 blocks), `frmsiz` alternates between two sizes at 44.1 kHz, as AC-3's `frmsizecod` does,
  `compr` changes in 1-5% of Dolby's frames. Each field coded against its own last value is
  nearly free. The substreams of a stream (dependent ones for 7.1, independent ones with
  `substreamid` > 0) each need their own history: 7.1 streams alternate the two.
- **The block side information**: the block switch and dither flags, `dynrng` (often absent or
  repeated), the strategies (coupling, rematrixing, exponents, bandwidth), the bit allocation
  parameters and SNR offsets (constant for long runs). Exponent strategies take 0.68-1.06 bits
  each given the block before's, against 2 in the bits.
- **The bits after the last block**: all zero in every DVD and FFmpeg frame (178 and 91 bits a
  frame on average for AC-3; 5300 for FFmpeg's E-AC-3, most of it at 1536-6144 kbps). Dolby's E-AC-3 is zero there too
  but for a 16-bit word just before `auxdatae` in every frame (not a CRC of any prefix of the
  frame, as far as a search tried; varying from frame to frame), which has to be kept. None of the
  frames has auxiliary data (`auxdatae` 0).
- **The fill of the last mantissa groups**: where a block's last group of three (bap 1, 2) or two
  (bap 4) mantissas isn't full, the encoder fills it: with code 0 in the DVD and FFmpeg streams,
  with the zero level (codes 1, 2, 5) in Dolby's. A constant per encoder.
- **Skip fields**: in the DVD AC-3 (two of the eight streams) they are padding, 1-6 bytes in most
  frames, nearly all in block 5, their content varying (0.2% of the bits); in Dolby's Atmos (JOC)
  and 5.1.4 streams they carry EMDF payloads (sync word 0x5838: the object metadata), 2% of the
  bits. Both are bytes to code with order-1 and 2 contexts and the last frame's field at the same
  place.

## Exponents

A channel's exponents in a block are an absolute value (4 bits) and deltas of -2..+2 in groups of
three (base 5, 7 bits for 125 of the 128 codes): 2.33 bits a delta in the bits. As deltas their
distribution is far from flat, and the last block's exponents at the same place are a good
prediction (bits a delta, conditional entropies over each group):

| | AC-3 test files | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby |
|---|---:|---:|---:|---:|
| in the bits | 2.333 | 2.333 | 2.333 | 2.333 |
| H0 | 1.913 | 2.160 | 2.300 | 1.929 |
| given the delta before | 1.808 | 2.062 | 2.233 | 1.708 |
| given the delta before and the last block's at the place | 1.771 | 1.947 | 2.177 | 1.560 |

That is 24% (DVD AC-3) to 33% (Dolby) of the exponents' bits saved with two contexts, 3% and 9%
of the streams. The last block's deltas are only comparable where its strategy was the same (else
the group sizes differ): an exponent model does better to predict the exponent at the bin from
the last exponents sent there (in the same or the previous frame) and code the delta to it,
with the strategy, the bin's band, the other channels' exponents at the bin (channel 1's predicts
the others' about as well as the block before does: 2.12 against 2.18 bits a bin in the DVD
streams) and the previous delta as further contexts. The base-5 grouping is the bitstream's; a
model codes the three deltas of a group one at a time.

## Mantissas

The mantissas are levels of uniform quantisers: bap 1-5 are symmetric quantisers of 3, 5, 7, 11
and 15 levels (three bap-1 values in 5 bits, three bap-2 in 7, two bap-4 in 7, bap 3 and 5 in 3
and 4 bits), bap 6-15 two's complement values of 5 to 16 bits. As levels (`q` of ac3tsv) their
distributions given the bap are close to flat. The share of the bits that an ideal coder of each
context would spend (over each group of streams; model cost added in parentheses):

| bap | AC-3 test files: share of mantissa bits | H0 | given the bin before | given the last block | given both |
|---|---:|---:|---:|---:|---:|
| 1 (3 levels) | 8.8% | 88.7% | 88.3% | 88.6% | 88.2% |
| 3 (7 levels) | 11.8% | 85.4% | 85.0% | 85.2% | 84.7% |
| 6 (5 bits) | 17.4% | 96.1% | 95.5% | 95.4% | 94.7% (+0.1%) |
| 8 (7 bits) | 11.5% | 96.2% | 95.4% | 95.3% | 94.8% (+0.3%) |
| all | | 93.4% | 92.9% | 92.8% | 92.4% (+0.4%) |

| all mantissas | AC-3 test files | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby (but AHT) |
|---|---:|---:|---:|---:|
| H0 | 93.4% | 91.8% | 73.7% | 94.5% |
| given the magnitude of the bin before (classes) | 92.9% | 91.2% | 72.6% | 93.4% |
| given the last block's coefficient at the bin, in this bin's steps | 92.8% | 90.4% | 71.8% | 93.3% |
| given both | 92.4% | 89.6% | 71.0% | 92.4% |

- **What they are**: at the usual bit rates the bit allocation does its job: a mantissa's level is
  nearly uniform over its range, a little more often small than large (the zero level is 52% of the
  bap-1 values, 30% of bap 3's, 8% of bap 6's). The small baps (1-5) give most of the gain: the
  fixed packing of 3, 5 and 11 levels costs nothing, but the levels aren't equally likely.
- **The signs**: one bit each, in all four groups, also given the sign at the bin in the block
  before (1.0000 bits). An MDCT coefficient's sign changes with the phase of the signal from block
  to block; a model codes them as they are.
- **High bit rates**: FFmpeg's E-AC-3 at 1536-6144 kbps gives most bins bap 15 (16 bits) with
  values far below the range (74% of their bits at H0): its mantissas lose a quarter, and the
  unused fifth of its frames all of it.
- **AHT** (E-AC-3): the vector indices (hebap 1-7, 1.5% of Dolby's bits) are nearly uniform (98.8%
  at H0); the gain adaptive quantiser's mantissas (11% of the bits) come to 94.5% given the hebap
  and the gain, with 9.6% of them escapes to a large mantissa.

What didn't help, measured: whether a bin's exponent is "tight" (the coefficient's own, so that
the mantissa's magnitude would be at least 1/2: a D15 exponent not raised to meet the ±2 rule of
the deltas, not shared in a D25/D45 group, in the block that sent it). The DVD encoders'
exponents aren't: for bap 3 and up, where all of that holds, 31-38% of the mantissas are at least
1/2, against 29-33% anywhere. The contexts of the tightness, with the bin before and the last block's, come to
89.4% of the DVD streams' mantissa bits, but 4.5% go to learning the larger table: no better than
92.4% with the two alone.

So the mantissas' gain is in the details of the magnitudes: the neighbours in frequency (the bins
before, and the exponent shape around the bin), the same bin in the last block (its coefficient
rescaled to this bin's quantiser step), the other channels at the bin (the coupling channel's
mantissas against the coupled ones; L and R where they aren't rematrixed), the bap, the band, and
the position in the block's mantissas; mixed, as context mixing does, rather than one table of
their product.

## Coupling, SPX, rematrixing

The coordinates of coupling (a 4-bit exponent and mantissa per band and channel, a master value
per channel) and SPX (4 + 2 bits) are sent when they change; given the same band's last values
they are nearly always the same or a step away. The phase flags and rematrixing flags (2/0) are
flags with long runs. 1% of the bits together.

## Estimate

With the contexts above and nothing else (each a table, no mixing), the shares of the stream
saved:

| | AC-3 test files | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby |
|---|---:|---:|---:|---:|
| mantissas (given both) | 6.1% | 7.4% | 18.5% | 3.5% + 0.6% (AHT) |
| exponents (two contexts) | 2.7% | 3.2% | 0.6% | 9.2% |
| unused bits after the blocks | 1.9% | 1.0% | 22.1% | 0.6% |
| sync, CRCs, header, side information (most of it) | ~2% | ~3% | ~0.5% | ~4% |
| the size left | ~87% | ~85% | ~58% | ~82% |

A context-mixing coder (ac3pack's: several contexts per decision, mixed, then refined) does
somewhat better than these single tables; what it can't change is the bulk of the mantissas, which
the bit allocation has already made nearly uniform. For comparison, `xz -9e` leaves 96.7% of the
DVD streams, 96.5% of FFmpeg's AC-3, 65.9% of FFmpeg's E-AC-3 and 87.0% of Dolby's (the zero
padding, the repeated headers and side information).

## Further: the decoder in the loop

m4vpack's largest gains came from decoding the pictures in the model. AC-3's coefficients are its
picture already, but two things are only visible after the arithmetic:

- **Rematrixed channels**: above the coupling start the coupled channels send no mantissas (the
  coupling channel's and the coordinates make them), so cross-channel contexts are for the bins
  below it; there, in 2/0 with rematrixing, a band's two channels are the sum and the difference,
  whose magnitudes predict each other less well than L's and R's, so such a context is better
  made of the values as the decoder makes them (L = M + S, R = M - S) than of the mantissas.
- **Tonal components across blocks**: the MDCT of a stationary sinusoid changes from block to block
  by its phase advance (with the TDAC aliasing terms), so two blocks' coefficients at a bin and its
  neighbours predict the third's, sign included, where a component is tonal; the measured sign
  entropy given one block is 1.000 bits, so this would need two blocks and the frequency, as an
  audio codec's long-term predictor does. Not tried.
