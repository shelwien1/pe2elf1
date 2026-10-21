// lv1.inc -- level -1, the fastest: no decoder in the loop, no CABAC
// states, and in each family four counters and one mixer, no APMs
// (h264p_levels.md sections 4 and 6).
// Its families as IDX/l1 declares them, generated into MOD/l1: each
// family's constants and tables (_h.inc), its index builder and its
// configuration class C1_<family> (_p.inc, by IDX/l1/family.inc and
// famcfg.inc); and the level's class, which Model<f_DEC, Lv1> takes.
#include "MOD/l1/nal_h.inc"
#include "MOD/l1/nal_p.inc"
#include "MOD/l1/hdr_h.inc"
#include "MOD/l1/hdr_p.inc"
#include "MOD/l1/skp_h.inc"
#include "MOD/l1/skp_p.inc"
#include "MOD/l1/mbt_h.inc"
#include "MOD/l1/mbt_p.inc"
#include "MOD/l1/cbp_h.inc"
#include "MOD/l1/cbp_p.inc"
#include "MOD/l1/ipm_h.inc"
#include "MOD/l1/ipm_p.inc"
#include "MOD/l1/mvs_h.inc"
#include "MOD/l1/mvs_p.inc"
#include "MOD/l1/qpd_h.inc"
#include "MOD/l1/qpd_p.inc"
#include "MOD/l1/cbf_h.inc"
#include "MOD/l1/cbf_p.inc"
#include "MOD/l1/sig_h.inc"
#include "MOD/l1/sig_p.inc"
#include "MOD/l1/lvl_h.inc"
#include "MOD/l1/lvl_p.inc"
#include "MOD/l1/sgn_h.inc"
#include "MOD/l1/sgn_p.inc"
#include "MOD/l1/mvg_h.inc"
#include "MOD/l1/mvg_p.inc"
#include "MOD/l1/qpg_h.inc"
#include "MOD/l1/qpg_p.inc"
#include "MOD/l1/pcm_h.inc"
#include "MOD/l1/pcm_p.inc"

struct Lv1 {
  static const int id = 1;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions, the motion vectors as
  // the decoder has them (mvp; the decoder and refs need them)
  // (not even the reference lists: they would take -1 0.8% smaller and
  // 9% slower; nor the motion vectors: 0.5% for 11%, h264p_l1_speed.md
  // 3.1)
  static const bool refs = false, recon = false, search = false, proxy = false, signpred = false, skippx = false,
                    deblock = false, cabpred = false, mvp = false;
#define LV_FAM(F) typedef C1_##F F;
  H264P_FAMS(LV_FAM)
#undef LV_FAM
};
