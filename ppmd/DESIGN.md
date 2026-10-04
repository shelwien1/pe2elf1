# PPMd compact context tree: design

This describes the context-tree storage of `ppmd.cpp`. The model itself (SEE, coding, update,
rescale, inheritance) is the one in `ppmd_orig.cpp`, statement for statement. Only the
storage changed, so compressed output is byte-identical to the original.

Measured results are in `RESULTS.md`. The analysis this design follows is
`ppmd_tree_compaction.md`.

## 1. What has to be stored

The tree is the depth-D suffix trie of the substrings that occurred at least twice
(§1.3 of the analysis). Every context except the root has exactly one owner: the
successor field of one state in its parent. There are no suffix pointers.

* **Per context:** NumStats, EscFreq (7 bits) and a rescaled flag. The last two exist only
  for multi contexts.
* **Per state:** symbol, freq, successor. The successor is null (order-0 only), a text
  pointer (the context is not materialised yet) or a context.

Everything the coder holds besides the tree is transient and rebuilt every symbol:
`SuffCache[0..order]` (the current context at each order), `StateCache`, and the parent
slot of each `SuffCache` entry. This is what makes relocation cheap: at a step boundary only
`SuffCache[]` and `parentSlot[]` can point into the tree.

## 2. Records

Every state is one 32-bit word.

```
State   [sym:8][tf:8][succ:16]

multi context   (NU >= 2 states)            2 + 4*NU bytes   (original: 2 + 6*NU)
   +0 [EscFreq:7 | rescaled:1]
   +1 [NumStats = NU-1]
   +2 State[0] ... State[NU-1]
      tf = T<<7 | (freq-1)       T=1: succ is a context ref, T=0: text pointer / null

binary context  (1 state)                   4 bytes          (original: 6)
   +0 State                       tf = freq (full 8 bits)
```

**Why multi freq fits 7 bits.** A stored multi-context freq is at most 124
(`MAX_FREQ`). It reaches 128 only transiently, before the rescale that immediately follows.

**Why binaries are different.** Binary freqs reach 196, and `UpdateLowerSuffix` can push
them to 255. There is no spare bit, and clamping would change the model. So a binary's
successor type is encoded in its **record address** instead:

* offset ≡ 0 (mod 4): the successor is a context (`K_BIN0`);
* offset ≡ 2 (mod 4): the successor is a text pointer (`K_BIN2`).

When `CreateSuccessors` materialises a binary's text successor, the 4-byte record moves to
a slot of the other parity. Its parent slot, its `SuffCache`/`StateCache` entries and
`FoundState` are updated on the spot.

**Null successor** (order-0 states of unseen symbols): T=0 with `succ == 0xFFFF`. Coarse
text values are always below 0xFFFF (§4).

A context handle (`Ctx`) is the record address with bit 0 = M (1 = multi). Records are
2-byte aligned, so the bit is free.

## 3. Pages and references

### 3.1 Pages

The tree lives in an arena of 64 KB pages. The arena is a single `mmap(MAP_NORESERVE)` sized
by `MMAX`, and pages are 64 KB-aligned, so a record's page base is its address masked.

Nothing is written ahead of use, so physical memory is committed only for the part of each
page that has actually been allocated (§6). The first 4 KB of every page (`RESV`) is never
allocated, so that OS page is never touched.

### 3.2 References (16 bits)

```
ref >= 4096 : near ref = page offset | M         child lives in the same page
ref <  4096 : far ref  = index into this page's far table
                 far table[i]  -> global root id g
                 GRoot[g]      =  {page, near ref inside that page}
```

* **Children are allocated in their parent's page.** Far refs exist only where a split moved
  a subtree out (0.3% of context edges).
* **The global root entry is the parent slot of a page root.** A root can move inside its
  page, or to another page, by updating its `GRoot` entry alone. The parent's far-table
  index never changes.
* Each page keeps the list of root ids living in it, which compaction needs.
* A page's far table holds at most 4096 entries. A split that would overflow it is
  restricted instead.

Resolving a successor: near is one add; far is far table → `GRoot` → page base + offset.
`child()` also returns the parent slot: `&state.succ` for a near child, `&GRoot[g].ref` for
a far one.

### 3.3 Per-page allocator

Each page has a bump pointer, a live-byte count and a high-water mark. Freed records go to
exact free lists. All metadata lives in a `PageMeta` array, never in the page itself:

| class | record |
|---|---|
| 0 / 1 | binary slot at ≡0 / ≡2 mod 4 |
| 2..47 | multi record with that many states |
| 48 | larger multi records (size kept in the free record) |

* **Larger-record fallback.** A request whose exact class is empty takes the smallest
  larger free record and carves it. The remainder stays a multi-sized free record, losing
  at most 2 bytes, so it can serve the next expansion. Expansion is the dominant churn: it
  frees NU states and needs NU+1.
* **Binary requests** carve one slot of the wanted parity out of a free multi record.
* **Bump allocation of binaries** pads 2 bytes when the parity is wrong. The pad is
  reclaimed by the next compaction.

## 4. Text pointers

A text successor stores only `P >> k`, the 2^k-byte window block holding P. k is the
smallest value with `WinSize >> k < 0xFFFF`: k=14 for enwik9, 11 for enwik8. Any window up
to 4 GB therefore fits in 16 bits.

**When P is needed.** Only in `CreateSuccessors`, at materialisation of the found state.
P is the end of the *first* occurrence of the found context's string plus its symbol, which
is exactly the last order+1 bytes of the current history. So the coder scans that block for
the first position where those bytes end, and that position is P. There is no earlier match
in the block by construction, so the recovery is exact. It matched the stored P in every
call measured.

* **The scan.** AVX-512BW, or AVX2 or scalar, chosen at run time. It keeps positions where
  both the first and the last byte of the pattern match, then verifies the middle with
  `memcmp`. It prefetches ahead.
* **Prefetch at `FoundState`.** The block's first lines are prefetched as soon as
  `FoundState` is known and lazy.
* **The lower-order sibling test** ("is the suffix state lazy with the same P?") becomes
  "is the suffix state lazy". The two always coincide (§6.2 of the analysis: "different P"
  never occurs).

The window is allocated with `MADV_HUGEPAGE` and 64 bytes of padding for the vector loads.

## 5. Model operations on the new storage

| original | new |
|---|---|
| `NumStats()` from the block header | `rec[1]` for multi, 0 for binary (M bit of the handle) |
| `STATE0*` + parallel `SUCC[]` | one `State*`; swapping states swaps 4-byte words |
| `ExpandContext` + `UpdateHigherOrder` | fused `ExpandAndAdd`: the binary's 8-bit freq is transformed (`2f-1` or 109) before it is stored as 7-bit freq-1 |
| `ShrinkContext` | same; shrinking to one state allocates a binary of the right parity |
| `rescale` | works on an unpacked copy, so transient zero freqs are representable; returns the permutation |
| `AllocContextNU(1)` in `CreateSuccessors` | binary allocated in the parent's page |
| `refresh_parent_cache` | exact slot tracking (below) |

**Parent-slot tracking.** `parentSlot[t]` points at the 16-bit field that references
`SuffCache[t]`. The original re-derived these after every change by comparing every child
of the modified context with all 256 `SuffCache` entries, which was ~60% of its run time.
The new code updates them exactly, at O(order) cost per change:

* `slotSwap` on state swaps (`processSymbol1`, `UpdateSuffixFreq`);
* `slotMove` when a record is reallocated (expand, shrink);
* `slotPermute` with the permutation produced by `rescale`.

Far children need no tracking, because their slot is the `GRoot` entry, which does not move.

## 6. Maintenance at the step boundary

All relocation happens in `EnsureHeadroom()`, at the start of `ProcessByte`. At that point
the only live transient references are `SuffCache[0..order]` and `parentSlot[1..order]`.

**Headroom guarantee.** During one step a current context can at most be expanded by one
state, shrunk by a rescale, receive one new binary child, and be moved once to the other
binary parity. So `need(c) = size(NU+1) + 12` bytes. Every page holding a current context
must have at least the sum of its contexts' needs as bump space. Nothing inside a step can
then fail to allocate.

* **Fast path.** A dense `pgSlack[]` array holds the bump space left before the next untouched
  OS page. If every current page has slack ≥ the total need, nothing more is checked.
* **Slow path, per page.** The page is acted on if it is too tight, or if it would touch a
  new OS page while holding dead bytes > live/64 (`PPMD_DEADSH`). Then:
  * if `live + need + 2 KB` still fits in the page, it is **compacted**;
  * otherwise it is **split**.

**`Repack(page, split)`** makes one breadth-first pass from the page's roots, through near
refs only, then one copy-and-patch pass.

* **Compaction.** The reachable records are laid out again from `RESV`, grouped as
  [multi][binary ≡0][binary ≡2] so parities need no padding, via a scratch page. Records that
  are no longer reachable disappear. That includes subtrees disconnected when rescale drops
  a state; the original leaks those. Far-table entries nobody references any more are freed,
  and their roots marked dead.
* **Split.** Subtree sizes come from the BFS list. The code descends along the heaviest
  child while that subtree is larger than 1.25 × target (target = 3/16 of the page,
  `PPMD_SPLIT`). It then takes that node's children, largest first, up to the cap.
  * The selected sibling subtrees move to a **receiver page**: the page that last received a
    split, while it has room and far-table space, otherwise a fresh page.
  * Each moved non-root subtree gets a new `GRoot` entry. Its parent field in the old page
    becomes a far ref, and a tracked parent slot pointing at that field is redirected to the
    `GRoot` entry.
  * The rest of the page is compacted in the same pass.
* **Remapping.** Each `SuffCache[i]` and `parentSlot[i]` pointing into the page is
  translated through the old→new offset table.
* **Returning memory.** When a page's bump drops ≥ 8 KB below its high-water mark, the tail
  is returned to the OS with `madvise(MADV_DONTNEED)`.

Splitting keeps pages between ~48 KB and ~59 KB live (about 50 KB on average on enwik9).
Receivers keep the number of half-empty pages small.

**Out of pages** (`MMAX` exhausted): `EnsureHeadroom` fails and the coder resets
(`StartModelRare`) or replays the tail of the window (`RestoreModelRare`, `reset_perc`) before
coding the byte. Encoder and decoder do this at the same point.

## 7. Memory accounting

| item | enwik9 o12 |
|---|---|
| live records | 2837 MiB |
| dead bytes inside pages (free lists, pads, unreachable) | 36 MiB (1.3%) |
| partly used last OS page of each page | 105 MiB (3.7%) |
| page metadata, far tables, root table | 35 MiB (1.2%) |
| text window | 954 MiB |
| **peak RSS** | **3972 MiB** |

The original's tree on the same input is 4125 MiB. Its peak RSS is 5224 MiB, or about
4830 MiB with the lazy-init fix in `ppmd_orig_lazy.cpp`.

## 8. Limits

* **Window:** up to 4 GB. k grows with the window, and the scan block (2^k bytes) with it.
* **Tree:** up to `MMAX` MB of pages. Page numbers are 32 bits and `GRoot` holds up to 2^25
  roots (enwik9 uses 1.16M).
* **Per page:** at most 4096 far entries; the largest measured is 307.
* **Orders:** 1..255 as in the original (tested 1..200).

## 9. Invariants (checked by `PPMD_CHECK=n`)

* Every reachable record lies in `[RESV, bump)` of its page and is reached exactly once (tree).
* Multi records have NumStats ≥ 1 and freqs ≤ 124. Binaries are even-aligned, and their
  parity matches their successor type by construction.
* Every far ref names a live far-table entry. Its root id is valid and listed in its page's
  root list.
* Every `SuffCache[0..order]` entry is reachable, and `*parentSlot[i]` references
  `SuffCache[i]`.

## 10. Knobs

These change speed and memory, never the output.

| knob | meaning | default |
|---|---|---|
| `PPMD_DEADSH` | compact when dead > live >> n | 6 |
| `PPMD_SPLIT` | fraction of a page moved per split, n/16 | 3 |
| `PPMD_MARGIN` | split when live + need + margin > 60 KB | 2048 |
| `PPMD_RECVGAP` | bytes a receiver page must keep free | 8192 |
| `PPMD_SCAN` | 0 scalar, 1 AVX2, 2 AVX-512BW | best available |

Debug knobs: `PPMD_CHECK=n`, `PPMD_STATS`, `PPMD_CENSUS`, `PPMD_ALLOCSTATS`. `PPMD_CZK=k`
forces larger scan blocks. Encoder and decoder must then agree on it.
