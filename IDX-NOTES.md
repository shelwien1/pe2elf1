# bmf and IDX

`IDX-FORMAT.md` is the format. This is what it means for this codec: which
numbers moved, how they get back out, and the things that went wrong on the way
in. Read the format first; nothing here restates it.

---

## 1. The two builds

```sh
./mk.sh           # tuning:   Debug 1, Const 0.  Every parameter is a live
                  #           mapping object with a "!MAP!" descriptor.
./mk.sh release   # shipping: Const 1.  Every parameter folds to a literal.
```

Both must code every image identically. That is the contract, and it is the
only check that catches a parameter which folds to something different from
what it evaluated to. It caught two real bugs during the port, both described
in §5.

`MOD/` is checked in, so the tree builds without perl; `mk.sh` regenerates it
when perl is present. A stale `MOD/` compiles fine and codes differently.

The checked-in form is the shipping one, so `./gc.sh` alone -- no perl, no
`mk.sh` -- gives the folded build. Which means **`./mk.sh` with no argument
leaves the tree dirty**: it rewrites `MOD/` into the tuning form, which is a
real change to a real build input, not a stray artefact. `./mk.sh release`
puts it back. Commit the tuning headers only if you mean the next person's
`./gc.sh` to produce a binary with 655 patchable mapping objects in it.

The shipping build times the same as the pre-port binary — 3.73 s encode,
0.83 s decode on `x_ci` against 3.77 and 0.81 — so the folding is real. The
tuning build pays about 6 % on decode for the indirection, which is what it is
for.

---

## 2. The modules

| module | prefix | what is in it |
|---|---|---|
| `bmf-P2.idx` | `P2` | the 28-tap NLMS cascade: seed coefficients, per-tap rates, blend weights, the covariance/variance EMAs and their floors, the per-bank context quantisers, the ternary cell's rescale schedule |
| `bmf-P1.idx` | `P1` | the MED-residual model: twelve counter bumps, the activity quantiser, the guess thresholds, the selector mixer's seed and ratio |
| `bmf-CN.idx` | `CN` | the counters both models share: `CounterNode`'s seeds, increment, rescale point and divisor; `BitCtr`'s step, seeding and rescale ladder |
| `bmf-CM.idx` | `CM` | `CtxModel`: the causal neighbourhood itself, table widths, neighbours per alphabet size, the match window, the anneal schedule, the whole neighbourhood probe |
| `bmf-AP.idx` | `AP` | the probability map's span, rate and blend; the mixer's stretch scale and learning rate |
| `bmf-PA.idx` | `PA` | the palette model's counter |
| `bmf-MB.idx` | `MB` | the slow model's context menu and shape: the fifteen seeded groups' flags, near/far rank bands, list-membership depths, the positions the context changes shape at |
| `bmf-GL.idx` | `GL` | limits the whole codec shares: the NLMS weight box, rescale points, occupancy limits, the degenerate-blend slack, the symbol lists' rescale and escape schedule |
| `bmf-CD.idx` | `CD` | decisions that pick a representation rather than code a pixel: the blend weight scale, the least-squares ridge, the DC window search |
| `bmf-IX.idx` | `IX` | every context index's bit positions, field widths and radix strides |
| `bmf-QZ.idx` | `QZ` | the five monotone quantiser ladders, as `Index` threshold mappings |

655 parameters, 6665 pattern bits. Both `IDX/opt.pl` and `IDX/sweep.py` print
those two numbers when they start, so they are checkable rather than
remembered.

---

## 3. Floats

The usual IDX convention stores a float as an integer over a round fixed-point
scale. That does not work here. `0.0024` over `1/32768` is `0.002411`, and
every NLMS rate and seed weight moving by a fraction of a per cent is a
different codec, not a port of this one — the tuning those numbers carry is
most of what `IMPROVEMENTS.md` records.

So each float goes in as **the exact mantissa of the float32 it replaces** and
comes back through `idx_f` with its own binary exponent:

```c
static const float bmf_p2_rate_reset = idx_f(P2_rate_reset, 31);
```

`5153961 * 2^-31` *is* `0.0024f` — not close to it, equal to it. Every float32
is `m * 2^-k` for some integer `m < 2^24`; `float(m)` is exact below `2^24`, the
divisor is a power of two so the division neither rounds nor loses a bit, and
what comes back is the float the literal named. `IDX/f2idx.py` computes the
pair.

The cost is that the patterns are wide — a 24-bit mantissa is 24 bits for the
optimizer to move — and that the exponents are irregular, since they come from
the values rather than from a house style. Freezing what has little leverage
(`!` at the start of a line) is the answer to the first; the second is the
price of the port being a port.

**One float would not go in this way.** The least-squares ridge is `0.1`, which
is not a dyadic rational, so there is no exact mantissa to carry. It goes in as
the denominator of a unit fraction instead: `1.0/10.0` in double is bit-for-bit
the double that the literal `0.1` denotes, which a fixed-point approximation
would not be.

---

## 4. What is *not* a parameter

Declared but frozen (`Const 1`), because they size arrays or drive template
arguments and must stay constant expressions: `kMaxNeighbours`, `kOrders`,
`kMaxAlpha`, `kMatchLens`, `kP1CounterCount`, `kFreqGridCount`, `no_symbol`,
`kMaxWidth`, and the row margins. They are in the `.idx` files so there is one
place to look, not because the optimizer can move them.

Not declared at all, because they cannot change behaviour or are not numbers in
the tuning sense: the range coder's carry and marker constants, histogram
centring offsets, the `0x8000` flag bit in `CounterNode::total`, the bit-packer
shifts. Two cases worth naming because they look tunable and are not:

* `lo = 256` in `CounterNode`'s rescale is the start of a minimum scan whose
  only use is a `lo<=1` test. Any value above 1 gives the same answer.
* `digit<Stride, Radix>`'s radix is unused by the method body — only the stride
  reaches the index. It is declared anyway, and the slow model's bucket index
  reads the radices to derive its strides, which is what makes that index safe
  to sweep (below).

Two couplings are expressed rather than assumed, so a sweep cannot break them:

* The blend weight scale appeared as `128`, as `<<6`, as `>>7`, as a `64:64`
  pair and as a `==128` test. It is one shift now, with the scale derived from
  it — otherwise a sweep could put a factor of two between the weights the
  encoder fits and the divisor that applies them.
* `CounterNode`'s seeded total is the sum of its seven seeds rather than its own
  number, so moving a seed cannot leave the total inconsistent.
* The slow model's bucket index is a mixed radix, and its strides are derived
  from its radices rather than declared beside them. A mixed-radix index whose
  strides do not match its radices is not a bijection; a sweep that moved one
  without the other collapsed two contexts onto one frequency record, and the
  model then divided by an empty total. Deriving them makes that
  unrepresentable, and removes two parameters that were never independent.

---

## 5. Five things that went wrong

Recorded because each of them compiled, and four of the five produced a working
codec that coded differently.

**A trailing space is one extra zero bit.** `idx2inc.pl` stripped comments but
not the whitespace they left behind, and every pattern-bearing regex captures to
end of line — so a single trailing space survived into the pattern and doubled
the parameter. Fixed in the tool.

**`import.pl` dropped every comment.** Its rewrite regexes also capture to end
of line, so folding the first optimizer results back would have stripped the
annotations out of the `.idx` sources. It holds a trailing comment aside and
puts it back now.

**In-class initializers must be constant expressions.** In the tuning build an
IDX value is a read from a patchable object, which is not one. Class constants
that come from IDX are declared in-class and defined after the class, where a
dynamic initializer is legal; a few that were never used qualified moved to
namespace scope instead.

**A static member of a class *template* is unordered dynamic initialization.**
`MixN<N>::kShift` could be initialized before the mapping objects it read,
leaving the mixer's learning rate at its clamp floor. The shipping build folded
and was correct; the tuning build silently learned at a different rate, and
fourteen of the 81 streams differed between them. Non-template statics defined
after the `MOD/` includes are ordered within the translation unit and do not
have this problem. **This is the failure the two-build contract exists to
catch, and nothing else would have caught it.**

**Round to nearest is not a ceiling.** The context model's probe tries three
shorter neighbourhoods at fractions of the longest, rounding to nearest.
Writing those as ceilings — `(n*5 + 5)/6` for `(n*5 + 3)/6` — changed which
neighbourhood length fourteen images chose. The rounding offsets are their own
parameters now.

---

## 6. Template arguments

The context indices' bit positions are template arguments, so an IDX value can
only reach them through the reference-typed non-type parameter of
`IDX-FORMAT.md` §8:

```c
template <IDXP(Pos)> INLINE CtxIdx &bit(bool b) { val += (uint32_t)b<<Pos; ... }
```

`const int32_t&` binds an object with static storage duration however that
object was initialized, so the shipping build binds literals and folds every
shift, the tuning build binds the objects `opt.pl` patches, and the call site is
spelled the same in both.

`CtxIdx`'s chaining methods lost their `constexpr` in the process: in the tuning
build their bodies read a runtime object, and a `constexpr` function that can
never be constant-evaluated is ill-formed. Nothing evaluated one at compile
time.

Why this matters rather than being pedantry: alt-P2 pulls a bank number out of
its context with a plain shift, so which bit a flag sits at decides whether that
flag is part of the bank or part of the row within it. The layout is a tuning
decision, and it was the last one that could only be made by editing a template
argument.

---

## 7. The ladders

Five monotone quantisers — activity level, activity group, predicted-value slot,
the alt-P2 delta context and its run length — were seven or fifteen integers
walked a step at a time to fill a lookup table. They are `Index` threshold
mappings now, one bit string apiece over the ladder's whole input range with a
`1` wherever an edge sits.

The value is unchanged, which is why the streams are: a threshold mapping
returns the number of edges below its argument, which is exactly what the
incremental walk computed.

What it buys is a different search — seven integers let the optimizer nudge
seven edges; 1661 pattern bits let it move an edge anywhere in the range, drop
one, or add one. What it costs is that those 1661 bits are most of the codec's
search space, so they are the first place to reach for a `!` when a pass is
taking too long.

This is the only place in bmf where an `Index` declaration fits. Elsewhere the
indices are bit-packed and their sub-fields are extracted downstream by shifts,
so IDX's mixed-radix builder would produce an equal-volume but differently
permuted index space — which is a redesign, not a port.

---

## 8. The loop

```sh
./mk.sh                                    # tuning build
perl IDX/opt.pl opt.lst                    # hill-climb; writes export.!!!
cd IDX && for f in bmf-*.idx; do           # fold the winners back
  perl import.pl $f ../export.!!! > t && mv t $f
done
cd .. && ./mk.sh                           # continue tuning
./mk.sh release                            # or ship
```

`opt.lst` is the corpus, one path per line. Optimizing on one image overfits it,
and the corpus has to cover every coding path the parameters reach — 1, 8, 24
and 32 bpp, palette and truecolour, packed and unpacked. A set that misses 8 bpp
leaves the P1 and context-model parameters drifting while nothing measures them.

`export.!!!` is written continuously, including mid-search states that have not
been validated yet. Folding it back after killing a run imports the search's
current position, not its best — which is legal but probably not what you meant.

---

## 9. The tests

```sh
./mk.sh && ./t.sh              # round trip, tuning build
./mk.sh release && ./t.sh      # round trip, shipping build
python3 IDX/sweep.py           # the clamp test
```

`sweep.py` is what `IDX-FORMAT.md` §5 asks for: a pattern is a search space and
the optimizer will visit the ends of it, so anything used as a size, a shift or
a divisor needs a range check at the point of use, and a parameter that can
crash the program when swept is a bug in the consumer. It flips pattern bits at
random across every parameter at once and round-trips images through the patched
binary, looking for a crash, a hang, or a round trip that stops matching.

Driven to each parameter's two extremes one at a time -- which is what it does
by default -- it found thirteen unguarded on the first pass and four more that
only showed up once those were fixed, in five kinds, all now closed:

* A probability ceiling swept to zero leaves the range coder a zero-width
  interval, and a floor above the ceiling inverts every clamp. The two are
  clamped and ordered against each other now.
* A counter step of zero leaves both counts at zero for ever, and the pair is
  then divided by its own total. So does a step of one, more subtly: the parent
  seed is `kStep/2`.
* The context model's neighbour counts index a table of 24 offsets, and the
  probe multiplies them up before anything else sees them.
* Several context indices address fixed-size tables. Those get `idx_bound` at
  the point of use — the one clamp here that cannot fold away, because the index
  is data — and every `CtxIdx` shift is bounded to 0..31, since a swept position
  is otherwise an out-of-range shift, which is undefined behaviour rather than a
  bad index.
* A record seeded from a coarser one divides by that record's total, and a
  collapsed context index leaves the coarse record empty. An empty source has
  nothing to contribute, so the guard is the answer rather than a paper-over —
  and the collapse now shows up where it happens rather than three layers away.

That last one took two rounds of guessing from signal numbers before the
obvious move worked: build with `-fsanitize=integer-divide-by-zero`, patch the
single parameter, and let it name the line.
