# How the Hutter Prize counts entry size: fx2-cmix, cmix-lex, fx2-cmix-transformer

Investigation date: 2026-09-30. Everything below comes from the entries' build
scripts and self-extraction code, and from parsing the published `cmix` and
`archive9` files byte by byte. No entry was re-run on enwik9. To measure the
layout options in §8, fx2-cmix was also rebuilt from source.

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
- **Layout alone can save about 48 KB on fx2-cmix** (§8), through stronger
  packing and build flags. With the compressor submitted as a zip using LZMA
  compression, and a decode-only program in archive9, the saving is about
  66 KB.
  - A decode-only program only pays off if the compressor is submitted as a
    source zip.
  - The separate-decompressor relaxation makes S larger.
- **The compressor's copy of english.dic can be replaced by a 45.6 KB side
  file** (`tools/dicrank.cpp`). The compressor expands it using the word
  statistics of enwik8. That saves about 50 KB of S on top of the layout
  changes (§8.7).

## 1. The rule

From <http://prize.hutter1.net/hrules.htm>:

> Publish a compression program comp9.exe that outputs archive9.exe given
> input enwik9. [...] Total size is measured as
> S := length(comp9.exe/zip)+length(archive9.exe).

- **Alternative form:** if the decompressor is a separate program,
  S = comp9a + 2 × decomp9 + archive9.bhm. The factor is 1× when comp9a
  and decomp9 are the same program. §8 compares the methods.
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
  each copy also carries code only the other side uses. §8.2 measures how
  much a decode-only program would save.
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

## 8. Improving the score through layout alone

This section keeps the algorithm and its output fixed and asks only how the
parts are packaged and counted. The worked example is fx2-cmix, where

S = 2P + 2D + O + X + 24

- P = packed program, 140,320
- D = dictionary stream, 100,088
- O = article-order stream, 201,043
- X = enwik9 stream, 110,111,245
- 24 = the two 12-byte headers

Sizes marked *rebuild* come from building fx2-cmix from source here, with
clang 18 instead of 17, the entry's own makefile flags, and PGO (the profile
comes from the entry's `prof_input/input`). They are applied as differences to
the shipped binary, so they are estimates. Everything else is measured on the
published files.

At the current record, 1 byte of S is worth about €0.0045 (€1 per ~220
bytes). More important is the 1% threshold: cmix-lex's official total cleared
it by only 13,558 bytes. Savings of 50-65 KB can decide whether an entry
qualifies at all.

### 8.1 The accounting methods the rules allow

| Method (hrules.htm) | Formula | fx2-cmix in these terms |
|---|---|---|
| Main rule: compressor that writes a self-extracting archive | S = comp9 + archive9 | (P + D + O) + (P + D + X) |
| Relaxation: separate decompressor | S = comp9a + 2 × decomp9 + archive9.bhm | (P + D + O) + 2 × decomp9 + archive9.bhm |
| Relaxation, same program for both | S = comp9a + decomp9 + archive9.bhm, i.e. 2 × program + archive9.bhm | 2 × (P + D + O) + X |
| Source zip | a zip of "the source code and makefile, which create X.exe" may replace comp9, comp9a or decomp9, but not archive9 | zip replaces P + D + O |
| Options | "If command-line options for execution or compilation are necessary, their length is added to S" | fx2-cmix's `-e enwik9 enwik9.comp` was not charged |

Two older variants are commented out in the page's HTML and are no longer
rules: "decomp9 plus archive9.bhm" without the 2×, and publishing only
archive9's size. The FAQ explains the 2×: if a separately submitted
decompressor were counted once, it could simply contain the archive.

### 8.2 Is the program counted twice? Would a decode-only program help?

Yes: the same packed program is in both files, so every byte of it costs 2
bytes of S.

**How it was measured.** A small patch, `tools/fx2-cmix-decode-only.patch`,
makes `main()` keep only the self-extract and `-d` paths. The linker's
`--gc-sections` then drops everything only the compressor uses: the forward
phda9 transforms, the article reordering, the dictionary encoder, the
arithmetic encoder and the other modes.

- Both builds' output was checked: the decode-only builds restore data
  compressed by the full build exactly.
- Builds without unwind tables produce byte-identical compressed output.

| Rebuild, with PGO | Unpacked | UPX 3.95 `-9` | UPX 4.2.4 `--ultra-brute` |
|---|---:|---:|---:|
| Full program | 303,152 | 134,996 | 123,660 |
| Decode-only | 274,304 | 125,212 | 114,772 |
| Full program, no unwind tables | 282,352 | 128,008 | 117,752 |
| Decode-only, no unwind tables | 257,600 | 118,952 | 109,488 |

The decode-only program is 9.5% smaller before packing and about 7% smaller
packed (8,264-9,784 bytes, depending on packer and flags). Most of the
program is the CM model, which both directions need.

**The catch: the compressor has to produce it.** Under the main rule the
compressor must write archive9, so it has to contain the decode-only program
somehow:

- **As an extra embedded copy.** That adds ~108-125 KB to the compressor, a
  large net loss.
- **By restructuring the compressor** as "decode-only program + an add-on
  with the encode-only code". archive9 and the compressor each save ~8.3 KB,
  but the compressor must also ship the add-on.
  - The encode-only code is 24,752 bytes unpacked (the no-unwind-table rows
    above). As a separately packed program it needs its own ELF and UPX
    overhead.
  - The net result is roughly break-even, a few KB at best (estimate, not
    built).
- **For free, if the compressor is submitted as a source zip (§8.4).** The
  compressor's own binary isn't counted then, and its build can produce both
  programs.

Under the separate-decompressor relaxation, the decompressor counts twice,
which is worse (§8.5).

### 8.3 Measured savings that leave the compressed output unchanged

| Change | Per copy of the program | On S | Basis |
|---|---:|---:|---|
| Pack with UPX 4.2.4 `--ultra-brute` (LZMA) instead of UPX 3.95 `-9` | −17,876 | −35,752 | Shipped binary: 140,320 → 122,444 |
| Also build with `-fno-asynchronous-unwind-tables -fno-unwind-tables`, link with `--no-eh-frame-hdr --build-id=none`, drop `.comment` | −5,908 | −11,816 | Rebuild at `--ultra-brute`. Code uses `-fno-exceptions`; output verified identical |
| Decode-only program in archive9 | −8,264 | archive9 only; see §8.2 for the compressor | Rebuild, both without unwind tables, `--ultra-brute` |
| Custom LZMA packer instead of UPX | at most −3,584 | at most −7,168 | Raw `xz --x86` of the unpacked program is 118,860 vs UPX's 122,444; a packer's own loader eats into this |

The same repack saves 56,624 on fx2-cmix-transformer 21 Aug (194,836 →
166,524 per copy). cmix-lex already packs with LZMA `--ultra-brute` and
strips `.comment` and notes.

### 8.4 Submitting the compressor as a source zip

The rules allow a zip of the source and makefile in place of comp9.exe. Its
size then replaces S1. archive9 is still the executable that the built
compressor writes. The build needs 75 source files, 512,206 bytes in total.

| Zip contents (all include makefile, build script, PGO input, and the dictionary and order streams stored uncompressed, 301,131 bytes) | Zip size | Excluding the two streams |
|---|---:|---:|
| The 75 source files as published, `zip -9` | 455,159 | 154,028 |
| All sources concatenated into one file, comments and indentation removed, `zip -9` | 425,171 | 124,040 |
| Same, but compressed with LZMA inside the zip (zip method 14) | 407,054 | 105,923 |
| For comparison: compressor as shipped / repacked with `--ultra-brute` | 441,463 / 423,587 | 140,332 / 122,456 |

- **A plain source zip is larger than the packed program.** Only a single
  concatenated file (deflate has no context across zip members, and each
  member costs about 100 bytes of headers) gets below the shipped
  compressor. Against a well-packed executable it is about break-even with
  deflate, and about 16.5 KB smaller with LZMA-in-zip.
- **The zip has to carry the cmix-compressed streams, not the raw files.**
  With deflate, english.dic is 175,133 bytes and the order file 466,163,
  instead of 100,088 and 201,043.
  - That requires a bit-exact reproducible build: the committee's build must
    decode the author's streams.
  - It is fragile. On a 50 KB input, the PGO-instrumented and PGO-optimized
    builds of the same source produce 6,140 vs 6,139 bytes.
  - The compiler version, the `-march` target (not `native`) and
    `-mrecip=none` would all have to be pinned, as fx2-cmix-transformer 21
    Aug does.
- **The real benefit is the decode-only archive program (§8.2).** The build
  script can build both programs and embed the decode-only one in the
  compressor at no cost to S.
- **Open questions for the committee:**
  - Does a concatenated, comment-stripped file count as "source code"? The
    documented source must be published anyway before payment.
  - Does LZMA-in-zip count as a zip file?
  - Note that the committee rebuilt cmix-lex from source and scored the
    compressor *executable* it built (470,599), not a zip.

### 8.5 Why the separate-decompressor relaxation doesn't help here

- **Same program for both jobs.** S = 2 × (P + D + O) + X. The article
  order is now counted twice: **+201,043**.
- **Separate decode-only decompressor.** The best case puts the dictionary
  in archive9.bhm (counted once) and uses the decode-only program:
  S = comp9a + 2 × P_d + (D + X). That is worse than the main rule by
  2 × P_d − P, about **+100 KB** at the same packing.
- **With zips.** Two copies of the zipped decoder source (about 115-124 KB
  each) cost more than one packed decode-only program (about 108 KB).
- **When it would help.** Only when the decompressor is much smaller than
  the compressor, i.e. asymmetric codecs. CM uses the same model in both
  directions.

### 8.6 Totals for fx2-cmix under each layout

| Layout | S1 | S2 | S | vs submitted |
|---|---:|---:|---:|---:|
| As submitted | 441,463 | 110,351,665 | 110,793,128 | — |
| Main rule, both copies repacked with `--ultra-brute` | 423,587 | 110,333,789 | 110,757,376 | −35,752 |
| Same, plus no unwind tables / `.comment` / build-id | 417,679 | 110,327,881 | 110,745,560 | −47,568 |
| Compressor as a zip (deflate), archive9 as submitted | 425,171 | 110,351,665 | 110,776,836 | −16,292 |
| Compressor as a zip (deflate), archive9 with a decode-only program, no unwind tables, `--ultra-brute` | 425,171 | 110,319,617 | 110,744,788 | −48,340 |
| Same, with LZMA inside the zip | 407,054 | 110,319,617 | 110,726,671 | −66,457 |
| Relaxation, comp9a = decomp9 = the shipped compressor | 441,463 + 441,463 | 110,111,245 | 110,994,171 | +201,043 |
| Relaxation, decode-only decomp9 counted twice, dictionary in archive9.bhm | 417,679 + 2 × 108,272 | 110,211,345 | 110,845,568 | +52,440 |

The decode-only program size, 108,272, is the shipped binary's
`--ultra-brute` size (122,444) minus the rebuild's 14,172-byte difference.

**Conclusion.** Keep the main rule.

- Packing and build flags alone give about −47.6 KB with no rule-interpretation
  risk.
- Submitting the compressor as a source zip adds almost nothing with deflate
  (−0.8 KB more). It adds −18.9 KB more with LZMA-in-zip, if that is
  accepted. Either way it needs a reproducible build.
- A decode-only archive program only pays off together with the zip route.

### 8.7 Rebuilding english.dic from enwik8 inside the compressor

The compressor has enwik9, and all 44,515 words of english.dic occur in its
first 10^8 bytes (enwik8). This holds when the text is split into words
exactly as cmix's word transform does it. A plain split into letter runs
misses 11 words, such as `cript` (from `JScript`) and `donalds` (from
`McDonalds`), which shows the dictionary was built with that same split.

So the compressor can carry a much smaller side file and rebuild english.dic
from it before compressing. archive9 still needs the full dictionary, because
it has no enwik9 yet.

**What the side file has to encode.** The dictionary's word order:

- **The first 3,920 words** (the 1- and 2-byte codeword tiers) are
  hand-ordered by meaning. They are stored as frequency ranks.
- **The other 40,595 words** form 510 alphabetical runs of one kind of word
  each: adverbs, inflected verbs, adjectives, plant names, medieval names,
  and so on.
  - They are stored as a membership bitmap over the frequency-sorted word
    list, plus a run number for each word.
  - The run numbers are listed in order of reversed spelling. Words with the
    same ending, which mostly belong to the same run, then sit next to each
    other.

| Side file for english.dic, compressed with fx2-cmix's own `cmix -c` | Size |
|---|---:|
| english.dic itself, as shipped | 100,088 |
| Every word as a plain frequency rank | 82,448 |
| Runs, run numbers in frequency order | 51,478 |
| Runs, run numbers in alphabetical order | 48,959 |
| Runs, run numbers move-to-front coded, reversed-spelling order | 47,738 |
| **Runs, run numbers in reversed-spelling order (`tools/dicrank`)** | **45,569** |
| The same, counting words over all of enwik9 instead of enwik8 | 50,272 |

- **Size breakdown of the chosen format.** Compressed separately, its three
  parts are 5,600 bytes (the first 3,920 words' ranks), 5,659 (bitmap) and
  34,360 (run numbers).
- **Prototype rows.** The rows between the plain ranks and the chosen format
  are prototype files with a one-line header. The tool's two-line header adds
  19 bytes.

**Net effect on S.** The compressor shrinks by 100,088 − 45,569 = 54,519
bytes. The rebuilding code is compressor-only, but in fx2-cmix's layout the
program is also copied into archive9, so that code counts twice.

- `tools/dicrank_dec.cpp` (no standard containers) adds 2,256 bytes per copy
  with UPX 3.95 `-9`, or 2,004 with `--ultra-brute`. The STL-based decoder in
  `dicrank.cpp` would add 12,116.
- Net: **about −50 KB** (−54,519 + 2 × 2,256 = −50,007).
- Together with the packing and build-flag changes of §8.3 it comes to about
  −98 KB (−47,568 − 54,519 + 2 × 2,004 = −98,079).

**What changes in the compressor:**

1. It stores the cmix-compressed side file where `comp_dict` was.
2. At `-e`, it unpacks the side file with `./cmix -d` (about a minute).
3. It rebuilds the dictionary with `DicrankDecode(enwik9, side, ".dict")`,
   which takes about 2 s.
4. It recreates the archive's dictionary stream with
   `./cmix -c .dict .dict.comp`. The shipped binary turns the rebuilt
   english.dic into a stream byte-identical to the one in archive9 (100,088
   bytes, 112 s here). So archive9 does not change, and compression takes
   about 3 minutes longer.

**Checks:**

- `dicrank` reproduces english.dic byte for byte from enwik8 and from enwik9.
  Both give the same side file, because only the first 10^8 bytes are read.
- `dicrank_dec` produces the same output as `dicrank d`. This was tested on
  english.dic, on dictionaries containing words that are not in the text, on
  a descending word list, and with `-s` = 0, 7, 80 and 100000.

**Where the remaining bytes are.** The run numbers (34 KB) dominate.
Spelling, word families and first occurrence predict them poorly: only 18% of
words share a run with their prefix word. But the runs are word classes, so a
model that compares the words' contexts in enwik9 could probably predict them
much better. The first 3,920 words' order costs close to what a random order
would.

[english_dic_reconstruction.md](english_dic_reconstruction.md) covers the
reconstruction in more detail, and ways to shrink the side file further.

- A v2 side file (`tools/dicrank2.cpp`, integer arithmetic only) predicts the
  order from word contexts in enwik9 and comes to 25,112 bytes.
- With its lean decoder (`tools/dicrank2_dec.cpp`, 5,832 packed bytes per
  copy), the compressor saves 63,312 bytes against english.dic, instead of
  50,007 with v1.

### 8.8 Ideas that would change the algorithm's inputs, not just the layout

- **Let the compressor build its own dictionary from enwik9**, as the FAQ
  suggests, instead of rebuilding english.dic as in §8.7. It would still
  have to be stored in archive9, and the word codes, and so X, would change.
- **Encode the article order more compactly.** It currently costs about 9.3
  bits per article.
- **Put code needed only after startup into a cmix-compressed overlay inside
  archive9.** This covers the inverse phda9 transform, article sorting and
  the word-transform decoder. It would save only a few KB.
- **Use a context-mixing packer for the program.** Its larger loader offsets
  much of the gain.

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
- `tools/fx2-cmix-decode-only.patch` adds a `-DDECODE_ONLY` build of
  fx2-cmix, used for the §8.2 measurements.
- `tools/dicrank.cpp` stores english.dic as a side file that is expanded using
  enwik8 or enwik9 (§8.7):
  - `dicrank e enwik9 english.dic dict.rank` encodes and checks its own
    output;
  - `dicrank d enwik9 dict.rank english.dic` decodes.
- `tools/dicrank_dec.cpp` is the same decoder without standard containers,
  for linking into a compressor: `DicrankDecode(text, side, out)`. Build it
  with `-DDICRANK_DEC_MAIN` for a command-line version.
- `tools/dicrank2.cpp` and `tools/dicrank2_dec.cpp` are the v2 side file:
  encoder/decoder, and the lean decoder (`Dicrank2Decode`). See
  english_dic_reconstruction.md §8.

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
