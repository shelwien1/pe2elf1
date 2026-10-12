# The compression algorithm of `vbn2.cpp`

## What it is

**An LZP coder: LZ77 with the match position predicted instead of sent.** In
the classification Fenwick gave the 1994–96 newcomers (block sorting, LZP,
ACB), this coder is of LZP's kind, *Shannon type-1 symbol ranking*:

- **Prediction.** It finds the earlier position whose preceding context
  matches the current one longest, and predicts that the data continues as it
  did there.
- **Coding.** It codes how many of the next bytes that prediction gets right,
  a match length 0..255 (a run of correct predictions). When the prediction
  fails at once, it codes the byte itself, with the failed prediction
  excluded.
- **No offsets.** No position, offset or dictionary reference is ever
  transmitted; the decoder repeats the search.

Two things are taken further than in Bloom's LZP:

- **The context search is context sorting.** LZP hashes a fixed-order
  context. Here the longest match comes from a binary trie over the bits that
  precede each position: the history's contexts in sorted order, as in the
  Burrows–Wheeler transform or Yokoo's context-sorting compressor. The
  context order is unbounded and found per position.
- **The final coder is a PPM-style blend.** The match length and the literal
  are coded arithmetically, with probabilities blended from up to 95
  neighbouring contexts in that order, each weighted by how many bits of
  context it shares with the current one. Order-0 and order-1 statistics are
  blended in too, and failed predictions are excluded, as PPM excludes.

So, although it descends from LZ77 through LZP, it is neither an
LZ77/LZ78/LZFG dictionary coder nor PPM:

- **Not a dictionary coder.** Nothing in the code refers back into the
  history: no offsets, no phrase indices, no trie nodes (LZFG codes
  references into a trie of earlier phrases; here the trie only serves the
  prediction, and the decoder keeps its own copy).
- **Not PPM proper.** It has no escapes and no context orders in the code.
  Bytes the prediction gets right go as one length. Only the bytes it misses
  are coded one by one, from a PPM-like blended distribution.

The program it comes from is ACB 2.00c, George Buyanovsky's *Associative
Coder of Buyanovsky* (1997); VBN 1.001 is ACB renamed (see
[Provenance](#provenance)). Descriptions of earlier ACB versions have the
coder say which neighbouring context continues best, as well as how far:
Fenwick calls that "phrase ranking". ACB 2.00c always follows the
best-matching context and codes only how far, which is LZP; the neighbours
only shape the probabilities.

## How well it does, and where that comes from

`book1` of the Calgary corpus (`input.txt`, 768,771 bytes):

| | bytes | bits/byte |
|---|---|---|
| **vbn2** (ACB 2.00c, MAX mode) | **222,682** | **2.32** |
| ACB 1.29b, as published (Fenwick 1997) | | 2.32 |
| LZP4, Bloom's best LZP (PPM-style literals), as published | | 2.35 |
| PPMd var. H, order 6 (7-Zip) | ≈ 209,800 | 2.18 |
| `bzip2 -9` (block sorting) | 232,598 | 2.42 |
| `xz -9e` (LZ77 + context-coded literals) | 261,376 | 2.72 |
| `gzip -9` (LZ77) | 312,285 | 3.25 |
| vbn2, best context only: plain unbounded-order LZP, order-0 literals | 418,430 | 4.35 |
| vbn2, no context blending for literals | 297,337 | 3.09 |

The last two rows are vbn2 with parts of the final coder switched off, still
decodable. Without the blend, the LZP skeleton alone does worse than `gzip`.
The blend is what makes it a PPM-class compressor: within a few percent of
PPMd and ahead of block sorting.

Measured inside vbn2 on `book1`:
- the prediction is right for the first byte 37% of the time;
- runs average 2.2 bytes;
- the best match averages 5.3 bytes of context;
- 63% of the code is spent on literals (3.4 bits each), 37% on the lengths.

On `vbn.exe.c`, a 371 KB decompiled C file with long repeats:
- runs average 7.8 bytes;
- lengths take half the code;
- vbn2 makes 41,623 bytes, against 40,396 for PPMd order 16 and 46,011 for
  `bzip2 -9`.

## Contents

- [What it is](#what-it-is)
- [How well it does, and where that comes from](#how-well-it-does-and-where-that-comes-from)
- [Provenance](#provenance)
- [The stream](#the-stream)
- [The coding loop](#the-coding-loop)
- [The window](#the-window)
- [The context tree](#the-context-tree)
- [The search: the associative set](#the-search-the-associative-set)
- [Coding a match length](#coding-a-match-length)
- [Coding a literal](#coding-a-literal)
- [The arithmetic coder](#the-arithmetic-coder)
- [Constants](#constants)
- [Quirks kept for bit-exactness](#quirks-kept-for-bit-exactness)
- [Relation to the published ACB descriptions](#relation-to-the-published-acb-descriptions)
- [Sources](#sources)

## Provenance

VBN 1.001 is ACB 2.00c under another name. `vbn.exe`'s image is that of
`ACB.EXE` 2.00c (Internet Archive item `acb-ver-2.00c`), rebased by
0x10000000. Apart from the rebase, they differ only in:
- the DOS calls the PE repackaging redirects;
- the program's name in the strings;
- the replaced banner and blanked shareware registration text;
- a disabled self-check of that banner.

Every function vbn2 was decompiled from is in `ACB.EXE` unchanged. The
arithmetic coder and the adaptive frequency tables go back further, to the
program Buyanovsky published with his 1994 article "Associative coding"
(`WriteEnd` there is `ArithCoder::flush` here).

## The stream

```
offset 0   uint32 little-endian   n, the size of the data
offset 4   ...                    the arithmetic code for n bytes
```

`vbn2 c` writes this. The code is exactly what ACB writes after its 3-byte
archive header, for the solid stream of the archive's file table followed by
its files. ACB's archive layer (header, title, volumes, error protection,
"trainable channel" context files) is outside the compressor and not in
`vbn2`.

## The coding loop

Data is coded byte by byte; positions are counted in bits (8 per byte) because
the context tree works on bits. `code_window` runs this for every position of
the window:

```
excluded = -1                      the byte known not to come next, or -1
while pos < end of data in window:
    search(pos, exclude = excluded)              -> candidates, weighted
    move the most recent of the top-weight candidates to the front
    len = code_match(pos)          0..255; sets `excluded` to the byte the
                                   best candidate predicted where it failed
    if len > 0:
        copy len bytes (decoder) ; register some of them in the tree
        pos += len                 the next byte is known not to be `excluded`
    else:
        code_literal(pos, excluded); register pos in the tree
        excluded = -1
        pos += 1
```

A match of length *L* < its maximum says the next byte is not the one the
candidate predicted, so that byte is excluded at the next position, from the
search and from the literal coding. A length of 0 says the very first byte
differs from the best candidate's prediction, and the literal that follows is
coded with that byte excluded.

Every 8 KB of code goes to the output (`ArithCoder::next_block`), and the
window slides when it is full.

## The window

The data lives in a window of **0x198000 bytes** (1,671,168, "1671 Kb"). When
coding reaches its end and data remains, it slides by **0x66000** bytes
(417,792), keeping the last **0x132000** (1,253,376). The decoder writes the
bytes that leave. The context tree is then rebuilt from scratch: every kept
position whose bit in `registered` is set (positions the coder had
registered) is inserted again, oldest first (`tree_rebuild`).

## The context tree

**Contexts.** The context of a position *p* is the data before *p*, read
backwards as a bit string: the byte before *p* first, each byte from its top
bit down. Sorting all positions by their contexts gives ACB's associative
dictionary. Buyanovsky's early programs kept it as a sorted array of pointers,
like a Burrows–Wheeler suffix array of the reversed text; ACB 2.00c keeps it
as a binary trie.

**Shape.** Nodes are pairs of 32-bit slots in one arena (`tree`; node *n* is
`tree[2n]`, `tree[2n+1]`, node 0 the root). From a node, a context's next bit
picks a slot: a 1 bit takes slot 0, a 0 bit slot 1. A slot holds a node
number (> 0) or a leaf: minus the bit position it registers (0 is an empty
slot, or position 0). The leaves, read left to right, are positions in the
order of their contexts, as far as the tree tells them apart.

**Insertion** (`tree_insert`, `tree_add`) is lazy. A position descends along
its context to the first slot holding a leaf. If that slot is within
64·log2(*nodes* + 32) levels of the root, the leaf is replaced by a new node
whose two slots both hold the new position. The old position drops out of the
tree, and the branch has grown one bit deeper. Deeper than that limit, no node
is made; a leaf more than 0x800 bits (256 bytes) old is replaced by the new
position, a younger one is kept.

So each path through the trie remembers its most recent occurrence, and the
trie grows one bit along a context each time that context recurs, within a
depth limit that grows with the tree: 1,280 bits (160 bytes) once it has
2^20 nodes. One position makes at most one node, and the tree starts over when the
window slides, so 0x198001 nodes (12.75 MiB) always suffice. `tree_updates`
counts the changes (nodes made beside an existing node, leaves renewed); the
search's thresholds grow with its logarithm.

**What gets registered.** A literal registers its position. A match of *L*
bytes registers its first `min(bsr(1.5·L + 1) + 1, L)` positions, a few,
logarithmic in *L*. It registers none when it ran to its maximum length and
the search found a strong context (`search_gain` ≥ 42), so long confident
repeats don't flood the tree.

## The search: the associative set

`search` (vbn.exe has two functions, `search_tree` and `search_excluding`)
finds the candidates for the current position.

1. **Descent.** From the root along the current context, recording the path,
   to the first slot that holds a leaf (or the data runs out). The depth
   reached is how many bits the deepest earlier context agrees with this one.
2. **Threshold.** A context must agree for more than *K* bits to count, *K*
   being Buyanovsky's "value of K is a separate big theme". It starts at
   `0.54·log2(tree_updates + 4)` (`276·bsr(·)/512`), a noise floor that rises
   with the amount of data. If the descent got deeper than *K*,
   `search_gain = 16·(depth − K)/log2(nodes + 7·updates)` measures by how much,
   and *K* rises to `((depth + depth/8) + 2K)/4`, so a deep match also demands
   deep candidates.
3. **Walk back up.** The leaf the descent found is the first candidate. Then,
   level by level up the path while the level is above *K*, the leaves under
   the *other* slot at that level join the set, left to right (`walk_cands`).
   Each of these is a position whose context agrees with the current one for
   exactly that many bits. A node's right leaf is taken only when its left
   child is a node; a fresh node holds one position twice.
4. **Weights.** A candidate found at level *d* weighs `d − K`, shifted left
   by a shift that starts at 4 and runs down: the first candidate takes the
   shift (×16) and lowers it to 3; each subtree takes half the shift and
   lowers it by one (×2, ×2, ×1, ×1, … after a first candidate); a single
   leaf takes the shift as it stands. Deep, specific contexts dominate.
5. **Limits.** The search stops as soon as it has `max_cands` candidates: 95
   in MAX mode (`u`), 55 in NORMAL (`b`), 31 in FAST (`B`). `max_cands` is
   the only mode setting the coder sees; it also picks the literal mixing's
   `mode_shift` (4, 6, 10).
6. **Exclusion.** After a match failed on byte *x*, candidates predicting *x*
   (the byte at their position) are left out of the set.
7. **Giving up.** Without an exclusion, if literals have been costing more
   than about 8 bits over the last 512 (`lit_cost9` > 0xFFE), the search
   stops after the descent with no candidates. Incompressible data is then
   coded as plain order-0 literals, quickly.

Afterwards `cand_floor` is a quarter of the weakest candidate's weight, and
among the candidates sharing the top weight the most recent position is put
first. That candidate is the **best candidate**: its position is the match
source.

## Coding a match length

`code_match` codes how many bytes from the current position equal the bytes
that followed the best candidate's position.

- With no candidates, nothing is coded: a literal follows, and the
  confidence trackers (`search_gain`, `literal_run`, `len_avg`) reset.
- **The maximum** is `min(255, the candidate's distance, bytes left)`, and
  lengths above it are removed from the model for this symbol.
- **The length model** has a count for each length 0..255. Each coded length
  adds 5 to its count for the next 4096 lengths (a ring of the last 4096
  lengths; the adds are 16-bit). Before any data the ring holds a prior of
  4,048 synthetic lengths, 2,662 of them length 0, then 633 of length 1, 321
  of 2, … one each of lengths 19–22. Every count also starts at 1.
- **No match is made less likely when literals are going well.** For this
  symbol, the count of length 0 is scaled by `5/(k + k/16)`, with
  `k = 9·literal_run + search_gain + lit_threshold/4 + 1`.
- **The other candidates vote.** Each other candidate is compared with the
  best one: if its predicted byte equals the best one's, its vote is the
  number of bytes their continuations agree on (capped as the match is),
  otherwise 0. Votes are grouped by length; a group weighs its candidates'
  weights above `cand_floor`, plus (*n* − *i*)/2 for its first member, *i*
  being that member's rank among the *n* candidates. The group for length 0
  is reweighed: by `len_avg` against `lit_threshold`, down (×3/4, ×21/32)
  when the best candidate's byte is what `next_byte` (below) predicts, and by
  `ctx_count` of that byte, a pair count indexed by the byte alone (a quirk):
  +1/512 at 0, then −1/32, −5/32, −3/16 (3 to 5) and −1/4 (6 or more). Each
  group's weight, scaled to 4, 6 or 9 times the model's total (more as
  literals get cheaper: `lit_cost8` below 0x44C, below 0x384) and to a
  confidence term (`n_cands + 4·search_gain + lit_cost8/64` over a term in
  `len_avg` and `lit_cost8`), plus `search_gain`, is added to its length's
  count for this one symbol. A length where other candidates part ways with the best one
  becomes more likely.
- The length is coded, the model restored, and the length counted
  (`len_model_add`). `len_avg`, the average length ×128, follows with weight
  1/64.

The encoder finds the length by comparing; the decoder decodes it and copies
that many bytes from the candidate's position. If the length is below the
maximum, the byte the candidate had there is the excluded byte.

## Coding a literal

`code_literal` codes the byte at a position where the match length was 0.

**The literal model** is 256 counts, kept sorted by count (largest first) with
an index from byte to entry (`lit_slot`), so the arithmetic coder's linear
search finds frequent bytes first.

- Each literal adds 16 to its byte's count for the next 363 literals (a
  ring).
- Every count also carries a uniform increment `lit_inc` that grows as
  literals get expensive (1, 2, 4, 8, 16, 32, 128, 1024 as `lit_cost8` passes
  0x2EC, 0x37F, 0x489, 0x56F, 0x63F, 0x70F, 0x75C, about 2.9 to 7.4 bits a
  literal). Hard data gets a flatter model.
- Sorting is lazy: every `lit_resort` literals, about the average literal
  cost in bits plus one, the two counts that just changed bubble into place.

**Costs.** `lit_cost8` … `lit_cost11` are the bits a literal takes, ×2^N,
averaged over about 2^N literals (N = 8…11). They drive most of the
adaptivity:
- `lit_threshold`, how much the candidates count for, is `54 + gain/16 −
  log2(lit_cost8/8 + 1) − Σ 1.5·(average bits + 1)` over the four averages,
  floored at 0;
- at 0, with a weak search, the candidates are dropped and the longer
  averages restart.

**Exclusions.** The byte the best candidate predicted (known wrong, since the
match length was 0) and the byte the search excluded both get count 0 while
this literal is coded.

**The candidates vote for bytes.** Each candidate predicts the byte at its
position; the votes are grouped by byte.
- A group weighs its candidates' weights above `cand_floor`, plus a bonus for
  its first member from how often the pair *(byte before, this byte)* came up
  among the last 1024 literals (`ctx_count`) and from the search's gain
  (`pair_bonus`): the member's weight times 1/8, 2, 3, 4, 5 (4 to 6 times)
  or 8 (7 or more), or after a search that gained 0x13 or more, 1/16, 1/2,
  1, 3/2, 2 or 5/2 (5 or more).
- Groups for the bytes the last-successor tables predict are boosted: ×7/4
  for `next_byte[previous byte]`, the byte that followed the previous byte
  last time, and ×3/2 for `next_byte2[byte two back]`, the byte that came two
  after the byte two back last time.
- The total vote is scaled to a share of the literal model that grows with
  its total, with the number of candidates that agree with another
  (candidates minus groups), with `lit_threshold` and with the search's gain.
  Each byte's scaled vote is added to its count for this one symbol.
- While literals cost under about 6.8 bits (`lit_cost8` < 0x6C9), a voted
  byte also gains 64 per recent occurrence of its pair, and, when
  `literal_run` is 0 (the last literal's pair was new), a quarter of its own
  count.

The byte is coded, the cost averages updated, and the model restored. The
pair *(byte before, byte)* is counted for the next 1024 literals (saturating
at 255), `literal_run` (literals in a row whose pair had come up before)
updated, and the model takes its +16.

## The arithmetic coder

`ArithCoder` codes a symbol as a share of its interval: its count, after the
counts *below* it, out of a total. The models keep their statistics as
frequency lists of `{count, symbol}` pairs and look a symbol's share up in
them (`Codec::encode_symbol` and `decode_symbol`); the coder sees only the
three numbers. In `vbn.exe` both halves are one function, `code_symbol`.

- **Interval.** 32-bit `low` and `high`. A symbol whose list entries before it
  sum to *below*, and *above* with its own count, narrows the interval to
  `high = low + above·size/total − 1`, `low += below·size/total`, with
  `size = high − low + 1` (up to 2^32) and the products in 64 bits.
- **Renormalisation.** The leading bits that `low` and `high` agree on are
  shifted out into the code buffer one at a time (written by toggling bits of
  a zeroed buffer). `pending`, 32 minus the bits last shifted out, tells the
  decoder how many fresh bits to read next and bounds `flush`'s tail.
- **No carry, no underflow handling.** When the interval gets narrower than
  twice the total (`2·total > high − low`), the code is ended: `flush`
  writes the shortest tail that keeps every continuation inside the interval
  (`0111…` or `1000…`), and coding starts afresh on the full interval. The
  same happens once at the end of the data. This is Buyanovsky's `WriteEnd` of
  1994.
- **Decoding.** `value` holds the 32 bits of code that line up with
  `low`; each symbol first reads the bits that renormalisation shifted out.
  The coder works out the count the code points at,
  `((value − low + 1)·total − 1)/size` (`decode_target`); the model finds the
  first symbol whose cumulative count exceeds it, and the coder narrows the
  interval to that symbol's share as the encoder did (`decode`).
- **Lists.** The models search their lists from the start. The literal
  model is sorted by count; the length model is in length order, which
  favours short lengths, and its tail above the maximum is cut off by
  lowering the total.

## Constants

The names are those of `vbn2.cpp`'s `Codec`, which keeps them together.

| What | Value | Name |
|---|---|---|
| window, slide, kept | 0x198000, 0x66000, 0x132000 bytes | `WINDOW`, `SLIDE`, `KEEP` |
| data per stream | ≤ 0x1FFFFFFF bytes (512 MB): positions count bits, in 32 bits | `MAX_SIZE` |
| context tree | ≤ 0x198001 nodes of two 32-bit slots | |
| node-making depth limit | 64·log2(nodes + 32) bits | `GROW_SHIFT`, `GROW_BIAS` |
| leaf renewal age (deep slots) | 0x800 bits (256 bytes) | `RENEW_AGE` |
| descent path | ≤ 1284 slots | `PATH_SLOTS` |
| candidates (MAX/NORMAL/FAST) | 95 / 55 / 31 | `max_cands` |
| candidate threshold *K*, initial | 276·log2(tree_updates + 4)/512 bits | `AVG_START`, `AVG_BIAS` |
| first candidate's weight | (depth − *K*) << 4 | `WEIGHT_SHIFT` |
| search gain | 16·(depth − *K*)/log2(nodes + 7·tree_updates) | `GAIN_MUL`, `GAIN_UPDATES` |
| no search (without an exclusion) | `lit_cost9` > 0xFFE (8 bits) | `GIVE_UP_COST9` |
| candidate floor | weakest weight / 4, / 8 for a literal | `FLOOR_SHIFT`, `LIT_FLOOR_SHIFT` |
| a match's positions registered | search gain < 0x2A, or after an exclusion | `INSERT_GAIN` |
| match length | 0..255 (0 = none) | `MAX_LEN` |
| length model | 256 counts from 1, +5 per length, over the last 4096 lengths | `LEN_START`, `LEN_STEP`, `LEN_RING` |
| `len_avg` | ×128, following each length by 1/64 | `LEN_AVG_RATE` |
| length 0's count | ×5/(k + k/16), k = 9·`literal_run` + gain + `lit_threshold`/4 + 1 | `NOMATCH_MUL`, `NOMATCH_RUN` |
| no-match group, `next_byte` agreeing | ×3/4 + 1, and ×7/8 + 1 when twice | `AGREE_QUARTERS`, `AGREE2_EIGHTHS` |
| a length group's rank bonus | (n − i)/2 | `RANK_SHIFT` |
| length groups' weight | 4, 6 or 9 × total as `lit_cost8` < 0x44C, < 0x384 | `LEN_COST8_MID`, `LEN_COST8_LOW` |
| length groups' share | (n + 4·gain + `lit_cost8`/64)·weight / ((`len_avg` + 1935 − `lit_cost8`)/8 + 1), scaled with 6 bits to spare | `CONF_GAIN`, `CONF_BIAS`, `SCALE_BITS` |
| literal model | 256 counts, +16 per literal over the last 363, plus `lit_inc` (1..1024, from 8) | `LIT_RECENT`, `LIT_RING1`, `LIT_START` |
| pair counts | `ctx_count[256·prev + byte]` over the last 1024 literals, ≤ 255 | `LIT_RING2` |
| pair bonus's second row | search gain ≥ 0x13 | `PAIR_GAIN` |
| pair counts in the literal model | below `lit_cost8` 0x6C9 (6.8 bits): +64 per pair seen, and +¼ of the count while `literal_run` is 0 | `PAIR_COST8`, `PAIR_COUNT_SHIFT`, `NO_RUN_SHIFT` |
| successor boosts | ×7/4 for `next_byte`'s byte, ×3/2 for `next_byte2`'s | `NEXT_QUARTERS`, `NEXT2_HALVES` |
| byte groups' share | grows with (3·`lit_threshold` + 5·1.25·gain)/8, and ×2 per step of (48 − `lit_threshold`)/8 below 4 (÷2 above) | `BLEND_THRESHOLD`, `BLEND_GAIN`, `THRESHOLD_TOP`, `BLEND_PIVOT` |
| `lit_threshold` | 54 + gain/16 − log2(`lit_cost8`/8 + 1) − the costs | `THRESHOLD_BASE` |
| literal cost averages | 4, over ~256/512/1024/2048 literals, from 5, 6, 7, 8 bits | `COST8_START` … `COST11_START` |
| long averages restart | at 8 bits, once `lit_cost8` ≥ 0x70F (7.1 bits), or at `lit_threshold` 0 after a gain ≤ 4 (dropping the candidates) | `RESTART_BITS`, `RESTART_COST8`, `DROP_GAIN` |
| arithmetic coder | 32-bit interval, bitwise output, 8 KB code buffer + 16 bytes, flush when range < 2·total | `BLOCK`, `SPILL` |
| decoder's first read | up to 2 bytes of code per byte of the first window | `FIRST_CODE` |

## Quirks kept for bit-exactness

`vbn2` must write ACB's code bit for bit, so it keeps what ACB does even where
that looks unintended. These are commented in the source.

- **The decoder's `sbb`.** ACB subtracts the 1 in `(value − low + 1)·total − 1`
  with `sbb eax,1` straight after the `mul`, so the `mul`'s carry (the product
  needing 64 bits) comes off as well. This can matter only when the code
  value sits exactly on a symbol's lower bound and the product is a multiple
  of the interval's size. Instrumented over 11.5 million decoded symbols (42
  inputs), it never changed even the computed count.
- **`ctx_count` indexed by the byte alone** when weighing the no-match group
  in `code_match`, where everywhere else it is indexed by the pair: it reads
  the count of the pair (0, byte).
- **One candidate less.** When candidates predicting `excluded` are skipped,
  `n_cands` drops by one however many were skipped.
- **16-bit count updates** in the length model (`add word ptr`).
- **Shift counts mod 32** in the scaling arithmetic (`shld`/`shrd` with a
  count that can exceed 31).
- **Bits toggled when decoding.** The renormalisation and the flush tail
  toggle buffer bits in the decoder too. It does no harm, since those bits
  lie before the 32 bits `value` holds when it is next used.

## Relation to the published ACB descriptions

Buyanovsky never published ACB 2.00c's details ("It is all what you can find.
I have never published any details", 1999). What he did publish matches the
code:

- **1994, "Associative coding", *Monitor* 8/94, and its program (ACB 1.02c).**
  - Contexts and contents are "pre-history" and "post-history".
  - The current context is located in a sorted table of pointers.
  - Its neighbours in both directions, agreeing for at least about `log2(N)`
    bits, form a "funnel of analogies" weighted by the length of agreement.
  - The coder codes which member of the funnel continues best (its rank),
    how far it continues, and the final symbol, with the members' next bytes
    mixed into the symbol's probabilities.
  - Its arithmetic coder, frequency tables and rings are those above.
- **1996, comp.compression.** Buyanovsky summarised the idea in seven steps:
  a sorted table of pointers "by analogy with BWT"; an associative set of the
  neighbours agreeing with the current line "not less than by K bits"; each
  weighted by its length of agreement; then the set sorted by the lines
  directed into the future, and the position of the coded line among them
  coded. He added: "The variant implemented in ACB.EXE differs very much from
  the described above but the general idea is the same."
- **ACB 2.00c (1997), this code.** It keeps the weighted associative set but
  changes the rest:
  - a binary trie over context bits replaces the sorted array;
  - no rank is coded: the best-weighted context is the match source (the
    coder becomes LZP), and the other members of the set shape the length
    and literal probabilities instead;
  - the 1.6 MB window serves both the archiver and its "trainable channel"
    (priming the coder with earlier data).

Fenwick (1997) put ACB together with block sorting and Bloom's LZP in one
class, symbol ranking, after Shannon's 1951 prediction experiments. A
predictor ranks what may come next from the context, and the coder codes how
the actual data relates to that ranking. ACB 2.00c's coder is the simplest,
type-1 form: is the top prediction right, and for how many bytes. The rest of
the ranking is kept for the probabilities. (Buyanovsky himself said ACB "can
be classified" under DMC, whose models also grow a step each time a context
recurs, as the trie here does.)

## Sources

- ACB 2.00c, George Buyanovsky, 1997: Internet Archive,
  <https://archive.org/details/acb-ver-2.00c> (`ACB.EXE`, `README.TXT`).
- G. Buyanovsky, "Associative coding", *Monitor* 8/94, with the ACB 1.02c
  program: `buianov.rar` and `sources/cm/acb102c.rar` on
  <https://www.compression.ru/download/cm.html>; Vadim Yoockin's 1997
  summary, `acb_yoo.rar`, on the same page.
- S. Lambert, M. Cohn, F. Pettena, "ACB reference material", `acb_alg.rar` on
  the same page. It includes Buyanovsky's 1996 comp.compression posts, Leonid
  Broukhis's notes, and Buyanovsky's 1999 letter to Lambert.
- S. Lambert, *Implementing Associative Coder of Buyanovsky (ACB) data
  compression*, MSc thesis, Montana State University, 1999,
  <https://scholarworks.montana.edu/items/bab4ae38-1145-4f77-ba3e-276537dae077>.
- C. Bloom, "LZP: a new data compression algorithm", *DCC* 1996.
- P. M. Fenwick, "Symbol ranking text compressors: review and
  implementation", 1997,
  <https://www.cs.auckland.ac.nz/~peter-f/FTPfiles/1997%20Sym%20Rank%20Compression.pdf>:
  block sorting, LZP and ACB as symbol-ranking compressors; LZP as "a direct
  implementation of a 'Shannon type 1' compressor"; the published ACB 1.29b
  and LZP results quoted above.
- P. M. Fenwick, "Symbol Ranking Text Compression with Shannon Recodings",
  *J.UCS* 3(2), 1997, 70–85,
  <https://www.jucs.org/jucs_3_2/symbol_ranking_text_compression.html>.
- H. Yokoo, M. Takahashi, "Data compression by context sorting", *IEICE
  Trans. Fundamentals* E79-A(5), 1996: prediction from the neighbours of the
  current context in sorted order, kept in a binary tree.
- J. G. Cleary, W. J. Teahan, I. H. Witten, "Unbounded length contexts for
  PPM", *The Computer Journal* 40(2/3), 1997 (PPM*).
