// lv3.inc -- level -3: in each family the six counters of -5 whose loss
// costs it most, and one mixer, no APMs (as m4vpack's level -3).
// Its families as IDX/l3 declares them, generated into MOD/l3: each family's
// constants and tables (_h.inc), its index builder and its configuration
// class C3_<family> (_p.inc, by IDX/l3/family.inc and famcfg.inc); and the
// level's class, which Model<f_DEC, Lv3> takes.
#include "MOD/l3/unt_h.inc"
#include "MOD/l3/unt_p.inc"
#include "MOD/l3/hdr_h.inc"
#include "MOD/l3/hdr_p.inc"
#include "MOD/l3/sec_h.inc"
#include "MOD/l3/sec_p.inc"
#include "MOD/l3/scf_h.inc"
#include "MOD/l3/scf_p.inc"
#include "MOD/l3/spc_h.inc"
#include "MOD/l3/spc_p.inc"
#include "MOD/l3/esc_h.inc"
#include "MOD/l3/esc_p.inc"
#include "MOD/l3/sgn_h.inc"
#include "MOD/l3/sgn_p.inc"
#include "MOD/l3/sbr_h.inc"
#include "MOD/l3/sbr_p.inc"

struct Lv3 {
  static const int id = 3;  // the header's level byte
#define LV_FAM(F) typedef C3_##F F;
  AAC_FAMS(LV_FAM)
#undef LV_FAM
};
