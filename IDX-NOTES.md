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
`./gc.sh` to produce a binary with 502 patchable mapping objects in it.

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
| `bmf-IX.idx` | `IX` | the surviving bit_of extraction bits: which bit of an expression carries a flag |
| `bmf-QZ.idx` | `QZ` | the five monotone quantiser ladders |

Every module except `QZ` and `IX` also carries `Index` declarations; §6 lists
what they size.

670 declarations across the eleven modules, of which 648 are live in the tuning
build -- the other 22 are the frozen structural ones, which have no mapping
object to patch -- for 6646 pattern bits. Both `IDX/opt.pl` and `IDX/sweep.py`
print the live count and the bit budget when they start, so those numbers are
checkable rather than remembered.

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
* `pz_signs` is indexed by `(ctx>>6)%243`, which extracts the mixer's digit
  back out of the cell context. A modulo is not a mask, so it is not a masking
  declaration; the shift and the modulo are both implied by `P2Ctx`'s factors.

Two couplings are expressed rather than assumed, so a sweep cannot break them:

* The blend weight scale appeared as `128`, as `<<6`, as `>>7`, as a `64:64`
  pair and as a `==128` test. It is one shift now, with the scale derived from
  it — otherwise a sweep could put a factor of two between the weights the
  encoder fits and the divisor that applies them.
* `CounterNode`'s seeded total is the sum of its seven seeds rather than its own
  number, so moving a seed cannot leave the total inconsistent.
* Every mixed radix derives its strides from its radices, because the generator
  does — see §7.

---

## 5. Seven things that went wrong

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

**`-` toggles Const, it does not set it.** `mk.sh` derives the shipping build by
rewriting the global directive, so a line frozen with a leading `-` under
`Const 0` goes *live* under `Const 1`. The mixer range did that, which made the
Volume sizing `counters[]` a runtime value, which is a variable-length array in
a struct, which does not compile. A line meant to be frozen belongs in its own
`Const 1` section.

**A folded comparison uses its operand's type.** A threshold mapping converts
its argument to `int`; the comparisons the shipping build folds it into use
whatever the parameter was declared as. One builder took its quantiser inputs
as `uint32_t`, so a negative run compared as a huge unsigned and landed in the
top bucket instead of the bottom one — in the shipping build only.

**Round to nearest is not a ceiling.** The context model's probe tries three
shorter neighbourhoods at fractions of the longest, rounding to nearest.
Writing those as ceilings — `(n*5 + 5)/6` for `(n*5 + 3)/6` — changed which
neighbourhood length fourteen images chose. The rounding offsets are their own
parameters now.

---

## 6. Context indices

Every context in the codec is an `Index` declaration. There is no bit-packing
left in the sources: the model hands a builder its factors and the builder
returns the index.

**Why the port is bit-exact.** bmf packed a context into bit fields and pulled
sub-contexts back out with shifts. That is a mixed radix whose radices happen to
be powers of two, and `MakeIndex` accumulates most-significant factor first,
which is the order the chains were written in. Same integer, not a permutation
of it.

**What it buys.** A bit position, made into a parameter, lets the optimizer move
a field. A declaration lets it change the structure — and the generated `_Volume`
is the number of distinct contexts, so the table each one addresses is sized by
the declaration that indexes it:

| index | Volume | table |
|---|---|---|
| `P1_P1Result` | 3^9 · 2·2·8 = 629856 | `counters[]` |
| `P2_P2Ctx` | 3^5 · 2·2·16 = 15552 | `freq[]`, `pz_fast[]` |
| `P2_BankSlot` | 5 · 32768 = 163840 | `p2_ctr[]` |
| `P2_Ctx1a`… | 2^11 · 4·4 = 32768 | the bank radix above |
| `P2_NbSlot` | 6·5·4·4·4 = 1920 | `nb_id[]` |
| `P2_CtxPair` | 64 · 16 = 1024 | the model arena's strips |
| `CN_Strip` | 4·2·2·64 = 1024 | the same |
| `MB_Sig1` | 188 · 2^6 · 16 = 192512 | `ctx_id1[]` |
| `MB_Sig2` | 13600 · 8 = 108800 | `ctx_id2[]` |
| `MB_Sig3` | 44500 · 16 = 712000 | `ctx_id3[]` |
| `MB_SymCache` | 65536 · 8 = 524288 | `sym_ctr[]` |
| `MB_CandNode` | 32 · 128 = 4096 | `bit_node[]` |
| `MB_CandFallback` | 2 · 8 = 16 | `bit_root[]` |
| `MB_MatchCtx` | 2^6 = 64 | `ctx_state[]` |
| `MB_Bucket` | 5·5·15 = 375 | `ctx_bucket[]` |
| `MB_EscCtx` | 32·2·2·2 = 256 | `esc_ctr[]`, plus its parent |
| `MB_RunCtr` | 3 · 16 = 48 | `run_ctr[]` |

The three forms the format provides all earn their place:

* **`ADD` factors** for the radix digits and the flag runs.
* **Threshold mappings** for the comparison sequences — a bit per input value
  with a 1 at each edge. Three edges as three integers can only be nudged;
  three edges as a bitmap can move anywhere in the range, be dropped, or be
  joined by a fourth, and the factor's radix is the mapping's own `Size`, so the
  Volume follows.
* **Maskings** for the coarse views: which bits of the cell context feed the
  seat counter and the probability map.

Two selectors count edges *above* their argument, which a mapping does not do.
They are the same mapping over the negated variable with a negative base —
`-run4` against edges at −1840 and −272 counts what `(run4<1840)+(run4<272)`
counted, and the folded build spells those two comparisons back out.

**What is still a `Number`.** A threshold list is only worth spelling as a
bitmap when its span is sane. alt-P2's gradient ladders run to 145408, and its
band edges and two of its four gradient quantisers scale their thresholds by
another variable, which no static pattern can express. Those keep integer edges.
`tri_sign`'s dead zone is over an unbounded difference, so it does too.

**What it costs.** 6646 pattern bits became 26165, and 20211 of those are the
eight threshold bitmaps. They are the first place to put a `!` when a pass is
taking too long, and both tools print the budget on startup so the cost is
visible before the pass rather than after.

---

## 7. Two invariants that only a declaration can state

Some couplings cannot be expressed as one parameter and are checked instead.

**A mixer's granularity.** Two selector mixers hand their sum to an index as its
most significant factor. That works only because their weights are multiples of
`1<<ctxw_shift` and the factors packed under the sum multiply to exactly that.
In the shipping build both sides are literals and the check folds away; in the
tuning build the shift is a load-time read, so `main` checks it once and refuses
rather than aliasing contexts silently.

**A mixed radix's strides.** They used to sit beside the radices as parameters
of their own, and a sweep that moved one without the other stopped the index
being a bijection — two contexts landed on one frequency record and the model
divided by an empty total. The generator derives strides from radices, so the
sources can no longer express the mismatch.

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
