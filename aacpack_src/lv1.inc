// lv1.inc -- level -1, the fastest: in each family the four counters of -5
// whose loss costs it most, which keep their probabilities stretched
// (cm_cnt2s), and one mixer, no APMs (as m4vpack's and h264p's level -1).
// Its families as IDX/l1 declares them, generated into MOD/l1: each family's
// constants and tables (_h.inc), its index builder and its configuration
// class C1_<family> (_p.inc, by IDX/l1/family.inc and famcfg.inc); and the
// level's class, which Model<f_DEC, Lv1> takes.
#include "MOD/l1/unt_h.inc"
#include "MOD/l1/unt_p.inc"
#include "MOD/l1/hdr_h.inc"
#include "MOD/l1/hdr_p.inc"
#include "MOD/l1/sec_h.inc"
#include "MOD/l1/sec_p.inc"
#include "MOD/l1/scf_h.inc"
#include "MOD/l1/scf_p.inc"
#include "MOD/l1/spc_h.inc"
#include "MOD/l1/spc_p.inc"
#include "MOD/l1/esc_h.inc"
#include "MOD/l1/esc_p.inc"
#include "MOD/l1/sgn_h.inc"
#include "MOD/l1/sgn_p.inc"
#include "MOD/l1/sbr_h.inc"
#include "MOD/l1/sbr_p.inc"

struct Lv1 {
  static const int id = 1;  // the header's level byte
#define LV_FAM(F) typedef C1_##F F;
  AAC_FAMS(LV_FAM)
#undef LV_FAM
};
