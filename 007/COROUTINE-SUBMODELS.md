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

## 8. Verdict

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
