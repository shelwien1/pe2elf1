# What to try next — round two

Written against the tree as it stands (`RC_FUSE_PP_ENC/DEC`, `RC_CHUNK=2048`
taken; `RC_FOLD_RPRE`, `RC_EAGER_CTY`, `RC_DEC_UNROLL`, 512-bit vector width
measured and rejected). Single-threaded throughout: `RC_THREADS` is out of
scope here and every figure below is a one-core figure.

The regime changed in the last round, and that is the whole story of this one.

---

## 0. Where the time is now

3.290 GHz (`misc/clk.c`), clang 18.1.3, 20 MB of enwik8, default knobs:

| `-march=skylake` | MB/s | cyc/byte | cyc/group | |
|---|---|---|---|---|
| encode | 78.90 | 41.7 | 83.4 | |
| decode | 47.31 | 69.5 | 139.1 | **8.69 cyc/bit** |

| `-march=native` | MB/s | cyc/byte | cyc/group | |
|---|---|---|---|---|
| encode | 81.79 | 40.2 | 80.4 | |
| decode | 46.95 | 70.1 | 140.1 | 8.76 cyc/bit |

Decode still costs **1.67x** encode (was 1.83x), so it is still the slow side,
but it is no longer slow for the reason the last plan assumed.

**Decode is no longer chain-bound.** `RC_FUSE_PP_DEC` took the dependent
`pp[state]` load off the per-bit chain, and what is left runs ~25 instructions
per bit at 8.77 cyc/bit — 2.9 IPC against a 4-wide machine. The 4-wide issue
floor at that instruction count is **6.25 cyc/bit**, so at most ~28% of the
current decode time is recoverable by latency work, and the rest has to come
out of the instruction stream or the front end.

**Encode is still rename-bound on both loops**, and `model_pass` is still the
majority of it. The split has not been re-measured since `RC_CHUNK` and
`RC_FUSE_PP_ENC` landed; it was 57/43, and both changes touched the model pass,
so re-measure before spending it.

### The calibration the last round handed us

Two decode points on the same box, same stream, same everything else:

| | chain | insn/bit | cyc/bit |
|---|---|---|---|
| default | ~10 | 25 | 8.77 |
| `RC_EAGER_CTY=1` | ~6 | 31 | 9.08 |

`RC_EAGER_CTY` shortened the chain by ~4 cycles, paid +6 instructions per bit
for it, and came out 3.4% behind. Fitting the crudest possible blend,
`T ≈ a·chain + b·insn`, to those two points gives **a ≈ 0.28, b ≈ 0.24** — i.e.
a marginal instruction costs ~0.24 cycles (≈4.2 IPC at the margin, which is the
right order for a 4-wide machine) and a cycle of chain latency shows up at
~0.28 of face value, because part of it is already being overlapped.

Two things follow, and they set the whole decode agenda:

1. **A chain shortening that costs no instructions is still worth having.**
   `RC_EAGER_CTY` did not fail because latency stopped mattering; it failed
   because it bought latency at 1.5 instructions per cycle saved. The fit says
   halving the effective chain at zero instruction cost is worth ~16%.
2. **Instructions are now the expensive currency**, at roughly 0.24 cycles
   each. Anything that removes one from the per-bit path is worth about as much
   as a cycle of chain is worth to remove four.

Both directions are open, and they compose.

---

## 1. Decode: shorten the chain without paying instructions

### 1.1 The model graph is a wavefront, not a chain

`P_byte` is `c = ctx&0xFF; ctx = 1;` and `model_pass` does the same thing with
`lctx = 1`. **The bit-context chain restarts at every byte and carries nothing
across.** The only cross-byte link is the counter values.

Index the work as (byte *n*, bit *k*). Two kinds of edge exist:

- `(n,k) → (n,k+1)` — the context chain. Eight steps, then it ends.
- `(n,k) → (n+1,k)` — a counter RAW through `cty[]`, and only when the two
  bytes share their first *k* bits, because at bit *k* the context lies in
  `[2^k, 2^(k+1))` and depth-*k* nodes are touched at bit *k* and nowhere else.

So the graph is a wavefront of depth 8, not a 16-step serial chain per group.
A group is exactly two bytes — lanes 0–7 make one, lanes 8–15 the next — and
they can run as two overlapping 8-step chains.

**The correctness condition is weak**: byte *n*'s step *k* must stay before
byte *n*+1's step *k* in program order. Nothing else. Depth-*k* nodes are
touched only at bit *k*, so interleaving

    ctx0 = ctx1 = 1;
    for k in 0..7:
        step(byte0, bit k, lane k)      // reads/writes a depth-k node
        step(byte1, bit k, lane k+8)    // same depth, strictly after
    c0 = ctx0&0xFF; put0(c0);
    c1 = ctx1&0xFF; put0(c1);

is bit-exact with the model unsplit and the stream unchanged. The RC side does
not notice: lanes are independent and each is still touched once per group,
with its bits in the same order.

**Instruction cost: about zero.** The same loads, the same updates, the same
`rc_Process` calls, with `j` still a compile-time constant under the full
unroll. What it needs is two contexts instead of one — and `predict.inc`
already has the API for that: `P_S_L`/`P_update_L` take a caller-held context.
Which leads to the item that should be done first, on its own:

### 1.2 Move the decoder to the caller-held context

The decoder still uses `P_S`/`P_update`, i.e. the member `ctx`. The encoder's
`model_pass` uses `P_S_L`/`P_update_L`, and `predict.inc` records exactly why:
the store to `cty[ctx]` may-alias `this->ctx` — `ctx` is a loaded index, so
nothing bounds it — LICM gives up, and the context does not stay in a register.

The decoder pays that today for no reason. This is a few lines, stream-neutral,
and it is the prerequisite for §1.1. Falsifier: the group loop's static
instruction count, currently ~405 for 16 bits.

### 1.3 What the two together should be worth

The blend from §0, with chain 10 → 5 at unchanged instruction count, says
8.77 → ~7.4 cyc/bit, i.e. **+19% decode**. The hard ceiling on any pure-latency
change is the 4-wide issue floor at 25 instructions per bit, 6.25 cyc/bit
(+40%). Treat +19% as the estimate and +40% as the wall.

If it lands, re-run the constant-`p` probe: past the point where the chain is
fully hidden, the remaining time is all instructions and front end, and every
later item should be judged on instruction count alone.

**Deeper than two bytes.** Byte *n*+2 relates to *n*+1 exactly as *n*+1 does to
*n*, so the wavefront can run wider — but bytes 2 and 3 live in the next
group's lanes, so it crosses the group boundary and drags the refill and the
tail handling with it. Do two, measure, then decide.

**Encode gets nothing from this.** `model_pass` is at the rename ceiling;
overlapping two chains in a loop that is already issue-limited buys nothing,
and the encoder's model pass does not wait on anything.

---

## 2. Decode: take instructions out

At ~0.24 cycles per instruction per bit, these are worth roughly what they
weigh. All three are stream-neutral.

### 2.1 One 32-bit load for the refill

The substreams run backwards, so a single unaligned load at `q−3` covers both
cases with no masking:

    w = load32(tmpbase + q - 3);   // LE: b[q-3] | b[q-2]<<8 | b[q-1]<<16 | b[q]<<24
    n == 1:  add = w >> 24;                    // b[q]
    n == 2:  add = w >> 16;                    // b[q-1] + (b[q]<<8)
    code = (code << 8n) + add;  range <<= 8n;  tmpptr = q - n;

Two byte loads and a shift-or become one load and one shift on the taken path,
and — because the fully unrolled loop stamps out 16 copies of this — the loop
body shrinks by rather more than the hot path does, which matters for §2.3.

The window reads 3 bytes below the cursor. `RC_FF_PADSIZE` is
`RC_FF_TRIM + 2*RC_LOWBYTES + 16` = 64 with a `static_assert` requiring only
`RC_FF_TRIM + RC_LOWBYTES` = 40, so there are 24 bytes of slack below whatever
the decoder over-reads today. It fits, but assert it rather than assume it:
tighten the existing `static_assert` to `RC_FF_TRIM + RC_LOWBYTES + 3` so a
future `RC_FF_TRIM` change cannot quietly break it.

The same identity applies to `rc_Init`: `code` is assembled from four
descending `get()` calls, which on little-endian is exactly `load32(q−3)` with
no byte swap. Once per lane per block, so it is only worth doing for the code
size.

### 2.2 Pair the output stores

Two bytes per group, to consecutive addresses. One 16-bit store instead of two
byte stores. Trivially small, and it falls out of §1.1's restructuring, where
both bytes complete together anyway.

### 2.3 Is the decoder front-end bound? Measure it, then act

The decode group loop is 1756 bytes. At 16 bytes/cycle of legacy fetch that is
110 cycles per group = 6.9 cyc/bit — close enough to the measured 8.77 that
DSB residency is a live question, not a theoretical one. Check
`IDQ.DSB_UOPS` against `IDQ.MITE_UOPS` and `IDQ_UOPS_NOT_DELIVERED` before
doing anything about it.

If it is running from MITE, the fix is to shrink the body **while keeping the
full unroll**. That is not what the last round tested: `RC_DEC_UNROLL` reduces
the unroll factor, which makes `j` a runtime value, turns the lane arrays into
indexed accesses and the `(j&7)==7` test into a real branch — it shrinks the
body by adding instructions to the hot path, which is why it lost at every
factor. The untested version is to keep `j` constant and sink the 16 refill
blocks out of line as *cold blocks* (placement), not as a `NOINLINE` helper: the
refill fires on ~1.3 lanes per group, so a real call would cost several uops
plus register discipline in the common case. §2.1 shrinks the same 16 copies
from the other direction and the two compose.

---

## 3. Decode: the kernel still carries the encoder's state

`mk_kernel.sh` resolves `rc.inc` once, with `RC_VECOUT=1`, and both models
include the same `rc_vecD.inc`. The prelude declares `lowl, lowh, rpre, Carry,
code, range, FFNum, Cache, stcl, stad, stn`; the decoder reads `range`, `code`,
`tmpptr` and `rpre`. The dead ones are not free even when the stores are
eliminated: `ALIGN(VECSIZE)` on the declarations forces `and rsp,-64`, which
forces a frame pointer and takes `rbp` out of the allocator for the whole
function — the documented reason `model_pass` is `NOINLINE` in the encoder.
The decoder is paying that for arrays it never touches.

An `f_DEC` guard in the generator's emission (or a second generated file) fixes
it, and if the decoder's lane state is scalar per lane — which it is, and which
§4 argues it should stay — the alignment can go too.

Two things to check in the same pass:

- `rc_Init` runs `RC_STAGE_INIT(rcidx, tmpptr[rcidx]-2)` unconditionally,
  before the `f_DEC==1` branch. In the decoder that address is inside the
  *input* row. Presumably the macro is a no-op there; it should be visibly one.
- The generated file is `rc_vecD.inc` and both models include it, but
  `KERNEL_CONF` is asserted only in `model0.inc`. Cheap to assert in both.

---

## 4. What not to do to the decoder

Scalar `imul` is 3 cycles; `vpmulld` is 10, plus the extracts to get the
selected product back into a register. Vectorizing the per-bit resolve puts
both on the critical path of a loop that still has ~28% of its time in latency.
The per-bit step stays scalar.

The lane-local state is a different matter: lane *j* is touched once per group,
so its `range`/`code`/`tmpptr` chain has ~139 cycles of slack and nothing in
the bit loop reads another lane. That is why §2.1 is an instruction-count item
rather than a latency one, and it is also why nothing about where `code -= rpre`
sits, how the range select is spelled, or whether the cursor is an index or a
pointer can matter for latency. If the group's bookkeeping is ever batched into
vector form at the group edges, it will be to remove instructions, and it
conflicts with dropping the alignment in §3 — decide that on the counter, not
in advance.

Gathers stay out on both sides: 16 backwards cursors are a poor gather
workload, and an AVX2 ymm gather is several uops and eight separate L1
accesses.

---

## 5. Encode: what is left

Nothing here is large. The sweep is against rename and the ALU ports at once,
and `model_pass` is against rename; the profile's conclusion still holds — the
next real win in the model pass would have to change what the counter update
does, not how it is addressed.

### 5.1 `FFNum` → one register accumulator

The carry fallback is block-granular: the only consumer is `ncarry &= FFNum[j]`
over all lanes, followed by a whole-block re-code. Per-lane `FFNum` is dead
information. Replace the two spilled arrays with one accumulator that never
touches memory —

    carryacc = vpand(carryacc, cy_mask);    // once per half, in ShiftLow

— and reduce it once at block end. Today those slots cost a store+reload and a
load+and+store every group.

This is not the same trade as `RC_FOLD_RPRE`. That one moved the staged store's
value onto the same iteration's `vpmulld`; this changes no dependency at all,
it only stops spilling a mask that is already computed. Which is an argument
for trying it, not a prediction — count instructions first, and run with the
control binary in the rotation.

### 5.2 The four emission items in the listing

`range>>15` spilled on the low half and reloaded as `vpmulld`'s memory operand;
lane 15's store address computed twice when `stad[15]` already holds it; two
broadcasts rematerialised per group with one re-spilled for a single use;
`add/cmp/jb` where a negative index counting to zero would be `add/jne`. ~9
uops together. Item 2's premise was "the freed registers from `RC_FOLD_RPRE`
will fix it", which is gone, so it needs forcing directly.

These are generator-emission changes, not source-level hopes: constants
declared before the lane arrays, a biased `pbit` base with a negative induction
variable, an explicit `stad[15]`. For a constant used *once* per group, a
32-byte `.rodata` operand beats broadcast + spill + reload and beats holding a
register; for one used repeatedly, keep the register. Per constant, not
uniformly.

### 5.3 `-fno-pie`

+0.4% encode, +2.5% decode on AVX2. A packaging call (no ASLR for the
executable) rather than an experiment.

### 5.4 Re-measure the encode split

57/43 predates `RC_CHUNK` and `RC_FUSE_PP_ENC`, both of which touched the model
pass. The `model pass only` probe in `avx2_profile.md` section 8 is the way to
get it. Everything in §5 is aimed at the 43% side; if the split moved, the
priorities move with it.

---

## 6. Settled — do not retry

Measured on this box, on this tree, byte-identical streams throughout:

| | |
|---|---|
| `RC_FOLD_RPRE=1` (apply `rpre` where produced) | −2.55%, and it *shortened* the loop by 6 instructions and 2 stores |
| `RC_EAGER_CTY=1` (fetch the child counter pair early) | −3.4% decode; +6 insn/bit for −4 cycles of chain |
| `RC_DEC_UNROLL` 1/2/4/8 | −7.6% to −30% decode |
| 512-bit vector width (`-mprefer-vector-width=512`) | −3.2% encode, and the sweep loop did halve |
| `RC_SCATTER_SKIP` 1/2/3 | loses on both targets; it is the misprediction, not the work |
| `RC_SCATTER=0` on AVX2 | +8.43 cyc/grp |
| commit deferred one group | +5.52 cyc/grp |
| `pp` fused into `FSM`, fat-struct shape | +3.42 cyc/grp — the two-scaled-term address, not the table size; `RC_FUSE_PP` is the shape that works |
| prefetching `pbit` | noise |
| `-mprefer-vector-width=128` | much worse |
| `RC_SHIFT_SAT=1` | slower — clang will not fold the poison away |
| reading `pbit` through a signed `short*` | −2.1% |
| BMI2 `rorx` for the bit extract | identical codegen |
| clang 23.1.0 | −7.4% encode on AVX2, −6.1% on AVX-512 |

Rejected on paper: packing four `pbit` pairs into one 64-bit store (saves 12
store uops, costs ~24 shift/or uops, and the model pass is rename-bound with
the store port at 62%); storing a state index in `pbit` instead of `p` (the
sweep would then need a 16-lane `pp[]` gather); `vpmuludq` pairs instead of
`vpmulld` (3–4 uops against 2, and the sweep is not latency-bound).

Also settled by construction, not by measurement:

- **Vectorizing the decode model across lanes.** There is one model, shared.
  Bit *i* goes to lane *i*%16, but every bit passes through a single counter
  tree in bit order and one output byte's eight bits come from eight different
  lanes. There is no per-lane model state to hold in a vector register. The
  parallelism that does exist is §1.1's, between bytes.
- **Byte-per-lane interleaving.** Mapping byte *b* to lane *b*%16 does not let
  16 bytes decode at once — byte *b*'s probabilities still depend on every
  counter update from bytes 0..*b*−1. Getting 16 concurrent byte streams
  requires 16 independent models, which is a ratio loss, not merely a format
  change. That is what `RC_STRIPE` in `misc/bench/rc_decoder_proto.patch` is,
  and why it is a patch behind a build-time constant rather than a knob.
- **`(range*p)>>15` in place of `(range>>15)*p`.** Different rounding,
  different stream.

---

## 7. Order

1. §1.2 caller-held context in the decoder. Small, and §1.1 needs it.
2. §1.1 two-byte interleave. Zero-instruction chain shortening is the one thing
   the `RC_EAGER_CTY` result says is still worth having; estimate +19%, ceiling
   +40%.
3. §2.3's counters, then §2.1 and §2.2. If DSB residency is the problem, the
   cold-block placement of the refill is the untested variant and it composes
   with the 32-bit window.
4. §3 decoder-only kernel emission.
5. §5.4 re-measure the encode split, then §5.1 and §5.2 against whatever it
   says. §5.3 whenever packaging is decided.

Re-run the constant-`p` probe after step 2. It is what says whether anything
after it should be judged on latency at all.

---

## 8. How to measure

The box has a ~2% spread on best-of-N, which is the size of most of these
effects.

1. Round-robin the variants, N iterations each, medians of best-of-N across
   runs. Never a single run, never best-of-N alone.
2. A byte-identical copy of the baseline binary in the rotation, reading
   0.0% ±0.5%. If it does not, the box is busy and the run is worthless.
3. Count static instructions before believing a timing. Every item above has a
   counting falsifier. Three changes in this project have now shortened a loop
   and lost anyway; static counts tell you the change happened, only the clock
   tells you it helped.
4. `t.sh` after every change: roundtrip, and byte-identical against the `-C`
   scalar reference. Everything in §1–§5 is stream-neutral; if the stream
   moves, the experiment is over regardless of the timing.
5. For §1.1 and §2.1 specifically, add a per-bit differential harness — `bit`,
   `range`, `code`, cursor and context after every decoded bit against the
   current decoder. An off-by-one in a refill rewrite or a mis-ordered counter
   update surfaces as a wrong output file thousands of bytes later, if at all,
   and `misc/bench/rc_decoder_probes.patch` already has the scaffolding for
   variant builds of that loop.

<!-- -->

    ARCH=skylake ./t.sh                            # correctness, both directions
    ./coder c ../enwik8 /dev/null ../FSM0.txt 6    # encode timing loop
    ./coder d t.enc t.dec ../FSM0.txt 6            # decode timing loop
    cc -O2 misc/clk.c -o clk && ./clk              # the clock the figures are against
