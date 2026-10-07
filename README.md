# ROSE with the open-source EDG front end, built by one Makefile

This repository contains everything needed to build the [ROSE](https://github.com/llnl/rose)
source-to-source compiler infrastructure for C and C++ with a plain GNU Makefile, using the
recently open-sourced [EDG C/C++ front end](https://github.com/edgcpp/compiler) built from source.

ROSE has always parsed C/C++ with EDG, but it was only distributed with prebuilt EDG
binaries, and ROSE's "EDG/Sage connection" (the code that turns EDG's intermediate language
into ROSE's Sage III AST) was never published.  `edg2sage/` is a new implementation of that
connection for the open-source EDG release.

```
make -j$(nproc)          # build/lib/librose.so, build/bin/identityTranslator, build/bin/dotGenerator
make check               # translate tests/ with identityTranslator and compare program output
build/bin/identityTranslator -c foo.c      # writes rose_foo.c and compiles it to foo.o
```

## Layout

| Directory | Contents |
|-----------|----------|
| `Makefile`, `rose-sources.mk` | the build: ROSETTA, generated Sage III IR, librose, EDG, tools |
| `config/` | templates for ROSE's generated configuration files (`rose_config.h`, `rosePublicConfig.h`, `rose_paths.C`) |
| `rose/` | the subset of ROSE 2.18.0 used by the build (C/C++ front end, Sage III, midend, unparser, ROSETTA) |
| `edg/` | the subset of the EDG front end used by the build |
| `edg2sage/` | the EDG IL → Sage III translator (new code), and the EDG configuration (`edgconfig/`) |
| `tools/` | translators linked against librose (`identityTranslator`, `dotGenerator` from ROSE's examples) |
| `tests/` | test programs and `run-tests.sh` (used by `make check`) |
| `scripts/vendor-sources.sh` | regenerates `rose/` and `edg/` from full checkouts |

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
   `edg2sage/`.
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
| `Cxx14_tests` | 31 | 30 |
| `Cxx17_tests` | 61 | 57 |
| all | 4,440 | 4,353 (98.0%) |

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
addresses of functions and members in template arguments.

Constructs that cannot be translated are reported as warnings (set `EDG2SAGE_DEBUG=1` for more
diagnostics) and skipped.  EDG options can be passed through ROSE with `--edg:<option>`, e.g.
`--edg:il_display` to dump the IL.

## Provenance and licenses

* `rose/`: ROSE 2.18.0, https://github.com/llnl/rose, commit
  `7e9c139f89a050e9fb9c0fddeef3c65cbfc4e643` — BSD 3-clause (`rose/LICENSE`).
* `edg/`: the EDG Compiler Project, https://github.com/edgcpp/compiler, commit
  `ab57e548ad206b0fe193a2d44a3f681c91e9c25f` — Apache License 2.0 with LLVM exceptions
  (`edg/LICENSE.txt`).  `edg2sage/edgconfig/cmake_defines.h` was generated by EDG's CMake
  build for its `linux-gcc-release` configuration.
* `tools/` are ROSE example translators (BSD).
* `edg2sage/`, the Makefile, `config/`, `scripts/` and `tests/` are new (MIT, see `LICENSE`).

The files in `rose/` and `edg/` are unmodified copies; `scripts/vendor-sources.sh` lists how
the subsets are chosen.
