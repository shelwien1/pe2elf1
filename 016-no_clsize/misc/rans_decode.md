# Why the rANS decoder is scalar, and what does not fix it

Read off the disassembly of `Model<0>::do_process` and `Model<1>::do_process`
(AVX-512 builds), plus the shapes this suggested, ported and measured. Decode
MB/s throughout are 20 MB of enwik8, best of 4-5 round-robin rounds,
cascadelake, clang 18.

## 1. The measurement that frames everything

|  | vector instructions | total | ratio |
|---|---|---|---|
| encoder `m0` | 385 | 843 | 46% |
| decoder `m1` | **130** | **1099** | **12%** |

and of the decoder's 130, the top entries are `vmovdqa` (66) and `vpextrd`
(24) -- lane state being moved across the scalar/vector boundary, not computed
on. The decoder is not "partly vectorised": it is scalar code that pays to
keep its state in vector registers.

That is not a defect in the code generation. It is what the model is: a byte
is one walk of a context tree, `ctx = ctx*2 + bit`, and bit *k* cannot be
decoded until bit *k-1* is known. `RCNUM/8` such walks run side by side
(model1.inc's wavefront) and that is all the parallelism there is.

## 2. The per-bit body

22 instructions, unrolled sixteen times per group:

```
    cmpl  $8388607, %r10d      ; x < RANSL ?          the renorm test
    jbe   .LBB11_13            ;                      out-of-line, 7.83% taken
    movl  4(%rdi), %ecx        ; cty[lctx]            fused {p, state}
    movzwl %cx, %edx           ; p
    movl  %r10d, %r8d          ; ---- three uses of x force two copies
    movl  %r10d, %r9d          ;
    andl  $32767, %r10d        ; s = x & (M-1)
    shrl  $15, %r8d            ; h = x >> 15
    imull %edx, %r8d           ; a = p*h            <-- starts immediately
    subl  %edx, %r9d           ; x - p
    subl  %r8d, %r9d           ; x - p - a            arm 1
    addl  %r10d, %r8d          ; a + s                arm 0
    xorl  %esi, %esi           ;
    cmpl  %edx, %r10d          ; s >= p ?
    setae %sil                 ; bit
    movl  %r8d, %edx           ;
    cmovael %r9d, %edx         ; x'
    movl  %edx, 328(%rsp)      ; spill x'           <-- register pressure
    shrl  $16, %ecx            ; state
    leal  (%rsi,%rcx,2), %ecx  ; state*2 + bit
    movl  (%r13,%rcx,4), %r8d  ; the FSM entry
    movl  %r8d, 4(%rdi)        ; write the counter back
```

Roughly 13 instructions of coder, 6 of model, 2 of renorm test, 1 spill.

**Corrected.** An earlier version of this section put the clock at ~3.5 GHz
from a guess, got ~10 cycles/bit and IPC "near 2", and concluded the loop was
bound by instruction count. `misc/clk.c` measures this box at **3.215 GHz**,
and 42.9 MB/s is 2.91 ns/bit = **9.36 cycles/bit**; with the refill amortised
at 7.83% the body is ~23.1 instructions, so **IPC is ~2.5**. That is not an
instruction-bound loop on a 4-wide machine -- it is about 38% of the issue
width idle, which is the signature of latency, branch mispredicts or the front
end. The measured shapes below are still what they are, but the *reason* is
not the one this file first gave: they lose because of what they add to the
chain or to memory traffic, not because the loop had no slots left.

The out-of-line refill is 14 instructions and runs on 7.83% of bits, so about
1.1 more amortised. `shldl %cl, %edi, %edx` does the whole `x = (x<<sh) |
(w>>(32-sh))` in one instruction, which is as good as that gets.

## 3. RC_DEC_SPLIT, ported -- and why it does not transfer

`rc.inc` cuts the range coder's decode step in two at the model, so that the
part which does not mention `p` can run for a whole group before the serial
walk. The precondition holds for us exactly: a group is RCNUM bit-slots and
lane *j* takes exactly one, so no lane's `x` moves between the top of the
group and its own step. And the cut line exists, because

    bit = (x & (M-1)) >= p

has no `p` on the left. So `s` can be produced for every lane up front, and
the walk becomes a load, a compare and the context step.

Ported as `RC_RANS_SPLIT`, correct (`t_matrix.sh` roundtrips both shapes),
and **much slower**:

| | decode MB/s |
|---|---|
| unsplit | **42.4** |
| 1 -- full split, p and bit carried per lane between passes | 28.3 |
| 2 -- renormalisation hoisted only, bit and update left in the walk | 25.9 |

The reason is what the split is *for*. In the range coder it moves a
**division** off the serial walk -- `code/(range>>SCALElog)`, which `RC_DIVALL`
then computes for a whole group at once. rANS's decode step has no division;
what the split can move is one `imull`. Carrying `s`, `p` and the bit between
passes costs three or four memory operations per bit, against an instruction
count that was already the binding constraint. Shape 2 is worse than shape 1
because it pays the same hoisting for less of the saving.

**That conclusion does not generalise, and this file first over-reached with
it.** The correct statement is narrower: a regrouping that spills state
between passes loses *when the regrouped half stays scalar*. Shelwien's
`rans_shapes` harness makes the same split pay by carrying it further -- the
body also loses its model load to a children-pair lookahead under a lag-2
schedule, and the edge is rewritten in AVX2 intrinsics so the refill, the
multiply and the state select run 8 lanes at a time. Reproduced here on the
harness (100 MB enwik8, `-march=native`, min of 3, two runs):

| harness shape | this box | their 7820X |
|---|---|---|
| A current shape | 41.5 | 58.1 |
| B split, plain-C edge | 29.8 | 57.2 |
| C2 split + pair lookahead, lag 2 | 29.3 | 61.2 |
| D C2 + AVX2 intrinsic edge | 42.2 | 80.5 |
| **F D + rolling two-byte pipeline** | **44.8** | **85.7** |
| E lookahead only | 37.1 | 53.9 |
| G early FSM pair | 32.5 | 54.1 |
| H one-byte refill | 40.3 | 59.8 |

The **ordering reproduces** -- F is the best shape on both machines and the
split alone is the worst -- but the **magnitude does not**: F is +8% here
against +44% there. Not the gather: replacing `edge8`'s
`_mm256_i32gather_epi32` with eight scalar loads costs D 20% and F 39% on this
box, so the gather is earning its place, not blocking it. The gap is the
hardware, and what it means practically is that the port's payoff has to be
measured on the target rather than assumed from the harness.

## 4. The step's algebra -- the default is not the obvious one

`RC_RANS_DECS` builds the same value three ways:

| | | decode MB/s |
|---|---|---|
| 0 | `x = bit ? (x-p-a) : (a+s)`, `a = p*(x>>15)` | **42.3** |
| 1 | `f = bit ? M-p : p; c = bit ? p : 0; x = f*(x>>15) + s - c` | 32.0 |
| 2 | as 1, with the two selects as mask arithmetic | 31.3 |

Shape 1 is the textbook form and looks cheaper -- one multiply, no duplicated
arm. It costs a quarter of decode, because its multiply comes *after* the
select that needs the bit, which puts three to five cycles of multiply latency
on the walk. Shape 0's multiply is the first thing in the step and depends
only on `x` and `p`, both of which are already there. Computing both arms is
how it buys that, and both arms share the multiply, so the duplication is two
adds and a `cmov`.

## 5. What the renorm shapes say

`rc.inc` carries fourteen `RC_DEC_RENORM` shapes. Most of what they explore
does not apply here, and the measurements above say why:

- **branchless refills** (shapes 2-5) trade a predicted branch for
  unconditional work. Ours fires on 7.83% of bits and the second byte 19 times
  in 12.5 million, so the branchy shape skips ~6 instructions 92% of the time
  and the branchless one cannot win against an instruction-bound loop.
- **`win` caching** (shape 9) exists to take the refill load's latency off the
  chain in a *branchless* shape. Behind a branch, that latency is already only
  paid 7.8% of the time and is not on the common path at all.
- **`clz` instead of the nested ternary** (shape 13) is about clang lowering
  `(a<a0) ? (a<a1) ? 16 : 8 : 0` through AVX-512 **mask registers**, at three
  cycles a `kmovd` across the domain. Our refill is inside a branch, so the
  inner select is a two-way `setb`/`lea` pair in GPRs -- there is no mask
  domain to escape. The Windows disassembly shows exactly that:
  `cmp; setb; lea ecx, 8[rax*8]`.
- **`RC_REFILLALL`** (shapes 11-12) moves the refill out of the lane body to
  one gather per group. A gather costs ~12 uops for 8 lanes and would run for
  the ~1.25 lanes of 16 that actually renormalise. `RC_RANS_SPLIT` already
  measured the cheaper half of this idea at -39%.

The one thing that did transfer is where the renormalisation sits: **before**
the step, not after it, so the branch tests a state the previous step
finished with rather than one this step's multiply just produced. That is
already in `rans.inc`, and it is the same choice `rc.inc` makes.

## 5a. The deferred counter update, priced

Every wide-SIMD decode proposal -- gather the counters for 8 lanes, step them
as a vector, scatter back -- needs one thing first, and it is not an
optimisation but a **format change**. At depth 0 every in-flight byte reads and
writes `cty[1]`. `model_pass` walks the block's bits in linear order and the
decoder is only decodable if it reproduces that order, so any scheme that
reads the counters for several bytes before writing any of them codes a
different stream. Making it legal means a defined adaptation lag on both ends.

That is cheap to price and nobody had the number. `RC_DEFER_UPD=G` (encoder
only, `model0.inc`) makes the bytes of each group of G read the counters as of
the start of the group and land their writes at the end, last writer winning
-- "read all, then write all", which is exactly what a G-wide vector step
does. The decoder is not changed, so such a build does not roundtrip; the only
number wanted is the size. enwik8:

| G | bytes | vs G=1 | |
|---|---|---|---|
| 1 (current) | 62,588,528 | — | |
| 2 | 62,782,202 | +193,674 | **+0.309%** |
| 4 | 63,273,872 | +685,344 | +1.095% |
| 8 | 63,777,626 | +1,189,098 | **+1.900%** |
| 16 | 64,255,210 | +1,666,682 | +2.663% |

The path is faithful: forced on at G=1 (`-DRC_DEFER_CHECK=1`) it reproduces
the default stream byte for byte.

**This closes the wide-SIMD design space.** The test proposed for it was "a
few hundredths of a percent and a whole design space opens; 0.5% and close the
space permanently". A vector step over 8 bytes costs **1.90%**, nearly four
times that bar, and even the two-byte lag the current NB=2 wavefront would
need is 0.31%. For scale, the entire ratio gap between this rANS coder and the
range coder it replaced is 0.121% -- so a lag-2 format costs 2.6x the whole
rANS ratio penalty and a lag-8 format costs 16x. Nothing on the speed side
that has ever been measured here is worth that.

Note what the rule does **not** forbid, and it is the reason D and F are legal:
the *coder* state -- `rx`, `tmpptr`, `s`, the refill -- has no such constraint.
Lane *j* is touched once per group and its state is final before the group's
bit walk starts. That is the half those shapes vectorise, and it needs no
format change at all.

## 5b. Three more shapes, measured in the real codec

**The one-byte refill** (`RC_RANS_REFILL=1`). Replace the dword window and its
0/8/16 ternary with one byte and a nested branch for the second. Exact, not
approximate: RANSL is a multiple of 256, so after `x = (x<<8)|b` the test
`x<RANSL` is true exactly when the original `x` was below `RANSL>>8`. The
second branch fires 19 times in 12,520,077 refills. It is reported at +1.3% on
a 7820X. Here it is **-9.2%** (39.11 against 43.08), and the harness agrees in
sign (-2.9%). The dword form compiles to a single `shldl %cl, %edi, %edx` that
does the shift, the merge and nothing else; the byte form trades that for a
byte load, a shift, an or and a second branch in a loop that already has
sixteen. Kept as a knob with the measurement, because the sign disagrees
across machines.

**Depth-sliced context tables.** The proposal is that with a per-depth base the
`ctx = 2*ctx+bit` recurrence becomes a constant base plus a prefix and the
address arithmetic leaves the chain. It does not: at depth *k* the index is
already `(1<<k) | prefix`, so `lctx` *is* the flat index, `prefix = 2*prefix+bit`
is the identical recurrence, and the disassembly already folds the whole
address into the load (`movl (%r13,%rcx,4), %r8d`). Nothing to move. Not built.

**The early FSM pair load** and **the lookahead alone** were both measured
negative in the harness on this box too (G 32.5, E 37.1 against A's 41.5),
matching the reported -18% / -7% and this project's own `RC_EAGER_CTY` -3.4%.
The FSM load feeds a store and nothing else, so it is not on the bit chain.

## 6. What is actually left

Nothing large, and the analysis says where the ceiling is. Of the 22
instructions, 13 are the coder step and §4 shows that form is the cheap one;
6 are the model's own load-compare-walk, which no coder change touches; 2 are
the renorm test, which §5 says is right; 1 is a spill.

The spill is the only visible waste, and it is a symptom rather than a cause:
sixteen unrolled bodies with two live wavefront chains is more state than the
register file has. Nothing in the coder makes it smaller -- the encoder's
sweep has the same problem from the other direction (21 of its 82 instructions
are moves and spills), and `misc/rans_comparison.md` §11 has that measurement.

If decode is to get materially faster it will be through the model -- fewer
instructions per bit in the counter walk, or a wavefront wider than
`RCNUM/8` -- not through the coder. `RCNUM=32` widens the wavefront to four
chains and measures 39.9 against 42.9, so that particular door is shut too.
