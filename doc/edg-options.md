# EDG front-end options

ROSE parses C and C++ with the EDG front end (version 7.0), which is part of librose: for each
source file, ROSE builds a command line for the front end from its own gcc-like command line and
runs it.  This document lists the options of the front end in this build, with notes on their use
with ROSE: which ones ROSE sets, which ones are useful, and which ones do not work here.

The descriptions are condensed from EDG's manual (the "Command Line" chapter of
https://edgcpp.org/doc/, `doc/source/ext_intf.rst` in https://github.com/edgcpp/compiler).  The
notes were checked with `identityTranslator` and `rose-ren` of the Linux build, made with GCC
13.3.  The Windows build behaves the same, with the version and the headers of MinGW-w64 GCC.

* [Passing options to the front end](#passing-options-to-the-front-end)
* [What ROSE passes](#what-rose-passes), [the language version](#the-language-version)
* [What the refactoring tools add](#what-the-refactoring-tools-add)
* The options: [language dialect](#language-dialect), [input and output](#input-and-output),
  [preprocessor](#preprocessor), [diagnostics](#diagnostics),
  [language features](#language-features),
  [alternative language behaviors](#alternative-language-behaviors),
  [template instantiation](#template-instantiation), [code generation](#code-generation),
  [precompiled headers](#precompiled-headers), [C++/CLI](#ccli),
  [miscellaneous](#miscellaneous), [undocumented options](#options-that-edg-does-not-document),
  [options that are not in this build](#documented-options-that-are-not-in-this-build)
* [Environment variables](#environment-variables)

## Passing options to the front end

`identityTranslator`, the refactoring tools (`rose-ren`, `rose-using`, `rose-m2g`; options before
the file name) and the programs that call ROSE's `frontend()` pass options to the front end in
these forms:

| On ROSE's command line | Given to the front end | Example |
|---|---|---|
| `--edg:name` | `--name` | `--edg:display_error_number` |
| `--edg:name=value` | `--name=value` | `--edg:error_limit=5` |
| `-edg:X` | `-X` | `-edg:H` (`--trace_includes`) |
| `-edg:Xvalue` | `-Xvalue` | `-edg:e5` (`--error_limit 5`) |

* Options with a value need `name=value`.  ROSE's `--edg_parameter: name value` and
  `-edg_parameter: X value`, which `identityTranslator --help` describes, do not work: with a
  space after the colon ROSE drops the value, without it ROSE passes the value as another option
  (`--value`).
* Single-letter options need `-edg:`: `--edg:E` becomes `--E`, which the front end rejects.
* A keyword can be abbreviated to any unique prefix (`--edg:display_error`).  Where an option is
  given more than once, the last one counts (`-I` and `-D` add up).  ROSE puts the `-edg:` and
  `--edg:` options after its own options, so they override them, and then `--no_warnings`
  (see [diagnostics](#diagnostics)).
* `--edg:c` and `--edg:old_c` also make ROSE treat the file as C, and `--edg:c89` and
  `--edg:c99` set ROSE's C standard; choose the language with `-std=` instead
  ([the language version](#the-language-version)).
* `-rose:verbose 1` shows the front end's command line for each file, for example
  `identityTranslator -rose:verbose 1 -c test.c`:

  ```
  identityTranslator[...] EDG_ROSE_Translation[INFO ]: EDG command line: edg2sage --edg_base ... test.c
  ```

* The environment variable `EDG2SAGE_EDG_OPTIONS` adds options (separated by spaces) to every
  run of the front end, before ROSE's options (which win where they conflict).  It is meant for
  debugging.
* Options that keep the front end from calling its back end (edg2sage, which translates the
  front end's intermediate language into ROSE's AST), such as `-edg:E`, `--edg:list_macros` and
  `--edg:no_code_gen`, still write their output, but ROSE then reports that the front end failed
  (`Errors in Processing Input File`, exit status 1).

## What ROSE passes

Without `--msvc` (an option of the refactoring tools, see
[below](#what-the-refactoring-tools-add)), the front end runs in its GNU mode: it emulates the GCC the build was made with (13.3 on Linux),
with the headers of that GCC, the C library and Linux (MinGW-w64 on Windows) that are bundled in
`include/edg`.  For a C++ file without options, its command line is (shortened):

```
edg2sage --edg_base <prefix>/edg-base --no_strict_gnu --gnu_version 130300
  -D__GNUG__=13 -D__GNUC__=13 -D__GNUC_MINOR__=3 -D__GNUC_PATCHLEVEL__=0
  --sys_include <prefix>/include/edg/sys0/c++/13 ... --sys_include <prefix>/include/edg/sys0
  -DROSE_LANGUAGE_MODE=1 -DROSE_USE_EDG_QUAD_FLOAT -DUSE_RESTRICT_POINTERS_IN_ROSE_TRANSFORMATIONS
  -DUSE_ROSE --g++ --no_warnings file.cpp
```

`<prefix>` is the folder of the binary package, or the build folder.

| Option | Set by | Why |
|---|---|---|
| `--edg_base <prefix>/edg-base` | edg2sage | The front end's run-time configuration: GCC's predefined macros (`lib/predefined_macros.txt`) and the `win64` target (`lib_win64/`).  The environment variable `ROSE_EDG_BASE` names another folder.  (An abbreviation of `--edg_base_dir`.) |
| `--gnu_version 130300` | ROSE | Emulate GCC 13.3.  It implies GNU mode: `--gcc` for C, `--g++` for C++. |
| `-D__GNUC__=13` ... | ROSE | GCC's version macros (the front end defines them too). |
| `--sys_include <dir>` | ROSE | The folders of `-isystem` options, then the bundled headers, in GCC's search order (after the `-I` folders).  `/usr/include`, or the folder that the environment variable `USR_INCLUDE` names, is searched last, so the headers of other libraries are found. |
| `-DROSE_LANGUAGE_MODE=0` (C) or `=1` (C++), `-DUSE_ROSE`, `-DROSE_USE_EDG_QUAD_FLOAT`, `-DUSE_RESTRICT_POINTERS_IN_ROSE_TRANSFORMATIONS` | ROSE | Macros that code can test. |
| `--gcc`, `--g++` | edg2sage, ROSE | GNU dialects (no `-std`, or `-std=gnu*`). |
| `--c` (C), no option (C++) | ROSE | ISO dialects (`-std=c*`, `-std=c++*`): GNU mode emulating `gcc -std=c*`. |
| `--c89` ... `--c++26` | ROSE | The language version, from `-std=` (see the next table). |
| `-D__STRICT_ANSI__=1` | ROSE | ISO dialects, as gcc defines it. |
| `--no_strict_gnu` | edg2sage | GNU dialects.  Since ROSE always gives the language version, the front end would otherwise emulate `-std=c*`: define `__STRICT_ANSI__`, so that the C library hides POSIX names such as `PATH_MAX`. |
| `-D__cpp_impl_coroutine` | ROSE | C++20 and later. |
| `-I`, `-D`, `-U` | ROSE | The same gcc options. |
| `--preinclude <file>` | ROSE | gcc's `-include <file>`. |
| `-D__OPTIMIZE__` | ROSE | gcc's `-O1`, `-O2`, ... |
| `--no_warnings` | ROSE | Always: the front end reports only errors (see [diagnostics](#diagnostics)). |
| `--auto_instantiation`, `-tused`, `-tlocal`, `-tall` | ROSE, removed by edg2sage | The front end instantiates by itself what the AST needs; there is no prelinker. |

The other compiler options are only for the compiler that ROSE runs on its output: the front end
parses with gcc's defaults.  For some, an option tells the front end the same:

| gcc option | Front end option |
|---|---|
| `-funsigned-char` | `--edg:unsigned_chars` |
| `-fshort-enums` (C) | `--edg:short_enums` |
| `-fno-exceptions` | `--edg:no_exceptions` |
| `-fno-rtti` | `--edg:no_rtti` |
| `-fgnu89-inline` (C) | `--edg:gcc89_inlining` |
| `-imacros <file>` | `--edg:preinclude_macros=<file>` |
| `-fconstexpr-depth=<n>` | `--edg:max_depth_constexpr_call=<n>` |
| `-ftemplate-depth=<n>` (not quite the same limit) | `--edg:pending_instantiations=<n>` |

### The language version

ROSE selects the language version from `-std=`:

| `-std=` | Front end options | Version macro |
|---|---|---|
| none (C) | `--gcc --c99` | `__STDC_VERSION__` is `199901L` (gnu99), while gcc 13 compiles gnu17 (`201710L`) by default: give `-std=gnu17` to parse as gcc compiles |
| `gnu89`, `gnu99`, `gnu11`, `gnu17`, `gnu18`, `gnu23` | `--gcc --c89` ... `--c23`, `--no_strict_gnu` | |
| `c89`, `c99` (`c9x`, `iso9899:1999`), `c11` (`c1x`), `c17`, `c18`, `c23` | `--c --c89` ... `--c23`, `-D__STRICT_ANSI__=1` | |
| `c2x`, `gnu2x` | not recognized by ROSE: as without `-std` | use `c23`, `gnu23` |
| none (C++) | `--g++` | `__cplusplus` is `201703L` (gnu++17), as for g++ 13 |
| `gnu++98`, `gnu++11`, `gnu++14`, `gnu++17`, `gnu++20`, `gnu++23` (`gnu++2b`), `gnu++26` | `--g++ --c++03` ... `--c++26`, `--no_strict_gnu` | |
| `c++98`, `c++03`, `c++11` (`c++0x`), `c++14` (`c++1y`), `c++17` (`c++1z`), `c++20` (`c++2a`), `c++23` (`c++2b`), `c++26` (`c++2c`) | `--c++03` ... `--c++26`, `-D__STRICT_ANSI__=1` | |

Select the version with `-std=` rather than with `--edg:c++20` and the like: ROSE also passes
`-std=` to the compiler, and sets its own language mode from it.

## What the refactoring tools add

`rose-ren`, `rose-m2g` and `rose-using` add options to ROSE's (`-rose:verbose 1` shows them).
They run the front end again with other options if it rejects the file.

| Options | When | Why |
|---|---|---|
| `--xref <temporary file>`, and `--instantiate used` for C++ | always | Records the cross-references (the front end gives each record to edg2sage); instantiates the templates that are used, so that the references in their bodies are seen. |
| `--no_defer_parse_function_templates` | first run of rose-ren and rose-m2g | Parses the bodies of all function templates, also those that are not instantiated (GNU mode parses them only when they are instantiated). |
| none | second run of rose-ren and rose-m2g | As g++ parses. |
| `--no_dep_name --no_parse_templates` | last run of rose-ren and rose-m2g, rose-using's only run | Looks up the names in templates where they are instantiated, as Visual C++ does without `/permissive-`. |
| `--error_output <temporary file>` | runs that may be followed by another | The diagnostics of a run are shown only for the run whose result is used. |
| `--target win64 --microsoft --microsoft_version <N> [--microsoft_build_number <B>] --sys_include <dir> ... -D_MT=1 -D_CPPUNWIND=1` | `--msvc` | Parses as Visual C++ does: its data model and class layout (the `win64` target configuration), its language and predefined macros, and its headers instead of GCC's.  ROSE's `--gnu_version`, `--sys_include` and `-D__GNUC__...` options are left out, `--g++` and `--gcc` become `--c++` and `--c`. |
| `--ms_c++14`, `--ms_c++17`, `--ms_c++20` or `--ms_c++latest`, `--ms_c11`, `--ms_c17` | `--msvc` | The language version, from `-std=` (default `--ms_c++14`, as cl.exe's `/std:c++14`). |
| `--no_ms_permissive` | `--msvc`, in the first run | Visual C++ parses templates where they are defined only with `/permissive-`. |

`--msvc` is the way to use the front end's Microsoft mode: `--edg:microsoft` with
`identityTranslator` is rejected ("language modes specified are incompatible"), since the front
end is in GNU mode.

## The options

The options are grouped as in EDG's manual.  Defaults are those of this build in the modes ROSE
selects.

### Language dialect

| Option | Effect | Notes |
|---|---|---|
| `--c`<br>`-m` | Compile C (C++ is the default). | Set by ROSE for C in ISO dialects. |
| `--c89` | C89. | From `-std=c89`, `-std=gnu89`. |
| `--c99`<br>`--no_c99` | C99 (or C89); implies C. | From `-std=c99`, `-std=gnu99`; ROSE's default for C. |
| `--c11` | C11. | From `-std=c11`, `-std=gnu11`. |
| `--c17`<br>`--c18` | C17 (C11 with corrections). | From `-std=c17`, `-std=c18`, `-std=gnu17`, `-std=gnu18`. |
| `--c23` | C23. | From `-std=c23`, `-std=gnu23`. |
| `--old_c`<br>`-K` | K&R C, as pcc compiles it. | Rejected in GNU mode ("language modes specified are incompatible"). |
| `--c++`<br>`-p` | Compile C++ (the default). | |
| `--c++03` | C++03: the C++11 features off (they can be enabled one by one). | From `-std=c++98`, `-std=c++03`, `-std=gnu++98`. |
| `--c++11`<br>`--c++0x`<br>`--no_c++11`<br>`--no_c++0x` | C++11 (or not). | From `-std=c++11`, `-std=gnu++11`. |
| `--c++14` | C++14. | From `-std=c++14`, `-std=gnu++14`. |
| `--c++17` | C++17. | From `-std=c++17`, `-std=gnu++17`; also the default of the g++ 13 that the front end emulates. |
| `--c++20` | C++20. | From `-std=c++20`, `-std=gnu++20`. |
| `--c++23` | C++23. | From `-std=c++23`, `-std=gnu++23`. |
| `--c++26` | Draft C++26. | From `-std=c++26`, `-std=gnu++26`. |
| `--anachronisms`<br>`--no_anachronisms` | Accept anachronisms of early C++. | Off by default. |
| `--cfront_2.1`<br>`-b`<br>`--cfront_3.0` | Accept what AT&T cfront 2.1 or 3.0 accepted. | Rejected in GNU mode. |
| `--c++cli`<br>`--cppcli`<br>`--clr`<br>`--no_c++cli`<br>`--no_cppcli` | C++/CLI; implies Microsoft mode (`--microsoft_version` 1600 or later). | Not usable: rejected in GNU mode, and ROSE's AST has no C++/CLI constructs. |
| `--c++cx`<br>`--cppcx`<br>`--no_c++cx`<br>`--no_cppcx` | C++/CX (preliminary). | As `--c++cli`. |
| `--clang`<br>`--no_clang` | Clang compatibility, also enabling GNU mode: mainly `__has_feature` and the like. | Fails with the bundled headers: the predefined macros in `edg-base/lib/predefined_macros.txt` (`__CHAR_BIT__`, ...) are made for gcc and g++ modes, and are not defined in clang mode. |
| `--clang_version <n>` | The version of clang to emulate (x.y.z as x*10000+y*100+z); implies clang mode. | As `--clang`. |
| `--embedded_c`<br>`--no_embedded_c` | The Embedded C extensions (C only): fixed-point types, named address spaces, named registers. | Parsed, but ROSE's AST cannot represent them: declarations and statements with fixed-point values are left out of the AST (with a warning). |
| `--embedded_c++` | Diagnose what the Embedded C++ subset excludes (templates, exceptions, namespaces, ...). | Its diagnostics are warnings, which are not shown. |
| `--g++`<br>`--no_g++` | GNU C++ mode; implies C++. | Set by ROSE for C++ in GNU dialects (`--gnu_version` implies it anyway). |
| `--gcc`<br>`--no_gcc` | GNU C mode; implies C. | Set by edg2sage for C in GNU dialects. |
| `--gnu_version <n>` | The version of GCC to emulate (x*10000+y*100+z); implies GNU mode. | Set by ROSE to the version of the build's GCC.  `--edg:gnu_version=120000` overrides it, but ROSE's `-D__GNUC__=13` and the headers stay those of GCC 13. |
| `--microsoft`<br>`--microsoft_16`<br>`--no_microsoft` | Microsoft mode (`--microsoft_16`: 16-bit, with `near` and `far`). | Rejected in GNU mode; the tools' `--msvc` sets it. |
| `--microsoft_version <n>` | The `_MSC_VER` of the Visual C++ to emulate; implies Microsoft mode. | Set by `--msvc`: from `crtversion.h` of the headers, or `--msvc-version=<n>`. |
| `--microsoft_build_number <n>` | The build number in `_MSC_FULL_VER` (default 99999); implies Microsoft mode. | Set by `--msvc` when the headers give it. |
| `--microsoft_bugs`<br>`--no_microsoft_bugs` | Emulate some bugs of Visual C++; implies Microsoft mode. | On in Microsoft mode. |
| `--ms_c++14`<br>`--ms_c++17`<br>`--ms_c++20`<br>`--ms_c++23`<br>`--ms_c++latest` | As cl.exe's `/std:c++14` ... `/std:c++latest`, for the emulated version. | Set by `--msvc` from `-std=`. |
| `--ms_c11`<br>`--ms_c17`<br>`--ms_c23` | As cl.exe's `/std:c11`, `/std:c17`, `/std:c23`. | Set by `--msvc` from `-std=` (`--ms_c17` also for C23). |
| `--ms_await`<br>`--ms_await_strict` | As `/await` and `/await:strict` (coroutines before C++20). | Microsoft mode. |
| `--ms_compatibility`<br>`--no_ms_compatibility` | As clang's `-fms-compatibility` (implies `--ms_extensions`); can be combined with other modes. | Accepted in GNU mode. |
| `--ms_extensions`<br>`--no_ms_extensions` | As clang's `-fms-extensions`; can be combined with other modes. | Accepted in GNU mode; does not make Microsoft's types such as `__int64` known. |
| `--ms_permissive`<br>`--no_ms_permissive` | As `/permissive` and `/permissive-` (permissive by default); implies Microsoft mode. | Set by `--msvc` in the tools' first run. |
| `--ms_std_preprocessor`<br>`--no_ms_std_preprocessor` | A standard preprocessor in Microsoft mode (as `/Zc:preprocessor`). | Microsoft mode. |
| `--ms_stdc`<br>`--no_ms_stdc` | As `/Zc:__STDC__`; implies Microsoft mode. | |
| `--ms_cplusplus_std_value`<br>`--no_ms_cplusplus_std_value` | As `/Zc:__cplusplus` (`__cplusplus` of the standard, not `199711L`); implies Microsoft mode. | |
| `--nonstd_gnu_keywords`<br>`--no_nonstd_gnu_keywords` | GNU keywords without underscores (`typeof`). | On in GNU mode, also with `-std=c++17` (unlike g++). |
| `--strict_gnu`<br>`--no_strict_gnu` | Emulate gcc's `-std=c*` or `-std=gnu*`.  Without either, a version option (`--c99`, `--c++17`) means `-std=c*`. | `--no_strict_gnu` is set by edg2sage for GNU dialects. |
| `--strict`<br>`-A`<br>`--strict_warnings`<br>`-a` | Strict ANSI/ISO mode: errors (or warnings) for extensions. | Rejected in GNU mode, also with `-std=c++17`. |
| `--stricter_template_checking` | Stricter checks in templates where the dialect weakens them. | |
| `--svr4`<br>`--no_svr4` | SVR4 C compatibility; implies ANSI C. | Rejected in GNU mode. |
| `--upc`<br>`--no_upc` | Unified Parallel C; implies C. | ROSE passes it, with `--restrict`, for `-rose:UPC` and `--edg:upc`.  Parsed, but the translation drops UPC's qualifiers (`shared int x;` becomes `int x;`). |
| `--upc_threads <n>` | A fixed number of UPC threads (`THREADS`). | From `-rose:upc_threads <n>`. |
| `--upc_strict`<br>`--upc_relaxed` | The default access mode of UPC shared objects. | |

### Input and output

| Option | Effect | Notes |
|---|---|---|
| `--error_output <file>` | Diagnostics to *file* instead of stderr. | Works: `--edg:error_output=<file>`.  The tools use it for runs that may be followed by another. |
| `--list <file>`<br>`-L<file>` | A raw listing: source lines, transitions into and out of include files, and diagnostics, each line starting with a key letter. | Works. |
| `--xref <file>`<br>`-X<file>` | A cross-reference listing: a line for each reference, *symbol-id name ref-code file line column* separated by tabs; *ref-code* is `D` definition, `d` declaration, `M` modification, `A` address taken, `U` use, `C` use and modification, `R` other reference, `T`/`t` template instantiation, `E` error. | Works; the symbol id is the address of the symbol (hexadecimal) in this build.  The tools pass it themselves, with a temporary file, which this option replaces. |
| `--output <file>`<br>`-o <file>` | The output file of preprocessing (default: stdout). | Only with preprocessing-only options; otherwise the front end stops ("an output file was specified, but none is needed").  Not ROSE's `-o`. |
| `--dependencies`<br>`-M` | Preprocess only, and write the dependencies of the file for make. | Written to stdout; then ROSE reports a front-end failure. |
| `--list_macros` | Preprocess only, and list the macros defined at the end: predefined, from the command line and from the file. | Shows the predefined macros of a mode (to stdout); then ROSE reports a front-end failure. |
| `--no_line_commands`<br>`-P` | Preprocess only, without line directives. | As `--preprocess`. |
| `--unicode_source_kind <kind>` | The encoding of source files without a byte order mark: `UTF-8`, `UTF-16`, `UTF-16LE`, `UTF-16BE`, `none`. | |
| `--create_header_unit <file>`<br>`--create_module_interface <file>`<br>`--create_module_internal_partition <file>`<br>`--module_interface`<br>`--module_internal_partition` | Write an EDG IFC module file (C++20 modules). | ROSE does not support modules. |

### Preprocessor

| Option | Effect | Notes |
|---|---|---|
| `--preprocess`<br>`-E` | Preprocess only, with line directives. | Use `-edg:E` (`--edg:E` does not work): the output goes to stdout, then ROSE reports a front-end failure.  With `--edg:no_preproc_only` too, ROSE also translates the file. |
| `--no_preproc_only` | With a preprocessing-only option: compile as well. | See `--preprocess`. |
| `--comments`<br>`-C` | Keep comments in the preprocessing output. | `-edg:E -edg:C`. |
| `--define_macro <name>[(<params>)][=<def>]`<br>`-D...` | Define a macro (as `1` without `=<def>`). | ROSE passes its `-D` options. |
| `--undefine_macro <name>`<br>`-U<name>` | Remove a predefined macro; after all `-D` options. | ROSE passes its `-U` options. |
| `--include_directory <dir>`<br>`-I<dir>` | Add a folder to search for `#include`s. | ROSE passes its `-I` options. |
| `--sys_include <dir>` | Add a folder of system headers: searched in command-line order, as `-I`, but no warnings are given for its headers. | Set by ROSE for `-isystem` and for the bundled headers.  The tools do not change system headers. |
| `--preinclude <file>` | Include *file* at the start. | ROSE passes `-include <file>` this way. |
| `--preinclude_macros <file>` | Only the macros of *file*, before the other preincludes. | ROSE does not pass gcc's `-imacros`: use `--edg:preinclude_macros=<file>`. |
| `--incl_suffixes <list>` | The suffixes tried for `#include` names without one (separated by colons). | Default `::stdh:`. |
| `--embed_directory <dir>` | Add a folder to search for `#embed` (C23). | |
| `--import_dir <dir>` | The folder of the files of `#import` (Microsoft mode). | |
| `--modules_directory <dir>`<br>`--header_unit <path>=<file>`<br>`--ms_header_unit <path>=<file>`<br>`--ms_header_unit_angle <path>=<file>`<br>`--ms_header_unit_quote <path>=<file>`<br>`--ms_mod_file_map [<name>=]<file>`<br>`--ms_translate_include`<br>`--no_ms_translate_include` | Find module files (made by Visual C++) for `import`, and header units for `#include`. | ROSE does not support modules. |
| `--old_style_preprocessing` | pcc-style preprocessing in ANSI C or C++ mode. | |
| `--stdc_zero_in_system_headers`<br>`--no_stdc_zero_in_system_headers` | `__STDC__` is 0 in system headers. | Off by default. |
| `--trace_includes`<br>`-H` | List the included files on stderr (indented by depth); compiles as usual. | Works: `--edg:trace_includes` or `-edg:H`; shows which header is used. |
| `--check_concatenations`<br>`--no_check_concatenations` | Diagnose a `##` that does not form a valid token. | Off by default. |
| `--no_token_separators_in_pp_output` | No extra spaces between tokens in the preprocessing output. | |

### Diagnostics

ROSE passes `--no_warnings` after the `--edg:` options, so the front end reports only errors:
warnings and remarks are not shown, and options that only add warnings or remarks have no
visible effect.  A program that uses the SDK can call
`Rose::global_options.set_frontend_warnings(true)` before `frontend()` to keep the warnings.

* A warning can be made visible by raising it to an error with `--edg:diag_error=<tag>`.  This
  is how the reason why precompiled headers are not used was found (see
  [precompiled headers](#precompiled-headers)).
* Errors whose number ends in `-D` (`error #38-D`, with `--edg:display_error_number`) are
  discretionary: `--edg:diag_suppress=38` (or `diag_warning`, `diag_remark`) lets such code
  through.  The other errors cannot be suppressed.
* A diagnostic is named by its number or its tag.  The number (`--edg:display_error_number`) is
  the position of the message in `edg/src/error_msg.txt`, counting from 0 and including the
  `REMOVED` lines; the tag is its name there without `ec_`: `#20` is `undefined_identifier`,
  `#38` is `pp_else_already_appeared`.

| Option | Effect | Notes |
|---|---|---|
| `--display_error_number`<br>`--no_display_error_number` | Show the number of each diagnostic (`error #20:`); `-D` marks discretionary errors. | Off by default; works. |
| `--diag_suppress <tags>`<br>`--diag_remark <tags>`<br>`--diag_warning <tags>`<br>`--diag_error <tags>` | Change the severity of diagnostics (numbers or tags, separated by commas). | Works: `--edg:diag_suppress=38`.  Lowering works for discretionary errors (and warnings); raising a warning to an error shows it despite `--no_warnings`. |
| `--diag_once <tags>` | Give these diagnostics only once, as warnings or remarks. | Not shown. |
| `--error_limit <n>`<br>`-e<n>` | Stop after *n* errors (default 100). | `--edg:error_limit=5` or `-edg:e5`. |
| `--brief_diagnostics`<br>`--no_brief_diagnostics` | A line per diagnostic, without the source line. | Works. |
| `--wrap_diagnostics`<br>`--no_wrap_diagnostics` | Wrap long messages (on by default). | |
| `--context_limit <n>` | The maximum number of template instantiation context lines (default 10; 0: no limit). | |
| `--add_match_notes`<br>`--no_add_match_notes` | Notes on why the candidates of an overloaded call do not match (on by default). | |
| `--template_typedefs_in_diagnostics`<br>`--no_template_typedefs_in_diagnostics` | Show typedefs of template classes in messages, or their underlying types. | |
| `--colors`<br>`--no_colors` | Colors, when stderr is a terminal (and `NOCOLOR` is not set, `TERM` is set, `EDG_COLORS` is not empty). | On by default. |
| `--output_mode text`<br>`--output_mode sarif` | Diagnostics as text, or as SARIF (JSON). | Works: `--edg:output_mode=sarif`. |
| `--no_warnings`<br>`-w` | Errors only. | Set by ROSE. |
| `--remarks`<br>`-r` | Also remarks, milder than warnings. | No effect: ROSE's later `--no_warnings` wins. |
| `--promote_warnings`<br>`-W` | Warnings as discretionary errors. | No effect: the warnings are suppressed first. |
| `--lossy_conversion_warning`<br>`--no_lossy_conversion_warning` | Warn about conversions that can lose data. | Not shown. |
| `--for_init_diff_warning`<br>`--no_for_init_diff_warning` | Warn where the old scope of `for` declarations would make a difference (C++). | Not shown. |
| `--no_use_before_set_warnings`<br>`-j` | No warnings about variables used before they are set. | Not shown anyway. |
| `--report_gnu_extensions` | Warn about GNU extensions outside system headers. | Not shown. |
| `--check_unicode_security`<br>`--no_check_unicode_security` | Warn about Unicode text that can disguise code (UTF sources). | Not shown. |
| `--constexpr_diag_suppress <tags>`<br>`--constexpr_diag_remark <tags>`<br>`--constexpr_diag_warning <tags>`<br>`--constexpr_diag_error <tags>` | The severity of the diagnostics that calls of `__builtin_constexpr_diag` give with these tags. | |
| `--module_import_diagnostics`<br>`--no_module_import_diagnostics` | Diagnostics while importing modules. | |
| `--timing`<br>`-#` | The CPU and elapsed time of the phases. | Works; the "back end" time is edg2sage's translation into the AST. |
| `--top_templates <n>` | At the end, the *n* templates substituted most (0: all). | Works: `--edg:top_templates=10`. |

### Language features

The defaults follow the language mode (`-std=`, emulating GCC 13).  An option changes what the
front end accepts, but not how the compiler compiles ROSE's output (`rose_<file>`): keep both in
agreement (`-fno-exceptions --edg:no_exceptions`, for example).

| Option | Effect | Notes |
|---|---|---|
| `--alternative_tokens`<br>`--no_alternative_tokens` | Digraphs, and the operator keywords of C++ (`and`, `bitand`, ...). | On in C++. |
| `--array_new_and_delete`<br>`--no_array_new_and_delete` | `new[]` and `delete[]` (C++). | |
| `--auto_type`<br>`--no_auto_type` | `auto` as a deduced type (C++11). | |
| `--auto_storage`<br>`--no_auto_storage` | `auto` as a storage class (C++ before C++11). | |
| `--bit_precise_integers`<br>`--no_bit_precise_integers` | `_BitInt(N)` (C23). | Parsed (with `-std=c23`), but ROSE's AST has no such type: `_BitInt(12) b` is written as `int b`. |
| `--bool`<br>`--no_bool` | `bool` (C++). | |
| `--c23_typeof`<br>`--no_c23_typeof` | C23's `typeof` and `typeof_unqual` in any C mode. | |
| `--c++11_sfinae`<br>`--no_c++11_sfinae` | C++11 template deduction: expressions in deduction contexts (for `decltype` return types). | |
| `--c++11_sfinae_ignore_access`<br>`--no_c++11_sfinae_ignore_access` | Whether access errors make deduction fail. | |
| `--char8_t`<br>`--no_char8_t` | `char8_t` (C++20). | On in C++20. |
| `--compound_literals`<br>`--no_compound_literals` | Compound literals (C99). | |
| `--concepts`<br>`--no_concepts` | Concepts (C++20). | On in C++20. |
| `--delegating_constructors`<br>`--no_delegating_constructors` | Delegating constructors (C++11). | |
| `--deprecated_string_conv`<br>`--no_deprecated_string_conv` | String literals converted to `char *` (when they are `const`). | Allowed in GNU mode, as g++ allows it with a warning. |
| `--designators`<br>`--no_designators` | Designators (C99; C only). | |
| `--extended_designators`<br>`--no_extended_designators` | Designator extensions of other C compilers (C only). | |
| `--digit_separators`<br>`--no_digit_separators` | `1'000'000` (C++14). | |
| `--dollar`<br>`-$` | `$` in identifiers. | Accepted in GNU mode anyway. |
| `--exceptions`<br>`--no_exceptions`<br>`-x` | Exception handling (C++). | On, as in g++.  ROSE does not pass `-fno-exceptions` on: add `--edg:no_exceptions` (then `__EXCEPTIONS` is not defined either). |
| `--exc_spec_in_func_type`<br>`--no_exc_spec_in_func_type` | Exception specifications as part of function types (C++17). | |
| `--explicit`<br>`--no_explicit` | `explicit` (C++). | |
| `--export`<br>`--no_export` | Exported templates (C++98, removed in C++11). | |
| `--extern_inline`<br>`--no_extern_inline` | `inline` functions with external linkage (C++). | |
| `--fixed_point`<br>`--no_fixed_point` | Fixed-point types (Embedded C; C only). | See `--embedded_c`. |
| `--lambdas`<br>`--no_lambdas` | Lambdas (C++11). | |
| `--long_long` | `long long` in strict modes of dialects that do not have it. | |
| `--modules`<br>`--no_modules` | C++20 modules (experimental: only module files of Visual C++ 16.8 can be read). | ROSE does not support modules. |
| `--multibyte_chars`<br>`--no_multibyte_chars` | Multibyte characters (Shift-JIS, ...) in comments and literals. | Off by default. |
| `--named_address_spaces`<br>`--no_named_address_spaces` | Named address spaces (Embedded C; C only). | See `--embedded_c`. |
| `--named_registers`<br>`--no_named_registers` | Named-register storage classes (Embedded C; C only). | See `--embedded_c`. |
| `--namespaces`<br>`--no_namespaces` | Namespaces (C++). | |
| `--nonstd_using_decl`<br>`--no_nonstd_using_decl` | Using-declarations of unqualified names outside classes (C++). | |
| `--nullptr`<br>`--no_nullptr` | `nullptr` (C++11, C23). | |
| `--old_specializations`<br>`--no_old_specializations` | Specializations without `template<>` (C++). | |
| `--relaxed_abstract_checking`<br>`--no_relaxed_abstract_checking` | Check abstract parameter and return types only where functions are defined or called. | |
| `--restrict`<br>`--no_restrict` | The `restrict` keyword. | A keyword in C99 and later; `__restrict` in GNU mode.  ROSE adds `--restrict` for UPC. |
| `--rtti`<br>`--no_rtti` | `dynamic_cast` and `typeid` (C++). | On, as in g++.  ROSE does not pass `-fno-rtti` on: add `--edg:no_rtti`. |
| `--rvalue_refs`<br>`--no_rvalue_refs` | Rvalue references (C++11). | |
| `--rvalue_ctor_is_copy_ctor`<br>`--rvalue_ctor_is_not_copy_ctor` | Whether a move constructor counts as a copy constructor (so that none is generated). | |
| `--thread_local_storage`<br>`--no_thread_local_storage` | `thread_local` (and `__thread`). | `__thread` is on in GNU mode. |
| `--trigraphs`<br>`--no_trigraphs` | Trigraphs (`??=`). | |
| `--typename`<br>`--no_typename` | `typename` (C++). | |
| `--type_traits_helpers`<br>`--no_type_traits_helpers` | Intrinsics such as `__is_union` and `__has_virtual_destructor` (C++). | |
| `--uliterals`<br>`--no_uliterals` | `u"..."`, `U"..."`, `u'...'`, `U'...'` (and `char16_t`, `char32_t` in C++). | |
| `--unrestricted_unions`<br>`--no_unrestricted_unions` | Unions with members that have constructors or destructors (C++11). | |
| `--user_defined_literals`<br>`--no_user_defined_literals` | User-defined literals such as `12.5_km` (C++11). | |
| `--utf8_char_literals`<br>`--no_utf8_char_literals` | `u8'a'` (C++17). | |
| `--variadic_macros`<br>`--no_variadic_macros` | Variadic macros. | |
| `--extended_variadic_macros`<br>`--no_extended_variadic_macros` | Variadic macro extensions of other compilers. | |
| `--variadic_templates`<br>`--no_variadic_templates` | Variadic templates (C++11). | |
| `--vla`<br>`--no_vla` | Variable length arrays (C99; an extension elsewhere). | Accepted in GNU C++ too, as g++ does. |
| `--wchar_t_keyword`<br>`--no_wchar_t_keyword` | `wchar_t` as a keyword (C++). | |

### Alternative language behaviors

| Option | Effect | Notes |
|---|---|---|
| `--aligned_new`<br>`--no_aligned_new` | Allocation functions with an alignment for over-aligned types (C++17). | |
| `--arg_dep_lookup`<br>`--no_arg_dep_lookup` | Argument-dependent lookup (C++). | |
| `--base_assign_op_is_default`<br>`--no_base_assign_op_is_default` | Accept a copy assignment taking a base class as the default one (cfront). | |
| `--class_name_injection`<br>`--no_class_name_injection` | The name of a class is declared in its scope (C++). | |
| `--const_string_literals`<br>`--no_const_string_literals` | String literals are `const` (C++). | |
| `--default_calling_convention <cc>` | The calling convention of functions declared without one: `__cdecl`, `__fastcall`, `__stdcall`, `__thiscall`. | Microsoft mode. |
| `--default_common_tentative_definitions`<br>`--default_nocommon_tentative_definitions` | Tentative definitions in common storage or not (as gcc's `-fcommon` and `-fno-common`). | |
| `--defer_parse_function_templates`<br>`--no_defer_parse_function_templates` | Parse a function template only when it is first instantiated (as g++ 3.4 and later). | On in GNU mode: templates that are not instantiated are not checked.  The tools turn it off in their first run. |
| `--dep_name`<br>`--no_dep_name` | Two-phase name lookup in templates (C++).  `--dep_name` cannot be combined with `--no_parse_templates`. | On by default.  `--no_dep_name --no_parse_templates` give the lookup of Visual C++ without `/permissive-` (rose-using; the last run of rose-ren and rose-m2g). |
| `--distinct_template_signatures`<br>`--no_distinct_template_signatures` | Whether a function that is not a template can stand for an instance of a template. | |
| `--enum_overloading`<br>`--no_enum_overloading` | Operator functions for enum operands. | |
| `--far_data_pointers`<br>`--near_data_pointers`<br>`--far_code_pointers`<br>`--near_code_pointers` | The default size of pointers with `near` and `far` (Microsoft 16-bit mode). | |
| `--friend_injection`<br>`--no_friend_injection` | Names declared only in friend declarations are visible (old C++). | |
| `--func_prototype_tags`<br>`--no_func_prototype_tags` | The scope of tags first declared in a prototype (C; Visual C++ puts them outside). | |
| `--gcc89_inlining` | GNU C89 semantics of `inline` in C99 mode (C). | gcc's `-fgnu89-inline` is not passed on. |
| `--gen_move_operations`<br>`--no_gen_move_operations` | Implicit move constructors and move assignments (C++11). | |
| `--guiding_decls`<br>`--no_guiding_decls` | `void f(int);` after `template <class T> void f(T)` declares an instance of the template. | |
| `--ignore_std` | `std` is a synonym of the global namespace (old g++). | |
| `--implicit_extern_c_type_conversion`<br>`--no_implicit_extern_c_type_conversion` | Conversions between pointers to `extern "C"` and `extern "C++"` functions. | |
| `--implicit_noexcept`<br>`--no_implicit_noexcept` | Implicit exception specifications of destructors and deallocation functions (C++11). | |
| `--implicit_typename`<br>`--no_implicit_typename` | Decide from the context whether a dependent name is a type (before `typename`). | |
| `--late_tiebreaker`<br>`--early_tiebreaker` | When tie-breakers such as cv-qualification apply in overload resolution. | |
| `--long_lifetime_temps`<br>`--short_lifetime_temps` | Temporaries live to the end of the scope (cfront) or of the full expression. | |
| `--long_preserving_rules`<br>`--no_long_preserving_rules` | The arithmetic conversions of K&R for `long`. | |
| `--max_depth_constexpr_call=<n>`<br>`--max_cost_constexpr_call=<n>` | Limits for evaluating `constexpr` calls (defaults 512 and 2,000,000). | A recursion depth of 600 fails by default; `--edg:max_depth_constexpr_call=1024` accepts it (as gcc's `-fconstexpr-depth=1024`). |
| `--ms_rvalue_cast`<br>`--no_ms_rvalue_cast` | Whether a cast of an lvalue to its own type is an rvalue; implies Microsoft mode. | |
| `--ms_strict_ternary`<br>`--no_ms_strict_ternary` | The standard `?:` in Microsoft mode; implies Microsoft mode. | |
| `--nonconst_ref_anachronism`<br>`--no_nonconst_ref_anachronism` | Non-const references bound to class rvalues (anachronism). | |
| `--nonstd_anonymous_unions`<br>`--no_nonstd_anonymous_unions` | Extensions of anonymous unions, and anonymous structs. | On in GNU mode. |
| `--nonstd_default_arg_deduction`<br>`--no_nonstd_default_arg_deduction` | Default arguments kept in deduced function types. | |
| `--nonstd_instantiation_lookup`<br>`--no_nonstd_instantiation_lookup` | A lookup during instantiation from before the C++ standard. | |
| `--nonstd_qualifier_deduction`<br>`--no_nonstd_qualifier_deduction` | Deduce template arguments in qualifiers (`A<T>::B`, `T::B`). | |
| `--old_id_chars`<br>`--no_old_id_chars` | The identifier characters of C++ before C++23, or of Unicode (the default). | |
| `--old_for_init`<br>`--new_for_init` | The scope of declarations in `for` initializers: cfront's, or the standard one. | |
| `--pack_alignment <n>` | The default maximum alignment of members, as `#pragma pack(<n>)`. | Changes the layout that the front end computes; gcc's `-fpack-struct=<n>` is not passed on. |
| `--parse_templates`<br>`--no_parse_templates` | Parse templates that are not classes in their generic form. | On with two-phase lookup; see `--dep_name`. |
| `--preserve_lvalues_with_same_type_casts`<br>`--no_preserve_lvalues_with_same_type_casts` | A cast of an lvalue to its own type is an lvalue (Microsoft, Sun). | |
| `--require_func_prototypes`<br>`--no_require_func_prototypes` | `int f();` means `int f(void);` (C; the default in C23). | |
| `--short_enums`<br>`--no_short_enums` | Enums get the smallest type that holds their values (GNU C). | gcc's `-fshort-enums` is not passed on: add `--edg:short_enums`. |
| `--signed_bit_fields`<br>`--unsigned_bit_fields` | The signedness of plain `int` bit-fields. | |
| `--signed_chars`<br>`-s`<br>`--unsigned_chars`<br>`-u` | The signedness of `char`. | Signed by default, as for gcc on x86-64; gcc's `-funsigned-char` is not passed on: add `--edg:unsigned_chars`. |
| `--special_subscript_cost`<br>`--no_special_subscript_cost` | A nonstandard overload cost for `[]` (cfront 3.0). | |
| `--no_stdarg_builtin` | `<stdarg.h>` is not treated as built in (for EDG's generated code). | |
| `--using_std`<br>`--no_using_std` | Implicit use of `std` with EDG's own library headers. | Not useful here. |

### Template instantiation

| Option | Effect | Notes |
|---|---|---|
| `--auto_instantiation`<br>`-T`<br>`--no_auto_instantiation` | Automatic instantiation with EDG's prelinker (C++). | ROSE passes `--auto_instantiation` in some cases; edg2sage removes it (there is no prelinker). |
| `--instantiate <mode>`<br>`-t<mode>` | Which external template entities to instantiate in this translation unit: `none` (default), `used`, `all`, `local`. | ROSE passes `-tused`, `-tlocal` or `-tall` in some cases; edg2sage removes them.  The tools pass `--instantiate used`.  `--edg:instantiate=used` with identityTranslator instantiates the bodies of the templates that are used (more work, the same output: ROSE does not write template instances). |
| `--pending_instantiations=<n>` | The maximum number of instantiations of a template in progress at a time (default 64; 0: no limit). | Recursive templates deeper than 64 fail ("excessive recursion"), which g++ (limit 900) accepts: `--edg:pending_instantiations=1000`. |
| `--implicit_include`<br>`-B`<br>`--no_implicit_include` | Find template definitions by including source files implicitly (cfront). | Off by default. |
| `--definition_list_file <file>`<br>`--ii_file <file>`<br>`--template_info_file <file>`<br>`--exported_template_file <file>`<br>`--suppress_instantiation_flags` | Files and flags of EDG's driver and prelinker. | Not used. |
| `--template_directory <dir>` | A folder to search for exported templates. | |

### Code generation

| Option | Effect | Notes |
|---|---|---|
| `--no_code_gen`<br>`-n` | Check the syntax only: the back end is not called. | The back end is edg2sage: ROSE reports a front-end failure. |
| `--force_vtbl`<br>`--suppress_vtbl`<br>`-V` | Define the virtual function tables of classes without a key function (or not). | For EDG's code generation; no use here. |
| `--keep_restrict_in_signatures`<br>`--no_keep_restrict_in_signatures` | Keep top-level `restrict` qualifiers of parameters in function types and mangled names. | |
| `--no_remove_unneeded_entities` | Keep the IL entities that are not needed. | No effect: they are kept in this build. |
| `--target <name>` | The target configuration to use. | `win64` (in `edg-base/lib_win64`), used by `--msvc`.  With identityTranslator it would make `long` 32 bits wide while GCC's headers for Linux are parsed. |

### Precompiled headers

| Option | Effect | Notes |
|---|---|---|
| `--pch` | Create and use precompiled headers automatically (in `--pch_dir`). | Not useful: the file is written (57 MB for `<vector>` and `<string>`), but never used.  The front end rejects it because its memory layout differs from run to run in ROSE ("memory usage conflict with precompiled header file", tag `memory_mismatch`, a warning that is not shown). |
| `--create_pch <file>`<br>`--use_pch <file>` | Create, or use, a precompiled header file. | As `--pch`. |
| `--pch_dir <dir>` | The folder of precompiled header files. | |
| `--pch_messages`<br>`--no_pch_messages` | A message when a precompiled header file is created or used. | |
| `--pch_verbose`<br>`--no_pch_verbose` | Why a precompiled header file cannot be used (automatic mode). | Not shown: the reasons are warnings. |

### C++/CLI

| Option | Effect | Notes |
|---|---|---|
| `--mscorlib_file_name <file>`<br>`--preusing <file>`<br>`--using_directory <dir>`<br>`--using_framework_directory`<br>`--no_using_framework_directory`<br>`--vcmeta_directory <dir>` | The .NET assemblies of C++/CLI. | Not usable (see `--c++cli`). |

### Miscellaneous

| Option | Effect | Notes |
|---|---|---|
| `--version`<br>`-v` | The version of the front end. | Prints "Edison Design Group C/C++ Front End, version 7.0" on stderr; the translation goes on.  (`identityTranslator --version` reports EDG 6.5.) |
| `--edg_base_dir <dir>` | The folder of the front end's run-time files. | Set by edg2sage (as `--edg_base`); use the environment variable `ROSE_EDG_BASE` instead. |
| `--il_display` | Print the IL, the front end's intermediate language, in readable form. | Works: to stdout (34,000 lines for a file that includes `<stdio.h>`); for debugging edg2sage. |
| `--dump_command_options` | Print all `--name` options, for shell completion. | Works (stdout). |
| `--dump_configuration` | Print the configuration macros of the front end, as `defines.h`. | Works in this build (stderr), although EDG documents it for debug builds only. |
| `--dump_legacy_as_target <name>` | Print the configuration as a target configuration. | |
| `--incognito`<br>`--no_incognito` | Do not define the front end's own macros (`__EDG__`, `__EDG_VERSION__`). | Works; for code that checks for EDG. |
| `--building_runtime` | Compile EDG's run-time library. | Not useful. |
| `--wdir <dir>` | The folder against which relative paths are expanded. | |

### Options that EDG does not document

| Option | Effect | Notes |
|---|---|---|
| `--set_flag <name>`<br>`--clear_flag <name>` | Set or clear an internal flag of the front end: `suppress_inline_corresp_check`, `allow_anon_types_in_anon_unions`, `emulate_gnu_abi_bugs`, `emulate_unsafe_gnu_abi_bugs`, `warn_about_tail_padding_use`, `reuse_tail_padding`, `packing_applies_to_base_classes`, `stack_referenced_include_directories`, `use_nonstd_partial_ordering`, `no_checking_pragmas`, `no_find_pragma_validation`, `warn_on_try_statement`, `disable_access_checking_in_microsoft_enum_bases`, `pending_generic_constraint_specifier_enabled`, `generic_arity_overload_allowed`, `no_ms_nonreal_base_classes`, `force_ms_type_info_not_in_namespace_std`, `terse_range_based_for_enabled`, `relaxed_constexpr`, `constexpr_implies_const`, `mangle_had_been_implicitly_const`, `coroutines`, `suppress_deferral_on_partial_spec_members`, `no_very_expensive_checking`, `diag_override_does_not_affect_sfinae`, `preload_builtin_functions`, `gen_edg_special_types`, `reflection`, `injection`, `alias_templ_intrinsics`, `var_templ_intrinsics`, `templ_type_member_intrinsics`, `lazy_field_initializers`, `core_constant_expr_is_noexcept`, `null_template_ptr_arg_enabled`, `skip_module_imports`, `skip_module_version_check`, `ignore_absolute_paths_for_header_units`, `extended_float_types`, `use_predefined_macro_file`. | For experiments: `--edg:set_flag=relaxed_constexpr`. |
| `--time_limit <seconds>` | A limit on CPU time (`setrlimit`). | Applies to the whole process, ROSE included, which the system stops at the limit. |

### Documented options that are not in this build

These are in EDG's manual, but not in this configuration of the front end (the front end
rejects them as invalid options):

* `--db`, `--db_name`, `-d` (debugging output: debug builds only)
* `--gen_c_file_name`, `--gen_c_clang_version`, `--gen_c_gnu_version`, `--gen_c_msvc_version`,
  `--module_init`, `-i`, `--old_line_commands`, `--inlining`, `--no_inlining`,
  `--inline_statement_limit` (EDG's C- and C++-generating back ends)
* `--no_il_lowering`, `-N` (lowering of the IL to C)
* `--remove_unneeded_entities`
* `--instantiation_dir`, `--one_instantiation_per_object`
* `--macro_positions_in_diagnostics`, `--no_macro_positions_in_diagnostics`
* `--mmap_address`, `--pch_mem`
* `--sun`, `--no_sun`, `--sun_linker_scope`, `--no_sun_linker_scope` (Sun CC mode)

## Environment variables

| Variable | Read by | Effect |
|---|---|---|
| `ROSE_EDG_BASE` | edg2sage | The folder of the front end's run-time configuration (default: `edg-base` of the installation, or of the build folder). |
| `EDG2SAGE_EDG_OPTIONS` | edg2sage | More front-end options, separated by spaces, before ROSE's. |
| `EDG2SAGE_DEBUG` | edg2sage | More diagnostics of the translation into ROSE's AST. |
| `USR_INCLUDE` | the front end | The folder searched instead of `/usr/include`, after all others. |
| `NOCOLOR`, `TERM`, `EDG_COLORS` | the front end | Colors of diagnostics: off if `NOCOLOR` is set, `TERM` is not set or `EDG_COLORS` is empty; `EDG_COLORS` also sets the colors (default `error=01;31:warning=01;35:note=01;36:locus=01:quote=01:range1=32`). |
| `EDG_MODULES_PATH` | the front end | Folders of module files. |
