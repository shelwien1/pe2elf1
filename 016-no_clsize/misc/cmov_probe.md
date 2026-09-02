# CMOV with a memory source: does it read, and does it wait?

`misc/cmov_probe.cpp`.  Two questions about

```c
int sel(const int* __restrict p, int q, int cond) {
    int t = *p;                                   // load must be unconditional
    return __builtin_unpredictable(cond) ? t : q;
}
```

which clang compiles to exactly the instruction the comment asks for:

```
    movl    %esi, %eax
    testl   %edx, %edx
    cmovnel (%rdi), %eax
    retq
```

1. **Does `cmov` read memory when the condition is false?**  If it does, the
   line ends up cached, and a timed read afterwards can see that.
2. **Does its result depend on the source it did not select?**  If it does,
   `sel(p, 0, false) / x` cannot divide until `p`'s line has arrived, even
   though the answer was `0` all along.

Four forms of `int f(const int* p, int q, int cond)` are timed: `sel` as the
compiler emits it, and three hand-assembled stubs in executable memory so the
instruction under test is not up to the compiler -- `cmov_mem` (`test; mov
eax,q; cmovne eax,[p]; ret`), `cmov_reg` (an explicit load, then the
register form), and `branch` (`test; jne; mov eax,q; ret; L: mov eax,[p];
ret`), which is the control: with `cond` false it never touches `p`.  1024
lines 4 KB apart in random order, so no prefetcher follows them and a cold
line is a page walk plus DRAM; Cascade Lake, TSC 2.80 GHz, min of 7.

## Q1: yes, it reads

Read latency of a freshly flushed line right after calling `f(line, 7, cond)`
on it (single-shot `rdtsc`, so ~30 ticks of the "warm" number is timing
overhead):

```
                       cond=false   cond=true
  none (cold read)        382.4
  sel (clang)              36.9        38.6
  cmov_mem                 36.9        38.3
  cmov_reg                 37.8        38.6
  branch                  374.1        36.8
```

Every `cmov` form leaves the line warm with the condition false; only the
branch leaves it cold.  This is the architecture, not an accident: the SDM
specifies that the memory operand of `CMOVcc` is read regardless of the
condition, which is why a `cmov` from an unmapped address faults even when it
would not have selected it.  The load uop is unconditional, and so is its
cache fill.

## Q2: yes, it waits

The naive test does not show it, and that is worth recording first.  The
literal `x = x / (f(line(i), 0, cond) + 1)` over independent lines:

```
                    cold,false   warm,false   cold,true   warm,true
  sel (clang)          32.9          9.6         35.8         9.6
  cmov_mem             34.1          9.5         34.1         9.6
  branch                9.4          9.4         34.5         9.4
```

Cold costs 24 ticks over warm, not the 250 a DRAM line costs, because the
addresses `line(i)` do not depend on `x`: the out-of-order engine issues the
loads for a dozen iterations ahead and memory-level parallelism overlaps
them.  Even so the branch form pays nothing at all with `cond` false, and
every `cmov` form pays the same whether the condition is true or false.

The dependent form is the one that answers the question.  Each line holds
the index of the next line, and so does a table `next[]`; the next address
comes out of `f`'s **result**:

```
    idx = f( line(idx), next[idx], cond )
```

With `cond` false the result is `q = next[idx]`, known before the call.  If
`cmov`'s output does not depend on the load it did not select, the chain runs
at call speed and the loads complete in the background.  If it does, every
step waits for its line:

```
                    cold,false   warm,false   cold,true   warm,true
  sel (clang)         258.5         67.9        249.3        67.8
  cmov_mem            253.2         68.6        281.8        69.0
  cmov_reg            265.8         69.0        268.9        69.2
  branch                5.3          6.1        248.3        67.1
```

`cmov` with the condition false is as slow as `cmov` with it true, and both
are as slow as the branch form *taking* the load: ~250 ticks a step cold,
~68 warm.  The branch with the condition false is 5 ticks.  The result of a
`cmov` depends on both of its sources, and a consumer of that result -- a
divide, an address, anything -- waits for the slower one.  `sel(p, 0, false)
/ x` waits for `p`.

## What follows from it

- **`cmov` is a select, not a conditional load.**  Its cost is the *maximum*
  of its inputs' arrival times, plus one.  It cannot be used to keep a load
  off the critical path "only when needed"; the load is always on the path.
  The eager-child-load idea (`RC_EAGER_CTY`) paid exactly this, and the
  if-tree won on text for the mirror-image reason: a *predicted branch* is
  the one construct that really does skip a load's latency.
- **The decoder's own `range = _b ? range : rpre` is the harmless case** --
  both sources are registers already in hand, and the select is one cycle.
- **The source-level intent is not portable.**  gcc compiles the same `sel`
  to a branch --

  ```
      movl    %esi, %eax
      testl   %edx, %edx
      je      .L2
      movl    (%rdi), %eax
  ```

  -- so under gcc the "unconditional" load is conditional: its `sel` row
  reads nothing with `cond` false (366 cold in Q1) and waits for nothing
  (5.0 in Q2a).  `__builtin_unpredictable` is clang's; gcc has no equivalent
  that forces `cmov`, and MSVC has neither.  If the `cmov` matters, the
  portable way to get it is the stub or inline assembly, not the ternary.

## ISPC: the vector analogue does mask

Would ISPC compile `cond ? *p : q` -- varying pointer, varying condition --
to a masked gather?  `misc/sel_gather.ispc`, ispc 1.22 / LLVM 17: yes, and it
lowers the ternary exactly like an `if`.  The IR's blocks are named
`select_eval_expr` / `select_done`, an any-lane test branches around the
gather when no lane needs it, and the gather's mask *is* the condition.
AVX-512:

```
    vptestmd   %zmm2, %zmm2, %k1          ; k1 = cond != 0
    kortestw   %k1, %k1 ; je ...          ; no lane true: skip the gather
    kmovq      %k1, %k2
    vpgatherdd (%rsi,%zmm3), %zmm1 {%k2}  ; masked by cond
    vpblendmd  %zmm1, %zmm0, %zmm2 {%k1}
```

AVX2 is the same shape with `vptest`/`je`, `vpgatherdd` under a
cond-derived mask, and `vblendvps`.  An explicit `if (cond) x = *p; else x =
q;` compiles to the identical code.

So the vector form has neither of the scalar `cmov`'s properties: a lane with
`cond` false issues no load -- no fault, no cache fill -- and the blend waits
on nothing it did not select.  It honours the SPMD rule that inactive
instances perform no memory operations.  What it costs is the gather, which
is the question `dec_vectorize.md` already answered for the per-bit path.
