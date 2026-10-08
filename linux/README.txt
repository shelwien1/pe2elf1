ROSE 2.18.0 for Linux (x86-64)
==============================

ROSE (https://github.com/llnl/rose) is a source-to-source compiler
infrastructure.  This build supports C and C++, parsed by the open-source EDG
front end (https://github.com/edgcpp/compiler).  It was built with GCC 13 on
Ubuntu 24.04; see https://github.com/shelwien1/pe2elf1 for the sources and how
it is built.

Contents
--------

  lib/librose.so              the ROSE library, with the EDG front end; the
                              programs below use it
  bin/identityTranslator      parses C/C++ files into ROSE's AST, writes the AST
                              back as source code (rose_<file> in the current
                              folder), and compiles that with the backend
                              compiler; it takes the same command line options
                              as gcc/g++
  bin/dotGenerator            writes the AST of a file as a GraphViz graph
                              (<file>.dot in the current folder)
  bin/rose-ren                lists the C++ declarations of a name, or renames
                              one of them
  bin/rose-using              adds using-declarations for the names of
                              dependent base classes that Visual C++ accepts
                              and GCC and Clang do not
  bin/rose-m2g                turns a member function into a global function
                              with an explicit This parameter
  test.sh                     a self test (see below)
  examples/                   the programs test.sh uses
  include/rose/, rose.mk, examples/tools/
                              the SDK, for programs that use ROSE (see below)
  edg-base/                   configuration of the EDG front end
  include/edg/                the system headers the front end parses with:
                              those of GCC 13 (libstdc++ and the GCC headers),
                              of the GNU C library 2.39 and of Linux
  licenses/                   licenses of ROSE, EDG and the bundled components

The folder can be moved anywhere: librose.so finds edg-base and include
relative to its own location (the environment variable ROSE_EDG_BASE overrides
the edg-base folder), and the programs find librose.so in ../lib.  The C++
run-time library and Boost are linked into librose.so and the C++ run-time
library into each program, so they only use the C library and libgcc_s of the
system.

Requirements
------------

64-bit x86 Linux with the GNU C library 2.34 or later: Ubuntu 22.04, Debian 12,
Red Hat Enterprise Linux 9 or later, for example.  (Tested on Ubuntu 24.04 and
Debian 12.)  Code is parsed the way GCC 13 parses it, with the headers in
include/edg; the headers of other libraries (Boost, zlib, ...) are found in
/usr/include, after those.

To compile its output, identityTranslator runs gcc (C files) or g++ (C++
files) from the PATH, as gcc itself would run its compiler passes: any recent
GCC, for example "apt install g++".  Without a compiler, use
-rose:skipfinalCompileStep: identityTranslator then only writes rose_<file>.

Testing the installation
------------------------

Run ./test.sh.  It translates examples/hello.c (C using the POSIX API) and
examples/shapes.cpp (C++17 using the standard library) with identityTranslator,
writes the AST of hello.c with dotGenerator, and changes a copy of
examples/refactor.cpp with rose-ren, rose-m2g and rose-using.  If gcc and g++
are in the PATH, it also builds hello.c and shapes.cpp directly and through
identityTranslator and checks that they print the same, and checks that the
changed refactor.cpp (which g++ does not compile before the changes) compiles
and prints what it should.  The files are written to a new folder in /tmp
($TMPDIR), shown at the start; the exit status is 0 if all tests pass.

Examples
--------

  identityTranslator -c test.c                 writes rose_test.c, compiles it to test.o
  identityTranslator -std=c++17 prog.cpp -o prog
                                               writes rose_prog.cpp, builds prog
  identityTranslator -rose:skipfinalCompileStep -I include src/file.cpp
                                               only writes rose_file.cpp
  dotGenerator test.c                          writes test.c.dot
  identityTranslator --help                    all options (ROSE's own are -rose:...)

The usual compiler options (-I, -D, -std=..., -c, -o, ...) are understood and
passed on to the backend compiler.  Diagnostics go to standard error;
-rose:verbose 1 shows the commands ROSE runs.

The refactoring tools
---------------------

The tools change source files in place: the file given and the headers it
includes, except system headers.  They take the compiler's options (-I, -D,
-std=...) before the file name, and --dry-run, which shows the changes
without making them.  They find names from the cross-references of the EDG
front end, so a name is found in all its uses: qualified names,
using-declarations, base classes, template arguments, and the bodies of
templates (also those that are not instantiated).  Uses that the front end
does not resolve (in code that the preprocessor skips, or members of template
parameters) are listed, and not changed.

Code that only Visual C++ compiles (see rose-using) is parsed as Visual C++
parses it, with a note; template code that is not instantiated is then not
seen by the front end.

  rose-ren file.cpp name
      lists the declarations called name, with an index: the file and line
      of the declaration, the full name, the type, the kind of declaration
      and the number of references (other than declarations), for example
        [0] main.cpp:9  Square::area  double () const  (virtual member function), 0 references
        [1] shapes.h:8  geo::Shape::area  double () const  (pure virtual member function), 2 references
        [2] shapes.h:16  geo::Circle::area  double () const  (virtual member function), 0 references

  rose-ren file.cpp name[:index] newname
      renames one of them (name alone if there is only one): its declarations
      and all its uses.  A class is renamed with its constructors and
      destructor, a virtual function with the functions that override it or
      that it overrides.  Nothing is changed if the new name is already
      declared in the same scope, or if the name is declared in a system
      header or written in a macro definition (--force renames anyway).

  rose-using file.cpp [class]
      Visual C++ (without /permissive-) looks up the names used in a class
      template also in its dependent base classes, GCC and Clang do not:
        template <class T> struct Stack : Container<T> {
          void push(const T& t) { items.push_back(t); }  // Container<T>::items
        };
      rose-using adds using-declarations for such names to the class
      templates ("using Container<T>::items;", "using typename ..." for
      types), with the access the members have in the base class, so that the
      code means the same for GCC and Clang.  Only class templates that are
      instantiated in the file can be checked.  With a class name, only that
      class template is changed; otherwise all those outside system headers.

  rose-m2g file.cpp Class::method
      turns the member function into a global function with an explicit
      This parameter:
        struct A {                     struct A {
          int v;                         int v;
          int get(int k) const {         friend int get(const A* This, int k);
            return v + k;              };
          }                            inline int get(const A* This, int k) {
        };                               return This->v + k;
        ... a.get(1) ... p->get(2)     }
                                       ... get(&a, 1) ... get(p, 2)
      In the function, "this" becomes This and members are accessed through
      This; the declaration in the class becomes a friend declaration (the
      function keeps its access to private members).  Calls become calls of
      the global function.  Virtual and static member functions, operators,
      member templates, members of class templates and functions whose
      address is taken are not converted.

Parsing with the headers of Visual C++
--------------------------------------

With --msvc, the refactoring tools parse code as Visual C++ does: in the front
end's Microsoft mode (the language, extensions and predefined macros of Visual
C++, such as _MSC_VER, _WIN64 and _MSVC_LANG, and its data model and class
layout for x64), with the headers of Visual C++ instead of those of GCC:

  rose-ren --msvc file.cpp name       the headers of the folders in the INCLUDE
                                      environment variable (separated by ;)
  rose-ren --msvc=/opt/VC2019 file.cpp name
                                      the headers of a portable Visual C++ in
                                      /opt/VC2019: its include, ucrt/include and
                                      sdk/include folders (and atlmfc/include)

The same options work with rose-using and rose-m2g.  The version of Visual C++
(_MSC_VER and _MSC_FULL_VER) is taken from its headers (crtversion.h);
--msvc-version=<_MSC_VER> sets another.  -std=c++17 and -std=c++20 select
/std:c++17 and /std:c++20 (/std:c++latest before Visual C++ 19.29); the default
is /std:c++14, as for cl.exe.  _MT and _CPPUNWIND are defined, as cl.exe
defines them with /EHsc; for the macros of other cl.exe options, use -D
(-D_DLL for /MD, for example).

The tools first parse the code with two-phase name lookup (/permissive-), so
that the bodies of all templates are parsed; code that cl.exe compiles only
without /permissive- (the names of dependent base classes that rose-using is
for) is then parsed as cl.exe parses it by default, with a note: the bodies
of templates are then only parsed where they are instantiated.

On Linux, the headers of the Visual C++ library and of the C run-time library
(include, ucrt/include) can be used, but not those of the Windows SDK
(<windows.h>): they include each other with names whose case does not match
the files, which only works on a file system that ignores case.

The SDK
-------

The package is also an SDK for programs that use ROSE (like the programs
above), built with g++:

  lib/librose.so        the library
  include/rose/         the headers (rose.h, RoseRefactor.h for refactoring
                        tools, and the parts of Boost that they include)
  include/rose/rose.rsp the compiler options, with the include folders
                        relative to include/rose (gcc -iprefix)
  rose.mk               the compiler and linker options, for Makefiles
  examples/tools/       the sources of the programs in bin/, with a Makefile

It needs g++ with C++14 support (tested with GCC 12 and 13; the C++ run-time
library in librose.so is its own), and the path of the package must not
contain spaces.  To build the examples:

  cd examples/tools
  make -j4

In another Makefile:

  include /path/to/rose-2.18.0-linux64/rose.mk
  tool: tool.o
  	$(CXX) $(ROSE_LDFLAGS) -o $@ $< $(ROSE_LIBS)
  tool.o: tool.C
  	$(CXX) -O2 $(ROSE_CXXFLAGS) -c $< -o $@

The programs find lib/librose.so through their run path (the absolute path of
lib/, set by ROSE_LDFLAGS).  RoseRefactor.h describes the cross-references and
the source editing that the refactoring tools use.

Notes
-----

* identityTranslator --version reports the EDG version as 6.5; it is EDG 7.0.
