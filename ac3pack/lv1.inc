// lv1.inc -- level -1: the fastest: in each family four counters, which keep their probabilities
// stretched (cm_cnt2s), and one mixer, of 16 inputs.
// Its families as IDX/l1 declares them, generated into MOD/l1: each family's constants and tables
// (_h.inc), its index builder and its configuration class C1_<family> (_p.inc, by
// IDX/l1/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv1> takes.
#include "MOD/l1/families.inc"

struct Lv1 {
  static constexpr int id = 1;  // the header's level byte
#define LV_FAM(F) typedef C1_##F F;
  AC3P_FAMS(LV_FAM)
#undef LV_FAM
};
