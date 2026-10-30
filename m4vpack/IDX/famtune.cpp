// famtune.cpp -- a family's dumps coded again (h264p's famtune.cpp, for m4vpack): what m4vpack,
// built with -DM4VPACK_DUMP, wrote of a family as it compressed streams (fam.inc), coded by the
// family as this binary has it -- one level's tuning build, whose "!MAP!" patterns IDX/ftune.py
// patches between runs. A trial of a family's parameters then costs a pass over its own
// decisions, not the compression of the streams. Prints the coded size, in bytes (the ideal code
// length, as m4vpack c -s counts it).
//
//   ./mk.sh famtune L           (the binary, ./famtune)
//   famtune FAMILY DUMP...      e.g. famtune sig dump/*/sig.dump
//
// Each dump file holds one or more streams (R), each coded from a fresh state, as m4vpack codes
// each stream.

#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#endif

// (the coder's bytes go through a coroutine's output, io.inc)
#include "../Lib3/common.inc"
#include "../Lib3/coro3b.inc"

#include "../base.inc"
#include "../io.inc"
#include "../cmbase.inc"
#include "../coder.inc"
#include "../cm.inc"
#include "../mapping.inc"
#include "../masking.inc"
#include "../fam.inc"
#include "../famvars.inc"

#ifndef M4V_ONLY
#error "famtune.cpp: build it for one level, -DM4V_ONLY=L (mk.sh famtune L)"
#endif
#if M4V_ONLY == 9
#include "../lv9.inc"
typedef Lv9 FtLv;
#elif M4V_ONLY == 7
#include "../lv7.inc"
typedef Lv7 FtLv;
#elif M4V_ONLY == 5
#include "../lv5.inc"
typedef Lv5 FtLv;
#elif M4V_ONLY == 3
#include "../lv3.inc"
typedef Lv3 FtLv;
#elif M4V_ONLY == 1
#include "../lv1.inc"
typedef Lv1 FtLv;
#else
#error "famtune.cpp: no such level"
#endif

namespace {

// the file fn's bytes, into d (n of them)
char* slurp(const char* fn, size_t& n) {
  FILE* f = fopen(fn, "rb");
  char* d = nullptr;
  size_t cap = 0;
  n = 0;
  if (!f) fatal("cannot read %s", fn);
  for (;;) {
    if (n + IO_BUF > cap) d = (char*)xrealloc(d, cap = 2 * cap + IO_BUF);
    size_t k = fread(d + n, 1, IO_BUF, f);
    if (!k) break;
    n += k;
  }
  fclose(f);
  return d;
}

// a hint's state from a dump (HS_*: the rows of HS_N it picks in Z, ZM and the final mixer's)
int hint_state(const char*& p, const char* e) {
  i64 hs = tc_dump_get(p, e);
  if (hs < 0 || hs >= HS_N) fatal("a hint state of %lld in a dump", (long long)hs);
  return (int)hs;
}

// One family's dumps, as Model codes them: its tables mapped fresh at each stream (zero, as
// tc_table maps them), the hint's counters made at once (code() makes them at its first hint,
// zero too).
template <class C>
void replay(char** files, int n, Coder& rc) {
  typedef tc_fam<0, C> F;
  static const int any = 0;  // (prpa only set: what bitm() asks of it)
  F* f = nullptr;              // the family (the last stream's), on zeroed pages
  typename C::T* t = nullptr;  // its tables
  typename C::V D;
  tcx X{};
  for (int i = 0; i < n; i++) {
    size_t nd;
    char* d = slurp(files[i], nd);
    const char *p = d, *e = p + nd;
    while (p < e) {
      char tag = *p++;
      if (tag == 'R') {
        if (t) C::quit(t), tc_unmap(t, sizeof(typename C::T));
        t = (typename C::T*)tc_map(sizeof(typename C::T));
        C::init(t);
        if (f) f->Quit(), tc_unmap(f, sizeof(F));
        f = (F*)tc_map(sizeof(F), "a family");
        f->Init();
        f->wire(t, &rc);
        f->Z.fill(C::kT * (sz)C::ND * HS_N, cm_cnt2{});
        f->ZM.fill(C::kT * (sz)C::MND * HS_N, cm_cnt{});
        continue;
      }
      if (!f) fatal("a dump that does not start with a stream");
      switch (tag) {
        case 'M': {
          int* v = reinterpret_cast<int*>(&D);
          for (size_t k = 0; k < sizeof D / sizeof(int); k++) v[k] = (int)tc_dump_get(p, e);
          C::make(D, X);
          break;
        }
        case 'S':
          f->select(X);
          break;
        case 'B': {
          int node = (int)tc_dump_get(p, e), b = (int)tc_dump_get(p, e);
          f->prp = (int)tc_dump_get(p, e);
          f->prpa = nullptr;
          f->hs = hint_state(p, e);
          f->bit(node, b);  // (its node checked: a dump of another shape fails, not writes past a table)
          break;
        }
        case 'm': {
          int nb = (int)tc_dump_get(p, e), q = (int)tc_dump_get(p, e), b = (int)tc_dump_get(p, e);
          f->hs = hint_state(p, e);
          f->prp = (int)tc_dump_get(p, e);
          f->prpa = tc_dump_get(p, e) ? &any : nullptr;
          u64 pref = (u64)tc_dump_get(p, e);
          f->bitm(tc_node_man(nb, q, pref, C::mt), b);
          break;
        }
        case 'G': {
          u32 sc = (u32)tc_dump_get(p, e);
          f->sign(sc, (int)tc_dump_get(p, e));
          break;
        }
        default:
          fatal("a broken dump (%s)", files[i]);
      }
    }
    free(d);
  }
  if (f) f->Quit(), tc_unmap(f, sizeof(F));
  if (t) C::quit(t), tc_unmap(t, sizeof(typename C::T));
}

#include "replay.inc"  // Replay: the replay as a coroutine

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {  // (a family, a dump at least)
    fprintf(stderr, "usage: famtune FAMILY DUMP...   (level -%d)\n", FtLv::id);
    return E_USAGE;
  }
  coder_tables_init();
  // (on pages of its own, zeroed: a coroutine's jmp_bufs are aligned)
  Replay* r = (Replay*)tc_map(sizeof(Replay), "the replay");
  static uint8_t out[IO_BUF];
  double bits = 0;
  r->Init(argc, argv);
  r->addout(out, sizeof out);
  while (r->coro_call(r) == Y_OUT) r->addout(out, sizeof out);
  for (int i = 0; i < NCAT; i++) bits += r->co.cost[i];
  printf("%.3f\n", bits / BYTE_BITS);

  return 0;
}
