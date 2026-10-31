// levels.inc -- the levels of this build: every level, or the one a tuning build has
// (-DAC3P_ONLY=L, as mk.sh builds it; LvOnly then). Each level is its generated families
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

#ifndef AC3P_ONLY
#define AC3P_LV(n) 1
#else
#define AC3P_LV(n) ((n) == AC3P_ONLY)
#endif
#if AC3P_LV(9) && __has_include("lv9.inc")
#include "lv9.inc"
#define AC3P_L9(X) X(9)
#else
#define AC3P_L9(X)
#endif
#if AC3P_LV(5) && __has_include("lv5.inc")
#include "lv5.inc"
#define AC3P_L5(X) X(5)
#else
#define AC3P_L5(X)
#endif
#if AC3P_LV(1) && __has_include("lv1.inc")
#include "lv1.inc"
#define AC3P_L1(X) X(1)
#else
#define AC3P_L1(X)
#endif
#define AC3P_LEVELS(X) AC3P_L9(X) AC3P_L5(X) AC3P_L1(X)
#ifdef AC3P_ONLY
#define AC3P_CAT2(a, b) a##b
#define AC3P_CAT(a, b) AC3P_CAT2(a, b)
typedef AC3P_CAT(Lv, AC3P_ONLY) LvOnly;  // (the one level of the build)
#endif
#define AC3P_FIRST(n) n,
static constexpr int kLevels[] = {AC3P_LEVELS(AC3P_FIRST)};
#undef AC3P_FIRST

// with_level<F>(level, a...): F<LvN>::run(a...) for the level N of this build that level is, else
// kNoLevel
constexpr int kNoLevel = -2;
template <template <class> class F, class... A>
int with_level(int level, A&&... a) {
  switch (level) {
#define AC3P_W(n) \
  case n:         \
    return F<Lv##n>::run(static_cast<A&&>(a)...);
    AC3P_LEVELS(AC3P_W)
#undef AC3P_W
  }
  return kNoLevel;
}
