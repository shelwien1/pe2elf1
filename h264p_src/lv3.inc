// lv3.inc -- level -3: no decoder in the loop, but its reference lists and
// the pictures' motion (ReconRefs); in each family -9's coder without its
// APMs (h264p_levels.md sections 4 and 6).
// Its families as IDX/l3 declares them, generated into MOD/l3: each
// family's constants and tables (_h.inc), its index builder and its
// configuration class C3_<family> (_p.inc, by IDX/l3/family.inc and
// famcfg.inc); and the level's class, which Model<f_DEC, Lv3> takes.
#include "MOD/l3/nal_h.inc"
#include "MOD/l3/nal_p.inc"
#include "MOD/l3/hdr_h.inc"
#include "MOD/l3/hdr_p.inc"
#include "MOD/l3/skp_h.inc"
#include "MOD/l3/skp_p.inc"
#include "MOD/l3/mbt_h.inc"
#include "MOD/l3/mbt_p.inc"
#include "MOD/l3/cbp_h.inc"
#include "MOD/l3/cbp_p.inc"
#include "MOD/l3/ipm_h.inc"
#include "MOD/l3/ipm_p.inc"
#include "MOD/l3/mvs_h.inc"
#include "MOD/l3/mvs_p.inc"
#include "MOD/l3/qpd_h.inc"
#include "MOD/l3/qpd_p.inc"
#include "MOD/l3/cbf_h.inc"
#include "MOD/l3/cbf_p.inc"
#include "MOD/l3/sig_h.inc"
#include "MOD/l3/sig_p.inc"
#include "MOD/l3/lvl_h.inc"
#include "MOD/l3/lvl_p.inc"
#include "MOD/l3/sgn_h.inc"
#include "MOD/l3/sgn_p.inc"
#include "MOD/l3/mvg_h.inc"
#include "MOD/l3/mvg_p.inc"
#include "MOD/l3/qpg_h.inc"
#include "MOD/l3/qpg_p.inc"
#include "MOD/l3/pcm_h.inc"
#include "MOD/l3/pcm_p.inc"

struct Lv3 {
  static const int id = 3;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions, the motion vectors as
  // the decoder has them (mvp; the decoder and refs need them)
  static const bool refs = true, recon = false, search = false, proxy = false, signpred = false, skippx = false, deblock = false,
                    cabpred = true, mvp = true;
#define LV_FAM(F) typedef C3_##F F;
  H264P_FAMS(LV_FAM)
#undef LV_FAM
};
