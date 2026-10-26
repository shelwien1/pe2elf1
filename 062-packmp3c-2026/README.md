# mp3c — MP3 frame-dump compressor

```
mp3c c cat.dmp cat.c      compress
mp3c d cat.c  cat.unp     decompress
```

`cat.dmp` and `path.dmp` are the bundled test dumps. `mp3c.exe` is the original
reference build (Intel C++, x64) kept for comparison. `t.bat` runs a roundtrip
check against both dumps.

Build with `g.bat` (MinGW g++), `gc.bat` (clang++ with the MSVC headers/libs) or
`c.bat` + `icl64.cfg` (Intel icl).

`mp3pack` ([mp3pack](#mp3pack), at the end) is mp3rw and mp3c in one
program, with aacpack's command line and threads: `mp3pack c in.mp3 out
outside` and `mp3pack d out outside in.mp3`.

`mp3c_improvement.md` measures where mp3c's output goes, part by part, and
what would make it smaller. The model's mixing stages ([Compression: mixing
stages](#compression-mixing-stages), at the end) came of it: the `.c`
files are 1.3% smaller on the Layer III test inputs, for about 1.3 times
the run time (1.4 times the instructions).

## What was broken

The program did not survive a build with anything newer than gcc 10.2. There
were two separate causes, and they had to be fixed together.

### 1. `#pragma pack(1)` disappeared with the move to `Lib3`

`mp3rw/frame_dump_io.inc` serialises the *raw bytes* of `struct mp3frame`
between `&GrbLen` and `&Garbage`, and derives `max_frame_size` from
`offsetof(mp3frame, ...)`. The on-disk dump format is therefore the struct
layout.

`Lib/common.inc` contained a `#pragma pack(1)` that was never popped, so the
whole program was byte-packed. `Lib3/common.inc` has that line commented out —
correctly, since it is a shared header — so switching to `Lib3` silently changed
the layout. `read_frame_dump` then deposited the frame fields at the wrong
offsets and the model indexed off the end of its tables: SIGSEGV inside
`ProcessHeader`.

Fix: `mp3c.cpp` now asks for the packing itself, right after `common.inc`, so
`Lib3` stays layout-neutral for other programs.

### 2. The hand-rolled `my_setjmp` stopped working in gcc 11

`Lib/coro2b.inc` used `Lib/my_setjmp2_x64.h`, whose inline asm declares fifteen
`"=r"` outputs purely to force the register allocator to spill everything to the
stack — the coroutine's live state has to be *in the saved stack window*
`[stkptrL, stkptrH)`, because `yield()` memcpy's that window out and back. It
also hard-codes a `+15` byte offset into the instruction stream as its return
address.

gcc 11 no longer spills the way that trick assumed. The coroutine came back from
its first resume with stale state and quit immediately — a 2-byte output file.
The bisect is clean: gcc 10.5 fine, gcc 11.5 / 12.4 / 13.3 / 14.2 all broken, at
every `-O` level.

`Lib3/coro3b.inc` already avoided this by using `__builtin_setjmp` /
`__builtin_longjmp` for gcc (`coro3_setjmp_x64.h`) and an asm block with a
complete clobber list for clang and icc (`coro3_setjmp_x64d.h`). The 32-bit
branch had not been updated, so it still miscompiled on gcc 11+; this tree adds
`Lib3/coro3_setjmp_x32b.h` and selects it the same way the x64 branch does.

The isolation is clean: swapping *only* the setjmp in the old `Lib/coro2b.inc`
for `__builtin_setjmp`, changing nothing else, makes gcc 11 / 13 / 14 produce
the correct 138150 bytes again.

It is worth not "simplifying" this back to one asm implementation for every
compiler. Compressing `cat.dmp` with each setjmp choice, over gcc 10.5 / 11.5 /
12.4 / 13.3 / 14.2 and clang 18, at `-O2` / `-O3` / `-Ofast` (18 builds each):

| setjmp used for gcc | result |
| --- | --- |
| `__builtin_setjmp` (shipped) | 18/18 correct |
| `coro3_setjmp_x64d.h` asm | 3/18 — only the clang builds |
| `<setjmp.h>` (`-DCORO_NOASM`, fortify off) | 18/18 correct |
| old `Lib/my_setjmp2_x64.h` asm | 15/18 — gcc 14 fails |

The last row is the point: the old asm happens to survive longer inside
`coro3b` than it did inside `coro2b`, but it still breaks, just later. Both
asm variants depend on the register allocator spilling in a particular way, and
that is not something a compiler promises.

## Changes

| file | change |
| --- | --- |
| `mp3c.cpp` | `coro2b.inc` → `coro3b.inc`; explicit `#pragma pack(1)` |
| `mp3rw/mp3mod.inc` | ten more `using mp3model<f_DEC>::…;` declarations |
| `Lib3/coro3_setjmp_x32b.h` | new — `__builtin_setjmp` variant for 32-bit |
| `Lib3/coro3b.inc` | 32-bit branch selects it for gcc, mirroring the x64 branch |
| `g.bat`, `gc.bat`, `icl64.cfg` | include path `Lib` → `Lib3` |
| `t.bat` | new — roundtrip check |

The `using` declarations matter because the names are looked up in a dependent
base class. gcc only accepted them under `-fpermissive` and clang only under
`-fms-compatibility`; declaring them makes the code ordinary C++, so it now
builds clean on gcc *without* `-fpermissive` and on clang without MSVC
compatibility mode.

## Verification

Every build below compresses `cat.dmp` to **138150 bytes** and `path.dmp` to
**4385281 bytes** — byte-identical to the reference `mp3c.exe` — and decompresses
back to the original.

| toolchain | target | flags |
| --- | --- | --- |
| MinGW-w64 g++ 13 | x86_64 PE | `-Ofast -std=gnu++11`, and `-O2 -std=c++23` |
| clang 18 | x86_64 PE (`--target=x86_64-w64-mingw32`) | `-O2 -std=c++17`, and `-O3 -std=c++23 -ffast-math` |
| g++ 10.5 / 11.5 / 12.4 / 13.3 / 14.2 | x86-64 ELF | `-O2`, `-O3`, `-Ofast` |
| clang 18 | x86-64 ELF | `-O2`, `-O3`, `-Ofast` |
| g++ 10 / 13, clang 18 | i386 (`-m32`) | `-O3` |

The Windows binaries were exercised under Wine. gcc builds no longer need
`-fpermissive`.

# mp3rw — MP3 frame dump / restore

```
mp3rw c cat1.mp3 .dmp .out      split into frame dump + everything else
mp3rw d .dmp .out cat1.unp      restore
```

`mp3rw0.exe` is the original (coro2) build; `.dmp` and `.out` are its output
for `cat1.mp3` (`trw.bat`). `gc.bat` builds `mp3rw.exe`.

## What was broken

After the port to `coro3b` there were three separate bugs.

### 1. The `.out` stream was on the wrong pin

In coro3b `pin[i]` yields `1+i`. `mp3rw.inc` writes `.out` through `pin[2]`
and `mp3rs.inc` reads it from `pin[2]` — the `r==3` the frontends handle — but
`coro_fp12.inc` / `coro_fp21.inc` gave their second buffer to `pin[3]`. `c`
crashed on the first `.out` byte (`pin[2].ptr` was NULL), and `d` hung asking
for `.out` data that kept going to `pin[3]`.

### 2. `#pragma pack(1)`

Same as for mp3c above: the dump is the raw bytes of `struct mp3frame`. Built
unpacked, every frame record has 2 bytes of padding after `CRC` (cat1:
3622909 bytes instead of 3622739), which is not the format of `mp3rw0.exe` and
mp3c.

### 3. `putbits` evaluation order

```
#define putbits( c,l ) ( ((uint&)Out[(OutPtr-=l)>>3])|=((c)<<(OutPtr&7)) )
```

decrements `OutPtr` in the left operand of `|=` and reads it in the right one.
That is unsequenced before C++17, and C++17 evaluates the right operand first.
clang does so in every `-std` mode, so `d` wrote every field at the wrong bit
offset; gcc does it when the right operand has a side effect, which is
`procbits( *scf++, num )` in `parse_scale2.inc`, so MPEG-2/2.5 files did not
restore. `c` was unaffected: `getbits` uses a shift, whose operands C++17
sequences left to right. Both macros now update the bit pointer in a separate
comma operand first.

## Changes

| file | change |
| --- | --- |
| `coro_fp12.inc`, `coro_fp21.inc` | second stream on `pin[2]`; `using Model::…` declarations, so gcc and clang build them without `-fms-compatibility` |
| `mp3rw.cpp` | `#pragma pack(1)` after `common.inc`, as in `mp3c.cpp` |
| `mp3rw/mp3_parse/parse__mp3.inc` | `getbits` / `putbits` update the bit pointer before using it |
| `mp3rw/mp3_model/model_scale2.inc` | mp3c: `scf++` moved out of the `ylong[scf-sf0].Process(...)` call (see below) |
| `mp3rw/mp3rw.inc` | a sequence's trailing bits are stored once, not twice (see [Audio between other data](#audio-between-other-data)) |
| `tp.bat` | new — mp3 → mp3rw → mp3c → mp3 roundtrip check |

## Verification

40 inputs: `cat1.mp3`; lame-encoded MPEG-1/2/2.5, mono/stereo/joint stereo,
CBR/VBR/ABR, CRC, ID3v1/v2, free format; truncated, corrupted, and concatenated
with junk (up to 6.4 MB, a 145 MB dump); empty, random, zero and 0xFF files.
For every input `c` writes `.out` byte-identical to `mp3rw0.exe` (run under
Wine), and `.dmp` too, except for inputs with more than one frame sequence
(see [Audio between other data](#audio-between-other-data)); `d` restores the
input exactly, with each of:

| toolchain | target | flags |
| --- | --- | --- |
| g++ 13 | x86-64 ELF, i386 (`-m32`) | `-O2`, `-O3`, `-Ofast`; `-std=gnu++11`, `-std=c++23` |
| clang 18 | x86-64 ELF, i386 (`-m32`) | `-O2`, `-O3`, `-Ofast`; `-std=c++17`, `-std=c++23` |
| MinGW-w64 g++ 13 | x86_64 PE | `g.bat` flags |
| clang 18 | x86_64 PE (`--target=x86_64-w64-mingw32`) | `gc.bat` flags with MinGW instead of MSVC headers and no `-fms-compatibility`; with and without `-flto` |

Linux builds need `-D'__max(a,b)=...'` / `-D'__min(a,b)=...'`; the MSVC and
MinGW headers define those.

## mp3rw + mp3c

`tp.bat` runs the whole chain, `mp3rw c` → `mp3c c` → `mp3c d` → `mp3rw d`,
and compares the restored mp3 with the original (`tp.bat` for `cat1.mp3`,
`tp.bat a.mp3 b.mp3` for others). Only the mp3 has to come back: `mp3c d` does
not reproduce the `.dmp` byte for byte (`p23sum`, `p23len` and the unused bits
of the last `Garbage` byte differ, and `mp3rw d` doesn't use them), so `t.bat`'s
`.dmp` comparison would report a mismatch on an mp3rw dump.

On the 40 inputs above every mp3 comes back exactly with 18 mp3c builds: g++ 13
and clang 18 at `-O2`/`-O3`/`-Ofast` with `-std=gnu++11`/`c++17`/`c++23`,
`-m32`, and MinGW g++ / clang x86_64 PE under Wine, each paired with an mp3rw
from the same compiler. Before the layer I/II support below, all 18 wrote `.c`
files byte-identical to the original `mp3c.exe` (the Sep 2017 build, run under
Wine) on every input, MPEG-2/2.5 included, and `.c` files decoded in both
directions. The layer I/II support adds a bit per frame, so `.c` files of this
version and of `mp3c.exe` no longer decode with each other.

`mp3rw/mp3_model/model_scale2.inc` had the same kind of expression as the old
`putbits`: in `ylong[scf-sf0].Process( rc,*scf++, ... )` a pre-C++17 compiler
may compute the context index after the increment, which would change the
MPEG-2/2.5 format. `mp3c.exe` and every gcc/clang build compute it before, so
the increment is now a separate statement after the call: same output, no
longer up to the compiler.

# Layer I / II

mp3rw and mp3c also take MPEG-1 and MPEG-2 LSF Layer I and Layer II frames
(`.mp1` / `.mp2`), alone or mixed with Layer III in one file. MPEG 2.5 and
free format layer I/II frames stay non-frame data, as free format does for
Layer III.

## mp3rw

- `mp3rw/mp3_parse/parse_l12.inc`: `ProcL12` reads or writes the audio data
  (bit allocation, scfsi, scalefactors, samples) through `procbits`, the way
  `ProcSideInfo` does for Layer III; `ReadFrameL12` / `WriteFrameL12` do the
  frame around it. Grouped samples (3, 5, 9 levels) are stored split up, and
  a group code that can't be split up means it's not a frame.
- `parse_l12tab.inc` has the bitrate, quantization class and the five Layer II
  bit allocation tables; they were checked against ffmpeg's, row by row.
- The dump record keeps its layout: `l12data` overlays `si` / `SF` / `MC` (a
  union in `parse_struct.inc`), the ancillary data goes into `Garbage` /
  `GrbLen`. `frame_dump_io.inc` is unchanged.
- A layer I/II frame is only taken if it follows the previous layer I/II frame
  or the next frame header follows it. Random data, cover art in ID3 tags for
  example, would otherwise turn up plenty of them.
- The CRC is checked over header bits 16..31 and the allocation (Layer II:
  and scfsi). If it doesn't match, the frame is still taken and its CRC kept.
  For Layer I joint stereo that counts the transmitted allocations only, as
  ISO dist10 and oxideav-mp1 do; ffmpeg checks a fixed 256 bits there.
- When layer I/II frames follow Layer III ones, `ReadFrame` ends the Layer III
  sequence first (it returns 3 and `mp3rw.inc` starts a new sequence), so
  the bit reservoir code doesn't change. A Layer III frame after layer I/II
  frames starts without a reservoir.

Two things in the Layer III reader came up on the way:

- It assumed no bytes past its frame end were buffered. Checking a layer I/II
  candidate reads up to a whole frame ahead, and when that candidate is
  rejected the bytes stay buffered; a Layer III frame found inside them then
  dropped the rest on restore. A fake layer II header placed right before an
  mp3 stream showed it. `ReadFrame` now keeps such bytes aside while it parses
  the frame, and the retry after a non-frame (`retval==2`) rewinds to the end
  of the last frame (`BufPtr=0`) instead of to `BufPtrSaved`, which assumed
  there was nothing buffered past it.
- The loop that clears `part2_3_length` on bad frames went through
  `si.gi[0][i]` with i up to 3, past the inner `[2]`. gcc warns that it may
  stop that loop at i=2; it's `si.gi[i>>1][i&1]` now.

## mp3c

`mp3rw/mp3_model/model_l12.inc`. Each frame record gets an adaptive bit for
Layer III vs layer I/II (about 15 bytes over 11 MB of Layer III `.c` files);
the Layer III modelling isn't touched. This changes the `.c` format.

The layer I/II model codes:

- header fields, each by the previous layer I/II header's value; padding by
  the last 8 padding bits; for a CRC only whether it matched;
- allocation by the previous frame's value and the subband below (channel 1:
  channel 0's);
- scfsi and scalefactors, each by the previous one in the subband;
- samples as offset from the middle code: its size (bit length) by class,
  previous size in the subband and the subband below, then the two bits below
  the top one, the rest raw, and the sign;
- ancillary bits order-1; their number follows from the allocation.

Channel 1 is coded against channel 0: a flag for "same sample as channel 0's"
where both have the same allocation, and channel 0's size and sign as
context otherwise; likewise a flag for "same scalefactor". Broadcast streams
are often mono carried as stereo, where these flags take most of the second
channel.

| file | | size | `.c` + `.out` |
| --- | --- | --- | --- |
| `fl1` (ISO conformance, `mp1-sample.mp1`) | L1 384 kbps 32 kHz stereo | 28224 | 54.8% |
| `mpeg_layer1_audio.mpg`, audio stream | L1 128 kbps 44.1 kHz stereo | 880084 | 44.3% |
| `harry-part.mp2`, audio stream | L2 224 kbps 44.1 kHz stereo | 979796 | 85.2% |
| `mp2-bug/mpeg-adts-mp2-48hz.mp2`, audio stream | L2 320 kbps 48 kHz, mono as stereo | 5215184 | 47.2% |
| twolame / ffmpeg encodes of test signals | L2, 32..384 kbps | | 74..87% |

The first four are from samples.ffmpeg.org.

## Verification

97 inputs: the 40 Layer III ones above; 26 Layer II streams from twolame and
ffmpeg (all five allocation tables, all four channel modes, CRC, MPEG-2 LSF,
VBR, padding, energy-level and reserved-bit ancillary data); 7 synthetic
Layer I streams (all modes and joint stereo bounds, CRC, ancillary data; ffmpeg
decodes them without errors); the real streams above; and 20 mixed and broken
ones (Layer I/II/III switching back and forth, junk between layers, ID3 tags,
truncated, corrupted, a 6 MB stream, fake syncs in random data, fake headers
right before an mp3 stream).

- In every twolame / ffmpeg stream without ancillary data, the bits after the
  audio data are all zero, and every CRC (twolame `-p`) matches: the allocation
  tables and the bit accounting agree with those encoders.
- `mp3rw c` output for all Layer III inputs, and for 8 real mp3s from
  samples.ffmpeg.org (cover art, "misidentified" files, a broken first frame),
  is byte-identical to `mp3rw0.exe`'s, except for the `.dmp` of the 6 inputs
  with more than one frame sequence (see below).
- The whole chain restores every input exactly with each of 18 builds, as
  above (gcc / clang, `-O2`..`-Ofast`, `-m32`, MinGW and clang PE under Wine),
  and all of them write the same `.c` files.

# Audio between other data

In a movie the audio frames alternate with video data: AVI and MKV put every
frame in a chunk of its own, MP4 a few frames per chunk. For mp3rw each run of
frames between two pieces of video is a frame sequence of its own.

## Bits stored twice

At the end of a sequence `mp3rw.inc` writes the bits after the last frame's
main data (`Garbage` / `GrbLen`) to `.out`, and it left them set. The first
frame of the next sequence has nothing of its own to put there (nothing is
pending for `GarbageStore`), so its `.dmp` record carried the same bits again.
`mp3rw d` doesn't use them, so files still restored; the copy only took space.
`mp3rw0.exe` does the same.

In a plain mp3 that's nothing, or tens to hundreds of bytes where files were
concatenated. In a movie it's most of the audio: a frame that starts a
sequence has lost the bit reservoir it was encoded with, so when its main data
starts in the reservoir (`main_data_begin` > 0), mp3rw keeps the whole frame
body as garbage, and every frame body ended up in both files. `mp3rw.inc` now
clears `GrbLen` after `write_garbage()`:

| file | size | before: `.c` + `.out` | now |
| --- | --- | --- | --- |
| `movie_l3.avi`: MPEG-4 video, mp3 128 kbps | 2583452 | 1031345 + 2509862 | 61945 + 2509862 |
| same, MKV | 2505327 | 1033928 + 2432655 | 62645 + 2432655 |
| same, MP4 | 2510138 | 1034843 + 2097108 | 427424 + 2097108 |
| `lowvideo.avi`: mp3 320 kbps | 1782112 | 1696126 + 1732223 | 36362 + 1732223 |

Only the first frame record of each sequence after the first changes, so
`.out` is the same as before and so is `.dmp` for a file with one sequence.
`mp3rw0.exe d` restores the new dumps. Of the 40 Layer III test inputs, 6 have
more than one sequence (concatenated files, junk, random data). The whole chain
restores all 97 inputs from above, the 5 movies, 8 real mp3s and 3 MPEG
program/transport streams exactly. The fix changes the dumps of 14 of them, and
for those all 18 builds restore them and write the same `.c` files.

## Not done: the bit reservoir across other data

The movies still hardly shrink: as above, a frame after video data has no
reservoir, so most frame bodies stay raw, now in `.out` only. The same audio
packed on its own (`ffmpeg -c:a copy`) shows what that costs:

| file | now | audio on its own, packed, + the rest of the file |
| --- | --- | --- |
| `movie_l3.avi` | 2571807 | 822235 + 1625603 = 2447838 |
| `movie_l3.mkv` | 2495300 | 822235 + 1547478 = 2369713 |
| `movie_l3.mp4` | 2524532 | 822235 + 1552289 = 2374524 |
| `lowvideo.avi` | 1768585 | 1229802 + 180284 = 1410086 |

Getting there means continuing the sequence across the other data: `mp3rw c`
would join the frames up as a demuxer does and record where the other data
goes, which changes the `.dmp` / `.out` format.

Layer I/II frames don't need a reservoir, but in a movie mp3rw mostly doesn't
find them: a layer I/II frame is only taken next to another one (see
[Layer I / II](#layer-i--ii)), and in an AVI every frame has video data on both
sides. Of the 375 Layer II frames in `movie_l2.avi`, mp3rw takes 11.

# IDX: the model's contexts and parameters

mp3c's model declares its context indices and its tunable parameters in
IDX (`mp3rw/mp3_model/IDX/*.idx`), which `IDX/idx2inc.pl` turns into C++
(`mp3rw/mp3_model/MOD/`). The framework is aacpack's (h264p's): its
generator, runtime (`mapping.inc`), optimizer (`IDX/opt.pl`), import
(`IDX/import.pl`), document (`IDX/IDX-FORMAT.md`) and `mk.sh`:

```
./mk.sh mod              MOD/ again, in the shipping form (every parameter folded)
./mk.sh release          ./mp3c, the shipping build
./mk.sh tuning           ./mp3c.tune: the knobs the .idx sources state live behind "!MAP!", for opt.pl
./mk.sh check DUMP...    the tuning and the shipping build compress the dumps alike
```

`MOD/` stays in the repository, so that `g.bat` and `gc.bat` need no perl;
`gc.bat` regenerates it with the new generator first, as `mk.sh mod` does.

What changed from mp3c's old IDX tools:

- aacpack's generator has none of three forms mp3c's model is written in:
  `Rate` (a row of the rate schedules `TTable`), `ADD` (a factor with no
  mapping) and `b&` (a binary tree's node, the library's `masking_b`). They
  are back, ported from the old generator (IDX-FORMAT.md §16).
- `IDY/`, the spectral model's declarations split by decision with their
  long place patterns cut in 24-bit pieces, is merged into
  `IDX/sh_model.idx`, as `gc.bat` used to merge it before every build. The
  pieces in `sh_model_counts.idx` are joined too: the new generator takes a
  pattern of any length.
- The templates' `def_Data` / `def_Init` blocks, which only the old
  generator had, are ordinary members and an `%M%InitData()` the model
  calls after `%M%_Init()`.
- The tuning form has its tables allocated as the model starts and every
  live knob a mapping object opt.pl patches in the binary. The `.idx`
  sources keep their `Const 1` lines: the parameters tuned so far stay
  constants in both forms until a section is made `Const 0`.

The shipping build writes byte-identical `.c` files to the old tools' on
all 124 test inputs, and so do the MinGW g++ and clang PE builds (under
Wine), and the tuning form with every knob made live (1451 patterns, 30249
bits) on 39 of them.

# Compression: mixing stages

mp3c codes each binary decision with two counters, an SSE on one of them
and a mixer of one weight. Most decisions now go through a **mixing
stage** instead, aacpack's context mixing (`aacpack_src/cm.inc`) cut down
to what the time allows: `mp3rw/mp3_model/freq_cm.inc` (`cm_code`). A
stage mixes mp3c's own two predictions of the bit (the counter's and the
SSE's) with four counters of its own, each with an adaptive and a fast
rate, in a mixer of 11 inputs, and trains all of it on the bit.

| decisions | the stage's counters, by | weight sets |
| --- | --- | --- |
| zero flags, \|v\|=1..4 and the \|v\|=x loop after them (`IDX/sh_model.inc`: `XC`), in place of mp3c's mixer there | the last granule's value at j rescaled to this granule's quantizer step (global_gain, the band's scalefactor, scalefac_scale, pretab); the places left before the count1 region and the zeros; the place; the last granule from j−2 to j+2 and the one before | mp3c's mixer contexts (`mm0A_g` .. `mmx_g`) |
| MPEG-1 long-block scalefactors (`IDX/sh_model_counts.inc`: `Process_sf1xlong`) | the channel's last granule's value; the other channel's; the band below's; the change of global_gain; the value that would keep the band's quantizer step | band, node |
| MPEG-1 short-block scalefactors (`Process_sf1short`) | the last short granule's value at the band and window; the window before; the band below; the other channel's | band and window, node |
| MPEG-2 scalefactors (`Process_sf2`) | as MPEG-1's long blocks, by position and kind of block | position, node |
| global_gain (`Process_GlGain`) | the channel's last change; the other channel's; its last value; the other channel's value | depth |
| the counts c1 and k (`Process_cnt`) | the channel's last value; the other channel's; the change of global_gain; the block type, and c1 for k | depth |

Their contexts and parameters are the `Const 0` sections at the end of
`IDX/sh_model.idx` and `IDX/sh_model_counts.idx` (IDX-FORMAT.md §16). The
parameters were tuned with `IDX/opt.pl` on 12 MB of eight dumps
(different encoders, rates and stereo modes); the contexts' patterns are
as written, untuned.

Two more changes to the model:

- **The scalefactor band table.** `curband`, the scalefactor of the band
  a place is in, which most spectral decisions see, came from the 44.1
  kHz table at every rate. It now comes from the frame's own table
  (`bandInfo[sfreq]`, `model_delta.inc`), as do the stages' bands and
  steps (mp3c_improvement.md §3.6).
- **Scalefactors that are not coded are 0 on both sides.** The decoder
  always had 0 where no group codes a scalefactor (MPEG-2's empty groups
  and the places after them, a mixed block's first short bands, a long
  block's short ones), the encoder whatever the dump brought, which in a
  frame mp3rw could not parse is left over from the stream. The new
  contexts read these places; reading such a difference made the decoder
  lose sync on two of the test streams before this was fixed.

The `.c` format is a new one again: files of the previous mp3c, and of
the original `mp3c.exe`, do not decode with this one, nor the other way
round.

## Results

The `.c` files of the model as it was before this (the build of the IDX
port, which writes what the earlier model wrote) and now:

| file | | `.c` before | now | |
| --- | --- | ---: | ---: | ---: |
| `path` | 224 kbps, stereo: the source of `path.dmp` | 4385282 | 4369342 | -0.36% |
| `01_-_Charity_Case` | 320 kbps, L/R | 5796038 | 5698454 | -1.68% |
| `Ed_Rush_-_Sabotage` | 128 kbps, L/R | 4858494 | 4745303 | -2.33% |
| `Silent_Light` | 128 kbps, joint | 3610729 | 3568705 | -1.16% |
| `broken-first-frame` | 192 kbps, L/R | 85905 | 85723 | -0.21% |
| `cat1` | 64-96 kbps, mono | 138150 | 137054 | -0.79% |
| `jpg_in_mp3` | VBR 128-192, L/R | 3634861 | 3606984 | -0.77% |
| `mp3_misidentified` | 256 kbps, L/R | 5289936 | 5257820 | -0.61% |
| `mp3_misidentified_2` | 192 kbps, L/R | 7031621 | 6972828 | -0.84% |
| `mp3_with_embedded_albumart` | 320 kbps, L/R, cut at 2 MB | 487211 | 485675 | -0.32% |
| `01_-_Charity_Case_lame_128` | LAME 3.100 -b 128, joint | 2668684 | 2632300 | -1.36% |
| `01_-_Charity_Case_lame_V2` | LAME 3.100 -V2, joint | 4007355 | 3935983 | -1.78% |
| `mp3_misidentified_lame_128` | LAME 3.100 -b 128, joint | 3092815 | 3061890 | -1.00% |
| `mp3_misidentified_lame_V2` | LAME 3.100 -V2, joint | 4708613 | 4656059 | -1.12% |
| `misid_22k_64` | LAME, 22.05 kHz 64 kbps (MPEG-2) | 1556724 | 1533464 | -1.49% |
| `misid_32k_128` | LAME, 32 kHz 128 kbps | 3144219 | 3120740 | -0.75% |
| `misid_48k_160` | LAME, 48 kHz 160 kbps | 3882770 | 3842407 | -1.04% |
| all 125 test inputs | | 91640345 | 90667110 | -1.06% |
| the 70 whose `.c` changed (those with Layer III frames) | | 73138796 | 72165561 | -1.33% |

All 125 inputs restore exactly through `mp3rw c`, `mp3c c`, `mp3c d` and
`mp3rw d`. Nine builds write the same `.c` files and decode them alike
(4 MB of four of the dumps, `path`'s among them, and a layer I stream):
gcc 13 at -O2, -O3, -Ofast, with -std=c++23 and with -m32; clang 18 at
-O2, -O3 and -Ofast; and MinGW-w64 g++ for x86-64 Windows with `g.bat`'s
flags, under Wine. (A 32-bit Windows build could not be run here: this
Wine has no 32-bit support.)

`path.mp3` gains least. Its scalefactors and global_gain are half a
percent of its `.c`, against 4-7% in most of the others, so the stages
there have little to work on; and it is one of the two dumps mp3c came
with, which its model's parameters were probably tuned on.

## Speed

The stages cost time per decision, and replacing mp3c's mixer at the
magnitude decisions paid for part of it: what the mixer's tuned contexts
and its own stretch tables cost turned out to be most of what the stage
costs. Changes that do not change what is coded paid for some more: the
layer I/II tables are made when the first such frame comes, `j_band`'s
thresholds and `recm` come from tables, `Node2ii::Update` is inlined,
and `idx2inc.pl` writes a pattern of ones then zeros as a clamp instead
of a table.

| the new model's instructions over the old's, on | compress | decompress |
| --- | ---: | ---: |
| 12 MB of `mp3_misidentified_lame_128` | 1.401 | 1.398 |
| 12 MB of `01_-_Charity_Case_lame_V2` | 1.385 | 1.381 |
| 12 MB of `Ed_Rush_-_Sabotage` | 1.391 | 1.388 |
| 12 MB of `Silent_Light` | 1.387 | 1.384 |
| 12 MB of `misid_22k_64` | 1.407 | 1.405 |
| 12 MB of `misid_48k_160` | 1.393 | 1.390 |
| all of `path.dmp` (40 MB) | 1.368 | 1.365 |

These are instruction counts (cachegrind), which are repeatable. The run
time grows less, as far as this machine (a VM, whose run times vary by
several percent) shows it: the least user time of three runs on the whole
dump, in seconds:

| | compress | | decompress | |
| --- | ---: | ---: | ---: | ---: |
| `path.dmp` | 2.79 → 3.64 | 1.30x | 3.06 → 3.88 | 1.27x |
| `mp3_misidentified_2.dmp` (87 MB) | 5.11 → 6.37 | 1.25x | 5.36 → 6.84 | 1.28x |

# mp3pack

mp3rw and mp3c in one program, with the front end of
[aacpack](https://github.com/shelwien1/pe2elf1/tree/claude/aacpack-lossless-compressor-rlba00):
its command line, files, progress line and threads (MTwrap).

```
mp3pack c [options] input output outside      compress
mp3pack d [options] input outside output      decompress
```

`c` parses the input as `mp3rw c` does, codes its frames with mp3c's model
into `output`, and writes what is not frames into `outside`: the bytes
between runs of frames, each run's length and the bits after its last frame.
That file is mp3rw's `.out`, byte for byte, and is not compressed (whatever
compresses the rest of an archive takes it). `d` restores the input from
the two. Any input goes: a file with no frames comes out as 17 bytes and
the file itself as its outside data.

| option | |
| --- | --- |
| `-v` | statistics on stderr: the sizes, the outside data, the frames and their runs; on threads, the parts |
| `-p` | the progress line even when stderr is not a terminal (where it is, always) |
| `-t N` | `c`: compress on N threads, 1 to 64 (default 1): the frames in parts (Threads, below); `d` takes N from the stream |
| `-c N` | `c -t`: the size of a part of the input, at least: N MB, 1 to 1024, or N KB with a `k`; 1 MB by default |

As in aacpack: input, output and outside can be `-` (stdin, stdout; one of
each at most), and no two the same file. An output is written under a
temporary name beside it (`name.PID.tmp`) and renamed when the call is done,
so a call that fails, or is ended by a signal (SIGINT, SIGTERM, SIGHUP,
SIGPIPE, ...), leaves what was there; `c` keeps both of its outputs or
neither. Exit status: 0 done, 1 an error (said on stderr), 2 a bad command
line. On a terminal (or with `-p`) `c` and `d` show their progress:

```
[enc] 4206720 > 3568720  100.0%  10086 frames  done in 3.74s  38.09M RAM at most
```

```
./mk.sh mp3pack           ./mp3pack (g.bat: mp3pack.exe, with MinGW g++)
```

mp3pack is one translation unit, `mp3pack.cpp` (its command line) and
`mp3pack_src/` (the rest), on Lib3, MTwrap (`MTwrap_v3/`, aacpack's, and
Lib3's `win_shim.inc`) and mp3rw's and mp3c's sources as they are.

## How it works

The parser (`mp3pack_src/split.inc`) is mp3rw's: a coroutine whose output
is mp3rw's dump, the frames' records, and which writes the outside data
straight to its file, through a buffer, as it comes, not through a second
output of the coroutine (mp3rw's pin 2). The dump goes to mp3c's model in
memory: the two coroutines chained (Lib3's `CoroutinePair`), and its records
not padded to 1 MB blocks as the `.dmp` file has them (`frame_dump_io.inc`,
`FRAME_DUMP_ALIGN`). `d` runs mp3c's model into the restorer (`join.inc`,
mp3rs's), which reads the outside data from its file the same way.

The stream: the magic `mp3pk`, a version byte, the threads (1: one model)
and the part size in KB (u32), then the stream of mp3c's model, the bytes
`mp3c c` writes for the dump (with one model the stream is mp3c's `.c` and
15 bytes), or on threads MTwrap's container of the workers' streams; and the
CRC-32 of the input, which `d` checks.

## Threads

As aacpack's `-t` (`mt.inc`): the parser, on the main thread, writes the
outside data and puts the frames' records out in parts, and MTwrap deals the
parts in turn to N workers, each with mp3c's model. A part ends before the
first frame at least `-c` of the input after its start, or at 16 times that
of records (a frame's record is about 5.5 KB: 5 to 80 times the frame). A
worker reads a part whole, codes it, and writes a record (the part's size
compressed, its records' size and its place in the file) and the part
compressed. A worker's model goes on from one of its parts to the next:
`d` deals the parts to the workers in the same turns, so each worker
decodes the parts it coded, in that order, and its model learns from all of
them (aacpack's starts from nothing in each part: Results, below). `d` runs as many threads as the stream says, and
MTwrap takes the parts' records from the workers in turn into the restorer,
on the main thread.

## Broken streams

`d` takes the last 4 bytes of its input for the CRC and the rest for the
stream. It fails, with exit status 1 and its output removed, where:

- the stream does not end where its input does (a model that ends before
  it, or wants more than 8 bytes past it: those are 0xFF, past which a
  range coder never reads more than 4), or a part's record does not match
  its part (on threads);
- mp3c's model decodes a frame mp3rw's parser would not take: a header with
  no bit rate or the reserved sampling rate, `big_values` past 288, more
  bits after the frame than mp3rw keeps (`BadData()`, which ends the
  model's run: these checks, and two reads now bounded, are the model's
  only change, and none touches what a valid stream codes);
- the restorer gets a frame its writer has no tables for, or whose bits
  after it would go past the writer's buffer, or the outside data ends
  before the dump does, or goes on after it;
- the file restored is not the one compressed (the CRC): another file's
  outside data, a stream changed.

## Results

Six of the test files end to end (`01_-_Charity_Case`, `Ed_Rush_-_Sabotage`,
`Silent_Light`, `jpg_in_mp3`, `mp3_misidentified` and `path`: 35679367
bytes, 58188 frames), on 4 cores of the VM the rest was measured on:

| | compressed | | `c` | `d` | memory |
| --- | ---: | ---: | ---: | ---: | ---: |
| `mp3rw c` + `mp3c c` (and back) | 27259506 and 82523 of `.out` | | 28.4 s | 28.9 s | |
| one model | 27259521 and the same `.out` | | 28.6 s | 28.3 s | 38 MB |
| `-t 2` | 27303105 | +0.16% | 14.3 s | 14.2 s | 123 MB |
| `-t 4` | 27352027 | +0.34% | 7.8 s | 7.8 s | 227 MB |
| `-t 4 -c 256k` | 27369982 | +0.41% | 7.5 s | 7.6 s | 174 MB |
| `-t 4 -c 4` | 27300196 | +0.15% | 8.3 s | 11.0 s | 417 MB |

With one model, mp3pack takes the time `mp3rw` and `mp3c` take, without
the dump between them (about 280 MB of it here). On threads the stream is
larger by what a model learns from the frames the other workers have: a
worker's model sees a quarter of them on 4. That its model goes on from
part to part keeps small parts cheap: on `Silent_Light` in parts of 256 KB
on 3 threads, +0.42%, where models started from nothing in each part cost
+1.27%. With few parts a worker can be left with more than the others: at
4 MB this file is 9 parts. Each worker has a model (40 MB) and holds a part
whole, its records: up to 16 times the part size.

## Tests

- The 125 test inputs round-trip, with one model and on 3 threads in parts
  of 64 KB. Their outside data is mp3rw's `.out`, and with one model the
  stream is mp3c's `.c`, byte for byte.
- gcc and clang builds write the same streams; so does the MinGW g++ build
  under Wine (x86-64), with and without threads, and each decodes the
  other's.
- Streams cut short, with bytes changed or after them, with another file's
  outside data or with outside data cut short or longer; pipes; an input
  and an output that are the same file; a signal mid-call (the temporary
  files go, a file that was there stays).
- `test/fuzz.py BIN WORKDIR SEED N files...` (as aacpack's): streams and
  outside data cut, with bits flipped, bytes inserted, deleted or repeated,
  of each file compressed with one model and on 2 threads. On ten small
  files (MPEG-1, 2 and 2.5 layer III, layers I and II, with CRCs, runs
  between other data), 12000 of them, 6000 with a UBSan build
  (`-fsanitize=undefined`, but for alignment and shifts of negative values,
  which mp3c's parser and its CRC make on purpose): `d` ends with 0 (the
  file back) or 1 on each, no crash, no hang, no report. The runs before
  found the frames mp3c's model and the restorer now refuse (above). ASan
  cannot run the chained coroutines, as aacpack's `c -t`: it crashes in
  their stack copies.
- `test/roundtrip.sh BIN files...` round-trips files with one model and
  with `OPTS` (default `-t 3 -c 64k`), and checks the outside data against
  `mp3rw c`'s `.out` where `MP3RW` names one.
