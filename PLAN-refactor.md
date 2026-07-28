# Plan 2 — split xadpcm into .inc parts, and finish the IDX story

Plan 1 changed the architecture and left the file monolithic. This one changes
no behaviour at all: it splits the 2,900-line xadpcm.cpp into parts along the
seams that already exist, moves the last tunable that is still a bare literal
into IDX, and makes IDX values usable as template parameters in **both** build
modes so the parameters that shape the hot loops can fold in the shipping build.

Still ONE translation unit — every part is `#include`d into xadpcm.cpp, exactly
as `dxt5comp_core.inc` assembles the d5c codec. No separately compiled modules,
no headers with declarations-here / definitions-there, every entity defined
exactly once in exactly one file.

**Hard acceptance criterion, again: byte-identical output.** Same archives as
rev 016 for both test files in all three solid modes, the two-input archive, the
mixed MS+IMA `-ss` stream, and all 14 synthetic wavs — in both build modes.

---

## 1. The file split

`xadpcm.cpp` becomes a table of contents plus `main()`. Everything else moves:

| file | holds |
|---|---|
| `xad_prelude.inc` | types, clamp helpers, the arithmetic-contract enum |
| `xad_idxparam.inc` | the bridge to the generated headers: `NCONF`, the IDX template-parameter idiom, the USE_NEW backstop — included *after* `MOD/` |
| `xad_util.inc` | crc32b, rd/wr 16/32, zig/unzig, safe_name, get_bits/put_bits, i16_of |
| `xad_logistic.inc` | SQT / STT / CLT, `RateSet`, `init_tables`, `squash` |
| `xad_rc.inc` | `RC<MD> : Coroutine` |
| `xad_counter.inc` | `Ctr`, `CtrS`, `CtrTab`, `CtrSTab` |
| `xad_mix.inc` | `Mixer<LR,CLAMPL>`, `APM<N,RATE>` |
| `xad_ima.inc` | IMA step table and the four `<BPS>` arithmetic helpers |
| `xad_ms.inc` | MS adaptation/coefficient tables, quantizer, `ms_dlog`/`ms_bucket`/`ms_conf`, `MS_M`/`MS_U` |
| `xad_pred.inc` | `Pred<VAR>` — the NLMS cascade |
| `xad_match.inc` | `MatchModel<ORD,HB,TB>` |
| `xad_wav.inc` | `Wav`, `Params`, `parse_wav`, the canonical-header builders, `block_samples`, `analyze` |
| `xad_arc.inc` | `Seg`, `Arc`, `same_type`, `scan_segments`, `write_canon`, `arc_geometry` |
| `xad_codec.inc` | `Codec<MD>` — itself a table of contents for the four below |
| `xad_codec_state.inc` | the context tables, `Ch`, `init` / `reset_stats` / `begin_seg`, the stack probe |
| `xad_codec_bits.inc` | `mbit`, `code_symbol<FMT,BPS>`, `code_signed`, `code_byte`, `code_bytes` |
| `xad_codec_ima.inc` | `code_data` dispatch + `code_data_ima<BPS,XST>` |
| `xad_codec_ms.inc` | `code_data_ms<XST>` |
| `xad_front.inc` | `Xad<MD> : RC<MD>` — header i/o, `do_encode`, `do_decode`, `do_process` |
| `xad_drive.inc` | `XadDrive<Carrier>`, `Menc` / `Mdec`, `report`, `describe` |

The four `xad_codec_*.inc` are included **inside** `Codec<MD>`'s class body, the
way `d5c_codec.inc` includes its parts — member functions stay where their data
is, and there is no second place saying what the class contains.

### Include order, and the two places it is load-bearing

Same class of constraint `dxt5comp_core.inc` documents at its top, and it is
not obvious from the file names, so it is written down in xadpcm.cpp:

1. **`common.inc` -> `coro3b.inc` -> `xad_rc.inc`.** `RC` derives from
   `Coroutine`, and `coro3b.inc` needs common.inc's `ALIGN`, `if_e1` and
   `__assume`. This is why the Lib3 includes are in the .cpp and not inside
   xad_rc.inc: a part that pulls its own prerequisites makes the order depend
   on who included whom first.
2. **`xad_prelude.inc` -> `sh_mapping.inc` -> `MOD/*.inc` -> `xad_idxparam.inc`
   -> everything that reads an IDX value.** The generated headers need
   `pclamp` and the contract constants for their sizes, so the prelude comes
   first; but the idiom in section 3 switches on `USE_NEW`, which *the
   generated headers define*, so it cannot live in the prelude and needs a file
   of its own on the far side of the MOD includes. Splitting those two apart is
   the price of that cycle — the same shape as d5c's counter / counter_upd
   split, and for the same reason. Do not try to merge them.

Everything else is ordinary bottom-up dependency order.

## 2. The last tunable that is not in IDX

A sweep of every integer literal left in the model found exactly one thing that
is a model choice, is safe to sweep, and is not reachable by opt.pl:

**The two match models share one history-ring size and one hash-table size.**

```c
mmS.init(G0_MATCH_ORDER,  G0_MATCH_HB, G0_MATCH_TB);
mmL.init(G0_MATCH_ORDER2, G0_MATCH_HB, G0_MATCH_TB);   // same HB, same TB
```

They already have separate storage (`vector<u8> hist` is per-object) and they do
genuinely different jobs — the short one is a high-order context that fires
constantly, the long one a repeat detector that fires rarely — so there is no
reason a 5-code window and a 63-code window want the same 2 MB ring and the
same 1 M-entry table. Two new Numbers, `MATCH_HB2` and `MATCH_TB2`, seeded to
the same values `MATCH_HB` and `MATCH_TB` have now, so the archive does not
move; what changes is that opt.pl can now separate them.

Everything else that looked like a candidate was checked and deliberately left:

* `MatchModel::B` (0x9E3779B1), `slot()`'s 0x2545F491, `tHJ`'s multiplier —
  hash mixers, not model parameters. Sweeping a multiplicative hash constant
  measures collision luck on the tuning corpus and generalises to nothing.
* `len < 0xFFFF`, `len = order` on a hash hit, `xch = other.cur + 1` — structure.
  `MakeMl` already quantizes the length, and row 0 is reserved for "no
  prediction"; these are the code that makes those true, not knobs.
* `2*base+1` (IMA) and `d+1` (MS) in the confidence divisors — the quantizer
  cell width. It is not a free number, it is *the* number.
* `MS_DLOG_SUB` / `E0` / `EMAX`, the iDelta floor of 16, `MS_DELTA_MAX`,
  `ms_adapt`, `ms_std_coef`, `step_table`, the index-update tables — format
  constants, and README-dual.txt already explains why `MS_DLOG_SUB` in
  particular only looks tunable.
* `nseg > 1<<24`, `nfile > 1<<20`, `ln > 4096`, `XadDrive::bufsize` — sanity
  limits and an i/o window size. Neither is in the model.
* The whole arithmetic-contract enum, for the reason the block's own comment
  gives.

## 3. IDX values as template parameters, in both build modes

The Const build already folds every IDX value, because idx2inc.pl has turned it
into a literal. The Debug build cannot — `G0_MATCH_ORDER` there is
`G0_MATCH_ORDER_.value * 1` off a `mapping` object opt.pl patches at load, so
it is not a constant expression and **cannot be a template argument at all**.
That is what has kept these values out of template parameter lists.

A non-type template parameter of **reference** type fixes it: `const int&` binds
an object with static storage and (since C++11) internal linkage, so
`MatchModel<G0_MATCH_ORDER, ...>` is well-formed even when that object's
initialiser is not constant. Same spelling at the instantiation site in both
modes; only the parameter's declared type differs.

```c
#if USE_NEW                                  // Debug: values are load-time
 #define IDXP(name)      const int& name
 #define IDXC(t,n,expr)  const t n = (expr)
#else                                        // Const: values are literals
 #define IDXP(name)      int name
 #define IDXC(t,n,expr)  static constexpr t n = (expr)
#endif
```

`IDXP` declares a parameter, `IDXC` a class constant derived from one.

### Where the flag comes from

`USE_NEW` is emitted by idx2inc.pl itself, into every generated `_h.inc`, and
`$UseNew = 0` is documented in the generator as *"Table() members are
fixed-size arrays (needs const-expression sizes, i.e. the Const build)"*. So
mk.sh goes back to the rev-016 convention it had drifted from — Debug generates
with `idx2inc.pl foo.idx 1`, release with `... 0` — and the mode flag then
travels **with the generated headers** rather than on the compiler command
line, where it could get out of step with whatever is actually in MOD/. (For
this tree the flag is the *only* difference the two settings make: xadpcm's
.idx files declare no `Table()`s, so the generated code is otherwise identical.
Verified by diff.)

Backstop, because the coupling is a convention of mk.sh and not a law:

```c
#if !USE_NEW
static_assert(G0_MATCH_ORDER >= 0 && M0_Md_Volume > 0 && I0_Ib_Volume > 0,
              "MOD/ says USE_NEW=0 but its parameters are not constant "
              "expressions -- regenerate with ./mk.sh release");
#endif
```

In the Debug shape those operands are not constant expressions, so the build
fails at that line with the comment above it. The failure is one-sided by
design: getting it wrong the *other* way (Const values, reference parameters)
compiles and runs correctly, just without the folding.

### Where the idiom is applied, and what each one buys

| type | parameters | what folds in the Const build |
|---|---|---|
| `Pred<VAR>` | `int VAR` (0 = A cascade, 1 = B) | the 13 shape values become `IDXC` class constants, so **all four NLMS loops get constant trip counts** — `dot` over n1/n2/n3, `dotx` over n4, and the matching `adapt`/`adaptx`. This is the hot loop of the whole codec: two cascades x two channels x (48+192+192+8) taps per sample. |
| `MatchModel<ORD,HB,TB>` | `IDXP` x3 | `order`, the ring/table masks, and `bpow = B^(order-1)` — currently computed by a loop in `init()` — become constants. Needs the reference form: the two instances differ *only* by IDX values. |
| `Mixer<LR,CLAMPL>` | `IDXP` x2 | the learning-rate multiply, and `cl = 1 << pclamp(CLAMPL,8,29)` which `upd()` recomputes on every coded bit. |
| `APM<N,RATE>` | `IDXP` x2 | `ncl = N+1` (a multiply in `pp()` on every coded bit) and the update rate. Two instances, again differing only by IDX values. |

`Pred<VAR>` takes a plain `int` rather than the shape values themselves: VAR is
0 or 1, `IDXC` does the selecting inside, and thirteen reference parameters in
one list would be unreadable for no gain.

Consequences to get right:

* `Ch::pd` / `Ch::pdB` become `Pred<0>` and `Pred<1>` — different types.
  `setvar()` disappears (the shape is the type); `reset(keepw)` stays.
* In the Debug build `IDXC` makes those 13 values `const` **non-static** members,
  initialised when the Codec is constructed rather than by `begin_seg`. That is
  equivalent because they never varied per segment — `begin_seg` called
  `setvar(0)` / `setvar(1)` with the same argument every time. It also makes
  `Pred` non-assignable, which nothing relies on.
* Global-construction order still holds: `Menc`/`Mdec` are declared at the
  bottom of the TU, after the MOD headers, so the `mapping` objects those 13
  values read are constructed first.

### What is deliberately NOT templated

`NCONF`, `MSMAP`, `MSCONF`, `HJ_BITS`, `SGN_*`, `ST_SCALE`, `WSH`, the rate
schedules: they are already plain reads of a value that is a literal in the
Const build, so the code that uses them already folds there. Wrapping them in a
template parameter would fold nothing extra in Const and nothing at all in
Debug, while multiplying instantiations. The four above earn it because in each
case the value is a **loop bound, an array stride or a per-bit multiply** — the
places where a constant changes the shape of the generated code rather than
just saving a load.

## 4. Risk list and the check for each

| risk | check |
|---|---|
| a part is included in the wrong order and silently reads an uninitialised IDX value | the ordering note in xadpcm.cpp, plus byte-identical output — a wrong parameter moves the archive immediately |
| `Pred<0>`/`Pred<1>` split changes the reset or warm-start path | `-s`/`-ss` runs, which are the only ones where `G0_SEG_KEEPW` does anything |
| `MATCH_HB2`/`MATCH_TB2` not seeded exactly equal | byte-identical output; and the two idx lines are diffed against the originals |
| `IDXC` const members change `Pred`'s size or layout in a way that matters | nothing serialises `Pred`; and again, byte-identical output |
| `USE_NEW` disagrees with what MOD/ actually contains | the `static_assert` above, in the dangerous direction only |
| the split introduces a second definition of something | it is one TU; a duplicate is a link error, not a silent bug |
| Const build and Debug build diverge | both are built and compared against the same baselines |

## 5. Verification protocol

```sh
./mk.sh && ./t.sh                 # Debug:   roundtrip + sizes, both corpora
python3 gen_testwavs.py && ./t.sh gen/*.wav
# every archive above compared byte-for-byte with the rev-016 baseline
./mk.sh release && <the same again>
# and the two builds' archives compared with each other
```

Plus, as after Plan 1: `nm` for out-of-line `mbit` copies, and `-v` for the
coroutine stack high-water mark, since both move when inlining decisions do.

---

## 6. Review of this plan — errors found and corrected

**6.1 The idiom cannot live in the prelude (fixed above).** As first written,
section 1 put the `IDXP`/`IDXC` macros in `xad_prelude.inc` and section 3 said
they had to sit "textually after the MOD includes" — which is impossible for a
file that is included *before* them. There is a genuine cycle here: the
generated headers need `pclamp` and the contract constants, and the idiom needs
`USE_NEW`, which those headers define. It takes two files, and `xad_idxparam.inc`
is now the second one.

**6.2 Cut the file textually; do not relocate constants.** The first draft
moved `MAX_CHANS` and `MS_MAXCOEF` into the contract enum on the grounds that
README-dual.txt lists them there. That is tidier and strictly riskier: every
move is a chance to put a definition after its first use, and the compiler
catches only some of those (a `static const int` read before its dynamic
initialiser runs is not a diagnostic, it is a zero). The split is therefore a
pure cut along the existing section-comment boundaries, and the only
declarations that move at all are the ones that must (`NCONF` and `verbose`,
which have to land on the far side of the MOD includes). Checked: with a pure
cut, every definition still precedes every use — `MS_MAXCOEF` (MS section) is
used by `Params::coef[]` in the codec section, `MAX_CHANS` (wav section) by
`analyze` and by `Codec::ch[]`, both later.

**6.3 `same_type`'s forward declaration becomes dead.** It exists only because
the current file defines `same_type` 900 lines after the `Params` that names
it. After the split, `xad_arc.inc` defines it and only `xad_front.inc` — which
comes later — calls it. The declaration goes; leaving it would be a second
place stating the signature.

**6.4 `bpow` is the point of templating `MatchModel`, not `order`.** The plan
said "`bpow` — currently computed by a loop in `init()` — becomes a constant",
which understates why that matters: `init()` runs once, so folding the *loop*
saves nothing measurable. What matters is that `bpow` stops being a member
*load* in `push()`, which runs once per coded symbol. So it must be declared
with `IDXC` (a `static constexpr` in the Const build), not merely computed from
a constant — and `B` has to become `static constexpr` for that to be legal.
The same applies to `hmask`/`tmask`: derived with `IDXC` from `HB`/`TB` rather
than read back from `hist.size()-1` after the allocation.

**6.5 `Pred::init(int)` is dead code.** `begin_seg` calls `setvar()` and
`reset()` directly and nothing calls `init()`. It is removed along with
`setvar()` rather than left behind as an `init()` that takes no argument.

**6.6 The Debug build must be measured, not assumed unchanged.** A `const int&`
template parameter is a load through a reference where the old code read a
global directly. That should be identical after the optimizer hoists it, but
"should be" is how the `mbit` inlining regression in `d5c_prelude.inc` happened.
So the Debug build is timed against the pre-refactor binary, not just checked
for correctness.

**Verified rather than assumed, before writing any code:**

* `template<const int& N>` instantiated with a `static const int` whose
  initialiser reads a `volatile` member — i.e. exactly the Debug shape — compiles
  and runs on g++ 13.3 at `-std=gnu++20 -O2 -fno-exceptions -fno-rtti`.
* The same template with a value parameter and a Const-shape `static const int`
  folds completely: the constructor loop that computes `bpow` disappears from
  the assembly (0 multiplies, against 1 for the reference form), and the value
  parameter is usable as an array bound and in an enum initialiser.
* `USE_NEW` is emitted by idx2inc.pl into every `_h.inc` and is the **only**
  difference between `idx2inc.pl foo.idx 1` and `... 0` for this tree — xadpcm's
  .idx files declare no `Table()`s, so the `_Size` accounting the two modes
  differ over is trivially zero either way. Confirmed by diffing all four
  generated headers, and by rebuilding with the `UseNew=1` set and checking the
  archives are still byte-identical.
* Detecting Const-ness *automatically* instead — `requires { ce_tag<G0_X>; }`,
  with and without a lambda to make the expression dependent — does **not**
  work: g++ 13 reports "the value of X is not usable in a constant expression"
  as a hard error rather than a substitution failure. Hence the `USE_NEW`
  route, which needs no detection at all.

**6.7 The stack probe was in the wrong place, and ASan cannot check this at
all.** Plan 1 put the high-water probe in `code_symbol`, which is several
frames above any yield — it measures what the model has on the stack, not what
`yield()` copies. It now lives in `RC::shift_low` and `RC::rd`, plus the
frontend's `putn`/`slurp`, which between them are the only places in the
program that touch a pin and therefore the only places that can yield. Exact
readings: **2,161 bytes encoding IMA, 1,777 encoding MS, 1,729 / 1,329
decoding** — against `STKPAD` = 65,560. Cross-checked statically with
`-fstack-usage`: the largest frame anywhere on the copied side is 976 bytes
(`do_encode`), and the 262,176-byte frame the listing shows for
`call_do_process0` is Lib3's `STKPAD0` pad, which sits *above* `stkptrH` and is
by construction not part of what gets copied.

Worth writing down because the obvious way to check this does not work:
**ASan's stack instrumentation is incompatible with a stack-copying coroutine**,
and it fails in a way that looks exactly like the bug you would be hunting.
`yield()` computes `stkptrH - stkptrL` across two frames; under
`-fsanitize=address` those frames are relocated into ASan's fake-stack region,
so the difference becomes nonsense and the `memcpy` is reported as a
"stack-buffer-underflow" reading hundreds of gigabytes. Reproduced with a
twenty-line coroutine that has no locals at all: delta 25 bytes uninstrumented,
the same multi-gigabyte fault under ASan. So ASan is run with
`--param asan-stack=0`, which keeps heap and global checking (which is where
this program's buffers actually live) and drops only the part that cannot work
here. UBSan needs no such treatment and runs clean over the whole synthetic
corpus.
