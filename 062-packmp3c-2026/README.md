# mp3c — MP3 frame-dump compressor

```
mp3c c cat.dmp cat.c      compress
mp3c d cat.c  cat.unp     decompress
```

`cat.dmp` and `path.dmp` are the bundled test dumps. `mp3c.exe` is the original
reference build (Intel C++, x64) kept for comparison. `t.bat` runs a roundtrip
check against both dumps.

Build with `g.bat` (MinGW g++), `gc.bat` (clang++ with the MSVC headers/libs) or
`c.bat` + `icl64.cfg` (Intel icl).

## What was broken

The program did not survive a build with anything newer than gcc 10.2. There
were two separate causes, and they had to be fixed together.

### 1. `#pragma pack(1)` disappeared with the move to `Lib3`

`mp3rw/frame_dump_io.inc` serialises the *raw bytes* of `struct mp3frame`
between `&GrbLen` and `&Garbage`, and derives `max_frame_size` from
`offsetof(mp3frame, ...)`. The on-disk dump format is therefore the struct
layout.

`Lib/common.inc` contained a `#pragma pack(1)` that was never popped, so the
whole program was byte-packed. `Lib3/common.inc` has that line commented out —
correctly, since it is a shared header — so switching to `Lib3` silently changed
the layout. `read_frame_dump` then deposited the frame fields at the wrong
offsets and the model indexed off the end of its tables: SIGSEGV inside
`ProcessHeader`.

Fix: `mp3c.cpp` now asks for the packing itself, right after `common.inc`, so
`Lib3` stays layout-neutral for other programs.

### 2. The hand-rolled `my_setjmp` stopped working in gcc 11

`Lib/coro2b.inc` used `Lib/my_setjmp2_x64.h`, whose inline asm declares fifteen
`"=r"` outputs purely to force the register allocator to spill everything to the
stack — the coroutine's live state has to be *in the saved stack window*
`[stkptrL, stkptrH)`, because `yield()` memcpy's that window out and back. It
also hard-codes a `+15` byte offset into the instruction stream as its return
address.

gcc 11 no longer spills the way that trick assumed. The coroutine came back from
its first resume with stale state and quit immediately — a 2-byte output file.
The bisect is clean: gcc 10.5 fine, gcc 11.5 / 12.4 / 13.3 / 14.2 all broken, at
every `-O` level.

`Lib3/coro3b.inc` already avoided this by using `__builtin_setjmp` /
`__builtin_longjmp` for gcc (`coro3_setjmp_x64.h`) and an asm block with a
complete clobber list for clang and icc (`coro3_setjmp_x64d.h`). The 32-bit
branch had not been updated, so it still miscompiled on gcc 11+; this tree adds
`Lib3/coro3_setjmp_x32b.h` and selects it the same way the x64 branch does.

The isolation is clean: swapping *only* the setjmp in the old `Lib/coro2b.inc`
for `__builtin_setjmp`, changing nothing else, makes gcc 11 / 13 / 14 produce
the correct 138150 bytes again.

It is worth not "simplifying" this back to one asm implementation for every
compiler. Compressing `cat.dmp` with each setjmp choice, over gcc 10.5 / 11.5 /
12.4 / 13.3 / 14.2 and clang 18, at `-O2` / `-O3` / `-Ofast` (18 builds each):

| setjmp used for gcc | result |
| --- | --- |
| `__builtin_setjmp` (shipped) | 18/18 correct |
| `coro3_setjmp_x64d.h` asm | 3/18 — only the clang builds |
| `<setjmp.h>` (`-DCORO_NOASM`, fortify off) | 18/18 correct |
| old `Lib/my_setjmp2_x64.h` asm | 15/18 — gcc 14 fails |

The last row is the point: the old asm happens to survive longer inside
`coro3b` than it did inside `coro2b`, but it still breaks, just later. Both
asm variants depend on the register allocator spilling in a particular way, and
that is not something a compiler promises.

## Changes

| file | change |
| --- | --- |
| `mp3c.cpp` | `coro2b.inc` → `coro3b.inc`; explicit `#pragma pack(1)` |
| `mp3rw/mp3mod.inc` | ten more `using mp3model<f_DEC>::…;` declarations |
| `Lib3/coro3_setjmp_x32b.h` | new — `__builtin_setjmp` variant for 32-bit |
| `Lib3/coro3b.inc` | 32-bit branch selects it for gcc, mirroring the x64 branch |
| `g.bat`, `gc.bat`, `icl64.cfg` | include path `Lib` → `Lib3` |
| `t.bat` | new — roundtrip check |

The `using` declarations matter because the names are looked up in a dependent
base class. gcc only accepted them under `-fpermissive` and clang only under
`-fms-compatibility`; declaring them makes the code ordinary C++, so it now
builds clean on gcc *without* `-fpermissive` and on clang without MSVC
compatibility mode.

## Verification

Every build below compresses `cat.dmp` to **138150 bytes** and `path.dmp` to
**4385281 bytes** — byte-identical to the reference `mp3c.exe` — and decompresses
back to the original.

| toolchain | target | flags |
| --- | --- | --- |
| MinGW-w64 g++ 13 | x86_64 PE | `-Ofast -std=gnu++11`, and `-O2 -std=c++23` |
| clang 18 | x86_64 PE (`--target=x86_64-w64-mingw32`) | `-O2 -std=c++17`, and `-O3 -std=c++23 -ffast-math` |
| g++ 10.5 / 11.5 / 12.4 / 13.3 / 14.2 | x86-64 ELF | `-O2`, `-O3`, `-Ofast` |
| clang 18 | x86-64 ELF | `-O2`, `-O3`, `-Ofast` |
| g++ 10 / 13, clang 18 | i386 (`-m32`) | `-O3` |

The Windows binaries were exercised under Wine. gcc builds no longer need
`-fpermissive`.
