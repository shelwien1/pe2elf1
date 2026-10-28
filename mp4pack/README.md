# mp4pack

Lossless compression of whole mp4/mkv/avi files, for Windows and Linux (see
[Linux](#linux)). `mp4pack.exe` is a wrapper: it takes the container apart with
`avip`, gives each stream to the packer made for it, and stores the results in
one archive in shar's format, written by mp4pack itself (what `shar a0` would
make). `mp4gui.exe` is the same with a window, on Windows (see
[mp4gui](#mp4gui)).

```
mp4pack c [options] input output     compress
mp4pack d [options] input output     decompress
```

The tools are looked up in the directory of `mp4pack.exe` itself, so put it next
to them: `avip`, `h264p`, `aacpack`, `mp3pack`, `oggcomp`, `ttfcomp`, `shar`. Each
can be there in builds for several CPUs, named by the gcc/clang `-march` target
they were made for: `h264p_haswell.exe`, `h264p_x86-64-v3.exe`,
`h264p_znver4.exe`, `h264p.exe` ... mp4pack runs the best one of those that run
on the CPU (see [Tool builds](#tool-builds-and-the-cpu)). Only `avip` and
`shar` are required; a stream whose packer is missing (or has no build that
runs here) goes to shar, with a note.

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

3. mp4pack stores the packed files and an index, `mp4pack.idx`, as the
   output: a Shar2 archive with them stored, the same bytes `shar a0` makes of
   them (see [The archive](#the-archive)). Each packed file is removed from
   the work directory once it is in (unless `-k`).

**d** undoes it: `shar x`, the unpackers, then `avip d`. Every stream is checked
against the size and CRC32 the index has for it before `avip d`, and the result
against the input's; a mismatch is an error (exit code 4) and the output is
removed.

The tools run with the work directory as their current directory and get
relative paths. They take ANSI command lines, so a path that does not survive
the conversion to the ANSI code page (e.g. Cyrillic names on a Western system)
never reaches them: avip gets the input's 8.3 name, or a hard link to it in the
work directory (a copy as the last resort); the output of `avip d` goes to its
8.3 name, or to the work directory and is moved; mp4pack writes the archive
itself, and `shar x` reads it through a pipe.

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
| `-t 3+3` | ttfcomp and shar get 1 thread each when not given, both at once, after the rest: `3+3,1+1` |
| `-t 4` | a missing audio number is the video's, after it: `4,4,1+1` |
| `-t 50%+50%-2` | with 8 threads: 8-2 = 6, half of it each: `3+3,1+1` |
| `-t -2` | the default with 2 threads left out: `6,6,1+1` with 8 threads |

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

Default: `100%,100%`, that is `N,N,1+1` with N threads: h264p with all of them,
then the audio with all of them, then ttfcomp and shar together, one thread
each; no thread is given out twice. h264p and mp3pack take their
decompression threads from the stream, so for d, `-t` only sets how many things
run at once.

mp4pack shows what `-t` came to on its first line, e.g.
`cpu: 8 threads, 4 cores, haswell (x86-64-v3), 7 free; -t 50%+50%-2 = 3+3,1+1`.

`-c C` &mdash; part sizes, in the same order (separators as in `-t`, they make no
difference here): MB, or KB with a `k` (`g`: GB). `0` or a missing number: the
default, a part for each thread: the stream's size over its group's threads,
rounded up, 1MB at least (for h264p and mp3pack no more than the memory
allows, below).

- h264p and mp3pack get it as their own `-c` (only with more than one thread,
  as they take it). Their memory grows with the part: per thread, h264p takes
  about 0.1GB + 1.6 times the part size, mp3pack about 9 times it. So their
  default part is at most 1GB, and at most the free physical memory, 3/4 of
  it, over the threads of the groups that run at the same time as theirs,
  less 128MB, over 2 for h264p and 10 for mp3pack. (aacpack takes 38MB and
  shar about 670MB at `a22`, whatever the part.)
- aacpack has no threads of its own, so mp4pack cuts the `.aac` stream into
  parts at ADTS frame boundaries and runs one aacpack per part, the parts
  joined again on d. With N threads: N parts of the same size (1MB at least),
  or, with `-c`, parts of at least that size. A part boundary costs ~1KB of
  compression. With 1 thread there is one part.
- ttfcomp: no part size; one solid archive.
- shar: with more than one thread, files bigger than the part size (default:
  the file's size over the threads, so a piece for each) are cut into pieces,
  each its own shar archive, so that big streams without a packer (hevc
  video, DTS audio) can be compressed in parallel. The other files make one
  solid archive.

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
none. The archive itself stores them (no level).

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

`-i T` &mdash; use only the tool builds whose code also runs on the `-march`
target `T` (see below). E.g. `-i x86-64-v3` (or `-i haswell`) on an AVX-512
machine leaves the AVX-512 builds out; `-i znver4` takes only builds that would
run on a Zen 4 too. Default: every build that runs on this CPU.

`-w D` &mdash; make the work directory in `D` (created if needed); default
`%TEMP%`. The work directory itself is a new `mp4pack_<pid>_<n>` in it, removed
at the end. It needs room for the unpacked streams (about the input's size) plus
the packed ones; streams are removed as soon as they are packed (or restored).

`-k` &mdash; keep the work directory (and everything in it).

`-v` &mdash; show the tools' command lines and let them write to the console
(otherwise their output goes to log files in the work directory, and the end of
a log is shown when a tool fails).

`-verify` &mdash; after c, decompress the archive into a temporary file beside
the work directories (`-w`) and check it as d does (each stream, and the whole
file, against the index's sizes and CRC32s); the file is removed after. It
needs room for the restored file and its streams. A failed check is exit code
4; the archive is kept.

On a console, a status line shows the elapsed time, the overall progress and an
estimate of the time left, and each running job with its progress. The jobs'
progress comes from the tools where they tell it (h264p, mp3pack and shar: the
percentages they write), from what mp4pack feeds them or takes from them
(aacpack, shar x, the archive), else from the size of what they have written so
far: exact where the size to come is known (oggcomp, ttfcomp and avip in d),
by a typical ratio in c (oggcomp about 87%, ttfcomp about a third). avip c
and ttfcomp x write only at the end, so they tell nothing. The time left is
that of the whole, but no less than what the slowest job running still needs
at its own pace (h264p is far slower by the byte than the rest).

## Tool builds and the CPU

At the start mp4pack says what it found:

```
cpu: 16 threads, 8 cores, znver4 (x86-64-v4), 12 free; -t 12,12,1+1
tools: avip_avx2.exe shar_avx2.exe h264p_znver4.exe aacpack_avx2.exe ...
```

The threads are the logical processors in all processor groups
(`GetActiveProcessorCount`), the cores are counted by
`GetLogicalProcessorInformationEx`. The CPU is read by `x86targets.h` (a copy
of the header of [cpuinfo](../cpuinfo), beside mp4pack.cpp): CPUID, and XGETBV for the registers the OS
saves. It knows gcc's and clang's `-march` targets (the names, and the features
each one lets the compilers use), works out which of them run on this CPU, and
ranks those as cpuinfo lists them. The `cpu` line names the first (here
`znver4`) and the CPU's x86-64 level.

A tool's builds are named by their targets: the tool, `_`, and a name gcc or
clang take for `-march`. E.g. `h264p_haswell.exe` (made with `-march=haswell`),
`h264p_x86-64-v3.exe`, `h264p_znver4.exe`, `h264p_skylake-avx512.exe`. These
are cpuinfo's build names, so an x86-32 build of an x64 target gets `-32`
(`h264p_haswell-32.exe`: `-m32 -march=haswell`), and the x86-32-only targets
keep their names (`h264p_pentium4.exe`). `h264p.exe`, with no target, is taken
to run anywhere. The level names of earlier mp4pack versions stand for these
targets:

| suffix | target |
|---|---|
| `_x64`, `_sse2`, `_v1` | x86-64 |
| `_sse3` | nocona |
| `_ssse3` | core2 |
| `_sse4`, `_sse41`, `_sse4.1` | penryn |
| `_sse42`, `_sse4.2`, `_v2` | x86-64-v2 |
| `_avx` | sandybridge |
| `_avx2`, `_v3` | x86-64-v3 |
| `_avx512`, `_v4` | x86-64-v4 |

A build runs here when the CPU has every feature of its target that compilers
use on their own (class C in `x86targets.h`; for a name both compilers know,
those of both). The ones only used through intrinsics (`aes`, `sha` ...) do
not count, as in cpuinfo without `-s`. Of the builds that run, the one whose
target cpuinfo ranks first is used. The ranking goes by the highest vector ISA
(AVX-512 over AVX2 ...), then the target `-march=native` names, the x86-64
level, the same tuning, more features, the CPU's vendor, newer targets. x64
builds go before x86-32 ones. A build with no target ranks as an `x86-64` one
(and, where that does not run, after all the others). When no build of a tool
runs, mp4pack says what each one needs that the CPU lacks, e.g. on a Haswell:

```
no h264p build that runs here (h264p_avx512.exe lacks avx512f avx512cd
avx512dq avx512bw avx512vl; h264p_znver5.exe lacks sse4a prfchw gfni ...)
```

The tools of `mp4pack_v0` are AVX2 builds (`x86-64-v3`), so they go in as
`<tool>_avx2.exe` (or `<tool>_x86-64-v3.exe`); in their directory, on Windows:

```
for %f in (avip aacpack h264p mp3pack oggcomp shar ttfcomp) do ren %f.exe %f_avx2.exe
```

With `-i T` a build has to run on `T` as well: the CPU's features are cut down
to `T`'s before the builds are checked. Without CPUID, only the builds with no
target are run (an x86 mp4pack always has it; this is for an mp4pack built for
another CPU, with the tools run under emulation). The archive's index lists the
builds it was made with (`mp4pack d -v` shows them). The builds of a tool are
taken to be interchangeable: the CRC checks of d would catch it if they were
not.

## mp4gui

`mp4gui.exe` is mp4pack in a window, for Windows: `mp4pack.cpp` compiled in,
run on a thread of its own instead of from a console. It goes next to the
tools, as `mp4pack.exe` does.

- **Compress** or **Decompress**: switching from one to the other swaps Input
  and Output (`x.mkv` -> `x.mkv.mpk`, then the archive back to `x.mkv`).
  **Verify** (for Compress) is `-verify`: once the archive is made, it is
  decompressed into a temporary file beside the work directories, and that is
  checked by the index's CRCs (each stream's and the whole file's), as d
  checks; the file is removed after. When the check fails, the archive is kept
  and the run ends with an error.
- **CPU Info** opens a window with what cpuinfo says of the CPU (name, threads
  and cores, memory, caches, features, what `-march=native` picks, the targets
  whose code runs here, ranked) and every build of every tool here: the one
  that runs (`*`), the others that could, and what the CPU lacks for those
  that cannot.
- **Input, Output**: typed or pasted (quotes around a path, as Explorer's "Copy
  as path" gives it, are taken off), chosen with `...` (the Open and Save
  dialogs), or the input dropped on the window, or given on mp4gui's command
  line (below). An input ending in `.mpk` selects Decompress. The output is
  named after the input (`x.mkv` -> `x.mkv.mpk`, and back) until you choose
  one; an existing output is replaced only after asking.
- **Work dir** (`-w`; empty: the temp directory): `...` chooses a folder,
  `%TEMP%` puts that in (expanded when it runs), Output dir the output's
  folder; "keep it" is `-k`.
- **Options**, as on the command line: Threads (`-t`; "keep free" adds a `-N`,
  "only currently idle threads" is `-f`), and under it what they come to, e.g.
  `= 4,4,1+1: h264p 4, then audio 4, then ttfcomp 1 + shar 1` (in red when it
  is not right); Levels (`-l`, for h264p, aacpack and shar; off for
  Decompress). The part sizes are mp4pack's defaults (a part for each thread,
  as the memory allows: `-c`), and the tool builds any that run here.
- **Start** (or Enter) runs it. The log gets mp4pack's messages, the command
  line it amounts to first. Progress shows the whole run's share done (avip
  and the archive take 3% each, packing or unpacking the rest; with Verify,
  the compression is the first half and the check the second), the time so
  far and about how much is left of the phase, and a row for each task as it
  comes: avip, each stream's packer (a stream's parts in one), ttfcomp, shar,
  the archive, the verify. `~` marks a share that is a guess (in c, by a
  typical ratio), a bar that runs to and fro a task that does not tell (see
  the status line above), a gray name one that waits. When a packer is done,
  its bar shows what the stream came to in green and what it saved in blue,
  and the gain beside it in blue: an h264 stream packed to 79.3% is
  `|green 79.3%|blue 20.7%| 20.7%`. In d the unpackers' rows show the same
  (the packed files against what they restore), and at the end the overall
  bar shows the archive against the input (c) or the output (d). A task that
  failed says so in red. (The bars are green on the classic look; with a
  visual style they are as it draws them while they run, and a finished one
  takes the green and the blue in its shading.) The taskbar button shows the
  progress too, and flashes at the end if the window is in the background.
- **Pause** holds the run until **Resume**: the tools running are suspended
  (all their threads), none starts, and the clock stops. Nothing is ended;
  Stop works while it is paused.
- **Stop** ends it as Ctrl+C does: the tools are stopped and the work
  directory is removed (unless kept); their rows say "stopped". Closing the
  window while it runs asks first, then stops it.
- The options and the window's place are kept in `mp4gui.ini` next to it.

mp4gui takes mp4pack's command line too: `mp4gui c [options] input [output]`
(or `d`) sets the mode, the options given and the files (the output named
after the input if not given), and waits for Start; `C` and `D` start it at
once, an existing output replaced without asking, as mp4pack does. E.g.
`mp4gui.exe C -t 50%+50% -verify "%1"` for a "Send To" or a file association.
The options are mp4pack's: `-t` (a `-N` at its end goes to "keep free"),
`-f`, `-l`, `-w`, `-k`, `-verify`; `-c` and `-i`, which have no fields, are
used for every run until mp4gui is closed and show on the line under
Threads; `-v` is ignored. What is not given stays as kept in mp4gui.ini. A
command line that is not right is shown, and none of it is used. Without `c`
or `d`, `mp4gui input [output]` takes the files only (as when a file is
dropped on mp4gui.exe).

The tools run without a console (`DETACHED_PROCESS`), so none opens a window;
the rest is as with mp4pack.exe, whose console output comes to the window
instead (mp4pack's `msg_hook` and `status_hook`). mp4gui.exe has no manifest
(it is one .cpp file): the themed look comes from an activation context made
from the one in shell32.dll, as programs without one do; where that fails, the
controls look classic.

## Errors

| exit code | |
|---|---|
| 0 | done |
| 1 | bad command line |
| 2 | a file cannot be read or written, a tool is missing (or has no build that runs on the CPU) |
| 3 | a tool failed (the end of its log is shown) |
| 4 | d (or `-verify`'s check): not an mp4pack archive, or a stream or the result is not as it was |
| 5 | aborted with Ctrl+C |

When compressing, a packer that fails (or crashes) does not stop it: the
stream it had goes to shar instead, with a warning. avip and shar
failing do stop it. On Ctrl+C mp4pack stops the tools and removes the work
directory (unless `-k`). The tools also run in a job object that ends them
when mp4pack ends, however that happens.

## The archive

The output is a Shar2 archive with the files stored, what `shar a0` makes
(mp4pack writes it itself), so `shar x archive dir\` or `shar l archive` shows
what is in it, e.g. for an mkv with h264, two aac tracks, an mp3, fonts and
subtitles:

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

The archive's own format, as shar 2 writes it with a0, and mp4pack the same
way, byte for byte:

```
"Shar2\n\x1A\0" "C"
each file's bytes, escaped, then 99 19 D3 FF
9A 19 D3 FF
the index, escaped (by 9A 19 D3 only), then 9A 19 D3 FF
```

Escaped: an FE goes before an FE or FF that follows 99 19 D3 or 9A 19 D3
within a file. The index has, for each entry, its parent's number (LEB128: 0
for the top, else the directory's place in the index, from 1), `F` or `D`, the
name (UTF-8) and a 0, the entries of a directory right after it, by name
ignoring case; then 0, `X` and the CRC-32C (Castagnoli) of all the files' bytes
and of the index up to the `X`, little-endian. `shar x` checks that CRC.

## Building

With MinGW-w64, on Linux (`./build.sh`) or Windows:

```
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
```

clang++ works the same way (for the MSVC target `wmain` needs no `-municode`).
`mp4pack.cpp` includes `x86targets.h`, a copy of cpuinfo's
(`../cpuinfo/x86targets.h`; copy a newer one over it), so this directory
builds on its own. For the Linux build see below.

mp4gui (`./build.sh` makes it too), plain Win32 with its common dialogs:

```
g++ -std=c++11 -O2 -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp -lcomdlg32 -lole32 -lshell32 -lgdi32
```

## Linux

The same `mp4pack.cpp` builds for Linux (x86-64, glibc), with `g++` or `clang++`:

```
./build_linux.sh        # g++ -std=c++11 -O2 -pthread -o mp4pack mp4pack.cpp
```

There `winapi_posix.h` takes the place of `<windows.h>`. It implements the Win32
functions mp4pack calls on POSIX, so the program is the same one as on Windows.
The functions cover files, pipes, processes, threads and waits, the console,
Ctrl+C and the processor counts.

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
`h264p_avx2.exe`). Apart from that line, only the output of mp3pack and h264p
with more than one thread differs (their parts come out in the order they are
done), and it differs from run to run on Windows too.

Tested on Linux as on Windows: the sample files with the `-t`/`-c`/`-l`
combinations, and archives from the Windows build decompressed by the Linux one
and the other way round. Names with spaces, quotes, `\`, Cyrillic and bytes
that are not UTF-8; paths of 389 characters; relative paths; a new nested `-w`
directory on another file system (`/dev/shm`). Tools that are missing, fail,
crash or cannot be run (they fall back to shar). Damaged and foreign archives.
SIGINT to the process group, SIGTERM, and SIGKILL (then the tools end at once
and only the work directory is left). And the movie (see [Testing](#testing)).

## Testing

`test\roundtrip.bat MP4PACK "OPTIONS" FILE...` (Windows) and
`test/roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...` (BINDIR with `mp4pack.exe`:
under wine; else the Linux `mp4pack`) compress each file with the options,
decompress it and compare, a line per file. E.g.
`test\roundtrip.bat c:\mp4pack_v0\mp4pack.exe "-t 50%+50%-1" a.mkv b.mp4`.
`test/parse_test.cpp` checks the `-t`/`-c`/`-l` parsing (build it like
mp4pack.exe and run it; on Linux `g++ -std=c++11 -pthread test/parse_test.cpp`).
`test/builds_test.cpp` (built the same way) shows which builds mp4pack would
run on other CPUs: `builds_test [-i T] TOOLDIR DUMP...` reads CPUID dumps in
cpuinfo's format (InstLatx64's `*CPUID*.txt`, or `cpuinfo -D`), and prints for
each the CPU's best target and the build of each tool in TOOLDIR that would
run there, or why none would. The builds can be empty files: only their names
count.

Done here with wine 9 on Linux: the `003` mkv/mp4 test files, synthetic files
with every kind of stream (h264, aac, mp3, vorbis, ac3, opus, flac, xvid,
ASS/SRT/tx3g subtitles, ttf attachments; mkv, mp4, avi), and a whole movie;
`-t` and `-c` in sequence, in parallel and mixed, with zeros, percentages and
`-N`, with parts and pieces; packers that fail or crash (they fall back to
shar); Cyrillic paths, a new nested `-w` directory, paths over 260 characters;
damaged, truncated and foreign archives, an empty input; tool builds named by
targets and by the old level names, `-i`, no build that runs. With
builds_test, the builds chosen for 19 InstLatx64 dumps (K8 to Zen 5, Pentium 4
and Core 2 to Sapphire Rapids, VIA Nano, Zhaoxin), with and without `-i`. Not
under wine: Ctrl+C (wine's console ends a program on it at once, without its
handler).

The archive mp4pack writes was compared with what `shar a0` makes of the same
work directory (kept with `-k`), on Linux and under wine, with packed files
and with a `store\` directory: the same bytes. The format was worked out from
shar's output; a reference writer in Python made the same bytes as shar a0 for
test directories with the escape sequences in files and across files, CRCs
forced to the marker values, more than 255 entries, empty files and
directories. The escaping in C++ was checked against a byte-at-a-time version
on 40,000 random cases cut at random places. With the same options the
archives are those the previous version made with shar a0 (but for the order
of h264p's and mp3pack's parts, which varies from run to run), and the old
archives decompress as before.

mp4gui was run under wine on a virtual X display (Xvfb), driven by a small
program sending it the messages a user's clicks and typing would, with a
screenshot at each step: compress and decompress (the archive the same as
mp4pack.exe's, the result identical), with Verify (and with an avip build
made to change a byte of what it writes in d: the check failed, as it
should), Pause (the tools got no CPU time while held) and Resume, Pause and
Stop in the archive step (a 1GB file stored), Stop during packing and while
paused, closing the window while it runs, a bad option, the replace question
(no, then yes), Cyrillic names, quoted paths pasted, the Work dir buttons,
the files swapping with the mode, the command line (c, C over an existing
output, one that is not right), the green and blue bars with wine's own
visual style and with the classic look, the settings kept in mp4gui.ini, and
no work directory or temporary file left in any case. Not on Windows itself: the
look there (Segoe UI, the themed controls) and the taskbar's progress were not
seen.

The movie, with `-t 4+4,1,1` (the default then) on a 4-thread Xeon VM under
wine:

| | size | time |
|---|---|---|
| TBB_04.mkv | 901,752,304 | |
| h264 track, h264p -t 4 | 648,922,836 -> 564,620,814 (87.0%) | 15:18 |
| aac tracks, aacpack in 8 parts | 253,612,715 -> 185,021,554 (73.0%) | alongside |
| archive | 749,649,784 (83.1%) | c 16:32, d 23:57 |

The same on Linux, on the same VM, with the Linux mp4pack and the tools
converted by pe2elf. The streams pack to the same sizes. The archive is
749,649,768 bytes, 16 smaller only because its index names the builds without
`.exe`. c took 17:40 (h264p 17:19), d 26:31 (h264p 26:15); the result was
identical to the input. The Windows build under wine decompressed the Linux
archive to an identical file too.
