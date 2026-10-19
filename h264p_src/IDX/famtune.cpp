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

template <class R>
R take(const char*& p, const char* e) {
  R r;
  if ((size_t)(e - p) < sizeof r) cm_fail("famtune: a dump cut short");
  memcpy(&r, p, sizeof r);
  p += sizeof r;
  return r;
}

// One family's dumps, as Model codes them: its tables mapped fresh at each
// stream (zero, as tc_table maps them), the hint's counters made at once
// (code() makes them at its first hint, zero too).
template <class C>
void replay(char** files, int n) {
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
        f->wire(t);
        f->Z.assign(C::kT * (sz)C::ND * 5, cm_cnt2{});
        f->ZM.assign(C::kT * (sz)C::MND * 5, cm_cnt{});
        continue;
      }
      if (!f) cm_fail("famtune: a dump that does not start with a stream");
      switch (tag) {
        case 'M':
          D = take<typename C::V>(p, e);
          C::make(D, X);
          break;
        case 'S':
          f->select(X);
          break;
        case 'B': {
          tc_dump_b r = take<tc_dump_b>(p, e);
          f->prp = r.pp;
          f->prpa = nullptr;
          f->hs = r.hs;
          f->bit_x(r.node, r.b);
          break;
        }
        case 'm': {
          tc_dump_m r = take<tc_dump_m>(p, e);
          f->prp = r.prp;
          f->prpa = r.pa ? &any : nullptr;
          f->hs = r.hs;
          f->bitm(tc_node_man(r.nb, r.p, r.pref, C::mt), r.b);
          break;
        }
        case 'G': {
          tc_dump_g r = take<tc_dump_g>(p, e);
          f->sign(r.sc, r.s);
          break;
        }
        default:
          cm_fail("famtune: a broken dump (%s)", files[i]);
      }
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: famtune FAMILY DUMP...   (level -%d)\n", FtLv::id);
    return 2;
  }
  try {
    cm_tables();
    tc_verbose = 1;
    tc_rc<0>::rc.rc_Init();
#define FT_FAM(F) !strcmp(argv[1], #F) ? replay<FtLv::F>(argv + 2, argc - 2) :
    FT_FAM(nal) FT_FAM(hdr) FT_FAM(skp) FT_FAM(mbt) FT_FAM(cbp) FT_FAM(ipm) FT_FAM(mvs) FT_FAM(qpd) FT_FAM(cbf)
        FT_FAM(sig) FT_FAM(lvl) FT_FAM(sgn) FT_FAM(mvg) FT_FAM(qpg) FT_FAM(pcm) cm_fail("famtune: no family %s", argv[1]);
#undef FT_FAM
  } catch (const std::exception& e) {
    fprintf(stderr, "famtune: %s\n", e.what());
    return 1;
  }
  printf("%.3f\n", tc_bits[0] / 8);
  return 0;
}
