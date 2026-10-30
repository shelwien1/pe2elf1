// lv7.inc -- level -7: in each family the ten counters, the three mixers and the final one, and
// the two APMs; no decoder in the loop.
// Its families as IDX/l7 declares them, generated into MOD/l7: each family's constants and tables
// (_h.inc), its index builder and its configuration class C7_<family> (_p.inc, by
// IDX/l7/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv7> takes.
#include "MOD/l7/families.inc"

struct Lv7 {
  static constexpr int id = 7;  // the header's level byte
  // the model's parts: no decoder in the loop
  static constexpr bool recon = false;
#define LV_FAM(F) typedef C7_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
