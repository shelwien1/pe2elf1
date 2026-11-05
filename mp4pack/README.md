# mp4pack

Lossless compression of whole mp4/mkv/avi files, for Windows and Linux (see
[Linux](#linux)). `mp4pack.exe` is a wrapper: it takes the container apart with
`avip`, gives each stream to the packer made for it, and stores the results in
one simple archive of its own (see [The archive](#the-archive)). `mp4gui.exe`
is the same with a window, on Windows (see [mp4gui](#mp4gui)).

```
mp4pack c [options] input output     compress
mp4pack d [options] input output     decompress
```

The input is a file, or `@list`: a text file with a file on each line (see
[Lists](#lists)). The output is a file, or a directory: one that ends with `\`
or `/`, or one that is there.

The tools are looked up in the directory of `mp4pack.exe` itself, so put it
next to them: `avip`, `h264p`, `m4vpack`, `aacpack`, `ac3pack`, `mp3pack`,
`oggcomp`, `ttfcomp`, `tangelo`. Each can be there in builds for several CPUs,
named by the gcc/clang `-march` target they were made for: `h264p_haswell.exe`,
`h264p_x86-64-v3.exe`, `h264p_znver4.exe`, `h264p.exe` ... mp4pack runs the
best one of those that run on the CPU (see [Tool
builds](#tool-builds-and-the-cpu)). Only `avip` and `tangelo` are required; a
stream whose packer is missing (or has no build that runs here) goes to
tangelo, with a note.

## What happens

Before anything else, c and d make the output's directory if it is not there
(and say so), and try the output: a file that is there is opened for writing
(and left as it is), else one is made and removed. An output that cannot be
written (on a drive that is not there, without permission, held open by
another program) ends the run at once, exit code 2, with the system's reason,
not after all the work.

**c**

1. A work directory is made (see `-w`), and `avip c input u\s` splits the
   container into `s.meta` and one file per track (`s.0.h264`, `s.1.aac`, ...;
   mkv attachments come out as files too, e.g. fonts as `s.7.ttf`). The input's
   CRC32 is computed meanwhile.
2. Each of avip's files goes to a packer, by its extension:

   | files | packer | group (`-t`/`-c`/`-l` position) |
   |---|---|---|
   | `.h264` | `h264p c [-L] -t N [-c C]` | 1 |
   | `.m4v` (MPEG-4 Part 2: XviD, DivX) | `m4vpack c [-L] -t N [-c C]` | 1 |
   | `.aac` | `aacpack c [-L] -t N [-c C]` | 2 |
   | `.ac3` `.eac3` (Dolby Digital, and Plus) | `ac3pack c [-L] -t N [-c C]` | 2 |
   | `.mp3` `.mp2` `.mp1` `.mpa` | `mp3pack c -t N [-c C]` | 2 |
   | `.ogg` `.oga` | `oggcomp c [-L] -t N [-c C]` | 2 |
   | `.ttf` `.otf` `.ttc` | `ttfcomp a`, all fonts in one solid archive | 3 |
   | everything else: `s.meta`, subtitles, flac, opus, dts, hevc video, ... | `tangelo a [-L]`, all in one solid archive | 4 |

   tangelo, a context-mixing compressor, packs the other files smaller than
   shar's zstd -22 did (by 1% to 28% in the samples here), but slowly: about
   0.14MB/s for each process on the Xeon VM here (2.1GHz, under wine and on
   Linux alike), and the same in d. The 196,741 bytes of other files of a
   sample (ac3, two subtitles, `s.meta`) came to 159,562 (`shar a22`:
   161,577), a 3.7MB font to 1,215,439 in 26 s (`shar a22`: 1,684,315 in
   2 s). A big stream without a packer (hevc video, DTS audio) takes long:
   give tangelo more threads, and its pieces run in parallel (`-c`).

   m4vpack and ac3pack are for the streams of MPEG-4 Part 2 video (XviD,
   DivX, mp4v) and of Dolby Digital (AC-3, E-AC-3) that tangelo had before.
   2 minutes of a movie made into XviD (33,269,652 bytes) came to 23,875,448
   (71.8%) in 86 s on one thread, 24 s on 4 (23,950,196), the AC-3 track
   with it (5,760,000) to 5,096,690 (88.5%) in 6 s, the same as E-AC-3 to
   5,124,561; tangelo had made 927,282 of a 1,035,350-byte XviD stream that
   m4vpack makes 640,897.

   aacpack and oggcomp have threads and levels in their newer builds
   (`aacpack_avx2.exe`, `oggcomp_avx2.exe`): each cuts its stream into parts
   and codes them on its threads itself, so a stream is one process
   (aacpack's earlier builds had no threads, and mp4pack cut its streams at
   ADTS frames, a process for each part). On the 4-thread VM here, a 19.5MB
   AAC track came to 16,482,978 in 12 s on 4 threads (16,509,900 in 40 s on
   one), the same in d; a 9.5MB Vorbis one to 7,449,297 in 4.3 s on 4
   (7,444,382 in 15 s on one). Their streams are not the earlier builds'
   (see [The archive](#the-archive)).

3. mp4pack stores the packed files and an index, `mp4pack.idx`, as the
   output, one after another in its own archive (see
   [The archive](#the-archive)). Each packed file is removed from the work
   directory once it is in (unless `-k`).

**d** undoes it: the archive's files into the work directory, the unpackers,
then `avip d`. Every stream is checked
against the size and CRC32 the index has for it before `avip d`, and the result
against the input's; a mismatch is an error (exit code 4) and the output is
removed.

The tools run with the work directory as their current directory and get
relative paths. They take ANSI command lines, so a path that does not survive
the conversion to the ANSI code page (e.g. Cyrillic names on a Western system)
never reaches them: avip gets the input's 8.3 name, or a hard link to it in the
work directory (a copy as the last resort); the output of `avip d` goes to its
8.3 name, or to the work directory and is moved; the archive mp4pack writes
and reads itself.

## Options

`-t T` &mdash; threads, for the four groups in the order of the table above:
the video packer, the audio packer, ttfcomp, tangelo. Numbers joined by `+` run
at the same time, by `,` one after the other:

| | |
|---|---|
| `-t 3+3+1+1` | all four groups at once: the video packer with 3 threads, the audio packer with 3, ttfcomp and tangelo with 1 |
| `-t 3,3,1,1` | the video with 3 threads; when it is done, the audio with 3; then ttfcomp; then tangelo |
| `-t 3+3,1,1` | video and audio at once, then ttfcomp, then tangelo |
| `-t 3+3,1+1` | video and audio at once, then ttfcomp and tangelo at once |
| `-t 3+3` | ttfcomp and tangelo get 1 thread each when not given, both at once, after the rest: `3+3,1+1` |
| `-t 4` | a missing audio number is the video's, after it: `4,4,1+1` |
| `-t 50%+50%-2` | with 8 threads: 8-2 = 6, half of it each: `3+3,1+1` |
| `-t -2` | the default with 2 threads left out: `6,6,1+1` with 8 threads |

Instead of a number, `N%` is that share of the threads there are, rounded up
(at least 1 unless it is `0%`). The threads there are: all of the CPU's, or the
free ones with `-f`. A `-N` at the end of `-t` leaves N of them out (for other
work) before the percentages are taken; alone, it works on the default.

Within a group, jobs run in order, as many at a time as its threads allow:
h264p, m4vpack, ac3pack and mp3pack take all of their group's threads (`-t N`),
aacpack and oggcomp as many as the stream has parts (see `-c`), so a small
track takes fewer and leaves the rest to the next. ttfcomp is always one
process with all the fonts (solid compression), and so is the tangelo archive
of the other files, except for pieces of big files (`-c`).

`0` for a group means no packer for it: its files go to tangelo instead; `0`
for tangelo stores the other files as they are (so `-t 0,0,0,0` stores
everything).

Default: `100%,100%`, that is `N,N,1+1` with N threads: the video with all of
them, then the audio with all of them, then ttfcomp and tangelo together, one
thread each; no thread is given out twice. The packers take their
decompression threads from the stream, so for d, `-t` only sets how many things
run at once.

mp4pack shows what `-t` came to on its first line, e.g.
`cpu: 8 threads, 4 cores, haswell (x86-64-v3), 7 free; -t 50%+50%-2 = 3+3,1+1`.

`-c C` &mdash; part sizes, in the same order (separators as in `-t`, they make no
difference here): MB, or KB with a `k` (`g`: GB). `0` or a missing number: the
default, a part for each thread: the stream's size over its group's threads,
rounded up, 1MB at least (for h264p, m4vpack, ac3pack and mp3pack no more than
the memory allows, below); aacpack's and oggcomp's own (below).

- h264p, m4vpack, ac3pack and mp3pack get it as their own `-c` (only with more
  than one thread, as they take it). Their memory grows with the part: per
  thread, h264p takes about 0.1GB + 1.6 times the part size, m4vpack about
  37MB + 1.3 times it, ac3pack about 26MB and a little more, mp3pack about 9
  times it. So their default part is at most 1GB, and at most the free
  physical memory, 3/4 of it, over the threads of the groups that run at the
  same time as theirs, less 128MB, over 2 (10 for mp3pack). (tangelo takes
  its level's model, 1164MB at -9, whatever the part.)
- aacpack and oggcomp cut the stream into parts themselves: aacpack into
  parts of 1MB at least, a model for each, oggcomp into four for each
  thread, of 64KB to 4MB, each thread with a model of its own. They get `-c`
  only when it is given (as their `-c`: parts of at least that size), as
  their own sizes beat bigger parts here. mp4pack gives them as many threads
  as the stream has parts (its size over the part size: 1MB for aacpack,
  64KB for oggcomp, or `-c`'s), up to the group's: a 3.6MB AAC track gets 3,
  a 900KB one 1. They take about 35MB (aacpack) and 55MB (oggcomp) for each
  thread, whatever the part.
- ttfcomp: no part size; one solid archive.
- tangelo: with more than one thread, files bigger than the part size
  (default: the file's size over the threads, so a piece for each) are cut
  into pieces, each its own tangelo archive, so that big streams without a
  packer (hevc video, DTS audio) can be compressed in parallel. The other
  files make one solid archive.

`-l L` &mdash; compression levels, in the same order (separators as in `-t`,
they make no difference here). A missing or empty one: the tool's default.

| group | packer | levels | default |
|---|---|---|---|
| 1 | h264p | `-N`, 1 to 9, the ones its build has: `mp4pack_v0`'s has 9 7 5 3 1, from the strongest to the fastest | 9 |
| 1 | m4vpack | `-N` as h264p: `mp4pack_v1`'s has 9 7 5 3 1 | 9 |
| 2 | aacpack | `-N`, 1 to 9 as with h264p: the builds with threads have 9 7 5 3 1 (`mp4pack_v0`'s had only 9) | 9 |
| 2 | ac3pack | `-N` as h264p: `mp4pack_v1`'s has 9 5 1 | 9 |
| 2 | oggcomp | `-N` as h264p: the builds with threads have 9 7 3 1 (7 is nearly as strong as 9, and the fastest) | 9 |
| 2 | mp3pack | none | |
| 3 | ttfcomp | none | |
| 4 | tangelo | `-N`, 1 to 9: its model takes 17, 21, 30, 48, 84, 156, 300, 588 and 1164MB (and a 16MB window), for each process, also in d | 9 |

E.g. `-l 5,9,,7`: `h264p c -5` (or `m4vpack c -5`), `aacpack c -9` (or `ac3pack
c -9`, `oggcomp c -9`), `tangelo a -7`; ttfcomp takes no level, so its place is
left empty (a number there would not be used, nor one for the audio when the
audio is mp3). The levels are for c; d needs none. The archive itself stores
them (no level).

Which levels a packer has depends on the build, so mp4pack asks it before using
one: after `avip c`, for the packers the streams need, it runs the tool on an
empty file in the work directory, without the level and with it
(`h264p c -4 -t 1 mp4pack.none mp4pack.none.out`). A build packs it (exit code
0) when it takes the switches, and refuses a level it does not have. Then
mp4pack stops (exit code 1) and names the levels the build has:
`error: h264p_avx2.exe has no level 4 (its levels: 1 3 5 7 9); see -l`.
aacpack and oggcomp are tried so (without a level) in d too, and in c also
without `-l`: a build of theirs from before their threads does not take `-t`,
and cannot unpack what the later ones packed. mp4pack stops then (exit code 2)
before any work: `error: oggcomp_x86-64-v4.exe does not take -t: an earlier
build?` (see [Tool builds](#tool-builds-and-the-cpu)).

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
progress comes from the tools where they tell it (h264p, m4vpack, aacpack,
ac3pack, mp3pack, oggcomp and tangelo: the percentages they write), from what
mp4pack does itself (the archive, its extraction), else from the size of what
they have written so far: exact where the size to come is known (ttfcomp and
avip in d), by a typical ratio in c (ttfcomp about a third). avip c and
ttfcomp x write only at the end, so they tell nothing. The time left is that
of the whole, but no less than what the slowest job running still needs at its
own pace (h264p is far slower by the byte than the rest).

## Lists

`mp4pack c @list.txt out\` compresses each file of the list in turn, each
into its own archive in `out\` (made if needed), named after it:
`x.mkv` -> `out\x.mkv.mpk`. `mp4pack d @archives.txt out\` does the same
backwards: `x.mkv.mpk` -> `out\x.mkv`. The same works for one file:
`mp4pack c x.mkv out\`. Two files of one name (from two directories) do not
write over each other: the second becomes `x (2).mkv.mpk`.

`mp4pack c @list.txt all.mpk` (an output that is not a directory) makes one
archive of all of them, a member for each, one after another (see
[The archive](#the-archive)). `mp4pack d all.mpk out` restores each file under
the name it had, in the directory `out` (made if needed; with no separator at
its end too, as there is more than one file); an archive of one file goes to
the output as it is named, as ever.

The list: a file on each line, as it would be on the command line (relative
names are from the current directory). UTF-8, or UTF-16 with its BOM
(PowerShell's `>`); else, on Windows, the ANSI code page, or the OEM one for
a name that only that finds (`dir /b > list.txt` in a console writes OEM).
Blank lines are left out, and spaces and quotes around a name.

A file that fails is told (with why) and left; the rest go on, and a line at
the end says how it all went: `5 files: 4 done, 1 not (y.mkv); 1246023 ->
688677 (55.27%), 0:00:04`. The exit code is then the first that failed's (5
after Ctrl+C). An output that cannot be written (a directory that cannot be
made, a full disk) stops the list: the files after it are not tried. An
archive member is only added when its file went well, and one that fails
while it is written is taken off again, so the archive holds the files that
went well. `-verify` checks each member as soon as it is written.

## Tool builds and the CPU

At the start mp4pack says what it found:

```
cpu: 16 threads, 8 cores, znver4 (x86-64-v4), 12 free; -t 12,12,1+1
tools: avip_avx2.exe tangelo_avx2.exe h264p_znver4.exe aacpack_avx2.exe ...
```

The threads are the logical processors in all processor groups
(`GetActiveProcessorCount`), the cores are counted by
`GetLogicalProcessorInformationEx`. The CPU is read by `x86targets.h` (a copy
of the header of [cpuinfo](../cpuinfo), beside mp4pack.cpp): CPUID, and XGETBV
for the registers the OS saves. It knows gcc's and clang's `-march` targets
(the names, and the features each one lets the compilers use), works out which
of them run on this CPU, and ranks those as cpuinfo lists them. The `cpu` line
names the first (here `znver4`) and the CPU's x86-64 level.

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
for %f in (avip aacpack h264p mp3pack oggcomp ttfcomp) do ren %f.exe %f_avx2.exe
```

`mp4pack_v1` adds `ac3pack.exe` and `m4vpack.exe` (named with no target: taken
to run anywhere) and a new `avip_avx2.exe`, which takes the old one's place;
they go in the same directory.

The aacpack and oggcomp with threads and levels (`aacpack_avx2.exe`,
`oggcomp_avx2.exe`) take the place of the earlier ones, which mp4pack no
longer runs: remove those. A build with a target ranked higher stays in front
of them else (an `oggcomp_x86-64-v4.exe` of before, on an AVX-512 CPU), and
mp4pack stops on it (see `-l`). An archive of an earlier mp4pack whose
streams they packed needs that mp4pack and its tools (see
[The archive](#the-archive)).

With `-i T` a build has to run on `T` as well: the CPU's features are cut down
to `T`'s before the builds are checked. Without CPUID, only the builds with no
target are run (an x86 mp4pack always has it; this is for an mp4pack built for
another CPU, with the tools run under emulation). The archive's index lists the
builds it was made with (`mp4pack d -v` shows them). The builds of a tool are
taken to be interchangeable: the CRC checks of d would catch it if they were
not. (tangelo checks it itself: it refuses an archive made by a build with
other model switches.)

## mp4gui

`mp4gui.exe` is mp4pack in a window, for Windows: mp4pack's engine compiled in,
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
- **List**: files to do one after another, as mp4pack's `@list`. The row
  shows how many there are and how far it got (`5 files: 2 done, 1
  failed`); a click on it shows them all, the next to be done (or the one
  running) highlighted, those done in gray, those that failed in red with
  "(failed)". `...` adds files (the Open dialog, more at once; they come
  sorted by name), and so does dropping more than one file on the window
  (one goes to Input, unless there is a list); Clear takes them all away.
  Adding files sets the mode when they say it (all archives: Decompress,
  none: Compress), and the output, unless you chose one: the directory of
  the first file, an output of each beside it. The output is as mp4pack's:
  a directory (ending with `\`) for an output of each file, or a file for
  one archive of them all (d: the files under the names they had). With a
  list, Start does the files not done yet, in order (when all are done, all
  of them again), each in Input as it comes, its progress in the rows
  below, and at the end says how it all went. One that fails is marked and
  left, the rest go on. Stopped, the one running is left to do: Start goes
  on from it, and into one archive it adds the rest to the one made so far.
  Before it starts, the outputs it would replace are asked about (all in
  one question).
- **Input, Output**: typed or pasted (quotes around a path, as Explorer's "Copy
  as path" gives it, are taken off), chosen with `...` (the Open and Save
  dialogs), or the input dropped on the window, or given on mp4gui's command
  line (below). An input ending in `.mpk` selects Decompress. The output is
  named after the input (`x.mkv` -> `x.mkv.mpk`, and back) until you choose
  one; an existing output is replaced only after asking.
- **Work dir** (`-w`; empty: the temp directory): `...` chooses a folder,
  `%TEMP%` puts that in (expanded when it runs), Output dir the output's
  folder; "keep it" is `-k`.
- **Options**, as on the command line: Threads (`-t`; "keep free" adds a
  `-N`, "only currently idle threads" is `-f`), and under it what they come to,
  e.g. `= 4,4,1+1: video 4, then audio 4, then ttfcomp 1 + tangelo 1` (in red
  when it is not right); Levels (`-l`, for the video and audio packers and
  tangelo; off for Decompress). The part sizes are mp4pack's defaults (a part
  for each thread, as the memory allows: `-c`), and the tool builds any that
  run here.
- **Container**: what the input holds. A moment after the input is set, avip
  describes it (`avip <file>`, its info mode: nothing is written): the
  container and its size, each track (codec, frames, bytes, and further right
  avip's own details of it), the attachments, then the files `avip c` would
  make, each with what would pack it as the options stand (h264p, tangelo,
  ...; e.g. `tangelo (no h264p build runs here)`), and last avip's notes on
  the container's layout. An mp4pack archive (avip cannot read those) is
  described by its index instead: the file it was made of (name, size,
  CRC32), each stream with its size, what it came to and its method (the
  fonts in `fonts.ttfc`, the files in a tangelo archive, together), and the
  tool builds it was made with. avip runs in the background (a 1GB mkv took
  27 s here), at a lower priority; a new input stops it. Start stops it too,
  as it would compete with the run for the file, and the description is made
  when the run ends. The last four are kept, so switching between Compress
  and Decompress shows them at once. The box has no scroll bars and no
  caret: hold the left mouse button on it and move the mouse, and the text
  follows it (by lines and characters, up to its ends). The wheel scrolls it
  too, and so do the keys when it has the focus: the arrows, Page Up/Down,
  Home and End (with Ctrl: the top and the bottom). Ctrl+A selects all of
  it, Ctrl+C (or the right-click menu) copies it, a click ends the
  selection.
- **Start** (or Enter) runs it. The log gets mp4pack's messages, the command
  line it amounts to first. Progress shows the whole run's share done (avip
  and the archive take 3% each, packing or unpacking the rest; with Verify,
  the compression is the first half and the check the second), the time so
  far and about how much is left of the phase, and a row for each task as it
  comes: avip, each stream's packer (a stream's parts in one), ttfcomp, tangelo,
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
  When a run fails, the status line says what went wrong, mp4pack's message:
  `Failed: cannot write D:\x\a.mkv.mpk: There is not enough space on the
  disk`. When that does not fit, its middle is cut out (`Failed: cannot write
  D:...a.mkv.mpk: There is not enough space on the disk`), so that the
  reason at its end shows; the log has all of it.
- **Pause** holds the run until **Resume**: the tools running are suspended
  (all their threads), none starts, and the clock stops. Nothing is ended;
  Stop works while it is paused.
- **Stop** ends it as Ctrl+C does: the tools are stopped and the work
  directory is removed (unless kept); their rows say "stopped". Closing the
  window while it runs asks first, then stops it.
- The options and the window's place are kept in `mp4gui.ini` next to it.

mp4gui takes mp4pack's command line too: `mp4gui c [options] input [output]`
(or `d`) sets the mode, the options given and the files (the output named
after the input if not given; an input `@list` fills the List), and waits
for Start; `C` and `D` start it at once. The options are mp4pack's: `-t` (a `-N` at its end goes to "keep
free"), `-f`, `-l`, `-w`, `-k`, `-verify`; `-c` and `-i`, which have no
fields, are used for every run until mp4gui is closed and show on the line
under Threads; `-v` is ignored. Two more are mp4gui's own:

- `-quit`: mp4gui quits when a run ends well (the one `C` or `D` starts, or
  with `c` or `d` the one Start starts), with `-verify` after its check. When
  the run fails or is stopped, the window stays, with the log.
- `-overwrite`: an existing output is replaced without asking (else mp4gui
  asks first, with `C` and `D` too).

E.g. `mp4gui.exe C -verify -quit -overwrite "%1"` for a "Send To" or a file
association. mp4gui's exit code is its last run's, as mp4pack's (0: done), for
`start /wait mp4gui ...` in a batch file. What is not given stays as kept in
mp4gui.ini. A command line that is not right is shown, and none of it is
used. Without `c` or `d`, `mp4gui input [output]` takes the files only (as
when a file is dropped on mp4gui.exe).

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
| 2 | a file cannot be read or written, a tool is missing (or has no build that runs on the CPU, or an aacpack or oggcomp build from before their threads is in front; or the archive is an earlier mp4pack's with their streams) |
| 3 | a tool failed (the end of its log is shown) |
| 4 | d (or `-verify`'s check): not an mp4pack archive (or one of another version, or not all of one), or a stream or the result is not as it was |
| 5 | aborted with Ctrl+C |

A message of a file that cannot be read or written, a directory that cannot
be made or a file that cannot be moved ends with the system's reason: `cannot
write D:\x\a.mkv.mpk: There is not enough space on the disk` (in the system's
language; on Linux strerror's: `No space left on device`). When a directory
cannot be made because a file is in the way, the message names that file.

When compressing, a packer that fails (or crashes) does not stop it: the
stream it had goes to tangelo instead, with a warning. avip and tangelo
failing do stop it. On Ctrl+C mp4pack stops the tools and removes the work
directory (unless `-k`). The tools also run in a job object that ends them
when mp4pack ends, however that happens.

## The archive

The output is mp4pack's own archive: one or more members, one after another,
a member for each file packed into it (more than one only from a list, see
[Lists](#lists)). A member is the packed files and the index, stored as they
are, one after another (the numbers little-endian):

```
"mp4pack"           7 bytes
version             1 byte: 2 (1: of an archive that may have one member only; read too)
number of files     4 bytes
then for each file:
  name length       2 bytes
  name              UTF-8, / between directories (store/s.0.h264)
  size              8 bytes
  data              size bytes
```

`mp4pack.idx` comes first, then the others by name. E.g. for an mkv with h264,
two aac tracks, an mp3, fonts and subtitles:

```
mp4pack.idx
fonts.ttfc              ttfcomp
other.tgl               tangelo (-9, or -l's level): s.meta, subtitles, ...
other1.tgl ...          the same: pieces of big files
fallback.tgl            the same: streams whose packer failed
s.0.h264.h264p          h264p (s.0.m4v.m4vp: m4vpack)
s.1.aac.aacp            aacpack
s.2.mp3.mp3o, .mp3p     mp3pack (what is not frames, and the frames)
s.3.ac3.ac3p            ac3pack (.eac3: s.3.eac3.ac3p)
s.4.ogg.oggc            oggcomp
store/...               -t ...+0: stored as they are
```

d takes an archive only with the signature and these versions, all of each
member and nothing after the last (else it restores the members whole before
what is wrong, and stops with exit code 4), every name a plain one (no `..`,
no root or drive) and each once in its member; else it stops with exit code
4. `mp4pack d -k` keeps the work directory, with the member's files in its
`p\`.

`mp4pack.idx` is text:

```
mp4pack 2
src <size> <crc32> <the input's name>
tools <the tool builds used>
stream <size> <crc32> <method> <name>          one per file from avip c
part <offset> <length> <packed file> [<name>]  what holds it (mp3pack: the
...                                            "outside" file; tangelo: the name
end                                            in the archive)
```

`mp4pack 1` is the index of mp4pack before aacpack and oggcomp had threads.
Its aacpack and oggcomp streams were packed by those builds (aacpack's in
parts, a `part` and a process for each), which the later ones cannot unpack
(aacpack: `compressed by version 8, this is version 9`). d of such an archive
stops before any work (exit code 2): `error: an earlier mp4pack's archive:
s.1.aac needs its aacpack`; the earlier mp4pack with its tools still
decompresses it. Its archives without those
streams decompress as ever, and mp4gui's Container box says it of one with
them. The earlier mp4pack refuses `mp4pack 2` (not an mp4pack archive, or a
later version).

## Building

With MinGW-w64, on Linux (`./build.sh`) or Windows:

```
g++ -std=c++11 -O2 -s -static -municode -o mp4pack.exe mp4pack.cpp
```

clang++ works the same way (for the MSVC target `wmain` needs no `-municode`).
The engine includes `x86targets.h`, a copy of cpuinfo's
(`../cpuinfo/x86targets.h`; copy a newer one over it), so this directory
builds on its own. For the Linux build see below.

mp4gui (`./build.sh` makes it too), plain Win32 with its common dialogs:

```
g++ -std=c++11 -O2 -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp -lcomdlg32 -lole32 -lshell32 -lgdi32
```

The sources: each program is one file, its main (`mp4pack.cpp`: the command
line; `mp4gui.cpp`: the window), which includes the rest from `inc/`, a part
for each purpose. Both include the engine, `inc/mp4pack.inc`, which includes
its parts in order: the platform (`base.inc`), what goes where
(`methods.inc`), the settings (`state.inc`), `strings.inc`, `messages.inc`,
`crc32.inc`, `files.inc`, the CPU and the tool builds (`builds.inc`),
`work.inc`, a tool's process (`tools.inc`), running the jobs (`stages.inc`),
the streams and the index (`streams.inc`), `-t`/`-c`/`-l` (`options.inc`),
`archive.inc`, `compress.inc`, `decompress.inc`, `lists.inc`, the run
(`run.inc`) and the command line (`cmdline.inc`). mp4gui's own parts are
`inc/gui_*.inc` (its controls, the Container box, the list, the progress, a
run ...). The parts are no headers: each takes what those before it have,
so they are included in their order, and only through these two files (and
the tests, which include the engine).

## Linux

The same `mp4pack.cpp` builds for Linux (x86-64, glibc), with `g++` or `clang++`:

```
./build_linux.sh        # g++ -std=c++11 -O2 -pthread -o mp4pack mp4pack.cpp
```

There `winapi_posix.h` takes the place of `<windows.h>`. It implements the Win32
functions mp4pack calls on POSIX, so the program is the same one as on Windows.
The functions cover files, pipes, processes, threads and waits, the console,
Ctrl+C, the processor counts and the errors (an errno becomes the Windows
error code for it, and `FormatMessageW` gives strerror's text for it).

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
./tools_linux.sh pe2elf mp4pack_v1 bin   # avip_avx2, ac3pack, m4vpack
./tools_linux.sh pe2elf newtools bin     # aacpack_avx2, oggcomp_avx2 (threads)
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
- The reasons in the error messages are strerror's (`No such file or
  directory`, `Permission denied`).

The archives are the same on both systems: an archive made on one decompresses
on the other. The index's `tools` line lists the builds (`h264p_avx2` or
`h264p_avx2.exe`). Apart from that line, only the output of h264p, aacpack,
mp3pack and oggcomp with more than one thread differs (their parts come out in
the order they are done; the size is the same), and it differs from run to run
on Windows too.

Tested on Linux as on Windows: the sample files with the `-t`/`-c`/`-l`
combinations, and archives from the Windows build decompressed by the Linux one
and the other way round. Names with spaces, quotes, `\`, Cyrillic and bytes
that are not UTF-8; paths of 389 characters; relative paths; a new nested `-w`
directory on another file system (`/dev/shm`). Tools that are missing, fail,
crash or cannot be run (they fall back to tangelo). Damaged and foreign archives.
SIGINT to the process group, SIGTERM, and SIGKILL (then the tools end at once
and only the work directory is left). And the movie (see [Testing](#testing)).

## Testing

`test\roundtrip.bat MP4PACK "OPTIONS" FILE...` (Windows) and
`test/roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...` (BINDIR with `mp4pack.exe`:
under wine; else the Linux `mp4pack`) compress each file with the options,
decompress it and compare, a line per file. E.g.
`test\roundtrip.bat c:\mp4pack_v0\mp4pack.exe "-t 50%+50%-1" a.mkv b.mp4`.
`test/parse_test.cpp` checks the `-t`/`-c`/`-l` parsing (it includes the
engine; build it like mp4pack.exe and run it; on Linux `g++ -std=c++11
-pthread test/parse_test.cpp`).
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
tangelo); Cyrillic paths, a new nested `-w` directory, paths over 260 characters;
damaged, truncated and foreign archives, an empty input; tool builds named by
targets and by the old level names, `-i`, no build that runs. With
builds_test, the builds chosen for 19 InstLatx64 dumps (K8 to Zen 5, Pentium 4
and Core 2 to Sapphire Rapids, VIA Nano, Zhaoxin), with and without `-i`. Not
under wine: Ctrl+C (wine's console ends a program on it at once, without its
handler).

m4vpack, ac3pack and the new avip (`mp4pack_v1`): round trips on Linux and
under wine (with `-verify`) of every sample, and of AC-3 and E-AC-3 in mkv,
mp4 and avi, MPEG-4 Part 2 video in avi (XviD and DivX tags, B-frames, qpel),
mp4 (mp4v) and mkv (V_MPEG4/ISO/ASP), made with ffmpeg, and 2 minutes of a
movie as XviD with AC-3 and with E-AC-3 (39MB each); `-l` with levels the
builds lack (4 for h264p and m4vpack, 7 for aacpack and ac3pack: refused with
the levels they have) and with ones they have. The Linux builds are the
Windows ones converted by pe2elf as they are; they make the same streams.

tangelo in place of shar: round trips of every sample on Linux and under wine
(with `-verify`), mp4pack with no shar beside it, the other files' archive,
pieces of a 13MB stream in 3 tangelo processes (`-t 0+2+1+3`), `-l ,,,5` and
`-l ,,,7` (and `,,,0` refused), and a packer made to fail, whose stream went to
`fallback.tgl`. The Linux tangelo is the Windows one converted by pe2elf as it
is; it makes the same archives.

Errors: outputs in new nested directories (c and d; mp4pack, and mp4gui with
`C ... -quit`), with a long name with spaces, brackets and a backtick in it;
outputs that cannot be written: on a drive that is not there (wine), under a
file in the way, a read-only file and directory, and a `-w` in that directory
(on Linux, as root without its override of file permissions: `setpriv
--bounding-set=-dac_override,-dac_read_search,-fowner`), a file another
program holds open without sharing (wine: refused at the start, the file left
as it was), a full disk (a 128KB tmpfs: the archive in c, the files taken
out of it in d with `-w` there); a missing input, a directory as the input or
the output. Each ended with the system's reason (avip, run out of room in a
full `-w`, with its exit code and the end of its log), at the start where the
output could be tried; mp4gui's status line showed it (cut in the middle at
the window's 640 pixels, the reason at its end in view).

The archive was checked against its format by a separate reader (in Python)
and against the files kept with `-k`, on Linux and under wine; archives made
by either build decompress with the other. Damaged archives (cut short, more
after the end, another signature or version, empty, a name with `..` or a
root, a name twice, no index) are refused with exit code 4, with nothing
written outside the work directory.

mp4gui was run under wine on a virtual X display (Xvfb), driven by a small
program sending it the messages a user's clicks and typing would, with a
screenshot at each step: compress and decompress (the archive the same as
mp4pack.exe's, the result identical), with Verify (and with an avip build made
to change a byte of what it writes in d: the check failed, as it should), Pause
(the tools got no CPU time while held) and Resume, Pause and Stop in the
archive step and in extracting it (a 1GB file stored), every task's row
finished after runs whose steps follow at once, Stop during packing and while
paused, closing the window while it runs, a bad option, the replace question
(no, then yes), Cyrillic names, quoted paths pasted, the Work dir buttons, the
files swapping with the mode, the command line (c, C, -quit after a run and not
after a failed verify, -overwrite, and the question without it, one that is not
right), the green and blue bars with wine's own visual style and with the
classic look, the settings kept in mp4gui.ini, and no work directory or
temporary file left in any case. Not on Windows itself: the look there (Segoe
UI, the themed controls) and the taskbar's progress were not seen.

Lists (on Linux and under wine): `c @list out\` and `d` of the archives
back to another directory (the files the same; two named `x.mp4`: the second
`x (2).mp4.mpk`, and back), one archive of a list (4 members, each verified
with `-verify`; a file in the list that is not there left out, exit code 2)
and `d` of it into a directory with no separator at its end, an archive of
one file to a file as before, an archive of version 1, one cut in its last
member (the 3 whole ones restored, exit code 4), an empty list and one that
is not there; lists in UTF-16, in UTF-8 with Cyrillic and accented names,
in the ANSI code page and in the OEM one (the name found by it); archives of
the Windows build read by the Linux one and the other way round. mp4gui: a
list from the command line (`c @list out\`), the row closed and open, Input
and the highlight going on while it ran, Start again when all were done (all
again, into one archive), files added with the Open dialog (sorted), Stop on
the third and Start again (the archive got the third added: its 3 files the
same), `D @archives` with one not there (marked failed, the summary saying
so), and `C @list ... -quit` (exit code 0).

The Container box, under wine: an mkv with fonts (its tracks, attachments
and files with their packers), its archive after the switch to Decompress
(and back: kept), `-t 0+2` (the h264 to tangelo), the tools without h264p
(`tangelo (no h264p build runs here)`), a text file (`unknown format`), no
file (empty), a cut mkv and a cut archive, a Cyrillic name in a Cyrillic
directory (avip got its 8.3 name) and a plain one in it (by its name, from
there), a 1GB file replaced while avip read it (that avip stopped) and Start
while it read (stopped, then read when the run ended), and `C ... -quit`.
Dragging it, with the mouse itself (`SendInput`) and with the messages sent
to it: up, down, left, right and past each end (it stops with the last line
at the bottom and the longest one's end at the right edge), no text
selected; the cursor over it (X's: a hand, the four arrows while it drags),
the wheel, and the bottom of the box drawn anew as the text moves. No caret
in it (by `GetGUIThreadInfo`), clicked, dragged, its text changed and
scrolled with the wheel while it had the focus; the keys (to each end),
Ctrl+A and Ctrl+C (the clipboard had its text), Tab and Shift+Tab out of it
and back.

aacpack and oggcomp with threads and levels (on Linux and under wine, with
`-verify`): every sample's round trip (AAC and Vorbis tracks of 18KB to
3.6MB), a 19.5MB AAC and a 9.5MB Vorbis track on 1 and 4 threads (the same
sizes under wine and on Linux, and each system's streams unpacked by the
other), `-c 0,512k`, the levels (`-l ,7`; `,5` refused for oggcomp, with its
levels 1 3 7 9; `,2` for aacpack; `,7` for an mkv with AC-3, for ac3pack), an
earlier oggcomp build ranked in front of the new one (stopped in c and d,
exit code 2), and archives of the earlier mp4pack (refused in d where they
have aacpack or oggcomp streams, restored where not). mp4gui: a run with
`-l ,7` (oggcomp `-7 -t 4`, aacpack `-7 -t 3`; their rows' progress from
their own percentages, and their gains) and its d (the same file); for an
earlier mp4pack's archive, the Container box's note and the status line
(`Failed: an earlier mp4pack's archive: s.1.aac needs its aacpack`); for an
earlier oggcomp build in front, the status line (`Failed:
oggcomp_x86-64-v4.exe does not take -t: an earlier build?`), both whole at
the window's 640 pixels.

The movie, with `-t 4+4,1,1` (the default then) on a 4-thread Xeon VM under
wine:

| | size | time |
|---|---|---|
| TBB_04.mkv | 901,752,304 | |
| h264 track, h264p -t 4 | 648,922,836 -> 564,620,814 (87.0%) | 15:18 |
| aac tracks, aacpack (its earlier build) in 8 parts | 253,612,715 -> 185,021,554 (73.0%) | alongside |
| archive | 749,649,784 (83.1%) | c 16:32, d 23:57 |

The same on Linux, on the same VM, with the Linux mp4pack and the tools
converted by pe2elf. The streams pack to the same sizes. The archive is
749,649,768 bytes, 16 smaller only because its index names the builds without
`.exe`. c took 17:40 (h264p 17:19), d 26:31 (h264p 26:15); the result was
identical to the input. The Windows build under wine decompressed the Linux
archive to an identical file too.
