// tangelo_w - the Tangelo model, coded a byte at a time.
//
// Same machinery as fpaq0mw.cpp (see ALGORITHM.md): the compiled model step is
// run speculatively down the prefix tree with every memory write journaled, so
// that the code length of all 256 possible next bytes can be read off and the
// byte coded in a single multi-symbol range coder step. Only the model differs:
// fpaq0mw's two counters and a mixer are replaced by Tangelo's 22-context
// ContextMap, match model, two-layer mixer and APM chain.
//
// Built twice by build.sh / build.bat:
//   -DSIM_FUNC -S   -> the model step alone, as assembly, for track.pl to journal
//   (no flags)      -> the program, linked against that instrumented assembly

enum{ PSCALE=1<<16 };

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#define assert(x)

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline))
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// __max/__min are MSVC/MinGW CRT macros; provide them elsewhere (glibc etc.)
#ifndef __max
 #define __max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef __min
 #define __min(a,b) (((a)<(b))?(a):(b))
#endif

// How deep the speculative walk goes before it gives up on a prefix (track.inc).
// Tangelo's predictions are far sharper than fpaq0mw's, so the default 0x90000
// (3 bits per bit) cuts into the distribution that matters; 0x80000 (4 bits per
// bit) is within five bytes of the unpruned walk. Measured on book1's first
// 64 KB - unpruned 20734 at 14.3s, 0x80000 20739 at 10.2s, 0x90000 20804 at
// 7.5s, 0xA0000 21307 at 5.2s, 0xB0000 25160 at 3.7s.
#ifndef PRUNE_LOG            // ... but -DPRUNE_LOG on the command line wins
 #define PRUNE_LOG 0x80000
#endif

#include "tangelo/tangelo.inc"

#include "sh_v1m.inc"

#include "coder_tangelo.inc"

#ifndef SIM_FUNC

// The one definition of every mutable global the model shares. The -DSIM_FUNC
// translation unit only declares them (tangelo/common.inc), so the instrumented
// step and the real update read and write the same objects - if either of these
// were to exist twice, the simulation would quietly drift away from the model
// it is supposed to be simulating.
int y = 0;
int bpos = 0;

Random  rnd;
Ilog    ilog;
Ptable  pt;
Stretch stretch;

#endif

// Model state that does not live inside the Coder object: the journal's address
// window has to cover it, and -DTRACK_VERIFY compares it (main.inc).
#define TRACK_STATE_RANGES \
  , {"y",   (byte*)&y,    sizeof(y)   } \
  , {"bpos",(byte*)&bpos, sizeof(bpos)} \
  , {"rnd", (byte*)&rnd,  sizeof(rnd) }

#include "main.inc"
