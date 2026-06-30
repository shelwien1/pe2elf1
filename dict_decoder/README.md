# dict_decoder

A small C++ decoder for the NNCP-style dictionary preprocessor format
(Fabrice Bellard's `preprocess`, an LZW-like scheme over a **16-bit
alphabet**). It reconstructs the original file from a dictionary (`dict`)
and a preprocessed symbol stream (`book1p`).

It is a clean, bit-exact reimplementation of the decode (`d`) path of
`preprocess.c`.

## Files

| File              | Purpose                                                        |
|-------------------|----------------------------------------------------------------|
| `DictDecoder.h`   | The decoder class (declaration + format notes).                |
| `DictDecoder.cpp` | Implementation.                                                |
| `decode.cpp`      | Driver, equivalent to `preprocess d <dict> <book1p> <out>`.    |
| `Makefile`        | `make` to build, `make test` to verify against `book1`.        |
| `testdata/`       | Sample `dict`, `book1p`, and the expected `book1`.             |

## Build & test

```sh
make            # builds ./decode
make test       # decodes the sample and checks it equals testdata/book1
```

`make test` prints `OK: output identical to book1` when the decoded output
is byte-for-byte identical to the reference file.

## The class

The core is `DictDecoder::unpack`, which turns **one 16-bit symbol** into
the **8-bit bytes** that can be written to the output file:

```cpp
#include "DictDecoder.h"
#include <vector>
#include <cstdio>

DictDecoder dec("dict");          // load the dictionary

std::vector<uint8_t> out;
// ... read a big-endian uint16 'sym' from book1p ...
dec.unpack(sym, out);             // append decoded bytes to 'out'
// ... fwrite(out.data(), 1, out.size(), fp); out.clear(); ...
```

Key methods:

- `void unpack(uint16_t sym, std::vector<uint8_t>& out)` — unpack one
  symbol, appending the resulting 8-bit bytes to `out`. This is the method
  requested: 16-bit symbol in, 8-bit buffer out.
- `void decode_stream(const uint8_t* be16, size_t nbytes, std::vector<uint8_t>& out)`
  — convenience helper that runs `unpack` over a whole big-endian symbol
  stream.
- `void reset()` — reset the case/space state machine to decode a fresh
  stream with the same dictionary.
- `size_t load_dict(const char*)` / `load_dict_mem(const uint8_t*, size_t)`
  — (re)load the dictionary.
- `size_t symbol_count()` — number of dictionary entries (valid symbols are
  `0 .. symbol_count()-1`).

The case/space machine keeps state across symbols, so use one
`DictDecoder` instance for an entire stream (or call `reset()` between
independent streams).

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
