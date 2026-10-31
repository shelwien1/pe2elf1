// lv5.inc -- level -5: in each family the ten counters and one mixer, no APM.
// Its families as IDX/l5 declares them, generated into MOD/l5: each family's constants and tables
// (_h.inc), its index builder and its configuration class C5_<family> (_p.inc, by
// IDX/l5/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv5> takes.
#include "MOD/l5/families.inc"

struct Lv5 {
  static constexpr int id = 5;  // the header's level byte
#define LV_FAM(F) typedef C5_##F F;
  AC3P_FAMS(LV_FAM)
#undef LV_FAM
};
