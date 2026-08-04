# Unpacking `phda9_no_LSTM` (modified UPX, ELF64/amd64)

`phda9_no_LSTM` is packed with UPX 3.94 for `linux/elf64-amd64`, but with a
patched LZMA codec, so stock `upx -d` and liblzma both refuse it.
`unupx.c` unpacks it back to a byte-exact copy of the original executable.

```
gcc -O2 -o unupx unupx.c
./unupx phda9_no_LSTM phda9_no_LSTM.unpacked
```

## What the container looks like

The packed file is a 555 892-byte, statically-linked-looking ELF with no
section headers and two program headers. Everything after the phdrs is
standard UPX bookkeeping:

| offset | struct | contents |
| --- | --- | --- |
| 176 | `l_info` | magic `UPX!`, `l_lsize` 4300, version 13, format 22 (linux/elf64-amd64) |
| 188 | `p_info` | original file size 2 035 352 |
| 200 | `b_info` chain | the compressed blocks |
| 550 944 | loader | entry point `0x486820` |
| 555 232 | `b_info` | blocks for the parts of the file no `PT_LOAD` covers |
| 555 856 | `PackHeader` | `UPX!`, method 14 (LZMA), level 10 |

The five blocks reconstruct the original file exactly:

| file range | size | source |
| --- | --- | --- |
| `0x000000`–`0x000238` | 568 | original `Ehdr` + 9 `Phdr`s |
| `0x000238`–`0x017284` | 94 284 | `PT_LOAD` 1 (`R E`, `0x400000`), filter `0x49`/cto `0x07` |
| `0x017284`–`0x017de8` | 2 916 | inter-segment alignment padding |
| `0x017de8`–`0x1f0620` | 1 935 416 | `PT_LOAD` 2 (`RW`, `0x617de8`) |
| `0x1f0620`–`0x1f0e98` | 2 168 | `.comment`, `.shstrtab` and the 29-entry section header table |

## The modification

`b_method` is 14 (`M_LZMA`), but the stream is not stock LZMA. Reading the
decompressor in the loader (at `0x486825`) shows three differences:

* **Property encoding.** UPX writes two property bytes rather than the usual
  single `(pb*5 + lp)*9 + lc` byte:

  ```
  b0 = ((lc + lp) << 3) | pb
  b1 = (lp << 4) | lc
  ```

  Here block 0 and the small trailing blocks use `18 03` → `lc=3 lp=0 pb=0`,
  and the two big blocks use `40 08` → `lc=8 lp=0 pb=0`.

* **`lc` up to 8.** liblzma only accepts `lc <= 4` with `lc + lp <= 4`, so it
  cannot decode the `lc=8` blocks even if handed the right parameters.

* **Range coder init.** `RC_INIT` loads `range = 0xFFFF0404` instead of the
  standard `0xFFFFFFFF` (`mov $0xffff0404,%r11d` at `0x486960`), which
  desynchronises any stock decoder from the very first bit.

The five bytes read into `code` and the `kNumBitModelTotalBits = 11`
arithmetic are otherwise unchanged.

## How `unupx.c` works

Instead of reimplementing that codec, it calls the one inside the file.
The UPX entry point is

```
0x486820:  e8 5f 0b 00 00    call 0x487384    ; return address == f_expand
0x486825:  <f_expand>
```

so `f_expand()` sits at `e_entry + 5` and takes the ordinary SysV arguments
(confirmed by the call site at `0x487380`):

```c
int f_expand(const u8 *src, size_t src_len, u8 *dst, unsigned *dst_len, int method);
```

`unupx` maps the whole packed image `RWX` at a scratch address and calls it
per block. The code is position independent (the loader itself relocates to
`0x32a00000` at run time), so a scratch base works fine.

The CALL/JMP unfilter (UPX filter `0x49`) is reimplemented in C, transcribed
from the loader's `f_unfilter()`. It rewrites marked `E8`/`E9`/`0F 8x`
operands — stored as `cto8` followed by a 24-bit big-endian target — back
into little-endian `rel32`. It only uses the operand's offset inside the
buffer, so it too is position independent.

## Verification

* The statically unpacked `PT_LOAD` images are byte-identical to a `gdb`
  dump of the real loader's output, taken at the `munmap` that precedes the
  jump into `ld.so`. The only difference is 4 bytes at file offset 12: the
  loader scribbles its `syscall; ret; nop` trampoline into the `e_ident`
  padding, which the static unpack correctly leaves as zero.
* The result is a well-formed dynamically linked executable, 2 035 352 bytes,
  with all 29 section headers and the original build ID
  `f9c0f180009254f9afce550a0f3e1168f6c7b6ac`.
* Packed and unpacked binaries produce byte-identical compressed output on
  the same input, and the unpacked binary round-trips it back.

```
$ ./phda9_no_LSTM.unpacked
phda9  is an experimental compressor for English texts
Version 1.8    (c) Alexander Rhatushnyak   2019-Jun-29
```

`phda9_no_LSTM.unpacked.xz` is the unpacked executable
(sha256 of the decompressed file:
`4c0928394d6d4881cf4709ef7082cb24e9b8682cfbaa019efdc8eca056b0bc8a`).
