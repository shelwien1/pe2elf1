# mine32 — mining the 40-bit x86-32 instruction space with Zydis

`mine32` is a frontend for the [Zydis](https://github.com/zyantific/zydis)
disassembler that exhaustively sweeps the **40-bit space** — every one of the
2⁴⁰ possible 5-byte sequences — and reports every sequence that the Zydis
decoder accepts as a valid x86-32 instruction.

A naive sweep would mean 1.1 trillion (`2^40`) decode calls. `mine32` walks the
space far faster by skipping over bytes that provably don't change the answer,
so it visits each distinct instruction *form* exactly once.

## Build

Requires `gcc`/`clang`, `cmake`, `make`, `git` and `pthreads`.

```sh
./build.sh        # clones + builds Zydis, then compiles ./mine32
./mine32 --help
```

`build.sh` clones Zydis into `./zydis/` (git-ignored) and builds it as a static
library, then links `mine32.c` against it.

### Windows build (MinGW cross-compile)

A prebuilt **`mine32.exe`** (64-bit, statically linked) is committed in the repo.
It depends only on `KERNEL32.dll` and `msvcrt.dll`, so it runs on a stock
Windows machine with no extra DLLs. To rebuild it:

```sh
sudo apt-get install mingw-w64     # one-time: the cross toolchain
./build-win.sh                     # -> mine32.exe (x86_64)
ARCH=i686 ./build-win.sh           # optional 32-bit build
```

`build-win.sh` uses the POSIX-threads MinGW variant (`*-gcc-posix`) so the
pthreads/`<stdatomic.h>` code links, and passes `-static` plus
`-DZYDIS_STATIC_BUILD -DZYCORE_STATIC_BUILD` so the result is a single
self-contained executable. The same `mine32.c` source builds for both Linux and
Windows.

## Usage

```
usage: ./mine32 [options]
  -t, --threads N     worker threads (default: 4)
  -b, --bytes N       window size in bytes, 1..8 (default: 5 = 40 bit)
  -m, --mode M        16 | 32 | 64 (default: 32)
      --no-skip-imm   count every disp/imm value as a distinct form
      --canon         merge instructions that differ only in registers
                      (every register -> class base: eax, xmm0, ...)
      --dump PREFIX   write found forms to PREFIX.<tid> (PREFIX in --canon)
      --range A B     only first-bytes [A,B) (for testing; default 0 256)
  -q, --quiet         no progress output
```

Mine the full 40-bit x86-32 space on 4 threads:

```sh
./mine32 -t 4
```

Dump every form (bytes + Intel disassembly) to files, restricted to opcodes
`0x00..0x10` for a quick look:

```sh
./mine32 -t 4 --dump forms --range 0x00 0x10
cat forms.* | sort -u | head
```

A dump line looks like (the `|` marks where the structural bytes end and the
wildcard displacement/immediate bytes begin):

```
0f a4 c0|00      shld eax, eax, 0x00
```

## What "decodable" means here

By default `mine32` enumerates distinct instruction **forms**, treating the
displacement and immediate bytes as wildcards. This is almost always what you
want: without it, a single opcode such as `mov eax, imm32` (`B8 imm32`) would
contribute all 2³² immediate values as 2³² separate "instructions".

* `mov eax, imm32` → **1** form by default, **4294967296** forms with
  `--no-skip-imm`.

Pass `--no-skip-imm` to instead count every concrete byte sequence.

## Collapsing register variants (`--canon`)

Even with immediates wildcarded, most of the 34.6M forms are the *same*
instruction with different register operands (all 64 `add r/m32, r32`
register-direct encodings, every base/index register in a memory operand, etc.).
`--canon` factors these out: each operand register is rewritten to the **base
register of its class** (GPR → `eax`/`ax`/`al` by width, XMM → `xmm0`, YMM →
`ymm0`, mask → `k0`, …) and the resulting disassembly string is de-duplicated.
The dedup key *is* the normalized text, so the reported count equals the number
of distinct lines you see.

```sh
./mine32 -t 4 --canon --dump forms.txt   # forms.txt = sorted unique forms
```

What is kept distinct: mnemonic, operand types/sizes, addressing mode, SIB
scale, and any explicit prefix (`lock`, `rep`, segment override, …) — only the
register *identities* are normalized. For example opcode `0x00` (`add r/m8, r8`)
has 4008 raw forms but collapses to 6:

```
add al, al
add [eax], al
add [eax+eax*1], al
add [eax+eax*2], al
add [eax+eax*4], al
add [eax+eax*8], al
```

Note `--canon` decodes operands for every candidate (it uses
`ZydisDecoderDecodeFull` and formats each form), so it is slower than the plain
count, but it shrinks the output by orders of magnitude. The scan still visits
every register encoding — duplicates are skipped at *output* time, across all
threads.

## How the sweep is made fast

The 40-bit value is laid out big-endian: byte 0 (the first instruction byte) is
the most significant. For each candidate Zydis is asked to decode the window,
and the result tells us how far we can jump:

* **Success** (length `L`): bytes after `L` aren't part of the instruction, so
  they're wildcards. With the default `--skip-imm`, the displacement/immediate
  bytes are wildcards too. We record the form once and skip every value of the
  trailing wildcard bytes.
* **Truncated** (`NO_MORE_DATA`, instruction needs more than the window): we
  re-probe with a full 15-byte buffer to learn the structural length, then skip
  the trailing value bytes (these encodings don't fit in 40 bits, so they're
  out of scope and not counted).
* **Invalid**: we probe with increasing buffer lengths to find the shortest
  prefix that already fails. Once a prefix is rejected, appending bytes can
  never make that same prefix valid, so the rest is skippable.

Work is partitioned by the first byte (256 chunks handed out from an atomic
counter). Because every form's first byte is fixed and all of its wildcard
variations keep that byte fixed, each form is owned by exactly one thread — no
locking on the hot path and no double counting.

## Output

```
===== results =====
decodable forms : <count of distinct instruction forms>
decode calls    : <Zydis decode calls actually issued>
elapsed         : <seconds>  (<throughput> M decodes/s)

forms by length (bytes):
  1 : ...
  ...
distinct mnemonics: <n>

top mnemonics by form count:
  ...
```

## Notes / limitations

* The `-b/--bytes N` flag generalises the window to N bytes (1..8); `5` is the
  40-bit default requested. Larger N explores longer instructions but the raw
  space grows as `2^(8N)`.
* `-m/--mode` switches the decoder to 16- or 64-bit; the task targets 32-bit.
* 3DNow! instructions encode their opcode in a trailing `imm8` suffix. With
  `--skip-imm` that suffix is treated as a wildcard, so the distinct 3DNow!
  variants collapse to one form per `0F 0F modrm` prefix. Use `--no-skip-imm`
  to separate them.
* "Decodable" means *Zydis accepts it*. Zydis is a very complete decoder, but
  decoder acceptance is not identical to what a specific physical CPU executes.
