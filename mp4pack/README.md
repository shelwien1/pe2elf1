# mp4pack

Lossless compression of whole mp4/mkv/avi files, for Windows. `mp4pack.exe` is a
wrapper: it takes the container apart with `avip`, gives each stream to the
packer made for it, and stores the results in one `shar a0` archive.

```
mp4pack c [options] input output     compress
mp4pack d [options] input output     decompress
```

The tools are looked up in the directory of `mp4pack.exe` itself, so put it next
to them (the `mp4pack_v0` set): `avip.exe`, `h264p.exe`, `aacpack.exe`,
`mp3pack.exe`, `oggcomp.exe`, `ttfcomp.exe`, `shar.exe`. Only `avip` and `shar` are
required; a stream whose packer is missing goes to `shar a22` (with a note).

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
| `-t 4` | missing numbers repeat the last one with its separator: `4,4,4,4` (and `-t 3+1` is `3+1+1+1`) |

Within a group, jobs run in order, as many at a time as its threads allow:
h264p and mp3pack take all of their group's threads (`-t N`), the aacpack parts
and oggcomp one each. ttfcomp is always one process with all the fonts (solid
compression), and so is the `shar a22` archive of the other files, except for
pieces of big files (`-c`).

`0` for a group means no packer for it: its files go to `shar a22` instead; `0`
for shar stores the other files as they are (so `-t 0` stores everything).

Default: `N+N+1+1` to compress, `N+N+N+N` to decompress, N = the number of
logical CPUs. h264p and mp3pack take their decompression threads from the
stream, so for d, `-t` only sets how many things run at once.

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

## Errors

| exit code | |
|---|---|
| 0 | done |
| 1 | bad command line |
| 2 | a file cannot be read or written, a tool is missing |
| 3 | a tool failed (the end of its log is shown) |
| 4 | d: not an mp4pack archive, or a stream or the result is not as it was |
| 5 | aborted with Ctrl+C |

When compressing, a packer that fails (or crashes) does not stop it: the
stream it had goes to `shar a22` instead, with a warning. avip and shar
failing do stop it. The tools run in a job object, so they end with mp4pack
however it ends; on Ctrl+C the work directory is removed too.

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
the options under wine, decompresses it and compares. Done here with wine 9 on
Linux, on the `003` mkv/mp4 test files, synthetic files with every kind of
stream (h264, aac, mp3, vorbis, ac3, opus, flac, xvid, ASS/SRT/tx3g subtitles,
ttf attachments; mkv, mp4, avi), and a whole movie; with `-t` and `-c` in
sequence, in parallel and mixed, with zeros, with parts and pieces; with
packers that fail or crash (they fall back to shar); with Cyrillic paths and a
new nested `-w` directory; with damaged, truncated and foreign archives, and an
empty input.
