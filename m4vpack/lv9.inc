// lv9.inc -- level -9: the strongest: the decoder in the loop, and in each family the ten
// counters, the three mixers and the final one, and the two APMs.
// Its families as IDX/l9 declares them, generated into MOD/l9: each family's constants and tables
// (_h.inc), its index builder and its configuration class C9_<family> (_p.inc, by
// IDX/l9/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv9> takes.
#include "MOD/l9/hdr_h.inc"
#include "MOD/l9/hdr_p.inc"
#include "MOD/l9/raw_h.inc"
#include "MOD/l9/raw_p.inc"
#include "MOD/l9/skp_h.inc"
#include "MOD/l9/skp_p.inc"
#include "MOD/l9/mbt_h.inc"
#include "MOD/l9/mbt_p.inc"
#include "MOD/l9/cbp_h.inc"
#include "MOD/l9/cbp_p.inc"
#include "MOD/l9/qpd_h.inc"
#include "MOD/l9/qpd_p.inc"
#include "MOD/l9/mvd_h.inc"
#include "MOD/l9/mvd_p.inc"
#include "MOD/l9/mvg_h.inc"
#include "MOD/l9/mvg_p.inc"
#include "MOD/l9/dcs_h.inc"
#include "MOD/l9/dcs_p.inc"
#include "MOD/l9/sig_h.inc"
#include "MOD/l9/sig_p.inc"
#include "MOD/l9/lvl_h.inc"
#include "MOD/l9/lvl_p.inc"
#include "MOD/l9/sgn_h.inc"
#include "MOD/l9/sgn_p.inc"
#include "MOD/l9/esc_h.inc"
#include "MOD/l9/esc_p.inc"

struct Lv9 {
  static constexpr int id = 9;  // the header's level byte
  // the model's parts: the decoder in the loop (recon.inc)
  static constexpr bool recon = true;
#define LV_FAM(F) typedef C9_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
