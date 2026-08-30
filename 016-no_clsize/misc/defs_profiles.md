# The defs profiles

After the cleanup, everything both profiles agreed on is the code's only state
and is gone from the -D space. What is left is the five knobs the two targets
actually disagree about:

```
set defs_avx2=  -DRC_SWEEP_NEGIDX=1 -DRC_FOLD_RPRE=1 -DRC_SCATTER_SKIP=0 ^
                -DRC_ENC_NSEL=0 -DRC_ENC_RENORM=0
set defs_avx512=-DRC_SWEEP_NEGIDX=0 -DRC_FOLD_RPRE=0 -DRC_SCATTER_SKIP=1 ^
                -DRC_ENC_NSEL=1 -DRC_ENC_RENORM=2
```

Baked in, no longer settable: `RC_DEC_WAVE=2`, `RC_LOAD32=1`, `RC_DEC_ALIGN=0`,
`RC_FF_LANES=8`, `RC_DEC_PUTW=1`, `RC_DEC_COLD=1`, `RC_SHIFT_SAT=0`,
`RC_DEC_RENORM=8`, `RC_DEC_PREFETCH=0`. The stream is byte-identical to what
the old profiles produced, and so is the speed.

Two constraints the baking made hard, both now `#error`ed rather than silent:

- **`RC_RCNUM` must be a multiple of 16.** The paired 16-bit output store (the
  old `RC_DEC_PUTW`) needs an even number of output bytes per group. `RCNUM=8`
  used to be rejected by that knob's own guard; without the guard it built and
  roundtripped to garbage, which is how the matrix caught it.
- **`RC_LOWBYTES - RC_CODBYTES >= 4`.** The one-branch refill reads a 32-bit
  window from the cursor, so the payload needs four bytes above it. At the
  default `RC_CODBYTES=4` that pins `RC_LOWBYTES` to 8; `RC_CODBYTES=3` also
  admits 7. `RC_LOWBYTES` 4..6 are no longer buildable.

## A. Add these — measured wins on the profile

| add | effect | |
|---|---|---|
| `-DRC_DEC_RENORM=8` | **+4.4% decode** | 1.864 → 1.786 s — **now baked in** |
| `-DRC_ENC_NSEL=1` | **+7.1% encode** | 92.05 → 98.61 MB/s — in `defs_avx512` |
| `-DRC_RCNUM=32` | **+9.0% decode** | 1.864 → 1.710 s |

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
cleanly. `RC_FF_LANES` is now fixed at 8 rather than following RCNUM, which is
the setting the +9.0% was measured with. This row was measured against its own
stream rather than round-robin against the baseline, so it is the weakest of
the three.

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

These were measured and lost, and the cleanup removed every one of them from
the -D space except `RC_CODBYTES`. Recorded so they are not rebuilt:

| | |
|---|---|
| `RC_DEC_UNROLL` 2 / 4 / 8 | −1.1 to −1.7% (1.893, 1.893, 1.903 s) |
| `RC_EAGER_CTY=1` | −1.1% (1.892 s) |
| `RC_DEC_RENORM=13` | −18% (2.215 s) — best *branchless* shape, still loses |
| `RC_DEC_SPLIT` 1 / 2 | −33 to −77%, see dec_vectorize.md |
| `RC_DEC_PREFETCH` 1..4 | −1.2 to −3.3%, every hint and distance |
| `-DRC_CODBYTES=3` | no speed, and costs 1.479% of ratio (**still a knob**) |
| `-DRC_ENC_RENORM=1/2` | inside noise (**still a knob** — the profiles differ) |

`RC_DEC_UNROLL` is the interesting negative. Section 7 of `dec_vectorize.md`
establishes this decoder is front-end bound, and the full unroll is what makes
the loop 1756 bytes — so shrinking it *should* have helped. It does not, at any
of 2, 4 or 8. Whatever the front end is short of, it is not this loop's size.
