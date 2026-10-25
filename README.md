# aacpack

A lossless compressor of AAC streams (ADTS, LOAS/LATM, ADIF, raw access
units). It takes the Huffman coding out of the AAC and codes the values
again with a context mixing model; `d` puts the Huffman coding back, bit
for bit. MP4 (M4A) files are mp4p's: aacpack keeps them as bytes.

    aacpack c [options] input output     # compress
    aacpack d [options] input output     # decompress

It is built from two earlier programs: the syntax walk of
[aac2tsv](https://github.com/shelwien1/pe2elf1/tree/claude/magical-davinci-64pswj/aac2tsv)
(lossless AAC ⇄ TSV) and the coder, models and streaming of
[h264p](https://github.com/shelwien1/pe2elf1/tree/claude/happy-keller-i8oles)
(a lossless compressor of H.264 streams).

## Build

    make                        # -> ./aacpack
    make CXXFLAGS="-O0 -g"      # debug build
    make PARCH=                 # plain C++, no AVX2: slower, codes identically

aacpack is one translation unit: `aacpack_src/aacpack.cpp` is its command
line and includes `aacpack_src/aacpack.inc`, which includes the rest. So
`g++ -O2 -march=x86-64-v3 -std=c++11 -fwrapv -fno-exceptions -fno-rtti -Iaacpack_src/Lib3 -pthread aacpack_src/aacpack.cpp -o aacpack -lm`
builds it too. As in h264p, it uses neither C++ exceptions nor RTTI.
Errors jump to traps the callers set (`err.inc`). It uses nothing of the
C++ library either (`mem.inc` has the few containers it needs), and its
structs are set up by `Init()` and freed by `Quit()`, not constructors and
destructors. The target is x86-64-v3 (AVX2); the mixers' SIMD code has
plain C++ beside it that codes identically, and GCC and clang builds code
the same bytes too.

## Use

| option | |
|---|---|
| `-v` | statistics on stderr: what each part of the stream took before and after, by stage and by syntax element; what the stream is and where it changes; the units it stored as bytes, and why |
| `-p` | what the stream is and the progress line, even when stderr is not a terminal |
| `-s` | `c`: fail where a frame would be stored as its bytes or kept partly as bits, instead of storing it so |
| `-9` | `c`: the model's level (there is one, `-9`; `d` reads it from the stream) |
| `-adts` `-latm` `-adif` | `c`: look for this container alone (default: whatever comes) |
| `-asc HEX` | `c`: the input is back-to-back raw_data_blocks of this AudioSpecificConfig, as `aac2tsv -asc` and `aacdec -asc` take them (not ER object types, whose access units have no END to tell where one ends) |

Input and output can be `-` (stdin, stdout), and not the same file. A
file is written under a temporary name beside it (`name.PID.tmp`) and
renamed when the call is done, so a call that fails leaves what was
there. The exception is `d`, which keeps what it restored before an
error in the compressed stream. An interrupted call (SIGINT, SIGTERM,
SIGHUP, SIGPIPE) removes the temporary file. On a terminal (or with `-p`),
`c` and `d` say what the audio is and show their progress:

    audio: ADTS, AAC LC + SBR (HE-AAC), 22050 Hz, channel configuration 2
    [enc] 9612410 > 8782155  100.0%  25860 frames  done in 19.07s  10.68M RAM at most

Exit status: 0 done, 1 an error (said on stderr), 2 a bad command line.

## What it compresses

`c` takes any input. What is AAC is parsed and coded by the model; the
rest is coded as bytes (ID3 and APE tags, junk, frames cut short or
broken), so `d(c(x)) == x` for every `x`. A CRC-32 of the input,
at the end of the compressed stream, is checked by `d`.

| | |
|---|---|
| containers | ADTS (CRC, several raw_data_blocks a frame), LOAS/LATM (AudioSyncStream: audioMuxVersion 0 and 1, one program and layer, frameLengthType 0, any number of subframes, other data, CRC), ADIF, AUS1 (an AudioSpecificConfig and length-prefixed access units, aac2tsv's format; `test/mp42aus1.py` writes one from an MP4 file's AAC track), raw access units (`-asc`); ADTS, LATM and other bytes one after the other in a file |
| object types | AAC Main (prediction), LC, SSR (gain control), LTP; HE-AAC (SBR) and HE-AACv2 (PS, IPD/OPD included); 1024 and 960 sample frames; ER AAC LC, LD and ELD in LATM and AUS1, without the error resilience tools |
| syntax | SCE, CPE, CCE, LFE, DSE, PCE, FIL, END; pulse, TNS, gain control, M/S, intensity, PNS; fill payloads: SBR (with or without CRC), DRC, padding, others as bits |
| as bytes | ER AAC with the resilience tools, ELD with LD-SBR, LTP in ER AAC LD, USAC and other object types; MP4 (mp4p's), MPEG-TS, Matroska and other containers |

The syntax follows aacdec, the FFmpeg 8.1.3 AAC decoder ported to C++.
aac2tsv was cross-checked against decoders with it, and aacpack has its
walk.

## How it works

### Lossless by construction

One piece of code walks the AAC syntax both ways (`syntax.inc`, `units.inc`;
aac2tsv's). Reading (R), it parses the bitstream and hands each value to a
channel. Writing (W), it takes each value from a channel and writes the
bits. Values come as numbers, with the variable-length coding removed:
codebooks, scalefactor deltas, quantized spectral values with their signs
and escapes, SBR and PS deltas. What the writer can derive is not a value:
frame and element lengths, counts, CRCs, block positions, escape and
sign bits, the bits of a codeword. Where the stream has one that is not
what the writer would derive (a CRC that is wrong, a length with padding
after it), the channel gets a flag and then the value.

`c` takes the input a unit at a time (an ADTS or LOAS frame, an ADIF
header, a raw_data_block, an access unit). It parses the unit into a log
of values, then writes the unit back from the log. If that does not give
the same bytes, or the same syntax state for the next unit (configuration,
SBR headers), the unit is stored as its bytes. Otherwise it is walked a
third time, writing again, with the model coding each value as the
walker takes it. `d` runs that same writing walk, with the model decoding
each value. The model therefore sees the same state at every value in
both directions: the walker's position, the channel's bands, codebooks,
scalefactors and spectral values so far, and the model's own records of
earlier frames. Every unit's walk starts from the same reset state.

An element that does not parse in a unit that otherwise does (a broken
SBR payload, an unknown extension, ER tools) is kept as its bits, and so
is the rest of the unit after it. The walk can write any such rest back,
so a broken frame costs its broken part, not the frame.

### The model

h264p's: binary context mixing, after oggcomp. Each value is a series of
binary decisions. Each decision is predicted by about ten adaptive
counters under different contexts, an APM, three mixers and a fourth
that mixes them, and a second APM, selected by the decision's place in
the value. The decisions are coded with a binary range coder (15-bit
probabilities). Values are grouped into families, each with its own
contexts and parameters (`famvars.inc`, `model.inc`):

- **spectra** (`spc`, `esc`): each quantized magnitude in unary, under
  its codebook and the magnitudes before it in the window. Other
  contexts are the magnitudes at its place in the channel's last frame,
  scaled by the change of the band's scalefactor; the other channel of a
  CPE at its place; its place in the band and the tuple; and the band's
  magnitudes so far. Escapes of codebook 11 are coded as numbers, under
  the last frame's.
- **signs** (`sgn`): under the signs before, the other channel's, and
  the last frame's at the place. They come out at about a bit each: the
  signs of MDCT coefficients are close to random. A per-line predictor
  across frames, as AAC Main's prediction does it, predicts strong
  stationary tones but saves only 0.2–0.4% of the sign bits of the test
  music, so it is not used.
- **sections** (`sec`): the codebook as a tree of decisions, and the
  section length as a decision for each band (does the section go on).
  Both are coded under the last section's codebook, the band's codebook
  and section start in the last frame of the channel, the other channel's
  codebook and the group before.
- **scalefactors** (`scf`): each delta under the last two deltas, the
  delta at the band in the last frame, and the last frame's value there
  relative to this band's predecessor. Intensity positions and noise
  energies have their own contexts. Also used: the band's codebook, the
  other channel, and the change of global_gain.
- **SBR and PS** (`sbr`): envelope and noise floor deltas (in frequency or
  time) and PS parameters, under the band, the deltas before it, the
  value at the band in the last envelope of the kind, and the level the
  band had.
- **fields** (`hdr`): everything else (headers, window sequences,
  max_sfb, global_gain, flags, TNS and pulse data, DSE and fill bytes).
  Each is coded as the difference to the last value of the element at its
  index in the channel. Its contexts are the last two differences, the
  element's value at the index before it (an M/S flag's band before), and
  for the sign whether the last value is at an end of the field's range (a
  flag's change has one sign). A CPE's second global_gain is coded
  against the first's. ADTS's buffer fullness is coded after its frame,
  against the encoder's bit reservoir: the last fullness, plus a mean
  frame's bits (from the frames so far), less this frame's.
- **bytes**: under order-0 and order-1 contexts and a match against
  the last 16 MB of bytes stored as they are (found by a hash of three
  bytes). A chunk of 64 KB or more that looks random is coded as it is,
  one coder step a byte, at no cost over its size.

Each family's contexts and parameters are declared in
`aacpack_src/IDX/l9/*.idx`. `aacpack_src/mk.sh` turns them into code
(`MOD/l9/`, kept in the repository so that `make` needs no perl) and can
tune them (below).

### The compressed stream

The magic `aacpk`, a version byte and a level byte, then one run of the
range coder, flushed once at its end. Its contents in order: the `-asc`
config if any; the SBR headers assumed (below); each unit's kind and the
unit, either its values or a count of bytes and the bytes; the end; and
the CRC-32 of the input. `d` takes data after the end for corruption.

An SBR payload can only be parsed with its element's SBR header, which
comes every so many frames. A stream cut out of a longer one can start
with payloads whose header comes later. Before compressing, `c` reads
ahead (at most 8 MB, or 1000 units with no such payload) for the first
header of each element whose payloads come before one. It assumes that
header from the start, and puts it in the compressed stream's own header.

### Streaming

Memory does not grow with the stream: `c` holds a window of the input
(a unit at most 1 MB, and at the start what the read-ahead for SBR
headers read: up to 1000 units, or 8 MB of bytes that are not AAC), and
`d` a unit's output. The model's tables are a
fixed 20 MB of address space, of which a run touches what its stream
needs (about 11 MB of memory in all for a stereo stream). `c` and `d`
are coroutines (h264p's Lib3) that ask their driver for
input and give it output, so pipes of any length work.

## Results

Two one-minute excerpts of a live concert recording (archive.org,
5gg 2010-08-27, tracks 1 and 6), encoded by fdkaac 1.0.0 (libfdk-aac
from its git, c41c3c7), the AAC encoder of ffmpeg 6.1.1 and faac 1.30:

<!-- TABLE1 -->

<!-- FATE -->

Where the bytes of fdkaac's 128 kbit/s stream go (`-v`):

| part | in the stream | compressed | |
|---|---:|---:|---:|
| spectral magnitudes (and escapes) | 626642 | 605420 | 96.6% |
| their signs | 209727 | 209038 | 99.7% |
| scalefactors | 53657 | 44933 | 83.7% |
| sections | 36080 | 17201 | 47.7% |
| headers and other fields | 35135 | 3474 | 9.9% |
| all | 961241 | 880078 | 91.6% |

The Huffman codes of AAC are already close to what the values carry. A
quantized spectral magnitude is all but white noise given its codebook
and its neighbours (in the frame, the last frame and the other channel),
and its sign is a coin toss. So most of what is left is the spectra, and
the gain is largest where the encoder's choices (sections, scalefactors,
M/S, the side information) are a larger part of the stream: low rates,
HE-AAC, ffmpeg's and faac's encoders.

## Limitations

- One thread, about 0.5 MB/s both ways on a current x86-64 core (30
  times real time at 128 kbit/s).
- The model's parameters are part of the format, and the version byte
  says which: a stream decodes with the version that made it.
- What is not AAC syntax (tags, other containers: MP4 is mp4p's, MPEG-TS
  packets) is coded by a general model of bytes.

## Test

    make test

makes a corpus in `test/corpus` (`test/gen.sh`, which needs ffmpeg and
python3, if it is not there: a test signal through ffmpeg's AAC encoder as ADTS, LATM, LOAS v1, ADIF,
AUS1 and raw access units, LC, Main, LTP, mono, 5.1,
PNS, intensity and TNS; with fdkaac and faac too, if present: HE-AAC,
HE-AACv2, LD, ELD, CRC, VBR). It round-trips every file through `c` and
`d` (`test/roundtrip.sh`, which takes any files: `test/roundtrip.sh
./aacpack *.aac`), then runs `test/check.sh`: no input, random bytes,
pipes, a stream cut short or with junk in it, strict
mode, and compressed streams cut short, with a byte changed, with bytes
after them, or not compressed streams at all.

`test/fuzz.py BIN WORKDIR SEED N files...` mutates inputs (random bytes,
cuts, bit flips, junk, splices, repeats, broken frame headers) and checks
that they round-trip with no unit stored as bytes for not coming out as it
was. It also mutates compressed streams, and checks that `d` fails cleanly
on them (exit status 1, no crash, no hang). Build with
`-fsanitize=address,undefined --param asan-use-after-return=0` (the
coroutines copy stacks) and run with
`ASAN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=0:replace_intrin=0`.

The streams checked in development: the AAC samples of FFmpeg's FATE suite
(the conformance streams al*, am*, ap*; SBR and PS ones; File1-7; PCE
channel configurations; ID3 tags), encodes of two one-minute music
excerpts by fdkaac (LC, HE, HEv2, LD, ELD, VBR, CRC), ffmpeg (LC, Main, LTP,
PNS/IS/TNS, mono) and faac, and these converted to ADIF, LOAS v0/v1 and
AUS1. All round-trip, and so do all of them under ASan and UBSan, and
thousands of fuzzed inputs and compressed streams.

## Sources

`aacpack_src/`:

| file | |
|---|---|
| `aacpack.cpp` | the command line |
| `aacpack.inc` | includes everything, in order |
| `bits.inc` `huff.inc` `tables.inc` `sbrfreq.inc` | bit reader and writer, CRCs; the Huffman codebooks; the tables of the syntax; SBR frequency bands |
| `elems.inc` `chan.inc` `log.inc` | the syntax elements' ids; the channel the walk exchanges values with; the log of a unit's values |
| `asc.inc` `syntax.inc` `units.inc` | AudioSpecificConfig; the syntax walk below the containers; ADTS, LOAS/LATM, ADIF, AUS1, raw access units |
| `model.inc` `famvars.inc` | the model: what each family is given, and the channel the model codes through |
| `cmstream.inc` | the compressed stream: the scanner, units parsed, verified and coded |
| `err.inc` `mem.inc` `cmbase.inc` `rc.inc` `cm.inc` `mapping.inc` `fam.inc` `famcfg.inc` `rcio.inc` | h264p's: errors, memory, the range coder, counters, mixers, APMs, the families and their configuration, the coroutine's I/O |
| `levels.inc` `lv9.inc` `IDX/` `MOD/` | the model's level, its families' IDX sources and the code generated from them |
| `cli.inc` `coro.inc` `drivers.inc` | files, progress, coroutines, the drivers of `c` and `d` |
| `Lib3/` | the stack-copying coroutines (mtwrap's, as h264p has them) |

### Tuning

`aacpack_src/mk.sh` regenerates `MOD/` from `IDX/` (`./mk.sh mod`), builds a
tuning binary whose parameters `IDX/opt.pl` hill-climbs in place
(`./mk.sh tuning 9`), and, faster, dumps each family's decisions while
compressing streams (`./mk.sh dump 9 DIR streams...`) so that
`IDX/ftune.py` can tune one family at a time by replaying them
(`./mk.sh famtune 9` builds the replayer). `./mk.sh check 9` checks that
the tuning, shipping and plain C++ builds code the same bytes. The
parameters are part of the format: a stream compressed by a patched
binary decodes only with that binary.

## License

MIT (`LICENSE`), except `aacpack_src/tables.inc`: its tables come from
FFmpeg (through aacdec and aac2tsv) and are LGPL 2.1 or later, as FFmpeg is.
