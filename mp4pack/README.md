# mp4pack

Lossless compression of whole mp4/mkv/avi files, for Windows. `mp4pack.exe` is a
wrapper: it takes the container apart with `avip`, gives each stream to the
packer made for it, and stores the results in one `shar a0` archive.

```
mp4pack c [options] input output     compress
mp4pack d [options] input output     decompress
```

The tools are looked up in the directory of `mp4pack.exe` itself, so put it next
to them: `avip`, `h264p`, `aacpack`, `mp3pack`, `oggcomp`, `ttfcomp`, `shar`. Each
can be there in builds for several instruction sets, named by a suffix:
`h264p_avx2.exe`, `h264p_avx512.exe`, `h264p.exe` ... mp4pack runs the best one
the CPU can run (see [Tool builds](#tool-builds-and-the-cpu)). Only `avip` and
`shar` are required; a stream whose packer is missing (or has no build for this
CPU) goes to `shar a22`, with a note.

## What happens

**c**

1. A work directory is made (see `-w`), and `avip c input u\s` splits the
   container into `s.meta` and one file per track (`s.0.h264`, `s.1.aac`, ...;
   mkv attachments come out as files too, e.g. fonts as `s.7.ttf`). The input's
   CRC32 is computed meanwhile.
2. Each of avip's files goes to a packer, by its extension:

   | files | packer | group (`-t`/`-c` position) |
   |---|---|---|
   | `.h264` | `h264p c -t N [-c C]` | 1 |
   | `.aac` | `aacpack c`, one process per part (below) | 2 |
   | `.mp3` `.mp2` `.mp1` `.mpa` | `mp3pack c -t N [-c C]` | 2 |
   | `.ogg` `.oga` | `oggcomp c` | 2 |
   | `.ttf` `.otf` `.ttc` | `ttfcomp a`, all fonts in one solid archive | 3 |
   | everything else: `s.meta`, subtitles, ac3, flac, opus, xvid/hevc video, ... | `shar a22` | 4 |

3. `shar a0` stores the packed files and an index, `mp4pack.idx`, as the
   output.

**d** undoes it: `shar x`, the unpackers, then `avip d`. Every stream is checked
against the size and CRC32 the index has for it before `avip d`, and the result
against the input's; a mismatch is an error (exit code 4) and the output is
removed.

The tools run with the work directory as their current directory and get
relative paths. They take ANSI command lines, so a path that does not survive
the conversion to the ANSI code page (e.g. Cyrillic names on a Western system)
never reaches them: avip gets the input's 8.3 name, or a hard link to it in the
work directory (a copy as the last resort); the output of `avip d` goes to its
8.3 name, or to the work directory and is moved; `shar a0`/`shar x` write and
read the archive through a pipe.

## Options

`-t T` &mdash; threads, for the four groups in the order of the table above:
h264p, the audio packer, ttfcomp, shar. Numbers joined by `+` run at the same
time, by `,` one after the other:

| | |
|---|---|
| `-t 3+3+1+1` | all four groups at once: h264p with 3 threads, the audio packer with 3, ttfcomp and shar with 1 |
| `-t 3,3,1,1` | h264p with 3 threads; when it is done, the audio with 3; then ttfcomp; then shar |
| `-t 3+3,1,1` | h264p and audio at once, then ttfcomp, then shar |
| `-t 3+3,1+1` | h264p and audio at once, then ttfcomp and shar at once |
| `-t 3+3` | ttfcomp and shar get 1 thread each when not given, one after the other: `3+3,1,1` |
| `-t 4` | a missing audio number is the video's, after it: `4,4,1,1` |
| `-t 50%+50%-2` | with 8 threads: 8-2 = 6, half of it each: `3+3,1,1` |
| `-t -2` | the default with 2 threads left out: `6+6,1,1` with 8 threads |

Instead of a number, `N%` is that share of the threads there are, rounded up
(at least 1 unless it is `0%`). The threads there are: all of the CPU's, or the
free ones with `-f`. A `-N` at the end of `-t` leaves N of them out (for other
work) before the percentages are taken; alone, it works on the default.

Within a group, jobs run in order, as many at a time as its threads allow:
h264p and mp3pack take all of their group's threads (`-t N`), the aacpack parts
and oggcomp one each. ttfcomp is always one process with all the fonts (solid
compression), and so is the `shar a22` archive of the other files, except for
pieces of big files (`-c`).

`0` for a group means no packer for it: its files go to `shar a22` instead; `0`
for shar stores the other files as they are (so `-t 0,0,0,0` stores
everything).

Default: `100%+100%`, that is `N+N,1,1` with N threads. h264p and mp3pack take
their decompression threads from the stream, so for d, `-t` only sets how many
things run at once.

mp4pack shows what `-t` came to on its first line, e.g.
`cpu: 8 threads, 4 cores, avx2, 7 free; -t 50%+50%-2 = 3+3,1,1`.

`-c C` &mdash; part sizes, in the same order (separators as in `-t`, they make no
difference here): MB, or KB with a `k` (`g`: GB). `0` or a missing number: the
default.

- h264p and mp3pack get it as their own `-c` (only with more than one thread,
  as they take it).
- aacpack has no threads of its own, so mp4pack cuts the `.aac` stream into
  parts at ADTS frame boundaries and runs one aacpack per part, the parts
  joined again on d. With N threads: N parts of the same size (1MB at least),
  or, with `-c`, parts of at least that size. A part boundary costs ~1KB of
  compression. With 1 thread there is one part.
- ttfcomp: no part size; one solid archive.
- shar: with more than one thread, files bigger than the part size (default
  64MB) are cut into pieces, each its own `shar a22` archive, so that big
  streams without a packer (hevc video, DTS audio) can be compressed in
  parallel. The other files make one solid archive.

`-f` &mdash; count only the threads free now, for the percentages and the
defaults of `-t`: the CPUs' idle share, measured over a second when mp4pack
starts, times their number (at least 1). For when something else is running.

`-i L` &mdash; use tool builds up to level `L` only (see below), e.g. `-i avx2` on
an AVX-512 machine. Default: what the CPU has.

`-w D` &mdash; make the work directory in `D` (created if needed); default
`%TEMP%`. The work directory itself is a new `mp4pack_<pid>_<n>` in it, removed
at the end. It needs room for the unpacked streams (about the input's size) plus
the packed ones; streams are removed as soon as they are packed (or restored).

`-k` &mdash; keep the work directory (and everything in it).

`-v` &mdash; show the tools' command lines and let them write to the console
(otherwise their output goes to log files in the work directory, and the end of
a log is shown when a tool fails).

On a console, a status line shows the elapsed time, the overall progress and an
estimate of the time left, and each running job with its progress.

## Tool builds and the CPU

At the start mp4pack says what it found:

```
cpu: 16 threads, 8 cores, avx2, 12 free; -t 12+12,1,1
tools: avip_avx2.exe shar_avx2.exe h264p_avx2.exe aacpack_avx2.exe ...
```

The threads are the logical processors in all processor groups
(`GetActiveProcessorCount`), the cores are counted by
`GetLogicalProcessorInformationEx`. The instruction sets come from CPUID (and
XGETBV, for whether the OS saves the AVX/AVX-512 registers), run from a few
bytes of x64 machine code put in executable memory: no compiler intrinsics or
inline assembly. Where that is not allowed, `IsProcessorFeaturePresent` tells
it instead (older Windows versions do not know the newer features there, so it
can only err low).

A build's suffix names its level; the CPU's level is the highest whose features
it has all of:

| level | suffixes | needs |
|---|---|---|
| x64 | none (`h264p.exe`), `_x64`, `_sse2` | any x64 CPU |
| sse3 | `_sse3` | SSE3 |
| ssse3 | `_ssse3` | SSSE3 |
| sse41 | `_sse41`, `_sse4`, `_sse4.1` | SSE4.1 |
| sse42 | `_sse42`, `_sse4.2`, `_v2` | SSE4.2, POPCNT |
| avx | `_avx` | AVX, and the OS saving YMM |
| avx2 | `_avx2`, `_v3` | AVX2, FMA, BMI1, BMI2, LZCNT, MOVBE, F16C (x86-64-v3, Haswell) |
| avx512 | `_avx512`, `_v4` | AVX-512 F, CD, BW, DQ, VL, and the OS saving ZMM (x86-64-v4) |

For each tool the highest level at or below the CPU's (or `-i`'s) is run. The
archive's index lists the builds it was made with (`mp4pack d -v` shows them).
The builds of a tool are taken to be interchangeable: the CRC checks of d would
catch it if they were not.

## Errors

| exit code | |
|---|---|
| 0 | done |
| 1 | bad command line |
| 2 | a file cannot be read or written, a tool is missing (or has no build for the CPU) |
| 3 | a tool failed (the end of its log is shown) |
| 4 | d: not an mp4pack archive, or a stream or the result is not as it was |
| 5 | aborted with Ctrl+C |

When compressing, a packer that fails (or crashes) does not stop it: the
stream it had goes to `shar a22` instead, with a warning. avip and shar
failing do stop it. On Ctrl+C mp4pack stops the tools and removes the work
directory (unless `-k`). The tools also run in a job object that ends them
when mp4pack ends, however that happens.

## The archive

The output is a plain `shar a0` archive, so `shar x archive dir\` shows what is
in it, e.g. for an mkv with h264, two aac tracks, an mp3, fonts and subtitles:

```
mp4pack.idx
s.0.h264.h264p          h264p
s.1.aac.0.aacp ...      aacpack parts
s.2.mp3.mp3p, .mp3o     mp3pack (frames, and what is not frames)
fonts.ttfc              ttfcomp
other.shar              shar a22: s.meta, subtitles, ...
other1.shar ...         shar a22: pieces of big files
store\...               -t ...+0: stored as they are
```

`mp4pack.idx` is text:

```
mp4pack 1
src <size> <crc32> <the input's name>
tools <the tool builds used>
stream <size> <crc32> <method> <name>          one per file from avip c
part <offset> <length> <packed file> [<name>]  what holds it (mp3pack: the
...                                            "outside" file; shar: the name
end                                            in the archive)
```

## Building

With MinGW-w64, on Linux (`./build.sh`) or Windows:

```
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
```

clang++ works the same way (for the MSVC target `wmain` needs no `-municode`).

## Testing

`test/roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...` compresses each file with
the options under wine, decompresses it and compares. `test/parse_test.cpp`
checks the `-t`/`-c` parsing (build it like mp4pack.exe and run it).

Done here with wine 9 on Linux: the `003` mkv/mp4 test files, synthetic files
with every kind of stream (h264, aac, mp3, vorbis, ac3, opus, flac, xvid,
ASS/SRT/tx3g subtitles, ttf attachments; mkv, mp4, avi), and a whole movie;
`-t` and `-c` in sequence, in parallel and mixed, with zeros, percentages and
`-N`, with parts and pieces; packers that fail or crash (they fall back to
shar); Cyrillic paths, a new nested `-w` directory, paths over 260 characters;
damaged, truncated and foreign archives, an empty input; tool builds for
several levels, `-i` limits, builds missing for the level. Not under wine:
Ctrl+C (wine's console ends a program on it at once, without its handler).

The movie, with the defaults on a 4-thread Xeon VM under wine (the d run shared
the CPU with other tests):

| | size | time |
|---|---|---|
| TBB_04.mkv | 901,752,304 | |
| h264 track, h264p -t 4 | 648,922,836 -> 564,620,814 (87.0%) | 20:00 |
| aac tracks, aacpack in 7 parts | 253,612,715 -> 185,034,562 (73.0%) | (alongside) |
| archive | 749,662,667 (83.1%) | c 21:22, d 30:14 |
