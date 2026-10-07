ROSE 2.18.0 for Windows (x86-64)
================================

ROSE (https://github.com/llnl/rose) is a source-to-source compiler
infrastructure.  This build supports C and C++, parsed by the open-source EDG
front end (https://github.com/edgcpp/compiler).  It was cross-compiled with
MinGW-w64 GCC; see https://github.com/shelwien1/pe2elf1 for the sources and
how it is built.

Contents
--------

  bin\identityTranslator.exe  parses C/C++ files into ROSE's AST, writes the AST
                              back as source code (rose_<file> in the current
                              folder), and compiles that with the backend
                              compiler; it takes the same command line options
                              as gcc/g++
  bin\dotGenerator.exe        writes the AST of a file as a GraphViz graph
                              (<file>.dot in the current folder)
  test.bat                    a self test (see below)
  examples\                   the programs test.bat translates
  edg-base\                   configuration of the EDG front end
  include\edg\                the system headers the front end parses with:
                              those of MinGW-w64 GCC 13 (libstdc++, the GCC
                              headers, and the MinGW-w64 C library and Windows
                              API headers)
  licenses\                   licenses of ROSE, EDG and the bundled components

The folder can be moved anywhere: the programs find edg-base and include
relative to their own location (the environment variable ROSE_EDG_BASE
overrides the edg-base folder).  The programs are self-contained (no DLLs).

Requirements
------------

64-bit Windows.  Code is parsed the way MinGW-w64 GCC parses it: GNU C/C++ with
the Windows data model (32-bit long, 16-bit wchar_t) and GCC's bit-field and
structure layout for Windows.

To compile its output, identityTranslator runs gcc (C files) or g++ (C++
files) from the PATH, as gcc itself would run its compiler passes.  Install a
MinGW-w64 GCC, for example from MSYS2 (https://www.msys2.org):

  pacman -S mingw-w64-x86_64-gcc      (then add C:\msys64\mingw64\bin to PATH)

Without a compiler, use -rose:skipfinalCompileStep: identityTranslator then
only writes rose_<file>.

Testing the installation
------------------------

Run test.bat (or double-click it).  It translates examples\hello.c (C using the
Windows API) and examples\shapes.cpp (C++17 using the standard library) with
identityTranslator, writes the AST of hello.c with dotGenerator, and, if gcc
and g++ are in the PATH, builds both programs directly and through
identityTranslator and checks that they print the same.  The files are written
to a new folder in %TEMP%, shown at the start; the exit status is 0 if all
tests pass.

Examples
--------

  identityTranslator -c test.c                 writes rose_test.c, compiles it to test.o
  identityTranslator -std=c++17 prog.cpp -o prog.exe
                                               writes rose_prog.cpp, builds prog.exe
  identityTranslator -rose:skipfinalCompileStep -I include src\file.cpp
                                               only writes rose_file.cpp
  dotGenerator test.c                          writes test.c.dot
  identityTranslator --help                    all options (ROSE's own are -rose:...)

The usual compiler options (-I, -D, -std=..., -c, -o, ...) are understood and
passed on to the backend compiler.  Diagnostics go to standard error;
-rose:verbose 1 shows the commands ROSE runs.  The files ROSE writes have
Windows (CR LF) line endings.

Notes
-----

* identityTranslator --version reports the EDG version as 6.5; it is EDG 7.0.
* __attribute__((gcc_struct)) is not taken into account for structure layout
  (sizeof); the default (Microsoft-compatible) bit-field layout of MinGW-w64
  GCC is.
