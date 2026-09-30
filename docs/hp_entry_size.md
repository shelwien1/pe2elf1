# How the Hutter Prize counts entry size: fx2-cmix, cmix-lex, fx2-cmix-transformer

Investigation date: 2026-09-30. Everything below comes from the entries' build
scripts and self-extraction code, and from parsing the published `cmix` and
`archive9` files byte by byte. No entry was re-run on enwik9.

## Summary

- **The score is S = size of the compressor file + size of archive9.**
  Neither file is compressed any further for scoring.
  - fx2-cmix: 441,463 + 110,351,665 = **110,793,128**, which is the current
    official record.
- **What each file holds:**
  - The compressor holds the UPX-packed program, `english.dic` compressed by
    the program itself, and the article order compressed the same way.
  - archive9 holds a byte-identical copy of the same program and compressed
    dictionary, plus the compressed enwik9.
  - fx2-cmix-transformer adds a third shared part: 2.93 MB of transformer
    weights in both files.
- **What is counted twice:**
  - The program and the compressed dictionary, 240,408 bytes for fx2-cmix.
  - For fx2-cmix-transformer, also the weights: 3.23 MB in total.
- **What is counted once:** the article order. Decompression doesn't need
  it, because articles are put back in order by sorting on `<id>`.
- **Everything except the enwik9 stream** is 0.62% of S for fx2-cmix,
  0.66% for cmix-lex and 6.6% for fx2-cmix-transformer.

## 1. The rule

From <http://prize.hutter1.net/hrules.htm>:

> Publish a compression program comp9.exe that outputs archive9.exe given
> input enwik9. [...] Total size is measured as
> S := length(comp9.exe/zip)+length(archive9.exe).

- **Alternative form:** if the decompressor is a separate program,
  S = comp9a + 2 × decomp9 + archive9.bhm.
- **Source instead of executables:** a zip of the source code and makefile
  may be submitted, and its size is counted instead.
- **Command-line options:** "If command-line options for execution or
  compilation are necessary, their length is added to S." In practice,
  nothing was added for the `-e enwik9 enwik9.comp` arguments.

All the entries below use the first form. They submit a compressor
*executable* (not a source zip) that writes a self-extracting `archive9`.

The FAQ (`hfaq.htm#addcomp`) says outright that counting both the compressor
and the decompressor is meant to make built-in data expensive:

> By counting both, L(C) and L(D), such tables become 2-3 times more
> expensive, and hence discourages them. If you need large (word) tables,
> write a short program that creates them from enwik9 itself.

Matt Mahoney's Large Text Compression Benchmark (LTCB) scores these entries
differently. It counts only archive9 (the decompressor size is shown as
`0 xd`) and lists the compressor size separately "because it is used for
scoring in the Hutter prize".

## 2. Comparison

All sizes are in bytes. "S1" is the compressor file and "S2" is archive9.

| Part | fx2-cmix | cmix-lex | fx2-cmix-transformer, 24 Jul 2026 | fx2-cmix-transformer, 21 Aug 2026 |
|---|---:|---:|---:|---:|
| LTCB entry | [#1103](https://www.mattmahoney.net/dc/text.html#1103) | [#1091](https://www.mattmahoney.net/dc/text.html#1091) | [#0969](https://www.mattmahoney.net/dc/text.html#0969) | [#0969](https://www.mattmahoney.net/dc/text.html#0969) |
| Program (UPX-packed ELF), in S1 and S2 | 140,320 | 159,396 | 196,708 | 194,836 |
| english.dic, compressed by the program, in S1 and S2 | 100,088 | 100,776 | 100,091 | 100,096 |
| Transformer weights (`FX2TFWC2`), in S1 and S2 | — | — | 2,930,652 | 2,930,652 |
| new_article_order, compressed by the program, S1 only | 201,043 | 199,754 | 201,007 | 201,042 |
| Size header at the end of each file | 12 | 12 | 16 | 16 |
| **S1: compressor `cmix`** | **441,463** | **459,938** | **3,428,474** | **3,426,642** |
| Compressed enwik9, S2 only | 110,111,245 | 108,929,925 | 93,768,731 | 93,768,588 |
| **S2: `archive9`** | **110,351,665** | **109,190,109** | **96,996,198** | **96,994,188** |
| **S = S1 + S2** | **110,793,128** | **109,650,047** | **100,424,672** | **100,420,830** |
| Stored twice (program + dictionary + weights) | 240,408 | 260,172 | 3,227,451 | 3,225,584 |
| Everything except the enwik9 stream (share of S) | 681,883 (0.62%) | 720,122 (0.66%) | 6,655,941 (6.63%) | 6,652,242 (6.62%) |
| Improvement over the fx2-cmix record, 1 − S/110,793,128 | — (this is the record) | 1.032% (official figure: 1.012%) | 9.358% | 9.362% |
| Published figures | HP record: 110,793,128 | LTCB "official size": 470,599 + 109,201,040 = 109,671,639, from the committee's rebuild | LTCB: compressor 3,428,474, archive9 96,996,198 | LTCB: compressor 3,426,642, archive9 96,994,188 |

The cmix-lex and fx2-cmix-transformer columns are measured from the files
the authors published. As of 2026-09-30 the Hutter Prize page still lists
fx2-cmix as the latest record, and the next record must be below
109,685,197.

Build and packing details:

| | fx2-cmix | cmix-lex | fx2-cmix-transformer, 24 Jul | fx2-cmix-transformer, 21 Aug |
|---|---|---|---|---|
| Program size before packing | 294,952 | 387,736 | 450,840 | 438,552 |
| Packer | UPX 3.95 `-9`, NRV2B | UPX 5.1.1 `--ultra-brute`, LZMA | UPX 3.95 `-9`, NRV2B | UPX 3.95 `-9`, NRV2B |
| Compiler, linking | clang 17.0.6 with profile-guided optimization, dynamically linked (libstdc++, libm, libgcc_s, libc) | clang-17 per its makefile (version not readable: `.comment` and notes stripped), otherwise same | same as fx2-cmix | same as fx2-cmix |
| Size header fields | dict, order, input | dict, order, input | dict, order, input, weights | dict, order, input, weights |
| english.dic source | md5 `b3a9cf9f…` | same file | same file | same file |
| new_article_order source | md5 `68fa9a4f…` | md5 `b5c1e116…` (the order from fxcm_v26) | same as fx2-cmix | same as fx2-cmix |
| enwik9 size after the word transform (from the stream header) | 586,459,020 | 587,138,826 | 586,459,020 | 586,459,020 |
| Embedded dictionary decodes on Linux 6.18, Intel Xeon | only with `tools/pinmmap.c`, then md5-identical | yes, md5-identical | only with `tools/pinmmap.c`, and even then **wrong** (see §7) | yes, md5-identical |
| Reciprocal-estimate instructions (results differ between Intel and AMD) | 18 | 13 | 30 | 0 |
| AVX-512 instructions (those using zmm or mask registers) | 816 | 0 | 4,988 | 0 |

The three repos share the same `english.dic`, but each build compresses it to
a slightly different size. The dictionary and order file are compressed by
each build's own model, and the model code and compiler flags differ between
builds.

## 3. fx2-cmix in detail

The container format is inherited unchanged from starlit (May 2021).
cmix-hp and fx-cmix use the same layout and 12-byte header (checked in
their `self_extract.h`). cmix-lex keeps it too, and fx2-cmix-transformer
adds a fourth field for the weights.

### 3.1 Compressor `run/cmix` (S1 = 441,463)

Built by `build_and_construct_comp.sh`:

```sh
make ... prof_gen; ./cmix -c prof_input/input ...; make ... prof_use  # PGO build, linked with -s
upx-ucl -9 cmix; cp cmix run/cmix_orig
./cmix_orig -c english.dic comp_dict                                  # 100,088
./cmix_orig -c new_article_order comp_order                           # 201,043
./cmix_orig -h 100088 201043 0                                        # header.dat (12 bytes)
cat cmix_orig comp_dict comp_order header.dat > cmix
```

| Offset | Size | Part |
|---:|---:|---|
| 0 | 140,320 | Program: 294,952 bytes before packing (.text 230,869, .eh_frame + .eh_frame_hdr 21,212, .rodata 13,040), packed with UPX 3.95 NRV2B |
| 140,320 | 100,088 | english.dic (411,996 bytes, 44,515 words) compressed with `cmix -c` |
| 240,408 | 201,043 | new_article_order (1,094,862 bytes, 172,277 article numbers) compressed with `cmix -c` |
| 441,451 | 12 | Header, three int32 little-endian: 100088, 201043, 0 |

### 3.2 archive9 (S2 = 110,351,665)

`cmix -e enwik9 enwik9.comp` (`src/runner.cpp`) first calls
`selfextract_comp()`. That splits the compressor file (opened by the name
`"cmix"`) into `.decomp_bin`, `.dict.comp` and `.new_article_order.comp`,
then unpacks the last two by running `system("./cmix -d ...")`. After
compressing enwik9, it writes:

```c
// archive9 = decomp_binary(upxed) + comp_dict + cmix_output + header.dat
cat(".decomp_bin", ".dict.comp", "dec1");
header.decomp_input_size = output_size;     // other fields copied from the compressor's header
cat("dec1", output_path, "dec2"); cat("dec2", "header4archive.dat", "archive9");
```

| Offset | Size | Part |
|---:|---:|---|
| 0 | 140,320 | Program, copied unchanged from the compressor |
| 140,320 | 100,088 | Compressed english.dic, copied unchanged from the compressor |
| 240,408 | 110,111,245 | Compressed enwik9 |
| 110,351,653 | 12 | Header: 100088, 201043 (copied along, not used), 110111245 |

### 3.3 How each part is compressed

- **Program:** a stripped, profile-optimized clang-17 build, packed with UPX
  3.95 (method 2 = NRV2B, level 9, filter 0x49).
- **english.dic and new_article_order:** compressed by the entry's own full
  model (`num models 461`) with `-c` and no dictionary.
  - Neither file contains a space. cmix's text detector needs more than 500
    printable characters with at least 5 spaces, so both are stored as one
    plain binary block (a 5-byte type and length header, then the raw
    bytes).
  - As a result, there is no word transform and no pretraining for them.
    Decoded sizes are 412,001 and 1,094,867 bytes, i.e. each file plus 5.
  - Rates: 1.94 bits per character for the dictionary, 1.47 for the order
    file.
- **cmix stream format:** a 5-byte length whose top bit means "dictionary
  used", then a 32-byte bitmap of which byte values occur (for inputs of
  10,000 bytes or more), then the arithmetic code.
- **enwik9:**
  1. Split by line number into intro, main and coda.
  2. Articles in main are reordered with `new_article_order`. The numbers
     refer to non-redirect articles; the remaining articles (redirects) keep
     their original order and are appended.
  3. The phda9 transforms are applied.
  4. The parts are joined as main + intro + coda, giving `.ready4cmix` of
     934,220,400 bytes.
  5. cmix's word transform with english.dic (`Loaded 44515 words`) gives
     586,459,020 bytes.
  6. The model, pretrained on english.dic, compresses that to 110,111,245
     bytes.
- **Uses of the dictionary at runtime:** the word transform, `Pretrain()`,
  and the fxcm model, which opens `.dict` from the current directory for
  its reverse dictionary contexts.

### 3.4 What `./archive9` does when run

`selfextract_decomp()`:

1. Opens the file `"archive9"` by name, not `/proc/self/exe`. It must be run
   as `./archive9` from its own directory.
2. Reads the 12-byte header at the end. The program size is the file size
   minus the dictionary size, the stream size and 12.
3. Writes the dictionary stream to a file and runs
   `system("./archive9 -d .dict.comp_decomp .dict")`. In other words, the
   archive runs itself as a plain cmix decompressor, which takes about 2
   minutes.
4. Decodes the main stream, undoes phda9, and sorts articles back into place
   with a bubble sort on `<id>` (`article_reorder.h: sort()`).
5. Joins the parts into `enwik9_uncompressed`.

The article order is never needed to decompress, and that is why it is not
in archive9.

## 4. What is duplicated, and what it costs

- **Program and compressed dictionary** (plus the transformer weights, where
  present) are byte-identical in both files. Every byte of them costs 2
  bytes of S. Because the same program both compresses and decompresses,
  each copy also carries code only the other side uses.
- **Article order** is only in the compressor and costs 1 byte of S per
  byte.
- **Size header:** there is one in each file, but they hold different
  values.
- **fx2-cmix-transformer weights:** the writeup (`writeup.md`) says the
  quantized weights "only add 2.9MB each binary (or 5.9MB combined)".
  - They are 4-bit weights in the range −7..7, range-coded to about 3.79
    bits per weight.
  - The 2,930,652 bytes come from `models/6m-q4-fp32.tch`, which is
    23,868,862 bytes.
  - Counted twice they cost 5,861,304 bytes of S. That is still paid back
    well: the enwik9 stream shrinks by 16,342,657 bytes (110,111,245 →
    93,768,588).

## 5. What was verified, and how

Files downloaded:

- **fx2-cmix:** archive9 from Google Drive, id `14QillUEElT5vR0ttmayRAXlciXuPwDWm`.
- **cmix-lex:** Drive folder `1oCl3jVF90Dq_AHf33eemB3J3L8EDIKkE`. Its sha256
  values match the README.
- **fx2-cmix-transformer:**
  - Drive ids `1JsmbKfd…` / `1bZ957n0…` (archive9 / compressor) hold the
    24 Jul submission (files dated 25 Jul; 96,996,198 / 3,428,474).
  - Drive ids `1-y6FveF…` / `1ZWsVvu4…`, listed under "The original
    submission was posted to Google Drive", hold the 21 Aug fix (files
    dated 22 Aug; 96,994,188 / 3,426,642). They are byte-identical to the GitHub release
    `astOwOlfo/fx2-cmix-transformer-v1/releases/download/binaries/`.
- **fx-cmix:** archive9 from Drive folder `1ZbnnOhsF5dL4IMKiGrPCqqDimC1SVN69`.

Checks:

1. **Layout.** `tools/hp_entry_parse.py` splits every file exactly: the parts
   add up to the file size, the program ends with UPX's trailer, and each
   cmix stream header is valid.
2. **Shared parts.** For cmix-lex and both fx2-cmix-transformer versions,
   the program, the compressed dictionary and (for the transformer) the
   weights are byte-identical between `cmix` and `archive9`.
3. **fx2-cmix dictionary.** The program taken from archive9, run as
   `cmix -d` on the embedded dictionary stream, reproduces the repo's
   `dictionary/english.dic` (md5 `b3a9cf9fac570b2dc9a0375534792980`).
   117 s here; the README's log shows 94 s. The same check passes for
   cmix-lex and for fx2-cmix-transformer 21 Aug (see §7 for the 24 Jul build).
4. **fx2-cmix compressor.** The fx2-cmix compressor was never published as a
   file, so it was rebuilt:
   - The same program compresses the repo's `new_article_order` to exactly
     201,043 bytes, the value stored in archive9's header (235 s here; the
     README's log shows 227 s to decode it).
   - Program + dictionary stream + order stream + header(100088, 201043, 0)
     gives a 441,463-byte compressor, and 441,463 + 110,351,665 =
     110,793,128.

## 6. Caveats about the official numbers

- **fx2-cmix:** the LTCB text says "441,468 (compressor)". Only 441,463 adds
  up to the official 110,793,128. 441,463 is the README's S1, and it matches
  the rebuilt file.
- **cmix-lex:** the author's files give 459,938 + 109,190,109 = 109,650,047.
  The LTCB gives the official size as 470,599 + 109,201,040 = 109,671,639.
  - Those numbers come from James Bowery compiling from source and
    compressing on an AMD Ryzen 9 5900X.
  - So for cmix-lex the committee scored its own rebuild, not the author's
    files, and the rebuild produced a different compressor and archive.
- **fx-cmix** (previous record, 112,578,322) cannot be reconstructed from the
  public files:
  - The submission PDF says 436,707 + 112,148,343 = 112,585,050.
  - The published archive9 is 112,142,259 bytes. Its header implies a
    compressor of 140,544 + 101,034 + 197,114 + 12 = 438,704, i.e. a total of
    112,580,963.
  - The official total implies a compressor of 436,063 bytes.

## 7. Running these binaries on current systems

**Memory remap bug (fx2-cmix, fx2-cmix-transformer 24 Jul).**

- **The cause.** cmix's PPMd keeps its 14,000 MiB heap in a shared memory
  mapping of `ppm.temp`. Every 20,000 bytes it does `munmap()` and then
  `mmap(NULL, ...)`, but the suballocator keeps absolute pointers into that
  heap.
- **What happens here.** On Linux 6.18 the mapping comes back at a different
  address. Both binaries segfault at exactly 4.85% of the dictionary (byte
  20,000), which is the first thing archive9 decodes.
- **Reproduction.** `tools/remap_test.c` reproduces it without cmix. Once
  another mapping sits directly below the heap, the first remap lands about
  15.75 GB lower, below that neighbouring mapping. With the shim, no remap
  moves.
- **Likely mechanism.** The heap is placed on a 2 MiB boundary. Newer
  kernels appear to pad large file-backed mappings for huge-page alignment,
  so the exactly-sized hole left by `munmap()` no longer fits.
- **The workaround.** `tools/pinmmap.c` (used with `LD_PRELOAD`) forces the
  remap back to the old address. With it, fx2-cmix decodes its dictionary
  correctly.
- **Upstream fixes:**
  - cmix-lex replaced the cycle with `MADV_DONTNEED` on a mapping that stays
    in place (comment at the top of its `ppmd.cpp`).
  - fx2-cmix-transformer 21 Aug uses `MAP_FIXED` ("Mmap Can Change
    Addresses" in its writeup). Its binary runs here without the shim.

**Intel vs AMD results (fx2-cmix-transformer 24 Jul).**

- The 24 Jul program contains 30 reciprocal-estimate instructions
  (`vrcpps`, `vrsqrtps`, `vrsqrtss`, and AVX-512 `vrcp14ps`/`vrsqrt14ps`).
  These compute approximate reciprocals whose exact bits differ between CPU
  vendors.
- With the shim on this Intel Xeon, its embedded dictionary decodes to the
  right length but the wrong bytes: the first difference is at byte 243, and
  394,044 of 411,996 bytes differ.
- The 21 Aug build uses `-mrecip=none`, contains 0 such instructions, and
  decodes correctly.
- The entry's build script says this is what broke the July submission on
  the prize committee's Intel laptop. The LTCB reports a segmentation fault
  on a Core i7-1165G7.

**Other notes:**

- **Build targets.** fx2-cmix and cmix-lex build with `-march=native` by
  default and without `-mrecip=none`. fx2-cmix-transformer 21 Aug builds for
  `x86-64-v3` with `-mrecip=none`, and its build script refuses AVX-512 and
  reciprocal-estimate instructions.
- **fx2-cmix and cmix-lex may have the same Intel/AMD problem.** They also
  contain reciprocal-estimate instructions (18 and 13). Their archives may
  therefore only decode on the same CPU vendor they were built on. This was
  not tested here beyond Intel.
- **AVX-512.** The fx2-cmix and 24 Jul transformer programs contain AVX-512
  instructions (816 and 4,988), so they may not run on CPUs without AVX-512
  (e.g. AMD Zen 2 and Zen 3).
- The compressor opens itself as `"cmix"` and the archive as `"archive9"`,
  both from the current directory. Neither file may be renamed.

## 8. Where bytes could still be saved

Program bytes count twice, so packing matters twice as much:

| Program | As shipped | UPX 4.2.4 `--lzma --best` | UPX 4.2.4 `--ultra-brute` | Saving on S |
|---|---:|---:|---:|---:|
| fx2-cmix | 140,320 | 122,580 | 122,444 | 35,752 |
| fx2-cmix-transformer 21 Aug | 194,836 | 166,812 | 166,524 | 56,624 |
| cmix-lex | 159,396 (already LZMA `--ultra-brute`) | — | — | — |

- **cmix-lex** also strips `.comment` and `.note.*` before packing. Its build
  script says LZMA "is ~23 KB smaller than the default UCL mode".
- **Dictionary:** the FAQ suggests building tables from enwik9 itself. A
  dictionary that exists only in archive9, and is built by the compressor,
  would be counted once instead of twice.
- **Article order** already follows that pattern: it is counted once, since
  the decompressor rebuilds the original order from `<id>`.

## 9. Tools in this repo

- `tools/hp_entry_parse.py FILE...` splits a compressor or archive9 into its
  parts.
  - It detects the 12-byte and 16-byte size headers automatically.
  - It prints each part's size, md5, UPX status and cmix stream header.
  - It lists parts that are byte-identical across the files given.
  - `--dump` writes the parts to disk.
- `tools/pinmmap.c` is the `LD_PRELOAD` workaround from §7:
  `gcc -O2 -Wall -shared -fPIC -o pinmmap.so tools/pinmmap.c -ldl`, then
  `LD_PRELOAD=$PWD/pinmmap.so ./archive9`.
- `tools/remap_test.c` is a standalone reproduction of the remap move. Run it
  with and without the shim.

To decode an embedded dictionary with an entry's own program (about 2
minutes, 2.6 GB of memory, and a 14.7 GB sparse `ppm.temp`):

```sh
gcc -O2 -Wall -shared -fPIC -o pinmmap.so tools/pinmmap.c -ldl
SHIM=$PWD/pinmmap.so
python3 tools/hp_entry_parse.py --dump archive9    # writes archive9.elf, archive9.dict.comp, ...
mkdir t && cp archive9.elf t/cmix && cp archive9.dict.comp t/dict.comp && chmod +x t/cmix
cd t && LD_PRELOAD=$SHIM ./cmix -d dict.comp english.dic
```

The shim has no effect on cmix-lex or the 21 Aug transformer build, which
never re-map with `addr = NULL`.

## Sources

- Hutter Prize: <http://prize.hutter1.net/>, [rules](http://prize.hutter1.net/hrules.htm), [FAQ](http://prize.hutter1.net/hfaq.htm)
- LTCB entries: [#1103 fx2-cmix](https://www.mattmahoney.net/dc/text.html#1103), [#1091 cmix-lex](https://www.mattmahoney.net/dc/text.html#1091), [#0969 fx2-cmix-transformer](https://www.mattmahoney.net/dc/text.html#0969)
- Source: [kaitz/fx2-cmix](https://github.com/kaitz/fx2-cmix), [blahem/cmix-lex](https://github.com/blahem/cmix-lex), [astOwOlfo/fx2-cmix-transformer-v1](https://github.com/astOwOlfo/fx2-cmix-transformer-v1), [kaitz/fx-cmix](https://github.com/kaitz/fx-cmix)
