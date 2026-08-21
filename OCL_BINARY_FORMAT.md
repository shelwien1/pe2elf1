# Intel OpenCL CPU device-binary format

Notes gathered while reverse-engineering the blob returned by
`clGetProgramInfo(prog, CL_PROGRAM_BINARIES, ...)` on the Intel OpenCL CPU
runtime (device `Intel(R) Core(TM) i7-7820X`, AVX-512 target, built with the
Intel oneAPI DPC++/C++ Compiler 2024.2.0).

`ocl2elf.py` implements everything described here.

## Overall layout

```
+--------------------------------------------------+ 0x00
|  48-byte proprietary preamble  ("mrpcCL")         |
+--------------------------------------------------+ 0x30
|  ELF64 container image                            |
|    +-- .ocl.ir    LLVM bitcode (SPIR64)           |
|    +-- .ocl.meta  kernel metadata                 |
|    +-- .ocl.obj   ELF64 x86-64 relocatable object |  <-- native AVX-512 code
|    +-- .ocl.ver   uint32 format version           |
|    +-- (shstrtab)                                 |
+--------------------------------------------------+ EOF
```

## The 48-byte preamble

| offset | size | field |
|--------|------|-------|
| `0x00` | 6  | magic `"mrpcCL"` |
| `0x06` | 2  | `uint16` version (observed: 1) |
| `0x08` | 32 | opaque build/cache key |
| `0x28` | 8  | `uint64` payload size == `filesize - 0x30` |

The 32-byte key is *not* a plain digest of the payload — SHA-256, SHA3-256,
BLAKE2s, MD5 and SHA-1 over the payload bytes were all checked and none match.
It is most likely a cache key over the source text plus build options. The
size field at `+0x28` does verify exactly, and `ocl2elf.py` uses it to confirm
the ELF base offset before falling back to a magic scan.

## Why the container ELF looks "non-standard"

The image at `0x30` is a real ELF64 section table, but it deviates from the
spec in four ways. Two of them are load-bearing — they are what makes binutils
reject the file:

| field | value | spec | effect |
|-------|-------|------|--------|
| `e_ident[EI_VERSION]` | `0` | `1` | **`objcopy: file format not recognized`** |
| `e_version` | `0` | `1` | same |
| `sh_type` (content sections) | `SHT_NULL` | `SHT_PROGBITS` | **`objcopy: the input file has no sections`** |
| `sh_addralign` | `0` | `>= 1` | tolerated |
| `e_type` | `0xff04` | — | tolerated (legal processor-specific value) |

`readelf` is lenient and will print the section table of the raw carve;
`objcopy`, `llvm-objcopy` and anything else built on BFD/libObject will not
touch it until `EI_VERSION`/`e_version` and `sh_type` are corrected.

**Section offsets are relative to the start of the embedded ELF**, i.e. to file
offset `0x30`. So carving `data[0x30:]` yields an internally self-consistent
image; no offset rewriting is needed, only the field fixes above.

## Sections

| name | contents | what to do with it |
|------|----------|--------------------|
| `.ocl.ir` | LLVM bitcode, `target triple = spir64-unknown-unknown` | `llvm-dis` — this is front-end output, *before* the CPU back-end vectorised anything |
| `.ocl.meta` | kernel metadata: kernel names, argument tables | — |
| `.ocl.obj` | **standard** ELF64 x86-64 `ET_REL` object | `objdump -d -M intel` — the native AVX-512 code |
| `.ocl.ver` | `uint32` (observed: `0x14`) | — |

`.ocl.obj` needs no repair at all: it is a perfectly ordinary relocatable
object emitted by the Intel back-end, complete with `.symtab`, `.rela.ltext`
and a `.comment` naming the compiler.

### Notes on `.ocl.obj`

* Native code lives in **`.ltext`**, not `.text` (`.text` is zero-length).
  The back-end uses the large code model; `.ltext` carries the
  `SHF_X86_64_LARGE` flag. `objdump -d` and `llvm-objdump -d` both handle it.
* The **Windows x64 ABI** is used — kernel arguments arrive in `rcx`/`rdx`,
  and `__chkstk` is an undefined symbol. Keep that in mind when reading
  argument loads; do not assume System V register order.
* Each OpenCL kernel appears as a `GLOBAL FUNC` symbol. The only other
  undefined symbol is `memset`.

## Usage

```sh
./ocl2elf.py program.cl --list           # dump container layout only
./ocl2elf.py program.cl -o out           # extract all sections
./ocl2elf.py program.cl -o out --disasm  # + Intel-syntax disassembly

objdump -d -M intel out/program.ocl_obj.elf | less
llvm-dis out/program.ocl_ir.bc -o out/program.ll
```

The extractor also writes `*.container.elf`: the outer image with the four
deviations above normalised, so you can drive binutils at the container
directly:

```sh
objcopy --dump-section .ocl.obj=native.elf program.container.elf /dev/null
```
