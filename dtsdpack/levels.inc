// levels.inc -- the levels of this build: every level, or the one a tuning build has
// (-DDTSD_ONLY=L, as mk.sh builds it; LvOnly then). Each level is its generated families
// (MOD/l<L>/), wrapped into configuration classes (famcfg.inc), and its own class, which Model
// takes (lv<L>.inc). c without a level takes the first, kLevels[0], and with_level() runs what a
// level is asked for. First what the levels are made of: the coder, the context mixing parts,
// IDX's runtime, the families and their variables.
#include "cmbase.inc"
#include "coder.inc"
#include "cm.inc"
#include "mapping.inc"
#include "masking.inc"
#include "fam.inc"
#include "famvars.inc"

#ifndef DTSD_ONLY
#define DTSD_LV(n) 1
#else
#define DTSD_LV(n) ((n) == DTSD_ONLY)
#endif
#if DTSD_LV(9) && __has_include("lv9.inc")
#include "lv9.inc"
#define DTSD_L9(X) X(9)
#else
#define DTSD_L9(X)
#endif
#if DTSD_LV(5) && __has_include("lv5.inc")
#include "lv5.inc"
#define DTSD_L5(X) X(5)
#else
#define DTSD_L5(X)
#endif
#if DTSD_LV(1) && __has_include("lv1.inc")
#include "lv1.inc"
#define DTSD_L1(X) X(1)
#else
#define DTSD_L1(X)
#endif
#define DTSD_LEVELS(X) DTSD_L9(X) DTSD_L5(X) DTSD_L1(X)
#ifdef DTSD_ONLY
#define DTSD_CAT2(a, b) a##b
#define DTSD_CAT(a, b) DTSD_CAT2(a, b)
typedef DTSD_CAT(Lv, DTSD_ONLY) LvOnly;  // (the one level of the build)
#endif
#define DTSD_FIRST(n) n,
static constexpr int kLevels[] = {DTSD_LEVELS(DTSD_FIRST)};
#undef DTSD_FIRST

// with_level<F>(level, a...): F<LvN>::run(a...) for the level N of this build that level is, else
// kNoLevel
constexpr int kNoLevel = -2;
template <template <class> class F, class... A>
int with_level(int level, A&&... a) {
  switch (level) {
#define DTSD_W(n) \
  case n:         \
    return F<Lv##n>::run(static_cast<A&&>(a)...);
    DTSD_LEVELS(DTSD_W)
#undef DTSD_W
  }
  return kNoLevel;
}
