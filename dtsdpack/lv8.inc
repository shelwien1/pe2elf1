// lv8.inc -- level -8: level -9's model, its families' shape the same (in each the ten counters,
// the three mixers and the final one, and the two APMs), with the parameters of hit, vqr, abt and
// smp tuned on a DTS:X encoder's stream (the Cymatics stream and its cuts), which c takes without
// a level where the binding mixes two WAV channels, one with a gain before the mix
// (Call::pick_level()).
// Its families as IDX/l8 declares them, generated into MOD/l8: each family's constants and tables
// (_h.inc), its index builder and its configuration class C8_<family> (_p.inc, by
// IDX/l8/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv8> takes.
#include "MOD/l8/families.inc"

struct Lv8 {
  static constexpr int id = 8;  // the header's level byte
#define LV_FAM(F) typedef C8_##F F;
  DTSD_FAMS(LV_FAM)
#undef LV_FAM
};
