# Plan 1 — port xadpcm to the ddsdet architecture

Goal: xadpcm.cpp gets the same skeleton ddsdet.cpp has —

* a `RC<MD> : Coroutine` range coder that is a **layer of** the frontend
  coroutine rather than a separate object, with every compressed byte moving
  through a coroutine **pin**;
* every file byte in and out of the process moving through pins too, serviced by
  a driver (`XadDrive`) that plays the role of Lib3's `CoroFileProc` /
  ddsdet's `CoroDrive`;
* the direction (`encode` / `decode`) and the handful of parameters that are
  fixed for a whole segment becoming **template parameters**, so the `if(enc)`
  and `if(format==FMT_MS)` branches fold at compile time instead of being
  re-tested per coded bit.

Non-goal for this step: moving code into separate `.inc` files. That is Plan 2.
This step edits `xadpcm.cpp` in place and leaves it one file.

**Hard acceptance criterion: byte-identical output.** The container format, the
model, the coding order and every parameter stay exactly as they are, so the
ported encoder must produce archives that compare equal to the ones the
baseline produced, and the ported decoder must restore both test files. Any
size difference at all is a bug in the port, not a tuning result.

Baseline to beat (`g++ -std=gnu++20 -O2 -fstrict-aliasing -fomit-frame-pointer
-fno-exceptions -fno-rtti -I.`, MOD/ as shipped = Debug/mapping build; the
Const build was verified byte-identical to it):

| input | mode | bytes |
|---|---|---|
| `wavs2` (2,505,390) | default | 1,248,335 |
| `wavs2` | `-s` | 1,248,865 |
| `wavs2` | `-ss` | 1,248,865 |
| `Player_Death_Music_ima.wav` (601,182) | default | 355,414 |
| `Player_Death_Music_ima.wav` | `-s` / `-ss` | 355,414 |

---

## 1. What comes across from the 021 tree

Copied verbatim, no edits:

```
Lib3/common.inc            types, INLINE/NOINLINE/ALIGN, if_e0/if_e1, __assume
Lib3/coro3b.inc            struct Coroutine, yield(), coro_call0()
Lib3/coro3_pin.inc         coro3_pin: get/put/chkinp/chkout/yield_r
Lib3/coro3_pin_0.inc       the raw ptr/beg/end window
Lib3/coro3_setjmp_x64.h    __builtin_setjmp/longjmp aliases (+ x32, x64d variants)
```

Adapted (they are the templates for the new code, not dropped in as-is):

```
021/d5c_rc.inc             -> the shape of RC<MD>
021/ddsdet_drive.inc       -> the shape of XadDrive<Carrier>
021/ddsdet_carve.inc       -> the shape of the frontend (RC as base, do_process)
021/coro_fp.inc            -> reference for the r==1 / r==2 pump
```

Smoke-tested on this toolchain (g++ 13.3, x86-64) with a trivial
`Coroutine`-derived filter: 2.5 MB round-trip through the pins is byte-exact,
so `__builtin_setjmp`/`__builtin_longjmp` and the stack-copy `yield` work here.

## 2. Include order in xadpcm.cpp

`common.inc` already defines `INLINE`, `NOINLINE`, `__min`, `__max`, `byte`,
`word`, `uint`, `qword`, so the duplicate definitions currently at the top of
xadpcm.cpp are deleted rather than guarded.

```c
#include <cmath> <cstdint> <cstdio> <cstdlib> <cstring> <string> <vector>
#include "common.inc"      // Lib3
#include "coro3b.inc"      // Lib3 -- defines Coroutine; RC derives from it
   ... the arithmetic-contract enum, pclamp, PMUL/PAPN ...
#include "sh_mapping.inc"
#include "MOD/xad-C0_h.inc" ... MOD/xad-M0_p.inc
   ... everything else ...
```

`common.inc` must precede `coro3b.inc` (it supplies `if_e1`, `ALIGN`,
`__assume`), and both must precede the definition of `RC`.

`std::byte` stays out of the way for the same reason it does today: xadpcm has
no `using namespace std`, so the global `byte` from common.inc wins.

## 3. `RC<MD>` — the range coder as a coroutine layer

Replaces `struct RC` verbatim in behaviour; only where the bytes come from and
go to changes.

```c
template<int MD>
struct RC : Coroutine {
  enum { enc = (MD == 0) };        // class constant, same name the flag had
  coro3_pin* rcio = 0;
  u32 range, code; u64 low; u8 cache; u64 cachesz;
  u64 emitted = 0, consumed = 0;

  coro3_pin* rc_defpin() { return &pin[enc ? 1 : 0]; }
  void init_enc(coro3_pin* p = 0);   // rcio = p ? p : rc_defpin()
  void init_dec(coro3_pin* p = 0);   // primes with RC_PRIME bytes
  u8   rd();                         // pin get(); uint(-1) at EOF -> 0
  void shift_low();                  // rcio->put(), emitted++
  INLINE int bit(int p1, int b);     // unchanged arithmetic; `enc` now folds
  void flush();
  void raw_put(const u8*, size_t);
  void raw_get(u8*, size_t);
};
```

Points that must not drift:

* `bit()` keeps `P_BITS` and `RC_TOP` from the contract enum, not d5c's
  hard-coded 12 / 1<<24.
* `flush()` and `init_dec()` keep `RC_PRIME` (5), not d5c's literal 5.
* `rd()` maps the pin's `uint(-1)` EOF to `0`. That reproduces the current
  `inpos<insize ? in[inpos++] : 0` exactly: a truncated stream decodes to
  zeros and is caught by the entry crc, never overruns.
* the encoder's default pin is `pin[1]` (output), the decoder's `pin[0]`
  (input) — which is exactly what xadpcm wants, so `rcio` is never overridden
  and the `p` argument exists only for symmetry with d5c.

`emitted` replaces `out->size()` for the size report.

## 4. Direction as a template parameter

`bool enc` currently threads through `code_symbol`, `code_signed`, `code_byte`,
`code_bytes`, `code_data`, `code_data_ima`, `code_data_ms`. It disappears:
`Codec` becomes `template<int MD> struct Codec` with `enum { enc = (MD==0) }`,
so every `enc ?` and `if(!enc)` in those bodies folds and the bodies themselves
need no editing beyond dropping the parameter.

`Codec<MD>` holds `RC<MD>& rc` — a reference to the frontend, which *is* the
range coder — so every existing `rc.bit(...)` call site stays as written. This
mirrors `d5c::Solid<MD>::cx` / `Codec(RC<MD>& r)`.

## 5. The other compile-time parameters

Chosen because each is fixed for a whole segment, is re-tested on the innermost
path today, and takes only a couple of values.

| param | values | what folds |
|---|---|---|
| `MD` | 0 enc, 1 dec | every `enc` test, and half of `RC` |
| `FMT` | 0 IMA, 1 MS | `code_data` dispatch, `MakeIb` vs `ms_bucket`, the IMA leftover confidence vs `ms_conf`, `nib_rows` |
| `BPS` | 2,3,4,5 (MS: 4 only) | the `for(j=nbit-1;...)` tree loop bound, and `nsym = 1<<BPS` in ~20 row-index multiplies per symbol |
| `XST` | 0 mono, 1 multi-channel | every `nch>1 ?` in the per-symbol path: `set_cross`, `xch`, `xch2` |

Rejected, with reasons:

* **solid mode** — read once per segment boundary, never on the coded path.
* **`canonical`** — read once per segment.
* `NCONF`, `MSMAP`, `MSCONF`, `MATCH_MIXSEL`, `XPRED` — already IDX constants,
  so they already fold; making them template parameters would only duplicate
  code.
* **exact channel count** — `XST` captures the branch; the loop bound `nch` is
  a trip count, not a branch, and templating 1..8 would multiply the
  instantiations for nothing.

Instantiation count of the payload walk: per direction, MS gives
`XST∈{0,1}` = 2 and IMA gives `BPS∈{2..5} × XST∈{0,1}` = 8, so 10 per
direction, 20 overall. `d5c_prelude.inc` records that at ~26 instantiations GCC
silently stopped inlining `mbit`, which cost 1.4% ratio — so `mbit` keeps its
`always_inline` attribute (it already has `INLINE`), and the binary is checked
for out-of-line copies of it after the port.

Dispatch is one switch at the top of `code_data`, run once per segment:

```c
void code_data(u8* d, size_t n) {
  if( pm.format==FMT_MS )
    pm.chans>1 ? code_ms<1>(d,n) : code_ms<0>(d,n);
  else switch( pm.bps ) {
    case 2: pm.chans>1 ? code_ima<2,1>(d,n) : code_ima<2,0>(d,n); break;
    ... 3, 4 ...
    default: pm.chans>1 ? code_ima<5,1>(d,n) : code_ima<5,0>(d,n); break;
  }
}
```

`code_symbol` becomes `code_symbol<FMT,BPS>`; inside it

```c
enum { NSYM = 1<<BPS };
const int NIB = FMT ? M0_Md_Volume : I0_Ib_Volume;
```

`NIB` is a genuine `constexpr` in the Const build; in the Debug build
`M0_Md_Volume` is `1*M0_d0.Size`, i.e. load-time initialised from a `mapping`,
so it stays an ordinary `const int` there. Both are correct — this is exactly
the value `begin_seg` puts in `nib_rows` today.

**Equivalence check that has to hold:** `code_symbol` indexes rows with the
*segment's* `nsym`/`nib_rows`, while `reset_stats` *allocates* with `amaxsym`
/`amaxnib` (the maxima over all segments). Replacing the members with `1<<BPS`
and the FMT-derived volume changes the indexing not at all, because
`begin_seg` already sets `nsym = 1<<p.bps` and `nib_rows = FMT ? M0_Md_Volume :
I0_Ib_Volume`. The allocation sites keep using the runtime `amaxsym`/`amaxnib`
members and are *not* templated.

## 6. Streaming structure

### Why the encoder still buffers its input

The container is

```
"XAC1" ver solid varint(nseg) varint(lead) seg[nseg] filetable crc32 | rc stream
```

`nseg`, the per-segment geometry and the crc32 of the *whole* input all have to
be known before the first coded byte is emitted, and they come from a full scan
(`scan_segments`) over the whole input. That is a two-pass requirement of the
format, not of the implementation, so the source stays buffered. What changes
is that the bytes arrive through `pin[0]` in windows (bulk `memcpy` out of the
window, `chkinp()` to yield for a refill) instead of one `fread` of the whole
file, and — the real gain — **the compressed output is no longer materialised
in memory**: `RC::shift_low` writes straight to `pin[1]`.

### Why the decoder still buffers its output

`decode()` today "rebuilds the whole stream and verifies the crc before writing
anything", which is a documented property worth keeping, and `-t` and the
split-into-a-directory mode both want the whole image. So the decoder keeps its
`vector<u8> wav`, and streams it out with a bulk `putn()` at the end. The gain
here is the mirror of the encoder's: the *compressed input* is no longer loaded
whole, it is pulled through `pin[0]` by `RC::rd`.

### The frontend

```c
template<int MD>
struct Xad : RC<MD> {
  typedef RC<MD> RCB;
  using RCB::pin; using RCB::f_quit; using RCB::get; using RCB::put;
  using RCB::chkinp; using RCB::chkout; using RCB::addinp; using RCB::addout;
  using RCB::coro_init; using RCB::coro_call; using RCB::getoutsize;

  Codec<MD> cx;
  Xad() : cx(*this) {}

  vector<u8> src;              // enc: the slurped container / dec: the output
  Arc ar;
  vector<string> names; vector<u64> sizes;   // filled by the driver (enc)
  int solid, test, rc_err;
  u64 hdr_bytes;

  void putn(const u8*, size_t);     // bulk write into pin[1], chkout per chunk
  void slurp();                     // bulk read pin[0] -> src until f_quit
  int  hget();                      // one header byte from pin[0], -1 at EOF
  void hput(u8);                    // one header byte to pin[1]
  u64  hget_varint(bool& ok);
  void hput_varint(u64);

  void do_encode();                 // MD==0 only
  void do_decode();                 // MD==1 only
  void do_process() { if constexpr(MD==0) do_encode(); else do_decode(); yield(this,0); }
};
```

`do_encode()`:

1. `slurp()` — the whole container arrives through `pin[0]`. The driver has
   already recorded one `(name,size)` pair per input file while feeding them.
2. `scan_segments(src, ar)`, `ar.crc = crc32b(...)` — unchanged code.
3. write the container header with `hput`/`hput_varint` — same field order,
   same encoding, so the bytes are identical to what `encode()` pushes into
   `out` today. Count them into `hdr_bytes`.
4. `rc.init_enc()` (defaults to `pin[1]`), then the existing segment loop
   verbatim, then `rc.flush()`.

`do_decode()`:

1. parse the container header by pulling bytes with `hget` / `hget_varint`.
   This replaces the index-into-`in[]` parse; the bounds checks against
   `insize` become EOF checks on `hget`.
2. `rc.init_dec()` (defaults to `pin[0]`).
3. size `src` from the segment table and run the existing segment loop.
4. crc-check; on mismatch set `rc_err` and write nothing.
5. `putn(src.data(), src.size())`.

`do_process` ends with `yield(this,0)`, as every Lib3 coroutine must.

### The driver

`XadDrive<Carrier>` extends the `CoroFileProc` pump with what xadpcm needs and
ddsdet did not: several input files behind one `pin[0]`, and an output that may
be a file, a memory buffer (for `-t`), or a sequence of files split by the
decoded file table.

```c
template<class Carrier>
struct XadDrive : Carrier {
  enum { bufsize = 1<<16 };
  ALIGN(4096) byte inpbuf[bufsize];
  ALIGN(4096) byte outbuf[bufsize];

  // input: either a list of paths (opened in turn) or one memory image
  char** in_paths; int in_n, in_i; FILE* in_f;
  const u8* in_mem; size_t in_memn; int in_mem_fed;

  // output: exactly one of these
  FILE*        out_f;      // single file
  vector<u8>*  out_mem;    // -t: keep the coded stream in memory
  const char*  out_dir;    // decode to a directory, split by the file table

  void sink(const byte* p, size_t n);
  uint run();              // r==1 refill, r==2 flush, r==0 done
};

XadDrive< Xad<0> > Menc;
XadDrive< Xad<1> > Mdec;
```

`run()` is the CoroDrive loop:

* `r==1` — read `bufsize` from the current input. On short/zero read, advance
  to the next path; **`f_quit` is set only when the last path is exhausted**,
  otherwise a multi-file encode would stop at the first file. Each file's byte
  count is pushed into `Carrier::sizes` and its basename into `Carrier::names`
  (only when more than one path was given, matching today's `nin>1` rule).
* `r==2` — `sink(outbuf, bufsize)` and re-arm `pin[1]`.
* `r==0` — `sink(outbuf, getoutsize())` and stop.

`sink()` writes to whichever of `out_f` / `out_mem` / the split-by-table
sequence is configured. The split sink consults `Carrier::ar.names` and
`Carrier::ar.sizes`, which the decoder fills while parsing the header — i.e.
before it can possibly emit an output byte, so the ordering is safe.

### `-t` (verify by decoding in memory)

Today `compress()` decodes `out` in memory and refuses to write on mismatch.
Preserved exactly: with `-t`, `Menc`'s sink is a `vector<u8>` instead of the
FILE, `Mdec` is then run with `in_mem` = that buffer and `out_mem` = a second
buffer, and the result is compared against `Menc.src` (the slurped input, still
live). Only if that passes is the buffer written to the output path.

## 7. Stack-budget check

`yield()` copies `stkptrH - stkptrL` bytes into `Coroutine::stk[STKPAD]`, and
`STKPAD` is 64 KB + 24. Deepest coroutine call chain after the port:

```
do_process -> do_encode -> code_data -> code_ima<B,X> -> code_symbol<F,B>
           -> mbit -> rc.bit -> shift_low -> coro3_pin::put -> chkout -> yield_r
```

The only sizeable frames are `code_symbol`'s `Ctr* cs[MIX_NIN]` (28 pointers)
and `code_data_ima`'s `vector` handles (8+8 × 24 bytes; the data is on the
heap). Well inside 64 KB. The rule this imposes on the port: **no large
automatic arrays anywhere below `do_process`** — `analyze`'s
`u8 probe[64+4*MS_MAXCOEF]` (192 B) is the largest and is fine, but nothing
new of that shape may be added.

## 8. Risk list and the check for each

| risk | check |
|---|---|
| header bytes differ from the `vector<u8> out` version | encoder output compared byte-for-byte with `wavs2.base` / `pdm.base` |
| `rd()` EOF semantics differ from `inpos<insize?...:0` | decode a deliberately truncated archive; must fail on crc, not crash |
| multi-file encode stops at the first file | `xadpcm c a.wav b.wav out` + `d out dir/` round-trip |
| `nsym`/`nib_rows` templating changes an index | byte-identical output is the proof; it would move immediately |
| `mbit` stops being inlined at 20 instantiations | `nm --defined-only` on the binary must show no `mbit` symbol |
| coroutine stack overflow | run under ASan; also `-fstack-usage` on the deepest frames |
| static-init order: carriers constructed before the `mapping` objects | carriers are declared after the `MOD/*_h.inc` includes, at the bottom of the TU, and their constructors read no IDX value |
| `-t` no longer proves anything | run `xadpcm c -t` on both test files |

## 9. Verification protocol for this step

```sh
./mk.sh                 # Debug/mapping build
./mk.sh release         # Const build
for f in wavs2 Player_Death_Music_ima.wav; do
  for m in "" -s -ss; do
    ./xadpcm c $m $f out && cmp out <baseline for that (file,mode)>
    ./xadpcm d out back && cmp back $f
  done
  ./xadpcm c -t $f out
done
# multi-input / multi-output
./xadpcm c wavs2 Player_Death_Music_ima.wav multi && ./xadpcm d multi dir/
# both builds must produce identical archives
```

Every one of those `cmp`s must pass before Plan 2 starts.

---

## 10. Review of this plan — errors found and corrected

Read back against the baseline source before writing any code. Six things were
wrong or missing; all are folded into the sections above.

**10.1 `Codec` must NOT be templated on `FMT`.** The obvious reading of
"template instance per audio type" is `Codec<MD,FMT>`, and it is wrong.
`SOLID_ALL` (`-ss`) deliberately carries counters, mixer weights, APM tables
and the match history across a wav boundary *whatever the codec on either
side*, and `SOLID_SAME` (`-s`) carries them across a boundary between two wavs
of the same format. Two `Codec<MD,FMT>` objects cannot share one set of tables,
so `-ss` over a stream holding both an MS and an IMA wav would silently lose
the carry-over — a capability regression that no test on the two single-format
corpora would catch. `FMT` therefore parameterises the *payload walk*
(`code_data_ima<BPS,XST>`, `code_data_ms<XST>`, `code_symbol<FMT,BPS>`), while
the tables live in one `Codec<MD>`. Everything the format decides still folds;
nothing about the archive changes.

**10.2 `FMT` in `code_symbol` selects one value, so it must be a template
parameter and `NIB` must not.** `code_symbol` uses the format only for
`nib_rows` in `tQI.row(qhat*nib_rows+ib)`. Making `nib_rows` itself the
template parameter is impossible: `M0_Md_Volume` is `1*M0_d0.Size` in the Debug
build — load-time initialised from a `mapping` object, not a constant
expression. `code_symbol<FMT,BPS>` with `const int NIB = FMT ? M0_Md_Volume :
I0_Ib_Volume;` compiles in both build modes and folds fully in the Const one.
Instantiations: `<0,2> <0,3> <0,4> <0,5> <1,4>` = 5 per direction.

**10.3 The IMA arithmetic helpers take `bps` at runtime and were left out.**
`index_update` switches on `bps` *per coded symbol*, and `code_magnitude` /
`ima_quantize` run `bps-1`-trip loops. With `BPS` already a template parameter
of the caller these become `template<int BPS>` free functions and the switch
becomes `if constexpr`. Same values, no branch. (`ima_apply` too, for its
`1<<(bps-1)` sign mask.) The MS helpers need nothing — MS is 4-bit by
definition and none of them reads `bps`.

**10.4 The multi-file input pump can stall on an empty file.** "Read the next
file when this one returns 0" is not enough: a zero-length input would make the
driver return 0 bytes with `f_quit` still clear, the coroutine would yield
`r==1` again, and the pump would hand back 0 bytes again — a live spin. The
`r==1` handler therefore loops internally: advance through paths until a read
returns bytes *or* the path list is exhausted, and only then set `f_quit`. An
empty input still gets its `(name, 0)` pair pushed, because that is what
today's `ar.sizes.push_back(one.size())` records for it.

**10.5 The split-into-a-directory sink was under-specified.** `decompress()`
today applies `safe_name()` to each stored name — an archive member called
`../../etc/x` must not decide where it lands — and joins it to the output
directory. That check lives in the *driver's* sink after the port, not in
`main()`, and must not be dropped: it is the only thing standing between a
hostile archive and an arbitrary path.

**10.6 Nothing enforces the 64 KB coroutine stack budget.** `yield()` copies
`stkptrH - stkptrL` bytes into `Coroutine::stk[STKPAD]` with no bound check,
and an overflow there is an intra-object overwrite that ASan does not
instrument — the same class of bug the `MIX_MAXIN` note in README-dual.txt
describes, and it would present as a wild jump rather than a clean crash. So
the port adds a measurement rather than an assumption: the frontend records
`stkptrH - (char*)&a_local` at the deepest coding site and `-v` prints the
high-water mark, which must come out far below 65560. Checked once per build
shape, since inlining decisions move it.

**Verified rather than assumed:** the Lib3 coroutine round-trips 2.5 MB
byte-exactly under `-O0`, `-O2 -fstrict-aliasing -fomit-frame-pointer
-fno-exceptions -fno-rtti` (the mk.sh flags), `-O3`, and `-Ofast
-march=haswell` on g++ 13.3 — `__builtin_setjmp`/`__builtin_longjmp` and the
stack-copy `yield` are not disturbed by the frame-pointer or aliasing
settings the shipping builds use.

**Checked and found already correct:** header byte order and encoding
(replicated field for field); `RC::rd()`'s EOF-to-zero mapping against
`inpos<insize ? in[inpos++] : 0`; `hdr_bytes + rc.emitted == out.size()`;
`code_symbol` indexing with the segment's `nsym`/`nib_rows` while `reset_stats`
allocates with `amaxsym`/`amaxnib`; static-init order of the global carriers
against the `mapping` objects; the pointer-to-member cast in `coro_call`
through the single non-virtual chain `Coroutine <- RC<MD> <- Xad<MD> <-
XadDrive<>`.
