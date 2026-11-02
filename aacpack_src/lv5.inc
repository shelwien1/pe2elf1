// lv5.inc -- level -5: in each family the ten counters and one mixer, no
// APMs (as m4vpack's and h264p's level -5).
// Its families as IDX/l5 declares them, generated into MOD/l5: each family's
// constants and tables (_h.inc), its index builder and its configuration
// class C5_<family> (_p.inc, by IDX/l5/family.inc and famcfg.inc); and the
// level's class, which Model<f_DEC, Lv5> takes.
#include "MOD/l5/unt_h.inc"
#include "MOD/l5/unt_p.inc"
#include "MOD/l5/hdr_h.inc"
#include "MOD/l5/hdr_p.inc"
#include "MOD/l5/sec_h.inc"
#include "MOD/l5/sec_p.inc"
#include "MOD/l5/scf_h.inc"
#include "MOD/l5/scf_p.inc"
#include "MOD/l5/spc_h.inc"
#include "MOD/l5/spc_p.inc"
#include "MOD/l5/esc_h.inc"
#include "MOD/l5/esc_p.inc"
#include "MOD/l5/sgn_h.inc"
#include "MOD/l5/sgn_p.inc"
#include "MOD/l5/sbr_h.inc"
#include "MOD/l5/sbr_p.inc"

struct Lv5 {
  static const int id = 5;  // the header's level byte
#define LV_FAM(F) typedef C5_##F F;
  AAC_FAMS(LV_FAM)
#undef LV_FAM
};
