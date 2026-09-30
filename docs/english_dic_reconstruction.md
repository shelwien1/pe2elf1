# Rebuilding english.dic from enwik9, and making dict.rank smaller

Date: 2026-09-30. This covers the dictionary shared by fx2-cmix, cmix-lex and
fx2-cmix-transformer. It explains how the compressor can rebuild that
dictionary from the first 10^8 bytes of enwik9 plus a small side file,
`dict.rank`. It then measures ways to make that side file smaller. The size
accounting around it is in [hp_entry_size.md](hp_entry_size.md) §8.7.

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
- **A prototype "v2" side file comes to 25,026 bytes.** It predicts that
  order from how the words are used in enwik9 (their neighbouring words),
  and leaves cmix as the coder. That is another 20.5 KB off.
  - A v2 decoder has not been written.
  - A real one needs integer arithmetic so that it reproduces bit-exactly on
    the committee's machine.

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

**What a real v2 needs:**

- **A decoder.** None exists yet, but every section only uses information a
  decoder has at that point:
  - the text's word counts, document frequencies and letter case;
  - context counts from enwik9 for the set of words already decoded;
  - the words and run assignments already decoded.
- **Bit-exact arithmetic.** The prototype uses floating point (logarithms,
  cosines, and SVD for the head vectors), which need not give identical
  rankings on another CPU or build. The float differences that broke
  fx2-cmix-transformer's July archive are the same kind of problem.
  - Use integer counts, PPMI from a fixed-point log table, sparse vectors
    (§7.1 shows SVD is unnecessary; the head section would need re-measuring
    without it), 64-bit integer dot products, and exact comparisons.
  - For example, compare dot²/|centroid|² across runs by 128-bit
    cross-multiplication.
- **Time.** Counting neighbours over enwik9's 141.6 million words and
  scoring 40,595 words against 510 runs takes seconds in C++. Memory is
  modest: only dictionary-candidate rows are kept.
- **Code size.** The extra decoder code (context counting, PPMI, centroid
  ranking, feature sort) is again stored twice in fx2-cmix's layout. A few
  KB packed would still leave roughly −15 KB net. This is not measured.

### 7.5 Further ideas, not measured

- **An arithmetic coder with the softmax model (approach B)** gains little
  over approach A. It saves about 0.5 KB on the run numbers (18,371 vs
  18,905) and 0.15 KB on the head (3,371 vs 3,525), and cmix already beats
  the bitmap context model. It is not worth code stored twice.
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
- **Replace english.dic with a dictionary the compressor builds from enwik9
  itself.** This is outside exact reconstruction: the dictionary would still
  be stored in archive9, and the word codes, and so the enwik9 stream, would
  change.

## 8. Reproducing the numbers

- **Side-file tools.** `tools/dicrank.cpp` and `tools/dicrank_dec.cpp`, with
  the build and usage lines above.
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
