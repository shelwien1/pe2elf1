// lv9.inc -- level -9: the strongest: the decoder in the loop, and in each family the ten
// counters, the three mixers and the final one, and the two APMs.
// Its families as IDX/l9 declares them, generated into MOD/l9: each family's constants and tables
// (_h.inc), its index builder and its configuration class C9_<family> (_p.inc, by
// IDX/l9/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv9> takes.
#include "MOD/l9/families.inc"

struct Lv9 {
  static constexpr int id = 9;  // the header's level byte
  // the model's parts: the decoder in the loop (recon.inc)
  static constexpr bool recon = true;
#define LV_FAM(F) typedef C9_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
