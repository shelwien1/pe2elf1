# IDX Format Specification

(h264p's document, as it is: the format and its tools are the same here. The projects it names
besides h264p are where the format came from; m4vpack's own use is in its README, "Tuning", and in
`mk.sh`.)

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
| `Debug` | can `opt.pl` see this knob? | `mdesc(...)` / `mmask(...)` — the same object, but its descriptor string has no `!MAP!` marker, so the optimizer's scan never finds it | `pdesc(...)` / `pmask(...)` — the pattern string is embedded in the binary and the optimizer flips its bits |

`Debug` only means anything when `Const 0`: once the value is folded there is no
object to patch and nothing to find.

That gives three shapes worth naming, of which the tree uses two:

| shape | flags | what it is for |
|---|---|---|
| **tuning** | `Debug 1, Const 0` | `./mk.sh tuning L`. Every knob live; `opt.pl` hill-climbs it |
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

### Thresholds or a mask, for a small integer

A dense axis of width 2^k is exactly a mask of k ones (`fld: fld, &11111`),
but for a small dense integer -- an id, a kind -- the threshold form is the
better choice, and by some distance:

| | groupings reachable | out of range | buckets for 13 ids |
|---|---|---|---|
| `&11111` | 32, and only strided — dropping bit *k* merges values differing in bit *k* | wraps: id 32 aliases onto id 0 | 32 |
| `1!` ×13 | 2^12 — a boundary between any two adjacent ids, so any run can be merged and any id isolated | saturates into the last bucket | 14 |

The mask's one advantage is those strided merges, which a threshold list cannot
express — and over an alphabet whose numbering is arbitrary, as an id's is,
there is no reason to want them. Everything else favours thresholds: finer,
smaller, and it cannot index off the end. `tsvcomp` measured it — a single
greedy pass over the threshold form found merges worth 6 bytes on the smallest
file in its corpus, and every one of them merged *specific adjacent* field ids,
which is exactly what a mask has no way to say.

A pattern need not be longer than its variable's range: a threshold past the
largest value never fires, and only gives `opt.pl` bits to flip for nothing
(h264p's `fld` of the nal family has 18 values, and 17 bits).

(The library's other forms -- `b&` masks of incrementally accumulated
contexts, `ADD` factors with no mapping behind them, `Rate`/`Rate1` -- are
not in this generator: h264p's IDX has none of them, and a line the
generator does not know is an error.)

---

## 5. Parameters

```
Number <name>, <multiplier>, <base>!<pattern>
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

The `.inc` template is copied through into `_p.inc` with substitutions:
`%M%` becomes the prefix plus underscore, `%F%` the family's name (the
prefix after its first `_`: `H9_skp` gives `skp`). The template of
`foo.idx` is `foo.inc`, or else `family.inc` beside it, one template for
every `.idx` of the directory: h264p's levels have one each,
`IDX/l<L>/family.inc`, since a level's families differ only in their names.

| marker | expands to |
|---|---|
| `MakeIndex <name>,...` | the index-building statements |
| `Table( T, %M%N, size )` | storage, lifted out into the struct of `_h.inc` — see §9 |

Every other line is copied as it is (`#define` and `#include` lines too:
h264p's templates end with their family's configuration class).

For each `Index` a row count is emitted:

```c
static const long long D0_Cont_Volume =
  tc_vmul(tc_vmul(tc_vmul(1, D0_m0.Size), D0_m1.Size), ... );
```

In the shipping build every factor is a literal, so `_Volume` is a constant
expression and can size an array or be a template argument. In the tuning build
it is a product of member reads — usable as a size at runtime, and *not* as a
constant expression. §8 is about the consequences.

**The type is `long long`, and the product goes through a saturating multiply
the host program must supply:**

```c
constexpr long long TC_VOLMAX = (long long) 1 << 60;
constexpr long long tc_vmul(long long a, long long b) {
  return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b;
}
```

A Volume is a product of factor sizes, and whatever sizes a table multiplies it
again — by the node count, and for an APM by the curve width. Two dozen factors
is not many when each is a `mapping` an optimizer can widen a bit at a time, and
in `int` that product wraps. The wrap that lands *positive* is the dangerous
one: a bounds check downstream is then satisfied by the wrong number, a table
far too small is allocated, and the coder indexes off the end of it with nothing
said. Saturating instead means the number that reaches the check is at least as
large as what was asked for, so an index that asks for more than the machine has
is refused by name rather than mis-sized. `_Volume` must therefore never be
narrowed back to `int`, and anything that multiplies one should use `tc_vmul`
too rather than a bare `*`.

---

## 7. The generated struct

`idx2inc.pl` emits, into every `_h.inc`:

```c
struct G0_T {
  ...                    // the Table() members
  void G0_Init( void );  // maps the tables
  void G0_Quit( void );  // unmaps them
};
```

The generator's second argument (`idx2inc.pl foo.idx 0`) chooses how the
`Table()` members are stored (§9): `1`, the default, pointers that `_Init()`
maps and `_Quit()` unmaps; `0`, arrays, members of the struct, with empty
`_Init()` and `_Quit()`. `mk.sh` passes `1` for the tuning build and `0` for
the shipping build, because the arrays need constant-expression sizes and only
the shipping build has them.

### Several modules in one program

Because the struct, its `_Init`, its Volumes and its mapping objects are all
named from `Prefix`, one program can hold as many modules as it has models —
`xadpcm` builds its index from four, and `tsvcomp` has one per family
(`IDX/tsvcomp-dig.idx`, `-sgn`, `-flr`, `-cls`, `-aux`, `-hdr`). Each is a
whole, independent generator input: its own `.idx`, its own `.inc` template,
its own `_h.inc`/`_p.inc` pair in `MOD/`, its own `%M%_T` object to map and
`_Init`. What was one 900-line file becomes six that can be read, and a
`Number` or a threshold is now named relative to its family.

Two things to get right when splitting one module into several:

* **Put the family in the `Prefix`, not in the names.** `Prefix TC_dig` with an
  index called `a` generates `TC_dig_a_Volume` — exactly what `Prefix TC` with
  an index called `dig_a` generated. Every emitted identifier stays as it was,
  which matters because those identifiers are what `opt.pl` finds in the
  executable and what an `export.!!!` is keyed by: get this right and a tuning
  export from before the split still imports afterwards.
* **No table may be called `T`.** The struct is `%M%_T`, so a `Table()` named
  `%M%T` is a member with its own class's name, which is ill-formed. `tsvcomp`
  renamed its mantissa plane to `%M%MT` for this and nothing else.

`import.pl` takes one `.idx` at a time, so importing an export across a split
model is a loop over the `.idx` files (`mk.sh`'s header has it).

h264p goes one step further: one set of modules per level of its model,
`IDX/l<L>/<family>.idx` with `Prefix H<L>_<family>`, generated into
`MOD/l<L>/`, so that a level's contexts and parameters are edited and tuned
without touching another's. The level in the `Prefix` keeps every name,
`!MAP!` pattern and export line apart, and `import.pl` folds an export back
only into the level it came from; `mk.sh tuning L` builds a binary of one
level alone, so that its patterns are all `opt.pl` finds.

---

## 8. IDX values as template parameters

In the shipping build every value is a literal and can be a template argument
like any other integer; in the tuning build it is a read of a patchable
object, which cannot. h264p's families take theirs through a class per level
and family (`famcfg.inc`, h264p_levels.md §7), whose members are the
generated constants, clamped, and which `tc_fam<f_DEC, C>` takes as `C`: the
family's code reads `C::lr`, `C::kM`, ... the same way in both builds, and
the shipping build folds them. (oggcomp's `IDXP`/`IDXC` macros did the same
with reference-typed template parameters, keyed on a `USE_NEW` define the
generated headers carried; h264p uses neither, and the generator writes no
such define.)

---

## 9. `Table()` in practice

```
Table( Ctr, %M%tTail, %M%TL_Volume * 256 );
```

| UseNew | member | allocation |
|---|---|---|
| `0` | `Ctr G0_tTail[G0_TL_Volume*256];` | none — it is a member of the module object |
| `1` | `Ctr* G0_tTail;` | `tc_map(sizeof(Ctr) * tbl_n(size))` in `%M%_Init()`, released by `tc_unmap` in `%M%_Quit()` |

Three details the generator handles that are easy to get wrong by hand:

* **`tbl_n(size)` bounds-checks the count** so the compiler can prove the byte
  product cannot overflow. Without it GCC 13+ raises
  `-Walloc-size-larger-than` on *its own* overflow guard, for every such line.
* **The host supplies the allocator, and it should map rather than allocate.**
  `tsvcomp`'s `tc_map` is `mmap(MAP_NORESERVE)` (`VirtualAlloc` on Windows), so
  a table costs address space when it is made and memory only where the codec
  reaches it. This is not a detail: an index whose factors multiply out to tens
  of gigabytes is *ordinary* once an optimizer has been at it, and such an index
  codes a real file in a few hundred megabytes resident, because a link visits
  a rounding error's worth of the rows it can address. Anything that touches
  the whole table — `new[]` and a `memset`, or a `.bss` the loader must commit —
  makes a table too large to fill a table too large to have, and puts most of
  the search space out of reach. It also keeps the two builds' memory behaviour
  identical, which is what lets `./mk.sh check` compare them on a set neither
  could allocate outright.
* **`%M%_Size` accumulates the module's table bytes.** A `constexpr` sum in the
  shipping build; accumulated in `_Init()` in the tuning build, where the counts
  are runtime values. It is address space, not memory: a cap on it is a sanity
  bound on how far a search has wandered, not a statement about the machine.

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
`p`-prefixed one — `mdesc` for `Number` and for a threshold mapping, `mmask`
for `&`. The object built is identical; only the descriptor string differs,
and only in that it does not begin with `!MAP!`.
`opt.pl` scans the executable for that marker, so a frozen knob is simply not in
the search space. The value compiles in exactly as it was.

```
!Number HW0, 1, 0!1111111111111111     # frozen: block-header rate schedule
```

Verified end to end: freezing `MLR` takes the marker count from 134 to 133,
`!MAP!G0_MLR_` disappears from the binary, and the corpus codes byte-for-byte
identically — which is the point, since the value has not moved.

> **The `m*` macros were missing.** `sh_mapping.inc` defined only the `p*`
> ones — in this tree, in `dxt5comp`'s, and in the oldest copy of the file
> available. A `!` line therefore emitted a call to an undeclared macro and
> the build failed on the next line, which is why no `.idx` in either project
> had ever carried one: the feature was described in the `.idx` comments and
> had never been exercised. The definitions are in `mapping.inc` now.
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
schedule and the predictor instead. And freeze what the program never reads:
h264p's families that code with `bit()` alone have no mantissa, no hint and
no signs, so their rates and indices for those are frozen at zero, each
`.idx` saying why.

---

## 11. The tuning loop

```
./mk.sh tuning L                          # tuning build of level L: Debug 1, Const 0
perl IDX/opt.pl opt.lst ../h264p          # hill-climb; writes export.!!!
cd IDX/lL && for f in *.idx; do           # fold the results back into source
  perl ../import.pl $f ../../export.!!! > t && mv t $f
done
cd ../.. && ./mk.sh mod && ./mk.sh check L   # MOD/ again, and the two builds agree
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

**Include order.** The generated headers need the consumer's helpers (the
clamps, `tc_rows`, `tc_vmul`, the node counts) to compute their sizes, so those
come before them; whatever reads the generated names comes after.

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

From `mapping.inc` (the library's `sh_mapping.inc`, cut to what the generator
emits):

| class | role |
|---|---|
| `mapping` | threshold quantization |
| `masking` | bitmask extraction |

| macro | emits |
|---|---|
| `pdesc(name, base, pattern)` | a tunable mapping — pattern visible to `opt.pl` |
| `pmask(name, pattern)` | a tunable bitmask |
| `mdesc` / `mmask` | the same two, frozen — no `!MAP!` marker, so invisible to `opt.pl` (§10) |

## 14. Output files

For `xad-G0.idx` + `xad-G0.inc`:

| file | holds |
|---|---|
| `xad-G0_h.inc` | constants, mapping declarations, `_Volume`s, the `G0_USES` list, the `G0_T` struct with `Init`/`Quit` |
| `xad-G0_p.inc` | the template with `MakeIndex` expanded |

### The variables the indices read: `%M%_USES`

`idx2inc.pl` also writes, after the Volumes, the names its mapping lines
read, in the order they first appear, as an X-macro:

```c
#define H1_skp_USES(X) X(st) X(a3) X(b3) X(c9) X(eq) X(lf) X(af) X(cd) X(cnt) X(c1) X(col) X(run) X(k9)
```

so that the program can compute only those (h264p: `famcfg.inc` turns the
list into a bit per field of the family's variables, and the model asserts
at compile time that a level reads nothing its parts cannot make). In the
folded build a line counts when its pattern has a 1 (a line of zeros adds
nothing to its index and is left out already); in the tuning build also
when it is live, since `opt.pl` may set any of its bits. Of an expression
(a mapping of one), every name not called counts.

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
 m5: OldLR,   &1
```

### `sh_model-D0_h.inc` (tuning build)

```c
pdesc( D0_m0, 1, "111111111111111" );
pdesc( D0_m1, 1, "1" );
pdesc( D0_m2, 1, "1" );
pmask( D0_m3, "00000011" );
pdesc( D0_m4, 1, "1" );
pmask( D0_m5, "1" );

static const long long D0_Cont_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(
  1, D0_m0.Size), D0_m1.Size), D0_m2.Size), D0_m3.Size), D0_m4.Size), D0_m5.Size);
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
  D0_m5.inc( Cont, OldLR );
  return Cont;
}
```

In the shipping build the same two files hold literals and inline shifts, and
`MakeCont` compiles to straight-line arithmetic with no member reads at all —
while the call site, and every template instantiated on a `D0_*` value, is
character-for-character the same source.
