# dicrank2

`dicrank2` stores the word dictionary of fx2-cmix, cmix-lex and
fx2-cmix-transformer (`english.dic`, 44,515 words, 411,996 bytes) as a small
side file. The dictionary is rebuilt from that file and the text it was made
for (enwik9). Every dictionary word occurs in that text, so the side file
only has to say which words to take and in what order.

- **Size.** For english.dic the side file is 172,252 bytes of text. fx2-cmix's
  own `cmix -c` compresses it to 25,112 bytes. The compressed english.dic in
  the entries is 100,088 bytes.
- **Arithmetic.** It is integer-only, so any compiler and build flags give the
  same side file and the same dictionary.
- **Self-check.** The encoder decodes its own output and fails unless it
  gets the dictionary back byte for byte.

| File | What it is |
|---|---|
| `dicrank2.cpp` | Encoder and decoder |
| `dicrank2_dec.cpp` | The same decoder without standard containers, for linking into a compressor |

Background, design and measurements:
[../docs/english_dic_reconstruction.md](../docs/english_dic_reconstruction.md) §7–8.
`dicrank3/` uses the same transform with a built-in entropy coder instead of cmix.

## Build

```sh
g++ -O2 -std=c++17 -o dicrank2 dicrank2.cpp
g++ -O2 -DDICRANK2_DEC_MAIN -o dicrank2_dec dicrank2_dec.cpp   # the lean decoder as a command-line tool
```

## Usage

```sh
dicrank2 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] TEXT DICTIONARY SIDEFILE   # encode
dicrank2 d [-v] TEXT SIDEFILE DICTIONARY                                           # decode
dicrank2_dec TEXT SIDEFILE DICTIONARY                                              # decode, lean decoder
```

A typical round trip:

```sh
./dicrank2 e enwik9 english.dic dict.rank     # ~45 s + ~40 s self-check, 1.5 GB
./cmix -c dict.rank dict.rank.cmix            # fx2-cmix's cmix: 25,112 bytes
./dicrank2 d enwik9 dict.rank english.dic     # ~35 s, 1.5 GB; identical to the original
```

### Arguments

| Argument | Meaning |
|---|---|
| `e` / `d` | Encode (make a side file) or decode (rebuild the dictionary). |
| `TEXT` | The text the dictionary is rebuilt from, e.g. enwik9. See below. |
| `DICTIONARY` | The dictionary: the encoder's input, the decoder's output. |
| `SIDEFILE` | The side file: the encoder's output, the decoder's input. |

**TEXT.**

- It is read twice, as two prefixes:
  - the first `-n` bytes, for word statistics;
  - the first `-c` bytes, for context statistics.
- The decoder needs a text whose prefixes are identical to the encoder's.
  The rest of the file is never read.
- Words are split as cmix's word transform splits them:
  - A run of letters is a word. Any other byte ends it.
  - A capital after lowercase letters starts a new word (`McDonalds` →
    `mc`, `donalds`).
  - A lowercase letter after two or more capitals starts a new word
    (`JScript` → `js`, `cript`).
  - Words are lowercased.

**DICTIONARY.**

- Format: lowercase `a`–`z` words, one per line, no duplicates, ending with
  a newline.
- The word order matters, and the decoder reproduces it exactly.
- Dictionary words that do not occur in the counted prefix of TEXT are
  allowed. They are stored spelled out in the side file ("extra words").

### Options

These apply to the encoder. The decoder takes every parameter from the side
file's header: it accepts `-n`, `-c`, `-k` and `-s` but ignores them, and
only `-v` has an effect.

| Option | Default | Meaning |
|---|---|---|
| `-n BYTES` | 100000000 (enwik8) | **Counted prefix.** Word counts, document frequencies and case statistics come from the first BYTES of TEXT. Capped at the text size. |
| `-c BYTES` | all of TEXT | **Context prefix.** The neighbour counts behind the context vectors come from the first BYTES of TEXT. `0` means all. |
| `-k K` | 4000 | **Context words**, 1 to 32768: the K most frequent words of the counted prefix. |
| `-s WORDS` | 3920 | **Explicit words:** the number of leading dictionary words kept in an explicit order. At most 10,000, and never more than the dictionary. |
| `-v` | off | Prints each stage's time to stderr. |

**What the counted prefix (`-n`) determines:**

- **The candidate list.** Every word of the prefix, by descending count,
  ties alphabetical. The side file refers to words by their rank in this
  list. Extra words come first.
- **The bitmap keys.** Document frequency (the number of `<page>`s a word
  occurs in; text before the first `<page>` counts as one page) and the
  lowercase share are two of the keys that order the membership bitmap.
- **Which words are context features** for `-k`.

**Context vectors (`-c`, `-k`).** A word's context vector records its
immediate left and right neighbours, whenever the neighbour is one of the K
context words. That gives 2K features, weighted by positive pointwise mutual
information and normalized. Similarity is the cosine between two vectors.

- More context words give sharper vectors, but need more memory.
- The neighbour counts take (dictionary words) × 2K × 4 bytes, in the
  encoder and the decoder alike. For english.dic that is 1.4 GB at
  K = 4000.
- The time for `-c` grows with its length. A 1 GB prefix takes about 21 s.

**Explicit words (`-s`).**

- **How the two parts are stored.** The first S words are stored as an
  explicit order. The rest are stored as membership plus alphabetical runs.
- **Why 3920.** In english.dic the first 3,920 words are cmix's 1- and
  2-byte codeword tiers (80 + 3,840), and they are ordered by hand, by
  meaning. The words after them form 510 alphabetical runs of one word
  class each.
- **Memory.** The explicit words' similarity matrix takes 4·S² bytes, plus
  2K·S·4 bytes for a copy of their vectors: 61 MB + 125 MB at the defaults.
  That is why S is capped at 10,000.

### What the parameters change

Measured on english.dic with enwik9 as TEXT. "After cmix" is fx2-cmix's
`cmix -c` on the side file. Most runs were three at a time on a 4-core
machine, so the times are rough. The self-check roughly doubles them.

| Options | Side file | After cmix | Encode time | Peak memory |
|---|---:|---:|---:|---:|
| defaults | 172,252 | **25,112** | 43 s | 1.5 GB |
| `-k 2000` | 173,110 | 26,145 | 35 s | 0.8 GB |
| `-k 8000` | 171,772 | 24,485 | 58 s | 2.9 GB |
| `-k 16000` | 171,639 | 24,240 | 75 s | 5.6 GB |
| `-c 100000000` (contexts from enwik8 only) | 180,359 | 31,847 | 20 s | 1.4 GB |
| `-n 1000000000` (counts over all of enwik9) | 378,047 | 30,401 | 61 s | 1.6 GB |
| `-s 0` | 179,158 | 28,853 | 71 s | 1.5 GB |
| `-s 80` | 178,783 | 28,515 | 71 s | 1.5 GB |

- **`-k`: more context words keep helping, with diminishing returns.**
  - 8000 saves 627 bytes over the default and 16000 saves 872.
  - Memory grows in proportion, and the decoder needs as much as the
    encoder.
- **`-c`: contexts from all of enwik9 are worth 6.7 KB.** Rare words need a
  lot of text to get useful vectors.
- **`-n`: keep the counts on enwik8.**
  - Counted over all of enwik9, far more words rank ahead of the rarest
    dictionary words.
  - The explicit-word bitmap grows from 4,090 to 16,903 bits.
  - The membership bitmap grows from 67,343 to 260,353 bits.
- **`-s`: 3920 matches the dictionary.**
  - Its hand-ordered head does not fall into alphabetical runs: with
    `-s 0` there are 2,485 runs instead of 510.
  - Keeping only the 80 one-byte words explicit (`-s 80`) is almost as bad
    (28,515).

### Output and exit codes

The encoder prints one summary line to stderr:

```
44515 words: 3920 explicit, 40595 in 510 runs, 0 not in the text; side file 172252 bytes; encode 43.0s, check 38.1s
```

- **explicit:** S.
- **in N runs:** the other words, and the number of alphabetical runs they
  form.
- **not in the text:** the extra words.
- **check:** the time of the self-check.

The side file is written only after the check passes.

With `-v`, each stage's time is printed as it finishes. The times below are
from decoding english.dic; the encoder runs the same stages, then all of
them again for the self-check.

| Stage | Time | What it does |
|---|---:|---|
| `word counts` | 1.3 s | Reads the counted prefix: counts, document frequencies, case |
| `context vectors` | 20.9 s | Scans the context prefix and weights the neighbour counts |
| `explicit word similarity` | 5.9 s | Builds the S × S similarity matrix |
| `explicit word order` | 0.1 s | Orders the explicit words |
| `run assignment` | 4.6 s | Assigns the other words to runs |

**Exit codes.**

- `dicrank2` returns 0 on success, 1 on an error (with a message
  `dicrank2: ...`) and 2 on a usage error.
- `dicrank2_dec` and the `Dicrank2Decode()` function return:
  - 0: success;
  - 1: the side file cannot be read;
  - 2: the side file is malformed or does not fit the text;
  - 3: the text is missing or shorter than a prefix, or the neighbour
    counts do not fit in memory;
  - 4: the dictionary cannot be written.

## The side file

It is plain text, so the compressor's own cmix can compress it well:

```
dicrank 2
<-n bytes> <-c bytes> <K> <S> <runs> <extra words>
<the extra words, one per line>
<explicit-word bitmap over ranks 0..the last explicit word: 1 = explicit>
<the first explicit word's position among the explicit words, by rank>
<S-1 lines: each next explicit word's position among the remaining ones>
<the number of candidates in the next bitmap>
<membership bitmap over the other candidates: 1 = dictionary word>
<one line per dictionary word outside the explicit ones, most frequent first>
```

**How each part is predicted:**

- **Explicit words.** Each next explicit word's position counts from the
  top of the remaining explicit words. They are sorted by
  2 · cos(previous word) + cos(the word before that), and ties go to the
  lower rank. With `-s 0` the bitmap line is empty and the position lines
  are absent.
- **Membership bitmap.** The bits are ordered by these keys, then by rank:
  1. word length / 3 (lengths over 12 count as 12);
  2. log2 of document frequency;
  3. log2 of count;
  4. the lowercase share, in five steps.

  That groups the words with similar chances of being in the dictionary.
- **Run lines.** Each word, in order of frequency, gets a line:
  - the position of its run among the runs started so far, sorted by the
    cosine between the word and each run's centroid, with ties going to the
    lower run number;
  - or `n<k>` when the word starts the k-th run not started yet.

The decoder recomputes the same vectors and scores, so the numbers are
small. For english.dic:

- 43% of the run positions are 0;
- half of the explicit-word positions are below 16.

Runs are numbered in dictionary order, and each run's words are sorted
alphabetically on output.

## Linking into a compressor

`dicrank2_dec.cpp` provides

```c
int Dicrank2Decode(const char* text_path, const char* side_path, const char* out_path);  // 0 on success
```

It uses only C stdio, `malloc` and `qsort`.

- **Code size.** In an fx2-cmix build it adds 5,832 bytes of packed code
  with UPX 3.95 `-9`, or 5,432 with UPX 4.2.4 `--ultra-brute`.
  `dicrank2.cpp`'s decoder would add 15,804 bytes. The program is stored
  twice, in the compressor and in archive9.
- **Cost.** It decodes english.dic from enwik9 in 37–45 s, with a 1.39 GB
  peak. The memory is freed before it returns.

To use it in the compressor:

1. Store the cmix-compressed side file in place of the compressed
   english.dic.
2. Unpack it with `cmix -d`.
3. Call `Dicrank2Decode("enwik9", side, ".dict")`.
4. Continue as before.

archive9 still needs the dictionary itself, because it has no enwik9.

**Net effect on fx2-cmix's entry size:** −63,312 bytes. The side file
replaces english.dic in the compressor, and the decoder's code is counted
twice. See [../docs/hp_entry_size.md](../docs/hp_entry_size.md) §8.7 and
[../docs/english_dic_reconstruction.md](../docs/english_dic_reconstruction.md)
§8.

## Determinism

No floating point is used on the data path.

- **Weights.** They use a fixed-point log2 computed with 128-bit products.
- **Vector lengths.** They use an exact integer square root, rounded up.
- **Scores.** Run scores use 128-bit intermediates.
- **Ties.** They are broken by index.

These builds produce byte-identical side files, and each rebuilds
english.dic exactly: g++ `-O2 -march=native`, clang++ `-O3`,
g++ `-O1 -fno-tree-vectorize`, and clang++ `-O2 -march=x86-64-v3
-ffast-math`. Differently compiled `dicrank2_dec` builds decode them
exactly too.
