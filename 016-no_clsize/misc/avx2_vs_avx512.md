# AVX2 and AVX-512 on the same coder

Three questions, off `m0_avx2.asm` / `m0_avx512.asm` / `m1_avx2.asm` /
`m1_avx512.asm`, the `log.txt` that came with them, and the two `%defs%` sets
in the `gc.bat` that built them:

1. Why did AVX2 and AVX-512 end up at almost the same speed?
2. Why do some options improve one side and hurt the other -- do those need
   two versions?
3. Why doesn't `RC_FOLD_RPRE=1` help AVX-512?

Short answers: **(1)** because 61% of the round trip is a decoder with zero
vector instructions in it, and 48% of what is left is a scalar model pass --
the sweep is the only part either ISA touches, and halving its instruction
count is worth 13%, not 50%. That 13% shows up here and does not show up on
your box, which is the one thing in the whole set that is genuinely odd; §1.3
says what to measure. **(2)** They don't. Every knob in your two def sets is
one-sided *by construction* -- the other column is measuring its own noise,
which is ±0.3% in your log. The real effect is three decoder knobs that lose
alone and win together. **(3)** On AVX-512 `RC_FOLD_RPRE` emits the *identical
70 instructions* either way -- same mnemonics, same counts, no spills on either
side. All it changes is the schedule, and it is the only knob in the set that
is purely a schedule. Those are the ones whose sign flips between machines.

## 0. Where the numbers come from

Two boxes. Yours, from the attached `log.txt`: clang 23, `-march=native
-mtune=native`, 100 MB of enwik8. The "avx2" build is the same target with
AVX-512 off -- `m0_avx2.asm` uses ymm0..ymm15, `vpextrd` and `vpminud`/
`vpcmpeqd` for the unsigned compares, and no mask register anywhere, against
`m0_avx512.asm`'s ymm16..ymm30, `vpcmpltud`, `vpmovm2d` and k0..k3.

Mine, for everything labelled *here*: a Xeon with AVX-512 BW/VL/FP16 (Sapphire
Rapids class) in a VM, clang 18, `-O3 -flto -march=native`, the same 100 MB of
enwik8, built from **your two def sets** verbatim -- `defs_avx2` with
`-mno-avx512f` added, `defs_avx512` as it stands.

Every figure below is the median of best-of-3, over 7 round-robin rounds, with
a byte-identical copy of the binary in the rotation as a control. The controls
came back within 0.5% (AVX2 pair) and 1.5% (AVX-512 pair), so 1.5% is the
noise floor on anything I claim here. Times are seconds per 100 MB, which is
your `log.txt`'s own second column and is what actually adds up; MB/s is
1048576 bytes, as `coder.cpp` computes it.

Absolute MB/s does not carry between the two boxes. Ratios do, and §1.3 is
entirely about one ratio that doesn't.

## 1. Why the two targets tie

### 1.1 The decoder is not a vector program, on either target

`m1_avx2.asm` and `m1_avx512.asm` both contain one hot loop -- `loc_1400FF080`,
16 lanes unrolled, one `imul` per lane. AVX2: 425 instructions. AVX-512: 434.
**Vector instructions in that loop, on either target: zero.**

That is not an optimisation that failed; it is the shape of the problem. The
encoder can run the model over a whole block and *then* code the bits, because
it knows all of them. The decoder cannot: it needs the coder's answer for bit
*i* before the model can produce the context for bit *i+1*. Model and coder are
interleaved per bit by construction, so there is no vector work to give either
ISA. The 9-instruction difference between the two dumps is register allocation.

Your measurements say the same thing to four digits:

| | AVX2 | AVX-512 | Δ |
|---|---|---|---|
| decode, baseline | 48.25 | 48.22 | −0.06% |
| decode, tuned | 54.07 | 54.15 | +0.15% |

And decode is the larger half: 1.761 s against 1.141 s, **61% of the round
trip**. Before the encoder is even considered, three fifths of the total is
identical on the two targets.

### 1.2 Half of the encoder is a scalar model pass

`Model<0>::do_process` is two passes over a block that share nothing but
`pbit[]`: `model_pass()`, which is scalar and the same code on every target,
and the coding sweep, which is the vector loop. Three builds separate them --
the two probes are now in the tree (`RC_PROBE_MODELONLY`, and
`RC_SCATTER_SKIP=9`, which used to mean something else on the AVX2 path and
now means "no commit" on both):

| build | | AVX2 | AVX-512 |
|---|---|---|---|
| `-DRC_PROBE_MODELONLY=1` | model pass only | 0.638 s | 0.653 s |
| `-DRC_SCATTER_SKIP=9` | + sweep arithmetic | 1.169 s | 1.088 s |
| (neither) | + commit = the real encoder | 1.355 s | 1.185 s |

Taking the model pass as the 0.646 s both builds agree on:

| part of encode | AVX2 | AVX-512 | Δ | share of AVX2 encode |
|---|---|---|---|---|
| model pass (scalar) | 0.646 s | 0.646 s | 0 | 48% |
| sweep arithmetic | 0.523 s | 0.442 s | −15% | 39% |
| commit (the RCNUM stores) | 0.186 s | 0.097 s | −48% | 14% |
| **encode** | **1.355 s** | **1.185 s** | **−12.6%** | |

So on this box AVX-512 is worth 12.6% of encode -- and encode is 39% of the
round trip, so 4.9% of the total. That is the ceiling the structure imposes,
and it is why the sweep's instruction count is so misleading: your two dumps
are **138 instructions (AVX2) against 66 (AVX-512)**, a 52% cut, and it buys
12.6%.

Where the 12.6% comes from is worth splitting once more, because it is not
where the instruction count says. `-DRC_SCATTER_W=0` keeps the AVX-512
arithmetic and commits with AVX2's block of 16 scalar stores:

| commit | cost | |
|---|---|---|
| block of 16 scalar stores, in the AVX2 build | 0.186 s | |
| the same source, in the AVX-512 build | 0.116 s | −0.070 s |
| two masked `vpscatterdd` | 0.097 s | −0.019 s |

The `vpscatterdd` **instruction** is worth 0.019 s -- 1.4% of encode. It is
microcoded and retires at roughly the rate the 16 scalar stores it replaces
retire; what it saves is the address extraction, and in the AVX-512 build that
was already cheap. The 0.070 s above it is the *same C code* running in a
function that has 32 vector registers instead of 16.

That is the difference the two dumps show most clearly. Counting memory traffic
in one iteration of each sweep loop:

| | AVX2 (138 insns) | AVX-512 (66 insns) |
|---|---|---|
| loads of `pbit[]` -- the real input | 2 | 2 |
| broadcasts of a loop constant from `.rdata` | **5** | 0 |
| register-to-register `vmovdqa` | **5** | 0 |
| vector stores to the stack | 8 | 0 |
| vector reloads from the stack | 4 | 0 |
| scalar reloads from the stack | **19** | 0 |
| stores that commit the group | 16 | 2 scatters |

Four of the eight vector stores and all nineteen scalar reloads are the
staging arrays doing their job -- `RC_SCATTER` writes `stad[]`/`stcl[]` as
contiguous vectors precisely so the commit can read them back scalar-wise
instead of extracting 24 addresses on port 5. The other four stores are plain
spills. On AVX-512 both categories are gone: the scatter takes its addresses
and its data straight out of registers, so the staged arrays are never
materialised at all.

The AVX-512 loop touches memory four times: two `vmovdqa64` of the packed
probabilities and the two scatters. Everything else it needs is in a register,
including the four constants (`0xfffffffe`, `0x00ffffff`, `0x0000ffff`, `16`)
and the two masks that AVX2 re-broadcasts every single iteration, which live in
ymm27..ymm30 and never move. That, not the scatter, is most of the win.

### 1.3 The part that is actually strange

Line up the two boxes on the three workloads:

| | yours | mine | yours / mine |
|---|---|---|---|
| decode (pure scalar, identical code) | 54.1 | 46.0 | **1.18** |
| encode, AVX2 | 83.2 | 70.4 | **1.18** |
| encode, AVX-512 | 83.6 | 80.5 | **1.05** |
| encode, AVX-512 + `RC_FOLD_RPRE=1` | 82.1 | 84.4 | **0.97** |

Your machine is 18% faster than mine at scalar work, and *exactly* 18% faster
at the AVX2 encoder. Your AVX2 build is behaving normally. It is the AVX-512
build that is out of line: it should be at ~95 MB/s on a machine 18% faster
than this one, and it is at 83.6. With `RC_FOLD_RPRE=1` it is slower than mine
in absolute terms, on a machine that is 18% faster.

So the honest answer to question 1 has two halves. The structural half --
61% decoder plus 48% scalar model inside encode -- caps the ISA's reach at
about 5% of the round trip and is true everywhere. The rest, the difference
between my 12.6% and your 0.5%, is *your AVX-512 build specifically*, and
nothing in the dumps you sent can settle it, because you sent the AVX-512
dump at `RC_FOLD_RPRE=0` and the AVX2 dump at `RC_FOLD_RPRE=1`.

Three builds on your box would settle it. All three are in the tree now:

```
rem what the two scatters cost you
set defs=%defs_avx512% -DRC_SCATTER_SKIP=9
rem the same encoder, committing the way the AVX2 build does
set defs=%defs_avx512% -DRC_SCATTER_W=0
rem how the encode splits on your machine
set defs=%defs_avx512% -DRC_PROBE_MODELONLY=1
```

The first two do not encode correctly (`SKIP=9` writes nothing, so the stream
is garbage) -- time them with a plain encode, `coder.exe c enwik8 nul
FSM0.txt 5`, not through `t.bat`. `-DRC_SCATTER_W=0` *does* produce the
reference stream and is in `t_matrix.sh` now, so if it wins you can just keep
it.

My prediction, and it is a prediction, not a measurement: `SCATTER_SKIP=9`
will show a much larger gap on your box than the 0.019 s it shows here, and
`RC_SCATTER_W=0` will be **faster** than your current AVX-512 build. The
supporting evidence is thin but it points one way -- your `RC_SCATTER_SKIP=1`
row (83.69 against 83.27 for `=0`) says masking the scatter down to the ~1.3
lanes that actually owe a store buys you 0.5%, which is what a scatter that
costs a fixed ~19 uops regardless of mask looks like. Two of those per group,
on a loop that is only 66 instructions long, is a large fraction of the loop.
If that is what is happening, `RC_SCATTER_W=0` gets you the register win
(0.070 s of the 0.089 s here) without the instruction that is costing you.

## 2. Options that help one side and hurt the other

They don't -- not one of them. Here is every knob in your two def sets, and
where its code physically is:

| knob | lives in | touches |
|---|---|---|
| `RC_DEC_WAVE` | `model1.inc` | decode only |
| `RC_DEC_PUTW` | `model1.inc` | decode only |
| `RC_DEC_ALIGN` | `model1.inc` | decode only |
| `RC_LOAD32` | `rc.inc`, under `if(f_DEC==1)` | decode only |
| `RC_DEC_COLD` | `rc.inc`, under `if(f_DEC==1)` | decode only |
| `RC_FOLD_RPRE` | `rc.inc`, under `if(f_DEC==0)` | encode only |
| `RC_SHIFT_SAT` | `rc.inc`, in `ShiftLow` | encode only |
| `RC_FF_LANES` | `rc.inc`, `model0.inc` (`FFNum`) | encode only |
| `RC_SWEEP_NEGIDX` | `model0.inc` | encode only |
| `RC_SCATTER_SKIP` | `model.inc` (the commit) | encode only |
| `RC_CHUNK` | `model0.inc` | encode only |

`f_DEC` is a template parameter, so `if(f_DEC==0)` is dead code the compiler
deletes -- an encoder-side change cannot reach `Model<1>` even in principle.

So when your log shows a decoder knob moving the encode column, that column is
measuring itself. It is worth knowing how much: across the eight rows of your
AVX-512 series that differ only in decoder settings, the encode column spans
**82.96 to 83.44 -- ±0.3%**. Going the other way, the decode column under a
group of rows that differ only in an encoder knob moves by at most 0.7%
(`RC_FF_LANES` 16/8/4: 51.94 / 52.23 / 52.32) and usually far less
(`SHIFT_SAT` 0/1: 54.12 / 54.11; `FOLD_RPRE` 0/1: 54.15 / 54.08). Any
cross-column effect smaller than that is not there.

### What *is* real, and it is not enc-vs-dec

Three decoder knobs each lose on their own and win in combination:

| | alone | in the tuned chain |
|---|---|---|
| `RC_DEC_PUTW=1` | 48.22 → 46.78 (**−3.0%**) | 52.42 → 53.93 (**+2.9%**) |
| `RC_LOAD32=1` | 48.76 → 45.56 (**−6.6%**) | 50.73 → 53.73 (**+5.9%**) |
| `RC_DEC_COLD=1` | 48.94 → 46.52 (**−4.9%**) | 53.73 → 54.22 (+0.9%) |

All three are decode-side, all three interact with `RC_DEC_WAVE=2`, and none
of them has anything to say about the encoder. `RC_DEC_WAVE=2` restructures the
decode loop so 16 lanes are in flight at once; `PUTW` and `LOAD32` each remove
one instruction from a per-lane dependency chain, which is worthless when the
loop is latency-bound on one lane and worth a lot when there are 16 chains
overlapping. That is a knob-ordering problem, not a two-versions problem -- and
it is why measuring knobs one at a time against a bare baseline mis-ranks them.
The tuned chain is the right baseline for a decode knob.

### If one ever does need two versions

The mechanism is already there and already used once. `RC_FUSE_PP_ENC` and
`RC_FUSE_PP_DEC` are the same source-level change (fold `pp[]` into the counter
state) split in two, because it wanted different answers on the two sides and
`f_DEC` being a template parameter means both can be compiled from one source
at no cost. `predict.inc` selects the counter per side:

```c
typedef PredictorT<RC_FUSE_PP_ENC> PredictorE;
typedef PredictorT<RC_FUSE_PP_DEC> PredictorD;
```

Any knob that turns out to be genuinely two-sided gets the same treatment. None
of the ones in your def sets is.

Your `gc.bat` splitting `defs_avx2` from `defs_avx512` is a different axis and
that one **is** real -- see `RC_SCATTER_SKIP=1`, which is +0.5% on AVX-512 and
**−25%** on AVX2 (61.44 against 81.98), because at `RC_SCATTER_W==0` there is
no mask and the only thing "skip" can mean is an unpredictable branch per
group. Keeping two def sets is right. It is the enc/dec axis that doesn't need
splitting.

## 3. Why `RC_FOLD_RPRE=1` doesn't help AVX-512

Because on AVX-512 it does not change the code. It changes the schedule.

`RC_FOLD_RPRE` moves `low += rpre` from `ShiftLow`, where it is applied to the
*previous* iteration's pending `rpre`, into `rc_Process`, where `rpre` is
produced. On AVX2 that is a real reduction: the sweep loop goes from 154 to
146 instructions, `vpextrd` count from 10 to 8, and stack references from 44 to
41. Worth nothing in time -- 69.90 → 69.92 here, 81.98 → 81.98 in your log --
but the instructions are genuinely gone.

On AVX-512 there is nothing to remove. Both loops are **70 instructions**, and
the mnemonic histograms are *byte-for-byte identical*: same instructions, same
counts, zero stack references either way. `diff` of the two sorted histograms
is empty. Only the order and the register assignment differ.

What the order does is move the scatters. With `RC_FOLD_RPRE=0`:

```
 1  vpaddd      ymm9,ymm9,ymm8          ; low += pending rpre
 2  vpcmpltud   k0,ymm9,ymm8            ; carry out
 3  vpmovm2d    ymm8,k0
 4  vpsubd      ymm11,ymm7,ymm8         ; <- the value that gets stored
...
58  vpscatterdd DWORD PTR [r15+ymm12*1]{k1},ymm11
...
63  vpscatterdd DWORD PTR [r15+ymm15*1]{k2},ymm13
```

The scatter's data register is produced four dependent operations into *this*
iteration, off the loop-carried `low`, so clang cannot place the scatters any
earlier than instructions 58 and 63 of 70. With `RC_FOLD_RPRE=1`:

```
28  vpscatterdd DWORD PTR [r15+ymm11*1]{k1},ymm10
...
32  vpsubd      ymm10,ymm9,ymm10        ; ymm10 written AFTER it is read at 28
```

`ymm10` is read at 28 and written at 32, so the value scattered is the
*previous* iteration's -- `low` was already final when `rc_Process` left it, so
`ShiftLow` has nothing to add and the staged value is ready a whole iteration
ahead. Address (`ymm11`) and mask (`k1`) are the previous cursor and the
previous range, also live at loop entry. All three operands ready, so clang
issues the scatters at 28 and 57 instead of 58 and 63.

Whether that is a win is exactly the question of whether the loop is
throughput-bound or occupancy-bound on the scatter:

| | AVX-512 `FOLD_RPRE=0` | `=1` | |
|---|---|---|---|
| this box | 80.5 | 84.4 | **+6.1%** |
| yours | 83.58 | 82.13 | **−1.7%** |

Here, starting a ~19-uop microcoded scatter 30 instructions earlier lets it
overlap the rest of the iteration, and it is worth 6%. On your box it is worth
−1.7%, which is the same sign the rest of §1.3 points to: if the scatter is the
long pole rather than something to be hidden, hoisting it does not help, and
you have paid for holding the previous iteration's staged value live across the
whole body.

This is the one knob in the set that has no instruction-count story at all --
identical code, different order -- so it is the one you should least expect to
transfer between machines or between clang versions (I am on 18, you are on
23). Your `defs_avx2` keeping it and `defs_avx512` dropping it is the correct
call for your box, and there is no general answer to port.

If §1.3's guess is right and `RC_SCATTER_W=0` turns out faster on your box,
re-test `RC_FOLD_RPRE=1` on top of it -- with the scatter gone, the reason it
loses goes with it, and the AVX2 measurement (`FOLD_RPRE` is 8 instructions
cheaper and time-neutral) says it should come back positive or flat.

## 4. What to run

In order, all on your box. 1--3 are timed with a plain encode,
`coder.exe c enwik8 nul FSM0.txt 5`; 4 and 5 go through `t.bat` as usual.

| # | `%defs%` | answers |
|---|---|---|
| 1 | `%defs_avx512% -DRC_PROBE_MODELONLY=1` | how your encode splits model vs sweep |
| 2 | `%defs_avx512% -DRC_SCATTER_SKIP=9` | what the two `vpscatterdd` cost you |
| 3 | `%defs_avx2% -DRC_SCATTER_SKIP=9` | what 16 scalar stores cost you |
| 4 | `%defs_avx512% -DRC_SCATTER_W=0` | AVX-512 arithmetic, AVX2 commit -- real stream, run it through `t.bat` |
| 5 | `%defs_avx512% -DRC_SCATTER_W=0 -DRC_FOLD_RPRE=1` | §3's follow-up, if 4 wins |

1--3 do not encode correctly -- they leave work out -- so `t.bat` would hand
its decode step a stream that does not decode. 4 and 5 produce the reference
stream, and `t_matrix.sh` covers `RC_SCATTER_W=0` now.

The numbers that would confirm §1.3: (2) much larger than the 1.4% of encode it
is here, and (4) faster than your current 83.58. The numbers that would refute
it: (2) small, in which case your AVX-512 sweep is losing its register
advantage somewhere the dumps don't show, and (1) becomes the interesting one --
if your model pass is a larger share of encode than the 48% it is here, there
is less sweep for either ISA to win in the first place, and the tie is just the
structure of §1.2 with a smaller numerator.
