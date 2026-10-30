// lv3.inc -- level -3: in each family six counters and one mixer, of 16 inputs.
// Its families as IDX/l3 declares them, generated into MOD/l3: each family's constants and tables
// (_h.inc), its index builder and its configuration class C3_<family> (_p.inc, by
// IDX/l3/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv3> takes.
#include "MOD/l3/families.inc"

struct Lv3 {
  static constexpr int id = 3;  // the header's level byte
  // the model's parts: no decoder in the loop
  static constexpr bool recon = false;
#define LV_FAM(F) typedef C3_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
