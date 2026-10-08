# ROSE with the open-source EDG front end, built by one Makefile

This repository contains everything needed to build the [ROSE](https://github.com/llnl/rose)
source-to-source compiler infrastructure for C and C++ with a plain GNU Makefile, using the
recently open-sourced [EDG C/C++ front end](https://github.com/edgcpp/compiler) built from source.

ROSE has always parsed C/C++ with EDG, but it was only distributed with prebuilt EDG
binaries, and ROSE's "EDG/Sage connection" (the code that turns EDG's intermediate language
into ROSE's Sage III AST) was never published.  `edg2sage/` is a new implementation of that
connection for the open-source EDG release.

```
make -j$(nproc)          # build/lib/librose.so and the programs in build/bin that use it
make check               # translate tests/ with identityTranslator and compare program output,
                         # and run the tests of the refactoring tools (tests/tools)
build/bin/identityTranslator -c foo.c      # writes rose_foo.c and compiles it to foo.o
```

The programs are ROSE's example translators `identityTranslator` and `dotGenerator`, and three
refactoring tools for C++ built on the EDG front end's cross-references (see
[Refactoring tools](#refactoring-tools)): `rose-ren` (lists or renames the declarations of a
name), `rose-using` (adds the using-declarations for names of dependent base classes that Visual
C++ finds and GCC and Clang do not) and `rose-m2g` (turns a member function into a global
function with an explicit `This` parameter).

The same Makefile cross-compiles ROSE for Windows with MinGW-w64, as `rose.dll` with an SDK (see
[Windows](#windows)).

## Layout

| Directory | Contents |
|-----------|----------|
| `Makefile`, `rose-sources.mk` | the build: ROSETTA, generated Sage III IR, librose, EDG, tools |
| `config/` | templates for ROSE's generated configuration files (`rose_config.h`, `rosePublicConfig.h`, `rose_paths.C`) |
| `rose/` | the subset of ROSE 2.18.0 used by the build (C/C++ front end, Sage III, midend, unparser, ROSETTA) |
| `edg/` | the subset of the EDG front end used by the build |
| `edg2sage/` | the EDG IL → Sage III translator (new code), the EDG configuration (`edgconfig/`), the cross-references for refactoring tools (`xref.C`), and a change to an EDG source (`patches/`) |
| `refactor/` | `RoseRefactor.h`: the API of the refactoring tools (cross-references, source text, edits), part of librose |
| `tools/` | the programs linked against librose: `identityTranslator` and `dotGenerator` (ROSE's examples), `rose-ren`, `rose-using`, `rose-m2g` |
| `tests/` | test programs and `run-tests.sh` (used by `make check`); `tests/tools/`: the tests of the refactoring tools |
| `win32/` | portability layer for the Windows build (MinGW-w64); the Windows package's `README.txt`, `test.bat` and examples; the SDK's `rose.mk` and examples Makefile (`sdk/`); the program that lists the exports of `rose.dll` (`build/`); the Makefile of the source package (`source/`) |
| `scripts/vendor-sources.sh` | regenerates `rose/` and `edg/` from full checkouts |
| `scripts/build-boost-mingw.sh`, `scripts/stage-sys-includes.sh`, `scripts/windows-sdk.sh`, `scripts/windows-source-package.sh` | Windows build helpers |

## Requirements

GNU make, g++ with C++14 support (tested with GCC 13.3), flex, bison, zlib and the Boost
libraries (chrono, date_time, filesystem, iostreams, program_options, random, regex, system,
thread, wave, serialization; tested with Boost 1.83).  On Debian/Ubuntu:

```
apt install g++ make flex bison zlib1g-dev libboost-all-dev
```

Make variables: `B=<build dir>` (default `build`), `OPT=` (default `-O2`), `CXX=`,
`BOOST_ROOT=<prefix>` for a Boost installation outside the default search paths,
`BACKEND_CC=`/`BACKEND_CXX=` (the compilers ROSE uses to compile unparsed code and whose
predefined macros and include paths the EDG front end emulates), `V=1` for full command lines.

A complete build compiles about 1,400 files; with `-j8` it takes roughly 20 minutes.

## How the build works

1. `config/*.in` are filled in with the compiler, Boost and version information
   (`build/gen/rose_config.h`, `rosePublicConfig.h`, `src/util/rose_paths.C`).
2. The ROSETTA generator (`CxxGrammarMetaProgram`) is built from `rose/src/ROSETTA` and run to
   generate the Sage III IR classes (`build/gen/src/frontend/SageIII/Cxx_Grammar*.{h,C}`, ...).
3. flex/bison generate the preprocessor-directive and OpenMP parsers.
4. The ROSE library sources listed in `rose-sources.mk` (the C/C++ parts of librose as configured
   by ROSE's own CMake build without binary analysis, Fortran, Java, Ada, Jovial or Python
   support) and the EDG sources are compiled into `build/lib/librose.so`, together with
   `edg2sage/` and `refactor/`; the programs in `build/bin` are linked against it.
5. EDG's error tables are generated with `mk_errinfo`, and its table of predefined macros
   (`build/edg-base/lib/predefined_macros.txt`) is generated from `BACKEND_CC`/`BACKEND_CXX`.

The EDG front end is compiled as C++ in namespace `edg`, configured (in
`edg2sage/edgconfig/edg_config.h`, on top of EDG's GCC/Linux configuration) as a callable
library that keeps the complete, unlowered IL with source sequence lists and calls a non-EDG
back end.  That back end is `edg2sage`: ROSE calls `edg_main()`, which runs the EDG front end
on the command line ROSE builds; once the IL of the translation unit is complete, EDG calls
`back_end()`, which translates it into the `SgSourceFile`.

## The translator (`edg2sage/`)

| File | Translates |
|------|------------|
| `edg_main.C` | entry point, EDG command line, the `back_end()` hook |
| `translator.C` | source positions, file names, scopes |
| `types.C` | types (and the declarations named types refer to) |
| `declarations.C` | the source sequence lists: variables, functions, classes, enums, typedefs, pragmas |
| `statements.C` | function bodies |
| `expressions.C` | expressions, constants, initializers, lambdas |
| `cxx.C` | namespaces, using declarations, base classes, constructor initializers, lambdas |
| `templates.C` | template declarations and template instances |
| `attributes.C` | GNU/C11 attributes that ROSE represents |

Status: C (C89 to C11 with GNU extensions) and C++ (up to C++17, including code using the
standard library) are translated.  ROSE's own compile tests, translated with
`identityTranslator -c` and the `-std` option ROSE's test harness uses for each directory
(`scripts/run-rose-tests.sh`; programs that GCC does not compile are not counted):

| Tests | Programs | Unparsed into code that compiles |
|-------|---------:|---------------------------------:|
| `C_tests` | 833 | 800 (96.0%) |
| `C99_tests` | 18 | 18 |
| `C11_tests` | 32 | 32 |
| `Cxx_tests` | 2,449 | 2,428 (99.1%) |
| `Cxx11_tests` | 1,016 | 988 (97.2%) |
| `Cxx14_tests` | 31 | 31 |
| `Cxx17_tests` | 61 | 57 |
| all | 4,440 | 4,354 (98.1%) |

Most of the remaining failures are tests that ROSE itself lists as failing
(`TESTCODE_CURRENTLY_FAILING` in their `Makefile.am`), a few that EDG rejects (they contain
copies of old GCC library headers), and limitations of ROSE's unparser: GNU attributes in
parameter types, transparent unions, `__builtin_va_arg`, function-like macros whose expansion is
printed, friend function definitions in classes in namespaces (printed with a qualified name),
and name qualification that ROSE computes once for constructs shared by several uses.

Templates are represented as ROSE's unparser expects them: template declarations
keep their text (EDG records it in the IL), which ROSE prints, and template instances become
`SgTemplateInstantiation*` declarations that are referenced (for names, types and calls) but not
printed, since the back-end compiler instantiates the templates again.  Explicit
specializations and instantiation directives are translated from the source.

Where ROSE has no representation of a type or expression, or its unparser prints one wrongly,
the translator uses the text EDG prints for it, as the name of a hidden typedef or variable:
GNU vector types, `decltype(auto)`, function and array types in template arguments, and the
addresses of functions and members in template arguments.  Generic lambdas (whose `operator()`
is a member template) are reproduced from their source text in the same way.

Constructs that cannot be translated are reported as warnings (set `EDG2SAGE_DEBUG=1` for more
diagnostics) and skipped.  EDG options can be passed through ROSE with `--edg:<option>`, e.g.
`--edg:il_display` to dump the IL.

## Refactoring tools

`refactor/RoseRefactor.h` is an API for refactoring tools that is part of librose: the
cross-references of a translation unit (each entity with its kind, full name, type, enclosing
class or namespace, access, base classes, overridden functions, and the positions of its
declarations and references), the source text (positions, a simple lexer, bracket matching), and
edits applied to the files all at once, keeping their formatting.  The cross-references come
from the EDG front end's cross-reference listing (`--xref`): a copy of EDG's `symbol_ref.c`
with the change in `edg2sage/patches/symbol_ref.c.sed` calls `edg2sage/xref.C` for each record,
while the symbol is in memory, and at the end of the front end each entity is described from
its IL entry.  Tools that only need the cross-references skip the translation into the AST.

The three tools change the source file and the headers it includes (except system headers) in
place; they take the compiler's options (`-I`, `-D`, `-std=...`) and `--dry-run`:

* `rose-ren file.cpp name` lists the declarations called `name` that are outside system headers,
  with an index: `[1] shapes.h:8  geo::Shape::area  double () const  (pure virtual member
  function), 2 references`.  `rose-ren file.cpp name[:index] newname` renames one (`name` alone
  if there is only one) in its declarations and all its uses (qualified names,
  using-declarations, base classes, template arguments and instances, names written in macro
  arguments); a class with its constructors and destructor, a virtual function with those that
  override it or that it overrides.  It refuses (unless `--force`) when the new name is already
  declared in the same scope, when a reference is also a reference to another entity (a template
  for other arguments), and when the name is written in a macro definition or declared in a
  system header.
* `rose-using file.cpp [class]` finds the names that class templates use without qualification
  and that refer to members of dependent base classes, which Visual C++ (without
  `/permissive-`) finds and GCC and Clang do not, and adds using-declarations for them to the
  class templates, with the access of the members (`using typename` for types, `template` for
  member templates).  The front end parses the file as Visual C++ does (`--no_dep_name
  --no_parse_templates`), so only the class templates that are instantiated are checked.
* `rose-m2g file.cpp Class::method` turns a member function into a global function with an
  explicit `This` parameter: `this` becomes `This`, members are accessed through it, static
  members, types and enumerators are qualified with the class, the declaration in the class
  becomes a friend declaration, a definition in the class moves after the class, and the calls
  become calls of the global function (`obj.f(x)` → `f(&obj, x)`, `p->f(x)` → `f(p, x)`).

By default, EDG emulates GCC by parsing the bodies of function templates only where they are
instantiated, so it records no references in templates that are not instantiated.  `rose-ren`
and `rose-m2g` therefore have all templates parsed (`--no_defer_parse_function_templates`); if
the front end rejects the file, it parses it again as GCC does, and then as Visual C++ does
(`RoseRefactor::setAlternativeFrontEndOptions`), so that code that only Visual C++ compiles can
be changed too, with a note.  The uses of the name that the front end did not resolve (in code
that the preprocessor skips, macro definitions, members of template parameters, or templates
that are not instantiated in the fallback modes) are listed and not changed.

With `--msvc`, the tools parse the code as Visual C++ does, with its headers instead of GCC's
(`RoseRefactor::setMicrosoftMode`): EDG's Microsoft mode (`--microsoft`, with the `_MSC_VER` and
build number of the headers' `crtversion.h`, or `--msvc-version=`) and its `win64` target
configuration (the data model and class layout of Visual C++ for x64; its table of predefined
macros, `edg-base/lib_win64`, is empty: EDG defines those of Visual C++, and edg2sage adds `_MT`
and `_CPPUNWIND`), with the folders of the `INCLUDE` environment variable (`--msvc`) or those of
a portable Visual C++ (`--msvc=<folder>`: `include`, `ucrt/include`, `sdk/include`) as the
system include directories instead of GCC's.  `-std=c++NN` becomes EDG's `--ms_c++NN`
(`/std:c++NN`).  The first run of the front end has two-phase name lookup (`--no_ms_permissive`,
cl's `/permissive-`), which it needs to parse the bodies of templates where they are defined;
the next ones parse as cl.exe does by default.  On Linux, the Visual C++ library and C run-time
headers can be used, but not the Windows SDK's, which include each other with names whose case
does not match the files'.  Tested with the headers of Visual Studio 2019 16.0 (`_MSC_VER` 1920),
on Linux and under Wine, where cl.exe 19.20 compiles the changed code with and without
`/permissive-`.

`tests/tools/` has the tests of the tools (run by `make check`): each test runs tools on copies
of its input files, compares the results with the expected files, and checks that the programs
build and print the same as before (or, for code that only Visual C++ compiles, that they
build after the changes).  The tests with the headers of Visual C++ (`tests/tools/msvc-headers`)
run when `MSVC_DIR` is the folder of a portable Visual C++.

## Windows

The Makefile cross-compiles ROSE for 64-bit Windows when `CXX` is a MinGW-w64 compiler.  The
result is `rose.dll` with the import library `rose.lib`, and a relocatable package (a 7z file)
with the DLL, the programs that use it, EDG's configuration, the system headers that the front
end parses with, and an SDK for programs that use ROSE: `rose.lib`, the headers, `rose.mk` for
Makefiles, and the programs' sources as examples (see `win32/README.txt`).  On Debian/Ubuntu:

```
apt install g++-mingw-w64-x86-64 libz-mingw-w64-dev 7zip wine   # cross compiler, zlib, 7-Zip; Wine for tests
scripts/build-boost-mingw.sh boost_1_83_0 $HOME/boost-mingw      # Boost for MinGW-w64, from an unpacked release
make B=build-win CXX=x86_64-w64-mingw32-g++-posix BOOST_ROOT=$HOME/boost-mingw -j$(nproc)
make B=build-win CXX=x86_64-w64-mingw32-g++-posix BOOST_ROOT=$HOME/boost-mingw package   # build-win/rose-2.18.0-win64.7z
```

Other programs are built the same way: `make B=build-win ... TOOL_SRC=<dir> build-win/bin/<name>.exe`
compiles `<dir>/<name>.C` and links it with `rose.lib`; or with the SDK, on Windows.  How the
Windows build differs from the Linux build:

* The programs that run during the build (ROSETTA's generator and EDG's `mk_errinfo`) are built a
  second time, for the build machine, with `HOST_CXX` (default `g++`) and its own Boost.
* The library is linked from the static library `librose.a` into `rose.dll`.  A DLL cannot
  export more than 65,535 names, and librose defines more than 100,000, so
  `win32/build/rose-exports.C` (run with `nm` on `librose.a`) writes the list of ROSE's API,
  `rose.def`: 62,643 names, without those of EDG, of the C and C++ run-time libraries, of
  instances of standard library and Boost templates, and of the internals of the IR classes.
  The MinGW C and C++ run-time libraries are linked statically into the DLL and into each
  program (no MinGW DLLs are needed); the programs have a 64 MB stack.  The SDK's headers are
  those that the programs include (from their dependency files, with the parts of Boost that
  they include), and its compiler options (`include/rose/rose.rsp`) are those the programs are
  compiled with, with the include directories relative to `include/rose` (`gcc -iprefix`), by
  `scripts/windows-sdk.sh`.  Built from the SDK under Wine with MSYS2's GCC 16.2 and
  `mingw32-make`, the programs pass the tests of the refactoring tools.
* The backend compiler is MinGW-w64 GCC: the predefined macros and system include directories
  are taken from the cross compiler, and the headers are copied into the package
  (`include/edg/`, by `scripts/stage-sys-includes.sh`).  At run time `rose.dll` finds them, and
  EDG's configuration, relative to its own location, and the translators run `gcc` and `g++`
  from the PATH to compile their output (any MinGW-w64 GCC, e.g. MSYS2's).
* EDG is configured for the MinGW-w64 x86_64 target (`edg2sage/edgconfig/mingw_x86_64.h`): the
  Windows data model (32-bit `long`, 16-bit `wchar_t`) and bit-field layout, with GCC's
  80-bit `long double` and the Itanium C++ ABI (`__attribute__((gcc_struct))` is not emulated).
  It reads source files as UTF-8, as GCC does.
* `win32/` adapts ROSE to MinGW-w64, so that `rose/` and `edg/` stay unmodified:
  `rose_mingw.h`, which is force-included in every compilation (it disables ROSE's DLL
  import/export attributes and declares the POSIX functions that `posix.C` implements),
  replacements for POSIX headers (`include/`), a `processSupport.C` that runs the backend
  compiler with `CreateProcess`, and changes to six ROSE sources (`patches/*.sed`, applied to
  copies of them by `patch-source.sh`): no DLL attributes in Sawyer, pointers formatted with a
  `0x` prefix (ROSE recognizes the names it makes from addresses by it), diagnostics on
  stderr, without ANSI colors, with the program's name, `\` as a separator in path names, and
  no check that IR nodes are not allocated within 10 KB of the stack frame (on Windows the heap
  can be next to the stack).  A few analysis modules that convert pointers to `long` are
  compiled with `-fpermissive` (the OpenAnalysis wrappers do not work on Windows).

`make check` runs the tests with Wine (the programs of the tests of the refactoring tools are
built with the host's `g++`); the translator needs a Windows `gcc`/`g++` in Wine's PATH, e.g.
MSYS2's `mingw-w64-x86_64-gcc` and its dependencies unpacked from
https://repo.msys2.org/mingw/mingw64/:

```
WINEPATH='Z:\path\to\mingw64\bin' make B=build-win CXX=x86_64-w64-mingw32-g++-posix BOOST_ROOT=$HOME/boost-mingw check
```

The package contains `test.bat`, a self test: it translates two example programs (C using the
Windows API, and C++17 using the standard library) with `identityTranslator`, writes the AST of
one with `dotGenerator`, changes a copy of a third (code that only Visual C++ compiles) with
`rose-ren`, `rose-m2g` and `rose-using`, and, when `gcc` and `g++` are in the PATH, checks that
the programs built from the translated code print the same as those built from the originals,
and that the changed program compiles and prints what it should.

`make ... source-package` makes `build-win/rose-2.18.0-win64-src.7z`, from which `rose.dll`,
`rose.lib` and the programs are built on Windows with MinGW-w64 GCC and `mingw32-make` alone
(`win32/source/BUILD.txt`).  It contains the files that the Windows build compiles and includes,
among them those that the build generates (so neither ROSETTA, flex, bison nor a shell is
needed); the parts of Boost that ROSE uses (the headers, selected with Boost's `bcp` tool, and
the sources of the few compiled parts that ROSE links); the files of the package that are not
built (the run-time files, and the SDK's headers, `rose.mk` and examples); and a Makefile
(`win32/source/Makefile`) whose recipes only run the compiler, `ar` and `rose-exports` (which
runs `nm`), with the compiler options in response files.  Built from it under Wine with MSYS2's
`mingw32-make` 4.4.1 and GCC 16.2 (1.5 hours with `-j4`), the programs pass `test.bat`, the tests
in `tests/` and those in `tests/tools/`.  Making it needs the Boost source tree, with `bcp` built
in it, and 7-Zip (`apt install 7zip`):

```
(cd boost_1_83_0 && ./bootstrap.sh && ./b2 tools/bcp)
make B=build-win CXX=x86_64-w64-mingw32-g++-posix BOOST_ROOT=$HOME/boost-mingw BOOST_SRC=$PWD/boost_1_83_0 source-package
```

ROSE's compile tests, translated under Wine with MSYS2's GCC 16.2 as the backend compiler
(`scripts/run-rose-tests.sh` with `CC='wine gcc'` and `CXX='wine g++'`; fewer C tests are
counted than on Linux, because many use POSIX headers that MinGW-w64 does not have):

| Tests | Programs | Unparsed into code that compiles |
|-------|---------:|---------------------------------:|
| `C_tests` | 765 | 735 (96.1%) |
| `C99_tests` | 19 | 18 |
| `C11_tests` | 30 | 30 |
| `Cxx_tests` | 2,385 | 2,363 (99.1%) |
| `Cxx11_tests` | 995 | 971 (97.6%) |
| `Cxx14_tests` | 30 | 30 |
| `Cxx17_tests` | 61 | 57 |
| all | 4,285 | 4,204 (98.1%) |

All but six of the tests that fail also fail on Linux.  Of the six, four have inline assembly
written for ELF targets or for a 64-bit `long` (the original programs do not assemble for
Windows either; the script only checks that GCC parses the tests), and two have a flexible array
member in an otherwise empty structure, which GCC 16 accepts and EDG, emulating the GCC 13 that
the package was built with, rejects.

## Provenance and licenses

* `rose/`: ROSE 2.18.0, https://github.com/llnl/rose, commit
  `7e9c139f89a050e9fb9c0fddeef3c65cbfc4e643` — BSD 3-clause (`rose/LICENSE`).
* `edg/`: the EDG Compiler Project, https://github.com/edgcpp/compiler, commit
  `ab57e548ad206b0fe193a2d44a3f681c91e9c25f` — Apache License 2.0 with LLVM exceptions
  (`edg/LICENSE.txt`).  `edg2sage/edgconfig/cmake_defines.h` was generated by EDG's CMake
  build for its `linux-gcc-release` configuration.
* `tools/identityTranslator.C` and `tools/dotGenerator.C` are ROSE example translators (BSD).
* `edg2sage/`, `refactor/`, the refactoring tools in `tools/`, the Makefile, `config/`,
  `scripts/`, `tests/` and `win32/` are new (MIT, see `LICENSE`).

The files in `rose/` and `edg/` are unmodified copies (the build applies its changes to a few of
them to copies: `win32/patches/`, `edg2sage/patches/`); `scripts/vendor-sources.sh` lists how
the subsets are chosen.
