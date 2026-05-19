
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>
//#include <conio.h>
#include <ctype.h>

#ifdef _MSC_VER
#include <intrin.h>
#include <process.h>   // _exit
#include <io.h>        // _chsize, _fileno
#else
#include <unistd.h>    // _exit, ftruncate
#include <x86intrin.h>
#include <xmmintrin.h>
#define _chsize(fd, sz) ftruncate(fd, sz)
#endif

#define _WINDOWS_
#include "defs.h"
#undef _WINDOWS_

#ifndef _MSC_VER
#define __declspec(x) __attribute__((x))
#define align(n) aligned(n)
#endif

#include "PPMonstr.h"

#ifndef _MSC_VER
#undef align
#undef __declspec
#endif

// ---------------------------------------------------------------------------
// Probe registry. Each decompiled function declares a Probe via PROBE_DECL
// and bumps its counter on entry via PROBE_HIT. The ExitProcess hook walks
// the linked list and prints all counters before the process dies.

struct Probe {
  Probe* next;
  const char* name;
  unsigned long long count;
  Probe(const char* n);
};

static Probe* g_probes = nullptr;
Probe::Probe(const char* n) : next(g_probes), name(n), count(0) { g_probes = this; }

#if 0
#define PROBE_DECL(sym) static Probe sym##_probe(#sym);
#define PROBE_HIT(sym)  __sync_fetch_and_add(&sym##_probe.count, 1ULL)
#else
#define PROBE_DECL(sym) 
#define PROBE_HIT(sym)  
#endif

// ---------------------------------------------------------------------------
// Decompiled function bodies.

#ifdef __GNUC__
#define _fileno fileno
//#define _chsize ftruncate
#endif

//typedef unsigned long long qword;
//#include "valloc.inc"

namespace {

#include "defs3a.h"
#include "subs__3b.inc"

}

// ---------------------------------------------------------------------------
// ExitProcess IAT hook. The shim's kernel32_ExitProcess ends in _exit(),
// which bypasses both DT_FINI and atexit, so we hook the IAT slot directly.
// The wrapper must be ms_abi: PPMonstr calls through the IAT using Win64.

static void my_ExitProcess(unsigned int code) {
  fprintf(stderr, "[probe] ExitProcess(%u), call counts:\n", code);
  for (Probe* p = g_probes; p; p = p->next)
    fprintf(stderr, "[probe]   %-32s %llu\n", p->name, p->count);
  fflush(stderr);
  _exit((int)code);
}

int main( int argc, const char **argv ) {

  //memset( blob1, 0, blob1_len );
  memcpy( blob1, blob, blob_len );

  (int&)__algn_141136BC4[1084+0] = 0x01000000;
  (int&)__algn_141136BC4[1084+4] = 0x03030201;
  (int&)__algn_141136BC4[1084+8] = 0x00000004;

  __main( argc, argv );

//  my_ExitProcess(0);
}
