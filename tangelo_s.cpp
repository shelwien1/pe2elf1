// tangelo_s - the Tangelo model coded a byte at a time, journaling its own
// writes.
//
// Same algorithm as tangelo_w.cpp and the same output, byte for byte, from a
// different mechanism. tangelo_w compiles the model step twice and has track.pl
// insert a journaling call in front of every store in the compiler's assembly
// output; here the model's source says where it writes - every write to model
// state is spelled W(x) = ... (see write.inc) - and the journal cell is appended
// inline. One translation unit, one copy of the step, no assembly, no perl, and
// none of the constraints the two-translation-unit build brings with it: no
// weak-symbol hazard, no red zone to disable, no -fno-builtin, no refusal to
// build on an instruction the instrumenter does not recognise.
//
// What it gives up is the guarantee. track.pl cannot miss a store; a human
// marking up a model can, and the failure is silent - encoder and decoder
// corrupt the model identically and still agree, so only the compressed size
// suffers. -DTRACK_VERIFY is the answer to that, and it is what the two
// programs agreeing on every byte of book1 is evidence for.
//
// What it gains, besides being portable: the model can say "all of this is
// about to change" once instead of journaling it a word at a time. The mixer's
// weight rows and input vector are declared that way in tangelo/mixer1.inc,
// which no instrumenter working from assembly could infer.
//
// Built in one step:  CXX CXXFLAGS tangelo_s.cpp -o tangelo_s

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

// The same pruning threshold as tangelo_w, so the two walk the same tree and
// their output can be compared byte for byte. See tangelo_w.cpp for the curve.
#ifndef PRUNE_LOG
 #define PRUNE_LOG 0x80000
#endif

// Journal the model's writes from the model's own source. This has to come
// before the model, because W() has to be declared before the model uses it,
// and after the journal, because that is what W() writes into.
#define TRACK_SRC 1
#include "log2lut.inc"
#include "track.inc"
#include "write.inc"

#include "tangelo/tangelo.inc"

#include "sh_v1m.inc"

#include "coder_tangelo.inc"

// One translation unit, so these are simply defined - no declare-here,
// define-there split, which is the whole of tangelo/common.inc's shared-state
// problem and one of the things this route does not have.
int y = 0;
int bpos = 0;

Random  rnd;
Ilog    ilog;
Ptable  pt;
Stretch stretch;

// Model state that does not live inside the Coder object: the journal's address
// window has to cover it, and -DTRACK_VERIFY compares it (main.inc).
#define TRACK_STATE_RANGES \
  , {"y",   (byte*)&y,    sizeof(y)   } \
  , {"bpos",(byte*)&bpos, sizeof(bpos)} \
  , {"rnd", (byte*)&rnd,  sizeof(rnd) }

#include "main.inc"
