# IDX Format Specification

IDX is a small language for declaring the two things a context-mixing compressor
is made of: **context indices** (how a handful of variables become one table row)
and **tunable parameters** (every number an optimizer is allowed to move).
`IDX/idx2inc.pl` turns a `.idx` declaration plus a `.inc` template into C++.

The point is that one source produces both a build whose every knob is a live,
patchable object an optimizer can hill-climb, and a build where the same knobs
are literals the compiler folds — with the *using* code spelled identically in
both. Most of this document is the mechanics; §7–§12 are what that costs and how
the two projects using it (`xadpcm`, `dxt5comp`/`ddsdet`) pay it.

---

## 1. The two axes

`Debug` and `Const` are **orthogonal flags, not two modes.** Describing them as
alternatives is the single most misleading thing one can say about this format,
so:

| flag | question it answers | `0` | `1` |
|---|---|---|---|
| `Const` | is the value a literal? | a `mapping` object plus `static const int N = N_.value * mult` — a **runtime** value | `static const int N = (pattern+base) * mult` — a **constant expression** |
| `Debug` | can `opt.pl` see this knob? | `mdesc(...)` / `mmask(...)` / `mmask2(...)` — the same object, but its descriptor string has no `!MAP!` marker, so the optimizer's scan never finds it | `pdesc(...)` / `pmask(...)` / `pmask2(...)` — the pattern string is embedded in the binary and the optimizer flips its bits |

`Debug` only means anything when `Const 0`: once the value is folded there is no
object to patch and nothing to find.

That gives three shapes worth naming, of which the tree uses two:

| shape | flags | what it is for |
|---|---|---|
| **tuning** | `Debug 1, Const 0` | `./mk.sh`. Every knob live; `opt.pl` hill-climbs it |
| **shipping** | `Const 1` | `./mk.sh release`. Every knob folded; no indirection, no mapping objects |
| frozen | `Debug 0, Const 0` | per-line, to shrink the optimizer's search space — see §10 |

`mk.sh` derives the shipping build from the same source with one substitution, so
the two can never drift:

```sh
sed 's/^Const 0/Const 1/' "$f" > "IDX/$b-const.idx"
( cd IDX && perl idx2inc.pl "$b-const.idx" 0 )
```

**Both builds must produce the same stream.** That is the contract, and in
`xadpcm` it is a test: `md5s.sh` is run under both and the 24 checksums compared.

---

## 2. Global directives

```
Prefix <name>      # prefix for every generated identifier: G0 -> G0_MLR
Debug <0|1>        # default for the lines that follow
Const <0|1>        # default for the lines that follow
```

## 3. Line modifiers

- `!` at line start — toggle `Debug` for this line (see §10)
- `-` at line start — toggle `Const` for this line
- `#` — comment

---

## 4. Context indices

### Index declaration

```
Index <name>
```

Begins an index. Every mapping line after it contributes one factor, and the
generator emits both the builder and the total row count.

### Threshold mapping — `tag: var, base!pattern`

```
m0: i, 1!111111111111111
```

Each `1` in the pattern places a threshold at `j + (base-1)`, so `1!111` is
`(var>0) + (var>1) + (var>2)` — four buckets. Widening the pattern widens the
range; the seeded value is what the bits currently spell.

```c
pdesc( D0_m0, 1, "111111111111111" );      // Const 0
```

### Bitmask mapping — `tag: var, &pattern`

Keeps the `1` positions and packs them contiguously.

```
m3: m1, &00000011        // low 2 bits
tp: prev, &11111001      // 6 of the 8 bits of prev -> 64 rows, not 256
```

The second is from `ddsdet`'s tail model and shows what the form is *for*: an
order-1 byte context does not have to be 256 rows wide. Dropping two middle bits
of the predecessor trades resolution for adaptation speed, and which wins is a
question for the optimizer rather than for the author.

```c
pmask( D0_m3, "00000011" );                // Const 0
index = (index << 2) + ((var) & 3);        // Const 1
```

### Incremental context mask — `tag: var, b&pattern`

For a context accumulated a bit at a time behind a leading `1`:

```c
ctx = 1;  ctx += ctx + bit;  ...
```

The mask selects the last N accumulated bits *whatever the current depth*, which
a plain `&` cannot do because the leading 1 moves. Uses `pmask2` / `masking_b`.

### Direct addition — `ADD <multiplier>: <expression>`

```
ADD 2: OldLR      ->    Cont = Cont*(2) + (OldLR);
```

For a factor that is already a small dense integer and needs no quantizer.

---

## 5. Parameters

```
Number <name>, <multiplier>, <base>!<pattern>
Rate   <name>, <base>!<pattern>       # index into TTable[]
Rate1  <name>, <base>!<pattern>       # inverse rate, eSCALE / value
```

`Number STEP, 1, 1!01001` is `(0b01001 + 1) * 1` = 10.

```c
// Const 0
pdesc( G0_STEP_, 1, "01001" );
static const int G0_STEP = G0_STEP_.value * (1);
// Const 1
static const int G0_STEP = (9+1) * (1);
```

**Consumers must clamp.** A pattern is a search space and the optimizer will
visit the ends of it, so anything used as a size, a shift or a divisor needs a
range check at the point of use — `xadpcm` has `pclamp(G0_ST_SCALE, 16, 2048)`,
`PMUL(x)`, `PAPN(x)` for exactly this. A parameter that can crash the program
when swept is a bug in the *consumer*, not in the sweep.

---

## 6. Templates

The `.inc` template is copied through with substitutions. `%M%` becomes the
prefix plus underscore.

| marker | expands to |
|---|---|
| `MakeTables` | the mapping/mask/constant declarations (normally routed to `_h.inc`) |
| `MakeIndex <name>` | the index-building statements |
| `Table( T, %M%N, size )` | storage — see §9 |
| `def_Data … end_Data` | verbatim into the generated struct |
| `def_Init … end_Init` | verbatim into `%M%_Init()` |

For each `Index` a row count is emitted:

```c
static const int D0_Cont_Volume = 1* D0_m0.Size* D0_m1.Size* ... * 2;
```

In the shipping build every factor is a literal, so `_Volume` is a constant
expression and can size an array or be a template argument. In the tuning build
it is a product of member reads — usable as a size at runtime, and *not* as a
constant expression. §8 is about the consequences.

---

## 7. The generated struct, and how the mode flag travels

`idx2inc.pl` emits, into every `_h.inc`:

```c
struct G0_T {
#undef USE_NEW
#define USE_NEW 1        // or 0
  ...                    // def_Data, Table() members
  void G0_Init( void );  // allocations + def_Init
  void G0_Quit( void );  // deallocations
};
```

`USE_NEW` is chosen by the generator's second argument (`idx2inc.pl foo.idx 0`),
not by the compiler command line, **and that is deliberate**: the flag is written
into the header it describes, so it cannot drift out of step with whatever is
actually in `MOD/`. Code downstream tests `USE_NEW` and gets an answer about the
headers it is compiled against, not about what someone meant to build.

`mk.sh` ties `USE_NEW=1` to the tuning build and `0` to the shipping build,
because `Table()`'s fixed-array form needs constant-expression sizes and only the
shipping build has them. That coupling is a convention, so §8 backs it with a
`static_assert`.

---

## 8. IDX values as template parameters — `IDXP` / `IDXC`

*New in `xadpcm`; the trick that let the codec's hot loops take their shape from
IDX rather than from `#define`s.*

**The problem.** In the shipping build `G0_MATCH_ORDER` is a literal and can be a
template argument like any other integer. In the tuning build it is
`G0_MATCH_ORDER_.value * 1` — a read from a patchable object, not a constant
expression, and therefore not a template argument at all. That kept every IDX
value out of parameter lists, which meant the parameters shaping the innermost
loops could not fold in the build that ships.

**The fix: a non-type template parameter of *reference* type.**

```c
#if USE_NEW
 #define IDXP(name)       const int& name
 #define IDXC(t, n, expr) const t n = (expr)
#else
 #define IDXP(name)       int name
 #define IDXC(t, n, expr) static constexpr t n = (expr)
#endif
```

`const int&` binds an object with static storage duration and internal linkage
regardless of how that object was initialised. So

```c
template<IDXP(ORD), IDXP(HB), IDXP(TB)> struct MatchModel { ... };
MatchModel<G0_MATCH_ORDER, G0_MATCH_HB, G0_MATCH_TB> mmS;
```

is well-formed in **both** builds and **spelled identically** at every
instantiation site. Only the parameter's declared type differs, and only the
shipping build folds — which is the build that matters.

`IDXC` covers the follow-on: a class constant derived from such a parameter is a
`static constexpr` where the operands are literals and a const member initialised
at construction where they are not.

### Two consequences worth knowing before you use it

**Internal linkage propagates.** Instantiating on a `static const int` gives the
instantiated type internal linkage, and GCC's `-Wsubobject-linkage` warns when a
class has a member of such a type. The warning is correct in general — several
translation units would each get a different type under one name. It is vacuous
for a single-translation-unit program, which is why `xadpcm` suppresses it with
that reasoning written down rather than silently:

```c
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wsubobject-linkage"   // clang has no such warning
#endif
```

**The `USE_NEW` coupling deserves a backstop.** If `MOD/` says `USE_NEW=0` while
actually holding tuning-mode mapping objects, `IDXP` declares plain `int`
parameters and the arguments are not constant expressions. The failure is
one-sided by design — the other mistake (literals bound to reference parameters)
compiles and runs correctly, merely without folding — so one assertion catches
the bad direction:

```c
static_assert(G0_MATCH_ORDER>=0 && G0_MLR>=0 && M0_Md_Volume>0 && I0_Ib_Volume>0,
              "MOD/ declares USE_NEW=0 but its parameters are not constant "
              "expressions -- regenerate it with ./mk.sh release");
```

---

## 9. `Table()` in practice

```
Table( Ctr, %M%tTail, %M%TL_Volume * 256 );
```

| `USE_NEW` | member | allocation |
|---|---|---|
| `0` | `Ctr G0_tTail[G0_TL_Volume*256];` | none — it is the object |
| `1` | `Ctr* G0_tTail;` | `anew<Ctr>(tbl_n(size), "G0_tTail")` in `%M%_Init()` |

Three details the generator handles that are easy to get wrong by hand:

* **`tbl_n(size)` bounds-checks the count** so the compiler can prove the byte
  product cannot overflow. Without it GCC 13+ raises
  `-Walloc-size-larger-than` on *its own* overflow guard, for every such line.
* **No `()` on the `new[]`.** Every table is filled by the codec's `init()`
  before it is read, and value-initialising here would be a second pass over
  hundreds of megabytes. It also keeps the two builds' semantics identical: in
  the fixed-array form these are members of a default-initialised object and are
  equally indeterminate until `init()` runs.
* **`%M%_Size` accumulates the module's table bytes.** A `constexpr` sum in the
  shipping build; accumulated in `_Init()` in the tuning build, where the counts
  are runtime values.

### When `Table()` is the wrong tool

`Table()` exists for storage whose size is an IDX `_Volume`. It cannot express a
size that comes from the *data*. `xadpcm` uses `MakeIndex` from four IDX modules
and no `Table()` at all, because every one of its tables is sized from the audio
geometry (`1<<bps`, `block_align`, channel count) rather than from a Volume — so
it sub-allocates from one `mmap`'d arena instead, sized by a dry measuring pass.
`dxt5comp` is the other way round: its geometry *is* the Volumes, and it uses
`Table()` throughout. Neither is the general answer; the question is only whether
the size is a Volume.

---

## 10. Freezing a knob

`opt.pl`'s cost is proportional to the total pattern bits it can see, and a
mature `.idx` runs to a thousand of them. A `!` at the start of a line toggles
`Debug` for that line:

```perl
$debug = s/^\!//;  $debug ^= $gdebug;     # idx2inc.pl
```

so with `Debug 1` in force a `!` line emits the `m`-prefixed macro instead of the
`p`-prefixed one — `mdesc` for `Number`/`Rate`/`Rate1` and for a threshold
mapping, `mmask` for `&`, `mmask2` for `b&`. The object built is identical; only
the descriptor string differs, and only in that it does not begin with `!MAP!`.
`opt.pl` scans the executable for that marker, so a frozen knob is simply not in
the search space. The value compiles in exactly as it was.

```
!Number HW0, 1, 0!1111111111111111     # frozen: block-header rate schedule
```

Verified end to end: freezing `MLR` takes the marker count from 134 to 133,
`!MAP!G0_MLR_` disappears from the binary, and the corpus codes byte-for-byte
identically — which is the point, since the value has not moved.

> **The three `m*` macros were missing.** `sh_mapping.inc` defined only `pdesc`,
> `pmask` and `pmask2` — in this tree, in `dxt5comp`'s, and in the oldest copy of
> the file available. A `!` line therefore emitted a call to an undeclared macro
> and the build failed on the next line, which is why no `.idx` in either project
> had ever carried one: the feature was described in the `.idx` comments and had
> never been exercised. The three definitions are now in `sh_mapping.inc`.
>
> Their one subtlety: every constructor does `S += strlen(S) + 1` to step over
> the descriptor and reach the pattern, so a frozen macro still has to supply
> **two** NUL-separated strings — and the pattern must stay a separate literal
> after the `"\x00"`, because `\x` escapes are greedy in hex digits and
> `"...\x00" Map` written as one literal would swallow the pattern's leading
> zeroes into the escape.

Freeze what has little leverage on the corpus in front of you. `xadpcm`'s
`.idx` recommends the block-header and literal rate schedules, which together
govern under 1% of a typical output, so the search spends its time on the code
schedule and the predictor instead.

---

## 11. The tuning loop

```
./mk.sh                                   # tuning build: Debug 1, Const 0
perl IDX/opt.pl opt.lst                   # hill-climb; writes export.!!!
cd IDX && for f in *.idx; do              # fold the results back into source
  perl import.pl $f ../export.!!! > t && mv t $f
done
./mk.sh          # continue tuning
./mk.sh release  # or ship
```

`opt.pl` does not parse or rebuild anything. The `pdesc` macro embeds each
pattern in the executable as a `"!MAP!name!base\0pattern"` string; the optimizer
flips bits **in the binary**, re-runs the corpus, and keeps what shrinks the
total. `export.!!!` accumulates the winners as `mdesc` lines, which `import.pl`
matches back onto the `.idx` sources by name.

Two practical notes carried in `opt.pl`'s own header:

* **Optimize on a corpus, not a file.** One file overfits it.
* **Cover every model.** Formats with separate context tables need
  representatives of each, or the untouched one drifts while nothing measures it.

---

## 12. Gotchas

**The include-order cycle is real.** The generated headers need the consumer's
helpers (`pclamp`, the arithmetic constants) to compute their sizes; the
consumer's IDX bridge needs `USE_NEW`, which only the generated headers define.
That is a cycle unless the consumer's prelude is split in two — everything the
generated headers need *before* them, everything that depends on them *after*.
`xadpcm` splits it as `xad_prelude.inc` / `xad_idxparam.inc` and says so in both
files, because merging them is the obvious-looking simplification that cannot
work.

**A generated `MOD/` is a build input, not a build artefact.** It ships generated
so the tree builds without perl, and `mk.sh` regenerates it when perl is present.
A stale `MOD/` compiles fine and codes differently.

**Clamp at the point of use, not in the `.idx`.** The pattern is the search
space; the clamp is the contract. See §5.

**The two builds must agree, and that is testable.** Whatever the parameters are,
tuning and shipping builds must produce identical output. Run the corpus under
both and compare checksums; it is the only check that catches a parameter that
folds to something different from what it evaluated to.

---

## 13. Runtime support

From `sh_mapping.inc`:

| class | role |
|---|---|
| `mapping` | threshold quantization, with the profiling hooks `opt.pl` needs |
| `masking` | bitmask extraction |
| `masking_b` | bit-grouped mask, for incrementally accumulated contexts |

| macro | emits |
|---|---|
| `pdesc(name, base, pattern)` | a tunable mapping — pattern visible to `opt.pl` |
| `pmask(name, pattern)` | a tunable bitmask |
| `pmask2(name, pattern)` | a tunable bit-grouped mask |
| `mdesc` / `mmask` / `mmask2` | the same three, frozen — no `!MAP!` marker, so invisible to `opt.pl` (§10) |

## 14. Output files

For `xad-G0.idx` + `xad-G0.inc`:

| file | holds |
|---|---|
| `xad-G0_h.inc` | constants, mapping declarations, `_Volume`s, the `G0_T` struct with `Init`/`Quit`, and the `USE_NEW` define |
| `xad-G0_p.inc` | the template with `MakeIndex` expanded |

---

## 15. Worked example

### `sh_model-D0.idx`

```
Prefix D0
Debug 1
Const 0

Index Cont
 m0: i,       1!111111111111111
 m1: k,       1!1
 m2: j,       1!1
 m3: m1,      &00000011
 m4: OldVal,  1!1
 ADD 2: OldLR
```

### `sh_model-D0_h.inc` (tuning build)

```c
pdesc( D0_m0, 1, "111111111111111" );
pdesc( D0_m1, 1, "1" );
pdesc( D0_m2, 1, "1" );
pmask( D0_m3, "00000011" );
pdesc( D0_m4, 1, "1" );

static const int D0_Cont_Volume = 1* D0_m0.Size* D0_m1.Size* D0_m2.Size* D0_m3.Size* D0_m4.Size* 2;
```

### `sh_model-D0_p.inc`

```c
uint MakeCont( uint OldLR, uint OldVal, uint m1, uint j, uint k, uint i ) {
  int Cont = 0;
  D0_m0.inc( Cont, i );
  D0_m1.inc( Cont, k );
  D0_m2.inc( Cont, j );
  D0_m3.inc( Cont, m1 );
  D0_m4.inc( Cont, OldVal );
  Cont = Cont*(2) + (OldLR);
  return Cont;
}
```

In the shipping build the same two files hold literals and inline shifts, and
`MakeCont` compiles to straight-line arithmetic with no member reads at all —
while the call site, and every template instantiated on a `D0_*` value, is
character-for-character the same source.
