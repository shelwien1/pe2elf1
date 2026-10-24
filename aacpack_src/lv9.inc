// lv9.inc -- level -9: in each family the ten counters, the three mixers
// and the final one, and the two APMs (h264p's level -9 shape). Its
// families as IDX/l9 declares them, generated into MOD/l9: each family's
// constants and tables (_h.inc), its index builder and its configuration
// class C9_<family> (_p.inc, by IDX/l9/family.inc and famcfg.inc); and the
// level's class, which Model<f_DEC, Lv9> takes.
#include "MOD/l9/unt_h.inc"
#include "MOD/l9/unt_p.inc"
#include "MOD/l9/hdr_h.inc"
#include "MOD/l9/hdr_p.inc"
#include "MOD/l9/sec_h.inc"
#include "MOD/l9/sec_p.inc"
#include "MOD/l9/scf_h.inc"
#include "MOD/l9/scf_p.inc"
#include "MOD/l9/spc_h.inc"
#include "MOD/l9/spc_p.inc"
#include "MOD/l9/esc_h.inc"
#include "MOD/l9/esc_p.inc"
#include "MOD/l9/sgn_h.inc"
#include "MOD/l9/sgn_p.inc"
#include "MOD/l9/sbr_h.inc"
#include "MOD/l9/sbr_p.inc"

struct Lv9 {
  static const int id = 9;  // the header's level byte
#define LV_FAM(F) typedef C9_##F F;
  AAC_FAMS(LV_FAM)
#undef LV_FAM
};
