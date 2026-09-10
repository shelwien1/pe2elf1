# oggcomp

A lossless recompressor for Ogg Vorbis.  `oggcomp c` turns a `.ogg` into a
smaller `.oc`, `oggcomp d` turns it back, and the file that comes back is
the file that went in, byte for byte -- not an equivalent stream, the same
stream, page boundaries, CRCs, serial numbers, padding bits and all.  A
file it cannot promise that for it refuses rather than coding.

    ./mk.sh          build ./oggcomp
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
same `.oc`, and checks the exit status for every way of being told no --
including that a refused run leaves no half-written output behind.
`testfiles/README.md` says what is in the corpus and why, and what it does
not reach; `-h` lists the options.

    ./t.sh && ./mk.sh check

is the whole of it: `t.sh` is one binary against the corpus, `mk.sh check`
is the two binaries against each other.

## Running it

    oggcomp c [-v] [-H] input.ogg output.oc
    oggcomp d [-v] [-H] input.oc  restored.ogg

`-v` says where the bits went, by stage.  `-H` asks for huge pages, which
costs about ten times the resident memory and is worth measuring before
believing.  A lone `-` is stdin or stdout.

Exit status is 0 for success, 1 for an input it will not code, 2 for a
command line it cannot parse, 3 for something the filesystem said.

The model tables reserve about 1.3 GB of address space and touch very
little of it -- some 50 MB resident on a megabyte of input -- but a
`ulimit -v` below the reservation turns every run into exit 3, and `-H`
rounds every sparse touch up to a 2 MB page and costs about ten times the
resident memory.  The coroutine wants ~288 kB of stack, so a `ulimit -s`
under that, or a caller thread with a small stack, is a segfault.

## What is here

| | |
|---|---|
| `oggcomp.cpp` | all of it: the Ogg and Vorbis parsers, the model, the coder |
| `IDX/` | the parameter and context declarations, and `idx2inc.pl`, which turns them into C++.  `IDX-FORMAT.md` is the format; `opt.pl` is the optimizer that drives a tuning build |
| `MOD/` | what `idx2inc.pl` generated, checked in |
| `Lib3/` | coroutines and the file layer |
| `testfiles/` | the corpus, and `gen.sh`, which builds it |
| `gc.bat`, `t.bat` | the Windows equivalents of `mk.sh` and `t.sh` |
