# Build + roundtrip notes (Linux / clang)

## What was run

`MOD/` arrived empty, so it was regenerated first.  IDX-FORMAT.md section 12
("a generated `MOD/` is a build INPUT, not an artefact") and `mk.sh` both say
the committed copy is the *shipping* one, so that is what is checked in here:

```sh
sed 's/^Const 0/Const 1/' IDX/paq8-G0.idx > IDX/paq8-G0-const.idx
cp IDX/paq8-G0.inc IDX/paq8-G0-const.inc
( cd IDX && perl idx2inc.pl paq8-G0-const.idx 0 )
mv -f IDX/paq8-G0-const_h.inc MOD/paq8-G0_h.inc
mv -f IDX/paq8-G0-const_p.inc MOD/paq8-G0_p.inc
rm -f IDX/paq8-G0-const.idx IDX/paq8-G0-const.inc
```

Build (clang 18.1.3, x86-64, `gc.bat`'s flags minus the MSVC/mingw-specific
ones):

```sh
clang++ -std=c++23 -O3 -march=native -DNDEBUG \
        -fomit-frame-pointer -fno-stack-protector -fstrict-aliasing \
        -ffast-math -fno-rtti -fno-exceptions -Wno-format \
        paq8hpc.cpp -o paq8hpc
```

Clean — no diagnostics.

`paq8hp.hpp` line 17 sets `FIXED_LEVEL 7`, so the dispatcher holds one
instantiation and the level argument must be 7; the default of 11 in `main`'s
usage string is rejected by `Dispatch` (`level 11 not supported (7..7)`).

## Roundtrip

```
./paq8hpc c book1 book1.hpc 7      768771 -> 192014   ~27 s
./paq8hpc d book1.hpc book1.unp    192014 -> 768771   ~27 s
md5  0a0fdbaf0589c9713bde9120cbb20199  book1
md5  0a0fdbaf0589c9713bde9120cbb20199  book1.unp
```

Roundtrip is exact.

## The tuning and shipping builds do NOT agree

IDX-FORMAT.md makes this the contract (section 1, section 12: "Both builds must
produce the same stream").  They do not:

| build | `idx2inc.pl` args | book1 |
|---|---|---|
| shipping (`Const 1`, `USE_NEW 0`) | `paq8-G0-const.idx 0` | 192014 |
| tuning (`Const 0`, `USE_NEW 1`)   | `paq8-G0.idx 1`       | 191984 |

Both roundtrip correctly; they just code differently.  Bisected as follows.

* Not the layout.  A hybrid build (`Const 1` + `USE_NEW 1` — folded values, but
  the tuning build's `APM_CELLS_MAX` grid and reference-typed `IDXP`
  parameters) is byte-identical to the shipping build.  The APM comment at
  paq8hp.hpp:1483 is right: the two layouts code identically.
* Not the mask objects.  Replacing the four `masking` objects with the folded
  shifts, keeping the `pdesc` parameter objects, still reproduces the *tuning*
  output.
* Not the values.  All 66 `G0_*` constants plus the four `_Volume`s were dumped
  from both builds at `main()` and are pairwise identical.

The cause is `paq8hp.hpp:1848`, the ContextMap bit-history decay:

```c
if( ns>=CM_DECAY_FROM&&(rnd()<<((CM_DECAY_BIAS-ns)>>CM_DECAY_SHIFT)) )
  ns -= CM_DECAY_STEP;
```

With the seeded values `CM_DECAY_FROM=225`, `CM_DECAY_BIAS=165`,
`CM_DECAY_SHIFT=3`, the guard restricts `ns` to `[225,255]`, so
`(165-ns)>>3` is **always negative** — between -8 and -12.  A negative shift
exponent is undefined behaviour; gcc's UBSan says so on the first coded byte:

```
paq8hp.hpp:1848:36: runtime error: shift exponent -8 is negative
```

The two builds then diverge because only one of them can see it:

* **Shipping build** — `CM_DECAY_FROM`/`CM_DECAY_BIAS` are constant
  expressions, so clang range-deduces `ns` from the `U8` state table, proves
  the shift is always negative, and deletes the branch as unreachable.  Verified
  directly: replacing the condition with `if(0)` produces a byte-identical
  archive.  **The bit-history decay is silently dead in every shipping build.**
* **Tuning build** — the same two are runtime reads from `mapping` objects, so
  nothing can be proven and the `shl` is emitted.  x86 masks the count to 5
  bits, so the decay does run, at an effective shift of `(165-ns)>>3 & 31`,
  i.e. 20..24.

So the contract failure is not a build-system problem; it is one line of
undefined behaviour that the optimizer is entitled to resolve differently
depending on how much it knows.

### Note on the seed

`CM_DECAY_BIAS`'s pattern is `0!010100101` — nine bits spelling 165.  Nine bits
is exactly what a value in `[256,511]` needs and one more than 165 needs, and
`pclamp(...,0,511)` allows that range; the stock paq8 spelling of this line uses
453 (`111000101`), for which `(453-ns)>>3` is 24..28 over the same `ns` range —
well-defined, and matching the comment at paq8hp.hpp:410 that the decay grows
more frequent closer to saturation.  That is suggestive of a transcription slip
rather than proof of one, and the reference `paq8hpc.exe` in this tree carries
165 too (it is a tuning build — 62 `!MAP!` markers), so the seed has been live
for a while.

Not changed here: reseeding it moves the model's output, which is a modelling
decision, not a build fix.  Either fix closes the UB —

* reseed `CM_DECAY_BIAS` to `111000101` in `IDX/paq8-G0.idx`, or
* clamp the shift at the point of use, per IDX-FORMAT.md section 5's rule that
  a pattern is a search space and the consumer owns the range check —
  `pclamp((CM_DECAY_BIAS-ns)>>CM_DECAY_SHIFT, 0, 31)`.

The second is needed regardless of the first: `CM_DECAY_BIAS` is a live knob
and `opt.pl` will visit both ends of a nine-bit pattern, so any value below
`CM_DECAY_FROM` re-enters the same UB.
