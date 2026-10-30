// levels.inc -- the levels of this build: every level, or the one a tuning build has
// (-DM4V_ONLY=L, as mk.sh builds it; LvOnly then). Each level is its generated families
// (MOD/l<L>/), wrapped into configuration classes (famcfg.inc), and its own class, which Model
// takes (lv<L>.inc). c without -L takes the first, kLevels[0], and with_level() runs what a level
// is asked for. First what the levels are made of: the coder, the context mixing parts, IDX's
// runtime, the families and their variables.
#include "cmbase.inc"
#include "coder.inc"
#include "cm.inc"
#include "mapping.inc"
#include "masking.inc"
#include "fam.inc"
#include "famvars.inc"

#ifndef M4V_ONLY
#define M4V_LV(n) 1
#else
#define M4V_LV(n) ((n) == M4V_ONLY)
#endif
#if M4V_LV(9) && __has_include("lv9.inc")
#include "lv9.inc"
#define M4V_L9(X) X(9)
#else
#define M4V_L9(X)
#endif
#if M4V_LV(7) && __has_include("lv7.inc")
#include "lv7.inc"
#define M4V_L7(X) X(7)
#else
#define M4V_L7(X)
#endif
#if M4V_LV(5) && __has_include("lv5.inc")
#include "lv5.inc"
#define M4V_L5(X) X(5)
#else
#define M4V_L5(X)
#endif
#if M4V_LV(3) && __has_include("lv3.inc")
#include "lv3.inc"
#define M4V_L3(X) X(3)
#else
#define M4V_L3(X)
#endif
#if M4V_LV(1) && __has_include("lv1.inc")
#include "lv1.inc"
#define M4V_L1(X) X(1)
#else
#define M4V_L1(X)
#endif
#define M4V_LEVELS(X) M4V_L9(X) M4V_L7(X) M4V_L5(X) M4V_L3(X) M4V_L1(X)
#ifdef M4V_ONLY
#define M4V_CAT2(a, b) a##b
#define M4V_CAT(a, b) M4V_CAT2(a, b)
typedef M4V_CAT(Lv, M4V_ONLY) LvOnly;  // (the one level of the build)
#endif
#define M4V_FIRST(n) n,
static constexpr int kLevels[] = {M4V_LEVELS(M4V_FIRST)};
#undef M4V_FIRST

// with_level<F>(level, a...): F<LvN>::run(a...) for the level N of this build that level is, else
// kNoLevel
constexpr int kNoLevel = -2;
template <template <class> class F, class... A>
int with_level(int level, A&&... a) {
  switch (level) {
#define M4V_W(n) \
  case n:        \
    return F<Lv##n>::run(static_cast<A&&>(a)...);
    M4V_LEVELS(M4V_W)
#undef M4V_W
  }
  return kNoLevel;
}
