# x64pp

A lossless preprocessor that makes x86-64 executables compress better with
xz/LZMA. It replaces the usual BCJ/E8E9 filter: run `x64pp c`, compress the
result with `xz`, and after decompression run `x64pp d`.

```
x64pp c program.exe program.x64pp && xz -9e program.x64pp
xz -d program.x64pp.xz && x64pp d program.x64pp program.exe
```

On the test sets below the xz output is 22-23% smaller than plain `xz -9e`,
17-20% smaller than `xz --x86`, and 13-15% smaller than with
[x64flt3](https://nishi.dreamhosters.com/u/x64flt3_v1.7z).

## Results

Sizes in bytes after `xz -T1 --check=none --lzma2=preset=9e`. `flt3` is
x64flt3 with its four output files concatenated before xz, which is slightly
better than compressing them separately (12,889,396 total on the first set).

Tuning set (all transforms were developed on these files):

| file | size | xz | xz --x86 | x64flt3 | x64pp |
|---|---:|---:|---:|---:|---:|
| elf_bash | 1,446,024 | 583,788 | 545,568 | 503,832 | 436,580 |
| elf_gdb | 8,920,528 | 3,002,788 | 2,723,940 | 2,629,292 | 2,278,940 |
| elf_libpython.so | 9,061,000 | 2,108,448 | 2,001,140 | 1,897,780 | 1,627,816 |
| elf_python | 8,020,928 | 2,118,392 | 1,993,560 | 1,900,992 | 1,714,644 |
| elf_rg (Rust) | 5,257,872 | 1,371,532 | 1,354,044 | 1,274,616 | 1,016,708 |
| elf_vim | 4,130,640 | 1,751,720 | 1,650,220 | 1,513,020 | 1,267,696 |
| elf_xz | 89,008 | 31,216 | 29,280 | 28,488 | 25,192 |
| pe_7za.dll | 413,184 | 164,940 | 156,212 | 155,424 | 140,256 |
| pe_7za.exe | 1,323,520 | 492,476 | 451,920 | 447,572 | 400,604 |
| pe_msvcp140.dll | 585,384 | 148,616 | 140,380 | 134,428 | 119,820 |
| pe_mtrand.pyd | 633,344 | 151,536 | 146,240 | 137,948 | 119,044 |
| pe_npcore.pyd | 4,160,512 | 969,912 | 932,712 | 895,996 | 782,928 |
| pe_rg.exe (Rust) | 5,407,744 | 1,450,748 | 1,359,536 | 1,304,228 | 1,175,536 |
| pe_t64.exe | 108,032 | 44,692 | 42,728 | 42,492 | 38,904 |
| pe_x64flt3.exe | 56,832 | 19,172 | 18,652 | 18,232 | 16,304 |
| **total** | 49,614,552 | 14,409,976 | 13,546,132 | 12,884,340 | **11,160,972** |
| vs xz | | | -5.99% | -10.59% | **-22.55%** |

Held-out set (never looked at during development):

| file | size | xz | xz --x86 | x64flt3 | x64pp |
|---|---:|---:|---:|---:|---:|
| elf_cmake | 11,796,472 | 3,349,772 | 2,919,212 | 2,728,136 | 2,233,904 |
| elf_gitlfs_go (Go) | 11,516,960 | 3,513,356 | 3,334,632 | 3,204,572 | 3,002,444 |
| elf_libllvm17.so | 123,671,544 | 22,926,564 | 22,282,464 | 21,042,852 | 17,490,232 |
| elf_php | 5,784,016 | 1,744,236 | 1,650,460 | 1,550,880 | 1,313,636 |
| elf_shim_go (Go) | 8,799,192 | 2,613,004 | 2,482,980 | 2,376,876 | 2,231,864 |
| pe_7zxa.dll | 216,064 | 88,576 | 85,472 | 84,892 | 76,944 |
| pe_npgen.pyd | 748,032 | 197,924 | 191,432 | 180,156 | 154,256 |
| pe_npsimd.pyd (AVX) | 2,236,928 | 262,060 | 241,492 | 176,024 | 106,336 |
| pe_openblas_mingw.dll | 20,269,568 | 3,575,148 | 3,501,848 | 3,142,108 | 2,709,056 |
| pe_w64.exe | 101,888 | 43,532 | 41,744 | 41,616 | 38,104 |
| **total** | 185,140,664 | 38,314,172 | 36,731,736 | 34,528,112 | **29,356,776** |
| vs xz | | | -4.13% | -9.88% | **-23.38%** |

Sources: ELF files from Ubuntu 24.04 packages, PE files from the numpy 2.1.3
win_amd64 wheel, pip 24.2 (launchers), ripgrep 14.1.1 and 7-Zip 24.08
(x64 "extra" package), plus x64flt3.exe itself.

Speed (single thread, one core of a cloud VM): about 30 MB/s forward
without verification, 70 MB/s inverse. Memory is about 4x the input
size. Forward verification (the default) costs one inverse pass.
`tools/bench.py` reproduces the tables:

```
python3 tools/bench.py --x64flt3 path/to/x64flt3 FILES_OR_DIRS...
```

## Usage

```
x64pp c [-v] [-n] input output   forward transform
x64pp d input output             inverse transform
x64pp s input prefix             write each stream to prefix.<name> (analysis)
  -v  print statistics
  -n  don't verify the forward transform by decoding it
  input/output may be - for stdin/stdout
```

Any input is accepted. Files without recognized x86-64 code are stored
unchanged apart from a 32-byte header. The encoder decodes its own output
and compares it with the input before writing anything (skip with `-n`).

Build: `make`, or `g++ -O2 -o x64pp src/x64pp.cpp` (C++11, no
dependencies; tested with gcc 13 and clang 18, MSVC untested).

## How it works

### 1. Finding code and tables

`src/analyze.h` scans the input for PE32+ (machine AMD64) and ELF64 x86-64
images, at offset 0 and also embedded anywhere in the file (tar archives,
installers). It records regions:

* code: executable sections (PE: `CNT_CODE`/`MEM_EXECUTE`, ELF:
  `SHF_EXECINSTR`, or executable `PT_LOAD` segments when there are no
  section headers), with their virtual addresses;
* tables: PE exception directory (`.pdata`), PE base relocations, ELF
  `.eh_frame`, `.eh_frame_hdr`, and `Elf64_Rela` sections.

The region list is stored in the output header. The decoder never parses
executable headers, so malformed or truncated executables round-trip like
anything else. Each embedded image gets its own 4 GiB virtual address
window.

### 2. Instruction stream split

`src/x64dec.h` is a table-driven x86-64 length decoder: legacy prefixes,
REX, VEX, EVEX, REX2 (APX), 3DNow!, all opcode maps. Its instruction
boundaries match objdump on 3.2 million instructions of ELF code; the only
differences are invalid byte sequences inside data. It splits an
instruction into structural bytes (prefixes, opcode, ModRM, SIB) and
operand fields, and the layout depends on the structural bytes only. The
inverse transform can therefore parse the opcode stream alone and know
where every operand goes.

Code regions are split into streams:

| stream | contents |
|---|---|
| op | structural bytes, plus imm8, disp8 and imm16 kept in place |
| j8 | rel8 branch targets (see 3) |
| jmp | rel32 `jmp`/`jcc` targets (see 3) |
| call | rel32 `call` targets (see 3) |
| rip | RIP-relative disp32 as absolute address, big-endian |
| disp32 | other disp32, big-endian |
| imm32 | imm32, big-endian |
| imm64 | imm64 and moffs64 |
| esc | branch targets that are not labels, absolute |
| data | everything outside code regions |

The streams are concatenated, each aligned to 16 bytes so that LZMA's
pb=2 position context lines up with the 4-byte records. For RIP-relative
targets the "next instruction" address includes any immediate, so the same
target always gives the same absolute value, which x64flt3's byte patterns
can't do. Keeping imm8/disp8/imm16 next to their opcode and moving everything
32-bit out was the best split measured.

### 3. Branch targets as labels

An instruction start that some `jmp`/`jcc`/`call`/rel8 branch targets is a
label. Labels are marked in the op stream with the byte 0xD6, an invalid
opcode in 64-bit mode; a genuine 0xD6 instruction is written as `D6 D6` and
never gets an explicit label. The first non-padding instruction after
`ret`, `jmp`, `ud2`, `hlt` or `int3` is an implicit label and needs no
marker. The decoder collects all labels in a first pass over the op stream,
then decodes operands:

* `jmp`/`jcc` rel32: zigzag-coded distance in labels from the next
  instruction, +1 (0 = escape to the esc stream), big-endian 32-bit;
* `call` rel32: label number +1, big-endian 32-bit;
* rel8: the 256 possible targets are ranked, labels first (nearest first),
  then the remaining addresses, and the rank is stored.

The markers cost about 1.3% of the output and the label coding saves about
2.3%. Before labels the same scheme used instruction indices, which was
already 1.5% better than absolute addresses.

### 4. Tables

All table transforms are in place, same size, and invertible without
headers (`src/tables.h`):

* `.eh_frame`: CIE pointer becomes the CIE offset; `pc_begin` becomes
  the gap to the end of the previous FDE; the LSDA pointer becomes absolute
  and is delta coded (null pointers stay null). This costs 24% less after
  xz.
* `.eh_frame_hdr`: the search table is the sorted FDE list of `.eh_frame`,
  so it is predicted from it and only differences are stored. This matched
  exactly in every file tested, including the 108,000 entries of libLLVM.
* `Elf64_Rela`: offsets delta coded, addends big-endian. For
  `R_X86_64_RELATIVE`/`IRELATIVE` the linker writes the addend into the
  relocated slot as well; the slot becomes `slot - addend` (zero) when it
  lies in plain data. The image's `PT_LOAD` map is stored with the region.
* PE `.pdata`: begin as gap to the previous end, length, unwind RVA delta.
* PE `.reloc`: page RVA delta, entries delta coded within a block.

### Format

```
"x64p" version(1) flags(1)
varint original size
varint region count, per region:
    type(1) varint gap-to-previous-region varint size
    [varint VA delta]          code, .eh_frame, .eh_frame_hdr
    [varint link+1]            .eh_frame_hdr: index of its .eh_frame region
    [varint n, n x (va,off,size)]  Elf64_Rela: PT_LOAD map
varint stream sizes (10)
streams, each starting at a multiple of 16
```

## Things that did not help

All measured on the tuning set with xz -9e, relative to the version at
the time:

| idea | result |
|---|---|
| register numbers (ModRM reg/rm, SIB, push/pop/mov-imm) in their own stream | +6.3% |
| same with move-to-front register ranks (one list / per field kind) | +10.1% / +11.3% |
| register fields moved right after the skeleton, same stream | +1.4% |
| partial variants (only push/pop/mov-imm, only reg, only rm, only base/index) | +1.0% to +3.1% |
| reversing pop runs before `ret` into push order, to match prologues | +0.02% |
| canonical reordering of instructions inside basic blocks by dependencies, permutation not even paid for | +0.2% |
| two label marker kinds (called / jumped-to) so calls index a smaller set | -0.04%, not worth the code |
| relative/absolute hybrid for jumps (relative below a threshold) | ±0.01% |
| relative jump targets instead of absolute (before labels) | +0.5% to +1.7% |
| calls as instruction index | +0.02% |
| calls through a sorted target dictionary / MTF cache / frequency rank | -3% / +6% / +9% to +28% of the call stream |
| code pointers in data (jump tables, vtables) as instruction indices | at most -0.1% |
| 3-byte instead of 4-byte index fields | +0.3% |
| disp32, imm32 or rel8 kept in the op stream | +0.3% to +1.3% |
| imm8/disp8/imm16 in separate streams | +0.5% (so they stay inline) |

LZMA does best with the register allocation left inside ModRM, and with
compiler instruction order: the same source code compiles to the same
bytes, and those exact repeats matter more than renamed or reordered
copies.

## Testing

* Round trip on all files above, on truncated and corrupted executables,
  random data, an empty file and a tar of several executables.
* Fuzzing with ASan/UBSan: mutated executables through the forward
  transform (which must round-trip), corrupted streams through the inverse
  (which must fail cleanly).
* Decoder instruction lengths checked against objdump on all code sections
  of the test files.

## License

MIT, see LICENSE.
