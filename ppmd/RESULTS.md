# Compact PPMd context tree: implementation and results

`ppmd.cpp` is the compacted coder. `ppmd_orig.cpp` is the version it replaces, and
`ppmd_tree_compaction.md` is the analysis this work follows.

* **Compressed output is byte-identical to the original.** The model, the probabilities and
  even the state order are unchanged. This holds on enwik8 and enwik9 and on all test files,
  for orders 1..200.
* **enwik9, order 12, full 1 GB window, no resets:** the tree takes 2871 MiB of RAM
  instead of 4125 MiB (−30.4%). Peak RSS of the process is 3865 MiB instead of 5224 MiB.
  Encode time is 2460 s, against 2056 s for the original (1.20x).
* Cold multi contexts are stored with 4-bit freqs (packed records, `DESIGN.md` §2.1). That
  saves 3.6% of the tree and costs ~1.5x time. With `PPMD_PACK=0` the tree is 2978 MiB
  (−27.8%) and encoding takes 1682 s, faster than the original.
* There is no 32-bit `SUCC` address-space limit any more. The original needs
  `1 + WinSize + arena/2 <= 2^32`, which caps the tree at 6.3 GB with the enwik9 window. Here,
  text pointers cover windows up to 4 GB and the tree size is bounded only by `MMAX`.

## Results

All runs are encode then decode with `ppmd c|d in out 12 MMAX`, on a Xeon @ 2.1 GHz (4 cores,
L3 260 MB). "tree" is the memory that holds the context tree: for the original it is
`alloc_size + waste_size`, for the new coder it is the committed (touched) page memory. The
live record bytes are given in parentheses. RSS is the peak resident set of the process,
window included.

| file | original: size / tree MiB / RSS MB / enc s | new: tree MiB (live) | new: RSS MB | new: enc / dec s |
|---|---|---|---|---|
| e8m (first 8 MiB of enwik8) | 1,937,536 / 77.5 / 217 / 17.7 | 56.0 (53.4) **−27.8%** | 69 | 8.1 / 8.6 |
| e32m (first 32 MiB) | 7,312,689 / 267.0 / 418 / 71.4 | 192.8 (183.8) **−27.8%** | 231 | 40.3 / 40.4 |
| enwik8 | 20,828,759 / 683.2 / 995 / 209 | 493.7 (470.3) **−27.7%** | 599 | 127.8 / 129.6 |
| enwik9 | 167,384,640 / 4125 / 5224 / 2056 | 2978 (2837) **−27.8%** | 3972 | 1682 / 1737 |

The table above is without packing (`PPMD_PACK=0`). With packing, the default:

| file | tree MiB (live) | vs original | RSS MB | enc / dec s |
|---|---|---|---|---|
| e8m | 53.6 (50.9) | **−30.8%** | 66 | 16.1 / – |
| enwik8 | 474.3 (450.3) | **−30.6%** | 580 | 218.7 / – |
| enwik9 | 2871 (2725) | **−30.4%** | 3865 | 2460 / 2395 |

On enwik9 Repack writes 259M packed records and 144M are unpacked again when they become
current. Each unpack leaves a dead copy, and the extra compactions it causes are where the
time goes. A recency filter (do not pack recently unpacked contexts) did not reduce the
unpacks measurably, so it was dropped. Compacting less often (`PPMD_DEADSH`) trades memory
back for time. On e8m: 6 (default) gives 53.6 MB in 16.2 s, 5 gives 54.5 MB in 13.7 s,
and 4 gives 55.1 MB in 12.3 s.

The compressed sizes of the new coder, with or without packing, are identical, byte for
byte, to the original column.
Page metadata (page table, far tables, root table) adds 0.6 MB, 2.2 MB, 5.6 MB and 34.5 MB
respectively; it is included in RSS. The original decodes at the same speed it encodes
(e8m: 17.7 s). The original's enwik9 run shared the machine with other jobs part of the time.
The e8m, e32m and enwik8 times, and the new coder's enwik9 encode, were measured with the
machine otherwise idle.

### Baseline RSS with lazy arena init

The original commits memory it has not used yet. `buildFreeChain` writes a link into every
64 KB block of the arena, which is arena/16 of RSS before anything is stored. `FormatBlock`
pre-links every slot of a new block. `ppmd_orig_lazy.cpp` is the original with both made
lazy: blocks come from a bump pointer, and slots from a per-block `fresh` counter. Its output
is still byte-identical, also with resets. Its RSS is the fairer baseline for the RSS column:

| file | original RSS | lazy original RSS | new RSS |
|---|---|---|---|
| e8m (MMAX 2048) | 216 MB | 89 MB | 69 MB |
| e32m (MMAX 2048) | 418 MB | 303 MB | 231 MB |
| enwik9 (MMAX 6284) | 5224 MiB | ≈4830 MiB (estimated: −arena/16) | 3972 MiB (−18%) |

`waste_size` (block headers plus block tails) is 0.04% on enwik9, so changing the block size
would gain nothing at this scale. It would only help small files, where up to 256 partly
used blocks are a fixed cost, and with lazy formatting their unused part is no longer
committed either.

The design of the new tree is described in `DESIGN.md`.

## Layout (what was implemented from the analysis)

Every state is one 32-bit word, `[sym:8][tf:8][succ:16]`. This is the unified 4-byte state of
§2.5, realised exactly for any window up to 4 GB.

* **Multi context** (NU >= 2): `[EscFreq:7|rescaled:1][NumStats:8]` followed by NU states, so
  2+4·NU bytes (was 2+6·NU). `tf = T<<7 | (freq-1)`. A multi-context freq is at most 128, so
  freq-1 fits 7 bits, and T says whether `succ` is a context ref (1) or a text pointer (0).
* **Binary context**: one state, 4 bytes (was 6). `tf` is the full 8-bit freq, because binary
  freqs reach 196+, so there is no room for T. Instead the successor type is encoded in the
  *record address*: binaries with a context successor live at offsets ≡0 mod 4, binaries with
  a text successor at ≡2 mod 4. Materialising a binary's successor moves the 4-byte record to
  the other parity. This keeps the model exact: no freq clamp (§2.5's constraint).
* **Coarse text pointers, 16 bits** (§8 taken to k = ceil(log2 W) − 16). The coder stores
  `P >> k`, with k = 14 for enwik9 and k = 11 for enwik8. The exact P is recovered only at
  materialisation (`CreateSuccessors`). It is the first position in that 2^k block where the
  context string + symbol, which is the last order+1 bytes of history, ends. P is the end of
  the first occurrence of that pattern, so the recovery is exact. Recovered P matched the
  stored P in every call measured, and every output is byte-identical. The lower-order
  sibling test compares "both lazy" instead of P (§8). The scan uses AVX-512BW or AVX2
  (first+last byte filter, verify the middle) and is prefetched at `FoundState` time.
* **Paged 16-bit context successors** (§2.2). The tree lives in 64 KB pages, which are
  virtual address ranges committed on first touch. A context ref is 16 bits:
  `offset | M` (M = multi) for a child in the same page, or a value below 4096 that indexes
  the page's *far table* → global root id → root entry `{page, ref}`. The root entry is the
  parent slot of a page root, so a root can move inside its page or to another page without
  touching its parent. Only 0.3% of context edges are far.
  * Children are allocated in their parent's page. There is a per-page allocator with exact
    free lists and carving.
  * **Maintenance happens only at the step boundary.** `EnsureHeadroom` guarantees that every
    page holding a current context can absorb the step's worst case. Otherwise it *compacts*
    the page (copying, in place) or *splits* it: a set of sibling subtrees, about 3/16 of the
    page, moves to a receiver page. At that point the only transient references are
    `SuffCache[0..order]` and their parent slots, so only those need remapping (§1.3 single
    owner).
  * Compaction is a copy from the page roots, so subtrees that rescale disconnected are
    garbage-collected. The original leaks them until the next reset.
* **No 32-bit address space.** Text pointers and context refs are separate value spaces (§7.1),
  so the `1 + WinSize + arena/2 <= 2^32` limit is gone.
* **Speed.** The original spends ~60% of its time in `refresh_parent_cache`, which scans all
  256 `SuffCache` entries for every state of every modified context. The new coder tracks
  parent slots exactly through swaps, reallocations and the rescale permutation, at
  O(order) cost per change. That is why it is faster despite the scans and the page
  maintenance.

Overhead of the paging, relative to the live tree, is the same on enwik8 and enwik9: dead bytes
inside pages 1.3%, partially used last OS page of each page 3.7%, metadata 1.2%. On enwik9 the
tree uses 57,369 pages (50.6 KB live each) and 1.16M page roots. The largest far table measured
(e32m) holds 307 entries out of 4096; a split that would overflow one is restricted instead.

## Not implemented, and why

* **Binary-chain records (§2.1, §6.3).** About 70% of inner binaries link to another binary
  in the same page, so a chain record could save ~8% more. It needs a fourth record kind that
  can be addressed from inside a chain. A 16-bit ref here has no spare bit: 15 bits of offset
  plus M, with the binary succ type in the address parity. Freeing a bit means 32 KB pages
  (twice the partial-page waste) or 4-byte-aligned multi records (+2 bytes per multi
  context), which costs about what chains would save. §6.3's text-anchored variant with
  nibble freqs also changes the model.
* **Store-once text pointers (§6.2b).** The higher-order members of a duplicate group cannot
  be reached when the lowest member materialises, because there are no back links. Afterwards
  P can only be recovered via the first-follower path, and identifying that follower needs
  text[P] itself. Keeping it exact needs per-context extra state that costs what it saves.
* **Entropy-coded stats (§6.4), freq quantisation (§3.3) of hot contexts, delayed materialisation (§6.3),
  suffix-automaton PPM (§3.1).** These either change the model, and so the compressed size,
  or pay a codec cost on every update for the ~10% the analysis estimates.
* **Page-level compression of cold pages.** Measured on e32m: 99.9% of page accesses come
  within 16K steps of the previous one, so pages are never cold enough.

## Reproducing

```
g++ -O2 -o ppmd ppmd.cpp
./ppmd c enwik9 enwik9.ppm 12 8000      # order 12, page budget 8000 MB (virtual)
./ppmd d enwik9.ppm enwik9.out 12 8000
./test.sh 12 4000 -- file...            # original vs new: identical output + roundtrip
```

`MMAX` is the virtual page budget: the number of 64 KB pages times 64 KB. The memory actually
committed is the "tree_touched" figure, and the coder resets (or replays with `reset_perc`)
when it runs out of pages. Under a memory limit the new coder resets less often, so it
compresses better. On e8m at order 12:

| MMAX | original: bytes (resets) | new: bytes (resets) |
|---|---|---|
| 16 | 2,148,996 (7) | 2,096,170 (4) |
| 24 | 2,091,203 (4) | 2,053,672 (3) |
| 40 | 2,031,805 (2) | 2,001,008 (1) |
| 64 | 1,992,189 (1) | 1,962,162 (1) |

Without resets the outputs are identical.

Environment knobs (they change speed and memory only, never the output; encoder and decoder
may use different values): `PPMD_PACK=0|1` (packed cold records), `PPMD_PDEAD`,
`PPMD_SCAN=0|1|2` (scalar / AVX2 / AVX-512 scan), `PPMD_DEADSH`
(compaction threshold: dead > live>>n), `PPMD_SPLIT` (fraction moved per split, n/16),
`PPMD_MARGIN`, `PPMD_RECVGAP`.

Debug knobs: `PPMD_CHECK=n` walks the whole tree every n steps and verifies every invariant
(refs, parities, far tables, root lists, `SuffCache` reachability, parent slots).
`PPMD_STATS=1`, `PPMD_CENSUS=1` and `PPMD_ALLOCSTATS=1` print maintenance, record and allocator
statistics. `PPMD_CZK=k` forces a larger scan block. It normally leaves the output unchanged,
but encoder and decoder must then use the same value.
