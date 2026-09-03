# CDM — compressed data model

A post-coder for high-entropy data: it consumes a stream that is already close to
incompressible (typically another compressor's output) and recovers the residual
redundancy that survives byte alignment, padding and probability quantisation.
The mechanism is enumerative coding — a block of `n` bits with population count
`k` is uniform over `C(n,k)` strings, not over `2ⁿ` — driven by an optimal parser
that picks block boundaries and per-block coding mode. See `CDM-analysis.md` for
the full write-up.

This is release r044 (`044-EOF--v4`), with the build ported to Linux.

## Build

```sh
make                 # -> 044-EOF--v4/cdm
make test            # build, then round-trip 044-EOF--v4/testfile
make CXX=clang++     # clang works too, and emits identical streams
```

or, equivalently, the shell twin of the original `g.bat`:

```sh
cd 044-EOF--v4 && ./g.sh
```

Both accept `CXX`, `CXXARCH` (e.g. `-march=native`) and `LDFLAGS` from the
environment. gcc 13 and clang 18 build clean with no warnings at default warning
level; the Windows `g.bat` still builds the same sources with MinGW.

## Usage

```sh
cdm c <input> <output>     # compress
cdm d <input> <output>     # decompress
```

The encoder is built with `TRACE_ON` (set in `cdm.inc`), so it writes a per-block
trace to stdout. Redirect it if you only want the timing:

```sh
./044-EOF--v4/cdm c infile infile.cdm >/dev/null
```

## Layout

| Path | What |
| --- | --- |
| `044-EOF--v4/cdm.cpp` | entry point; instantiates the encoder and decoder coroutines |
| `044-EOF--v4/cdm.inc` | model parameters, price tables, the optimal parser |
| `044-EOF--v4/codec.inc` | block header and block body coding |
| `044-EOF--v4/opt_tok.inc` | parser token, per-position price computation |
| `044-EOF--v4/opt_calc.inc` | closed-form price estimates used to fill the tables |
| `044-EOF--v4/sh_v1m.inc` | rangecoder (binary, byte and enumerative paths) |
| `044-EOF--v4/MOD/` | model constants generated from `IDX/sh_model.idx` |
| `044-EOF--v4/IDX/` | parameter description and the perl tuning pipeline |
| `Lib3/` | coroutine, file and common helpers shared with other Shelwien projects |

## Parameter tuning

`IDX/sh_model.idx` describes the tunable parameters; `IDX/idx2inc.pl` expands it
into `MOD/sh_model_h.inc`, and `IDX/import.pl` folds an optimizer export back
into the `.idx`. `IDX/import-all.sh` is the POSIX twin of `import-all.bat`.
`IDX/IDX-FORMAT.md` documents the format.

## Notes on the Linux port

* `<windows.h>` was only reachable for a commented-out `GetTickCount` trace; it
  is now behind `#ifdef _WIN32`.
* `_bit_scan_forward` is an MSVC/ICC intrinsic that MinGW happens to supply via
  `<windows.h>` → `<intrin.h>`. `Lib3/common.inc` now defines it in terms of
  `__builtin_ctz` on non-Windows gcc/clang.
* The templates relied on `-fpermissive` to find members of dependent base
  classes by unqualified name. Those names now have explicit `using`
  declarations in `CDM` (`cdm.inc`) and `CoroFileProc` (`Lib3/coro_fp2.inc`), so
  the build no longer needs `-fpermissive`.
* `%I64i` is an msvcrt printf modifier. `Lib3/common.inc` defines an `I64` macro
  that picks `"I64"` on msvcrt and `"ll"` elsewhere.
