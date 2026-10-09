# The cmix executable: structure, data types, compression

This note describes `cmix` (454,912 bytes), the executable that x64pp was
last tuned on. It covers how the file is laid out, what kinds of code and
data it contains, what each part costs under xz with and without x64pp, and
what could make it smaller. The sources are IDA's listing of the binary,
`readelf`/`objdump`, and the compression measurements listed at the end.

Sizes marked "xz" are raw LZMA2 streams at `-9e`. They have no .xz
container and run about 50 bytes below an .xz file. On that scale the whole
file is 170,621 bytes and x64pp `-a` output (options `drum`) is 130,292.
Two kinds of cost appear in the tables:

* **xz alone**: the part compressed by itself.
* **in context**: how much the whole file's compressed size drops when the
  part is replaced by zeros (or, for code, by `int3`). This is what the part
  really costs inside the file. It is noisy for small parts (see 5.4).

## Summary

* The binary is **fx2-cmix**, a Hutter Prize style build of cmix: context
  mixing models, an LSTM, a PPMd model and an added transformer, plus the
  enwik9 preprocessing pipeline. It is C++ compiled by clang 17 with AVX2 and
  FMA, linked by GNU ld as a PIE against libstdc++, libm, libgcc_s and libc,
  and stripped.
* **Code** is 80% of the file (363,662 bytes of `.text`, 77,774
  instructions, 466 functions) and 87% of its plain xz size. VEX
  (AVX2/FMA) instructions are 24% of the instructions and take 33% of the
  code bytes.
* **After x64pp** the code streams cost about 115 KB of the 130 KB, and the
  opcode stream alone is 84 KB. The scalar code (341 functions with under 10%
  VEX bytes) is 56 KB, 43% of the output, at 11.5 bits per instruction. The
  AVX kernels repeat so much that they cost only 8.8 bits per instruction.
  The most expensive function is the wiki-text context model at 0x33590: 27
  KB of code, 10.5 KB compressed (8.1% of the output).
* **Data** after x64pp is 14.7 KB:
  * `.rodata`: 8.1 KB, of which strings are about 5.5 KB.
  * `.eh_frame`: 2.2 KB.
  * Dynamic-linking metadata (symbols, names, versions, PLT relocations and
    slots, `.dynamic`): about 2.5 KB.
  * Exception tables: 0.7 KB.
  * Relocations: 0.7 KB.

  The x64pp header adds another 1.3 KB.
* **What is left to gain while keeping xz** (section 5):
  * Data-side transforms: about 2.5-4 KB (2-3%). Each is small, and most of
    them would help every ELF file.
  * Code-side ideas: 1-2 KB, at higher effort and risk.
  * Rebuilding the binary, where that is allowed: about 3 KB of messages,
    help text and assert strings, plus unwind tables and the code behind
    unused options.

## 1. What the program is

The strings identify the parts:

* **Archive pipeline** (`main`, 0x1C770). The no-argument mode decompresses
  enwik9. It runs itself and `archive9` as subprocesses to unpack the
  dictionary and the article order (`./cmix -d .dict.comp .dict`,
  `./archive9 -d .dict.comp_decomp .dict`,
  `./cmix -d .new_article_order.comp .new_article_order`), and it reads the
  transformer weights (`.tfweights`). It then restores the article order
  and the XML. `-h` writes the Hutter Prize header
  (`comp_dict_size comp_new_order_size decomp_input_size tf_weights_size`).
* **Context-mixing models.** The RTTI and assert strings name `Model`,
  `ByteModel`, `ByteMixer`, `Mixer`, `MixerInput`, `Direct`,
  `Indirect<Nonstationary>`, `Indirect<RunMap>`, `Sparse`, `Match`,
  `ContextHash`, `Bracket`/`BracketContext`, `State`, `Nonstationary`,
  `RunMap`, `PPMD` (`N4PPMD4PPMDE`) and `LstmLayer`. The containers are
  `emhash6` hash maps and `llvm::SmallVector`.
* **Transformer**, `fx2::opt` from `cpp_infer/src`:
  * The weights file has the magic `FX2TFWC` and config blocks named
    `config.ints` and `config.kimi`.
  * Each layer has query/key/value projections and convolutions,
    forget- and output-gate projections (low-rank `up`/`down`), a beta
    projection, `dt_bias`, `log_baseline_decay_rate` and a fused norm gate.
    That is a gated linear-attention block in the style of Kimi Delta
    Attention.
  * The model also has an MLP, token and prior embeddings, skip connections
    and an unembedding.
  * Weights are stored as `ENC_INT4`, `ENC_BF16` or `ENC_PLANE4`, and
    activations are quantized to int8.
  * The kernels require `d_in == 192`. The vocabulary is the 205 byte values
    that occur in enwik9.
* **enwik9 preprocessing**: XML and wiki markup, redirects, timestamps,
  article boundaries, an English word model with a Porter2 (Snowball)
  stemmer, and dictionary coding.
* **Command line**: seven modes and nine flags, long explanatory error
  messages for invalid combinations, and a 53-line usage text.

## 2. File layout

| part | offset | size | contents | xz alone | in context | after x64pp |
|---|---:|---:|---|---:|---:|---:|
| ELF header + program headers | 0 | 792 | 13 program headers | 188 | 430 | unchanged |
| `.interp`, `.note.*` | 0x318 | 128 | loader path, GNU property, build ID (20-byte SHA-1), ABI tag | 133 | 126 | 133 |
| `.gnu.hash` | 0x3A0 | 40 | symbol hash table | 39 | 44 | 23 (recomputed) |
| `.dynsym` | 0x3C8 | 3,192 | 133 symbols | 373 | 925 | 373 |
| `.dynstr` | 0x1040 | 3,470 | 151 names | 1,293 | 1,262 | 1,293 |
| `.gnu.version` | 0x1DCE | 266 | version index per symbol | 117 | -46 | 117 |
| `.gnu.version_r` | 0x1ED8 | 400 | 4 libraries, 21 versions | 193 | 49 | 193 |
| `.rela.dyn` | 0x2068 | 7,824 | 326 relocations | 1,048 | 1,610 | 715 |
| `.rela.plt` | 0x3EF8 | 2,832 | 118 PLT relocations | 255 | 408 | 168 |
| padding | 0x4A08 | 1,528 | zeros up to the code page | | | |
| `.init`, `.plt`, `.plt.got` | 0x5000 | 1,939 | PLT stubs | 468 | 116 | code streams |
| `.text` | 0x57A0 | 363,662 | code | 148,119 | 148,340 | code streams |
| `.fini`, padding | 0x5E430 | 3,024 | | | | |
| `.rodata` | 0x5F000 | 25,648 | constants, strings, tables | 8,143 | 7,875 | 8,143 |
| `.eh_frame_hdr` | 0x65430 | 3,788 | FDE search table | 1,570 | 1,474 | 43 |
| `.eh_frame` | 0x66300 | 24,484 | 2 CIEs, 472 FDEs | 6,272 | 6,079 | 2,183 |
| `.gcc_except_table` | 0x6C2A4 | 1,336 | 11 LSDAs | 923 | 924 | 717 |
| padding | 0x6C7DC | 3,884 | zeros up to the data page | | | |
| `.init_array`, `.fini_array` | 0x6D708 | 56 | 7 constructor/destructor pointers | 46 | 24 | 28 |
| `.data.rel.ro` | 0x6D740 | 1,608 | vtables, typeinfo, pointer arrays | 377 | 320 | 30 |
| `.dynamic` | 0x6DD88 | 528 | 29 entries | 143 | 94 | 143 |
| `.got`, `.got.plt` | 0x6DF98 | 1,040 | 9 + 121 slots | 187 | 106 | 187 |
| `.data` | 0x6E3B0 | 1,036 | pointer arrays (word lists) | 238 | 183 | 30 |
| `.bss` | (0x6E7C0) | (150,176,136) | zero-initialized model tables, no file bytes | | | |
| `.comment` | 0x6E7BC | 84 | "GCC 14.2.0" (crt files), "clang 17.0.6" | 78 | 52 | 78 |
| `.shstrtab` | 0x6E810 | 297 | section names | 196 | 159 | unchanged |
| section headers | 0x6E940 | 1,984 | 31 headers | 361 | 307 | unchanged |

Plain xz spends 148.3 KB on the code and about 22.3 KB on everything else.
x64pp takes these to about 115.4 KB of code streams (-22%) and 14.7 KB of
data stream (-34%), plus a 1.3 KB header.

## 3. Code

### 3.1 Components

The binary is stripped, so functions are named by address. Roles come from
the strings and RTTI names each function references and from its instruction
mix. "Cost" is the in-context cost after x64pp `-odrum`, that is, how much
the output shrinks when the function is replaced by `int3`.

| function | bytes | insns | VEX bytes | cost | bits/insn | role |
|---|---:|---:|---:|---:|---:|---|
| 0x53750 | 29,632 | 4,717 | 53% | 3,078 | 5.2 | transformer setup: maps the weights, checks the config, walks the tensor names, repacks weights with 720 `vpermd` + 720 `vpblendd` (fully unrolled) |
| 0x33590 | 27,232 | 5,647 | 5% | 10,551 | 14.9 | wiki-text context model (`text`, `nowiki`, `math`, `pre`, `page`): 1,390 RIP-relative accesses to global state in `.bss`, 633 imm32, a 70-entry switch |
| 0x1DB50 | 21,488 | 3,916 | 47% | 4,961 | 10.1 | LSTM (`SmallVector<LstmLayer>`) |
| 0x30190 | 11,168 | 2,787 | 1% | 4,803 | 13.8 | English word model: Porter2 stemmer rules, word lists, 4 switch tables |
| 0x4D3F0 | 8,096 | 1,420 | 60% | 2,280 | 12.9 | float kernel (`vmulps`, `vfmadd213ps`, `vdivps`) |
| 0x26F60 | 7,776 | 1,707 | 20% | 3,205 | 15.0 | model code, no identifying strings |
| 0x4B910 | 6,880 | 1,284 | 78% | 787 | 4.9 | FMA kernel, 230 `vfmadd231ps`, unrolled |
| 0x23FD0 | 6,672 | 1,219 | 48% | 1,747 | 11.5 | float model code (`vmovups`, `vmovss`) |
| 0x2A080 | 4,848 | 1,076 | 12% | 1,749 | 13.0 | model code |
| 0x28DC0 | 4,800 | 1,054 | 36% | 1,882 | 14.3 | mixer (`SmallVector<Mixer>`) |
| 0x17160 | 4,432 | 1,052 | 0% | 912 | 6.9 | enwik9 XML reconstruction (`</page>`, `</revision>`, timestamps) |
| 0x425E8 | 4,407 | 1,005 | 8% | 1,655 | 13.2 | weights file reader: magic, tensor table, dtype and shape checks |

Other recognizable functions:

* `rms_norm_quant192_multi` at 0x4F4B0 (90% VEX).
* An int8 dot-product kernel pair at 0x46070 and 0x473F0 (`vpmaddubsw`,
  `vpmovzxwd`, `vpaddd`).
* Activation quantization at 0x503A0 (`vdivps`, `vmaxps`/`vminps`,
  `vcvtps2dq`, `vpackssdw`/`vpacksswb`).
* int8-to-float conversion unrolled 24 times at 0x48C00.
* Article boundary handling at 0x1A540.
* `main` at 0x1C770 (3,200 bytes), the archive pipeline.

The rest is mostly C++ library code: `std::vector`, `std::string`,
`emhash6` rehashing, iostreams, and exception landing pads.

Code by how much AVX it contains:

| functions | count | bytes | insns | cost after x64pp | bits/insn | share of output |
|---|---:|---:|---:|---:|---:|---:|
| AVX kernels (≥50% VEX bytes) | 47 | 101,536 | 17,353 | 19,138 | 8.8 | 14.7% |
| mixed (10-50%) | 77 | 103,010 | 20,952 | 30,092 | 11.5 | 23.1% |
| scalar (<10%) | 341 | 159,102 | 39,096 | 56,028 | 11.5 | 43.0% |

Costs measured this way do not add up exactly to the 115 KB of code
streams, because each group is removed while the others stay in place.

### 3.2 Instruction mix

The most common instructions overall:

| instruction | share | instruction | share |
|---|---:|---|---:|
| `mov` | 25.1% | `jmp` | 2.8% |
| `lea` | 6.2% | `push` | 2.2% |
| `cmp` | 5.5% | `xor` | 2.1% |
| `call` | 4.5% | `pop` | 2.0% |
| `add` | 4.0% | `jnz` | 2.0% |
| `vmovups` | 3.9% | `vmovss` | 1.8% |
| `jz` | 3.1% | | |

The most common instructions in the AVX kernels:

| instruction | share | instruction | share |
|---|---:|---|---:|
| `vmovups` | 9.5% | `vfmadd231ps` | 2.9% |
| `mov` | 7.8% | `vfmadd213ps` | 2.5% |
| `vmovaps` | 5.1% | `vdivps` | 2.1% |
| `vmovdqu` | 5.1% | `vbroadcastss` | 1.7% |
| `vpermd` | 4.2% | `prefetcht0` | 1.4% |
| `vpblendd` | 4.2% | `vpmaddubsw` | 1.3% |
| `vmulps` | 3.5% | | |

The kernels use all 16 ymm registers, so the same operation appears with
many register combinations.

Operands:

* 3,441 direct calls.
* 10,109 disp32 that are not RIP-relative.
* 4,166 imm32.
* 4,040 RIP-relative references to 1,440 distinct addresses:
  * `.bss` globals: 2,194 references to 551 addresses, mostly `mov`/`lea`.
  * `.rodata`: 1,614 references to 722 addresses. These are strings and
    tables (806 `lea`) and float constants (`vbroadcastss`, `vmovss`,
    `vmulss`).
  * `.got`, `.got.plt` and `.data.rel.ro`: 211 references. These are the
    PLT's indirect jumps, vtable addresses and loads of external data such
    as `stdout`.

### 3.3 Patterns that matter for compression

* **Unrolled AVX2 loops.** 9.7% of instructions sit in periodic runs, where
  the same instructions repeat with displacements shifted by a constant.
  Plain LZMA loses its match at every changed displacement. x64pp's `d`
  (disp32 as a delta per base register) and `m` (imm8/disp8 out of the
  opcode stream) turn the iterations into exact repeats. That is most of the
  2.6% `-a` gains over the default.
* **Unrolled initializers.** 0x5BEB0 zeroes an array of 56-byte structs
  with four stores per element: 453 instructions, 13 distinct skeletons, and
  442 disp32 values in arithmetic progression.
* **Near-duplicate functions.** These are template instances for different
  types or sizes. 12 pairs share 85% or more of their instruction sequence:
  * 0x49390 = 0x49770 (identical),
  * 0x2F090 / 0x2F4F0 / 0x2F940,
  * 0x46070 / 0x473F0,
  * 0x43B40 / 0x443E0,
  * 0x265C0 / 0x3AFD0 / 0x3B740,
  * and four more pairs.

  The second copies take 15,924 bytes. xz compresses them alone to 8,724
  and to 2,526 when it has seen the first copy. The remainder is register
  and offset differences.
* **Global-state scalar code.** The context models keep counters and tables
  in global arrays in `.bss`, which is 143 MiB. For example, 0x33590 updates
  a block of counters at 0x4151E60-0x4151E90 one `mov` at a time. Each
  access carries a RIP displacement. With `r` it becomes a move in the
  sorted target table, mostly one byte.

### 3.4 Streams after x64pp

| stream | raw | xz alone | share |
|---|---:|---:|---:|
| op (opcodes, ModRM, SIB, dictionary codes, label markers) | 199,692 | 84,105 | 64.6% |
| data (everything outside code) | 89,298 | 14,746 | 11.3% |
| inl (imm8, disp8, imm16; option `m`) | 19,315 | 10,246 | 7.9% |
| disp32 (delta coded, option `d`) | 41,188 | 5,029 | 3.9% |
| imm32 | 17,136 | 4,072 | 3.1% |
| call | 13,764 | 2,539 | 1.9% |
| rip (table moves, option `r`) | 4,040 | 2,276 | 1.7% |
| jcc | 2,746 | 1,961 | 1.5% |
| esc | 6,548 | 1,938 | 1.5% |
| j8 | 3,642 | 1,477 | 1.1% |
| jmp | 1,446 | 1,127 | 0.9% |
| imm64 | 1,984 | 597 | 0.5% |
| header (regions, dictionary, RIP target table) | 2,592 | 1,347 | 1.0% |

The opcode stream averages 2.57 bytes and 8.65 compressed bits per
instruction. The dictionary gives one-byte codes to 166 skeletons, which
cover 25.8% of the instructions.

## 4. Data types

### 4.1 `.rodata` (25,648 bytes)

By address range, from 0x5F000 up:

| content | bytes | xz alone | in context |
|---|---:|---:|---:|
| lookup tables, jump tables, RTTI names (0x5F000-0x5FF70) | 3,952 | 1,472 | 1,406 |
| weights loader and arena error messages | 1,054 | 539 | 363 |
| transformer config and tensor names | 899 | 432 | 312 |
| assert and `__PRETTY_FUNCTION__` strings | 2,712 | 701 | 549 |
| English word lists, stemmer rules | 1,149 | 699 | 602 |
| runtime messages | 686 | 396 | 196 |
| enwik9 pipeline: XML templates, file names, shell commands | 1,766 | 865 | 584 |
| command-line validation messages | 5,833 | 1,810 | 1,269 |
| usage text | 3,784 | 1,117 | 738 |
| float and integer constants, SIMD vectors, string fragments (0x6454B-0x65430) | 3,813 | 1,469 | 1,219 |

By item kind, from IDA's listing:

| kind | bytes | xz alone |
|---|---:|---:|
| strings | 18,298 | 5,520 |
| byte tables | 3,422 | 1,272 |
| SIMD constants | 2,112 | 601 |
| scalar constants | 848 | 604 |
| jump tables | 732 | 215 |
| alignment padding | 178 | |

**Jump tables.** There are 8 clang switch tables (about 700 bytes) of int32
entries, each holding `target - table`. Four belong to the stemmer, and one
is the 70-entry switch of 0x33590, where all but 9 entries point to the
default case.

**Byte lookup tables** of 256 or 512 entries:

* A 512-byte state transition table (used by 0x6D10; random-looking).
* Character-class tables with long runs of 0-7 (0x5F4C0, 0x5F5C0,
  0x5F6F0).
* A bucket table (0x5FBD0: 0xC0 zeros, 0x20 ones, 0x10 twos, ...).
* Monotone tables: `0, 0, 1, 2, ..., 254` (0x5FD70) and a logarithmic
  quantizer `0, 0, 1, ..., 31, 31, 32, 32, ...` (0x5FE70).

The monotone tables have tiny byte-to-byte differences, but to LZMA every
byte is a new literal.

**SIMD constants** (16/32-byte aligned):

* Index vectors: the dwords 0..127 in 16 consecutive ymmwords at 0x64C00,
  qwords 0..31 in groups of four, and 1..8.
* Broadcast masks (`0F` bytes, `0080` words, `FF` dwords, all ones).
* `vpshufb`/`vpermd` controls.
* The int-to-double conversion constants `0x4330000000000000` and
  `0x4530000000000000`.
* The doubles 16369.0 to 16376.0, followed by k·ln 2 for k = 16377,
  16378, ...

Counting the monotone byte tables, which also read as dword and qword
progressions, 13 runs of 8 or more dwords or qwords with a constant step
cover 1,308 bytes of `.rodata`.

**Scalar constants.** float32 and float64 values loaded by
`vbroadcastss`, `vmovss`, `vmulss` and so on. They are nearly
incompressible: 604 bytes from 848.

**Strings:**

* The usage text, indented with 38-space runs, and long explanatory errors
  for invalid option combinations (5.8 KB).
* printf-style messages for the weights loader and the archive pipeline.
* Assert and `__PRETTY_FUNCTION__` strings. For example, 16 variants of
  `reference llvm::SmallVectorTemplateCommon<T>::operator[](size_type) [T = T]`
  differ only in the type name, which appears twice in each.
* English word lists: stop words, numerals, gendered nouns, Porter2
  stemmer suffixes and exception words (`skis`, `skies`, `dying`, `news`,
  `atlas`, ...).
* Wiki XML templates: `      <text xml:space="preserve">#REDIRECT` in five
  spellings, language-link fragments, and `<timestamp>` formats.
* File names and shell commands of the archive pipeline.
* Transformer tensor names (dotted paths such as
  `forget_gate_projection.up`).
* RTTI names in Itanium length-prefixed form (`13Nonstationary`,
  `9ByteMixer`).

The linker merges string tails, so a pointer can land inside another
string. The stemmer's `ize` is the tail of `alize`, its `less` is the tail
of a help line ending in "argument-less", and its exception word `bias` is
the tail of the tensor name `dt_bias`. clang also copies some literals with
16-byte loads and keeps the pieces as separately aligned constants
(`--load-transform`, `--save-transform`, `ticle-boundaries`, ...). These
are exact duplicates of text stored elsewhere.

### 4.2 Pointers and relocations

The pointer data:

* `.data.rel.ro` (1,608 bytes) holds vtables of the model classes, typeinfo
  objects and pointer arrays.
* `.data` (1,036 bytes) holds `char*` arrays: the word lists and the
  stemmer's rule pairs.
* `.init_array` has 6 entries and `.fini_array` has 1.

`.rela.dyn` holds 326 entries:

* 304 `R_X86_64_RELATIVE`: 175 in `.data.rel.ro`, 122 in `.data`, 7 in the
  init and fini arrays.
* 13 `R_X86_64_64`: 12 typeinfo objects pointing at the libstdc++
  typeinfo vtables, and the personality routine pointer.
* 9 `GLOB_DAT`.

Successive offsets differ by 8 in 280 cases. The RELATIVE addends point to:

| target | count |
|---|---:|
| start of a `.rodata` string | 185 |
| inside a merged `.rodata` string | 69 |
| other `.rodata` | 3 |
| `.data.rel.ro` (typeinfo, vtables) | 19 |
| code (virtual functions, constructors) | 27 |
| `.data` | 1 |

117 of 245 consecutive string addends point to the string right after the
previous one. Every relocated slot also holds its addend. x64pp already
zeroes those slots, which is why `.data.rel.ro` and `.data` cost about 30
bytes each. The addends now carry the information: 534 of the 715 bytes
`.rela.dyn` costs.

### 4.3 Dynamic linking metadata

* `.dynsym`: 133 entries, 120 of them undefined `FUNC GLOBAL` with zero
  value and size. `st_name` offsets into `.dynstr` are not in string order.
* `.dynstr`: 3,470 bytes, made of:
  * 4 library names,
  * 59 mangled C++ names (2,553 bytes, up to 107 characters, e.g.
    `_ZSt7getlineIcSt11char_traitsIcESaIcEERSt13basic_istream...`),
  * about 20 version names (`GLIBC_2.2.5`, `GLIBCXX_3.4.29`,
    `CXXABI_1.3.11`, `GCC_3.0`, ...),
  * the C symbol names.
* `.gnu.version`: one 16-bit version index per symbol.
* `.gnu.version_r`: per library and version, a `.dynstr` offset and the
  ELF hash of the name.
* `.rela.plt`: 118 `JUMP_SLOT` entries. Offsets step by 8, and the symbol
  index steps by 1 in 106 of 117 cases.
* `.got.plt`: 3 reserved slots and 118 lazy-binding slots. Every slot is
  exactly `.plt + 16·(i+1) + 6`.
* `.dynamic`: 29 entries, almost all addresses and sizes of other sections.
* The ELF header, 13 program headers, 31 section headers and `.shstrtab`
  are largely derivable from each other.

### 4.4 Unwind and exception tables

* `.eh_frame_hdr`: x64pp predicts it exactly, leaving 43 bytes.
* `.eh_frame`: 2 CIEs (`zR`, `zPLR`) and 472 FDEs. After x64pp, 423 CFA
  programs are all zero. The remaining 2,183 bytes break down as:

  | field | cost |
  |---|---:|
  | CFA programs (49 not predicted) | 558 |
  | `pc_begin` residuals | 467 |
  | `pc_range` residuals | 357 |
  | FDE `length` | 322 |
  | CIE pointers | 50 |
  | CIEs | 44 |
  | augmentation | 28 |

  `pc_begin` is nonzero in 106 FDEs because the FDEs are not in address
  order. GNU ld moves `.text.startup`, `.text.hot` and `.text.unlikely`
  input sections, while `.eh_frame` keeps object-file order. 414 of the 472
  function starts are direct call targets.
* `.gcc_except_table`: 11 LSDAs, 1,336 bytes, 717 after call-site ranking.

### 4.5 `.bss`

The 143 MiB of `.bss` hold the model tables and hash tables. It has no
bytes in the file, but 54% of the RIP-relative references point into it,
spread over a 150 MB range.

### 4.6 x64pp header

The header is 2,592 bytes (1,347 xz alone):

* Region list and opcode dictionary: 928 bytes (621).
* RIP target table: 1,664 bytes, the 1,440 targets as varint deltas.

## 5. What can be improved

### 5.1 Data transforms

These would be new table transforms in x64pp, with the same machinery as
the existing `.eh_frame` and `.gnu.hash` ones. The estimates are per item
for cmix; "measured" means the saving was measured on the plain file.

| item | cost now | transform | estimated saving |
|---|---:|---|---:|
| ELF metadata: `.got.plt`, `.rela.plt`, `.dynsym` fixed fields, `.gnu.version`, `.gnu.version_r` hashes, `.dynamic`, ELF/program/section headers, `.shstrtab` | ~1.9 KB (sum alone, without `.dynstr`) | predict each field from the other tables (all exact or nearly exact in this file); `st_name` and `vna_name` as string indices | 0.8-1.2 KB |
| `.eh_frame` FDE fields | 1,146 | `length` from the predicted CFA program; `pc_begin` as an index into the sorted function starts (call targets and previous ends), which handles out-of-order FDEs; `pc_range` to the next start in address order | 0.5-0.8 KB |
| `.dynstr` | 1,293 | built-in dictionary of common libc/libm/libstdc++ symbol and version names | 0.5-0.8 KB |
| `.rela.dyn` addends | 534 | addend as (string index, offset from the string's end) or function label index, delta from the previous one; raw otherwise | ~0.3 KB |
| arithmetic sequences and monotone tables in `.rodata` | | runs of ≥8 dwords/qwords with a constant step, and monotone byte tables, stored as residuals against the step | 0.36 KB (measured) |
| x64pp header | 1,347 | dictionary against a built-in default list of common skeletons; `.rodata` RIP targets predicted from item starts (strings after a NUL, aligned constants) | 0.3-0.5 KB |
| English strings | ~4.8 KB in context | word substitution with a built-in English dictionary | 0.3-0.6 KB, uncertain |
| jump tables | 215 | entries as label numbers | <0.1 KB (code pointers in data gave at most -0.1% on the corpus) |

Together these come to roughly 2.5-4 KB, or 2-3% of the output. The
metadata, `.eh_frame` and relocation transforms would help every
dynamically linked ELF file. The `.dynstr` and English dictionaries make
x64pp larger and only help files that use those libraries or that language.

### 5.2 Code

The code streams are about 115 KB, and most of it is the opcode stream. In
the scalar code, what is left at 11.5 bits per instruction is mostly
register allocation and instruction choice in code that occurs once. With
LZMA as the back end, the direct attacks on that failed (see "Things that
did not help" in the README):

* register fields in their own stream: +6.3%,
* move-to-front register ranks: +10%,
* de-interleaving by dependencies: +0.2%,
* dictionary codes for skeleton pairs: +0.6% to +0.8% on cmix.

These ideas still look plausible:

1. **Per-file variants of codings rejected on the corpus average.** Two
   earlier experiments helped cmix and hurt other files: disp32 predicted
   by stride in periodic code (-0.3% on cmix) and the disp32 delta applied
   only to VEX instructions (-0.5% on cmix). `-a` can now choose them per
   file. Estimate: 0.4-0.8 KB.
2. **RIP references with a small per-function cache.** Scalar functions
   touch tight clusters of globals (the counters of 0x33590, for example).
   An index into a short list of recent targets, falling back to the table
   move, could cut the 2.3 KB rip stream by 0.3-0.6 KB. This needs a test,
   because move-to-front coding lost badly for call targets.
3. **Near-duplicate functions as copies with a register substitution.** The
   second copies cost 2.5 KB now. A "copy function k, rename registers
   like this" token with escapes for the differences might save about 1 KB.
   This means a lot of work: function matching, register maps, a fallback.
4. **Folding unrolled iterations** into one iteration plus a count and a
   displacement stride. `d` and `m` already make these cheap LZMA matches
   (8.8 bits per instruction in the kernels), so the gain is probably
   0.3 KB or less.

A back end that models instructions directly (context mixing with x86
contexts, as in paq8px's exe model) would gain much more on the opcode
stream, but that is outside xz.

### 5.3 Build-side changes

If the binary itself can be changed (in the Hutter Prize the decompressor's
size counts), these parts have known costs in plain xz:

| part | in context |
|---|---:|
| command-line validation messages | 1,269 |
| usage text | 738 |
| runtime and pipeline messages | ~780 |
| assert and `__PRETTY_FUNCTION__` strings (`-DNDEBUG` also removes the checks) | 549 |
| `.eh_frame` + `.eh_frame_hdr` + `.gcc_except_table` (8.5 KB in plain xz; feasible only if no exceptions are relied on) | 2.9 KB after x64pp |
| `.dynstr` names of iostream and string functions (fewer libstdc++ facilities) | part of 1.3 KB |

Options that the no-argument enwik9 decompression never uses (`--save-*`,
`--load-*`, `-s`, `-h`) also bring code: the validation logic, the
article-boundary writer at 0x1A540 and their library calls. `-Os` for the
cold code (command line, I/O, preprocessing) would shrink it, while the
AVX kernels stay at `-O3`. Static linking would add libstdc++ code and make
things worse.

### 5.4 Measurement caveat

xz reacts chaotically to small layout changes. Inserting a few bytes into
the x64pp header moves the cmix output by up to 0.4%, which is about 0.5 KB.
Estimated or measured gains below about 0.5 KB should be confirmed on
several layouts or files before they count.

## Appendix: how the numbers were measured

* **Section costs**: each section was zeroed in a copy of the file and the
  copy compressed. The "xz alone" figure compresses the section bytes
  separately.
* **Costs after x64pp**: `x64pp s -odrum` writes each stream to a file. The
  data stream was cut into slices at the section boundaries (code regions
  removed), and each slice was compressed alone.
* **Function costs**: the function bytes were replaced by `0xCC`, and the
  file run through `x64pp c -odrum` and xz.
* **Instruction mix, RIP targets and near-duplicates**: from `objdump -d`.
  Near-duplicates were pairs of functions of 150 or more instructions with
  a mnemonic-sequence similarity of at least 0.85. Their conditional cost
  is `xz(A+B) - xz(A)`.
* **Data item kinds and roles**: from IDA's listing (segments, `db`/`dd`/
  `xmmword` items, jump table annotations, string cross-references).
* **Arithmetic sequences**: runs of ≥8 dwords or qwords with a constant
  step were replaced by their residuals, and the file compressed again
  (170,621 → 170,261).
