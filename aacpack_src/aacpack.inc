// aacpack.inc -- aacpack but its command line (aacpack.cpp): a lossless
// compressor of AAC streams (c, d), built on h264p's: the syntax of the
// stream, with the Huffman coding removed (the syntax walk of aac2tsv), is
// coded with a context mixing model (model.inc) and coded back by the
// decompressor; both run as coroutines, streaming (coro.inc).
//
// One translation unit: a program includes this file once, as aacpack.cpp
// does, and it includes every source. Its names stay in namespace aacpack:
// Lib3's global byte, uint and qword, and whatever more of the library a
// program has after it, see only theirs.
//
// It takes nothing of the C++ library, and links without it (the C
// library's headers only): no exceptions or RTTI (err.inc), no containers,
// strings or algorithms (mem.inc), no statics in functions that need
// guards; and its structs have no constructors or destructors -- Init()
// sets one up, Quit() frees what it holds -- so that they can live on pages
// that are mapped, not constructed, as its coroutines do (coro.inc). (The
// channels' vtables aside, chan.inc.)
#ifndef AACPACK_INC
#define AACPACK_INC

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#define PSAPI_VERSION 2  // (GetProcessMemoryInfo from kernel32)
#include <psapi.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#endif

// The stack-copying coroutines of Lib3 (unchanged from mtwrap's, as h264p
// has them), which aacpack's compressor and decompressor are (coro.inc).
// Before aacpack's own sources, which see its INLINE and NOINLINE, and
// after the system headers, which its setjmp and longjmp macros must not
// reach.
#ifndef FILE_API_STD
#define FILE_API_STD  // (its stdio file API on Windows too)
#endif
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"

// errors (without C++ exceptions) and memory (without the C++ library's
// containers), the bits, the tables and codebooks of the syntax, the
// syntax walk and the channels it exchanges the values with (the value
// log, the model), the containers
#include "err.inc"
#include "mem.inc"
#include "cmbase.inc"
#include "bits.inc"
#include "tables.inc"
#include "huff.inc"
#include "sbrfreq.inc"
#include "elems.inc"
#include "chan.inc"
#include "log.inc"
#include "asc.inc"
#include "syntax.inc"
#include "units.inc"
// the model: the coder and the families
#include "rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "mapping.inc"
#include "fam.inc"
#include "famvars.inc"
#include "model.inc"

// the front end: a call's options, progress and files, the compressed
// stream, the coroutines and the calls
#include "cli.inc"
#include "cmstream.inc"
#include "coro.inc"
#include "drivers.inc"

#endif  // AACPACK_INC
