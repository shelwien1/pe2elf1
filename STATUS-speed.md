# xadpcm speed work — status log

Implementing `xadpcm_speed_v4.md`. Rule for this pass: **take everything that is
bit-exact; take a ratio trade only if it costs a few bytes at most.** That makes
Tier 0 and Tier 1 in scope wholesale, and puts most of Tier 2 out of scope by
construction — §3.1/§3.2/§3.3 each cost kilobytes, not bytes. Each is measured
and reported below rather than assumed.

## Method

* **Correctness:** `./md5s.sh` — md5 of the compressed output over 24
  (file, mode) combinations: both real files × {default, `-s`, `-ss`}, all 14
  synthetic wavs, their concatenation × {default, `-ss`}, and a two-input
  archive × {default, `-ss`}. Every Tier 0/1 commit must leave all 24 md5s
  untouched. `./verify.sh` additionally round-trips everything.
* **Speed:** `./bench.sh [binary] [reps]` — `perf` is not available in this
  container, so it reports **min-of-N user CPU seconds** (N = 5). Minimum, not
  mean: the noise here is additive (shared box), so the fastest run is the best
  estimate of the true cost. Spread across runs is under 2%. Four measurements:
  encode/decode × MS (`wavs2`, 2.5 MB) / IMA (`Player_Death_Music_ima.wav`,
  600 KB); `TOTAL` is their sum and is what the table below tracks.
* Builds are `./mk.sh release` (Const) unless stated. Baseline flags are the
  tree's own: `-O2 -fstrict-aliasing -fomit-frame-pointer -fno-exceptions
  -fno-rtti`, no `-march`.
* Machine: Intel Xeon @ 2.80 GHz, 4 cores, AVX2 + AVX-512F available.

## Results

| # | item | TOTAL (s) | vs base | md5 |
|---|---|---|---|---|
| — | baseline (rev after the refactor) | 39.067 | 1.00× | — |
| 1 | §1.5 clz + §1.9 hoisting + §1.11 stats gate | 36.948 | 1.06× | identical |

---

## Log

### 1 — §1.5, §1.9, §1.11 (the "afternoon" group)

**§1.5 bit-scan loops → clz.** Three loops, ~8 calls per sample-channel:
`Pred::adapt` and `Pred::adaptx` each ran `for(u64 e = en+n*fl; e>1; e>>=1) s++`
(up to 64 iterations), and `ms_dlog` ran `for(u32 x=d; x>>=1;) e++`. Replaced
with `63-__builtin_clzll(x|1)` and `31-__builtin_clz(d)`. The `|1` guards
`clzll(0)`, which is undefined; the argument is never actually 0 here (`n` and
`fl` are both `pclamp`ed to ≥1, and `adaptx` only runs under `if(n4)`), but the
guard is free and survives a widened clamp. `ms_dlog`'s `d` is floored at 16 by
its own first line, so no guard is needed there.

**§1.9 per-symbol hoisting.** Three changes in `code_symbol`/`mbit`:

* The 24 `cs[n++] = row+nd` stack stores were rebuilt on every bit — 88 stores
  per 4-bit symbol that differ only by `nd`. `cs[]` is now built once per symbol
  as row *bases* and `mbit` takes `nd` and indexes `cs[i][nd]`.
* `tMS.row(srow)` / `tML.row(lrow)` genuinely change per bit (the row depends on
  the expected bit and on whether the match is still alive), but only three rows
  per model are reachable inside one symbol. Those three are resolved once and
  selected in the loop.
* `if(sl)` in `mbit` was dead — one call site, always passing `rQs+nd`. Deleted.

**§1.11 compile out the accounting.** `stat[scat] += CLT[pf][b]` ran on every
coded bit and kept the 16 KB `CLT` table hot in L1 for the benefit of `-v`
alone. Now behind `XAD_STATS` (default 0), which also drops `CLT` itself and its
`log2()` initialisation. `-v` without it reports the byte totals it still has
and says to rebuild with `-DXAD_STATS=1` for the per-stream split.

Result: **39.067 → 36.948 s, 1.06×**, all 24 md5s identical.
