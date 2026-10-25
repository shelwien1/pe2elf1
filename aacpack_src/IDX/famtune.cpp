// famtune.cpp -- a family's dumps coded again: what aacpack, built with
// -DAACPACK_DUMP, wrote of a family as it compressed streams (fam.inc),
// coded by the family as this binary has it -- one level's tuning build,
// whose "!MAP!" patterns IDX/ftune.py patches between runs. A trial of a
// family's parameters then costs a pass over its own decisions, not the
// compression of the streams. Prints the coded size, in bytes (the ideal
// code length, as aacpack -v counts it). (h264p's, with aacpack's names.)
//
//   ./mk.sh famtune L           (the binary, ../famtune)
//   famtune FAMILY DUMP...      e.g. famtune spc dump/*/spc.dump
//
// Each dump file holds one or more streams (R), each coded from a fresh
// state, as aacpack codes each stream.

#include <inttypes.h>
#include <limits.h>
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

// (the coder's bytes go through a coroutine's output, rcio.inc)
#include "../Lib3/common.inc"
#include "../Lib3/coro3b.inc"

#include "../err.inc"
#include "../mem.inc"
#include "../cmbase.inc"
#include "../rcio.inc"
#include "../rc.inc"
#include "../cm.inc"
#include "../mapping.inc"
#include "../fam.inc"
#include "../famvars.inc"

#ifndef AACPACK_ONLY
#error "famtune.cpp: build it for one level, -DAACPACK_ONLY=L (mk.sh famtune L)"
#endif
// (the level's families, inside the namespace as model.inc includes them)
namespace aacpack {
#if AACPACK_ONLY == 9
#include "../lv9.inc"
typedef Lv9 FtLv;
#else
#error "famtune.cpp: no such level"
#endif
}  // namespace aacpack

using namespace aacpack;

namespace {

// the file fn's bytes, into d
void slurp(const char* fn, Buf<char>& d) {
  FILE* f = fopen(fn, "rb");
  if (!f) err_raise("cannot read %s", fn);
  d.clear();
  char buf[1 << 16];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) d.append(buf, n);
  fclose(f);
}

// One family's dumps, as Model codes them: its tables mapped fresh at each
// stream (zero, as tc_table maps them).
template <class C>
void replay(char** files, int n, tc_rc<0>& rc) {
  typedef tc_fam<0, C> F;
  static const int any = 0;  // (prpa only set: what bitm() asks of it)
  F* f = nullptr;              // the family (the last stream's), on zeroed pages
  typename C::T* t = nullptr;  // its tables
  typename C::V D;
  tcx X{};
  Buf<char> d;
  d.Init();
  for (int i = 0; i < n; i++) {
    slurp(files[i], d);
    const char *p = d.data(), *e = p + d.size();
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
        continue;
      }
      if (!f) err_raise("a dump that does not start with a stream");
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
          f->bit(node, b);  // (its node checked: a dump of another shape fails, not writes past a table)
          break;
        }
        case 'm': {
          int nb = (int)tc_dump_get(p, e), q = (int)tc_dump_get(p, e), b = (int)tc_dump_get(p, e);
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
          err_raise("a broken dump (%s)", files[i]);
      }
    }
  }
  if (f) f->Quit(), tc_unmap(f, sizeof(F));
  if (t) C::quit(t), tc_unmap(t, sizeof(typename C::T));
  d.Quit();
}

// The replay as a coroutine, as the model codes (its coder is a coroutine's,
// rcio.inc), whose output main() throws away: what counts is the size -v
// counts (tc_bits, stage 0 throughout)
struct Replay : tc_rc<0> {
  char** argv;
  int argc;
  bool failed;
  char err[256];  // (failed) why
  void Init(int c, char** v) {
    coro_init();
    tc_rc<0>::Init();
    argc = c, argv = v;
    failed = false;
    err[0] = 0;
  }
  void do_process() {
    io_init();
    ErrTrap t;  // (an error: err.inc)
    if (!setjmp(t.jb)) {
      err_push(t);
      rc_Init();
#define FT_FAM(F) !strcmp(argv[1], #F) ? replay<FtLv::F>(argv + 2, argc - 2, *this) :
      AAC_FAMS(FT_FAM) err_raise("no family %s", argv[1]);
#undef FT_FAM
      err_pop(t);
    } else {
      failed = true;
      snprintf(err, sizeof err, "%s", t.why);
    }
    for (;;) yield(static_cast<Coroutine*>(this), failed ? 3 : 0);
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: famtune FAMILY DUMP...   (level -%d)\n", FtLv::id);
    return 2;
  }
  // (on pages of its own, zeroed: a coroutine's jmp_bufs are aligned)
  Replay* r = (Replay*)tc_map(sizeof(Replay), "the replay");
  static uint8_t out[1 << 16];
  r->Init(argc, argv);
  r->tc_verbose = true;
  r->addout(out, sizeof out);
  while (r->coro_call(r) == 2) r->addout(out, sizeof out);
  if (r->failed) {
    fprintf(stderr, "famtune: %s\n", r->err);
    return 1;
  }
  printf("%.3f\n", r->tc_bits[0] / 8);
  return 0;
}
