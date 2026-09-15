
//#include "tsx.inc"
//#include "tsx1.inc"
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


#include "sh_mixer.inc"

#include "model.inc"

#include "sh_v1m.inc"

#include "coder.inc"

#include "main.inc"
