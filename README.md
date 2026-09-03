# CDM — compressed data model

A post-coder for high-entropy data: it consumes a stream that is already close to
incompressible (typically another compressor's output) and recovers the residual
redundancy that survives byte alignment, padding and probability quantisation.
The mechanism is enumerative coding — a block of `n` bits with population count
`k` is uniform over `C(n,k)` strings, not over `2ⁿ` — driven by an optimal parser
that picks block boundaries and per-block coding mode. See `CDM-analysis.md` for
the full write-up.

Two revisions live here:

| revision | coder | termination |
| --- | --- | --- |
| `044-EOF--v4` | `Rangecoder_SH1m` — byte-granular, delayed-carry cache | explicit EOF flag per block, plus a rangecoder flush |
| `045-BIJ--v1` | `sh_bit` — bit-granular, reserved inverse-FO endings | bijective: no EOF symbol, no flush |

r044 is the upstream release with the build ported to Linux. r045 replaces its
rangecoder with the bijective coder from `sh_bits`, wired as a `CoroutinePair`
pipeline; the model, the price tables and the optimal parser are unchanged.

## Build

```sh
make                 # -> 044-EOF--v4/cdm and 045-BIJ--v1/cdm
make test            # round-trip 044-EOF--v4/testfile through both
make selftest        # r045's exhaustive forward/backward suite (~20 s)
make check           # the same suite with every coder invariant asserted (~40 s)
make ab              # size comparison of the two revisions
make CXX=clang++     # clang works too, and emits identical streams
```

or, equivalently, the shell twin of the original `g.bat`, in either directory:

```sh
cd 044-EOF--v4 && ./g.sh
cd 045-BIJ--v1 && ./g.sh
```

Both accept `CXX`, `CXXARCH` (e.g. `-march=native`) and `LDFLAGS` from the
environment. gcc 13 and clang 18 build clean with no warnings at default warning
level; the Windows `g.bat` still builds the same sources with MinGW.

## Usage

```sh
cdm c <input> <output>     # compress
cdm d <input> <output>     # decompress
cdm t                      # r045 only: the self-test suite
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
| `044-EOF--v4/sh_v1m.inc` | r044's rangecoder (binary, byte and enumerative paths) |
| `045-BIJ--v1/sh_bitrc.inc` | r045's coder: sh_bit's bijective rangecoder behind that same API |
| `045-BIJ--v1/rcio.inc` | sh_bit's psi/psi-inverse codestream I/O over coroutine pins |
| `045-BIJ--v1/tests.inc` | the `cdm t` self-test suite |
| `Lib3/bitwrap.inc` | `BitstringWrap` — the bijective bit/byte wrap |
| `044-EOF--v4/MOD/` | model constants generated from `IDX/sh_model.idx` |
| `044-EOF--v4/IDX/` | parameter description and the perl tuning pipeline |
| `BWTS/` | bijective BWT: `bwts` bytewise, `bwth`/`bwtl` bitwise MSB-/LSB-first |
| `tools/` | CDM2 measurement tools: `ctxscan` (screen), `lanesplit` (direct test) |
| `Lib3/` | coroutine, file and common helpers shared with other Shelwien projects |

## r045: the bijective coder

`045-BIJ--v1` swaps CDM's rangecoder for the one from `sh_bits` — a 32-bit
carryless coder made bijective on bitstrings by reserving *inverse finitely-odd*
endings — and runs it as the first stage of a two-stage `CoroutinePair`, exactly
the shape `sh_bitc.cpp` uses:

```
encode:  file ---bytes--> CDM<0> ---bits--> midbuf --> BitWrap<0>::bit2byte ---bytes--> archive
decode:  archive ---bytes--> BitWrap<1>::byte2bit ---bits--> midbuf --> CDM<1> ---bytes--> file

typedef CoroFileProc< CoroutinePair< CDM<0>, BitWrap<0> > > t_encproc;
typedef CoroFileProc< CoroutinePair< BitWrap<1>, CDM<1> > > t_decproc;
```

### The adapter

`sh_bitrc.inc` is the whole port: sh_bit's coder wearing `Rangecoder_SH1m`'s
method surface, so `codec.inc`, `opt_calc.inc` and `opt_tok.inc` keep calling
`rc_Process` / `rc_BProcess` / `rb_Process` / `rs_process_byte` unchanged. Two
structural gaps had to be bridged:

* **`code` is absolute, not interval-relative.** `Rangecoder_SH1m` normalises
  `low` to zero and subtracts from `code`; sh_bit tracks `low` explicitly because
  its ending machinery works in absolute codestream coordinates
  (`in_window(v)` is `uint(v-low()) < range`). Every `code >= rnew` becomes
  `code-low() >= rnew`, and every `code -= x` disappears — the unconditional
  `lowc += x` carries both directions.
* **Renormalisation is by the bit, not the byte.** `rc_Renorm1` and `rc_Renorm2`
  are byte-granular shortcuts that are exact only while `range >= 2^24`. With
  `range >= 2^31` and one bit per shift, `rb_Process` with `freq0 = 1, n = 2049`
  drops eleven bits at a stroke, so both collapse to the full
  `while( range<sTOP )` loop.

`rcio.inc` is sh_bit's ψ/ψ⁻¹ one-bit-delay layer, unchanged except that its
codestream accessors are spelled `bij_get`/`bij_put`: CDM's `decode_block` calls a
bare `put(c)` to emit a decoded *plaintext* byte, which must reach
`Coroutine::put` and not the bit layer.

### Where the ending is reserved

sh_bit checks and reserves an ending at every symbol, because every symbol
boundary can end the message. CDM can only stop at a **block** boundary, so
`rc_ending()` is called exactly where `process_EOF()` used to be — once per block,
on both sides. The injectivity argument survives the thinning: what it needs is
that an ending live at one stop point is never the final ending at a later stop
point, and reserving at exactly the stop points gives that.

Reserving per block rather than per symbol also makes the two limits the sh_bit
paper documents in its §7 disappear. The 4090-ending cap — which `sh_bitc`
reaches after roughly 760 000 identical bytes, forfeiting injectivity — and the
linear reservation scan that made 1 MiB of zeros a projected hour are both driven
by the number of simultaneously live endings. Measured here, that number is **1**
on every input tried, including 200 000 identical bytes:

| input | max live endings | discarded |
| --- | --- | --- |
| 200 000 × `00` | 1 | 0 |
| 200 000 × `FF` | 1 | 0 |
| gzip -9 output | 1 | 0 |
| 1 MB urandom | 1 | 0 |

### What it buys, and what it does not

Bijective termination removes the per-block EOF flag, the final EOF symbol and the
rangecoder's flush. Archives come out 1–2 bytes smaller than r044 on every input,
and the empty file encodes to the empty archive:

| input | r044 | r045 |
| --- | --- | --- |
| empty | 2 | **0** |
| 1 byte | 4 | 3 |
| 200 000 × `00` | 1620 | 1619 |
| popcount-skewed 200 000 | 187 751 | 187 749 |
| gzip -9 output 485 091 | 484 763 | 484 761 |
| urandom 1 000 000 | 1 000 026 | 1 000 024 |

Speed is unchanged (0.68 s vs 0.80 s to encode 485 kB; decode 0.06 s vs 0.05 s).

**It does not make CDM a bijection on archives**, and cannot. `sh_bitc` can be one
because its order-0 model codes one symbol per byte with no choices, so an archive
determines the encoder's every move. CDM's encoder runs an optimal parser: it picks
block lengths and one of three coding methods per block by price. An arbitrary
archive therefore decodes to a message whose implied parse is generally not the
parse the encoder would choose, and re-encoding gives a different archive. A second,
independent cause: the decoder reads an implied tail of `1`s past the end of the
archive, and with blocks of up to 256 bytes it can consume much more codestream
than the archive pays for, so a re-encoding can also come out *longer*. Both are
properties of a parsing compressor, not of the coder — abandoning them would mean
abandoning the parser, which is the compressor.

`./045-BIJ--v1/cdm t` therefore asserts what holds and reports what does not:

```
[0] TERMINATION empty file <-> empty archive        : enc=0 dec=0 PASS
[1] FORWARD  decode(encode(m))==m, |m|<=2 bytes     : ok=65793 fail=0 maxarc=4 PASS
[2] BACKWARD encode(decode(a))==a, |a|<=2 bytes     : ok=65277 differ=516 (99.2%) runaway=0 reported
[3] STABILITY 2000 random archives, 3<=|a|<=64      : ok=2000 unstable=0 runaway=0 PASS
[4] FUZZ     300 random files, |m|<4000             : ok=300 fail=0 PASS
```

Test [3] asserts the property that *does* hold on arbitrary archives: whatever
message one decodes to, that message survives its own encode/decode round trip.
Test [5] pins determinism, which the others would miss: the same input must give
the same archive whatever ran before it. `make check` reruns the suite with
`-DCDM_CHECK`, which turns every coder invariant into an assertion — `range`
never 0, normalised at every entry, the decoder's value inside the window, model
frequencies in range, renormalisation bounded. It passes over 100 000 arbitrary
archives.

### One robustness fix, inherited from r044

`codec.inc` re-checks `(freq0==0)+(freq1==0)` once per **byte**, so the remaining
bits of a byte are coded with a count already at zero. That is deliberate and
free — the split is degenerate, `rnew` is 0 or `range`, and the bit costs
nothing. Almost: `rnew` is really `range - (0xFFFFFFFF mod n)·range/2^32`, which
leaves a sliver of codestream values the encoder can never produce. Decoding an
*arbitrary* archive can land there, and the raw comparison then returns the bit
the count forbids, wrapping the counter to `0xFFFFFFFF` and indexing
`rb_itotal` out of bounds on the next call. The measured mass of that sliver is
about 1.6·10⁻⁴ per self-test run, so it is reachable, not theoretical.

r045's `rb_Process` clamps the decoded bit to the one the counts allow:

```cpp
if( f_DEC ) bit = (bit | (freq0==0)) & (freq1!=0);
```

On any encoder-produced stream that is a no-op — verified byte-identical across
the whole corpus — and it costs nothing measurable. r044 has the same hazard; it
is simply never asked to decode a foreign archive.

### One fix in the model

r044's `CDM::Init()` resets `inppos` and the parse array along with the ~1 MB price
tables, so a CDM object was single-shot — a second `processfile()` on the same
object restarted with a stale `inppos` and looped. That is invisible when a process
compresses one file and exits, but the self-test runs thousands of encodes. r045
splits the cheap per-run half out as `Reset()`, called at the top of `do_encode`
and `do_decode`; it restores `inppos`, the one adaptive global (`p_maxblk`) and the
touched prefix of `tok_array`. Each of those three is covered by test [5] —
removing any one of them makes it fail (or, for `inppos`, hang exactly as r044
did).

`p_EOF` and its tunable `M_EOF` are gone with the EOF symbol; the `IDX` entry is
removed too, so the parameter optimizer does not spend cycles tuning something
that no longer reaches the output.


## The transform matters more than the coder

CDM is a post-coder for a bitwise BWT, and nearly all of the compression comes
from the transform. Measured on 262 kB of text:

| pipeline | output | gain |
| --- | --- | --- |
| `cdm` alone | 257 267 | 1.86 % |
| `BWTS/bwts` (bytewise) + `cdm` | 239 083 | 8.80 % |
| `BWTS/bwth` (bitwise, MSB-first) + `cdm` | **75 630** | **71.15 %** |

Bit order is a real, data-dependent parameter: on `gzip -9` output `bwtl`
(LSB-first) gives 3.01 % where `bwth` gives 0.42 %, a 7× difference, because
deflate packs its Huffman codes LSB-first. Try both.

```sh
make bwts
./BWTS/bwth c256 input input.bwth      # chunk size in KB for bwth/bwtl
./045-BIJ--v1/cdm c input.bwth archive
```

`CDM2-design.md` proposes replacing the BWT's fixed context grouping with a
searched context tree, and measures 19.8 % beyond `bwth`+`cdm` on that text case.
`make tools` builds the two measurement tools it depends on.

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
