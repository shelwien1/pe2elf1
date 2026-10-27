# mp4pack

Lossless compression of whole mp4/mkv/avi files, for Windows and Linux (see
[Linux](#linux)). `mp4pack.exe` is a wrapper: it takes the container apart with
`avip`, gives each stream to the packer made for it, and stores the results in
one `shar a0` archive.

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
CPU) goes to shar, with a note.

## What happens

**c**

1. A work directory is made (see `-w`), and `avip c input u\s` splits the
   container into `s.meta` and one file per track (`s.0.h264`, `s.1.aac`, ...;
   mkv attachments come out as files too, e.g. fonts as `s.7.ttf`). The input's
   CRC32 is computed meanwhile.
2. Each of avip's files goes to a packer, by its extension:

   | files | packer | group (`-t`/`-c`/`-l` position) |
   |---|---|---|
   | `.h264` | `h264p c [-L] -t N [-c C]` | 1 |
   | `.aac` | `aacpack c [-L]`, one process per part (below) | 2 |
   | `.mp3` `.mp2` `.mp1` `.mpa` | `mp3pack c -t N [-c C]` | 2 |
   | `.ogg` `.oga` | `oggcomp c` | 2 |
   | `.ttf` `.otf` `.ttc` | `ttfcomp a`, all fonts in one solid archive | 3 |
   | everything else: `s.meta`, subtitles, ac3, flac, opus, xvid/hevc video, ... | `shar a22` (`aL` with `-l`) | 4 |

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
compression), and so is the shar archive of the other files, except for
pieces of big files (`-c`).

`0` for a group means no packer for it: its files go to shar instead; `0`
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
  64MB) are cut into pieces, each its own shar archive, so that big
  streams without a packer (hevc video, DTS audio) can be compressed in
  parallel. The other files make one solid archive.

`-l L` &mdash; compression levels, in the same order (separators as in `-t`,
they make no difference here). A missing or empty one: the tool's default.

| group | packer | levels | default |
|---|---|---|---|
| 1 | h264p | `-N`, 1 to 9, the ones its build has: `mp4pack_v0`'s has 9 7 5 3 1, from the strongest to the fastest | 9 |
| 2 | aacpack | `-N`, 1 to 9 as with h264p; `mp4pack_v0`'s has only 9 | 9 |
| 2 | mp3pack, oggcomp | none | |
| 3 | ttfcomp | none | |
| 4 | shar | `aN`: zstd's levels, -131072 to 22. 1 to 19 as usual, 20 to 22 the "ultra" ones (more memory), below 0 the fast ones; 0 stores | 22 |

E.g. `-l 5,9,,19`: `h264p c -5`, `aacpack c -9`, `shar a19`; ttfcomp takes no
level, so its place is left empty (a number there would not be used, nor one
for the audio when the audio is mp3 or vorbis). The levels are for c; d needs
none. The final `shar a0` stays a0.

Which levels h264p and aacpack have depends on the build, so mp4pack asks it
before using one: after `avip c`, for the packers the streams need, it runs the
tool on a file that is not there with the level (`h264p c -4 mp4pack.none
...`); a build without the level says so (exit code 2) before it looks for its
input. Then mp4pack stops (exit code 1) and names the levels the build has:
`error: h264p_avx2.exe has no level 4 (its levels: 1 3 5 7 9); see -l`.

On a 5-second h264 sample (304,970 bytes), h264p's levels on one thread came to:

| | -9 | -7 | -5 | -3 | -1 |
|---|---|---|---|---|---|
| size | 76.8% | 77.8% | 80.7% | 81.8% | 84.4% |
| time | 1.08 s | 0.88 s | 0.58 s | 0.55 s | 0.30 s |

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

The tools of `mp4pack_v0` are AVX2 builds, so they go in as `<tool>_avx2.exe`;
in their directory, on Windows:

```
for %f in (avip aacpack h264p mp3pack oggcomp shar ttfcomp) do ren %f.exe %f_avx2.exe
```

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
stream it had goes to shar instead, with a warning. avip and shar
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
other.shar              shar a22 (or -l's level): s.meta, subtitles, ...
other1.shar ...         the same: pieces of big files
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
For the Linux build see below.

## Linux

The same `mp4pack.cpp` builds for Linux (x86-64, glibc), with `g++` or `clang++`:

```
./build_linux.sh        # g++ -std=c++11 -O2 -pthread -o mp4pack mp4pack.cpp
```

There `winapi_posix.h` takes the place of `<windows.h>`. It implements the Win32
functions mp4pack calls on POSIX, so the program is the same one as on Windows.
The functions cover files, pipes, processes, threads and waits, the console,
Ctrl+C and the CPU.

The tools are the Windows ones, converted to Linux programs by
[pe2elf](https://github.com/shelwien1/pe2elf). They run with its
`winapi_shim.so`, loaded from their own directory. This needs the fixes in
[shelwien1/pe2elf#28](https://github.com/shelwien1/pe2elf/pull/28) (until that
is merged, its branch `claude/mp4pack-tools`). `tools_linux.sh PE2ELF_DIR
TOOLS_DIR [OUT_DIR]` converts each `<tool>.exe` in TOOLS_DIR to
`OUT_DIR/<tool>` and puts `winapi_shim.so` beside them:

```
git clone -b claude/mp4pack-tools https://github.com/shelwien1/pe2elf
make -C pe2elf
./tools_linux.sh pe2elf mp4pack_v0 bin   # mp4pack_v0/h264p_avx2.exe -> bin/h264p_avx2 ...
cp mp4pack bin/
bin/mp4pack c movie.mkv movie.mpk
bin/mp4pack d movie.mpk movie2.mkv
```

The tools are looked up as on Windows, without the `.exe`: `h264p_avx2`,
`h264p` ... (rename the `mp4pack_v0` builds to `<tool>_avx2.exe` first, as for
Windows). Everything else works as described above, with these differences:

- The work directory goes in `$TMPDIR`, or `/tmp`.
- Any file name works, also one that is not UTF-8. The tools take UTF-8 names
  and treat `\` as `/`. So a name with a `\` or with bytes that are not UTF-8
  never reaches them. The input is then given to avip as a link in the work
  directory (a hard link, or a symbolic one across file systems). The output is
  written in the work directory and then moved.
- Ctrl+C, SIGTERM, SIGHUP and SIGQUIT stop mp4pack as Ctrl+C does on Windows:
  the tools are stopped, the work directory is removed, and the exit code is 5.
  The tools die with mp4pack however it ends (`PR_SET_PDEATHSIG`, in place of
  the job object).
- The threads are the processors mp4pack may run on (its affinity, e.g. with
  `taskset`); the cores are counted from sysfs, and `-f` reads `/proc/stat`.
- A tool killed by a signal shows as exit code 128+N (e.g. 139 for SIGSEGV).

The archives are the same on both systems: an archive made on one decompresses
on the other. The index's `tools` line lists the builds (`h264p_avx2` or
`h264p_avx2.exe`). Apart from that line, only mp3pack's output with more than
one thread differs, and it differs from run to run on Windows too.

Tested on Linux as on Windows: the sample files with the `-t`/`-c`/`-l`
combinations, and archives from the Windows build decompressed by the Linux one
and the other way round. Names with spaces, quotes, `\`, Cyrillic and bytes
that are not UTF-8; paths of 389 characters; relative paths; a new nested `-w`
directory on another file system (`/dev/shm`). Tools that are missing, fail,
crash or cannot be run (they fall back to shar). Damaged and foreign archives.
SIGINT to the process group, SIGTERM, and SIGKILL (then the tools end at once
and only the work directory is left). And the movie, below.

## Testing

`test\roundtrip.bat MP4PACK "OPTIONS" FILE...` (Windows) and
`test/roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...` (BINDIR with `mp4pack.exe`:
under wine; else the Linux `mp4pack`) compress each file with the options,
decompress it and compare, a line per file. E.g.
`test\roundtrip.bat c:\mp4pack_v0\mp4pack.exe "-t 50%+50%-1" a.mkv b.mp4`.
`test/parse_test.cpp` checks the `-t`/`-c`/`-l` parsing (build it like
mp4pack.exe and run it; on Linux `g++ -std=c++11 -pthread test/parse_test.cpp`).

Done here with wine 9 on Linux: the `003` mkv/mp4 test files, synthetic files
with every kind of stream (h264, aac, mp3, vorbis, ac3, opus, flac, xvid,
ASS/SRT/tx3g subtitles, ttf attachments; mkv, mp4, avi), and a whole movie;
`-t` and `-c` in sequence, in parallel and mixed, with zeros, percentages and
`-N`, with parts and pieces; packers that fail or crash (they fall back to
shar); Cyrillic paths, a new nested `-w` directory, paths over 260 characters;
damaged, truncated and foreign archives, an empty input; tool builds for
several levels, `-i` limits, builds missing for the level. Not under wine:
Ctrl+C (wine's console ends a program on it at once, without its handler).

The movie, with the defaults (`-t 4+4,1,1`) on a 4-thread Xeon VM under wine:

| | size | time |
|---|---|---|
| TBB_04.mkv | 901,752,304 | |
| h264 track, h264p -t 4 | 648,922,836 -> 564,620,814 (87.0%) | 15:18 |
| aac tracks, aacpack in 8 parts | 253,612,715 -> 185,021,554 (73.0%) | alongside |
| archive | 749,649,784 (83.1%) | c 16:32, d 23:57 |
