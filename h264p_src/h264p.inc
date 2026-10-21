// h264p.inc -- h264p but its command line (h264p.cpp): a lossless
// compressor of H.264 Annex B streams (c, d). The syntax of the stream, with
// the entropy coding removed, is coded with a context mixing model
// (model.inc) and coded back by the decompressor; both run as coroutines,
// streaming (coro.inc), and on threads, the stream in parts (mt.inc).
//
// One translation unit: a program includes this file once, as h264p.cpp
// does, and it includes every source. Its names stay in namespace h264p:
// Lib3's global byte, uint and qword, and whatever more of the library a
// program has after it, MTwrap's included, see only theirs.
//
// It takes nothing of the C++ library, and links without it (the C
// library's headers only): no exceptions or RTTI (err.inc), no containers,
// strings or algorithms (mem.inc), no statics in functions (their guards
// are the library's: what it makes once is made as the program starts);
// and its structs have no constructors or destructors -- Init() sets one
// up, Quit() frees what it holds -- so that they can live on pages that
// are mapped, not constructed, as its coroutines do (coro.inc). (The
// channels' vtables aside, chan.inc.)
#ifndef H264P_INC
#define H264P_INC

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#define PSAPI_VERSION 2  // (GetProcessMemoryInfo from kernel32)
#include <psapi.h>
#else
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#endif

// The stack-copying coroutines of Lib3 (unchanged from mtwrap's), which
// h264p's compressor and decompressor are (coro.inc), and the library's
// CoroutinePair, which chains two of them.
// Before h264p's own sources, which see its INLINE and NOINLINE, and after
// the system headers, which its setjmp and longjmp macros must not reach.
#ifndef FILE_API_STD
#define FILE_API_STD  // (its stdio file API on Windows too, as h264p had)
#endif
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro2pair.inc"
// MTwrap (mtwrap's, but that it waits only on events: a failure to make
// one fails Init() instead of falling back to Sleep() polling; Lib3's
// win_shim.inc it includes is unchanged: build with -I Lib3, and with
// threads, -pthread), which runs coroutines on threads (mt.inc)
#include "MTwrap_v3/MTwrap.inc"

// errors (without C++ exceptions) and memory (without the C++ library's
// containers), then the syntax: the bitstream, its walk and the channels
// it exchanges the values with (the value log, the model)
#include "err.inc"
#include "mem.inc"
#include "bits.inc"
#include "tables.inc"
#include "cabac.inc"
#include "cavlc.inc"
#include "chan.inc"
#include "syntax.inc"
#include "slice.inc"
#include "log.inc"
// the model: the coder, the families and the decoder in the loop
#include "cmbase.inc"
#include "rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "mapping.inc"
#include "fam.inc"
#include "famvars.inc"
#include "pixel.inc"
#include "recon.inc"
#include "model.inc"

// the front end: the byte stream, the splitter of -t, a call's options,
// progress and files, the per-NAL step, the compressed stream, the
// coroutines and the calls
#include "annexb.inc"
#include "h264split.inc"
#include "cli.inc"
#include "nal.inc"
#include "cmstream.inc"
#include "coro.inc"
#include "mt.inc"
#include "drivers.inc"

#endif  // H264P_INC
