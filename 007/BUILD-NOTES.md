# Build + roundtrip notes (Linux / clang)

## One archive, both builds

Maintaining two results was the bug.  The point of the two modes is that
`./mk.sh` measures and `./mk.sh release` ships *the thing that was measured*,
faster; a release that codes differently from the build `opt.pl` hill-climbed
is not a release of that model.  IDX-FORMAT.md sections 1 and 12 say so, and
the tree was failing it.

The entire divergence was one line — `paq8hp.hpp`'s ContextMap bit-history
decay — and it was undefined behaviour, not a parameter difference.  The
diagnosis is under "Why the two builds used to disagree" below; the short form
is that `CM_DECAY_BIAS` was seeded 165, which is *below* `CM_DECAY_FROM`'s 225,
so `(CM_DECAY_BIAS-ns)>>CM_DECAY_SHIFT` was negative for every state the guard
admits.  The shipping build folded the constants, proved the branch undefined
and **deleted the decay outright**; the tuning build could not prove it and ran
the shift with x86's count masked to five bits.

Two changes, and they are one fix:

* **`PSH()` on the shift, at the point of use.**  `CM_DECAY_BIAS` is a live
  nine-bit knob and `opt.pl` visits both ends of it, so the clamp is what keeps
  the expression defined *as it is swept* — section 5's rule, which this line
  had never obeyed.  It costs nothing in the shipping build: `ns` comes from a
  `U8` state table and the guard bounds it to [225,255], so clang proves the
  count lands in [20,24] and drops the clamp.  The same range deduction that
  used to delete the decay now discharges its bounds check.

* **`CM_DECAY_BIAS` reseeded 165 -> 421**, one bit away — `010100101` ->
  `110100101`.  421 is not a new schedule, it is the *existing* one written
  down: `(421-ns)>>3` equals `((165-ns)>>3)&31` for every reachable `ns`,
  because 256>>3 is exactly 32.  So the decay keeps the behaviour every
  `opt.pl` run actually measured, and now says it in an expression that is
  defined.

The result is one archive from both builds, and it is the tuning build's:

```
./mk.sh         && ./paq8hpc_idx c book1 -> 191984   b2d99f27581ffbc505e08e89a9d482c8
./mk.sh release && ./paq8hpc_idx c book1 -> 191984   b2d99f27581ffbc505e08e89a9d482c8
```

That is the direction that had to win.  The shipping build's old 192014 was the
output of a model with its bit-history decay silently missing — 30 bytes worse
on book1 and not the model anyone tuned.  UBSan is clean on the decay line, the
roundtrip is exact, and timing is unchanged (23.1s vs 24.9s over the same file;
the branch the release build had been deleting costs nothing measurable).

`IDX/export.!!!` carries the new seed too.  It had to: `import.pl` matches
`opt.pl`'s winners back by name, so a stale `010100101` there would have walked
the bug straight back into the `.idx` on the next fold-back.

## The models' context constants moved into IDX

Every arbitrary number in the four models' context blocks is now a declared
parameter.  One module per model — `paq8-W0` (WordModel), `paq8-S0`
(SparseModel), `paq8-R0` (RecordModel), `paq8-C0` (ContextModel) — because a
model is what a reader looks at one at a time and what `opt.pl`'s search space
is easiest to reason about a slice of.  `!MAP!` markers in the tuning binary go
from 59 to 241.

What was converted: the mask saying which bits of a shift register a context
keeps; the multiplier spreading a narrow context across a hash; the tag
separating two contexts that share a ContextMap; the byte a predicate compares
against; the shift moving a context into its own region of a table; the word
chain's nine multipliers and the four pairs that keep two-word contexts from
aliasing; the thirteen order-hash primes; the caps on `col`, `frstchar` and
`spafdo`.

### Index or Number

An **Index** where the packing is *tight* — where each field starts exactly
where the one below it ends, so `MakeIndex` reproduces the hand-written shifts
character for character while turning the field widths into the parameters.
`scm6.set((words&12)*16+(w4&12)*4+(b1>>4))` is the textbook case and became

```
Index Scm6
 s6_words: words, &00001100
 s6_w4:    w4,    &00001100
 s6_b1:    b1,    &11110000
```

which generates `((words>>2)&3)<<6 | ((w4>>2)&3)<<4 | ((b1>>4)&15)` — the same
value, with the three widths now movable.  Eleven contexts across the four
models are of this shape.

A **Number** applied with `&` everywhere else, and the reason is not stylistic:
a `&` mapping does not merely select bits, it *packs the survivors
contiguously*.  That is right when the result indexes a table and wrong when it
feeds `ContextMap::set`, which hashes — a repack rehashes and moves the
archive.  Same for a *loose* packing: `frstchar<<11|c` leaves three bits spare
above an eight-bit `c`, and an Index would close the gap.  There the shift is
the knob and stays a Number.  Getting this choice wrong is silent — it
compiles, it runs, and the output moves — which is why the rule is written down
in each `.idx` rather than left to be inferred.

### 32-bit masks are declared one byte lane at a time

`x4`, `c4` and the `h = w4<<6` window are byte-per-byte registers, so a mask
over one is four independent byte masks and is declared that way; `LANES4()`
reassembles it and the call site still reads as one mask.  A lane is the
register's own field, so `opt.pl` moves a byte of history rather than an
arbitrary bit.  It is also the only form available: `mapping::value`
accumulates a pattern into a **signed int**, so a 32-bit pattern overflows it —
IDX cannot spell a full-width mask at all.

### What was deliberately left alone

* **Character identities** — `'a'`, `'z'`, `'.'`, 32, 10, and the tests for 8,
  6, 127, 12.  Those say what a word character *is* and what a line ending
  *is*.  They are the alphabet, the same category as `P_BITS` in `paq8-G0.idx`.
* **`MIX_R3` and `MIX_R5`'s constants** in `ContextModel::p`.  Those are the
  *range* of a mixer selector, and the six ranges must sum exactly to
  `MIXER_ROWS`, a fixed array dimension of `Mixer::wx`.  Widen the `tt` mask by
  one bit and the sum overruns `wx` — the tuning build would catch it in
  `Mixer::set`, the shipping build would not, because `MIXER_ROWS` is a literal
  there.  Handing those to `opt.pl` would be handing it the shape of the mixer.

### A silent bug in idx2inc.pl, found by this

The generator stripped `/#.*$/` but not the whitespace the comment left behind,
and a pattern is whitespace-sensitive in the worst way: `mapping`/`masking`
read it a character at a time as `(c & 1)`, so a trailing space is a valid zero
bit.  `0!01000    # d<<8` therefore spelled **2048 instead of 8**, with no
diagnostic anywhere — which is why no `.idx` in the tree had ever carried a
comment after a pattern.  `import.pl` has always taken the leading whitespace
with the comment (`s/(\s*#.*)$//`); `idx2inc.pl` now does the same and also
drops any trailing whitespace.  Verified to leave `paq8-G0`'s and `paq8-S0`'s
generated headers byte-identical.

### Verification

Byte-identical, not equivalent — each model was converted and checked on its
own before the next:

| build | book1 | vs. pre-change |
|---|---|---|
| shipping (`./mk.sh release`) | 192014 | identical (`50ebbc27…`) |
| tuning (`./mk.sh`) | 191984 | identical (`b2d99f27…`) |

Both numbers are pre-fix: at the time of this change the two builds still
disagreed, for the reason under "One archive, both builds" above, and each was
checked against its own predecessor.  Since that fix there is one number,
191984.

Both roundtrip to the original md5.  `import.pl <each>.idx export.!!!` is a
no-op against all five sources, so `opt.pl`'s fold-back path still agrees with
what is declared.

## `_NUM`/`_DEN` pairs folded into single P_SCALE-unit multipliers

Every ratio that IDX spelled as two parameters is now one.  A ratio held as a
numerator and a denominator is not one knob: opt.pl can reach the same rate by
many routes (7/1, 14/2 and 28/4 are one point visited three times), so most of
the product space is duplicates, and a shared denominator is worse still — the
three `MIX_MUL_*` gains could not move independently because `MIX_MUL_DEN` sat
under all of them.

| was | is | ratio |
|---|---|---|
| `MIX_LR_NUM` 7 / `MIX_LR_DEN` 1 | `MIX_LR_MUL` 1792 | 7 |
| `MIX2_LR_NUM` 2 / `MIX2_LR_DEN` 4 | `MIX2_LR_MUL` 128 | 1/2 |
| `MIX_MUL_0` 4 / `MIX_MUL_DEN` 4 | `MIX_MUL_0` 256 | 1 |
| `MIX_MUL_1` 4 / `MIX_MUL_DEN` 4 | `MIX_MUL_1` 256 | 1 |
| `MIX_MUL_2` 7 / `MIX_MUL_DEN` 4 | `MIX_MUL_2` 448 | 7/4 |

The new values are in units of `P_SCALE/256` — the unit the fixed mixing
weights already use — so the constant is `NUM1` and the use site is
`NUM1/P_SCALE`, with `P_SCALE` serving as the fixed-point denominator.  It
cancels against the declaration unit, so every ratio above is exactly what the
pair evaluated to and none of them moves with `P_BITS`.

Consumer side:

* `p_scaled(v, mul)` — new two-argument overload, same shape as the three
  argument one (single division at the end, so the truncation still happens
  once) with the reference rescaling kept.  Used by `Mixer::update`/`update2`.
* `st_scaled(v, mul)` — the same without `P_UP`/`P_DN`, because `Mixer::mul`'s
  inputs are logistic-domain and follow `ST_SCALE` rather than `P_BITS`.  Both
  form the product at 64 bits: `P_SCALE*PMUL_MAX` is past `int` at every
  `P_BITS`, and the old pair form never got that large.
* `PMUL(x)` clamps a `P_SCALE`-unit ratio, replacing `PSMALL` on these five.
  `PSMALL`'s ceiling of 255 was for small numerators and would have truncated
  every one of the new values.
* The three-argument `p_scaled` stays — the models still use it for literal
  fitted coefficients (`3/64`, `1/16`, `7/64`), which are not knobs.

`IDX/export.!!!` was updated in the same commit and is not cosmetic: opt.pl's
accumulated winners are matched back onto the `.idx` **by name**, and
`MIX_MUL_0` still exists under that name.  Left alone, the next
`perl import.pl` would have folded the stale four-bit `"0100"` into a line that
now reads twelve bits of `P_SCALE/256`, quietly changing that gain from 1 to
1/4.  The five entries for names that no longer exist were dropped.
`import.pl paq8-G0.idx export.!!!` is now a no-op against the source, which is
the check that says the two agree.

Verified byte-identical, not just equivalent:

| build | book1 | vs. pre-change |
|---|---|---|
| shipping (`Const 1`, `USE_NEW 0`) | 192014 | identical (`50ebbc27…`) |
| tuning (`Const 0`, `USE_NEW 1`)   | 191984 | identical (`b2d99f27…`) |

Both numbers are pre-fix: at the time of this change the two builds still
disagreed, for the reason under "One archive, both builds" above, and each was
checked against its own predecessor.  Since that fix there is one number,
191984.

Both roundtrip.  The equality also holds off the reference scale, which is
where the widened arithmetic could have shown: rebuilding the pre-change tree
and this one at `-DPAQ_P_BITS=12`, `13` and `16` gives identical archives at
all three.

Cost, stated plainly: `opt.pl`'s search is proportional to total pattern bits,
and these five lines went from 27 bits to 60 — eight knobs to five, but each at
1/256 resolution over the same 0..16 range the pair form reached, instead of
the coarse and largely duplicate `NUM/DEN` grid.  That is the same trade the
`.idx` already makes for the fixed mixing weights.  `!MAP!` markers in the
tuning binary: 62 -> 59.  If the finer grid is not worth the search time on a
given corpus, section 10's `!` prefix freezes any of them individually.

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

## Why the two builds used to disagree (fixed; kept for the diagnosis)

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

See "One archive, both builds" at the top for how this was resolved.
