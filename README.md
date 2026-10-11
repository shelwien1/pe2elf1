# vbn.exe → Linux ELF with pe2elf32

VBN 1.001 is a 1990s DOS-extender archiver. `vbn.exe`
is VBN repackaged as a PE32, with its DOS calls routed to a small shim DLL
(`vbnshim.dll`, built from `shim.c`). This repo converts that PE with
`pe2elf32` into a 32-bit Linux ELF, `vbn`, and builds the same `shim.c` as
`libvbnshim.so` for it. VBN's compressor, archive code and CRT run as the
original x86 bytes. Only the DOS services are reimplemented.

```sh
./vbn u archive input.txt    # MAX mode: writes archive.vbn
./vbn r archive              # restores input.txt into the current directory
./vbn                        # VBN's own help (commands b/B/u, r, l, p/c/d, volumes)
```

`libvbnshim.so` has to sit next to `vbn`, which finds it through
`DT_RUNPATH=$ORIGIN`. Running needs only the i386 glibc runtime
(`/lib/ld-linux.so.2`; Debian/Ubuntu: `libc6-i386`).

## Build and test

```sh
make         # pe2elf32/pe2elf32, then vbn and libvbnshim.so
make test    # or ./test.sh
```

The shim is compiled `-m32`, so building it needs the 32-bit dev files
(Debian/Ubuntu: `gcc-multilib`). The converter is a native host tool. `vbn`
and `libvbnshim.so` are committed, and `make` rebuilds them byte-identically.

`test.sh` checks that:

1. `vbn u archive input.txt` writes a 222691-byte `archive.vbn`;
2. `vbn r archive`, run in a directory holding only the archive, restores
   `input.txt` byte-exact, with its timestamp;
3. the original `vbn.exe` plus `vbnshim.dll` (cross-compiled from the same
   `shim.c`) under Wine writes a byte-identical archive. This step runs only
   if `wine` and `i686-w64-mingw32-gcc` are installed.

| File | |
|---|---|
| `vbn.exe` | Input: VBN as a PE32 (imports `vbnshim.dll!vbn_start`, `vbn_interrupt`) |
| `shim.c` | The DOS-service shim: Windows DLL, x86-64 host, and i386 for pe2elf32 |
| `pe2elf32/` | The converter, copied unmodified from [shelwien1/pe2elf](https://github.com/shelwien1/pe2elf/tree/claude/pe2elf32-implementation-zr219x) at `fe2cb91` |
| `vbn`, `libvbnshim.so` | Build outputs |
| `input.txt` | Test input: `book1` from the Calgary corpus |
| `test.sh` | The end-to-end test above |

## Archive size depends on the input's timestamp

VBN stores each file's DOS date and time, to the minute, in the archive, and
compresses them along with the data. So the archive's size depends on
`input.txt`'s mtime as well as its contents. Over the first hour after the
DOS epoch it ranges from 222681 to 222701 bytes, and a fresh copy of the
uploaded file gave 222697. 222691 comes up for 6 of those 60 minutes, the
first being 1980-01-01 00:13, which `test.sh` pins (in UTC). A copy of
`book1` with another mtime can give another size, from the original
`vbn.exe` just as from this port.

With equal timestamps, the port's output is byte-identical to the original
Windows build running under Wine. That holds for `u`, `b` and `B`, directory
recursion, `bf` with a mask, multi-volume archives (`b100`), and the
`p`/`c`/`d` error-protection cycle, and also for `l` and `r` (same listing,
same restored trees).

## How it fits together

`vbn.exe` (ImageBase `0xfff0000`, no relocations) has four sections:

- `.boot` is the entry point, `push 0x10000000; call [vbn_start]`.
- `.idata` holds the two imports.
- `.vbn` is VBN's original flat image at `0x10000000`, 64 MB with its BSS.
  The shim also puts VBN's heap, stack and argv there.
- `.shim` has one thunk per live `int 21h` site. Each thunk replays the
  instructions its `jmp` displaced, runs
  `pushfd; pushad; cld; push esp; call [vbn_interrupt]; add esp,4; popad; popfd`,
  and jumps back.

`pe2elf32 vbn.exe vbn --shim-soname libvbnshim.so` wraps the image in an
ET_EXEC ELF32 at that base, changing nothing but the zeroed IAT slots. `ld.so` fills the two IAT slots with
`vbnshim_vbn_start` and `vbnshim_vbn_interrupt` (pe2elf32 names an import
`<dll>_<function>`), and pe2elf32's startup trampoline calls
`shim_register_tls` before jumping to the PE entry point.

`vbn_start` copies argv into the image, with `/` turned into `\` in the
path arguments. It writes a startup stub that calls VBN's init routines,
`main` and `exit`, switches to the image's stack, and jumps to the stub.
`vbn_interrupt` reads the registers from the thunk's `pushad`/`pushfd`
frame, runs the DOS or extender service with POSIX calls, and writes the
results and CF back into the frame for `popad`/`popfd`.

## What changed in shim.c

The provided `shim.c` had two targets:

- the Windows DLL, whose `vbn_interrupt` is called by the thunks;
- an x86-64 host, where a 64-bit process far-returns into 32-bit code and
  services are dispatched from a SIGILL handler on `ud2`.

pe2elf32's output is a 32-bit process in which the thunks can call the shim
directly, so the new `#elif defined(__i386__)` path is the Windows calling
scheme on POSIX:

- `vbn_interrupt(frame)` does the same frame handling as the DLL.
- `vbn_start(image)` is stdcall, like the DLL export. It gets argc and argv
  from a constructor, because glibc passes them to shared-object constructors
  while the PE entry point receives neither.
- `shim_register_tls` is a stdcall no-op. pe2elf32's trampoline always calls
  it, and `vbn.exe` has no TLS directory.
- `vbnshim_vbn_start` and `vbnshim_vbn_interrupt` are exported as aliases.
- SIGSEGV, SIGBUS and SIGILL only report the fault: EIP, and its offset
  into the image.
- Non-Windows builds define `_FILE_OFFSET_BITS 64`, because on i386 plain
  `stat()` and `readdir()` fail with `EOVERFLOW` on 64-bit inode numbers.

The Windows and x86-64 code is unchanged. Preprocessed for mingw, the file
is identical to the original. For x86-64 only libc's headers differ: they
select the LFS (`*64`) names, which are the same types and functions there.

## Behaviour inherited from the shim

These are in the shim's shared code, so the Windows build behaves the same:

- The directory `vbn` starts in is the DOS drive root. Paths are resolved
  inside it, case-insensitively, and `..` cannot leave it.
- Only DOS 8.3 names fit. A matching name longer than 12 characters stops a
  directory scan with `VBN shim: DOS name exceeds 12 bytes`.
- `VBN_SHIM_TRACE=1` logs every DOS call to stderr.

VBN itself refuses inputs that are too small ("The file(s) is (are) too
small.").

## Decompiling `u` and `r` (`incdec/`)

`incdec/` moves vbn's own code for the plain `u` and `r` commands out of the
image and into C++, following `incdec/incdec.md`:

- 43 functions, taken from the Hex-Rays output;
- each one is patched in at its original address, then checked by an
  end-to-end gate (archives and restores byte-identical to this port's).

See [incdec/README.md](incdec/README.md).
