// lv7.inc -- level -7: -9 without the decoder's template searches, and in
// each family without the APMs (h264p_levels.md sections 4 and 6).
// Its families as IDX/l7 declares them, generated into MOD/l7: each
// family's constants and tables (_h.inc), its index builder and its
// configuration class C7_<family> (_p.inc, by IDX/l7/family.inc and
// famcfg.inc); and the level's class, which Model<f_DEC, Lv7> takes.
#include "MOD/l7/nal_h.inc"
#include "MOD/l7/nal_p.inc"
#include "MOD/l7/hdr_h.inc"
#include "MOD/l7/hdr_p.inc"
#include "MOD/l7/skp_h.inc"
#include "MOD/l7/skp_p.inc"
#include "MOD/l7/mbt_h.inc"
#include "MOD/l7/mbt_p.inc"
#include "MOD/l7/cbp_h.inc"
#include "MOD/l7/cbp_p.inc"
#include "MOD/l7/ipm_h.inc"
#include "MOD/l7/ipm_p.inc"
#include "MOD/l7/mvs_h.inc"
#include "MOD/l7/mvs_p.inc"
#include "MOD/l7/qpd_h.inc"
#include "MOD/l7/qpd_p.inc"
#include "MOD/l7/cbf_h.inc"
#include "MOD/l7/cbf_p.inc"
#include "MOD/l7/sig_h.inc"
#include "MOD/l7/sig_p.inc"
#include "MOD/l7/lvl_h.inc"
#include "MOD/l7/lvl_p.inc"
#include "MOD/l7/sgn_h.inc"
#include "MOD/l7/sgn_p.inc"
#include "MOD/l7/mvg_h.inc"
#include "MOD/l7/mvg_p.inc"
#include "MOD/l7/qpg_h.inc"
#include "MOD/l7/qpg_p.inc"
#include "MOD/l7/pcm_h.inc"
#include "MOD/l7/pcm_p.inc"

struct Lv7 {
  static const int id = 7;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions, the motion vectors as
  // the decoder has them (mvp; the decoder and refs need them)
  static const bool refs = true, recon = true, search = false, proxy = true, signpred = true, skippx = true, deblock = true,
                    cabpred = true, mvp = true;
#define LV_FAM(F) typedef C7_##F F;
  H264P_FAMS(LV_FAM)
#undef LV_FAM
};
