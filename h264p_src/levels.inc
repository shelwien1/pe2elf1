// levels.inc -- the levels of this build (h264p_levels.md sections 4, 6
// and 7): every level, or the one a tuning build has (-DH264P_ONLY=L, as
// mk.sh builds it). Each level is its generated families (MOD/l<L>/),
// wrapped into configuration classes (famcfg.inc), and its own class, which
// Model takes (lv<L>.inc). c without -L takes the first, kLevels[0], and
// with_level() runs what a level is asked for. (Included by model.inc,
// inside namespace h264p.)
#ifndef H264P_ONLY
#define H264P_LV(n) 1
#else
#define H264P_LV(n) ((n) == H264P_ONLY)
#endif
#if H264P_LV(9)
#include "lv9.inc"
#define H264P_L9(X) X(9)
#else
#define H264P_L9(X)
#endif
#if H264P_LV(7)
#include "lv7.inc"
#define H264P_L7(X) X(7)
#else
#define H264P_L7(X)
#endif
#if H264P_LV(5)
#include "lv5.inc"
#define H264P_L5(X) X(5)
#else
#define H264P_L5(X)
#endif
#if H264P_LV(3)
#include "lv3.inc"
#define H264P_L3(X) X(3)
#else
#define H264P_L3(X)
#endif
#if H264P_LV(1)
#include "lv1.inc"
#define H264P_L1(X) X(1)
#else
#define H264P_L1(X)
#endif
#define H264P_LEVELS(X) H264P_L9(X) H264P_L7(X) H264P_L5(X) H264P_L3(X) H264P_L1(X)
#define H264P_FIRST(n) n,
const int kLevels[] = {H264P_LEVELS(H264P_FIRST)};
#undef H264P_FIRST

// with_level<F>(level, a...): F<LvN>::run(a...) for the level N of this
// build that level is, else kNoLevel (not kRestart, -1, a run's status)
const int kNoLevel = -2;
template <template <class> class F, class... A>
int with_level(int level, A&&... a) {
  switch (level) {
#define H264P_W(n) \
  case n:          \
    return F<Lv##n>::run(std::forward<A>(a)...);
    H264P_LEVELS(H264P_W)
#undef H264P_W
  }
  return kNoLevel;
}
