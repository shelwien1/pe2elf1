# exe2dll + rundll

Turn a console `.exe` into a `.dll` that a launcher runs with
`rundll tool.dll,run <args>`, producing the same result as the original exe —
**without rewriting the tool as a DLL and without any runtime patching, manual
mapping, or debugger tricks.** The output is a plain, valid DLL that the normal
Windows loader (`LoadLibrary`) maps, relocates, and binds like any other.

The intended use is a wrapper GUI that drives a set of console tools: ship the
tools as `.dll`s so people don't casually double-click/run them, and have the
GUI invoke them through the bundled `rundll` loader.

Two tools:

| tool | runs on | what it does |
|------|---------|--------------|
| `exe2dll` | build machine (host or Windows) | static PE surgery: `exe` → `dll` |
| `rundll`  | Windows (end users) | loads the converted dll and runs it |

---

## How it works

`exe2dll` makes three small, static edits to a normal PE32+ (x64) image. It is
append-only: it writes one extra section header into the PE header's spare
padding and appends one section at the end of the file. Every existing section
keeps its file offset and RVA, so code, relocation targets, imports, TLS and
exception data stay valid untouched.

1. **Set `IMAGE_FILE_DLL`** in the COFF characteristics, so the loader accepts
   the file via `LoadLibrary` instead of refusing it.
2. **Replace the entry point with a `DllMain` stub** (`mov eax,1 ; ret`). So
   `LoadLibrary` does *not* run the program at load time — the stub just returns
   `TRUE` for `DLL_PROCESS_ATTACH`.
3. **Add an export directory** exporting one name (`run` by default) that points
   at the exe's *original* entry point — its C-runtime startup.

Calling the `run` export therefore runs the CRT startup exactly as the exe
would: `/GS` cookie init, global constructors, `main()`, then the CRT ends the
process (`exit` → `ExitProcess`). Because the input keeps its `.reloc` table and
`DYNAMIC_BASE`, the loader can place the dll at any base and fix it up normally.

`rundll` is the companion loader. It:

1. parses `rundll <dll>,<export> [args...]` (also accepts `rundll <dll> <export> [args...]`),
2. `LoadLibrary`s the dll and `GetProcAddress`es the export (ordinary loader,
   nothing exotic),
3. rewrites the process command line **in place** so the embedded program's CRT
   parses `argv` as if you had run `<dll> <args...>` directly (argv[0] = module
   path, argv[1..] = your args, forwarded verbatim including quoting),
4. calls the export. The tool runs and exits the process itself, so the exit
   code is the tool's own.

The command-line rewrite only ever writes a string *shorter* than `rundll`'s own
(it drops the `rundll.exe ` prefix and the `,export` selector), so it always
fits the existing buffers — nothing is grown or reallocated. It is an ordinary
console program in your user context; no elevation is required.

---

## Build

Needs a C++ compiler for `exe2dll` and a MinGW-w64 (or MSVC) toolchain for the
Windows `rundll`.

```bash
make                 # exe2dll (host) + rundll.exe (MinGW-w64)
```

Or by hand:

```bash
g++ -O2 -o exe2dll exe2dll.cpp
x86_64-w64-mingw32-g++ -O2 -static -o rundll.exe rundll.cpp
# MSVC:  cl /O2 /EHsc rundll.cpp
```

---

## Usage

Convert, then run:

```
exe2dll ttfcomp.exe ttfcomp.dll            # exportname defaults to "run"
rundll ttfcomp.dll,run a archive.bin font.ttf
```

Worked example (identical to running the exe):

```
rundll ttfcomp.dll,run a  arc.bin  input.ttf      ==  ttfcomp a arc.bin input.ttf
rundll ttfcomp.dll,run x -v arc.bin out.ttf        ==  ttfcomp x -v arc.bin out.ttf
```

A custom export name is supported: `exe2dll tool.exe tool.dll go` → `rundll tool.dll,go ...`.

---

## Requirements and limitations

- **x64 (PE32+) only.** The converter rejects 32-bit images.
- **Input must be relocatable** (have a `.reloc` directory / `DYNAMIC_BASE`),
  which console exes from modern linkers do. `exe2dll` warns if not; such an
  image would only load reliably at its preferred base.
- **One invocation = one run.** The CRT ends the process when the tool finishes,
  exactly like the exe. The `run` call does not return to the launcher; use a
  fresh `rundll` process per invocation (which is how a GUI driving a CLI tool
  works anyway).
- **This is a speed bump, not a security boundary.** It stops casual direct
  execution (the `.dll` won't do anything useful if double-clicked or run as a
  process), but a determined user can still call the export. Don't rely on it to
  protect secrets.
- The converter overwrites export data-directory slot 0, so it's meant for exes
  that don't already export anything (the normal case).
