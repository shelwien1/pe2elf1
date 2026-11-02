// levels.inc -- the levels of this build (as h264p's): every level, or the
// one a tuning build has (-DAACPACK_ONLY=L, as mk.sh builds it). Each level
// is its generated families (MOD/l<L>/), wrapped into configuration classes
// (famcfg.inc), and its own class, which Model takes (lv<L>.inc). c without
// -L takes the first, kLevels[0], and with_level() runs what a level is
// asked for. (Included by model.inc, inside namespace aacpack.)
#ifndef AACPACK_ONLY
#define AACPACK_LV(n) 1
#else
#define AACPACK_LV(n) ((n) == AACPACK_ONLY)
#endif
#if AACPACK_LV(9)
#include "lv9.inc"
#define AACPACK_L9(X) X(9)
#else
#define AACPACK_L9(X)
#endif
#if AACPACK_LV(7)
#include "lv7.inc"
#define AACPACK_L7(X) X(7)
#else
#define AACPACK_L7(X)
#endif
#if AACPACK_LV(5)
#include "lv5.inc"
#define AACPACK_L5(X) X(5)
#else
#define AACPACK_L5(X)
#endif
#if AACPACK_LV(3)
#include "lv3.inc"
#define AACPACK_L3(X) X(3)
#else
#define AACPACK_L3(X)
#endif
#if AACPACK_LV(1)
#include "lv1.inc"
#define AACPACK_L1(X) X(1)
#else
#define AACPACK_L1(X)
#endif
#define AACPACK_LEVELS(X) AACPACK_L9(X) AACPACK_L7(X) AACPACK_L5(X) AACPACK_L3(X) AACPACK_L1(X)
#define AACPACK_FIRST(n) n,
const int kLevels[] = {AACPACK_LEVELS(AACPACK_FIRST)};
#undef AACPACK_FIRST

// with_level<F>(level, a...): F<LvN>::run(a...) for the level N of this
// build that level is, else kNoLevel
const int kNoLevel = -2;
template <template <class> class F, class... A>
int with_level(int level, A&&... a) {
  switch (level) {
#define AACPACK_W(n) \
  case n:            \
    return F<Lv##n>::run(static_cast<A&&>(a)...);
    AACPACK_LEVELS(AACPACK_W)
#undef AACPACK_W
  }
  return kNoLevel;
}
