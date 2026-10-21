// h264p.inc -- h264p but its command line (h264p.cpp): a lossless
// compressor of H.264 Annex B streams (c, d). The syntax of the stream, with
// the entropy coding removed, is coded with a context mixing model
// (model.inc) and coded back by the decompressor; both run as coroutines,
// streaming (coro.inc), and on threads, the stream in parts (mt.inc). The
// same syntax can be dumped to a TSV file (t) and the identical stream
// restored from it (r: tsvfile.inc).
//
// One translation unit: a program includes this file once, as h264p.cpp
// does, and it includes every source. Its names stay in namespace h264p:
// Lib3's global byte, uint and qword, and whatever more of the library a
// program has after it, MTwrap's included, see only theirs.
#ifndef H264P_INC
#define H264P_INC

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <sys/stat.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
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
// MTwrap (unchanged from mtwrap's, as is Lib3's win_shim.inc it includes:
// build with -I Lib3, and with threads, -pthread), which runs coroutines on
// threads (mt.inc)
#include "MTwrap_v3/MTwrap.inc"

// the syntax: the bitstream, its walk and the channels it exchanges the
// values with (TSV rows, the value log, the model)
#include "bits.inc"
#include "rows.inc"
#include "tables.inc"
#include "cabac.inc"
#include "cavlc.inc"
#include "chan.inc"
#include "syntax.inc"
#include "slice.inc"
#include "tsv.inc"
#include "log.inc"
// the model: the coder, the families and the decoder in the loop
#include "cmbase.inc"
#include "rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "mapping.inc"
#include "fam.inc"
#include "famvars.inc"
#include "recon.inc"
#include "model.inc"

// the front end: the byte stream, the splitter of -t, a call's options,
// progress and files, the per-NAL step, the TSV and the compressed streams,
// the coroutines and the calls
#include "annexb.inc"
#include "h264split.inc"
#include "cli.inc"
#include "nal.inc"
#include "tsvfile.inc"
#include "cmstream.inc"
#include "coro.inc"
#include "mt.inc"
#include "drivers.inc"

#endif  // H264P_INC
