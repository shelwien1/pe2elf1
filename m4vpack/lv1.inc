// lv1.inc -- level -1: the fastest: in each family four counters, which keep their probabilities
// stretched (cm_cnt2s), and one mixer, of 16 inputs.
// Its families as IDX/l1 declares them, generated into MOD/l1: each family's constants and tables
// (_h.inc), its index builder and its configuration class C1_<family> (_p.inc, by
// IDX/l1/family.inc and famcfg.inc); and the level's class, which Model<f_DEC, Lv1> takes.
#include "MOD/l1/hdr_h.inc"
#include "MOD/l1/hdr_p.inc"
#include "MOD/l1/raw_h.inc"
#include "MOD/l1/raw_p.inc"
#include "MOD/l1/skp_h.inc"
#include "MOD/l1/skp_p.inc"
#include "MOD/l1/mbt_h.inc"
#include "MOD/l1/mbt_p.inc"
#include "MOD/l1/cbp_h.inc"
#include "MOD/l1/cbp_p.inc"
#include "MOD/l1/qpd_h.inc"
#include "MOD/l1/qpd_p.inc"
#include "MOD/l1/mvd_h.inc"
#include "MOD/l1/mvd_p.inc"
#include "MOD/l1/mvg_h.inc"
#include "MOD/l1/mvg_p.inc"
#include "MOD/l1/dcs_h.inc"
#include "MOD/l1/dcs_p.inc"
#include "MOD/l1/sig_h.inc"
#include "MOD/l1/sig_p.inc"
#include "MOD/l1/lvl_h.inc"
#include "MOD/l1/lvl_p.inc"
#include "MOD/l1/sgn_h.inc"
#include "MOD/l1/sgn_p.inc"
#include "MOD/l1/esc_h.inc"
#include "MOD/l1/esc_p.inc"

struct Lv1 {
  static constexpr int id = 1;  // the header's level byte
  // the model's parts: no decoder in the loop
  static constexpr bool recon = false;
#define LV_FAM(F) typedef C1_##F F;
  M4V_FAMS(LV_FAM)
#undef LV_FAM
};
