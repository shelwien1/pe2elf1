// famtune.cpp -- a family's dumps coded again: what h264p, built with
// -DH264P_DUMP, wrote of a family as it compressed streams (fam.inc), coded
// by the family as this binary has it -- one level's tuning build, whose
// "!MAP!" patterns IDX/ftune.py patches between runs. A trial of a
// family's parameters then costs a pass over its own decisions, not the
// compression of the streams. Prints the coded size, in bytes (the ideal
// code length, as h264p -v counts it).
//
//   ./mk.sh famtune L           (the binary, ../famtune)
//   famtune FAMILY DUMP...      e.g. famtune sig dump/*/sig.dump
//
// Each dump file holds one or more streams (R), each coded from a fresh
// state, as h264p codes each stream.

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// (the coder's bytes go through a coroutine's output, rcio.inc)
#include "../Lib3/common.inc"
#include "../Lib3/coro3b.inc"

#include "../cmbase.inc"
#include "../rcio.inc"
#include "../rc.inc"
#include "../cm.inc"
#include "../mapping.inc"
#include "../fam.inc"
#include "../famvars.inc"

#ifndef H264P_ONLY
#error "famtune.cpp: build it for one level, -DH264P_ONLY=L (mk.sh famtune L)"
#endif
// (the level's families, inside the namespace as model.inc includes them)
namespace h264p {
#if H264P_ONLY == 9
#include "../lv9.inc"
typedef Lv9 FtLv;
#elif H264P_ONLY == 7
#include "../lv7.inc"
typedef Lv7 FtLv;
#elif H264P_ONLY == 5
#include "../lv5.inc"
typedef Lv5 FtLv;
#elif H264P_ONLY == 3
#include "../lv3.inc"
typedef Lv3 FtLv;
#elif H264P_ONLY == 1
#include "../lv1.inc"
typedef Lv1 FtLv;
#else
#error "famtune.cpp: no such level"
#endif
}  // namespace h264p

using namespace h264p;

namespace {

std::vector<char> slurp(const char* fn) {
  FILE* f = fopen(fn, "rb");
  if (!f) cm_fail("cannot read %s", fn);
  std::vector<char> d;
  char buf[1 << 16];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) d.insert(d.end(), buf, buf + n);
  fclose(f);
  return d;
}

// One family's dumps, as Model codes them: its tables mapped fresh at each
// stream (zero, as tc_table maps them), the hint's counters made at once
// (code() makes them at its first hint, zero too).
template <class C>
void replay(char** files, int n, tc_rc<0>& rc) {
  typedef tc_fam<0, C> F;
  static const int any = 0;  // (prpa only set: what bitm() asks of it)
  std::unique_ptr<F> f;
  typename C::V D;
  tcx X{};
  for (int i = 0; i < n; i++) {
    std::vector<char> d = slurp(files[i]);
    const char *p = d.data(), *e = p + d.size();
    while (p < e) {
      char tag = *p++;
      if (tag == 'R') {
        typename C::T* t = new (tc_map(sizeof(typename C::T))) typename C::T;
        C::init(t);
        f.reset(new F());
        f->wire(t, &rc);
        f->Z.assign(C::kT * (sz)C::ND * 5, cm_cnt2{});
        f->ZM.assign(C::kT * (sz)C::MND * 5, cm_cnt{});
        continue;
      }
      if (!f) cm_fail("famtune: a dump that does not start with a stream");
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
          f->hs = (int)tc_dump_get(p, e);
          f->bit_x(node, b);
          break;
        }
        case 'm': {
          int nb = (int)tc_dump_get(p, e), q = (int)tc_dump_get(p, e), b = (int)tc_dump_get(p, e);
          f->hs = (int)tc_dump_get(p, e);
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
          cm_fail("famtune: a broken dump (%s)", files[i]);
      }
    }
  }
}

// The replay as a coroutine, as the model codes (its coder is a coroutine's,
// rcio.inc), whose output main() throws away: what counts is the size -v
// counts (tc_bits, stage 0 throughout)
struct Replay : tc_rc<0> {
  char** argv;
  int argc;
  std::string err;  // empty: done
  void do_process() {
    io_init();
    try {
      rc_Init();
#define FT_FAM(F) !strcmp(argv[1], #F) ? replay<FtLv::F>(argv + 2, argc - 2, *this) :
      FT_FAM(nal) FT_FAM(hdr) FT_FAM(skp) FT_FAM(mbt) FT_FAM(cbp) FT_FAM(ipm) FT_FAM(mvs) FT_FAM(qpd) FT_FAM(cbf)
          FT_FAM(sig) FT_FAM(lvl) FT_FAM(sgn) FT_FAM(mvg) FT_FAM(qpg) FT_FAM(pcm) cm_fail("famtune: no family %s", argv[1]);
#undef FT_FAM
    } catch (const std::exception& e) {
      err = e.what();
    }
    for (;;) yield(static_cast<Coroutine*>(this), err.empty() ? 0 : 3);
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: famtune FAMILY DUMP...   (level -%d)\n", FtLv::id);
    return 2;
  }
  // (on pages of its own: a coroutine's jmp_bufs are aligned)
  Replay* r = new (tc_map(sizeof(Replay), "the replay")) Replay();
  static uint8_t out[1 << 16];
  r->coro_init();
  r->argv = argv;
  r->argc = argc;
  r->tc_verbose = true;
  r->addout(out, sizeof out);
  while (r->coro_call(r) == 2) r->addout(out, sizeof out);
  if (!r->err.empty()) {
    fprintf(stderr, "famtune: %s\n", r->err.c_str());
    return 1;
  }
  printf("%.3f\n", r->tc_bits[0] / 8);
  return 0;
}
