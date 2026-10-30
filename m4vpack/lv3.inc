// lv3.inc -- level -3: in each family six counters and one mixer, of 16 inputs.
// Its families as IDX/l3 declares them, generated into MOD/l3: each family's constants and tables
// (_h.inc), its index builder and its configuration class C3_<family> (_p.inc, by
// IDX/l3/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv3> takes.
#include "MOD/l3/hdr_h.inc"
#include "MOD/l3/hdr_p.inc"
#include "MOD/l3/raw_h.inc"
#include "MOD/l3/raw_p.inc"
#include "MOD/l3/skp_h.inc"
#include "MOD/l3/skp_p.inc"
#include "MOD/l3/mbt_h.inc"
#include "MOD/l3/mbt_p.inc"
#include "MOD/l3/cbp_h.inc"
#include "MOD/l3/cbp_p.inc"
#include "MOD/l3/qpd_h.inc"
#include "MOD/l3/qpd_p.inc"
#include "MOD/l3/mvd_h.inc"
#include "MOD/l3/mvd_p.inc"
#include "MOD/l3/mvg_h.inc"
#include "MOD/l3/mvg_p.inc"
#include "MOD/l3/dcs_h.inc"
#include "MOD/l3/dcs_p.inc"
#include "MOD/l3/sig_h.inc"
#include "MOD/l3/sig_p.inc"
#include "MOD/l3/lvl_h.inc"
#include "MOD/l3/lvl_p.inc"
#include "MOD/l3/sgn_h.inc"
#include "MOD/l3/sgn_p.inc"
#include "MOD/l3/esc_h.inc"
#include "MOD/l3/esc_p.inc"

struct Lv3 {
  static constexpr int id = 3;  // the header's level byte
  // the model's parts: no decoder in the loop
  static constexpr bool recon = false;
#define LV_FAM(F) typedef C3_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
