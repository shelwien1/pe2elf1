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

Roughly 13 instructions of coder, 6 of model, 2 of renorm test, 1 spill. At
~10 cycles/bit measured that is an IPC near 2 -- so the decoder is **bound by
instruction count**, not by either chain, and the way to make it faster is to
issue fewer instructions per bit, not to shorten anything.

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

This is the general shape of the answer: **the rANS decoder is cheap enough
per bit that any regrouping which spills state between passes loses.**

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
