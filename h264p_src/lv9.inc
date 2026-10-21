// lv9.inc -- level -9, the strongest: every part of the model, and in
// each family the ten counters, the three mixers and the final one, and
// the two APMs (h264p_levels.md sections 4 and 6).
// Its families as IDX/l9 declares them, generated into MOD/l9: each
// family's constants and tables (_h.inc), its index builder and its
// configuration class C9_<family> (_p.inc, by IDX/l9/family.inc and
// famcfg.inc); and the level's class, which Model<f_DEC, Lv9> takes.
#include "MOD/l9/nal_h.inc"
#include "MOD/l9/nal_p.inc"
#include "MOD/l9/hdr_h.inc"
#include "MOD/l9/hdr_p.inc"
#include "MOD/l9/skp_h.inc"
#include "MOD/l9/skp_p.inc"
#include "MOD/l9/mbt_h.inc"
#include "MOD/l9/mbt_p.inc"
#include "MOD/l9/cbp_h.inc"
#include "MOD/l9/cbp_p.inc"
#include "MOD/l9/ipm_h.inc"
#include "MOD/l9/ipm_p.inc"
#include "MOD/l9/mvs_h.inc"
#include "MOD/l9/mvs_p.inc"
#include "MOD/l9/qpd_h.inc"
#include "MOD/l9/qpd_p.inc"
#include "MOD/l9/cbf_h.inc"
#include "MOD/l9/cbf_p.inc"
#include "MOD/l9/sig_h.inc"
#include "MOD/l9/sig_p.inc"
#include "MOD/l9/lvl_h.inc"
#include "MOD/l9/lvl_p.inc"
#include "MOD/l9/sgn_h.inc"
#include "MOD/l9/sgn_p.inc"
#include "MOD/l9/mvg_h.inc"
#include "MOD/l9/mvg_p.inc"
#include "MOD/l9/qpg_h.inc"
#include "MOD/l9/qpg_p.inc"
#include "MOD/l9/pcm_h.inc"
#include "MOD/l9/pcm_p.inc"

struct Lv9 {
  static const int id = 9;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions, the motion vectors as
  // the decoder has them (mvp; the decoder and refs need them)
  static const bool refs = true, recon = true, search = true, proxy = true, signpred = true, skippx = true, deblock = true,
                    cabpred = true, mvp = true;
#define LV_FAM(F) typedef C9_##F F;
  H264P_FAMS(LV_FAM)
#undef LV_FAM
};
