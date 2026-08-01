# Submodels as coroutines — analysis

**Proposal.** Make each submodel a coroutine: it mixes a prediction, yields it,
receives the coded bit, then proceeds with its update. In theory this makes
submodels completely independent and lets one work with a single submodel in
isolation.

Everything below is measured against this tree (`paq8hp.hpp`, level 7, book1,
the Lib3 coroutine layer the I/O already runs on), not argued from principle.

---

## 1. The observation is correct, and sharper than stated

`Predictor::p()` is, in full:

```c
int p() const {
  return pr;
}
```

It is a getter. The prediction it returns was computed by the *previous*
`Perceive()`, which does:

```c
void Perceive(int bit) {
  y = bit;  ema_tgt_y = ...;  ema_bias_y = ...;
  update();               // ... state update ...
                          // ... then: pr = contextModel.p(); pr = APM chain(pr)
}
```

So the call named "predict" does no prediction and the call named "update" ends
by predicting. A reader looking for where the probability comes from will not
find it in `p()`.

This is not an accident of style — it is what happens when a two-phase object is
given a one-phase interface. The arithmetic coder genuinely needs `p` before it
can code the bit, and the bit is genuinely unknown until after. Something has to
carry state across that boundary. Today it is the member `pr` plus an implicit
convention about who runs when; the proposal is to make it the coroutine's
suspended stack instead.

**That part of the diagnosis is right.** The question is whether coroutines are
the right cure, and whether "completely independent" follows.

---

## 2. What the coroutine version costs

The I/O layer already uses Lib3's `Coroutine`, so this is measurable rather than
speculative. Lib3's is a **stack-copying** coroutine: `yield` does
`memcpy(stk, stkptrL, stkptrH-stkptrL)` and the resume copies it back, so a
round trip is two memcpys of the live frame span.

Measured on this machine (`clang -O2`, 1M round trips per row):

| live stack at the yield point | per round trip |
|---|---|
| 57 B | 34.7 ns |
| 185 B | 37.3 ns |
| 569 B | 48.6 ns |
| 2 105 B | 89.3 ns |
| 8 249 B | 170.3 ns |

The budget it comes out of: book1 at level 7 is 6 150 168 bits in ~19 s, i.e.
**3 089 ns/bit**. Four submodels each yielding once per bit is four round trips:

| live stack | added per bit | share of the bit |
|---|---|---|
| ~57 B | 140 ns | 4.5% |
| ~570 B | 196 ns | 6.3% |
| ~2.1 KB | 356 ns | 11.5% |
| ~8.2 KB | 680 ns | 22.0% |

The realistic figure is the middle rows. A submodel yielding from inside its
own `mix()`, called from `ContextModel::p()`, called from `Predictor`, has the
whole chain live — several hundred bytes to a couple of KB. **Call it 6–12%.**

That is a real cost but not a disqualifying one, and it is worth saying so
plainly: the performance objection to this idea is weaker than it first looks.
Lib3's coroutine is cheap because it copies only the live span, and the live
span here is small.

Two costs that are easier to miss than the time:

* **Memory.** Every `Coroutine` carries `stk[STKPAD]` = 64 KB + 24, and its
  first call goes through `call_do_process0`, which allocates `STKPAD0` = 256 KB
  of stack padding so the coroutine's frames cannot collide with the frontend's.
  Four submodel coroutines is 256 KB of saved-stack buffers and 1 MB of stack
  padding. Irrelevant next to a 313 MB model, but it is not free and it is not
  where anyone would look for it.
* **Tooling.** `setjmp`/`longjmp` plus a relocated stack defeats the profiler's
  call-graph unwinding and most debuggers' backtraces. The codec's hot loop is
  currently the thing most worth profiling.

---

## 3. Why "completely independent" does not follow

This is the substantive objection, and it is not about coroutines at all.

### 3.1 The submodels are not predictors

They do not produce predictions. They produce **mixer inputs** — 466 of them per
bit, into one shared `MainMixer`:

| source | inputs |
|---|---|
| bias | 1 |
| 3 RunContextMaps | 3 |
| core ContextMap (9 contexts × 6) | 54 |
| WordModel (46 contexts) | 276 |
| SparseModel | 40 |
| RecordModel | 92 |
| **total** | **466** |

A submodel cannot "mix a prediction and yield it" because it has no mixer. To
give it one is not a refactor; it is a change to a **hierarchical mixing**
architecture — per-submodel mixers plus a top-level mixer over their outputs.
That is a legitimate design (paq8px and cmix both do it) but it is a modelling
change with its own gains and losses, and it must be measured, not assumed. See
§5.

### 3.2 The shared feature bank has write ordering

The submodels read a bank of ~20 file-scope variables, and several of them
**write** it:

| writer | writes | read by |
|---|---|---|
| `WordModel::mix` | `col`, `frstchar`, `spafdo`, `words`, `spaces`, `wordcount`, `spacecount`, `nl`, `nl1` | SparseModel, mixer selectors 2/4/6 |
| `ContextModel::p` | `x4` (phantom punctuation injection), `order` | WordModel, SparseModel, mixer selectors 1/2/5 |
| `RecordModel::mix` | `cxtfl` (toggled around three of its own maps) | `ContextMap::mix1/mix2` |

`order` is the sharpest case: it is produced by `cm.mix()` **inside the same
bit** and consumed by three of the six mixer selectors a few lines later. There
is a strict happens-before between the core ContextMap and the mixer's own
context selection.

Suspending a submodel between its prediction and its update does not remove any
of these edges. It makes them *worse*: right now the ordering is at least
visible as straight-line call order in `ContextModel::p()`. With four suspended
stacks interleaved by a scheduler, "who wrote `col` before whom" becomes a
property of the resume order — the same coupling, now invisible.

**Coroutines would hide the coupling, not remove it.** The coupling is the thing
standing between here and independent submodels.

### 3.3 Layout coupling on the mixer input vector

`ContextModel::p` rewinds `m.nx` after `cm.mix()` and rescales four contexts'
inputs in place (the `mul` block). It addresses them **by position**. §10.5 of
the analysis flags this; this tree already had to fix the divisor when the core
map went from 7 to 9 contexts. Any scheme where submodels contribute inputs in a
scheduler-determined order breaks it outright.

---

## 4. The proposal contains two separable ideas

They are tangled together, and they have very different cost/benefit:

**(a) Make the two phases explicit.** Right now the phase boundary is a
convention. It could be an interface:

```c
struct Submodel {
  void predict();      // add inputs to the mixer
  void update(int y);  // consume the bit
};
```

This is a **pure refactor** — no bitstream change, no cost, no coroutine. It
gets the readability win that motivated the proposal. The reason it is not
already this shape is that `predict` and `update` for these submodels are
*interleaved* inside `mix()`: `ContextMap::mix1` updates the previous bit's
state and selects the next slot in one pass over the same cache line, on
purpose. Splitting it into two passes would double the line traffic on the
structure §4 identifies as the memory bottleneck (225 random 64 B lines per
byte). So the honest version of (a) is: **name the phases, do not separate the
passes** — which is a documentation and interface change, not a control-flow one.

**(b) Give each submodel its own mixer and combine hierarchically.** This is the
part that would actually make submodels independent, and it has nothing to do
with coroutines. It is worth evaluating on its own merits (§5).

Coroutines are the mechanism the proposal reaches for, but neither (a) nor (b)
needs them.

---

## 5. Hierarchical mixing, on its own merits

If the goal is "a submodel is a thing that produces a probability", the change is:

* each submodel gets a `Mixer<n_i, rows_i, 1, w0>` over its own inputs;
* a top mixer combines the 4–5 submodel outputs (plus bias) into `pr`;
* the APM chain stays where it is.

Arguments for: each submodel's weights specialize to its own feature scale
instead of competing in a 466-wide vector; the top mixer learns *which submodel
to trust* per context, which is exactly what selector 1's `order` term is
groping toward today; and a submodel can genuinely be disabled or tested alone.

Arguments against, specific to this codec:

* **The flat mixer is already context-selected six ways.** Six 466-wide rows
  selected per bit is not a plain linear model — it is a mixture of six experts
  over the full feature set. Some of what hierarchy would buy is already bought.
* **Capacity moves.** 466 × 12 544 rows ≈ 5.8 M weights today. Split into
  per-submodel mixers the total falls unless each gets its own selector set, and
  then the row budget has to be re-divided. Every one of the six selector ranges
  is currently welded to `MIXER_ROWS` (§10.5 and this tree's `static_assert`).
* **The one-bit training lag.** `m.update()` trains on the *previous* bit's
  input vector and `sub.update2()` runs at the top of `p()`. A two-level scheme
  has to decide the lag structure at each level; get it wrong and the change
  looks like a modelling loss when it is a scheduling bug.
* **It is bitstream-changing and unranked.** The analysis document's §11 does
  not list it, which is not an argument against it, but §11.1 (match model) is
  ranked first and is a smaller change with a larger expected gain.

**Recommendation: this is a real candidate, but it should be proposed and
measured as "hierarchical mixing", not as "coroutines".** Framing it as a
control-flow change hides that it is a model change requiring a full A/B.

---

## 6. Getting the stated benefits more cheaply

The proposal's motivations, and what each actually needs:

| motivation | what it needs |
|---|---|
| "prediction happens in `update`, which is confusing" | rename and document the two phases; §4(a) |
| "submodels should be independent" | break the shared-feature-bank writes and the shared mixer; §3.2, §5 — coroutines do neither |
| "work with just one submodel" | a submodel gate + a dynamic input count |

The third is worth spelling out, because it is the concrete goal and it is close
at hand. What blocks single-submodel operation today is not control flow, it is
that the mixer's input count `N` is a compile-time constant with **zero slack**:
466 inputs into `Mixer<466,…>`, N rounded to 480, and every submodel's inputs
identified only by their position in the vector. §10.5 puts it exactly: "any
feature-count change in `mix2` silently pushes `add()` past `tx[]` with no check
anywhere."

So the cheap version is:

1. Have each submodel's `mix()` return how many inputs it added (it knows).
2. Sum them and `assert(m.nx <= N)` — the check §11.11 already asks for.
3. Gate submodels behind a mask; size `N` from the mask at compile time.
4. Replace the `mul` block's positional addressing with a base captured before
   the submodel runs — this tree already did that for the core map, using
   `decltype(cm)::C` and a saved `nxend`.

That is bitstream-neutral when the mask is "all on", it makes
"just WordModel" a one-line experiment, and it removes the fragility §10.5
describes as a side effect. It is strictly less work than the coroutine version
and delivers the actual goal.

The remaining obstacle after that is §3.2 — with WordModel off, `col`,
`frstchar` and `spafdo` stop being maintained, and SparseModel and two mixer
selectors read them. That is a genuine dependency to untangle (move the feature
bank's maintenance out of WordModel and into the byte-boundary update where the
rest of it already lives), and it is untouched by any control-flow scheme.

---

## 7. Where coroutines *are* the right tool here

Worth stating, since the mechanism is already in the tree and this is not an
argument that coroutines are bad:

* **The I/O layer** — where they now are. Yields happen at 64 KB window
  boundaries, so the ~40 ns is amortized over ~65 000 bytes and the alternative
  (threading a buffer state machine through `rc_Process`) is genuinely worse.
  This is the shape the mechanism is designed for: **rare** yields across a
  **deep** call stack.
* **A match model (§11.1)**, if one is added. Walking a match is naturally a
  generator, and it yields per *match*, not per bit.
* **A multi-pass or block-restructuring front end**, where a stage wants to be
  written as a loop but driven as a filter — which is exactly what `ddsdet`'s
  4-pin carver uses them for.

The distinguishing property is the yield *rate* relative to the work between
yields. The I/O layer yields once per 65 536 bytes. The proposal yields four
times per **bit** — 2 million times more often, on the codec's hottest path.

---

## 9. Three objections, taken seriously

The three points below were raised against the first draft. Two of them are
right and change the analysis; the third rests on a premise the source does not
support. Working through them is more useful than restating the verdict.

### 9.1 "Update functions are functions, so they load/save their state each call"

True, and I underweighted it. But for *this* proposal on *this* coroutine
implementation it points the other way, and the reason is worth being precise
about.

**Lib3's Coroutine copies the stack.** `yield` does
`memcpy(stk, stkptrL, stkptrH-stkptrL)` and the resume copies it back. So state
kept alive across a yield is not "kept in registers" — it is memcpy'd out and
memcpy'd back, twice per round trip. A member of the submodel object is copied
**zero** times; it is loaded when read and stored when written, and it stays
where it is.

Order of magnitude, using the measured live-frame sizes from §2:

```
4 submodels x 2 memcpys x ~600 B live   = ~4 800 B/bit moved by the coroutine
~7 live scalars per submodel x 4 x 4 B  =   ~112 B/bit of reloads it avoids
                                          ~43x more traffic than it saves
```

That is the whole efficiency argument inverted: with a stack-copying coroutine,
**the more state you keep on the coroutine's stack — which is the point of the
idea — the more it copies per bit.**

Two qualifications, because this is implementation-specific rather than
fundamental:

* **A different coroutine mechanism changes the answer.** A stack-*switching*
  coroutine (separate stacks, swap `rsp` and the callee-saved registers; ucontext,
  Boost.Context, or a hand-rolled switch) does not copy anything — the frame
  stays put and only registers move. Under that mechanism the efficiency
  intuition holds. Lib3 does not have one, and adding one is a different project
  from this one.
* **The state that would benefit is small anyway.** WordModel's live per-bit
  scalars are `word0..word4`, `nl`, `nl1` — seven words. Its *real* per-bit
  state is `ContextMap`'s `cp[46]`, `cp0[46]`, `runp[46]`, `cxt[46]`: 184
  pointers and hashes walked every bit. Those are arrays under any scheme; no
  coroutine puts them in registers. The register-residency win is bounded by
  those seven scalars, not by the submodel's working set.

So: the observation is correct in general, the arithmetic does not work out for
Lib3's coroutine, and the ceiling on the win is low even for the right
coroutine.

### 9.2 "Their logic is somewhat inside-out"

**This is the strongest argument for the proposal and the first draft did not
give it enough weight.** It is also where the tension is sharpest.

The inside-out shape is concentrated in `ContextMap::mix1t`, which selects a
slot in a depth-3 bit tree by asking which bit position it is at:

```c
if( PAQ_BP>1 && runp[i][0]==0 )        cpi = 0;
else if( PAQ_BP==1||PAQ_BP==3||PAQ_BP==6 ) cpi = cp0[i]+1+(cc&1);
else if( PAQ_BP==4||PAQ_BP==7 )            cpi = cp0[i]+3+(cc&3);
else { /* bpos 0, 2, 5: fetch a fresh line */ }
```

That is a hand-unrolled state machine. Written as a coroutine it is a walk:
fetch a line, predict/yield/update, descend to `[1+b]`, predict/yield/update,
descend to `[3+(bb)]`, … — the shape the data structure actually has. §4 of the
analysis document has to draw an eight-row table to explain what the code does;
the coroutine form would be the table.

**But that shape is load-bearing for speed, in two documented ways.**

`mix1t<CF,BP>` is *templated on the bit position*. `PAQ_BPOS_T` compiles eight
instantiations so `>>(7-bpos)` becomes a constant shift, the `bpos==7` reset
folds away, and the prefetch guards resolve at compile time — at the cost of
eight copies of the hot loop. A coroutine with `for(i=0;i<8;i++)` gets none of
that unless the loop is manually unrolled eight ways, at which point the
"readable straight-line walk" is eight straight-line walks and the readability
win is spent.

Worse, the comment at `paq8hp.hpp:85` documents that the loop-with-a-bit-counter
form is *actively hazardous* here:

> `!! Do NOT hoist PAQ_BP into a local variable. !!` … Caching it in a
> `const int` makes it provably loop-invariant, which enables gcc's loop
> unswitching … and gcc 13.3 -O3 generates WRONG CODE for the unswitched loop:
> book1[0:65536] at L0 goes 23173 → 32656 bytes.

A coroutine's inner loop counter is exactly the provably-loop-invariant form
that triggered that. Not a reason never to do it — clang is fine and the bug is
gcc's — but it is a live landmine in the direction the proposal walks.

So the honest statement is: **the readability win is real and is concentrated
precisely where the current shape earns its speed.** That is a genuine trade,
not a free improvement, and it should be made with a measurement rather than on
principle.

### 9.3 "Let submodels use their own hashtables instead of shared ones"

**They already do.** Every table in the model is owned by exactly one submodel:

| owner | tables |
|---|---|
| `ContextModel` | `cm` (`MEM*16`, 9 contexts), `rcm7`, `rcm9`, `rcm10` |
| `WordModel` | `cm` (`MEM*16`, 46 contexts), `t1[256]`, `t2[65536]` |
| `SparseModel` | `cn` (`MEM*2`, 5 contexts), ten SSCMs |
| `RecordModel` | `cm`, `cn`, `co`, `cp`, `cq` (32 KiB…128 KiB), `cpos1`, `wpos1` |

There is no cross-submodel table anywhere. So there are no cross-submodel
collisions to remove — the premise does not hold.

The collision pressure is real, but it is **inside** one submodel: WordModel's
46 contexts share a map the same size as the core's 9. Per-context line pressure
is ~5× higher there (the analysis document says 6.6× against the original 7),
and §11.10 already identifies it as the thing to fix — by **resizing**, which is
one template argument, not by restructuring anything.

The instinct is right and lands one level down from where it was aimed:
*within* WordModel, 46 contexts is a lot to put through one 16-bit checksum and
one eviction policy, and §11.13 (indirect contexts) is the analysis document's
answer to that specific pressure.

**The mixer half of the sentence is a different matter and does hold** — that
one *is* shared, and it is §5's hierarchical-mixing candidate. Nothing in §9
changes that conclusion: giving each submodel its own mixer is worth measuring,
and it does not need coroutines.

---

## 10. Revised verdict

| | |
|---|---|
| Is `p()`/`update()` confusing? | **Yes**, and `Predictor::p()` being `{ return pr; }` is the proof. |
| Is the logic inside-out? | **Yes**, in `ContextMap::mix1t` especially — and that is where a coroutine would read best. |
| Would coroutines be faster? | **No, with Lib3's.** Its stack-copying yield moves ~43x more bytes than the member reloads it would save. With a stack-*switching* coroutine the argument holds, but Lib3 has none and the state that benefits is ~7 scalars per submodel. |
| Would they cost much? | ~6–12% of the per-bit budget, plus losing the `PAQ_BPOS_T` specialization unless the bit loop is unrolled eight ways anyway. |
| Own hashtables? | **Already the case** — every table has exactly one owner. The pressure is intra-WordModel and is a sizing question (§11.10/§11.13). |
| Own mixers? | **The one shared resource, and a real candidate** — as hierarchical mixing, measured against §11.1, not as a control-flow change. |

What survives from the proposal, in order of confidence:

1. **Name the phases.** Free, bitstream-neutral, fixes the thing that prompted
   all of this.
2. **Account for mixer inputs** (each `mix()` returns its count, `assert`,
   gate mask) — this is what actually delivers "work with just one submodel".
3. **Resize WordModel's map** against the core's — the collision pressure the
   third objection was reaching for.
4. **Per-submodel mixers**, as an explicitly bitstream-changing experiment.
5. **Coroutines**, only if 4 is adopted *and* someone adds a stack-switching
   implementation — at which point 9.1's arithmetic reverses and the readability
   win in `mix1t` can be bought without the memcpy.

---

## 8. Verdict (first draft — superseded by §10)

| | |
|---|---|
| Is the diagnosis right? | **Yes.** `p()` returning a value computed by the previous `Perceive()` is genuinely confusing, and the phase split deserves to be explicit. |
| Would coroutines make submodels independent? | **No.** Independence is blocked by one shared mixer, a shared feature bank that three submodels write, and positional coupling in the mixer input vector. None of those is a control-flow problem. |
| Would coroutines make it *look* independent? | Yes — which is the risk. The ordering constraints would move from visible call order into invisible resume order. |
| What does it cost? | ~6–12% of the per-bit budget at realistic stack depths, plus 1.25 MB of stack/padding and loss of profiler backtraces. Real, but not the main objection. |
| What should happen instead? | Split the proposal in two. Do the interface/naming half now (free, bitstream-neutral) together with the input-count accounting from §6 — that is what buys "work with just one submodel". Evaluate the other half as **hierarchical mixing**, on measurement, ranked against §11.1. |

The single most useful thing to take from the proposal is not the coroutine: it
is the observation that a submodel ought to be a thing you can name, count the
inputs of, and switch off. That is reachable now, and it does not require the
control flow to change at all.
---

## 11. Three follow-ups: static layout, per-submodel mixers, threads

### 11.1 The mixer layout is static, and building it dynamically is the thing to fix

**Confirmed against the source, with one correction that only showed up on
implementing it.** `m.add()` walks `tx[nx++]`, but the number and order of adds
is not data-dependent. `mix2` emits five inputs or four, and which it is depends
on `cxtfl` — a flag RecordModel sets around three of its own maps, i.e. a
property of *which map is mixing*, not of the input. Every other add is
unconditional. So the 466 slots are the same 466 slots, in the same order.

**But not from the first bit.** A `ContextMap`'s `cn` counts the contexts
`set()` has been handed; it is filled at bpos 0 and cleared at bpos 7. For the
seven model invocations inside the *first byte*, before any bpos-0 call has
happened, every map has `cn == 0` and mixes nothing — the vector is 4 slots
long, not 466. The layout is static **from the first byte boundary onward**.
This was found by the layout check catching it, not by reading, which is a fair
advertisement for adding the check.

The ramp-up is stream-neutral, and that is what makes a fixed layout viable at
all: `Mixer::tx` is zeroed at construction, nothing writes those slots until the
first `set()`, a zero input contributes zero to every dot product, and `train()`
moves a zero-input weight by zero. So a short vector and a fully-laid-out one
holding zeros code identically — which is the same property gating relies on.

That is a compile-time-known layout being reconstructed at runtime, and the
instinct that it is not worth it is right. What it costs:

* **A serial dependency.** Every `add()` is `tx[nx++] = x`. The increment chains
  through all 466, and the `mul` rescale block then *rewinds* `nx` and does a
  read-modify-write pass over 24 of them.
* **§10.5, in full.** Positional addressing is the reason the `mul` block had to
  be repaired when the core map went 7 → 9 contexts, the reason nothing can
  assert the input budget, and the reason a `mix2` feature-count change walks off
  `tx[]` silently.

With fixed slices — each submodel owning `tx[BASE_i .. BASE_i+N_i)` as named
constants — three things fall out, and they are the reason this matters more
than the cycles:

1. **Submodel gating becomes free and exact.** A disabled submodel leaves its
   slice zero. A zero input contributes zero to every dot product, and `train`
   computes `w += (0*err*2>>16 + 1)>>1` = 0, so its weights never move either.
   The slice is *inert*, not merely ignored. That is exactly equivalent to the
   submodel not existing, with no `N` resize, no assert, and no renumbering —
   and it is the thing §6 wanted. Note this only works with a fixed layout:
   under `nx++`, skipping a submodel shifts every later input into a different
   weight column, which is a different model, not a smaller one.
2. **The `mul` rescale folds into the store.** `tx[BASE+i] = st_scaled(x, MUL)`
   at the point of production, instead of a rewind and a second pass.
3. **Disjoint slices are what threads need** — see §11.3.

This is bitstream-neutral if the slice assignment reproduces the current order,
which it can, because the current order is static. It is the highest
confidence-to-effort item in this whole discussion.

### 11.2 Per-submodel mixers with their own rates and contexts

This is §5's hierarchical mixing, and the added detail — *independent update
parameters, possibly an independent mixer context* — is what makes it more
interesting than "the same thing with more levels".

Today there is one learning rate for all 466 weights (`MIX_LR_MUL`, seeded 7)
and six selectors shared by every feature. But the feature families have very
different statistics: WordModel's 276 inputs are sparse, slow-moving word
statistics; RecordModel's 92 are fast recency signals over 32–128 KiB maps; the
core's 54 are dense high-order bit histories. One rate for all three is a
compromise nobody chose — it is what a flat mixer forces.

Per-submodel mixers would let each carry its own rate *and its own selector*:
WordModel's natural context is word-shaped (`spafdo`, word length, `frstchar`),
RecordModel's is positional (`pos&3`, column). Right now every selector is
global and every submodel sees the same six.

The honest counter is **cross-family interaction**. A flat 466-wide mixer can
learn "WordModel's order-2 word context is confident *and* RecordModel's column
context agrees"; a two-level mixer collapses each family to one scalar first and
can only recover that through the top mixer's context. Whether that matters here
is unmeasured. Two things temper it: the flat mixer is already a
six-way-context-selected mixture rather than one linear model, so some of the
interaction is already being learned per selector rather than per feature pair;
and paq8px/cmix run hierarchical mixing successfully at much larger feature
counts.

Capacity is the other thing to watch: 466 × 12 544 ≈ 5.8 M weights today. Split
per submodel, the total falls unless each gets its own row budget, and every one
of the six ranges is currently welded to `MIXER_ROWS` by this tree's
`static_assert`. That is a re-division to design, not a detail.

**Still the right way to run this: as a measured A/B, ranked against §11.1's
match model.** But §11.1 above (fixed layout) is a prerequisite for doing it
cleanly, which is a good reason to do that first regardless.

### 11.3 Threads, and why the encoder really can run ahead

**This is the strongest idea in the thread, and the source supports it.**

The claim to check is that during encoding a submodel does not need the final
prediction. Verified:

* `order` is written once (`order = cm.mix(m)-1`) and read at exactly three
  places — mixer selectors 1, 2 and 5. **No submodel reads it.**
* `fails`/`failz`/`failcount` depend on `pr`, but they are read only by the APM
  chain and the final blend — **post-mixer**.
* Every submodel's state update is `nex(*cpi, y)` / `ema(..., y, ...)` / a byte
  register shift. All functions of the coded bit and the byte history.

So **each submodel is a pure function of the byte stream.** When encoding, the
byte stream is known up front, so every submodel's entire trajectory —
contexts, states, and its slice of the 466 mixer inputs, for every bit of the
file — is computable without ever seeing a prediction. Only mixing, the APM
chain and the coder are inherently sequential.

The pipeline that follows: N submodel threads each fill a ring buffer with their
own slice for bits *i…i+K*; the mixer thread consumes slices in order, mixes,
runs the APM chain, codes. Synchronisation is a ring-buffer index, amortised
over K bits, not a per-bit join — which is precisely the point about not needing
to sync often. K = 1024 costs about 1 MB of buffers.

Two properties make this safe rather than hopeful:

* **The tables are already per-submodel** (§9.3). Threads partitioned on submodel
  boundaries touch disjoint memory, so there is no race and no locking. This is
  the payoff from that section — the partition that is safe is exactly the one
  the ownership already draws.
* **Determinism is preserved.** The mixer consumes slices in bit order, so the
  computation is identical; only its schedule changes. The bitstream does not
  move.

Now the honest ceiling. Serial work is the six 480-wide dot products plus
`train`, the APM chain and the coder: ~5 760 int16 madds/bit ≈ 60 ns of SIMD
plus overheads, call it ~200 ns against the measured 3 089 ns/bit. That is a 94%
parallel fraction, and Amdahl would say 3.35× on four threads.

**It will not get that, because the submodels are wildly unequal:**

| submodel | contexts | lines/byte | share |
|---|---|---|---|
| WordModel | 46 | 138 | **60%** |
| RecordModel | 17 | 51 | 22% |
| core cm | 9 | 27 | 12% |
| SparseModel | 5 | 15 | 6% |

The critical path is WordModel alone, so a per-submodel split ceilings at
**1/0.60 ≈ 1.67×**, not 3.35×. To go past that WordModel's own 46 contexts have
to split across threads — and that is *not* safe as-is, because they share one
`ContextMap`: two contexts can hash to the same 64-byte line and both mutate it,
which is a data race and therefore nondeterminism, which the decoder cannot
reproduce. Splitting it safely means giving the halves separate maps, which is a
bitstream change — but it is the same change §11.10 of the analysis document
already recommends for a different reason (per-context line pressure). Those two
arguments point the same way, which is worth noticing.

Three caveats to record:

* **Encode-only.** The decoder cannot run ahead; it has no bits until it decodes
  them, and decoding needs the mixed prediction. Encoding gets ~1.7×, decoding
  stays where it is. For a symmetric-by-tradition PAQ that is a design choice to
  make deliberately, not a free win.
* **The feature bank could be precomputed for the whole file when encoding** —
  `words`, `col`, `frstchar`, `x4`, `w4`, `f4`, `tt` are all pure functions of
  the bytes — which would dissolve §3.2's write-ordering problem entirely on the
  encode side. But then the encoder and decoder maintain that bank by two
  different code paths, and they must agree exactly. One implementation with two
  drivers, or it is a desync waiting to happen.
* **Coroutines are not needed for any of this.** Threads plus ring buffers is
  the mechanism; a coroutine is a *scheduling* construct for a single thread and
  would, if anything, get in the way of a real one. The two ideas have been
  travelling together in this discussion but they are independent, and this one
  is the one with a number attached.

### 11.4 How the three chain

They are not three options; they are a sequence, and each enables the next:

```
fixed mixer layout (11.1, bitstream-neutral)
   ├─> free, exact submodel gating          -> "work with just one submodel"
   ├─> mul folded into the store            -> removes the §10.5 coupling
   └─> disjoint slices per submodel
          ├─> per-submodel mixers (11.2, bitstream-changing, A/B)
          └─> threaded encode (11.3, bitstream-neutral, ~1.7x)
                 └─> split WordModel's map  -> past 1.7x, and §11.10 wants it anyway
```

The first step is free and unlocks the other two. That is the order to do them
in, and none of the three requires the control flow to become coroutines.


---

## 12. Implemented: `mix_layout.inc`

§11.1 is done and verified byte-identical (book1 191980, `442b2735`, both
builds, empty/1-byte/200 KB round trips, both charsets).

`mix_layout.inc` holds one enum per submodel, each starting at 0 and naming the
*start* of every block it contributes. A one-slot block needs no initialiser —
auto-increment is exactly right for it, which is why SparseModel's ten
`SmallStationaryContextMap`s are a bare list and the trailing `S_N` falls out of
the same mechanism. A wider block spells `= prev + width`.

```c
enum {                                      // SparseModel
  S_cn,                                     // auto: 0
  S_scm1 = S_cn + S_CN_CTX*MIXIN_CM,        // 30
  S_scm2, S_scm3, S_scm4, S_scm5,           // auto: 31..34
  S_scm6, S_scm7, S_scm8, S_scm9, S_scma,   // auto: 35..39
  S_N                                       // auto: 40
};

enum {                                      // the submodels, end to end
  MIXB_X,                                   // auto: 0
  MIXB_W = MIXB_X + X_N,                    // 58
  MIXB_S = MIXB_W + W_N,                    // 334
  MIXB_R = MIXB_S + S_N,                    // 374
  MIX_INPUTS = MIXB_R + R_N                 // 466
};
```

Three things follow from having it:

* **The context counts live there and the maps are declared from them.**
  `ContextMap<((U32)MEM(L)*16), W_CM_CTX> cm;`. One source of truth: change a
  count and the map, the layout, the mixer width and the `mul` block's stride
  all move together. Getting that backwards is what made the 7→9 change a
  two-place edit with only one place checked.
* **`MainMixer` is `Mixer<MIX_INPUTS, …>`** instead of a hand-maintained 466.
* **The `mul` block is addressed, not measured.** It was dividing by a literal
  context count and relying on `3+2+1+1` summing to the same number to land the
  cursor back where `cm.mix` left it. Now the start, the stride and the end are
  `MIXB_X + X_cm + 3*MIXIN_CM`, `MIXIN_CM` and `MIXB_X + X_N`, none of them
  inferred.

Plus a `static_assert` that the four submodels tile the vector with no gap and
no overlap, and a per-bit `mix_expect()` after each submodel — four integer
compares against a ~3000 ns budget, which is not worth gating behind a flag.

**Not done, and deliberately:** the `add()` calls still walk `tx[nx++]` rather
than writing `tx[BASE+i]` directly. That conversion has to reach into
`ContextMap::mix1t`, `mix2`, `RunContextMap::mix` and `SmallStationaryContextMap::mix`
to thread a slot index through, and it is the step that buys the serial-chain
removal and the disjoint slices for §11.3. The layout above is its prerequisite
and is worth having on its own — it is what makes that conversion checkable
rather than hopeful.
