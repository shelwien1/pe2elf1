# PPMd compact context tree: design

This describes the context-tree storage of `ppmd.cpp`. The model itself (SEE, coding, update,
rescale, inheritance) is the one in `ppmd_orig.cpp`, statement for statement. Only the
storage changed, so compressed output is byte-identical to the original. (Under a memory
limit the smaller tree resets less often, which changes the output and makes it smaller.)

Measured results are in `RESULTS.md`. The analyses this design follows are
`ppmd_tree_compaction.md` (the first layout) and `ppmd_tree_compaction2v3.md` (leaf records
and cold records, its §4.1, §4.3 and §4.4).

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
2-byte aligned, so the bit is free. Bit 62 (`LEAFB`) marks a leaf context (§2.1), bit 61
(`COLDB`) a cold record (§2.2).

### 2.1 Leaf records

A context at depth MaxOrder (a *leaf*) never gets a context successor, and the text value of
its successors is never read. So its states have no `succ` field:

```
leaf multi    [EscFreq:7 | rescaled:1][NumStats] + NU x [sym][freq-1]    2 + 2*NU bytes
leaf binary   [sym][freq]                                               2 bytes
```

* **Why the successor is dead** (§2 I1 of the second analysis). `OrderFall + order <=
  MaxOrder` always holds, so at `order == MaxOrder` the coder takes `CreateSuccessors(Skip=1)`
  and never materialises the found leaf state. A leaf is created only with text successors
  (`CreateSuccessors`, `ExpandAndAdd`). The only use of the leaf state's text value is as
  the start of the block scan, and the lazy state one order below (`ps[0]`) holds the same
  occurrence. So `CreateSuccessors` recovers the position from `ps[0]`, with a pattern one
  byte shorter. A leaf reached with `OrderFall != 0` would be fatal; it never happens.
* **Depth is always known.** `SuffCache[i]` is at depth i, a child is one deeper than its
  parent, and each `GRoot` entry stores its root's depth. `child()` sets `LEAFB` from the
  depth. State loops step by 2 or 4 bytes (`SI`, `SH`).
* A leaf binary has no successor type, so it is not bound to an address parity and never
  moves.
* **Gain:** 2 bytes per leaf state. On enwik8 the tree drops from 493.7 to 459.3 MiB (−7.0%).
  Speed is unchanged.

### 2.2 Cold records

Most contexts are rarely visited. A context that is not current is stored by `Repack` (§6) in
a smaller *cold* form, which the model never reads or writes directly:

```
path record  [A:1 | L-1:4][sym x L][freq x L][end ref:16]
             a chain of L binaries, each the only child of the previous one.
             No end ref when the last element is a leaf, or when A=1: then the end child
             is the cold record that follows this one.
NU=2 record  [0x2000 + code, 2 bytes big-endian][y0][y1][succ0:16][succ1:16]
             leaf: [code][y0][y1], no successors.
             code -> (EscFreq|rescaled, tf0, tf1) through the codebook.
```

| context | hot | cold |
|---|---|---|
| chain of L binaries, ending in a non-leaf | 4L | 1 + 2L + 2, or 1 + 2L with A |
| chain of L binaries, ending in a leaf | 4(L-1) + 2 | 1 + 2L |
| one binary whose child is cold | 4 | 3 (A=1, L=1) |
| NU=2 | 10 (leaf 6) | 8 (leaf 4) |

* **Where they live.** Cold records fill `[RESV, pgCold[p])` of page p, byte-aligned, in
  breadth-first order with each A-child placed right after its parent. A near ref below
  `pgCold[p]` is the exact byte offset of a cold record. The first byte tells the kind
  (below 0x20: path). Hot records start at `pgCold[p]` (even), so hot refs keep their M bit.
  `child()` compares a near ref with `pgCold[p]`.
* **Interior elements are not addressable.** A path record is entered only at its head. A
  chain element becomes current only one step after its parent was current, and a current
  path head is expanded as a whole. So no reference can point inside a cold path.
* **The codebook** maps 24-bit keys (header byte, tf0, tf1) to codes. Codes are reference
  counted: `Repack` acquires a code for every NU=2 record it writes cold and releases it when
  the record is decoded again; expansion releases it too. A code whose count drops to 0 is
  recycled. There are 57344 codes. On enwik9 they suffice for 99.6% of the NU=2 records; the
  rest stay hot. Unreachable cold records (dropped by rescale) keep their codes until the next
  model reset, which clears the codebook.
* **Expansion.** At the start of a step `EnsureHeadroom` expands every cold `SuffCache`
  entry into its hot form in the same page (a path into L binaries), updates `*parentSlot`
  and `SuffCache`, and counts the cold bytes as dead (`cdead`) until the next `Repack`. The
  headroom for this is part of the page's need (§6).
* **Read-only use.** `BequeathFreq` reads the next step's context before it is expanded (I5 of
  the second analysis). It reads a hot-format copy (`coldView`).
* **Gain** (on top of §2.1): enwik8 459.3 → 429.6 MiB, enwik9 see `RESULTS.md`. The cost is
  time: about 0.27 expansions per input byte on enwik9, and more frequent, slower Repacks.

## 3. Pages and references

### 3.1 Pages

The tree lives in an arena of 64 KB pages. The arena is a single `mmap(MAP_NORESERVE)` sized
by `MMAX`, and pages are 64 KB-aligned, so a record's page base is its address masked.

Nothing is written ahead of use, so physical memory is committed only for the part of each
page that has actually been allocated (§6). The first 4 KB of every page (`RESV`) is never
allocated, so that OS page is never touched.

### 3.2 References (16 bits)

```
ref >= pgCold[p]        : near ref = page offset | M     hot child in the same page
4096 <= ref < pgCold[p] : near ref = exact byte offset   cold child in the same page (§2.2)
ref <  4096             : far ref  = index into this page's far table
                            far table[i]  -> global root id g
                            GRoot[g]      =  {page, ref inside that page, depth}
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
exact free lists. All metadata lives in a `PageMeta` array, never in the page itself.

Free records are kept by size, in 2-byte units: class k holds records of 2k bytes, with two
classes for 4 bytes (≡0 and ≡2 mod 4, for the two binary parities), and one first-fit list
for records of 126 bytes or more (size kept in the free record). A 64-bit mask tells which
classes are non-empty.

* **Larger-record fallback.** A request whose exact class is empty takes the smallest larger
  free record (one bit scan of the mask) and carves it. Every remainder of 2 bytes or more is
  a usable free record, because 2-byte leaf binaries fill the smallest holes.
* **Parity.** A non-leaf binary that needs ≡0 or ≡2 and lands on the wrong parity gives up 2
  bytes at the front, which become a 2-byte free record.
* The cold region is never put on free lists. It is rebuilt only by `Repack`.

## 4. Text pointers

A text successor stores only `P >> k`, the 2^k-byte window block holding P. k is the
smallest value with `WinSize >> k < 0xFFFF`: k=14 for enwik9, 11 for enwik8. Any window up
to 4 GB therefore fits in 16 bits.

**When P is needed.** Only in `CreateSuccessors`, at materialisation of the found state.
P is the end of the *first* occurrence of the found context's string plus its symbol, which
is exactly the last order+1 bytes of the current history. So the coder scans that block for
the first position where those bytes end, and that position is P when P is the first
occurrence of that pattern. That is not guaranteed (the second analysis, §9, measured 0.01% of
text states where it fails); the scan then finds the earlier occurrence in the same block.
Encoder and decoder still agree, and in every run measured the output was identical to the
original. For a leaf the scan starts from the lazy state one order below, with a pattern one
byte shorter (§2.1).

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
binary parity. So `need(c) = size(NU+1) + 12` bytes, plus the size of its hot form if it is
cold. Every page holding a current context must have at least the sum of its contexts' needs
as bump space. Nothing inside a step can then fail to allocate.

* **Fast path.** A dense `pgSlack[]` array holds the bump space left before the next untouched
  OS page. If every current page has slack ≥ the total need, nothing more is checked. A
  second test does the per-page checks below with the total need as an upper bound of each
  page's need, which avoids grouping the contexts by page.
* **Slow path, per page.** The page is acted on if it is too tight, or if it would touch a new
  OS page while holding dead bytes > live/64 (`PPMD_DEADSH`). Dead cold bytes up to live/32
  (`PPMD_CDEADSH`) are not counted for this. Then:
  * if `live + need + 2 KB` still fits in the page, it is **compacted**;
  * otherwise it is **split**.
* Then the cold current contexts are expanded (§2.2).

**`Repack(page, split)`** builds a list of the page's contexts, breadth first from its roots
through near refs: one node per hot record, per cold NU=2 record and per cold path record.
Hot non-leaf states are read in place; the others are decoded into a buffer. Then:

1. **Formats** (forward). A cold record stays as it is (copied, its refs patched) unless it
   is current or moves to the receiver; a cold path is then split into its elements again.
   Records that hold a `SuffCache` entry or a `parentSlot` target stay hot. Every other NU=2
   context gets a code and turns cold, and chains of unpinned binaries become path records
   of up to 16 elements.
2. **Adjacency** (backward, children first). A path whose end child is cold drops its end ref
   and stores the child right after itself; a lone binary with a cold child becomes a path of
   1. The byte totals of each group are summed here.
3. **Offsets**: [cold records][multi][binary ≡0][leaf binary][binary ≡2] from `RESV`, via a
   scratch page, so parities need no padding.
4. **Copy and patch.** Every child ref is rewritten from the child's new offset. Records that
   are no longer reachable disappear, including subtrees disconnected when rescale drops a
   state (the original leaks those). Far-table entries nobody references any more are
   freed, and their roots marked dead.

* **Split.** Subtree sizes (of the hot forms) come from the BFS list. The code descends along
  the heaviest child while that subtree is larger than 1.25 × target (target = 3/16 of the
  page, `PPMD_SPLIT`). It then takes that node's children, largest first, up to the cap.
  * The selected sibling subtrees move, in hot form, to a **receiver page**: the page that
    last received a split, while it has room and far-table space, otherwise a fresh page.
  * Each moved non-root subtree gets a new `GRoot` entry. Its parent field in the old page
    becomes a far ref, and a tracked parent slot pointing at that field is redirected to the
    `GRoot` entry.
  * The rest of the page is rebuilt in the same pass.
* **Remapping.** Each `SuffCache[i]` and `parentSlot[i]` pointing into the page is
  translated through its node's new offset.
* **Returning memory.** When a page's bump drops ≥ 8 KB below its high-water mark, the tail
  is returned to the OS with `madvise(MADV_DONTNEED)`.

Splitting keeps pages between ~48 KB and ~59 KB live (about 50 KB on average on enwik9).
Receivers keep the number of half-empty pages small.

**Re-tiering.** A record that stops being current stays hot until its page is repacked.
Records touched since the last Repack are therefore hot (about a quarter of the NU=2
contexts at any time). With `PPMD_COLDQ=2` they are kept hot one Repack longer, which costs
memory and saves expansions.

**Out of pages** (`MMAX` exhausted): `EnsureHeadroom` fails and the coder resets
(`StartModelRare`) or replays the tail of the window (`RestoreModelRare`, `reset_perc`) before
coding the byte. Encoder and decoder do this at the same point: every storage decision
depends only on the model state, never on time or RSS.

## 7. Memory accounting

enwik9, order 12:

| item | no packing (`PPMD_PACK=0`) | packing (default) |
|---|---|---|
| live records | 2837 MiB | 2725 MiB |
| dead bytes inside pages (free lists, pads, unreachable, unpacked copies) | 36 MiB (1.3%) | 53 MiB (1.9%) |
| partly used last OS page of each page | 105 MiB (3.7%) | 94 MiB (3.4%) |
| page metadata, far tables, root table | 35 MiB (1.2%) | 33 MiB (1.2%) |
| **tree (touched pages)** | **2978 MiB** | **2871 MiB** |
| text window | 954 MiB | 954 MiB |
| **peak RSS** | **3972 MiB** | **3865 MiB** |

The original's tree on the same input is 4125 MiB. Its peak RSS is 5224 MiB, or about
4830 MiB with the lazy-init fix in `ppmd_orig_lazy.cpp`.

## 8. Limits

* **Window:** up to 4 GB. k grows with the window, and the scan block (2^k bytes) with it.
* **Tree:** up to `MMAX` MB of pages. Page numbers are 32 bits and `GRoot` holds up to 2^25
  roots (enwik9 uses 1.16M).
* **Per page:** at most 4096 far entries; the largest measured is 307.
* **Orders:** 1..255 as in the original (tested 1..200).

## 9. Invariants (checked by `PPMD_CHECK=n`)

* Every reachable record is reached exactly once (tree). Hot records lie in
  `[pgCold[p], bump)`, cold records in `[RESV, pgCold[p])`.
* A context is a leaf exactly when its depth is MaxOrder; `SuffCache[i]` has depth i.
* Multi records have NumStats ≥ 1 and freqs ≤ 124. Leaf states have no context successor.
  Binaries are even-aligned, and their parity matches their successor type by construction.
* Path records have L ≥ 2 unless A is set, end at or above the leaf level, and an A-child
  lies in the cold region. NU=2 records name a live code; no code is used by more records
  than its reference count.
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
| `PPMD_PACK` | 1: write cold records (§2.2), 0: never | 1 |
| `PPMD_CDEADSH` | dead cold bytes up to live >> n do not trigger compaction | 5 |
| `PPMD_COLDQ` | 1: everything not current turns cold at a Repack; 2: not what was touched since the previous one | 1 |

Debug knobs: `PPMD_CHECK=n` (and `PPMD_CHECKFROM=s`: every step from step s), `PPMD_STATS`,
`PPMD_CENSUS`, `PPMD_ALLOCSTATS`. `PPMD_CZK=k` forces larger scan blocks. Encoder and decoder
must then agree on it.
