# Where the AVX2 encode time goes

`Model<0>::do_process` at `-march=skylake` (AVX2, no AVX-512): what the cycles
are spent on, which of the obvious moves pay, and which do not. Every number
here is measured on this tree; the derivations say so where they are derived.

## Method

Machine: Intel Xeon, cascadelake (family 6, model 85, stepping 7), 4 vCPU,
32 KB L1d, 1 MB L2 per core. clang 18, `-O3 -flto`, `RCNUM=16`,
`BLKSIZE=64K`, `RC_LOWBYTES=8`, `RC_LOWSPLIT=1`. Corpus: the first 20 MB of
enwik8, `./coder c /tmp/e20 /dev/null ../FSM0.txt 6`, best of 6 iterations
across 5-6 runs.

**The clock is not the one `/proc/cpuinfo` reports.** It says 2.8 GHz; a
dependent-`add` chain measures the core actually running at **3.235 GHz**.
Every cycle figure below is at the measured clock. This matters: at 2.8 GHz
the model pass came out at 23.9 cycles per input byte for 111 instructions,
which is 4.65 IPC on a 4-wide machine -- impossible, and the tell that the
clock assumption was wrong.

    gcc -O2 -o clk misc/clk.c && ./clk    # 4 dependent adds/iteration, 1 cycle each

Components are measured by deletion, with probe builds whose output is
deliberately wrong:

| probe | what it removes |
|---|---|
| model pass only | the whole coding pass (`g_codepass` guard) |
| no store | `(uint&)tmpbase[tmpptr-2] = cl` in `ShiftLow` |
| staged, no commit | the staged commit block, staging kept |
| staged, cold arrays | commit reads globals instead of the just-written staging |

"cyc/grp" throughout is cycles per **16-bit group** -- 16 coded bits, one pass
of the unrolled RCNUM-lane sweep, two input bytes.

## The budget

    encode total                    102.41 cyc/grp   (63.18 MB/s)
      model pass                     55.06   54%
      coding sweep                   47.35   46%
        of which the ShiftLow store  14.71   31% of the sweep

For contrast, the same tree at `-march=native` (AVX-512, `RC_SCATTER=1`):
84.42 total, 55.06 model pass, 29.36 sweep, 1.96 of that the scatter. **The
model pass costs the same on both targets. The entire AVX2/AVX-512 gap is the
sweep, and almost all of that is the store.**

## 1. The model pass -- 54%, and it is at the ceiling

One iteration of `BB11_8` is one input byte: 111 instructions, ~110
fused-domain uops, measured 27.53 cycles. Skylake renames 4 uops/cycle and the
loop contains exactly one macro-fusible pair, so the floor is 27.5 cycles.
**It is rename-width bound and it is already there.** Nothing that does not
remove instructions can help it.

Per byte: 23 loads, 17 stores, 1 branch. The store port is 62% busy, the load
ports 42% -- neither binds. The 8 bits of a byte are *not* serialised: `bit`
comes from the input byte, so `ctx` is known ahead and the counter loads
(`cty[ctx].state`, `pp[state]`, `FSM[state].s[bit]`) issue independently.
That is why 111 instructions fit in 27.5 cycles at all.

**The one piece of pure waste is PIE.** `pp[]` and `FSM[]` are `extern`
globals, so every reference goes through the GOT, and clang rematerialises the
GOT load rather than keeping a base register:

    movq  pp@GOTPCREL(%rip), %rdx
    addw  (%rdx,%r8,2), %r9w
    ...
    addq  FSM@GOTPCREL(%rip), %r13
    movzwl (%r13,%r8,4), %r8d

12 `GOTPCREL` loads in `do_process`, 6 in the model-pass loop alone.
`-fno-pie -no-pie` removes all 12. Measured **-2.64 cyc/grp** on the inline
build. Cheap and free of risk; not made the default here because it is a
packaging decision (no ASLR for the executable), so it is left as
`OPT='-O3 -fno-pie -no-pie' ./build.sh`.

Two things that look like they should help the model pass and do not:

- **Fusing `pp[]` into `FSM[]`** so one state is one cache line and one base
  register (the layout the commented-out `word pp;` in `FSM.cpp` hints at):
  **+3.42 cyc/grp, i.e. 3.3% slower.** The split arrays are the better layout;
  padding the fused entry to 8 bytes makes the table 256 KB and costs more in
  L2 than it saves in lines touched.
- **Prefetching `FSM`/`pp`:** they are 128 KB + 64 KB, L2-resident, and the
  loop is rename-bound, not miss-stalled. A prefetch is another uop against
  the binding constraint.

## 2. The coding sweep -- 46%, saturated on two ceilings at once

One iteration of `BB11_14` is 16 coded bits: 134 instructions, 161
fused-domain uops, 125 of them into the p0/p1/p5 ALU pool. Measured 42.05
cycles at 2.8 GHz-scaled, 47.35 at the true clock against the two ceilings:

    rename       161 / 4-wide          = 40.3 cycles
    ALU ports    125 / 3 ports         = 41.7 cycles
    measured                             42.05

96% and 99% of both (the uop classification is derived, by hand, from the
disassembly; the 42.05 is measured). The loop-carried `range` chain is ~20 cycles
(`vpmulld` 10 + `vblendvps` 2 and so on) and the two independent 8-lane halves
cover it easily, so this is **not** a dependency-limited loop and not a
store-port-limited one.

The store is 62 of those 161 fused uops. `(uint&)tmpbase[tmpptr-2] = cl` goes
to a per-lane address, and SLP -- which is what vectorises the sweep, since
the sweep is straight-line code under `RC_UNROLL` and not a loop -- has no
scatter to emit, so it vectorises the arithmetic and then extracts:

    vpextrd  $1, %xmm12, %esi              ; x24, pull the addresses out
    vextracti128 $1, %ymm12, %xmm12        ; x4
    vpextrd  $1, %xmm9, 69632(%rbx,%rsi)   ; x16, the stores themselves

**31 port-5-only uops per group** on a port that retires one per cycle -- 28
of them (24 `vpextrd` + 4 `vextracti128`) from the store alone.

## 3. Does the delayed-store trick work on AVX2? YES -- +8.4%, shipped

On AVX-512 `RC_SCATTER` has `ShiftLow` stage the dword and its offset in
`stcl[]`/`stad[]` and the group commit with one `vpscatterdd`. AVX2 has no
scatter, so the commit is just a block of RCNUM scalar stores. It still wins,
because **the staging was never about the scatter -- it is about the address
extraction.**

    base sweep     134 insns   31 port-5-only uops   24 stores   17 loads
    staged sweep   151 insns    3 port-5-only uops   27 stores   50 loads

31 port-5 uops (1/cycle) traded for 33 extra loads (2/cycle). Measured:

| | MB/s | cyc/grp |
|---|---|---|
| inline store | 63.18 | 102.41 |
| **staged, block of scalar stores** | **68.85** | **93.97** |
| staged + `-fno-pie` | 69.48 | 93.12 |

**+8.4%**, byte-identical stream. This is now the default on AVX2:
`RC_SCATTER_W` gained a value `0` meaning "no scatter instruction, commit as a
block", and `RC_SCATTER`'s gate is per-width.

It does not pay at every geometry, because the staged arrays cost registers:

| RCNUM | inline | staged | |
|---|---|---|---|
| 8 | 59.69 | 61.65 | +3.3% |
| 16 | 63.35 | 68.72 | +8.5% |
| 32 | 63.15 | 61.13 | **-3.2%** |

so the gate is `RCNUM<=16` for the block form. And on AVX-512 the scatter is
still better than the block (75.4 vs 72.5), so the width selection stays.

## 4. Does "branch past all-zero" work on AVX2? NO

A lane whose renorm emitted nothing (`n==0`) owes no store -- the cursor did
not move, so its next store rewrites the same window. That is true, and all
the skip variants produce identical streams. The coder emits ~0.08 bytes per
coded bit, so a whole group of 16 lanes stages nothing about a quarter of the
time. It still loses, and by a lot:

| | cyc/grp | vs staged |
|---|---|---|
| staged, always commit | 93.97 | |
| + one branch per group | 114.47 | **+20.5** |
| + one branch per 8 lanes | 129.32 | **+35.4** |

The arithmetic: the commit costs 8.28 cyc/grp, and a branch could skip it 27%
of the time, so the ceiling is **2.2 cycles saved**. Against that, one branch
per group at ~27% taken is essentially unpredictable, and it costs 20.5.
There is no frequency at which this pays -- it is the misprediction, not the
work. (Same verdict on AVX-512, where the `RC_SCATTER_SKIP=9` probe puts the whole
scatter at 1.96 cyc/grp and the branch cost 28%.)

## 5. Would prefetches help? NO

Working set per 64 KB block: `pbit` is `word[BLKSIZE*8]` = **1 MB**, written
straight through by the model pass and read straight through by the sweep;
`rcio.tmpbuf` is 16 rows x ~61660 bytes = **986 KB**, with the 16 lane cursors
~61660 bytes apart, i.e. 16 distinct 4 KB pages touched every group; `FSM` +
`pp` = 192 KB; two 64 KB coroutine buffers. L2 is 1 MB.

So there is real traffic. But both hot loops are issue/port-saturated rather
than miss-stalled, every stream is sequential within a page and covered by the
hardware prefetcher, and a `prefetcht0` is another uop against the binding
constraint. Measured, prefetching `pbit` 128 words ahead in the sweep:
**-0.82 cyc/grp, inside run-to-run noise** (the base itself moves ~1.5 between
runs).

The interesting negative result is **chunking**. The model pass writes all
1 MB of `pbit` before the sweep reads any of it, so nothing is warm when the
sweep starts; interleaving them over sub-blocks -- `model_pass(chunk)`,
`sweep(chunk)` -- should fix that, and the bit-to-lane mapping survives as
long as the chunk is a multiple of 2 bytes. It is **10.3 cyc/grp slower**
(and 6.6 of that is the chunking itself, not the restructuring, which costs
1.5):

| chunk | MB/s |
|---|---|
| whole block | 62.29 |
| 8192 | 57.95 |
| 4096 | 57.58 |
| 2048 | 58.05 |
| 1024 | 58.47 |
| 512 | 57.32 |

The mechanism: unchunked, the model pass runs alone against `FSM`+`pp`
(192 KB) and the sweep runs alone against `tmpbuf` (986 KB), and each keeps
its own working set in L2. Interleaved, `tmpbuf` alone nearly fills L2 and
evicts `FSM`/`pp` on every switch. The pbit locality gained is smaller than
the table locality lost.

## 6. Are there avoidable branches or bounds checks? NO -- it is already clean

Every hot inner loop contains **exactly one conditional branch: its own back
edge.**

    loop_modelpass  branches=1   (jne)
    loop_sweep      branches=1   (jb)
    loop_third      branches=1   (jne)

`do_process` has 43 branches in total, all of them in per-block setup and
teardown, not per group. Specifically:

- `RC_IO_CHECK` (rc_io.inc) is 0 and the vector path does not go through
  `RC_IO` at all -- it addresses `tmpbase`/`tmpptr` directly. There is no
  bounds check to remove.
- `get0()`/`put0()` in `coro3_pin.inc` are unchecked by construction
  (`return *ptr++`); `chkinp()` runs once per block, `chkout()` once per
  output chunk inside `rc_Write`.
- the substream-overflow test (`novf`) and the carry test (`ncarry`) are once
  per block, after the sweep.
- every data-dependent test *inside* the coder is already branchless. The
  `(sh==0) ? 0u : (lowl >> (32-sh))` guard in `ShiftLow` compiles to
  `vblendvps`/`vpand`, not a branch; `range = _b ? range : rpre` likewise.
  Zero `cmov` and zero data-dependent `j*` in the sweep.

The one place a branch could be added is the skip in section 4, and it costs
20 cycles to save 2.

## 7. Where it stalls

**The model pass does not stall.** It is at the 4-wide rename ceiling with the
store port at 62% and the load ports at 42%. The only lever is fewer uops.

**The base sweep does not stall either.** It is at 96% of rename and 99% of
the p0/p1/p5 ceiling simultaneously. Again: fewer uops, which is exactly what
staging does.

**The staged sweep has one real stall, and it is the dominant part of its
remaining cost.** The commit reads back `stad[]`/`stcl[]` with 4-byte scalar
loads, which the sweep just wrote with 32-byte vector stores. Skylake will not
forward a 32-byte store to a narrower load, so each load waits for the store
to reach L1:

    staged, committed        93.97 cyc/grp
    staged, cold arrays      87.78          <- same 16 stores, no forwarding
    staged, no commit at all  85.70

**The 16 stores themselves cost 2.08 cycles. The forwarding stall costs
6.20** -- 75% of the commit.

Two attempts to remove it, both worse:

- **Park the group and commit it one iteration later**, so the loads are a
  full group away from the stores. Copying the staged arrays aside costs the
  same stall (a 32-byte load out of a 32-byte store does not reliably forward
  either): **+13.95 cyc/grp** with an explicit `_mm256` copy, and a plain
  element-wise copy is no better.
- **`-mprefer-vector-width=128`**, so the staging stores are 16 bytes and
  forwarding to a 4-byte load is supported: the arithmetic then runs 4 lanes
  at a time and the loop loses far more than it gains -- 51.70 MB/s.

So ~6 cyc/grp (6% of encode) sits behind a store-forwarding rule, and the two
obvious ways out both cost more than the stall. Getting it would need the
staged values to reach memory as narrow stores without the arithmetic getting
narrower -- which SLP will not do from this source shape.

## 8. Everything measured, in one table

20 MB of enwik8, best of 6 iterations x 6 runs, 3.235 GHz.

| variant | MB/s | cyc/grp | vs base |
|---|---|---|---|
| inline store (`RC_SCATTER=0`) | 63.18 | 102.41 | +0.00 |
| **staged store (now default)** | **68.85** | **93.97** | **-8.43** |
| inline + `-fno-pie` | 64.85 | 99.77 | -2.64 |
| staged + `-fno-pie` | 69.48 | 93.12 | -9.29 |
| prefetch `pbit` +128 | 63.69 | 101.59 | -0.82 (noise) |
| `pp` fused into `FSM` | 61.14 | 105.82 | +3.42 |
| chunked model/sweep, 2048 | 57.42 | 112.68 | +10.27 |
| commit deferred one group | 59.95 | 107.92 | +5.52 |
| staged + skip branch | 56.52 | 114.47 | +12.07 |
| staged + 2 skip branches | 50.03 | 129.32 | +26.92 |
| *probe* model pass only | 117.51 | 55.06 | -47.35 |
| *probe* inline, no store | 73.78 | 87.69 | -14.71 |
| *probe* staged, no commit | 75.50 | 85.70 | -16.71 |
| *probe* staged, cold arrays | 73.71 | 87.78 | -14.63 |

## 9. What is left

After staging, encode is 93.97 cyc/grp: **55.06 model pass (59%)**, 38.91
sweep (41%). Of the sweep, 8.28 is the commit (6.20 of it the forwarding
stall) and ~30 is the rangecoder arithmetic, which sits at the rename and ALU
ceilings.

In rough order of what is actually available:

1. `-fno-pie` -- 2.6 cyc/grp, one flag, no risk. A packaging call.
2. The forwarding stall -- 6.2 cyc/grp, but no cheap way in from this source
   shape (section 7).
3. The model pass is 59% of encode and pinned at the 4-wide rename ceiling.
   Only a smaller instruction stream moves it; the layout experiments here
   (fusing the tables, prefetching, chunking) all lost. That is where the next
   real win would have to come from, and it would have to be an algorithmic
   change to the counter update, not a scheduling one.

## Reproducing

    cd 016-no_clsize
    ARCH=skylake ./build.sh                       # staged store, the default now
    ARCH=skylake ./build.sh -DRC_SCATTER=0        # the inline store
    ARCH=skylake OPT='-O3 -fno-pie -no-pie' ./build.sh
    ARCH=skylake ./build.sh -DRC_SCATTER_SKIP=3   # branch past all-zero groups
    ARCH=skylake ./t.sh                           # roundtrip + scalar-reference check

    ./coder c ../enwik8 /dev/null ../FSM0.txt 6   # the timing loop used here
