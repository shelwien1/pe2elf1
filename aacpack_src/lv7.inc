// lv7.inc -- level -7: -9 without the APMs -- in each family the ten
// counters, the three mixers and the final one (h264p's level -7 shape).
// Its families as IDX/l7 declares them, generated into MOD/l7: each family's
// constants and tables (_h.inc), its index builder and its configuration
// class C7_<family> (_p.inc, by IDX/l7/family.inc and famcfg.inc); and the
// level's class, which Model<f_DEC, Lv7> takes.
#include "MOD/l7/unt_h.inc"
#include "MOD/l7/unt_p.inc"
#include "MOD/l7/hdr_h.inc"
#include "MOD/l7/hdr_p.inc"
#include "MOD/l7/sec_h.inc"
#include "MOD/l7/sec_p.inc"
#include "MOD/l7/scf_h.inc"
#include "MOD/l7/scf_p.inc"
#include "MOD/l7/spc_h.inc"
#include "MOD/l7/spc_p.inc"
#include "MOD/l7/esc_h.inc"
#include "MOD/l7/esc_p.inc"
#include "MOD/l7/sgn_h.inc"
#include "MOD/l7/sgn_p.inc"
#include "MOD/l7/sbr_h.inc"
#include "MOD/l7/sbr_p.inc"

struct Lv7 {
  static const int id = 7;  // the header's level byte
#define LV_FAM(F) typedef C7_##F F;
  AAC_FAMS(LV_FAM)
#undef LV_FAM
};
