# x64pp

A lossless preprocessor that makes x86-64 executables compress better with
xz/LZMA. It replaces the usual BCJ/E8E9 filter: run `x64pp c`, compress the
result with `xz`, and after decompression run `x64pp d`.

```
x64pp c program.exe program.x64pp && xz -9e program.x64pp
xz -d program.x64pp.xz && x64pp d program.x64pp program.exe
```

On the test sets below the xz output is 24-25% smaller than plain `xz -9e`,
19-22% smaller than `xz --x86`, and 15-17% smaller than with
[x64flt3](https://nishi.dreamhosters.com/u/x64flt3_v1.7z).

## Results

Sizes in bytes after `xz -T1 --check=none --lzma2=preset=9e`. `x64flt3` is
x64flt3 with its four output files concatenated before xz, which is slightly
better than compressing them separately.

Tuning set (all transforms were developed on these files):

| file | size | xz | xz --x86 | x64flt3 | x64pp |
|---|---:|---:|---:|---:|---:|
| elf_bash | 1,446,024 | 583,788 | 545,568 | 503,832 | 410,708 |
| elf_gdb | 8,920,528 | 3,002,788 | 2,723,940 | 2,629,292 | 2,229,804 |
| elf_libpython.so | 9,061,000 | 2,108,448 | 2,001,140 | 1,897,780 | 1,584,944 |
| elf_python | 8,020,928 | 2,118,392 | 1,993,560 | 1,900,992 | 1,674,596 |
| elf_rg (Rust) | 5,257,872 | 1,371,532 | 1,354,044 | 1,274,616 | 996,700 |
| elf_vim | 4,130,640 | 1,751,720 | 1,650,220 | 1,513,020 | 1,237,880 |
| elf_xz | 89,008 | 31,216 | 29,280 | 28,488 | 24,952 |
| pe_7za.dll | 413,184 | 164,940 | 156,212 | 155,424 | 137,968 |
| pe_7za.exe | 1,323,520 | 492,476 | 451,920 | 447,572 | 394,936 |
| pe_msvcp140.dll | 585,384 | 148,616 | 140,380 | 134,428 | 118,264 |
| pe_mtrand.pyd | 633,344 | 151,536 | 146,240 | 137,948 | 117,116 |
| pe_npcore.pyd | 4,160,512 | 969,912 | 932,712 | 895,996 | 771,380 |
| pe_rg.exe (Rust) | 5,407,744 | 1,450,748 | 1,359,536 | 1,304,228 | 1,159,784 |
| pe_t64.exe | 108,032 | 44,692 | 42,728 | 42,492 | 38,132 |
| pe_x64flt3.exe | 56,832 | 19,172 | 18,652 | 18,232 | 16,084 |
| **total** | 49,614,552 | 14,409,976 | 13,546,132 | 12,884,340 | **10,913,248** |
| vs xz | | | -5.99% | -10.59% | **-24.27%** |

Held-out set (not looked at during development):

| file | size | xz | xz --x86 | x64flt3 | x64pp |
|---|---:|---:|---:|---:|---:|
| elf_cmake | 11,796,472 | 3,349,772 | 2,919,212 | 2,728,136 | 2,186,144 |
| elf_gitlfs_go (Go) | 11,516,960 | 3,513,356 | 3,334,632 | 3,204,572 | 3,000,240 |
| elf_libllvm17.so | 123,671,544 | 22,926,564 | 22,282,464 | 21,042,852 | 16,919,488 |
| elf_php | 5,784,016 | 1,744,236 | 1,650,460 | 1,550,880 | 1,262,288 |
| elf_shim_go (Go) | 8,799,192 | 2,613,004 | 2,482,980 | 2,376,876 | 2,231,700 |
| pe_7zxa.dll | 216,064 | 88,576 | 85,472 | 84,892 | 75,560 |
| pe_npgen.pyd | 748,032 | 197,924 | 191,432 | 180,156 | 152,288 |
| pe_npsimd.pyd (AVX) | 2,236,928 | 262,060 | 241,492 | 176,024 | 100,896 |
| pe_openblas_mingw.dll | 20,269,568 | 3,575,148 | 3,501,848 | 3,142,108 | 2,706,760 |
| pe_w64.exe | 101,888 | 43,532 | 41,744 | 41,616 | 37,384 |
| **total** | 185,140,664 | 38,314,172 | 36,731,736 | 34,528,112 | **28,672,748** |
| vs xz | | | -4.13% | -9.88% | **-25.16%** |

With xz's default preset (`-6`) the tuning set gives 14,397,548 / 13,547,052
/ 12,884,832 / 10,923,232 (-24.13%; measured before the last 1 KB of tuning).

Sources: ELF files from Ubuntu 24.04 packages, PE files from the numpy 2.1.3
win_amd64 wheel, pip 24.2 (launchers), ripgrep 14.1.1 and 7-Zip 24.08
(x64 "extra" package), plus x64flt3.exe itself.

Speed (one core of a cloud VM): 25-30 MB/s forward without verification,
50-70 MB/s inverse (libLLVM, 124 MB: 4.9 s and 2.4 s); memory about 4.5x
the input size. The forward transform
verifies itself by default, which costs one inverse pass. `tools/bench.py`
reproduces the tables:

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

Any input is accepted. Files without recognized x86-64 code pass through
unchanged apart from a 32-byte header. The encoder decodes its own output
and compares it with the input before writing anything (skip with `-n`).

Build: `make`, or `g++ -O2 -o x64pp src/x64pp.cpp` (C++11, no
dependencies). Windows: `make x64pp.exe` with mingw-w64, or any C++11
compiler on the single file. Tested with gcc 13, clang 18 and mingw-w64 13
(the Windows binary was run under Wine and produces byte-identical output);
MSVC untested.

## How it works

Sources: `src/x64dec.h` (instruction decoder), `src/analyze.h` (finding code
and tables), `src/tables.h` (table transforms), `src/x64pp.cpp` (code
transform, container, command line).

### 1. Finding code and tables

The input is scanned for PE32+ (machine AMD64) and ELF64 x86-64 images, at
offset 0 and also embedded anywhere in the file (tar archives, installers).
Regions are recorded for:

* code: executable sections (PE `CNT_CODE`/`MEM_EXECUTE`, ELF
  `SHF_EXECINSTR`, or executable `PT_LOAD` segments when there are no
  section headers), with their virtual addresses;
* tables: PE exception directory and base relocations, ELF `.eh_frame`,
  `.eh_frame_hdr`, `.gnu.hash` and `Elf64_Rela` sections.

The region list goes into the output header, so the decoder never parses
executable headers: malformed or truncated executables round-trip like any
other data. Each embedded image gets its own 4 GiB virtual address window.

### 2. Instruction stream split

`x64dec.h` is a table-driven x86-64 length decoder: legacy prefixes, REX,
VEX, EVEX, REX2 (APX), 3DNow!, all opcode maps. Its instruction boundaries
match objdump on 3.2 million instructions of ELF code; the only differences
are invalid byte sequences inside data. It splits an instruction into
structural bytes (prefixes, opcode, ModRM, SIB) and operand fields, and the
layout depends on the structural bytes only. The inverse can therefore parse
the opcode stream alone and know where every operand goes.

| stream | contents |
|---|---|
| op | structural bytes, label markers, plus imm8, disp8, imm16 in place |
| j8 | rel8 branch targets |
| jmp, jcc | rel32 `jmp` / `jcc` targets |
| call | rel32 `call` targets |
| rip | RIP-relative disp32 as absolute address, big-endian |
| disp32 | other disp32, big-endian |
| imm32 | imm32, big-endian |
| imm64 | imm64, moffs64 |
| esc | escaped branch targets |
| data | everything outside code regions |

The streams are concatenated, each aligned to 16 bytes so that LZMA's pb=2
position context lines up with 4-byte records. For RIP-relative targets the
"next instruction" address includes any immediate, so the same target always
gives the same absolute value. Keeping imm8/disp8/imm16 next to their
opcode and moving the 32-bit fields out was the best split measured.

### 3. Branch targets as labels

An instruction start that a `jmp`/`jcc`/`call`/rel8 branch targets is a
label. Labels are marked in the op stream with 0xD6, an invalid opcode in
64-bit mode; a genuine 0xD6 instruction is written as `D6 D6` and never gets
an explicit label. The first non-padding instruction after `ret`, `jmp`,
`ud2`, `hlt` or `int3` is an implicit label and needs no marker. The decoder
collects all labels in a first pass over the op stream, then:

* `jmp`/`jcc` rel32: z = zigzag(label distance from the next instruction)+1
  in one byte if z < 255; otherwise 255, and the absolute label number in
  the esc stream (far jumps are mostly tail calls and shared exits);
* `call` rel32: label number +1, big-endian 32-bit;
* rel8: the 256 possible targets are ranked, labels first (nearest first),
  then the remaining addresses; the rank is stored;
* 0 marks a target that is not a label; its address is in the esc stream.

The markers cost about 1.3% of the output and the label coding saves more
than twice that.

### 4. Tables

All table transforms are invertible from the region list and the decoded
image (code is decoded before tables are restored):

* `.eh_frame`: CIE pointer becomes the CIE offset. `pc_begin` becomes the
  gap to the end of the previous FDE minus the padding found there in the
  code. `pc_range` loses the predicted function length: up to the first
  `ret`/`jmp`/`call`/`ud2` followed by padding (and, for functions starting
  with `endbr64`, by the next `endbr64`). The LSDA pointer is delta coded.
  In the CFA program every `advance_loc` target is replaced by its rank among
  the ends of stack-changing instructions of the function (push/pop, rsp
  arithmetic, frame setup, `leave`, `ret`), so most become 0..3.
* `.eh_frame_hdr`: the search table is the sorted FDE list of `.eh_frame`;
  it is predicted from it and only differences are stored (exact in every
  file tested).
* `.gnu.hash`: bloom filter, buckets and chains are recomputed from the
  `.dynsym` names and XORed with the table (exact in every file tested).
* `Elf64_Rela`: offsets delta coded, addends big-endian. For
  `R_X86_64_RELATIVE`/`IRELATIVE` the linker also writes the addend into the
  relocated slot; the slot becomes `slot - addend` (zero) when it lies in
  plain data.
* PE exception directory: function start and length predicted like the
  FDE fields above, unwind RVA delta coded.
* PE `UNWIND_INFO`: the unwind codes are generated from the prolog the way
  MSVC lays them out (pushes, allocations including `__chkstk`, nonvolatile
  saves relative to the final rsp, frame register) and XORed with the actual
  ones; 79-98% of them become all zero.
* PE base relocations: page RVA delta, entries delta coded within a block.

### Format

```
"x64p" version(1) flags(1)
varint original size
varint region count, per region:
    type(1) varint gap-to-previous-region varint size
    [varint VA delta]     code, .eh_frame, .eh_frame_hdr
    varint n, n x varint  parameters: .eh_frame_hdr -> its .eh_frame,
                          .eh_frame/.pdata -> image VA bias (+ section map),
                          Elf64_Rela -> PT_LOAD map, .gnu.hash -> .dynsym/.dynstr
varint stream sizes (11)
streams, each starting at a multiple of 16
```

## Things that did not help

Measured on the tuning set with xz -9e, relative to the version at the
time. Several of these were suggested ideas worth checking; with LZMA as
the back end the answer was mostly "no":

| idea | result |
|---|---|
| register numbers (ModRM reg/rm, SIB, push/pop/mov-imm) in their own stream | +6.3% |
| same with move-to-front register ranks (one list / per field kind) | +10.1% / +11.3% |
| register fields moved right after the skeleton, same stream | +1.4% |
| partial variants (only push/pop/mov-imm, only reg, only rm, only base/index) | +1.0% to +3.1% |
| reversing pop runs before `ret` into push order, to match prologues | +0.02% |
| canonical reordering of instructions inside basic blocks by register dependencies (de-interleaving), even with the permutation for free | +0.2% |
| two label marker kinds (called / jumped-to) so calls index a smaller set | -0.04%, not worth the code |
| jmp/jcc label distances inline in the op stream (to match whole fragments) | +0.4% to +1.1% |
| near calls as 1-byte relative codes, far calls absolute | +0.3% to +0.4% |
| byte-transposed jmp/call/rip/disp32/imm32 streams | +0.02% to +1.5% |
| relative/absolute hybrid for jumps (relative below a threshold) | ±0.01% |
| calls through a sorted target dictionary / MTF cache / frequency rank | -3% / +6% / +9% to +28% of the call stream |
| code pointers in data (jump tables, vtables) as instruction indices | at most -0.1% |
| PE relocated pointers (from `.reloc`) as RVA or delta | -0.14% at best |
| RELA addend delta, offset big-endian, info delta | -0.04% to +0.4% |
| 3-byte instead of 4-byte index fields | +0.3% |
| disp32, imm32 or rel8 kept in the op stream | +0.3% to +1.3% |
| imm8/disp8/imm16 in separate streams | +0.5% (so they stay inline) |

LZMA does best with register allocation left inside ModRM and with the
compiler's instruction order: identical source compiles to identical bytes,
and those exact repeats matter more than renamed or reordered copies. What
pays off is removing information the decoder can already derive (branch
targets from labels, unwind and hash tables from code and symbols) and
turning absolute values into small, repeating numbers.

## Testing

* Round trip on all files above, truncated and corrupted executables,
  random data, an empty file, and a tar of several executables.
* Fuzzing with ASan/UBSan (2,400 iterations): mutated executables through
  the forward transform, which must round-trip, and corrupted streams
  through the inverse, which must fail cleanly.
* Decoder instruction lengths checked against objdump on all code sections
  of the test files.

## License

MIT, see LICENSE.
