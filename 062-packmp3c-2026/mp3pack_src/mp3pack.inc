// mp3pack.inc -- mp3pack but its command line (mp3pack.cpp): a lossless
// compressor of MP3 files (c, d), with aacpack's front end around mp3c.
// mp3rw's parser takes the frames out of the file (split.inc) and mp3c's
// model codes them (mp3rw/mp3mod.inc); what is not frames goes to a second
// file, the outside data, as mp3rw writes it. d puts the file back from
// the two (join.inc). Both run as coroutines (Lib3's), streaming, and c on
// threads, the frames in parts, through MTwrap (mt.inc).
//
// One translation unit: mp3pack.cpp includes this once. mp3c's sources
// (Lib3, mp3rw/) stay global, as in mp3c and mp3rw; mp3pack's own are in
// namespace mp3pack.
#ifndef MP3PACK_INC
#define MP3PACK_INC

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
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

#ifndef __max
#define __max(a, b) (((a) > (b)) ? (a) : (b))
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef INC_LOG2I
#define INC_LOG2I  // (common.inc's log2i(), which mp3c's model takes)
#endif

// Lib3: the stack-copying coroutines, CoroutinePair (two of them
// chained), and MTwrap (aacpack's, h264p's: mtwrap's, but that it waits
// only on events), which runs coroutines on threads (Lib3's win_shim.inc
// is its Windows API on POSIX: build with -ILib3, and -pthread). After
// the system headers, which Lib3's setjmp and longjmp must not reach.
#include "common.inc"
#include "coro3b.inc"
#include "coro2pair.inc"
namespace mp3pack {
[[noreturn]] void out_of_memory();
}
#define MTWRAP_NOMEM() mp3pack::out_of_memory()
#include "MTwrap_v3/MTwrap.inc"

// mp3c's parser and model, as mp3rw.cpp and mp3c.cpp build them (packed
// frames), but that the records of the dump between them are not padded
// (frame_dump_io.inc): mp3pack cuts the dump into parts, and holds it only
// in memory
#define FRAME_DUMP_ALIGN 0
#pragma pack(push, 1)
#include "mp3hdr.inc"
#include "mp3_parse/parse__mp3.inc"
#include "mp3mod.inc"
#pragma pack(pop)

// the front end: memory, CRC-32; a call's options, progress and files; the
// splitter of c, the joiner of d; the workers of -t; the calls
#include "base.inc"
#include "cli.inc"
#include "split.inc"
#include "join.inc"
#include "mt.inc"
#include "drivers.inc"

#endif  // MP3PACK_INC
