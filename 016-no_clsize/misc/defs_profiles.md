# What to add to the defs profiles

Measured on this box (clang 18, `-march=native`, AVX-512, 100 MB of enwik8)
with `defs_avx512` **verbatim** as the baseline, not against library defaults —
several of these interact with the profile and read differently on top of it.
Every row is a median with a byte-identical twin build in the rotation.

```
defs_avx512 = -DRC_DEC_WAVE=2 -DRC_LOAD32=1 -DRC_DEC_ALIGN=0 -DRC_FF_LANES=8
              -DRC_SWEEP_NEGIDX=0 -DRC_FOLD_RPRE=0 -DRC_DEC_PUTW=1
              -DRC_DEC_COLD=1 -DRC_SHIFT_SAT=0 -DRC_SCATTER_SKIP=1
```

## A. Add these — measured wins on the profile

| add | effect | |
|---|---|---|
| `-DRC_DEC_RENORM=8` | **+4.4% decode** | 1.864 → 1.786 s |
| `-DRC_ENC_NSEL=1` | **+7.1% encode** | 92.05 → 98.61 MB/s |
| `-DRC_RCNUM=32 -DRC_FF_LANES=16` | **+9.0% decode** | 1.864 → 1.710 s |

**`RC_DEC_RENORM=8` is your own one-branch shape** (`rc_renorm_1br.txt`), with
the rpre fold duplicated into both arms as you wrote it. It is the smallest
decode body of the ten shapes — 431 instructions per group against shape 0's
460 — and the best of them here. It is also the one number our two boxes
disagree on: your exploration log has it at 47.28 against shape 0's 48.21,
i.e. −1.9%. One build settles it, and it is the first thing to try.

**`RC_ENC_NSEL=1` reads much bigger on this profile than on defaults** — +7.1%
here against the +2.3% measured against library defaults, so it interacts with
something in the set. Encode only; decode is untouched.

**`RC_RCNUM=32` changes the stream format** — streams are not interchangeable
with RCNUM=16 builds, and a mismatched decoder segfaults rather than failing
cleanly. Note also that your profiles pin `RC_FF_LANES=8` while the default is
`RC_RCNUM`; at RCNUM=32 raising it to 16 is worth another 1.5% (1.735 → 1.710).
This row was measured against its own stream rather than round-robin against
the baseline, so it is the weakest of the three.

## B. Worth a build — box- or compiler-dependent, cannot be settled from here

- **`-DRC_CHUNK=`** (default 2048). The repo measured +2.6 to +4.2% encode flat
  across 256…8192 on skylake; never re-swept on clang 23. Decode is untouched —
  it has no model pass to split.
- **`-DRC_FUSE_PP_ENC=0`** as a control. The default auto-selects on
  `__clang_major__ < 19`, so at clang 23 you get 1, and the note says clang 23
  measured +3.98% for it. But the cut sits between the only two versions ever
  measured and 19…22 are untested — worth one control build to confirm you are
  on the right side.
- **`-DRC_SCATTER_W=16 -mprefer-vector-width=512`** (AVX-512 only). Measured
  72.1 MB/s against the 8-lane default's 66.9 on cascadelake, i.e. still
  behind, but that is one machine.
- **`-DRC_LOWBYTES=4/5/6`, `-DRC_VECSIZE`, `-DRC_BLKSIZE`** — never swept in
  this round at all.

For **`defs_avx2`**: `RC_SCATTER_W` auto-selects 0 without AVX-512, which is the
path the repo measured as the good one there, and `RC_SCATTER_SKIP=0` is
already right (the branch loses on AVX2 too — 66.9 → 60.0). `RC_DEC_RENORM=8`
is still the first thing to try.

## C. Do not spend a build — measured losses here

| | |
|---|---|
| `-DRC_DEC_UNROLL=2 / 4 / 8` | −1.1 to −1.7% (1.893, 1.893, 1.903 s) |
| `-DRC_EAGER_CTY=1` | −1.1% (1.892 s) |
| `-DRC_DEC_RENORM=13` | −18% (2.215 s) — best *branchless* shape, still loses |
| `-DRC_DEC_SPLIT=1/2` | −33 to −77%, see dec_vectorize.md |
| `-DRC_DEC_PREFETCH=1..4` | −1.2 to −3.3%, every hint and distance |
| `-DRC_CODBYTES=3` | no speed, and costs 1.479% of ratio |
| `-DRC_ENC_RENORM=1/2` | inside noise |

`RC_DEC_UNROLL` is the interesting negative. Section 7 of `dec_vectorize.md`
establishes this decoder is front-end bound, and the full unroll is what makes
the loop 1756 bytes — so shrinking it *should* have helped. It does not, at any
of 2, 4 or 8. Whatever the front end is short of, it is not this loop's size.
