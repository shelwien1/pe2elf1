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
