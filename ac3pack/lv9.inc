// lv9.inc -- level -9: the strongest: in each family the ten counters, the three mixers and the
// final one, and the two APMs.
// Its families as IDX/l9 declares them, generated into MOD/l9: each family's constants and tables
// (_h.inc), its index builder and its configuration class C9_<family> (_p.inc, by
// IDX/l9/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv9> takes.
#include "MOD/l9/families.inc"

struct Lv9 {
  static constexpr int id = 9;  // the header's level byte
#define LV_FAM(F) typedef C9_##F F;
  AC3P_FAMS(LV_FAM)
#undef LV_FAM
};
