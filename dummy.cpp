
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>
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

#ifdef __GNUC__
#define _fileno fileno
#endif

#include "defs1.h"

// ---------------------------------------------------------------------------
// Probe registry. Each function declares a Probe via PROBE_DECL
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

namespace {

#include "defs3f.h"
#include "rc.inc"
//#include "subs__3b.inc"
#include "subs__3c.inc"
#include "stats.inc"
#include "main1.inc"

}

void my_ExitProcess(unsigned int code) {
  fprintf(stderr, "[probe] ExitProcess(%u), call counts:\n", code);
  for (Probe* p = g_probes; p; p = p->next)
    fprintf(stderr, "[probe]   %-32s %llu\n", p->name, p->count);
  fflush(stderr);
  _exit((int)code);
}

int main( int argc, const char **argv ) {

  //memcpy( blob1, blob, blob_len );

  return __main( argc, argv );

//  my_ExitProcess(0);
}
