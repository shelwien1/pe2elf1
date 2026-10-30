// lv7.inc -- level -7: in each family the ten counters, the three mixers and the final one, and
// the two APMs; no decoder in the loop.
// Its families as IDX/l7 declares them, generated into MOD/l7: each family's constants and tables
// (_h.inc), its index builder and its configuration class C7_<family> (_p.inc, by
// IDX/l7/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv7> takes.
#include "MOD/l7/hdr_h.inc"
#include "MOD/l7/hdr_p.inc"
#include "MOD/l7/raw_h.inc"
#include "MOD/l7/raw_p.inc"
#include "MOD/l7/skp_h.inc"
#include "MOD/l7/skp_p.inc"
#include "MOD/l7/mbt_h.inc"
#include "MOD/l7/mbt_p.inc"
#include "MOD/l7/cbp_h.inc"
#include "MOD/l7/cbp_p.inc"
#include "MOD/l7/qpd_h.inc"
#include "MOD/l7/qpd_p.inc"
#include "MOD/l7/mvd_h.inc"
#include "MOD/l7/mvd_p.inc"
#include "MOD/l7/mvg_h.inc"
#include "MOD/l7/mvg_p.inc"
#include "MOD/l7/dcs_h.inc"
#include "MOD/l7/dcs_p.inc"
#include "MOD/l7/sig_h.inc"
#include "MOD/l7/sig_p.inc"
#include "MOD/l7/lvl_h.inc"
#include "MOD/l7/lvl_p.inc"
#include "MOD/l7/sgn_h.inc"
#include "MOD/l7/sgn_p.inc"
#include "MOD/l7/esc_h.inc"
#include "MOD/l7/esc_p.inc"

struct Lv7 {
  static constexpr int id = 7;  // the header's level byte
  // the model's parts: no decoder in the loop
  static constexpr bool recon = false;
#define LV_FAM(F) typedef C7_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
