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
-- writes each out, or with `-c` writes the `.oc` that `oggcomp c` would,
and puts the original back byte for byte from what it wrote.

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
the segment of `.oc` the carver wrote for a stream to be the file
`oggcomp c` writes for that stream alone.  `testfiles/README.md` says what is in the corpus
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
    oggdet c -c input prefix             carve: prefix.oc, prefix%08X.jpg, prefix.meta
    oggdet d [options] prefix output     restore

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

`-c` compresses: every stream is coded by oggcomp into the one file
`prefix.oc`, one segment per distinct stream, in place of the `.ogg`
files.  A segment is the file `oggcomp c` would have written for that
stream -- the same model, run in the same process and reset between
streams -- so cut out of `prefix.oc` (eight bytes of magic, then a varint
length in front of each segment) it is `oggcomp d`'s to read; the file as
a whole is not, and `oggcomp d` says so.  The reset costs a few
milliseconds per stream, which shows on a container of thousands of tiny
ones and on nothing else.

`-S` is solid: `-c` with the model kept from one stream to the next, so
each stream is coded with what the ones before it taught the model --
worth having on a container of many streams from one encoder, and the
per-stream reset goes with it.  The price is that a solid segment decodes
only after the ones before it, in order, and only by `oggdet d`; cut out,
`oggcomp d` reads the first and refuses the rest.  Duplicates are coded
again under `-S`: the carver calls two streams one when the segment it
coded for the second is byte for byte one already in the file, which
solid segments of one stream are not, and the second copy costs less
than the first but not nothing -- a container of many copies of a few
streams is `-c`'s case.
The carver is the compressor plus a front end, so everything above about
memory, the stack, `-`, and outputs deleted on failure holds for it too;
its exit status is 1 for input it cannot restore from -- a `.meta` that is
not one or is cut short, a stream file missing or damaged -- 2 and 3 as
for `oggcomp`.

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
