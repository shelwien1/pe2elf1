# The compression algorithm of `vbn2.cpp`

`vbn2.cpp` implements the compressor of **ACB 2.00c**, George M. Buyanovsky's
*Associative Coder of Buyanovsky* (1997), in its MAX mode. VBN 1.001 is ACB
2.00c under another name (see [Identification](#identification)). The method
is Buyanovsky's **associative coding**.

The past is kept as a dictionary of contexts sorted backwards, held here as a
binary trie over the bits that precede each position. At every position the
coder looks up the current context and collects the earlier positions whose
contexts agree with it for at least an adaptive number of bits (the
*associative set*), each weighted by how far it agrees. Then it codes, with an
adaptive arithmetic coder:

- how many bytes of the coming data the best candidate's continuation
  predicts correctly (a match length, 0..255), with the length
  probabilities shaped by where the other candidates part ways with the best
  one;
- when that length is 0, a literal byte, from an order-0 model mixed with the
  bytes the candidates predict, minus the bytes already ruled out.

The match source is never transmitted. Both sides find the same candidates in
the same tree, so the decoder knows which earlier position the length refers
to. In that respect it is LZ77 with the offset replaced by a deterministic,
context-driven choice (LZP, Bloom 1996, does the same with a hash of a
fixed-order context). ACB generalises it to a variable-order, weighted set of
contexts, and the literal coding borrows PPM's mixing and exclusion.

On `book1` of the Calgary corpus (`input.txt`), 768,771 bytes become 222,682
(2.32 bits per byte; `bzip2 -9`: 232,598, `xz -9e`: 261,376, `gzip -9`:
312,285), at about 0.25 MB/s, symmetric.

## Contents

- [Identification](#identification)
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

## Identification

`vbn.exe`'s 32-bit image is the protected-mode image of `ACB.EXE` from the
ACB 2.00c distribution (Internet Archive item `acb-ver-2.00c`, `ACB.EXE`
71,849 bytes, md5 `277af97270a9394a833879e6b4f26821`), rebased from 0 to
`0x10000000`. Comparing the two byte for byte over 53,912 bytes gives:

| | bytes |
|---|---|
| identical | 51,170 |
| differ only in an absolute address's top byte (the rebase: `0x10` vs `0x00`) | 1,788 |
| differ otherwise, in 75 runs | 954 |

The 954 are, in full:

- 41 DOS `int 21h` call sites, which the PE repackaging redirects to
  `vbnshim.dll`;
- the program's name in its strings, `ACB`/`acb` → `vbn`;
- the banner `Associative Coder of Buyanovsky / ACB ver.2.00c (Shareware)
  Copyright(C) 1994-97 George Buyanovsky 04.25.1997`, replaced by `vbn
  ver.1.001 (freeeware)`, and the shareware registration text, blanked;
- one instruction pair in `coder_run`: before coding, ACB checksums the
  first 1,505 bytes of its banner and help text and stops with "ACB.EXE CRC
  Error !!!" if they changed. In VBN the comparison's constant is changed and
  its `je` made a `jmp`, so the check always passes. (The coder then reuses
  that banner's memory as its code buffer.)

Every function `vbn2.cpp` was decompiled from (`code_symbol`, `code_flush`,
`code_literal`, `code_match`, the two searches, the walks, `tree_insert`,
`tree_rebuild`, `code_window`, `coder_run`, `code_buffer_io`,
`len_model_add`) is in `ACB.EXE` byte for byte, apart from the rebase and
that disabled check (vbn2 has no check). VBN's help text is ACB 2.00c's
(which is also its `README.TXT`) with the name changed: the "TRAINABLE
CHANNEL" mode, B/b/u for FAST/NORMAL/MAX, `*.acp` for error-protected
archives, the 64 MB file, 4096-file and 256 KB title limits, Pentium-133 with
16 MB. The README also gives the window: "When the length of the channel
background reaches 1671 Kb, the size of the context file stabilizes"; that is
0x198000 = 1,671,168 bytes.

The lineage shows in Buyanovsky's own published code as well. The C program
accompanying his 1994 article "Associative coding" (*Monitor* 8/94: `ac.c`,
"Demonstration of the data compression stream algorithm by the associative
coding method", dated 08.08.1994) contains `WriteEnd`, which is `code_flush`
statement for statement; `CodAH_D`, whose decoder reads bits and
renormalises as `code_symbol` does; `Stk` and `InitTabAH`, the ring buffers
and adaptive frequency tables behind the literal and length models; and
`LoadLst`, the "funnel of analogies" that the candidate search grew out of.

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

Every 8 KB of code goes to the output (`code_buffer_io`), and the window
slides when it is full.

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
  `ctx_count` of that byte, a pair count indexed by the byte alone (a quirk).
  Each group's
  weight, scaled to the size of the model and to a confidence term
  (`n_cands + 4·search_gain + lit_cost8/64` over a term in `len_avg` and
  `lit_cost8`), plus `search_gain`, is added to its length's count for this
  one symbol. A length where other candidates part ways with the best one
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
  (`pair_bonus`).
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

`code_symbol` is an arithmetic coder over frequency lists of `{count,
symbol}` pairs.

- **Interval.** 32-bit `low` and `high`. A symbol whose list entries before it
  sum to *below*, and *above* with its own count, narrows the interval to
  `high = low + above·size/total − 1`, `low += below·size/total`, with
  `size = high − low + 1` (up to 2^32) and the products in 64 bits.
- **Renormalisation.** The leading bits that `low` and `high` agree on are
  shifted out into the code buffer one at a time (written by toggling bits of
  a zeroed buffer). `code_pending`, 32 minus the bits last shifted out, tells
  the decoder how many fresh bits to read next and bounds `code_flush`'s
  tail.
- **No carry, no underflow handling.** When the interval gets narrower than
  twice the total (`2·total > high − low`), the code is ended: `code_flush`
  writes the shortest tail that keeps every continuation inside the interval
  (`0111…` or `1000…`), and coding starts afresh on the full interval. The
  same happens once at the end of the data. This is Buyanovsky's `WriteEnd` of
  1994.
- **Decoding.** `code_value` holds the 32 bits of code that line up with
  `low`; each symbol first reads the bits that renormalisation shifted out.
  The count the code points at is `((value − low + 1)·total − 1)/size`, and
  the symbol is the first whose cumulative count exceeds it.
- **Lists.** The literal model is sorted by count; the length model is in
  length order, which favours short lengths, and its tail above the maximum
  is cut off by lowering the total.

## Constants

| What | Value |
|---|---|
| window, slide, kept | 0x198000, 0x66000, 0x132000 bytes |
| context tree | ≤ 0x198001 nodes of two 32-bit slots |
| node-making depth limit | 64·log2(nodes + 32) bits |
| leaf renewal age (deep slots) | 0x800 bits (256 bytes) |
| candidates (MAX/NORMAL/FAST) | 95 / 55 / 31 |
| candidate threshold *K*, initial | 276·log2(tree_updates + 4)/512 bits |
| match length | 0..255 (0 = none) |
| length model | 256 counts, +5 per length, over the last 4096 lengths |
| literal model | 256 counts, +16 per literal over the last 363, plus `lit_inc` (1..1024) |
| pair counts | `ctx_count[256·prev + byte]` over the last 1024 literals, ≤ 255 |
| literal cost averages | 4, over ~256/512/1024/2048 literals |
| arithmetic coder | 32-bit interval, bitwise output, 8 KB code buffer, flush when range < 2·total |

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
  lie before the 32 bits `code_value` holds when it is next used.

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
  - no rank is coded: the best-weighted context is the match source, and
    the other members of the set shape the length and literal
    probabilities instead;
  - the 1.6 MB window serves both the archiver and its "trainable channel"
    (priming the coder with earlier data).

Close relatives:

- **LZP** (Bloom, 1996), which also predicts a match position from the
  context and codes only its length, but with a single fixed-order hashed
  context;
- **PPM**, for mixing context predictions into the literal model, and for
  exclusion;
- **BWT and context sorting**, for the sorted backwards contexts;
- **DMC**, under which Buyanovsky himself said ACB "can be classified", for a
  context structure that grows one step each time a state recurs.

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
