# PPMd context tree: options for further compaction

Target: the `ppmd.cpp` variant (no suffix pointers, `NumStats` taken from the 64 KB block header, binary contexts without a header, SoA `STATE0[]` + `SUCC[]`, 32-bit `SUCC` with arena offsets stored halved).
All numbers come from instrumented copies of `ppmd.cpp` (see Appendix), run with `MMAX=256` so no resets happened. Sections 0–5 use **book1** (768,770 bytes). §6–8 add enwik8 (100 MB), its first 8 MiB (e8m) and first 32 MiB (e32m), and revise the ranking for enwik9 (§6.6). book1 is small and has few long repeats. Its tree saturates above order 16 (~9.8 MB) and its best ratio is at order 6, so its chain lengths are a lower bound (enwik8: §6.0).

---

## 0. Summary

| # | Option | Exact? | Saving @o12 (tree 8.92 MB) | Effort / risk |
|---|---|---|---|---|
| A | Variable-width `SUCC` (ctx 2 B paged, text 3 B), type bit in `Freq` MSB | yes | **−2.01 MB (−22.6%)** | high: needs subtree pages + per-page allocator |
| B | Unified 4-byte state (text states drop `Symbol`, read `text[P-1]`) | yes | **−2.74 MB (−30.7%)** | high + window reads in the coding loops; exact 24-bit P needs window ≤ 16 MB. For enwik9 use the coarse-P variant instead (§8, k=14, scan cost unmeasured) |
| C | 16-bit page-local ctx successors only | yes | −1.29 MB (−14.5%) | needs A-style variable width to realise |
| D | 24-bit text pointers | yes | −0.72 MB (−8.1%) | low. Exact P: window ≤ 16 MB. Coarse P>>8 + block rescan: any window ≤ 4 GB, incl. enwik9, ~0.5% time (§8) |
| E | Binary-chain records (user idea #1) | yes | −0.48…−0.63 MB (−5.4…−7.1%); −0.16 MB once B is in | medium |
| F | Text-pointer dedup across orders | yes in principle | ≤ −1.79 MB (62% of text ptrs are duplicates) | hard to keep exact |
| G | Allocator: per-class partial blocks | yes | footprint is 11.93 MB for an 8.92 MB tree (≈3.0 MB fixed) | low |
| H | Endpos-class sharing (suffix automaton PPM) | no | −47% contexts, −38% state records, unbounded order | research |
| I | SA/BWT-based encoder (user idea #4) | n/a | no gain for a PPMd-exact model | see §4 |

The table covers §1–5 (book1). Later sections add enwik-scale options, quoted against the enwik8 o12 tree: text-anchored binary runs (§6.3, −12.6%), store-once text pointers (§6.2b, −16%), per-NumStats layouts (§7, −10%) and coarse 3-byte text pointers (§8, −7.9%). §6.6 has the ranking for enwik9.

Key facts that drive the ranking:

* **`SUCC` is 61% of the tree** (5.48 of 8.92 MB). `STATE0` is 31%, headers 8%.
* **53% of successors are text pointers** (724,831 vs 644,518 ctx pointers). Text pointers are the single biggest item, bigger than all binary contexts together.
* **Context pointers are almost perfectly paginable.** A static subtree partition into 64 KB pages leaves 0.30% of ctx edges crossing pages.
* **The tree is exactly the depth-D suffix trie** of substrings that occurred ≥2 times (verified, §1.3). Binary chains are suffix-tree edges, so the suffix-tree / DAWG / CST literature applies directly.

---

## 1. Baseline census

### 1.1 Layout recap

| record | bytes |
|---|---|
| binary context | `STATE0`(2) + `SUCC`(4) = 6, no header |
| multi context, NU states | `_EscFreq`(2) + 2·NU + 4·NU |
| `SUCC` value | 0 = null, 1..WinSize = text offset+1, >WinSize = arena offset/2 |

### 1.2 Order 12, book1

| item | value |
|---|---|
| contexts | 644,519 (binary 294,629 = 45.7%, multi 349,890) |
| states | 1,369,524 |
| `SUCC` kinds | ctx 644,518 · text 724,831 · null 175 (order-0 init) |
| bytes: `STATE0` / `SUCC` / headers | 2.74 MB / 5.48 MB / 0.70 MB |
| arena footprint | 182 blocks = 11.93 MB; free slots inside used blocks 3.00 MB |

Bytes by NU: binary 19.8% · NU=2 34.4% · 3–4 21.6% · 5–8 13.0% · 9–16 7.1% · ≥17 4.0%.
Contexts by depth peak at d=6..8. The binary share grows with depth: 15% at d=2, 55% at d=12.

Across orders:

| order | ratio (bytes) | tree MB | contexts | states | text-ptr share | binary ctx | chain nodes (L≥2) |
|---|---|---|---|---|---|---|---|
| 4 | 216,300 | 1.04 | 42,144 | 163,184 | 74% | 11,413 | 2,302 |
| 6 | 209,343 | 3.46 | 193,997 | 535,193 | 64% | 71,221 | 34,425 |
| 8 | 209,603 | 6.08 | 394,432 | 936,470 | 58% | 164,530 | 110,027 |
| 12 | 209,793 | 8.92 | 644,519 | 1,369,524 | 53% | 294,629 | 234,773 |
| 16 | 209,809 | 9.59 | 711,660 | 1,472,792 | 52% | 333,229 | 273,702 |
| 64 | 209,820 | 9.79 | 734,385 | 1,503,286 | 51% | 349,132 | 289,589 |

### 1.3 The tree is the depth-D suffix trie

Context counts equal the number of distinct substrings with |u| ≤ D occurring ≥2 times, exactly at every D tested. State counts equal the number of (u, next symbol) pairs, exactly for D ≥ 16 and within 2% at D=4.

| D | PPMd contexts (excl. root) | substrings occ≥2, len≤D | PPMd states (excl. order-0) | (substring, next) pairs |
|---|---|---|---|---|
| 4 | 42,143 | 42,143 | 162,928 | 166,178 |
| 12 | 644,518 | 644,518 | 1,369,268 | 1,369,270 |
| 64 | 734,384 | 734,384 | 1,503,030 | 1,503,030 |

Consequences:

* A binary context is an *implicit* suffix-tree node and a multi context is an explicit (right-branching) one. Binary chains are suffix-tree edges.
* Every subtree contains at least one text pointer: all states at depth D are text pointers. So the string of any context can be recovered from text plus depth, which is the suffix-tree "edge label = text pointer" property.
* **Single owner.** Since there are no suffix links, the only persistent reference to a context is its trie parent's `SUCC`. Everything else (`SuffCache`, `StateCacheCtx`, `parent_iSuc_cache`) is transient and rebuilt every symbol. This is what makes relocation, compaction and paging cheap in this variant compared with stock PPMd.
* **Verified invariant:** for every state with a text successor P, `Symbol == text[P-1]`. There were 0 mismatches out of 724,831 at o12, and 0 at every order tested.

---

## 2. Exact options (bit-identical output)

### 2.1 Binary chains as one record (user idea #1)

**Census (o12).** Binary contexts split by what their successor points to: 158,064 point to a binary, 115,306 to a multi, and 21,259 to text. The text ones are leaves, and they essentially all sit at d=D, because below D a binary context is materialised further as soon as it is used.
Maximal unary chains: 136,565, of which 76,709 have L ≥ 2. Those hold 234,773 of the 294,629 binaries (80%).

| terminator | chains | L=1 | 2 | 3–4 | 5–8 | 9–16 | max L |
|---|---|---|---|---|---|---|---|
| text | 21,259 | 4,285 | 4,468 | 7,009 | 5,308 | 189 | 11 |
| multi ctx | 115,306 | 55,571 | 30,623 | 24,048 | 5,048 | 16 | 9 |

At o64 the longest chain is 60, but the totals barely move (289,589 nodes in chains).

**Freq values don't compress along a chain.** Over consecutive pairs (deeper vs shallower), freq increased 85,256 times, decreased 51,130 times and stayed equal 21,678 times. Only 4,760 of 76,709 chains have all-equal freqs. So store one freq byte per element; there is no RLE or monotone delta to exploit.

**Record design.** This needs no `EscFreq` byte, because binary contexts have no header. The hook is `MEM_BLK::_pad`, which is free today: use it as a block *kind*, with chain-capacity classes alongside the NU classes.

```
MEM_BLK.kind (was _pad): 0 = ordinary NU block, else chain capacity class C
chain record (cap C):    [len:1][pad:1] ([sym:1][freq:1]) x C  [succ:4]     = 2C+6 bytes, even
element i address        = rec + 2 + 2i      (even -> the existing halved-offset SUCC addresses it as-is)
NumStats(elem)           = 0                 (binary semantics unchanged; BinSumm/SEE path untouched)
state0s(elem)            = (STATE0*)elem
succ(elem)               = (i < len-1) ? elem+2 : rec->succ     (implicit link to the next element)
locate rec from p        = blk + hdr + ((p-blk-hdr) / (2C+6)) * (2C+6)     (per-class reciprocal)
```

An interior pointer *is* the context: a parent `SUCC` pointing at element i needs no new field.

**Operations.**

* **Extend.** When the last element's text successor gets materialised (`CreateSuccessors`), append in place if `len < C`. Otherwise start a new segment linked from the old terminal `succ`. A whole chain cannot be moved, because the head's external parent slot is not known when the active element is deep in the chain: the parent of `SuffCache[k]` is the *previous step's* `SuffCache[k-1]`, not any current entry. Moving would cost a 4-byte back-pointer per record (2C+10 variant).
* **Split.** When element i gets a second symbol, element i is the current `SuffCache[k]`, so its parent is known: either element i−1, an implicit link that becomes the head's terminal `succ` after `len ← i` in place, or `parent_iSuc_cache[k]` when i=0.
  1. Allocate an NU=2 context for element i.
  2. Copy the tail i+1.. into a new record (or a plain binary if it is a single element).
  3. Scan `SuffCache[]` for pointers into the moved tail. These exist on periodic text, because `"aa"`, `"aaa"` can be in the same chain.
* **API.** `succAt()` currently returns a `SUCC&`. For implicit links it has to become a tagged proxy, and `parent_iSuc_cache` needs an "implicit, previous element" marker.

**Savings @o12.**

| variant | saving | share of tree |
|---|---|---|
| ideal 2L+4, L≥2 (no length byte) | 632 KB | 7.1% |
| record above, 2L+6 (`len`+`pad`), exact-fit capacity, L≥2 | 479 KB | 5.4% |
| text-terminated chains with implicit symbols (L+4, see below) | 698 KB | 7.8% |
| movable with back-pointer (2L+10, L≥3) | 242 KB | 2.7% |
| on top of 4-byte states (§2.5) | 163 KB | 1.8% |

Text-terminated chains with terminator P have symbols `text[P-L..P-1]`, which was checked for all 21,259. Capacity slack and segmentation will eat part of the ideal number.
**Verdict:** on book1, worthwhile only if `SUCC` stays 32-bit. On enwik8 the ideal saving grows to 9.8% / 12.0% (§6.0), and §6.3 gives the preferred form (text-anchored records with freq nibbles).

Symbol-free variant for multi-terminated chains: anchor the record at a text position Q (any text pointer in the subtree minus depth). Element symbols are then `text[Q+i]`. Walking the chain is walking the text, which makes it effectively a match model. The record becomes L+8..9 bytes vs 2L+4, so it pays only for L > 5.

### 2.2 Subtree pages, 16-bit successors (user idea #2)

**Static census (final tree, bottom-up partition, page roots = maximal subtrees ≤ P):**

| order | P | page roots | top nodes | cross ctx edges | FFD pages | fill |
|---|---|---|---|---|---|---|
| 12 | 64 KB | 1,867 | 60 | 1,926 / 644,518 (**0.30%**) | 136 | 99.9% |
| 12 | 128 KB | 956 | 27 | 982 (0.15%) | 68 | 100% |
| 64 | 64 KB | 1,990 | 64 | 2,053 / 734,384 (0.28%) | 150 | 99.4% |

**Design.**

* **Page.** 64 KB with byte offsets, or 128 KB with 2-byte units. Contexts of mixed NU share a page, so `NumStats` can no longer come from the block. It goes into the **high byte of `_EscFreq`** for multi contexts; that byte is always 0 today. Binary contexts have no header, so they are flagged by a tag bit in the 16-bit pointer, which is free with 2-byte alignment at 64 KB.
* **Cross edges.** They point at a 4-byte far stub inside the source page (0.3% of edges, about 8 KB total).
* **Allocation policy.** Allocate a child in its parent's page. That is the natural locality: `UpdateModel` and `CreateSuccessors` create u·s right after visiting u.
* **Overflow.** Evict the largest child-subtree of the page root into a fresh page. Only one pointer changes, and its slot is inside the old page.
* **Per-page allocator.** PPMd-style unit free lists with 16-bit offsets.
* **Unmeasured:** the dynamic fill factor. B-tree-style splitting will land well below the static 99.9%.

On its own this saves nothing: text pointers still need ≥24 bits, so a uniform 32-bit `SUCC` gains nothing. It pays only combined with a variable-width `SUCC` (2.3, 2.5).

### 2.3 Text pointers (largest single item)

**Census (o12).**

* 724,831 text pointers; 92% of their states have freq ≤ 7 (max 121 in multi contexts; 196 in MaxOrder binaries at o4/o6).
* Age: 84% are older than 2^17 bytes, so relative-to-`pText` coding is useless.
* Only 276,953 distinct values: 62% are duplicates. Multiplicity counted per distinct value: 113,332 occur once, 51,034 twice, 66,956 3–4×, 42,902 5–8×, 2,729 9+×. Duplicates come from one `UpdateModel` step assigning `pText` to new states in all contexts `maxorder..order+1`, and from `CreateSuccessors` handing `upPtr+1` to all new suffix-siblings.
* 62,893 (8.7%) live in depth-D contexts and are never materialised (no depth D+1). They serve only as `iUpBranch` and the `sym1` source in `CreateSuccessors(Skip=1)`.

**Options.**

1. **24-bit text pointers** when the window is ≤ 16 MB: −0.72 MB (8.1%). This is configurable per run; `WinSize` is known before `StartSubAllocator`.
2. **Implicit `Symbol`** for text-successor states (invariant from 1.3): −0.72 MB. The cost is window reads of `text[P-1]` in the coding loops, on both sides. The encoder's `FindState` scan and `processSymbol2` read every unmasked state's symbol. The decoder reads only the hit symbol on a hit, but on every escape it sets `CharMask` for all symbols of the context, so it pays the same reads. `CacheNumstatsAndFlags` also scans symbols every step (HasText = any symbol ≥ 0x40). Measured: 2.8–3.2 such reads per byte (§7.3). Coarse text pointers (§8) give the same byte saving without these reads; the two don't combine.
3. **Variable-width `SUCC` inside a context.** Use a type bit in the `Freq` MSB: multi-context freq is ≤ 128, so `freq-1` fits 7 bits. Binary records carry their type in the parent pointer's tag instead. Then ctx = 2 B (paged) and text = 3 B (or 4 B for big windows).
   * PPMd always scans a context linearly: the coder loops, `FindState`, `SummFreq`, the rescale insertion sort. So the byte offset of `succ[i]` can be accumulated during the same scan, with no random-access penalty. `swapStateAt` moves records of possibly different widths, which means a small `memmove` within the context.
   * Saving @o12: 5.48 → 3.46 MB, **−2.01 MB (−22.6%)**.
   * With a 4-byte exact text successor (window > 16 MB), only the ctx half survives: −1.29 MB (−14.5%). Coarse 3-byte text pointers (§8) restore the full saving for windows up to 4 GB.
4. **Dedup across orders**, the 62% duplicates: up to 447,878 × 4 = 1.79 MB.
   * A flag "same P as my suffix's state" plus a lookup into `SuffCache[k-1]` (the walk `CreateSuccessors` already does) would cover it.
   * Problem: the suffix state can materialise independently, from a b·u visit, and its P is then overwritten by a ctx pointer. The value is still recoverable from the new child's text pointer minus 1 (`ctstate.setSucc(upPtr+1)`) and from deeper descendants minus their depth offset. That stays true only while the descent follows the same occurrence.
   * Keeping this exact is fragile. Treat it as a research item, not a first step.

### 2.4 Delta + variable-length pointers (user idea #3)

**Measured `|child − parent|`** for ctx successors at o12, with the current NU-segregated allocator:

| range | edges | share |
|---|---|---|
| < 2^6 bytes (siblings allocated back-to-back) | 131,666 | 20.4% |
| 2^6 .. < 2^13 | 2,968 | 0.5% |
| 2^13 .. < 2^24 (median ≈ 2^21) | 509,884 | 79.1% |

Text-pointer deltas against `pText` are worse (see 2.3). In the current layout VLC buys roughly 32 → 21 bits at best, and costs repacking on every change.

Where VLC fits:

* **Inside one context's record.** Records are rewritten on every expand/shrink anyway and always scanned linearly. Freqs as nibbles with escape (92% of text-state freqs and 34% of multi-context ctx-state freqs are ≤ 7) and page-relative successors as 1–2 B varints are cheap here. The record must be re-encoded when a freq crosses a width boundary, which is O(NU) and the same order as the scan already paid.
* **Burst buckets** (burst-trie / HAT-trie style). The deep, low-fanout part of the tree is stored as serialized subtree buckets of 256 B–4 KB with VLC deltas, rewritten on modification and burst into pages when too big. Cost model: each symbol modifies ≤ D contexts, mostly at the deepest orders, so rewrites concentrate in the leaves. Bucket size controls the write amplification.
* **Frozen snapshots** (pretrained/dictionary model preload, serialization). A LOUDS trie (2 bits/node) + label bytes + freq bytes + VLC text positions gives roughly 3 B/state.

### 2.5 Unified 4-byte state (combination of 2.2 + 2.3.1 + 2.3.2)

```
32-bit word w
  bit0 = 1 : text state   freq = bits1..7  (freq-1),  P = bits8..31 (24 bits)   symbol = text[P-1]
  bit0 = 0 : ctx state    bit1 = child-is-binary, bits2..15 = page offset/4,
                          bits16..23 = freq, bits24..31 = sym
binary context = one word (+ chain records, 2.1); text-successor binaries with freq > 128 don't fit, see constraints
multi header   = [EscFreq:7|resc:1][NumStats:8] (+ pad to 4 if offsets are /4)
```

| order | states × 4 B | saving vs 6 B/state | share of tree |
|---|---|---|---|
| 12 | 5.48 MB | −2.74 MB | −30.7% |
| 64 | 6.01 MB | −3.01 MB | −30.7% |

Constraints:

* Window ≤ 16 MB with exact P. With coarse P (§8) the 24 bits cover any window ≤ 4 GB.
* The text-state freq must fit 7 bits. In multi contexts it does: the cap is `MAX_FREQ`+4 = 128 (max 121 observed). Binary freqs are capped at 196 (`rs.Freq<196`; observed in MaxOrder binaries at o4/o6), so a text-successor binary needs a fifth byte or a freq clamp, which changes the model.
* Context successors must be in the same 64 KB page (14-bit offset in 4-byte units), with far stubs for the rest.

### 2.6 Allocator overhead

The real footprint is 11.93 MB for an 8.92 MB tree. The main program prints `alloc_size=8.50 MB`, which hides it. There are 61 NU classes in use and all 61 have a non-full block; the free slots inside used blocks total 3.0–3.3 MB at every order (3.28 MB at o4, where the tree is only 1.04 MB).

This is a fixed cost of at most 256 × 64 KB. It is negligible for GB-scale models but dominates small ones.

Fixes:

* Smaller blocks for rare large-NU classes.
* Rounding NU to coarser classes above ~16; only 4% of bytes are in NU ≥ 17.
* **Copying compaction instead of reset/restore.** The tree has a single owner per context, so a DFS copy from `Order0` into a fresh arena only needs to rewrite each parent's `SUCC`. It also produces subtree order, which is the precondition for 2.2/2.4.
* Prune during the copy: drop text states in depth-D contexts, or low-freq deep contexts, PPMd var.I cut-off style.
* This could replace `RestoreModelRare`'s replay.

### 2.7 Measured small or negative results

* **Inlining leaf binaries into the parent's `SUCC`.** A leaf binary is just "text pointer P + freq": its symbol is `text[P]` and its successor is P+1. But there are only 21,259 at o12 (127 KB, 1.4%), 419 at o24 and 41 at o64. Not worth it on book1. At scale it is: enwik8 o12 has 6.40 M leaf binaries (38 MB, 5.4% of the tree; §7.2). The text-anchored records of §6.3 subsume this.
* **Hash-consing identical binaries.** Suffix-siblings created together by `CreateSuccessors` share symbol, freq and P at creation. By the end, 54% of binary a·u still have u with the same symbol and freq, but **0** are byte-identical including the successor: they diverge as soon as either materialises. No sharing is possible.
* **Freq RLE/delta along chains:** no pattern, see 2.1.

---

## 3. Model-changing options

### 3.1 Endpos-class sharing → suffix-automaton PPM

The suffix-direction redundancy is large:

* 51.1% of multi contexts a·u (d ≥ 2) have a suffix u with the identical symbol set (178,878 / 349,812).
* Every binary a·u has u with the same symbol.

Such suffix contexts are fully masked whenever they are reached by escape from a·u. The existing `while(NumStats_Cache[order]==see.NumMasked)` loop skips them at coding time, but their memory is still spent.

Merging contexts with identical end-position sets gives the suffix automaton:

| D | PPMd contexts | endpos classes (occ≥2, minlen≤D) | PPMd states | class transitions |
|---|---|---|---|---|
| 8 | 394,431 | 281,273 (−29%) | 936,214 | 730,882 (−22%) |
| 12 | 644,518 | 377,322 (−41%) | 1,369,268 | 913,466 (−33%) |
| 64 / ∞ | 734,384 | 391,996 (−47%) | 1,503,030 | 938,341 (−38%) |

Of the 391,996 classes, 231,358 are branching (777,703 transitions) and 160,638 are unary. The unary ones are the DAWG analogue of chains; CDAWG would compress them further.

Notes:

* **Order becomes unbounded at O(n) size.** The book1 class count saturates at ~392K regardless of D.
* **Construction needs a suffix link and `len` per class**, re-adding what this variant removed. A rough byte estimate is 938K × 6 + 392K × (2 header + 4 link + 2 len) ≈ 8.8 MB vs 9.8 MB at o64. So the main win is unbounded order rather than bytes, unless combined with §2 packing.
* **Classes split over time** (the clone step); stats are copied on clone. That is exact for "same history ⇒ same stats" only if per-class stats depend on occurrence history alone. PPMd's do not (MaxContext dynamics, inheritance), so this is a different model and compression must be measured.
* **The one-occurrence states** (1.16M total SAM states vs 392K with occ≥2) are prefix states identified by position. A lazy SAM can keep them as text pointers, mirroring PPMd's lazy successors, but they get materialised when they become a suffix-link target. That is non-trivial.

### 3.2 Deep deterministic chains → match model

Binary contexts are 1.77 MB (20%) at o12, mostly unary chains, i.e. long repeats. Options:

* Cap the tree at a moderate order (o6 is the best ratio on book1 anyway) and predict longer deterministic contexts with a match model: one hash table of positions plus SSE keyed by match length and freq-like counters.
* Or remove long repeats before PPM with a dedup pass (srep-like), which deletes the chains at the source.

### 3.3 Quantized binary freqs

`BinSumm` uses only `QTable[Freq-1]` (27 levels), but inheritance (`BequeathBinaryCtx`, the `ns1==0` branch of `UpdateHigherOrder`) uses the exact freq. A 4–5-bit nonstationary counter for binary records frees bits for 2.5-style packing, but changes the model.

### 3.4 Lookahead strings instead of text pointers

Storing k next-symbol bytes instead of a position breaks even with a 24-bit pointer at k=3. The point is not size: it **removes the text window**, which is WinSize = file size here, 1 GB for enwik9. Continuation is lost beyond k symbols until the next recurrence.

### 3.5 Hash-addressed contexts

Hash-addressed contexts remove ctx successors entirely, in exchange for check bits and load-factor slack. Text pointers (53% of successors) still need storage or a 3.4-style replacement. Compare against `ppmd_htnative` with the census tool to see which half of `SUCC` it really eliminates.

---

## 4. SA/BWT-based encoder (user idea #4)

**What it can do.** The encoder knows the whole input, so SA + LCP can be built upfront (libsais). LCP can be clipped to one byte because MaxOrder ≤ 255. That is SA 4n + LCP n + text n = 6n = 4.6 MB for book1, independent of D.

A context of order k at position i is the LCP-interval at depth k containing rank(i), taken in the reversed-text order (left contexts). Its id is (interval node, k − node mindepth), since contexts along a reversed-suffix-tree edge are distinct PPM contexts.

**Why it doesn't pay for a PPMd-exact model.** The encoder must reproduce the decoder's adaptive per-(context, symbol) state exactly: freqs, `EscFreq`, rescale and inheritance history. Only navigation can be dropped, meaning `SUCC` and the text window semantics.

At o12:

| encoder memory | MB |
|---|---|
| `STATE0` + headers | 3.44 |
| context-id → state-array index (4 B × 644K) | 2.58 |
| SA side | 4.6+ |
| **total** | **≈ 10.6** |
| vs. today: 8.92 tree + 0.77 window | 9.69 |

No gain at D=12; it only wins where the PPMd tree is O(n·D) and the SA side is O(n).

**Where SA/BWT does help.**

1. **History-only models.** Make the per-context statistics a pure function of the occurrence history: counts or decayed counts over earlier occurrences, with escapes, exclusion and SEE in a separate time-major pass with small global state. Then the encoder needs no per-context state at all.
   * "Count of s among occurrences of u before i" is a 2D dominance query over (SA-rank interval, position < i). Answer it with a wavelet tree over the next-symbol column, or with offline per-interval sweeps.
   * The decoder still needs an online structure (SAM, 3.1, or a tree), so this only helps asymmetric setups.
2. **Encoder-side analysis to steer the decoder model.** For example, choose a max order or dedup ranges per block and signal them, or use the SA-derived longest-match length as an SSE input that the decoder recomputes with its own match model. Any decoder-side memory saving has to be signalled, because the decoder lacks the future.
3. **Frozen models.** Build a static compressed suffix tree (FM-index + LCP + succinct topology) of a dictionary or training text, plus per-node freq arrays.

---

## 5. Suggested experiment order

This is the book1-based order. §6.6 supersedes it for enwik9 and includes the coarse-pointer result from §8.

1. **G (allocator):** fixed-overhead reduction plus copying compaction. Cheap, exact, and the copy pass is reusable infrastructure for everything below.
2. **D + 2.3.3:** 24-bit text pointers and variable-width `SUCC`, without paging first. That is −0.72 MB, and it validates the scan-accumulated offsets.
3. **2.2 paging:** gets ctx successors to 2 B; with step 2 this reaches −22.6%. Measure dynamic fill and split frequency.
4. **2.3.2:** implicit text-state symbol, reaching the 4-byte state (−30.7%). Measure encoder speed.
5. **2.1 chains:** only if repetitive targets show long chains in the census.
6. **3.1 SAM PPM:** as a separate model, for unbounded order.

---

## 6. Follow-ups: enwik scale, text-pointer structure, laziness, packed descriptions, bitmasks

New data, all at order 12:
* **enwik8** (100 MB): full census, no suffix automaton (too big). Compressed size 20,828,753.
* **e8m** (first 8 MiB of enwik8): full census including the suffix automaton. Compressed size 1,937,536.

### 6.0 Census at enwik scale

| item | book1 | e8m | enwik8 |
|---|---|---|---|
| tree bytes | 8.92 MB | 81.3 MB | 716.1 MB |
| tree / n | 11.6 | 9.7 | 7.2 |
| contexts / states | 0.64 M / 1.37 M | 6.46 M / 12.65 M | 54.6 M / 111.5 M |
| `SUCC` share of bytes | 61.4% | 62.2% | 62.3% |
| text pointers (share of successors) | 0.72 M (52.9%) | 6.19 M (48.9%) | 56.9 M (51.0%) |
| text-pointer bytes / tree | 32.5% | 30.5% | 31.8% |
| binary-context bytes / tree | 19.8% | 27.8% | 25.9% |
| chain saving, 2L+4 / text-implicit | 7.1% / 7.8% | 11.2% / 13.4% | 9.8% / 12.0% |
| cross-page ctx edges @64 KB | 0.30% | 0.38% | 0.40% |
| allocator free slots / footprint | 25% | 5.5% | 0.9% |

The structural picture holds at scale. The allocator overhead (§2.6) vanishes, while chains matter more than on book1.

### 6.1 enwik9 constraints (Q2)

* **Window = 10^9, so exact text pointers need 30 bits.** That rules out the exact-P forms of options D and B. Coarse pointers (§8) bring both back: k=8 gives 3 B at ~0.5% time, and k=14 gives 2 B with the scan cost unmeasured.
* **The current `SUCC` encoding needs `1 + WinSize + arena/2 ≤ 2^32`.** With the full window that caps the arena at 6.59 GB. Tree/n falls with n (9.7 → 7.2 from 8 MiB to 100 MB). If it reaches about 5–6 for enwik9, o12 sits right at the 32-bit limit and anything higher means resets or a wider `SUCC`. So compaction directly buys order or fewer restarts.
* **Options that work at enwik9 window size** (enwik8 o12, savings against the 716 MB tree):

| option | saving |
|---|---|
| ctx successors 2 B, paged (§2.2) | −109 MB (−15.2%) |
| coarse 3-byte text pointers (§8, k=8), ~0.5% time | −57 MB (−7.9%) |
| *or* implicit `Symbol` in text states (5 B instead of 6), window reads in the coding loops | −57 MB (−7.9%) |
| binary chains | −70 / −86 MB (−9.8 / −12.0%) |
| binary runs as text-anchored records with nibble freqs (§6.3) | −90 MB (−12.6%), +0.06% ratio |
| removing text pointers entirely (§6.2) | up to −228 MB (−31.8%) |

### 6.2 Text-pointer structure and dedup (Q1)

**Measured.**

* **P is the end of the first occurrence of (context string + symbol).** That holds for 724,830 of 724,831 in book1 and 6,187,884 of 6,188,286 in e8m (99.99%).
* **The suffix state either shares P or is a context.** For every text state in a·u, the same symbol's state in u is either:
  * the same P: 447,878 (book1) · 3,563,819 (e8m) · 29,367,852 (enwik8), or
  * already a child context: 276,950 · 2,624,466 · 27,524,967.

  "Different P" is **0** in all three.
* **Duplicate groups are consecutive depths in 100% of cases** (163,621 · 1,245,444 · 11,254,462 groups). A group is `SuffCache[order+1..maxorder]` of one `UpdateModel` step, or the suffix-siblings of one `CreateSuccessors`. Same P means the same end position, so the members are suffixes of one string.
* **Duplicates rarely share a forward subtree.** Only 8.6% · 11.6% · 10.8% of duplicates sit in the same top-level (first-symbol) subtree as another member of their group.
* **Deltas don't help either.** Against the first occurrence of the owning context, delta = 0 for 33% / 35% of pointers: these are the "first follower" states. The rest are as wide as raw: Elias-gamma costs 23.8 / 26.3 bits per pointer vs log2(n) = 19.6 / 23.0.

**So forward-subtree / prefix-tree compaction can't capture this redundancy.**
* A group lies on one *suffix* path. That is the reversed-trie direction: what suffix links walk and what `SuffCache` holds at every step.
* Per-subtree delta or prefix coding of the values gains nothing beyond the first follower.

**What does work.**

**(a) A text pointer is a previous-occurrence oracle.** `ppmd_mf.cpp` takes `upPtr` in `CreateSuccessors` from a matchfinder lookup instead of the stored P: the most recent previous occurrence of the last order+1 bytes, verified by `memcmp`. Lower-order siblings are included iff they are lazy.

| | book1 | e8m |
|---|---|---|
| hits equal to the stored P | 300,803 / 300,803 | 2,555,219 / 2,555,219 |
| output | byte-identical | byte-identical |
| materialisations per byte | 0.40 | 0.32 |

The output stays identical because a miss falls back to the stored P. So stored text pointers carry no information beyond a lazy bit, as long as the window can be searched.

The catch is misses.
* Direct-mapped per-length tables (13 lengths × 2^B entries) miss 2.8% at 2^22, 10.4% at 2^20 and 31% at 2^18 on book1; on e8m 4.8% at 2^24 and 39% at 2^20. The needed occurrence is usually the *only* previous one and far away; ages are 2^24–2^26 on enwik8.
* A naive miss policy (guess `sym1` from the lower context) wrecked the model: book1 +20% at 2^22. A miss has to mean "no lookahead, stay lazy", which needs `CacheSuccessors` to tolerate gaps (see 6.3).
* An exact matchfinder with LZ-style hash chains costs 4n bytes: 400 MB for enwik8 vs 228 MB of text pointers. No win.
* The route that can win at enwik9 is a compressed index of the window: block-built FM-indexes merged LSM-style, plus a small hash for the not-yet-indexed tail. A string that occurred once before has a backward-search interval of size 1, so locate gives P exactly. The cost is order+1 rank operations plus one locate per materialisation (0.3–0.4 per byte). Memory is roughly n·H_k plus SA samples, i.e. ~0.3 GB for enwik9 against an extrapolated ~1.6–2 GB of text pointers. This is research-level, but it is the only option that removes the 32%.

**(b) Store-once per group (exact).**
* Add a flag "same P as my suffix state". P lives in the lowest group member, and the suffix state is always `SuffCache[k-1]` when a·u is current.
* When the suffix state materialises, P survives as child.P − 1. That is directly present in 40% / 48% / 50.5% of cases; otherwise follow the first-follower path down, which needs a first-follower index per context.
* It covers the duplicate share: 61.8% / 57.6% / 51.6% of text pointers, i.e. 1.8 / 14.3 / 117.5 MB.

### 6.3 More laziness (Q3)

Today a context is materialised at the 2nd visited occurrence of its string, and binary runs are built one level per symbol while a repeat is followed.

**Measured.**
* **Contexts whose string occurs exactly twice in the whole file** were created at the 2nd occurrence and never used again:

| | multi | binary | share of tree |
|---|---|---|---|
| book1 | 1.88 MB | 1.10 MB | 33% |
| e8m | 12.6 MB | 12.5 MB | 31% |

* **How much private state does a binary context need?** (`ppmd_vbin.cpp`, `-DVMODE=n`):

| binary freq | book1 o6 | book1 o12 | e8m o12 |
|---|---|---|---|
| baseline | 209,343 | 209,793 | 1,937,536 |
| frozen at creation | +2.3% | +3.5% | +4.8% |
| virtual: recomputed at use from the first multi suffix (`BequeathBinaryCtx`), never stored | +1.5% | +2.4% | +3.2% |
| 4-bit saturating | +0.03% | +0.02% | +0.06% |

So a binary context needs about 4 bits of its own; its symbol and successor come from the text.

**Design: materialise at divergence, not at recurrence.**
* A deterministic context is (P, depth): its symbol is `text[P]` and its successor is (P+1, depth+1).
* While the history follows the text from P, each order just advances P and allocates nothing. Allocation happens when a second distinct follower appears.
* Store binary runs as text-anchored, path-compressed records with one freq nibble per node:
  * text-terminated: `{P:4, len:1, nibbles}` = 5 + L/2 B (symbols are `text[P-L..P-1]`, see 2.1)
  * multi-terminated: `{succ:4, len:1, syms:L, nibbles}` = 5 + 1.5L B
* On enwik8 that turns 185.8 MB of binaries into 95.7 MB: **−90 MB (−12.6%)**, at +0.06% (e8m).
* Going fully virtual (no nibble) costs +2.4–3.2% unless a better stateless estimator is found. Candidates: match length so far, the suffix multi's freq, and the depth gap, combined in an SSE.

**Delaying multi contexts to the 3rd occurrence.**
* At the 3rd occurrence of u both earlier followers are recoverable from text: the 1st from the lazy P, the 2nd from the most recent previous occurrence of u.
* That avoids the occ=2 multi contexts: 21% (book1) / 15.5% (e8m) of the tree.
* It needs the exact oracle from 6.2a, plus gap-tolerant `CacheSuccessors`. Lazy levels inside `SuffCache` are harmless for coding: a lazy binary has the same symbol as the longer context it is a suffix of, so after that longer context escapes it is fully masked and skipped (all binary a·u have u with the same symbol, §3.1).
* The compression effect is unmeasured.

### 6.4 Packed / entropy-coded context descriptions (Q4)

Information content vs storage at o12:

| component | storage (book1 / e8m / enwik8) | estimate | model |
|---|---|---|---|
| symbols | 1.37 / 12.65 / 111.5 MB | 1.21 / 11.2 / 97.8 MB | log2 C(256, NU) per context, self-contained |
| | | 0.13 / 1.19 / 11.0 MB | subset of the suffix context's symbol set (0 violations) |
| freqs | 1.37 / 12.65 / 111.5 MB | 0.62 / 6.18 / 57.3 MB | H0 per class (binary / multi-text / multi-ctx) |
| | | 0.52 / 4.93 / – | conditioned on the suffix context's freq for the same symbol |
| `EscFreq` byte | 0.35 / 2.70 / 23.6 MB | 0.18 / 1.39 / 12.3 MB | H0 |
| text pointers | 2.90 / 24.75 / 227.6 MB | 1.77 / 17.8 / 189 MB at log2(n) bits; ~0 given an occurrence index | |
| ctx pointers | 2.58 / 25.8 / 218.3 MB | ~0 if subtrees are serialised in DFS order (plus skip sizes) | |

* **Self-contained per-context coding of the stats saves only ~11–12%** of the tree (book1 −1.08 MB, e8m −9.2 MB, enwik8 −79 MB). The stats (symbols, freqs, `EscFreq`) are ~35% of the bytes and this removes only about a third of them; pointers are the other 62%.
* **The big conditional gains** (symbols ~10×, freqs another 16–20%) come from coding a context relative to its suffix. The suffix is always unpacked in `SuffCache` when the context is touched, but it is **mutable**. When u gains or drops a symbol, or changes a freq, every left-extension's code becomes undecodable, and those extensions can't be found because there are no back-links.
  * Usable only in frozen snapshots, or for symbol sets relative to an append-only list (u's symbols in insertion order, with tombstones on rescale). That bitmask costs 0.31 / 3.02 MB, about 1.8–1.9 bits/state.
  * Freq conditioning stays out.
* **Per-symbol work in a packed design.**
  * `CacheNumstatsAndFlags` reads every `SuffCache` context (order+1 of them); the `NumStats`, rescaled and HasText bits can live unpacked in a header.
  * Coding reads `MinContext` and the escape chain.
  * Updates rewrite the found context, order−1, maxorder..order+1, and the `CreateSuccessors` siblings.

  That is about 3–6 re-encodes per symbol. Entropy-coded records change length on most updates (freq += 4), so they need a size-class allocator with slack. The low orders are hot and should stay unpacked.
* **Verdict:** pack only after the pointers are gone or implicit. Otherwise you pay the codec cost on every update for an ~11% saving.

### 6.5 Bitmasks and recency (Q5)

**State order carries no probability information in this `ppmd.cpp`.**
* `ppmd_canon.cpp` keeps states sorted by symbol: no swaps, insertion at the sorted position, and rescale compacts in place and adds `a` to the found state.
* Compressed size is **identical**: book1 o6 209,343, o12 209,793; e8m o12 1,937,536. All round-trips are OK.
* The reason is that the coded interval is freq·(range/total) wherever the symbol sits, and `processSymbol1` sets `PrevSuccess = 0` in both branches.
* In Shkarin's original, `PrevSuccess` comes from a state[0] hit. If you restore that, an MPS flag is enough: multi freq ≤ 128, so store freq−1 in 7 bits plus the flag. The other option is a 4-bit MPS index.
* The cost of canonical order is speed: encoding is 9% slower on book1 and 13% on e8m, because frequent symbols are no longer at the front.

**Storage.**
* The 2-level nibble mask (16-bit group mask + one 16-bit mask per used group) costs 2.28× / 2.41× / 2.33× the raw symbol bytes: book1 3.12 MB, e8m 30.5 MB, enwik8 259.8 MB.
* Picking min(list, mask) per context saves only −5.2% / −5.7% / −6.6%.
* 93–94% of contexts have NU ≤ 4, so masks lose on memory.
* Masks pay for **speed** in big contexts: membership and exclusion become 256-bit AND-NOT instead of the `CharMask`/`EscCount` scan, and HasText becomes a mask test.

### 6.6 Revised order for enwik9

1. **Coarse 3-byte text pointers** (§8, k=8): −7.9%, exact, ~0.5% time. This is the cheapest step, so it goes first.
2. **Binary runs → text-anchored records with nibble freqs** (6.3): −12.6%, +0.06%. This also builds the lazy-advance machinery. Its text-terminated records need an exact P, so they keep a 4-byte anchor (§8).
3. **Paged 2-byte ctx successors** (§2.2): −15%.
4. **Text pointers beyond 3 B:**
   * store-once per group (6.2b): exact, −52% of them, about −16% of the tree at 4 B each (less on top of step 1); or
   * an occurrence oracle (6.2a): removes all of them (−32% at 4 B), but it needs a compressed index of the window.
5. **Entropy-coded stats** (6.4): −11%, last.

Implicit `Symbol` (§2.3 item 2) is dropped: coarse pointers give the same saving without window reads in the coding loops.

---

## 7. Per-NumStats descriptor classes

### 7.1 Why it's cheap in this variant

* **The class is free to look up.** `NumStats` already comes from the 64 KB block header, and `MEM_BLK::_pad` is free to serve as a *kind* byte. Class = (NU, kind), so layout dispatch costs nothing beyond the header read that `NumStats()` already does.
* **States can be grouped by successor type.** State order carries no probability information (§6.5), so a context can store its child-context states first and its lazy text-pointer states second. Types are then implied by position, with no type bits.
* **Class changes are just reallocations.** Every context whose class changes is in `SuffCache`, with a known parent slot (`parent_iSuc_cache`), so a class change is realloc + copy exactly like `ExpandContext`.
* **Slot addressing instead of halved byte offsets.** A context pointer becomes `block<<14 | slot`, which allows odd record sizes. A 64 KB block holds at most 13,107 records of the smallest class (5 B, §7.3), so 14 bits suffice.
* **This lifts the enwik9 pointer limit.** With types implied by layout, text and context pointers no longer share one 32-bit value space. Text pointers cover a window up to 4 GB, and context pointers an arena up to 2^18 blocks × 64 KB = 16 GB. The `1 + WinSize + arena/2 ≤ 2^32` limit (6.6 GB with the enwik9 window, §6.1) disappears.

### 7.2 Class profile (o12)

| class | book1 bytes | e8m bytes | enwik8 bytes | share of states that are text pointers (book1/e8m/enwik8) |
|---|---|---|---|---|
| NU=1 | 19.8% | 27.8% | 25.9% | 7.2 / 14.7 / 20.7% (leaves) |
| NU=2 | 34.4% | 28.0% | 26.9% | 77.1 / 74.6 / 74.1% |
| — of which nT=2 (both successors lazy) | 21.5% | 16.6% | 15.8% | 100% |
| NU=3..8 | 34.5% | 30.6% | 32.2% | 53–69% |
| NU=9..32 | 10.5% | 11.8% | 12.8% | 31–51% |
| NU≥33 | 0.7% | 1.8% | 2.2% | ≤ 27% |

**NU=2 with both successors lazy is 59–62% of all NU=2 contexts.** Its count is close to the occ=2 multi contexts found by the suffix automaton (book1 136,953 vs 134,077), i.e. contexts whose string occurred exactly twice and that are never used again (§6.3).

### 7.3 Proposed classes

| class | layout | bytes (now) |
|---|---|---|
| NU=1, leaf (text successor) | `{freq, P}`, symbol = `text[P-1]` | 5 (6) |
| NU=1, inner | `{sym, freq, ptr}`, or chain records (§2.1 / §6.3) | 6 / 2L+4… |
| NU=2..8, kind = nT (+ HasText bit) | `[Esc7│resc1] sym[NU-nT] freq[NU] ctxptr[NU-nT] P[nT]` | 1+6·NU−nT (2+6·NU) |
| NU≥9 | `[Esc7│resc1][nT] sym[NU] freq[NU] ptr[NU]`, context states first | 2+6·NU, unchanged; gains the separate pointer spaces |
| NU≥33, optional | fixed 256-bit mask instead of `sym[]` | 34+5·NU (+32 for a type mask, see below) |
| order-0 | arrays indexed by symbol | – |

The HasText flag (any symbol ≥ 0x40) goes into the kind, so the per-step `CacheNumstatsAndFlags` scan never reads implicit symbols. HasText only changes when states are added or dropped, which reallocates anyway. That gives 42 kinds for NU=2..8, or 84 with the HasText bit.

A mask fixes the state order to symbol order, so "context states first" no longer implies the successor types. The mask class then needs a second 256-bit mask for types (66+5·NU, a gain only for NU > 64), or type bits in the pointers. Either way it is negligible in memory; its value is speed (§7.4).

With coarse text pointers (§8), keep `sym[NU]` and store a 3-byte P instead of using implicit symbols. The saving per text state is the same and the window reads below disappear.

**Savings.** These are exact in probabilities; the bitstream differs only through state order.

| | book1 | e8m | enwik8 |
|---|---|---|---|
| NU 2..8: drop the pad byte | 3.8% | 3.2% | 3.1% |
| NU 2..8: implicit text symbols | 7.1% | 6.0% | 6.0% |
| NU=1 leaves at 5 B | 0.2% | 0.7% | 0.9% |
| fixed mask for NU>32 (without a type mask; upper bound) | 0.02% | 0.08% | 0.10% |
| **sum** | **11.2%** | **10.0%** | **10.1%** |
| with NU=1 chain records (text-implicit) instead of the leaf line | 18.7% | 22.6% | 21.2% |

**Costs.**

* **Window reads for implicit symbols.** Encoder and decoder must read `text[P-1]` for text states while scanning or masking. `ppmd_reads.cpp` counts **2.8 (book1) / 3.2 (e8m) such reads per byte**; the extra ~2 per byte from the HasText scan go away. These reads are random, i.e. cache misses, against ~2 µs/byte today. The speed cost is unmeasured. A cheaper variant uses implicit symbols only in the all-lazy kinds (nT = NU).
* **Materialisation now reallocates.** Turning a text successor into a context in an NU≤8 context adds a symbol byte, so it means a realloc instead of an in-place pointer store. There are 0.3–0.4 materialisations per byte, each touching the suffix-siblings involved.
* **Fixed allocator overhead.** About 330 classes, each with one partial block, is ≤ 22 MB. That is irrelevant at enwik scale; use smaller blocks for rare classes on small files.
* **Code.** Per-class templates are needed for `FindState`, `processSymbol1/2`, `SummFreq`, `rescale` and expand/shrink, dispatched on (NU, kind).

### 7.4 More per-class options

* **NU=2, nT=2** (the "dead" class): 11 B. It is also the natural eviction class under memory pressure. Alternatively, it is never materialised at all if multi contexts are delayed to the 3rd occurrence (§6.3).
* **Big classes:** a mask turns the escape-path `CharMask` loop (set every symbol) into one 256-bit OR. That is a speed win; memory-wise it is negligible.
* **Paged 2-byte context pointers (§2.2)** combine with all of the above. Inside a class layout they are just a narrower `ctxptr[]`.

---

## 8. Coarse text pointers: store (P−1)>>k, rescan the window block

**Idea.** Store only the block index of the end of the context pattern (context string + symbol). When P is needed, scan that 2^k-byte block of the window for the first position where the pattern ends, and take P = that position + 1.

**Why it is exact.** P is the end of the *first* occurrence of the pattern in the whole text (§6.2, 99.99%), so no earlier match can exist in its block. In the 0.01% case where P isn't the first occurrence, the scan returns an earlier one. Encoder and decoder do the same scan, so the result is still decodable, just a slightly different model.

**When P is needed.** Only in `CreateSuccessors`, at materialisation (0.32–0.40 times per byte). At that moment the pattern is the last order+1 bytes of the current history, so it is available, and the scan is anchored on the block that the `sym1 = text[P]` read touches anyway.
* The other use of P, comparing lower-order siblings' P with `iUpBranch`, becomes "both lazy". That was already shown to give identical output (§6.2a).
* New lazy states store `(pText-1)>>k`; the child continuation P+1 is computed from the recovered P.

**Emulation** (`ppmd_coarse.cpp`, env `CZK=k`). It keeps the exact P, but uses the scanned P and counts agreement. The scan is a naive `memchr` on the last byte plus `memcmp` of the rest.

| data (o12) | k | materialisations | recovered P == stored P | avg bytes scanned | encode time | output |
|---|---|---|---|---|---|---|
| book1 | 8 / 12 / 16 | 309,535 | 100% | 129 / 2,028 / 30,524 | – | identical |
| e8m | 8 / 12 / 13 | 2,683,153 | 100% | 129 / 2,050 / 4,096 | – | identical |
| e32m (first 32 MiB of enwik8) | base | – | – | – | 66.8 s (65.5 / 68.0) | – |
| e32m | 8 | 9,502,142 | 100% | 128 | 70.2 s (+5%) | identical |
| e32m | 10 | 9,502,142 | 100% | 513 | 70.5 s (+5.5%) | identical |
| e32m | 12 | 9,502,142 | 100% | 2,048 | 79.1 s (+18%) | identical |
| e32m | 13 | 9,502,142 | 100% | 4,095 | 90.9 s (+36%) | identical |

These are whole-run times, so they include run-to-run noise of about ±2%, and the scanner here is the naive one. The emulation still carries the full-size pointers, so any cache benefit from the smaller tree isn't counted. Timing only the scan (§8.1) puts the naive k=8 scan at ~3% and the SIMD scan with prefetch at ~0.5%; most of the +5% above is noise.

**Pointer widths.** A coarse pointer needs ceil(log2 W) − k bits, for window size W.

| target | k | width | cost | saving, enwik8 o12 |
|---|---|---|---|---|
| any window ≤ 4 GB, incl. enwik9 | 8 | 3 B (24 bits) | ~0.5% (SIMD + prefetch, §8.1) | −56.9 MB (−7.9%) |
| enwik8 | 11 | 2 B | naive scan ~+10% (whole-run, between k=10 and k=12) | −113.8 MB (−15.9%) |
| enwik9 | 14 | 2 B | avg 8 KB scan; naive extrapolation ~+70% | – |

At k ≥ 11 the scan is no longer latency-bound like the 256-byte case in §8.1; it streams many lines per call. SIMD cost at k=11..14 is unmeasured.

**Consequences for the earlier options.**

* **The unified 4-byte state works for enwik9 after all.** Text state `{sym, freq, P16}` and child-context state `{sym, freq, ptr16 paged}`, with the type implied by the §7 grouping. The cost is the k=14 scan.
* **It beats implicit symbols (§2.3 item 2, §7).** Same byte saving, but the symbol stays stored, so there are no window reads in the coding/masking loops (~3 per byte there). Window reads happen only at materialisation, where one already occurs. The two don't combine, because an implicit symbol needs the exact P.
* **Text-terminated chain records** with symbols taken from `text[P-L..P-1]` (§2.1) need the exact P on every access. Keep explicit symbols, or an exact anchor, in chain records.


### 8.1 SIMD block scan for k = 8 (3-byte text pointers)

**Setup.** Scanners in `ppmd_coarse.cpp`, selected with env `CZSCAN`.
* Only the scan is timed (rdtsc), in TSC cycles at 2.1 GHz, including ~25 cycles of rdtsc overhead.
* Machine: Xeon with AVX2 + AVX-512BW, L2 4 MB, L3 260 MB. All test windows are L3-resident; enwik9's won't be.
* Output was byte-identical to baseline for every scanner.

| `CZSCAN` | method | false candidates / call (book1 / e8m) |
|---|---|---|
| 9 | reference: exact pointer, read `text[P-1]` only | 0 |
| 10 | reference: touch every line of the block, no compare | 0 |
| 0 | `memchr` on the last byte, then `memcmp` | 8.1 / 6.8 |
| 1 | AVX2 first+last byte filter (Muła), `memcmp` verify | 0.62 / 0.48 |
| 2 | AVX2 AND of all min(L,16) byte compares; no verify for L ≤ 16 | 0 |
| 3 / 4 | AVX-512 versions of 1 / 2 | |
| 5 | AVX2 first+last filter, verify the last 16 bytes with one xmm compare | 0.62 / 0.48 |
| 6 | AVX-512 version of 5 | |
| 7 / 8 | whole 256-byte block in 8 (AVX2) / 4 (AVX-512) fixed vector steps, then the first candidate | |

`CZPF=1` prefetches the block (5 lines) as soon as `FoundState` is final and lazy, i.e. before `UpdateSuffixFreq` and `refresh_parent_cache` run.

**Cycles per materialisation** (minimum over runs):

| `CZSCAN` | book1 | e8m | e32m |
|---|---|---|---|
| 9 exact pointer | 34 | 51 | 61 |
| 9 exact pointer + prefetch | – | 46 | 47 |
| 10 touch all lines | – | 53 | – |
| 0 memchr | 296 | 411 | 514 |
| 1 | 155 | 291 | – |
| 2 | 204 | – | – |
| 3 | 176 | – | – |
| 4 | 206 | – | – |
| 5 | 137 | 239 | 333 |
| 6 | 147 | 237 | – |
| 7 | 131 | 244 | 323 |
| 8 | 151 | – | – |
| **5 + prefetch** | – | **113** | **120** |
| 7 + prefetch | – | – | 138 |

**Findings.**

* **A 2-byte filter beats a 16-byte compare at every position.** The first+last filter leaves only ~0.5 false candidates per call. Comparing all min(L,16) bytes at every position costs ~8× the loads and compares for no gain. The 16 bytes are best spent as one xmm compare that verifies a candidate (scanner 5): −20 to −50 cycles vs `memcmp` verify and −70 vs the full AND.
* **AVX-512 doesn't help at 256-byte blocks.** The scan isn't ALU-bound.
* **The remaining cost was exposed memory latency, not compares.**
  * Touching all 5 lines with independent loads costs the same as touching one (53 vs 51–54 cycles).
  * The scan's dependent loop exits serialise those misses.
  * Prefetching at `FoundState` time cuts scanner 5 from 239 → 113 (e8m) and 333 → 120 (e32m).
* **Net cost:** (120 − 47) cycles × 0.28–0.32 materialisations/byte ≈ 22 TSC cycles ≈ 10 ns per byte, against ~2.0 µs per byte of encode time, i.e. **~0.5%**. Whole-run times stay within run-to-run noise (e8m 16.7–18.0 s, e32m 66.6–70.6 s).
* **Saving:** k = 8 gives 3-byte text pointers for any window up to 4 GB. enwik9 uses 22 of the 24 bits, leaving 2 spare bits. That is −1 byte per text state, −57 MB on enwik8 at o12 (−7.9%).
* **enwik9 caveat:** the 1 GB window is DRAM-resident, so the block read is a DRAM miss either way; the exact-pointer version pays it too, for `sym1 = text[P]`. The prefetch matters more there, and it lands well ahead of `CreateSuccessors` because `refresh_parent_cache` runs in between.

---

## Appendix: reproducing the numbers

* `stats.inc` is included inside `struct Model` of `ppmd.cpp`, followed by `const qword Model::PG[2] = {65536, 131072};`. Call `C.DumpStats(stderr)` after `C.do_process()` (encoder only). `ppmd_stat.cpp` is exactly that.
  * Build: `g++ -O2 -Wno-invalid-offsetof -o ppmd_stat ppmd_stat.cpp`
  * Run: `./ppmd_stat c book1 /dev/null 12`
  * It reports the census, chains, freq histograms, pointer-delta and age histograms, the static paging partition with FFD packing, suffix redundancy, and text-pointer multiplicity.
* `sam.cpp`: `g++ -O2 -o sam sam.cpp && ./sam book1`. It builds a suffix automaton and prints, per D, substrings occ≥2, (substring, next) pairs, endpos classes and class transitions.
* `stats2.inc` (census part 2) adds: symbol-storage costs (raw / nibble masks / enumerative), freq and `EscFreq` entropies, the text-state vs suffix-state relation, duplicate-group structure, and with `CENSUS_SAM=1` the first-occurrence check, occurrence counts and first-occurrence deltas (it builds a suffix automaton, so use it on ≤ ~10 MB). `CENSUS_SKIP1=1` skips part 1.
* Experiments (each is `ppmd.cpp` plus a small patch; decode needs the same order argument as encode):
  * `ppmd_canon.cpp`: symbol-sorted states.
  * `ppmd_vbin.cpp`: binary-freq variants, `-DVMODE=0..3`.
  * `ppmd_mf.cpp`: matchfinder in place of the stored text pointer. Env: `MFBITS=B` sets the table size; `MFMISS=0` falls back to the stored P on a miss, `MFMISS=1` is the naive degrade.
  * `ppmd_reads.cpp`: counts per-byte symbol reads of text-pointer states.
  * `ppmd_coarse.cpp`: coarse text pointers recovered by a block scan. Env: `CZK=k` sets the block size; `CZSCAN=0..10` selects the scanner (§8.1); `CZPF=1` prefetches the block at `FoundState`. Build with `-O2`; the AVX2 / AVX-512 kernels use target attributes.
  * `cz_bench.sh <file>` times k = base/8/10/12/13. `cz_bench2.sh <file> <runs> <scanners…>` times scanners at k = 8.
* `stats3.inc` (census part 3): per-NU class profile and layout savings. Skip parts 1–2 with `CENSUS_SKIP1=1 CENSUS_SKIP2=1`.
* The paging numbers are a static partition of the *final* tree; dynamic behaviour is unmeasured. Highly repetitive data (source, logs) hasn't been censused yet; do that before deciding on 3.2.
