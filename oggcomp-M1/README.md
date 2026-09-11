# oggcomp

A lossless recompressor for Ogg Vorbis.  `oggcomp c` turns a `.ogg` into a
smaller `.oc`, `oggcomp d` turns it back, and the file that comes back is
the file that went in, byte for byte -- not an equivalent stream, the same
stream, page boundaries, CRCs, serial numbers, padding bits and all.  It
takes any file: what the Vorbis parser can place is modelled, and the rest
-- a tag in front of the first page, junk after the last, a Skeleton stream
beside the audio, a page with a bad CRC, a file that is not Ogg at all --
is coded as bytes.  `oggcomp c` does not refuse input.

Beside it is `oggdet`, the carver: it finds every Ogg stream in a file that
holds other things too -- an archive, a disk image, a download that stopped
-- writes each out, or with `-c` writes one file holding each stream as
the `.oc` that `oggcomp c` would make of it and the rest coded too, and
puts the original back byte for byte from what it wrote.

    ./mk.sh          build ./oggcomp and ./oggdet
    ./t.sh           round-trip the test corpus and check it came back
    ./mk.sh check    prove the tuning and shipping builds code identically

## Building

Needs a C++ compiler and perl, on x86-64.  `c++` and `-O2` are the
defaults; override with `CXX` and `CXXFLAGS`.  There is one translation
unit and no library beyond libm, so there is no makefile and nothing to
configure.  g++ and clang++ both work and produce the same stream.

x86-64 is not a preference: `Lib3/coro3b.inc` switches between an x86-64
and an i386 setjmp written in inline assembly and has no third branch, so
on aarch64 the i386 one is what gets compiled.  `./mk.sh` says so and
stops rather than letting the assembler say it.  `-O3`, `-march=native`
and `-flto` were measured and are noise -- the program is bound by a
gigabyte of model tables, not by code.

| command | what it builds |
|---|---|
| `./mk.sh` | the **tuning** build: every model parameter is a live object carrying a `!MAP!` marker, which is what `IDX/opt.pl` hill-climbs |
| `./mk.sh release` | the **shipping** build: every parameter folded to a literal, no indirection, no markers |
| | both write `./oggdet` too, from the same `MOD/`: the carver includes the compressor, so a change to the model is a change to both |
| `./mk.sh check [list]` | both, over one `.ogg` per line of *list* (default: `testfiles/`), compared byte for byte |
| `./mk.sh mod` | regenerate `MOD/` in the shipping form, build nothing |
| `./mk.sh pgo f.ogg` | the shipping build, laid out from a profile of *f.ogg* |

Both builds must produce the same stream.  That is the format's contract
and `./mk.sh check` is the test of it; see `IDX/IDX-FORMAT.md` §1 for why
the two exist and how one is derived from the other.

`MOD/` is a build input, not an artefact: it ships generated so that a
machine without perl can still compile, and `./mk.sh` rewrites it from
`IDX/`.  So a tuning build leaves twelve modified files in `git status`,
which is not damage -- `./mk.sh mod` writes the shipping form back.  Run
`./mk.sh` after editing any `IDX/tsvcomp-<family>.idx` or its `.inc`: a
stale `MOD/` compiles fine and codes differently.

## Testing

`./t.sh` round-trips every file in `testfiles/`, compares each restored
file with its original, checks that coding the same input twice gives the
same `.oc`, feeds `c` things that are not Vorbis at all -- an empty file, a
stream cut off mid-page, a `.oc`, the scripts, the binary -- and requires
each back byte for byte, and checks the exit status for every way of being
told no -- including that a refused run leaves no half-written output
behind.  Then it carves a container of streams and other bytes with
`oggdet`, with and without `-c`, requires the container back, and requires
the segment `-c` wrote for a stream to be the file `oggcomp c` writes for
that stream alone.  `testfiles/README.md` says what is in the corpus
and why, and what it does not reach; `-h` lists the options.

    ./t.sh && ./mk.sh check

is the whole of it: `t.sh` is one binary against the corpus, `mk.sh check`
is the two binaries against each other.

## Running it

    oggcomp c [-v] [-H] input.ogg output.oc
    oggcomp d [-v] [-H] input.oc  restored.ogg

`-v` says where the bits went, by stage.  `-H` asks for huge pages, which
costs about ten times the resident memory and is worth measuring before
believing.  A lone `-` is stdin or stdout.

Two sharp edges worth knowing before scripting it.  **A run that fails
deletes the file its output path names** -- so that a half-written `.oc`
is never left looking like a whole one -- and it does that whether or not
it wrote anything, so a refused run over an existing file destroys it.
Point outputs at scratch paths.  And there is no `--`: anything beginning
with `-` and longer than one character is read as options, so a file whose
name starts with a dash has to be written `./-name.ogg`.

A comment or setup header seen before in the run costs one symbol
instead of being coded again.  Every encoder writes the same codebooks
for the same settings, so the links of a chained file, and with
`oggdet -S` the streams of a container, mostly share one setup header --
a kilobyte or more, coded each time it is not shared -- and the comment
header of an untagged file is the vendor string, shared the same way.
The same file twice, chained, codes its headers for a few bytes more than
once; `./t.sh` checks that.

Exit status is 0 for success, 1 for a `.oc` that `d` cannot read -- not
oggcomp's, another version's, cut short, or damaged: every `.oc` ends in a
CRC of the file it holds, and what does not check is not kept -- 2 for a
command line it cannot parse, 3 for something the filesystem said.  `c`
does not exit 1: whatever the input is, it is coded, and what is not
Vorbis in it costs about eight bits a byte.

The model tables and the static pools behind them reserve about 860 MB
of address space and touch a fraction of it -- some 85 MB resident on a
megabyte of input -- but a `ulimit -v` below the reservation turns every
run into exit 3, and `-H`
rounds every sparse touch up to a 2 MB page and costs about ten times the
resident memory.  The coroutine wants ~288 kB of stack, so a `ulimit -s`
under that, or a caller thread with a small stack, is a segfault.

### oggdet

    oggdet c [options] input prefix      carve: prefix%08X.ogg, prefix%08X.jpg, prefix.meta
    oggdet c -c [options] input file     carve, compressed, into the one file
    oggdet d [options] prefix restored   restore from the prefix's files
    oggdet d [options] file restored     restore from the one file

`oggdet c` scans any file for Ogg pages -- a candidate counts only when
its CRC verifies, so the payload of a page is never taken for one --
writes each run of them to a numbered file, each cover image found in a
comment header to a file of its own, and everything else, with the
layout, to `prefix.meta`.  Streams that went through a text-mode transfer
(every LF turned into CR LF) are recognised and written repaired; files
that come out byte-identical are written once.  `oggdet d` reads the
`.meta` and the files and gives the input back byte for byte, checked by
a CRC of the whole of it.  `-t` looks and writes nothing, `-v` reports
each stream, `-h` the rest of the options.

`-c` compresses, and puts all of it into the one file named: each stream
coded by oggcomp into a segment that is the file `oggcomp c` would have
written for it -- the same model, run in the same process and reset
between segments -- each cover image as it is, and the bytes between the
streams coded too, with the layout, in chunks between the segments.  The
file starts with eight bytes of magic and one of flags; each segment is a
varint of its length times four plus its kind (0 metainfo, 1 stream, 2
image) and the bytes.  Cut out, a stream's segment is `oggcomp d`'s to
read; the file as a whole is not, and `oggcomp d` says so.  `-m FILE`
keeps the metainfo out of the file, plain, and `oggdet d` then has to be
given it with `-m` as well.  The reset costs a few milliseconds per
segment, which shows on a container of thousands of tiny streams and on
nothing else.

`-S` is solid: `-c` with the model kept from one segment to the next, so
each is coded with what the ones before it taught the model, the bytes
between streams included, and a stream whose headers an earlier one had
pays a symbol for them -- worth having on a container of many streams
from one encoder, where that is most of them, and the per-segment reset
goes with it.  Three hundred copies of a 2.7 kB stream code to 9.6 kB
solid, against 306 kB before the headers were shared and 403 kB fresh.  The price is
that the file decodes only front to back, and only by `oggdet d`; cut
out, `oggcomp d` reads the first segment and refuses the rest.  Nothing
is deduplicated under `-S` but the images: a second copy of a stream,
coded with the model as the first left it, is not the first's segment, so
it is coded again -- for less than the first, the model having seen it,
but not for nothing -- and a container of many copies of a few streams is
`-c`'s case.

The carver is the compressor plus a front end, so everything above about
memory, the stack, `-`, and outputs deleted on failure holds for it too;
`-c` to `-` works, and then nothing is deduplicated, since nothing can be
read back from a pipe.  Its exit status is 1 for input it cannot restore
from -- a `.meta` or a file that is not one or is cut short, a segment or
stream file missing or damaged -- 2 and 3 as for `oggcomp`.

## On Windows

`gc.bat` is the Windows build; it drives clang with hard-coded toolchain
paths and is not what these scripts replace.  What matters for a Linux
reader is that `Lib3/file_api.inc` chooses between two implementations of
the same `filehandle` struct -- stdio, and CreateFile/ReadFile -- and only
the stdio one is ever compiled here.  `-DFILE_API_WIN` selects the other.

Code written against one and compiled against the other is a bug that no
Linux build can see, so `Lib3/file_api_test.cpp` asserts the two agree; its
header comment says how to run it both ways.  Building for Windows at all
takes nothing exotic:

    x86_64-w64-mingw32-g++ -O2 -fwrapv -static -o oggcomp.exe oggcomp.cpp
    x86_64-w64-mingw32-g++ -O2 -fwrapv -static -DFILE_API_WIN -o oggcomp.exe oggcomp.cpp

The same two lines with `oggdet.cpp` and `oggdet.exe` build the carver.
Both were built and run under wine over the whole corpus, and both produce
the same compressed stream as the Linux build, byte for byte, including
when either end is a pipe.

`-` means a standard stream on both, which took a fix on each side: the
WinAPI backend had no notion of it and would have made a file called `-`,
and the stdio one handed over stdin and stdout in Windows' text mode,
where every `0x0A` written becomes `0x0D 0x0A`.  That second one did not
fail -- `oggcomp d in.oc - > out.ogg` exited 0 and wrote 168196 bytes for
an input of 167685, silently not the file it was given.

## What is here

| | |
|---|---|
| `oggcomp.cpp` | the include list and `main()`.  The program is the `.inc` files beside it, one per layer, in the order they are included: `ogg_*` the container, `vb_*` Vorbis, `oc_rcio.inc`, `rc.inc` and `cm.inc` the coder's byte I/O, the range coder and the mixing primitives, `tc_*` the model machinery, `oc_*` the six models and their assembly, and `oc_coro.inc` the coroutine that is the compressor -- what `oggdet` includes.  `REFACTOR.md` and `REFACTOR2.md` say what is in each |
| `oggdet.cpp`, `oggart.inc` | the carver, and the cover-art extraction it uses.  `oggdet.cpp` includes the compressor's `.inc` files up to `oc_coro.inc` and drives that coroutine from inside its own |
| `IDX/` | the parameter and context declarations, and `idx2inc.pl`, which turns them into C++.  `IDX-FORMAT.md` is the format; `opt.pl` is the optimizer that drives a tuning build |
| `MOD/` | what `idx2inc.pl` generated, checked in |
| `Lib3/` | coroutines and the file layer.  `file_api.inc` picks stdio or the WinAPI; `file_api_test.cpp` is the test that the two are one interface |
| `testfiles/` | the corpus, and `gen.sh`, which builds it |
| `gc.bat`, `t.bat` | the Windows equivalents of `mk.sh` and `t.sh` |
