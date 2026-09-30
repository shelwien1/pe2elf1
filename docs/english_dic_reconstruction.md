# Rebuilding english.dic from enwik9, and making dict.rank smaller

Date: 2026-09-30. This covers the dictionary shared by fx2-cmix, cmix-lex and
fx2-cmix-transformer. It explains how the compressor can rebuild that
dictionary from the first 10^8 bytes of enwik9 plus a small side file,
`dict.rank`. It then measures ways to make that side file smaller, and
describes an integer-only implementation of the best of them (v2, §8). §9
tries replacing cmix with a coder of its own (v3). The size accounting
around it is in [hp_entry_size.md](hp_entry_size.md) §8.7.

## Summary

- **Every word is in enwik8.** All 44,515 words of `english.dic` occur in
  enwik8, which is the first 10^8 bytes of enwik9. This holds when the text
  is split into words the way cmix's word transform splits it.
- **The compressor can rebuild the dictionary exactly.** The side file
  `dict.rank` (from `tools/dicrank.cpp`) records which words to take and in
  what order.
- **It is less than half the size.** `dict.rank` is 45,569 bytes after cmix,
  against 100,088 for `english.dic`. The rebuilding code has to go into the
  program, which fx2-cmix stores twice. Net, the score S drops by about
  50 KB.
- **The order is what costs bytes.** 34.4 KB of the 45.6 KB go into the run
  numbers that record the order of the 40,595 words in alphabetical runs.
- **A "v2" side file comes to 25,112 bytes.** It predicts that order from
  how the words are used in enwik9 (their neighbouring words), and leaves
  cmix as the coder.
  - It is implemented with integer arithmetic only (`tools/dicrank2.cpp`,
    §8); builds with different compilers and flags give byte-identical
    output.
  - With its lean decoder (`tools/dicrank2_dec.cpp`) counted twice, the
    compressor saves 63,312 bytes against english.dic, which is 13.3 KB
    more than v1.
- **A built-in coder makes it smaller, but costs more than it saves.**
  `tools/dicrank3/` (§9) codes v2's numbers itself instead of leaving them
  to cmix.
  - It uses a context-mixing coder built from the tsvcomp coder's
    components.
  - It sees what cmix can't: candidate scores, and each word's case,
    length, frequency and suffixes.
  - The side file drops to 21,764 bytes.
  - But the coder adds about 18 KB of packed code to each copy of
    fx2-cmix's program, which is stored twice.

## 1. The dictionary and where it is stored

`english.dic` has 44,515 lowercase words, one per line (411,996 bytes). The
same file (md5 `b3a9cf9f…`) is used by fx2-cmix, cmix-lex and
fx2-cmix-transformer.

**What cmix uses it for:**

- **The word transform.** It replaces each dictionary word by a code whose
  length depends on the word's position: words 0–79 get 1-byte codes, words
  80–3,919 get 2-byte codes, and the rest get 3-byte codes. So the exact
  order matters, not just the set of words.
- **`Pretrain()`.** The model is trained on the dictionary text before
  coding starts.
- **The fxcm model**, which reads `.dict` for its dictionary-based contexts.

**Where it is stored.** Both the compressor and archive9 carry it,
cmix-compressed to 100,088 bytes. Only the compressor can rebuild it,
because only the compressor has enwik9. archive9 needs the dictionary before
it can decode anything.

## 2. Every dictionary word occurs in enwik8

**Splitting text into words.** The word transform
(`preprocessor::Dictionary::Encode`) splits text into words like this:

- A capital letter after lowercase letters starts a new word: `McDonalds`
  becomes `mc` + `donalds`.
- A lowercase letter after two or more capitals starts a new word: `JScript`
  becomes `js` + `cript`.
- Anything other than A–Z/a–z ends a word.
- Words are lowercased.

With exactly these rules, all 44,515 words occur in enwik8. A plain split
into letter runs misses 11 of them: `bramson`, `contrass`, `cript`,
`overnors`, `millan`, `cormick`, `xchange`, `kellen`, `dowell`, `donalds`,
`kinley`. So the dictionary was built with the transform's own split.

**The candidate list.** It is all distinct words of enwik8 (281,598), sorted
by descending count, with ties broken alphabetically. Where the dictionary's
codeword tiers fall in that list:

| Positions in english.dic | Codeword length | Median frequency rank | Largest rank |
|---|---|---:|---:|
| 0–79 | 1 byte | 54 | 136 |
| 80–3,919 | 2 bytes | 2,104 | 4,089 |
| 3,920–44,514 | 3 bytes | 26,665 | 71,262 |

The tiers follow frequency closely, but the order *within* each tier does
not. Only 51–53% of neighbouring words ascend in rank, about what a random
order would give.

**enwik8 is better than all of enwik9 here.** Counting words over all of
enwik9 lets many more non-dictionary words outrank rare dictionary words.
The membership bitmap then grows from 67,343 to 260,353 entries, and the
side file from 45,569 to 50,272 bytes after cmix.

## 3. How the dictionary is ordered

**The first 3,920 words (the 1- and 2-byte tiers) are ordered by meaning.**
For example:

> … effort approach benefits benefit limit limits defeat capture causes cause
> drop change increases increase advance turn support cover move pass fly
> break fight match play draw operate survive serve grow …

Their maximal ascending runs average two words, which means there is no
alphabetical structure.

**The other 40,595 words form 510 maximal alphabetical runs.** They average
80 words; the largest have 1,044, 1,025, 624 and 598 words. Each run is one
kind of word:

| Run | Words | Examples |
|---:|---:|---|
| 1 | 3 | astrologically symbolically variously |
| 50 | 6 | afloat aground ashore hurriedly operationally safely |
| 100 | 53 | abiding abstracted akin analogous answerable antithetical … |
| 200 | 48 | accommodates adjoined boarding boasting boasts cater … |
| 300 | 152 | afternoon anybody ashes … yesterday yours yourselves |
| 400 | 90 | aardwolf acacia agapanthaceae agapanthus agavaceae agouti … |
| 509 | 263 | abbasid abbasids absalon adhemar … xiongnu yazdegerd yorkist |

**Runs are not word families.** Only 18% of words share a run with their
longest dictionary word that is a prefix of them. `boast`, `boasting` and
`boasted` are in three different runs, while `compile`, `compiles` and
`compiling` share one. When words are taken in order of first occurrence in
enwik8, only 7% of neighbours share a run. The runs look like classes by
part of speech and meaning, which is what a word's contexts reveal (§7).

## 4. The dict.rank format (`tools/dicrank.cpp`)

The side file is text, meant to be compressed with the entry's own
`cmix -c` like the entry's other side files:

```
dicrank 1
<bytes scanned> <explicit words S> <runs> <bitmap length> <extra words>
<extra words, one per line>          words that do not occur in the text
<S lines>                            frequency rank of each explicitly ordered word
<bitmap>                             one 0/1 per candidate, frequency order,
                                     explicit words skipped, up to the last member
<one line per remaining word>        its run number, words in order of reversed spelling
```

**Decoding:**

1. Count the words of the first `<bytes scanned>` bytes of the text.
2. Build the candidate list: the extra words first, then the counted words
   by descending count, ties alphabetical.
3. Take the explicitly ordered words by rank.
4. Read the bitmap to get the other words.
5. Sort those by reversed spelling and read one run number each.
6. Output each run sorted alphabetically, runs in number order.

**Determinism.** Only integer counting, sorting and string comparison are
involved. The same bytes come out on any machine and with enwik8 or enwik9
as input, since only the first 10^8 bytes are read.

**Why reversed spelling?** It puts words with the same ending next to each
other, e.g. all `-ically` adverbs and `-ing` forms. That lets cmix learn
which runs are common locally.

**Usage.** The encoder decodes its own output and fails unless it
reproduces the dictionary byte for byte:

```sh
g++ -O2 -std=c++17 -o dicrank tools/dicrank.cpp
./dicrank e enwik9 english.dic dict.rank     # encode (~6 s including the self-check)
./dicrank d enwik9 dict.rank english.dic     # decode
```

**`tools/dicrank_dec.cpp`** is the same decoder with no standard containers:
an open-addressing hash table, libc `qsort` and stdio. It is meant for
linking into a compressor as `DicrankDecode(text, side, out)` and rebuilds
english.dic from enwik8 in 1.8 s.

**Tests.**

- Both decoders reproduce english.dic from enwik8 and from enwik9.
- They also produce identical results for dictionaries containing words
  that are not in the text, a descending word list, and `-s` 0, 7, 80 and
  100000.

## 5. Measurements

All sizes are after compression with fx2-cmix's own `cmix -c`, the binary
extracted from the shipped archive9.

| Encoding of english.dic | Bytes |
|---|---:|
| english.dic itself | 100,088 |
| Every word as its frequency rank | 82,448 |
| Runs; run numbers in frequency order | 51,478 |
| Runs; run numbers in alphabetical order | 48,959 |
| Runs; run numbers move-to-front coded, reversed-spelling order | 47,738 |
| **Runs; run numbers in reversed-spelling order (`dicrank`)** | **45,569** |
| The same, counting words over all of enwik9 | 50,272 |

- **Breakdown of the 45,569 bytes.** Compressed separately, the three
  sections come to 5,600 (the 3,920 ranks), 5,659 (bitmap) and 34,360 (run
  numbers).
- **Prototype rows.** The four rows between the plain ranks and `dicrank`
  are prototype files with a one-line header; the tool's two-line header
  adds 19 bytes.

## 6. Using it in fx2-cmix

1. **Replace the stored dictionary stream.** The compressor stores the
   cmix-compressed `dict.rank` instead of `comp_dict`.
2. **Rebuild the dictionary at startup.** At `-e`, it unpacks `dict.rank`
   (`./cmix -d`, about a minute) and calls
   `DicrankDecode("enwik9", ".dict.rank", ".dict")` (about 2 s).
3. **Recreate archive9's copy.** Run `./cmix -c .dict .dict.comp`. The
   shipped binary turns the rebuilt english.dic into a stream byte-identical
   to the one in archive9 (100,088 bytes, 112 s here), so archive9 does not
   change.

**Cost.** The decoder is compressor-only code, but fx2-cmix's program is also
copied into archive9, so it counts twice.

- `dicrank_dec.cpp` adds 2,256 bytes per copy with UPX 3.95 `-9`, or 2,004
  with `--ultra-brute`. The STL decoder in `dicrank.cpp` would add 12,116.
- Net change in S: −54,519 + 2 × 2,256 = **−50,007 bytes**.
- Compression takes about 3 minutes longer.

A complete modified fx2-cmix has not been built or run on enwik9.

## 7. Making dict.rank smaller

**Where the 45.6 KB goes.** Almost everything is order information:

- which of the ~4,000 most frequent words are among the first 3,920, and in
  what order (5.6 KB);
- which rarer words are in the dictionary at all (5.7 KB);
- above all, which alphabetical run each of those words belongs to
  (34.4 KB).

**What can predict it.** Spelling and first occurrence hardly help (§3). But
the runs and the hand-made order are about meaning and part of speech, and
the compressor holds a gigabyte of text showing how each word is used. So
the experiments below predict order from contexts.

- **Context vectors.** Each word's vector records which of the 4,000 most
  frequent words appear directly before and after it. The counts are
  weighted by positive pointwise mutual information (PPMI), and words are
  compared by cosine.
- **Two ways to use the predictions:**
  - (A) turn them into small numbers (the rank of the true answer among
    predictions) and keep cmix as the coder, which adds little code;
  - (B) code the answer with an explicit probability model and an
    arithmetic coder, which adds more code.

The estimates are adaptive code lengths (−log2 of the probability, learned
as it goes), or real cmix output where marked. The scripts are in
`tools/dicrank_experiments/` (§8).

### 7.1 Run numbers (34,360 bytes today)

**Method.**

- Visit the words from most to least frequent. Each run's centroid is the
  sum of the vectors of the words already assigned to it.
- Code each word as the rank of its true run among the runs started so far,
  sorted by cosine to the word's vector. A word that starts a new run is
  coded as "new" plus its index among the runs not started yet (about
  480 bytes in total).
- Variant (B) instead codes the run with
  p(run) ∝ (words in run + 0.5) · e^(β·cosine), with an adaptive escape for
  new runs.

| Context statistics | Vector | True run ranked first | Rank-coded (A), bytes | Softmax (B), bytes |
|---|---|---:|---:|---:|
| Plain run ids, no prediction (adaptive order-0) | — | — | 40,249 | — |
| enwik8, 2,000 neighbours | SVD 128 | 16.2% | 31,075 | 28,786 (β = 20) |
| enwik8, 2,000 neighbours, plus suffix features | SVD 128 | 17.3% | 29,690 | 27,402 (β = 20) |
| enwik8, 4,000 neighbours, plus suffix features | SVD 200 | 20.3% | 28,345 | 25,941 (β = 20) |
| enwik8, 4,000 neighbours, suffix features at full weight | SVD 200 | 8.6% | 32,947 | 33,608 (β = 20) |
| enwik8, 4,000 neighbours | sparse PPMI, no SVD | 27.7% | 25,919 | 27,773 (β = 40) |
| enwik9, 4,000 neighbours, plus suffix features | SVD 200 | 35.8% | 20,588 | 18,371 (β = 20) |
| **enwik9, 4,000 neighbours** | **sparse PPMI, no SVD** | **42.2%** | **19,032** | 20,862 (β = 40) |
| Same, rank symbols compressed by cmix | | | **18,905** | |

- **Counting contexts over all of enwik9 is the main win.** The rarer
  dictionary words get much better vectors.
- **SVD isn't needed for rank coding.** Sparse PPMI vectors do better
  there (19,032 vs 20,588), and they can be computed exactly in integers.
  The softmax variant prefers the SVD vectors.
- **Suffix features help the SVD vectors a little.** Given full weight, they
  hurt.

### 7.2 The first 3,920 words (5,600 bytes today)

A random order of 3,920 words would cost log2(3920!) ≈ 5,143 bytes, so plain
ranks are near that bound. Word families don't help: only 2.2% of
neighbours share their first four letters.

What works is to code the set and the order separately:

- **The set** is a bitmap over the 4,090 most frequent words: **158 bytes**
  with cmix.
- **The order:** each next word is coded as its rank among the remaining
  words, sorted by similarity to the previous word plus 0.5 × the word
  before it (enwik9 vectors).

| Head-order encoding | enwik8 vectors | enwik9 vectors |
|---|---:|---:|
| Similarity ranks, compressed by cmix | — | **3,525** |
| Similarity ranks, adaptive order-0 estimate | 3,880 | 3,895 |
| Softmax model, β = 10 (estimate) | 3,349 | 3,371 |

These are frequent words, so enwik8 alone already gives them good vectors.

### 7.3 The membership bitmap (5,659 bytes today)

The bitmap says which of the 67,343 candidates (in frequency order) are
dictionary words.

- **Other sort keys make it longer and costlier.** Sorting by document
  frequency (the number of articles a word appears in) raises the estimate
  from 5,788 to 6,364 bytes; lowercase-occurrence count gives 6,926.
- **Four cheap features predict membership well:** word length, log2
  document frequency, log2 count, and the share of lowercase occurrences.
  Words frequent within only a few articles, such as names repeated inside
  one article, are rarely in the dictionary.
  - An adaptive context model on these features: about 2,986 bytes.
  - Simply sorting the bits by these features and letting cmix code them:
    **2,544 bytes** (xz: 3,696).

### 7.4 Prototype v2 side file: 25,026 bytes

`build_v2.py` assembles §7.1–7.3 into one text file, with every section
coded by cmix (approach A):

| Section | dicrank (v1) | v2 prototype |
|---|---:|---:|
| First 3,920 words: set | (in ranks) | 158 |
| First 3,920 words: order | 5,600 | 3,525 |
| Membership of the other words | 5,659 | 2,544 |
| Run numbers | 34,360 | 18,905 |
| **Whole file, compressed as one** | **45,569** | **25,026** |

Against english.dic in the compressor this is −75,062 bytes, i.e. −20,543
beyond `dicrank` v1.

The prototype uses floating point, including SVD for the explicit-word
vectors, which need not rank identically on another CPU or build. §8
describes the integer implementation that replaces it: 25,112 bytes, and
the same output from every build tested.

### 7.5 Further ideas, not measured

- **An arithmetic coder with the softmax model (approach B)** gains little
  over approach A. It saves about 0.5 KB on the run numbers (18,371 vs
  18,905) and 0.15 KB on the head (3,371 vs 3,525), and cmix already beats
  the bitmap context model. It is not worth code stored twice. §9 measures
  a full context-mixing coder: it saves more, but its code costs far more
  still.
- **Better context vectors.**
  - Wider windows (±2 words), context distribution smoothing, shifted PMI.
  - Features from document structure: same article, same section, inside
    link or template syntax.
  - Suffix features added to the sparse vectors.
- **Nearest-neighbour voting instead of centroids.** Some runs probably
  contain several clusters.
- **Better visiting order for the runs**, e.g. most confident predictions
  first, or starting each run from its most typical word.
- **Joint coding of membership and run.** A word's contexts also say whether
  it is likely to be in the dictionary at all.
- **Structure in the head.** Use the tier boundary (80 / 3,840) and the
  pairs such as `benefits benefit` and `limit limits`.
- **Trim the header** (~40 bytes).
- **Shrink `dicrank2_dec`.** It adds 5.8 KB of packed code per copy of the
  program. Every byte saved there is worth two in S, for example by sharing
  the tokenizer and hash table with cmix's own dictionary code.
- **Replace english.dic with a dictionary the compressor builds from enwik9
  itself.** This is outside exact reconstruction: the dictionary would still
  be stored in archive9, and the word codes, and so the enwik9 stream, would
  change.

## 8. The v2 implementation (`tools/dicrank2.cpp`, `tools/dicrank2_dec.cpp`)

`dicrank2` implements the v2 side file in C++ with integer arithmetic only,
so every machine and compiler rebuilds the same dictionary. There are two
programs:

- **`dicrank2.cpp`** encodes, and also decodes. The encoder decodes its own
  output and fails unless it reproduces the dictionary byte for byte.
- **`dicrank2_dec.cpp`** is a small decoder for linking into a compressor:
  `Dicrank2Decode(text, side, out)`. It avoids standard containers and uses
  only stdio, malloc and `qsort`.

```sh
g++ -O2 -std=c++17 -o dicrank2 tools/dicrank2.cpp
./dicrank2 e enwik9 english.dic dict2.rank    # encode (~37 s) + check (~37 s)
./dicrank2 d enwik9 dict2.rank english.dic    # decode (~37 s; dicrank2_dec 37-45 s)
```

Word counts, document frequencies and case come from the first 10^8 bytes;
context statistics come from all of the text (`-n`, `-c` change that). The
other options are `-k` (context words, default 4000), `-s` (explicitly
ordered words, default 3920, at most 10,000) and `-v` (stage timings).

**Format** (text, compressed with the entry's own `cmix -c`):

```
dicrank 2
<counted bytes> <context bytes> <K> <explicit words S> <runs> <extra words>
<extra words, one per line>            words that do not occur in the counted text
<bitmap over ranks 0..>                which of them are explicit words
<position of the first explicit word among them>
<S-1 lines>                            position of each next explicit word, by similarity
                                       to the previous two
<number of candidates covered by the next bitmap>
<bitmap>                               dictionary membership of the other candidates, in
                                       order of word length, document frequency, count,
                                       lowercase share
<one line per member, most frequent first>
                                       position of its run among the runs started so far,
                                       by similarity to their centroids; n<k> = k-th run not
                                       started yet
```

**How the arithmetic stays exact.** No floating point is used anywhere on
the data path.

- **Weights (PPMI).** Counts are exact integers. The weight is
  L(count × total) − L(row sum × column sum), kept if positive, where L is
  log2 in 16-bit fixed point. L is computed by repeatedly squaring the
  mantissa with 128-bit products.
- **Vector length.** Each vector is scaled to length ≤ 2^15 using an exact
  integer square root, rounded up. That keeps the dot product of two
  vectors within 32 bits.
- **Explicit-word scores.** Score = 2 × dot(previous word) + dot(the word
  before). The dot products come from an int32 similarity matrix of the
  explicit words.
- **Run scores.** Score = floor(dot(word, centroid) × 2^20 / isqrt(|centroid|²)),
  with a 128-bit intermediate. Centroid lengths are kept exact by updating
  |c + u|² = |c|² + 2 c·u + |u|².
- **Ties** are broken by lower rank or run number, so encoder and decoder
  always pick the same item.

**Size.** Compressed with fx2-cmix's own cmix:

| Section | v1 (`dicrank`) | Float prototype | **v2 (`dicrank2`)** |
|---|---:|---:|---:|
| Explicit words: set | (in ranks) | 158 | 158 |
| Explicit words: order | 5,600 | 3,525 (SVD vectors) | 3,595 (sparse vectors) |
| Membership of the other words | 5,659 | 2,544 | 2,546 |
| Run numbers | 34,360 | 18,905 | 18,908 |
| **Whole file** | **45,569** | **25,026** | **25,112** |

The integer version matches the float prototype to within 86 bytes. Most of
that (70 bytes) comes from the explicit-word order, which now uses sparse
vectors instead of SVD.

**Checks:**

- **Round trip.** english.dic with enwik9 reproduces byte for byte.
- **Determinism across builds.** Four builds of `dicrank2` produce
  byte-identical side files, and each also passes its own round-trip check:
  - g++ `-O2 -march=native` (AVX-512);
  - clang++ `-O3`;
  - g++ `-O1 -fno-tree-vectorize`;
  - clang++ `-O2 -march=x86-64-v3 -ffast-math`.

  Three differently compiled `dicrank2_dec` builds rebuild english.dic
  exactly from that side file.
- **Edge cases.** Both decoders reproduce these exactly:
  - dictionaries containing words that are not in the text (in both parts);
  - `-s 0`, and `-s 80`;
  - every word its own run;
  - all words explicit (a small dictionary).

  AddressSanitizer and UBSan runs are clean.

**Cost.**

- **Time.** Decoding with enwik9 splits into word counts 1.4 s, context
  vectors 22 s (the 1 GB scan), explicit-word similarity 8 s, and runs 5 s.
- **Memory.** Peak 1.5 GB (`dicrank2`) or 1.39 GB (`dicrank2_dec`). Nearly
  all of it is the dense neighbour counts: 44,515 words × 8,000 features ×
  4 bytes.
- **Code in fx2-cmix.** Measured by linking each decoder into an fx2-cmix
  build compiled with `-Os`, packed with UPX 3.95 `-9` / `--ultra-brute`:
  `dicrank2_dec` adds 5,832 / 5,432 bytes per copy; `dicrank2.cpp`'s decoder
  adds 15,804 / 14,420.

**Net effect on fx2-cmix's compressor**, with the entry's UPX 3.95 packing
and the decoder code counted twice:

| The compressor carries | Side file | Decoder code × 2 | Total | vs english.dic |
|---|---:|---:|---:|---:|
| english.dic, as shipped | 100,088 | — | 100,088 | — |
| v1 side file + `dicrank_dec` | 45,569 | 4,512 | 50,081 | −50,007 |
| **v2 side file + `dicrank2_dec`** | **25,112** | **11,664** | **36,776** | **−63,312** |
| v2 side file + `dicrank2.cpp` decoder | 25,112 | 31,608 | 56,720 | −43,368 |

**Only the lean decoder makes v2 pay.** With the STL decoder, v2 would lose
to v1.

**Integration** is the same as in §6, except that the compressor calls
`Dicrank2Decode("enwik9", ".dict.rank", ".dict")`. That takes under a
minute and 1.4 GB, freed before cmix's model allocates its memory. A complete modified
fx2-cmix has not been built or run on enwik9.

## 9. dicrank3: a built-in coder instead of cmix (`tools/dicrank3/`)

**dicrank2 has no entropy coder.** It writes its positions and bitmaps as
text and leaves all the coding to cmix. cmix sees only the digits. On the
run numbers it does about as well as a model of the position alone: 18,908
bytes, against 18,933 for dicrank3's coder with every context except the
position replaced by a constant.

At each step the decoder knows much more than the digits: how far each
candidate scored below the top, and which runs already hold words with the
same ending or the same capitalization. None of that is in the text file.

**dicrank3 codes the streams itself.** dicrank2 is kept unchanged, and
dicrank3 reuses its transform code unchanged.

- **The coder** is built from the tsvcomp coder's components. Every binary
  decision goes through five stages:
  1. 28 hashed contexts, each mapped to a `Counter` cell;
  2. 7 `MixN` group mixers;
  3. a `MixN` final mixer;
  4. an interpolated SSE stage;
  5. a `Mix2` of the mixer output and the SSE output.

  Every stage learns end to end. The knobs are declared in the IDX files
  (`IDX/dr3-*.idx`), and `IDX/opt.pl` tunes them by patching the tuning
  binary.
- **How each stream is coded:**
  - **The two bitmaps:** one decision per bit.
  - **Explicit-word order:** each next word's position among the remaining
    words, sorted by dicrank2's score. The first 16 positions are yes/no
    decisions; after that comes an Elias-gamma escape.
  - **Run numbers:** a "new run?" flag. A new run's index is coded as a
    bit tree. Otherwise, one yes/no decision per started run, in the order
    of dicrank2's centroid score.
- **Contexts:**
  - the candidate's position and score, and its margins to the top and to
    the next candidate;
  - the word's case, length, document frequency, count and suffixes;
  - run statistics: size, recency, usual fit, and the share of members
    with the word's suffix, case class, length class and count class.

**Result** for english.dic, with the context statistics from enwik9:

| Section | dicrank2 + cmix | dicrank3, tsvcomp's knobs | dicrank3, partly tuned |
|---|---:|---:|---:|
| Explicit words: set | 158 | 28 | 40 |
| Explicit words: order | 3,595 | 3,610 | 3,532 |
| Membership of the other words | 2,546 | 1,646 | 1,498 |
| Run numbers | 18,908 | 16,844 | 16,636 |
| Header | — | 58 | 58 |
| **Side file** | **25,112** | **22,186** | **21,764** |

The dicrank2 sections were compressed separately; its whole file,
compressed as one, is 25,112 bytes.

**Where the gain comes from.**

- **Membership bitmap (−1,048).** The word's own features (case, length,
  document frequency, count, suffix) are direct contexts. cmix gets them
  only indirectly, through the order the bits are sorted in.
- **Explicit-word order (−63).** Little. The order was made by hand, and
  the similarity scores are the only real information about it.
- **Run numbers (−2,272).** Case contributes the most of any group.
  Replacing a group of contexts by a constant (tuned model) costs:

| Contexts replaced by a constant | Run numbers | Change |
|---|---:|---:|
| none | 16,636 | |
| Case: the word's lowercase share against the run members' (5 contexts) | 17,150 | +514 |
| Score: the candidate's score, its margins to the top and to the next candidate (4) | 16,811 | +175 |
| Run identity: the run alone, and with the word's length, burstiness, count class, fit (6) | 16,799 | +164 |
| Suffix: members sharing the word's last 1-3 letters, the word's suffix per run (4) | 16,789 | +153 |
| Run size, recency, usual fit, members' count class, the previous word's run (6) | 16,771 | +135 |
| Case, length and count class agreement together (2) | 16,656 | +20 |
| Everything except the position (27 of 28) | 18,933 | +2,297 |

The case contexts compare the word's lowercase share with the run's. Many
runs are proper nouns or common words only, and the context vectors hardly
capture capitalization.

**Tuning (partial).** `opt.pl` climbed 8 knobs (126 bits) in about half
an hour, in parallel runs whose results were merged. It took the four
streams from 22,128 to 21,706 bytes. The knobs that moved are:

- the counters' starting logistic scale (`H0 K`);
- how far the counters' decay-rate updates follow the final error instead
  of the counter's own (`H0 E2Euv`);
- the step gains of the counters' scale and prior-mix updates (`H0 NWk`,
  `H0 NWm`);
- the final Mix2's bias step (`M1 NWb`).

**How the tuning runs.** `dr3_tune` codes the dumped streams in about 10 s
instead of dicrank3's minute, and `opt.pl` patches the tuning binary
between runs.

**What is left.**

- A full pass over the 44 knobs picked for tuning would take several
  hours.
- A pass over all 3,503 knob bits would take about 9 hours.
- Further passes should shrink the file more.

**Floating point.** The coder computes in floats. Encoder and decoder agree
only if both round every operation the same way.

- **What dicrank3 enforces:**
  - it refuses to compile with `-ffast-math`;
  - it disables FMA contraction with pragmas for GCC and clang;
  - it requires `FLT_EVAL_METHOD == 0`;
  - it sets flush-to-zero and denormals-are-zero while coding;
  - its exp/log are Schraudolph's bit tricks, not libm.
- **What was tested.** These builds produce the same stream:
  - g++ `-O1 -fno-tree-vectorize`, `-O2` and `-O3 -march=native`;
  - clang `-O3` and `-O2 -march=x86-64-v3`;
  - the IDX tuning build.
- **Cross-build decoding.** A clang `x86-64-v3` build decodes a gcc build's
  side file exactly.
- **Without the pragmas**, the gcc and clang FMA builds each produced a
  different stream (22,169 and 22,168 bytes instead of 22,162). This was
  measured on an earlier version of the model.
- **Edge cases.** The dicrank2 cases also round-trip: words missing from
  the text, `-s 0`, `-s 80`, all words explicit, every word its own run.

**Cost.**

- **Time and memory.** Encoding takes about 52 s, plus 50 s for the self
  check. Decoding takes about 50 s, of which 14 s is the coder, and peaks
  at 1.86 GB. The coder's hash tables take 367 MB.
- **Code**, linked into an fx2-cmix build as in §8 (`-Os`, but the dicrank3
  file without `-ffp-model=fast`):

| Decoder in fx2-cmix | UPX 3.95 `-9` | UPX 4.2.4 `--ultra-brute` |
|---|---:|---:|
| none | 131,588 | 120,544 |
| `dicrank2_dec` (lean) | +5,832 | +5,432 |
| `dicrank2.cpp`'s decoder | +15,804 | +14,420 |
| `dicrank3.cpp`'s decoder | +34,056 | +30,924 |

The coder is the difference between the last two rows: +18,252 / +16,504
bytes per copy of the program. In the unlinked object file:

- the components take about 21 KB, including two `Counter` updates of
  2.9 KB each and the float constants' initialization;
- the four stream coders take 15.6 KB.

**Net effect on fx2-cmix's compressor.** UPX 3.95, with the decoder code
counted twice:

| The compressor carries | Side file | Decoder code × 2 | Total | vs english.dic |
|---|---:|---:|---:|---:|
| v2 side file + `dicrank2_dec` | 25,112 | 11,664 | 36,776 | −63,312 |
| v3 side file + `dicrank3.cpp`'s decoder | 21,764 | 68,112 | 89,876 | −10,212 |
| v3 side file + lean transform + this coder (estimate) | 21,764 | 48,168 | 69,932 | −30,156 |

**Verdict.**

- **In compression, the coder beats cmix** by 3,348 bytes (13.3%),
  because it sees what the decoder knows.
- **In fx2-cmix it does not pay.** Its code is counted twice, and it costs
  more than ten times what it saves. To pay, the coder's code would have
  to fit in about 1,674 packed bytes per copy. One `Counter` update alone
  is 2.9 KB unpacked.
- **The source-zip layout** (hp_entry_size.md §8.4) counts compressor-only
  code once, as compressed source. That halves the cost, which is still far
  above the saving.

**What might pay.**

- **A small integer coder** for the few contexts that carry most of the
  gain. Not built. Options:
  - lpaq-style counters, one mixer and one APM;
  - fx2-cmix's own mixer and SSE classes.

  The code for the contexts alone might use up the budget.
- **Case agreement in the transform: pays, measured.** It goes where cmix
  codes it for free.
  - The change: dicrank2's run order ranks by
    score + 0.01 · 2^35 · ln((m + 0.5)/(n + 1)).
    Here n is the run's size and m is the number of its members in the
    word's case class (the BitmapKey lowercase-share bucket).
  - The result: cmix codes the run numbers in 18,280 bytes instead of
    18,908, i.e. −628. The weight was scanned on a proxy, the sum of
    log2(position + 1).
  - The cost is a few lines of transform code. It would need an integer
    log for determinism, and `Log2Q16` is already there.
  - This is not in dicrank2, which stays as it is.
- **A better sort order for the membership bitmap: no gain.** Sorting the
  bits by a 17-weight logistic model of membership (case, length, document
  frequency, count, rank) gives cmix 2,577 bytes, against 2,540 in
  BitmapKey order. What the coder gets from the word features (suffixes,
  per-group bit history) does not fit into one sort key.

## 10. Reproducing the numbers

- **Side-file tools.**
  - v1: `tools/dicrank.cpp` and `tools/dicrank_dec.cpp`.
  - v2: `tools/dicrank2.cpp` and `tools/dicrank2_dec.cpp`, with the build
    and usage lines above.
  - v3: `tools/dicrank3/`, built with its `build.sh`; see its README.
- **§9 numbers.**
  - `dicrank3 e -v` prints each stream's size.
  - `dicrank3 e -D dump ...` followed by `DR3_DUMP=dump dr3_tune c sbhr out`
    recodes the streams in seconds.
  - The ablations replace context groups with `DR3_NOCX=0:<hex mask>`.
  - The two transform experiments read a dicrank3 dump; compress their
    output with cmix. They are in `tools/dicrank_experiments/`:
    - `case_rerank.cpp` puts case agreement into the run ranking:
      `case_rerank DUMP 0.01 out.txt`;
    - `bitmap_logit.py` sorts the membership bitmap by a logistic model.
- **Experiments.** They are in `tools/dicrank_experiments/`, need numpy and
  scipy, and take their paths from the environment (`ENWIK8`, `ENWIK9`,
  `ENGLISH_DIC`, `WORK`). Run from that folder:

```sh
python3 prep.py      # enwik8: word counts, candidate order, ids, pages, case
python3 prep9.py     # enwik9 mapped to enwik8 candidate ranks (~2.5 min)
python3 runs.py      # §7.1 table (~10 min)
python3 head.py      # §7.2
python3 bitmap.py    # §7.3
python3 build_v2.py  # WORK/dict_rank_v2.txt and one file per section
```

- **cmix sizes.** They come from the fx2-cmix binary extracted from archive9
  (`tools/hp_entry_parse.py --dump archive9`), run as
  `cmix -c file out`. On Linux 6.18 it needs
  `LD_PRELOAD=pinmmap.so` (`tools/pinmmap.c`).
- **Estimates vs measurements.** Adaptive-coding estimates are marked as
  such; everything else is measured.
