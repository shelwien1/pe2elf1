# dict_decoder

A small C++ decoder for the NNCP-style dictionary preprocessor format
(Fabrice Bellard's `preprocess`, an LZW-like scheme over a **16-bit
alphabet**). It reconstructs the original file from a dictionary (`dict`)
and a preprocessed symbol stream (`book1p`).

It is a clean, bit-exact reimplementation of the decode (`d`) path of
`preprocess.c`.

## Files

| File         | Purpose                                                                          |
|--------------|----------------------------------------------------------------------------------|
| `decode.cpp` | Self-contained: the `DictDecoder` class **and** the driver (= `preprocess d`).   |
| `Makefile`   | `make` to build, `make test` to verify against `book1`.                          |
| `testdata/`  | Sample `dict`, `book1p`, and the expected `book1`.                               |

Everything is in the single translation unit `decode.cpp`: the
`DictDecoder` class (all methods defined inline in the class body) followed
by `main()`. There is no separate header or implementation file, and **no
STL** — the dictionary lives in plain `malloc`'d tables and the output goes
through a fixed global byte array, mirroring the plain-C original.

## Build & test

```sh
make            # builds ./decode
make test       # decodes the sample and checks it equals testdata/book1
```

`make test` prints `OK: output identical to book1` when the decoded output
is byte-for-byte identical to the reference file. The program also prints
`max out.size() = 20` — the largest number of bytes any single `unpack()`
ever produces for this sample (see below).

## The class

The `DictDecoder` class is defined at the top of `decode.cpp` (lift it into
your own project as-is if you want to reuse it). The core is
`DictDecoder::unpack`, which turns **one 16-bit symbol** into the **8-bit
bytes** ready to be written to the output file:

```cpp
#include <cstdio>
#include <cstdint>
// ... DictDecoder class (as in decode.cpp) ...

DictDecoder dec("dict");                 // load the dictionary

uint8_t out[OUT_MAX];                    // OUT_MAX >= dec.max_word_len()
// ... read a big-endian uint16 'sym' from book1p ...
int n = dec.unpack(sym, out);            // out[0..n) = decoded bytes
// ... fwrite(out, 1, n, fp); ...
```

Key methods:

- `int unpack(uint16_t sym, uint8_t* out)` — unpack one symbol into `out`,
  returning the number of 8-bit bytes written. This is the requested
  method: 16-bit symbol in, 8-bit buffer out. `out` must hold at least
  `max_word_len()` bytes.
- `void reset()` — reset the case/space state machine to decode a fresh
  stream with the same dictionary.
- `size_t load_dict(const char*)` — (re)load the dictionary; returns the
  entry count.
- `size_t symbol_count()` — number of dictionary entries (valid symbols are
  `0 .. symbol_count()-1`).
- `uint32_t max_word_len()` — longest dictionary word, i.e. an upper bound
  on what `unpack()` can write (so a safe `OUT_MAX`).

The case/space machine keeps state across symbols, so use one
`DictDecoder` instance for an entire stream (or call `reset()` between
independent streams).

### Buffer size

Because case/space decoding only ever **drops** a byte or maps it **1:1**
(it never inserts), one symbol expands to at most the length of its
dictionary word. The longest word in this sample is 20 bytes (symbol 592,
`",' said the maltster"`), which is why the driver reports
`max out.size() = 20`. The global `out[OUT_MAX]` array uses `OUT_MAX = 512`
— the encoder's own per-word limit (`SORT_MAX_LEN` in `preprocess.c`) — and
`main()` checks `max_word_len() <= OUT_MAX` at startup.

## Format summary

- **`book1p`** is a sequence of **big-endian 16-bit symbols**. Each symbol
  is an index into the dictionary.
- **`dict`** is a text file, one entry per line, separated by raw `\n`
  (`0x0a`). Within an entry, `\n` (backslash + `n`) means a newline byte
  and `\\` means a backslash byte; all other bytes are literal. Empty
  lines are skipped. The first 256 entries are the single bytes `0x00`..
  `0xff`; the rest are multi-byte words.
- Decoding is two stages:
  1. **symbol → dictionary word** — a short byte string that may embed the
     reserved control codes `1..4`.
  2. **case/space decode** — a small state machine restores the spaces and
     upper-case letters the encoder factored out:
     - `1` (`CH_NO_SPACE`) — suppress the space before the next word.
     - `2` (`CH_TO_UPPER`) — upper-case the following word.
     - `3` (`CH_FIRST_UPPER`) — upper-case only the first letter.
     - `4` (`CH_ESCAPE`) — the next byte is a literal reserved code.

## Reference

To regenerate the sample from the original NNCP tool (`preprocess.c`
includes `cutils.c` and `cp_utils.c`, so it builds standalone — only the
math library needs linking):

```sh
gcc -O2 -o preprocess preprocess.c -lm
preprocess c dict book1 book1p 4096 16   # encode (build dict + book1p)
preprocess d dict book1p out             # decode (out == book1)
```
