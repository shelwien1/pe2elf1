# Incremental decompilation of vbn's `u` and `r`

This directory applies [incdec.md](incdec.md) (written for PPMonstr on x64)
to the i386 port in the parent directory. Each function is taken from the
Hex-Rays output (`vbn.exe.c`), compiled with g++ into `dummy.so`, and patched
in at its original address with a `jmp`. Then the same end-to-end test runs
again. The moved bodies run alongside the original code that is still in
the image, so every function can be checked on its own.

**Status: all 43 functions that plain `vbn u` and `vbn r` execute are moved.**

- The gate passes on all of its runs:
  - 6 corpus entries (5 files and a directory tree);
  - for each one, `u` and `r`, plus `r` of the `b` and `B` archives;
  - 2 user errors.

  Archives are byte-identical, restores are exact (with their mtimes), and
  stdout is identical.
- Every moved function's probe fires (the table below).
- `fail.txt` is empty.
- The generated C++ compiles with g++'s default diagnostics and nothing
  suppressed (no `-fpermissive`, no `-w`): no errors, no warnings.
- `vbn2/vbn_ur.cpp`, generated from the same bodies, implements plain `u`
  and `r` as a standalone program: no `vbn.exe`, no pe2elf32, no shim. It
  passes the same gate ([Standalone](#standalone-vbn2vbn_urcpp)).
  `../vbn2.cpp`, VBN's compressor on its own (`vbn2 c|d`), started as that
  file.

The code that is still original is the C runtime (from `0x1000704C` on) and
the functions only other commands use (`list.txt`).

```sh
./build.sh            # dummy.cpp -> dummy.so; vbn.exe -> vbn.elf with dummy.so injected
./test.sh             # the gate (incdec.md §9); first run also writes ref/ with ../vbn
./step.sh sub_XXXXXXXX  # one iteration of §3: extract, build, gate; keep it or back it out
./xcheck.sh           # other commands, original port vs moved build (not a gate)
python3 tools/mklist.py   # list.txt: the scope, traced with gdb
python3 tools/arity.py    # conv.txt's parameter counts vs the pushes at each call site
python3 tools/mkvbn2.py   # vbn2/vbn_ur.cpp: the moved functions as a standalone program
./test.sh --exe vbn2/vbn_ur   # the gate, on another build (here: vbn_ur)
./vbn2/xcheck.sh      # more u/r sequences, ../vbn vs vbn_ur (not a gate)
```

You need what the parent directory needs (`gcc-multilib`, the i386 glibc),
plus `g++`, `python3`, `objdump`, `gdb` (for `mklist.py`) and the Python
`capstone` module (for `regflow.py`/`mkconv.py`).

## Scope: `list.txt`

`tools/mklist.py` runs the un-injected port (`../vbn`) under gdb, with a
one-shot breakpoint on every function entry, and records which entries were
hit. It does everything the gate does:

- `u` and `r` on each corpus entry;
- `r` of the entry's `b` and `B` archives;
- the commands in `errors.txt`.

That is incdec.md §1, narrowed to the two commands:

- **In scope:** vbn's own functions, below the C runtime, that these runs
  reach. There are 43.
- **Not in scope:** two functions in the program's range that only the C
  runtime calls: `sub_100001C7` (the extender's sbrk, DOS `350Ch`) and
  `sub_10000212` (stack-avail). They stay in the image with the rest of the
  library.
- **Not in scope:** 25 functions that plain `u`/`r` never reach:
  - 20 error-protection routines (`p`, `c`, `d`, and the `p` modifier);
  - 3 context-model routines (`"context`);
  - the volume-size parser (`b100`);
  - `sub_10000000`, the trap at the image base that reports a call through
    a null code pointer.

The corpus decides what is in scope, so it was grown until plain `u`/`r`
stopped finding new functions:

- `big.bin` is larger than the 0x198000 bytes the coder holds at once,
  which brought in the window slide, `sub_100050F8`.
- `tree/` has several files in two levels of subdirectories, which brought
  in qsort's comparator, `sub_10000458`. The tree also exercises recursion
  and directory creation on restore.
- `errors.txt` brought in the error exit, `sub_1000128F`.

The `b`/`B` restores and path arguments (`u sub/arc a.txt sub/deep/d.exe`)
reach nothing new.

## The gate: `test.sh`

The corpus is:

- `../input.txt` (book1);
- `../vbn.exe`;
- `corpus/mixed.bin`, `corpus/rep.bin`, `corpus/big.bin` (2.36 MB) and
  `corpus/tree/` (five files in `sub/` and `sub/deep/`), all deterministic,
  from `tools/mkcorpus.py`.

For each entry, the gate runs `u archive <file>` in a directory holding only
that input; for the tree, it copies the tree and runs `u archive *.*`. It
then runs `r archive` in an empty directory, plus `r` of the entry's `b` and
`B` archives, which the baseline made with `../vbn`. The binaries run from
their own directory, so that a `*.*` scan sees only the input. The gate
checks that:

- the archive is byte-identical to the reference (`ref/`, written by
  `test.sh --baseline` with the un-injected `../vbn`);
- each of the three restores recreates the input tree exactly: names,
  contents and mtimes (the inputs' mtimes are pinned to 1980-01-01 00:13
  UTC; VBN stores them);
- the stdout of every run matches the reference, except for the
  elapsed-time line.

Then each command in `errors.txt` runs in an empty directory and must give
the reference stdout and exit code:

- `r archive` with no archive: "Error archive file", "Process aborted";
- `u archive nosuch.txt`: "Nothing do".

Probe counts (§7.1) are summed over all of the gate's runs into
`probes.txt`:

| VA | function | role | probe |
|---|---|---|---:|
| `100002AC` | sub_100002AC | file size (DOS 4202h/4200h) | 46 |
| `100002E0` | sub_100002E0 | create (3Ch) | 36 |
| `100002FA` | sub_100002FA | open (3Dh) | 116 |
| `10000313` | sub_10000313 | read (3Fh) | 452 |
| `10000330` | sub_10000330 | write (40h) | 164 |
| `1000034D` | sub_1000034D | close (3Eh) | 112 |
| `10000369` | sub_10000369 | set attributes (4301h) | 30 |
| `100003ED` | sub_100003ED | volume extension `.NNb` (volume number in hex) | 42 |
| `10000458` | sub_10000458 | qsort's comparator for the file table: by extension, then by size | 8 |
| `1000055F` | sub_1000055F | split a path into drive, directory, name and extension | 33 |
| `100006FA` | sub_100006FA | scan a directory (and its subdirectories) into the file table | 9 |
| `10000C41` | sub_10000C41 | enter a relative path one directory at a time (creating missing ones), or climb back out | 425 |
| `10000D0E` | sub_10000D0E | input reader: files, archive volumes, or the in-memory title | 394 |
| `1000128F` | sub_1000128F | error exit: "Process aborted", free the table, exit(0) | 1 |
| `100012A8` | sub_100012A8 | output writer: archive volumes, restored files, or the in-memory title | 164 |
| `100018D0` | sub_100018D0 | parse a restored title into the file table | 18 |
| `10001AE2` | sub_10001AE2 | save the current drive and directory, change to another | 84 |
| `10001B38` | sub_10001B38 | change back to the saved drive and directory | 117 |
| `10001BA1` | sub_10001BA1 | build the file table, sort it, and serialize it as the title | 7 |
| `10001DAB` | sub_10001DAB | open the archive and read its header (or total the input sizes) | 43 |
| `10002017` | sub_10002017 | finish: rename `name.00b` to `name.vbn` | 24 |
| `100020E5` | sub_100020E5 | free the file table's name strings | 25 |
| `10002110` | sub_10002110 | split a `"context` name off the archive argument | 26 |
| `10002160` | sub_10002160 | main | 26 |
| `1000426D` | sub_1000426D | CRC (of the banner; the check itself is patched out) | 24 |
| `100048F0` | sub_100048F0 | dword copy | 482 |
| `1000490B` | sub_1000490B | dword clear | 340 |
| `10004924` | sub_10004924 | coder buffer: write 8 KB out (u), or read 8 KB in (r) | 466 |
| `10004993` | sub_10004993 | coder: emit the interval's settled bits and reset it | 802 |
| `10004A5F` | sub_10004A5F | coder driver (both directions) | 42 |
| `100050B0` | sub_100050B0 | frequency table: push a symbol into the 4096-entry history | 170016 |
| `100050F8` | sub_100050F8 | after the window slides: re-register its positions in the context tree | 8 |
| `10005251` | sub_10005251 | context tree update | 2210095 |
| `100053E2` | sub_100053E2 | tree search (with the table walk) | 2129121 |
| `100056C4` | sub_100056C4 | compressor/decompressor loop | 50 |
| `10005989` | sub_10005989 | tree search (with the plain walk) | 5235437 |
| `10005C50` | sub_10005C50 | coder core | 7364558 |
| `100061F4` | sub_100061F4 | coder core (frequency-list coding) | 12366765 |
| `100064C0` | sub_100064C0 | coder core | 5149473 |
| `10006CB0` | sub_10006CB0 | tree walk (register interface) | 26888439 |
| `10006E22` | sub_10006E22 | early exit, longjmp half | 7060607 |
| `10006E31` | sub_10006E31 | tree walk with a byte table (register interface) | 12756471 |
| `1000703B` | sub_1000703B | early exit, setjmp half | 7364558 |

Each function was moved alone, and the gate was green after every step
(`step.sh`). `accepted.txt` keeps them callee-first for the build; the git
history records the order they moved in, in batches.

`xcheck.sh` covers what the gate does not. Since `main` is moved, every
command now goes through decompiled code. The script runs these command
sequences with `../vbn` and with the moved build, in identical trees:

- the usage text;
- `l`;
- compressing with `b` and `B`;
- two named files;
- `b`/`bf` with `*.*`, recursing into two levels of subdirectories.

Stdout, exit codes and every resulting file are the same. These sequences
also run original code that was never moved, so passing them is evidence,
not a gate.

## How a function is moved

1. **Convention.** Hex-Rays prints most of vbn's functions as `__usercall`,
   with register parameters. `tools/regflow.py` follows every path through
   each function, into its `.shim` thunks and back, and reports the
   registers read before they are written. Apart from the cases in
   `mkconv.py`'s `MANUAL` table, none are. So the real interface is the
   stack arguments, and the `ret N` the function executes says who pops
   them. `conv.txt` holds the result: the convention, the bytes popped, and
   which Hex-Rays parameters are fake. The fake ones are dropped from the
   definition and from every call site.
2. **Extraction.** `tools/extract.py <name>` writes `inc/<name>.inc` from
   the Hex-Rays body:
   - typed references to every global (`vbn.txt`, from `tools/mksyms.py`),
     with arrays extended to the end of the image;
   - forwarders to callees that are already moved, and references at their
     PE address to callees that are not;
   - stack-frame reassembly when a local's address is taken;
   - the textual rewrites listed below;
   - a probe.

   `override/<name>.inc`, if it exists, is used instead (15 functions).
   `fixups.txt` holds exact-text corrections, applied before everything
   else (58 lines, 19 functions).
3. **Build and gate.** `tools/order.py` sorts `accepted.txt` callee-first and
   regenerates every `.inc`. `build.sh` compiles `dummy.so`, and `test.sh`
   runs the gate. Green: the step keeps the function and snapshots the
   sources into `backup`. Red: it takes the function back out.

## What vbn needed beyond incdec.md

**Calling conventions are a property of the call sites.**

- vbn's code relies on callees preserving every register but EAX,
  including ECX and EDX. `sub_10001DAB` sets ECX=0 as the open call's mode,
  then uses the same ECX as a `repnz scasb` count. So every moved body is
  `no_caller_saved_registers` (which needs `-mgeneral-regs-only`), and also
  `force_align_arg_pointer`, because vbn does not keep ESP 16-aligned.
- `sub_100056C4` is `ret 0Ch`. regflow lost track of it inside a loop and
  missed that, so `pop` comes from a linear scan of the function's own
  `ret` instructions.

**Register interfaces get thunks** (`override/`).

- The tree walkers `sub_10006CB0` and `sub_10006E31` take their inputs in
  EBX/EDX/ESI/ECX/EDI and return results in ECX/EDI. Each is plain C behind
  a naked thunk.
- The compressor's early exit is a setjmp/longjmp pair:
  - `sub_1000703B` records the stack slot where the *next* call's return
    address will go;
  - `sub_10006E22` restores ESP/EBP and executes `ret 4`, which returns
    from that next call.

  Only ESP and EBP survive the jump, so a g++ caller cannot make the pair
  of calls itself. `head.cpp`'s naked `vbn_restartable()` makes them, with
  EBX/ESI/EDI saved in a frame the exit never unwinds.

**Hex-Rays output that does not mean what it says:**

- *Lost variadic arguments.* Every `printf`/`scanf` call that passes
  arguments lost them: 23 sites in the moved functions. `fixups.txt`
  restores each one from the pushes in `vbn.exe.asm`:
  - "Code size= %u Ratio = %u%% or %u.%05d bpc" is recomputed from the
    asm's 64-bit `div`/`shrd` sequence;
  - " Time = %d sec." takes the value of the second `time()` call, which
    keeps the number of calls the same.
- *Wrong callee prototypes.* Hex-Rays gave mkdir (`sub_1000862D`) no
  parameter and no result. mkdir is `mov ah,39h` followed by a jump into
  chdir's tail, which reads the path at `[esp+4]`. So the moved
  `sub_10000C41` called mkdir without its path, and tested an
  uninitialized `v7` for the result.
  - extract.py cannot see this kind of error: the call matched the
    prototype.
  - The gate found it once the tree case made a restore create
    directories.
  - `tools/arity.py` now compares `conv.txt`'s parameter counts with the
    pushes at every call site in the moved functions. It finds no other
    mismatch.
- *Three-way compares.* `repe cmpsb; sbb eax,eax; sbb eax,-1` returns -1, 0
  or 1. Hex-Rays wrote it as `memcmp(...) != 0`, which is never negative.
  In qsort's comparator (`sub_10000458`) that changes the order of the
  files in the archive. The tree case fails with Hex-Rays' version and
  passes with the fixup.
- *EBP used as data.* `sub_1000426D` and `sub_10005251` keep data in EBP,
  which Hex-Rays prints as `&savedregs`. Both are rewritten by hand.
- *Flag arithmetic.* `sub_100050F8`'s unrolled tree descent (per bit:
  `add cl,cl; sbb; adc; jle`) came out as chains of
  `__CFADD__`/`__OFADD__`. The descent is the same as `sub_10005251`'s, and
  `sub_100050F8` is written by hand in the same way.
- *8-bit comparisons.* Comparisons between `char` and `unsigned char`
  values are made 8-bit again, using the types of the operands.
- *Shift counts.* Shift counts are masked with `& 31`, as x86 does.
- *Uninitialized registers.* `v13 = v10 - 1` with `v10` uninitialized is
  really `dec ecx` on the ECX=0 that survives the open call. Fixed with a
  fixup.
- *Signedness.* `_DWORD n13369312_1` makes one comparison unsigned where
  the asm uses `jge`. Fixed with a fixup.
- *Pointer types.* C++ rejects assigning `char *` to `_DWORD *`, which is
  legal C. Main needed 6 casts.
- *Names taken from strings.* Hex-Rays names some variables after the
  strings they point to: `.`, `..`, `_Data_size__%_10u_n`, and a global
  called `123456789012345`. These are renamed as whole tokens, and only in
  code. A global whose name cannot be a macro has its uses rewritten
  directly.
- *Globals without a type.* `_UNKNOWN` globals (`"*.*"`) become byte
  arrays, so their bare name is their address.
- *Fake parameters.* A function's own Hex-Rays signature can be wrong at
  its call sites too. `getdisk` was printed as `__thiscall`, and Hex-Rays
  passed it a bogus `this`.
- *Uninitialized values.* `g++ -Wall` flags 15 reads of variables that
  may be uninitialized. Each one was checked against the asm:
  - *A real divergence, latent.* `bsr` leaves its destination unchanged
    when the source is 0 (and so does `head.cpp`'s `_BitScanReverse`). In
    `sub_100064C0` the asm's EDX holds `v29` at that point, while the C++
    variable was never set. The source is 0 whenever `n2_1 == v86`, and the
    result feeds later shifts. Fixed: `v31 = v29;` before the `bsr`.
  - *A real divergence, in the exit status.* `sub_100020E5` returned an
    uninitialized value when the file table was empty, and that value
    becomes the exit status: `main` returns it, and the shim's startup stub
    is `call main; push eax; call exit`. The asm returns the caller's EAX,
    which is 0 on the one path that gets there (`u` with nothing to do).
  - *Wrong but harmless.* Two return values Hex-Rays read from stack slots,
    where the asm returns a popped register: `sub_1000128F` (unreachable
    after `exit`) and `sub_100018D0` (unused).
  - *A fictitious argument.* Hex-Rays gave the early exit's longjmp half,
    `sub_10006E22`, an argument its callers never push. Three call sites
    read an uninitialized local to pass it. `conv.txt` now gives it no
    parameter.
  - *False positives (the other 8).* Each is a `bsr` whose source is a
    counter plus a constant or a divisor used right after, or `v90`, which
    `sub_100064C0` saves and restores under the same condition.
- *C++ is not MSVC's C.* incdec.md §8.1 builds with `-fpermissive`. That
  turned 53 errors into warnings, which `-w` then hid. They are fixed where
  they come from:
  - `const` is dropped from casts and forwarders, as it already was from
    declarations;
  - a pointer cast to a byte or a word (`(char)v1`: Hex-Rays taking the low
    byte of a register) goes through `uintptr_t`;
  - the global `src__1`, which Hex-Rays declared as one `_BYTE`, is the
    archive's base name and is typed `char`;
  - three one-off sites are fixups. One of them replaces a `"\\"` literal
    with the image's own string, which Hex-Rays had inlined.
- *Signed overflow.* The range coder (`sub_10004993`, `sub_100061F4`)
  doubles its interval bounds as `int`s, which overflow, and then tests their
  sign bits; it also computes the range `high - low` and `range + 1` as `int`.
  The x86 code wraps. `build.sh`'s `-fwrapv` gave the bodies the same
  semantics, which hid this until vbn2 was built without it: then g++ -O2
  wrote a 222760-byte archive for `input.txt`. `-fsanitize=signed-integer-overflow,shift`
  located the sites, and 9 fixups make them unsigned. Three of them turn
  `(x << k >> m) & mask` bitfield extractions in `sub_100006FA` into plain
  shifts and masks.
- *Compiler assumptions.* `build.sh` still passes `-fno-strict-aliasing`,
  `-fwrapv`, `-fno-delete-null-pointer-checks` and
  `-fno-tree-loop-distribute-patterns`, which let Hex-Rays' C keep x86's view
  of memory and arithmetic on paths the gate does not run. The gate's paths
  no longer need them: `vbn2/vbn_ur.cpp` is built and tested without any of
  them.

**The DOS calls.**

- The seven file wrappers are rewritten (`override/`). Each fills a frame
  in `pushad` order and calls the shim through the IAT slot at
  `0x0FFF3064`, the way the thunks do.
- The shim refuses pointers outside the 64 MB image. vbn's stack is inside
  the image, so stack buffers work, but a string literal compiled into
  `dummy.so` cannot be passed to DOS. `extract.py` warns when it sees one.

**Startup and exit.**

- pe2elf32's entry never goes through `__libc_start_main`, so `dummy.so`'s
  destructors never run. The probe dump is registered with `atexit()`
  instead.
- The shim installs its signal handlers after `dummy.so`'s constructor. So
  the fault locator (it names the module and symbol of a crash) is armed on
  the first probe hit.

## Standalone: `vbn2/vbn_ur.cpp`

`tools/mkvbn2.py` assembles the moved functions into one C++ file,
`vbn2/vbn_ur.cpp`, that implements plain `u` and `r` by itself. (It is
generated, not committed. `../vbn2.cpp` began as this file and is VBN's
compressor alone now, `vbn2 c|d`; see `../README.md`.) Nothing of vbn.exe
runs: no pe2elf32 conversion, no shim, and none of vbn's machine code.

```sh
python3 tools/mkvbn2.py                         # vbn2/vbn_ur.cpp (about 8500 lines)
g++ -m32 -O2 -o vbn2/vbn_ur vbn2/vbn_ur.cpp
vbn2/vbn_ur u archive input.txt                 # or masks, *.* recursing into subdirectories
vbn2/vbn_ur r archive
```

The file has four parts:

1. **`vbn2/head.cpp`**: what the bodies used from outside, as native code.
   - Hex-Rays' types and helpers. The part-of-a-variable macros
     (`LODWORD`, `HIDWORD`, ...) use `may_alias` types, so they stay correct
     under g++'s strict aliasing.
   - vbn's memory. The bodies reach every global at its address, keep
     pointers in 32-bit integers, and tell a tree node (an address) from a
     leaf (a position, negated) by its sign. So the 64 MB image is mapped
     where it was, at `0x10000000` (`MAP_FIXED_NOREPLACE`), and its
     initialized data is copied in. The rest starts zeroed, like the BSS.
   - The heap. `malloc` carves blocks from the image at `0x11000000`, as
     the C runtime's sbrk (DOS `350Ch`) did.
   - DOS, as the shim serves it:
     - the start directory is the root, names resolve case-insensitively,
       and `..` cannot leave it;
     - directories list sorted, with 8.3 names only;
     - handles 5–255, and findfirst's DTA at `0x1000CD58`;
     - `4301h` only makes a file writable, and `5701h` goes through
       `mktime`/`futimens`.
   - vbn's C runtime, each function under the name and type vbn's code
     calls it by:
     - `printf` and `puts` (every newline comes out as `\r\r\n`, as through
       vbn's two text-mode layers), `scanf`, `getch`, `exit`;
     - `malloc`/`free`, `strcat`, `memset`;
     - vbn's own `fnsplit`, and Borland's `qsort`, step for step (its order
       for equal keys decides the order of files in the archive);
     - `getcwd`, `getdisk`/`setdisk`, disk free;
     - `chdir`, `mkdir`, `unlink`, `rename`, `findfirst`/`findnext`,
       setting a file's time, `time`.
   - The 9 out-of-scope functions that moved code can call (error
     protection, `"context` models, volume sizes) stop with "not supported".
     `main()` lets only `u` and `r` through, and those never reach them.
   - The early exit as `setjmp`/`longjmp`, and the walkers' exit.
2. **The 43 bodies**, callee-first, from `inc/`. For 11 of them the incdec
   version only works inside vbn.elf, and `vbn2/override/` replaces it:
   - the seven DOS wrappers called the shim through its IAT slot, and now
     call the native DOS layer;
   - the tree walkers lose their register-interface thunks;
   - the setjmp/longjmp pair is `head.cpp`'s.

   mkvbn2.py drops the probes and the `#ifndef __PE_DECL_...` references to
   functions left in vbn.exe, and it stops if one is not provided by
   `head.cpp`, or if any reference to vbn's code would remain.
3. **`vbn2/tail.cpp`**: `main()`, doing what vbn's startup did. It copies
   argv to `0x13E00000` (with `/` turned into `\` in the names), opens handles
   0–2, and returns main's result `& 255`. It runs vbn's main on a stack
   inside the image, at `0x13D00000` as in the port, so every address vbn's
   code handles is an image address.
4. **vbn's initialized data** (`0x1000A08C`–`0x1000D400`, 13172 bytes: its
   messages, tables and the C runtime's variables), as a string literal.

What was checked:

- The gate passes on vbn_ur (`./test.sh --exe vbn2/vbn_ur`), on all of its runs.
- `vbn2/xcheck.sh` runs 13 more sequences with `../vbn` and with `vbn_ur`.
  Output, exit codes and resulting files (mtimes included) are the same:
  - several files, masks, and a mask in a subdirectory;
  - names in another case;
  - restoring over existing files (vbn asks, and stdin is empty);
  - the same archive written twice;
  - an empty file, a truncated archive, and a file that is not an archive;
  - `r archive <dir>`, into an existing directory and into a new one.
- Builds with g++ 13 at `-O0`, `-O1`, `-O2`, `-O3` and `-Os`, and with
  clang++ 18 at `-O2`, all pass the gate. Under `-fsanitize=undefined` the
  gate passes too, and the only reports are misaligned loads and stores:
  vbn's archive structures are packed, and x86 allows them.
- Under strace, vbn_ur opens nothing but libc, its working directory,
  `/etc/localtime`, its inputs and its archive.
- It runs about 15% slower than vbn's machine code. For `big.bin`, `u` and
  `r` each take 5.4 s against 4.7 s.

vbn_ur implements nothing else. The other commands (`b`/`B`, `l`, `p`/`c`/`d`,
volumes, contexts) need the 25 out-of-scope functions, and vbn_ur prints its
usage for them. Its DOS layer is the shim's, so the shim's rules apply: the
start directory is the root, and only 8.3 names fit.

## Caveats

- **What is verified.** Only paths that the gate or `xcheck.sh` executes
  are verified. Some paths are moved but never run:
  - the error paths other than the two in `errors.txt` ("Disk full",
    "CAN'T CREATE FILE", "Write error", a full disk's prompt for a new
    path);
  - the prompt for a missing volume;
  - the context-model (`"context`) checks;
  - the "File %s is too large" message.

  Their bodies are the same mechanical translation, but nothing tests them.
- **What the moved code depends on.** In `vbn.elf` the moved code is not
  standalone:
  - it reads and writes vbn's globals at their image addresses;
  - it calls the C runtime and the 25 out-of-scope functions at their PE
    addresses;
  - it reaches DOS through the shim.

  `vbn2/vbn_ur.cpp` replaces the C runtime and DOS with native code, and keeps
  the globals at their addresses in a mapping of its own. The out-of-scope
  functions are not in it. vbn_ur is i386 code for the same reason vbn is:
  the bodies keep addresses in 32-bit integers.
- **Readability.** The bodies are Hex-Rays text plus mechanical rewrites,
  not cleaned-up source. Variable names are Hex-Rays', many of them taken
  from string contents.

## Files

| File | |
|---|---|
| `incdec.md` | The protocol (written for PPMonstr; §-references in comments point here) |
| `vbn.exe.c`, `defs.h` | Hex-Rays output and its header |
| `vbn.exe.asm` | IDA's listing, the reference for every fixup |
| `head.cpp` | `dummy.cpp`'s head: Hex-Rays types, intrinsics, the DOS call, the early-exit helpers, probes, fault locator, `patch_jmp` |
| `accepted.txt` | The moved functions, callees first |
| `inc/` | One `.inc` per moved function (generated, or copied from `override/`) |
| `override/` | Hand-written bodies: DOS wrappers, CRC, dword copy, context-tree update and its re-registration after a slide, tree walkers, early exit |
| `fixups.txt` | Exact-text corrections to Hex-Rays bodies, applied by `extract.py` |
| `conv.txt` | Real calling convention of every function (`tools/mkconv.py`) |
| `funcs.txt`, `vbn.txt` | Function banners and the symbol table (`tools/mkfuncs.py`, `tools/mksyms.py`) |
| `list.txt`, `fail.txt` | Tracking files (§3 step 9) |
| `build.sh`, `test.sh`, `step.sh`, `xcheck.sh` | Build, gate, one iteration, cross-check |
| `vbn2/` | `vbn_ur.cpp`'s own parts: `head.cpp` (native memory, DOS and C runtime), `override/` (its 11 bodies), `tail.cpp` (`main`), `xcheck.sh` (vbn vs vbn_ur) |
| `errors.txt` | The user-error commands the gate runs |
| `tools/` | The generators above, plus `extract.py`, `order.py`, `regflow.py`, `mkasm.py` (`vbn.asm`), `mkcorpus.py`, `mklist.py`, `arity.py`, `mkvbn2.py` (`vbn2/vbn_ur.cpp`) |
