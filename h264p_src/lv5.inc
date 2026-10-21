// lv5.inc -- level -5: the decoder only for the maps of the last picture,
// the textures and the edge misses, the pictures unfiltered, no CABAC
// states; in each family the ten counters and one mixer, no APMs
// (h264p_levels.md sections 4 and 6).
// Its families as IDX/l5 declares them, generated into MOD/l5: each
// family's constants and tables (_h.inc), its index builder and its
// configuration class C5_<family> (_p.inc, by IDX/l5/family.inc and
// famcfg.inc); and the level's class, which Model<f_DEC, Lv5> takes.
#include "MOD/l5/nal_h.inc"
#include "MOD/l5/nal_p.inc"
#include "MOD/l5/hdr_h.inc"
#include "MOD/l5/hdr_p.inc"
#include "MOD/l5/skp_h.inc"
#include "MOD/l5/skp_p.inc"
#include "MOD/l5/mbt_h.inc"
#include "MOD/l5/mbt_p.inc"
#include "MOD/l5/cbp_h.inc"
#include "MOD/l5/cbp_p.inc"
#include "MOD/l5/ipm_h.inc"
#include "MOD/l5/ipm_p.inc"
#include "MOD/l5/mvs_h.inc"
#include "MOD/l5/mvs_p.inc"
#include "MOD/l5/qpd_h.inc"
#include "MOD/l5/qpd_p.inc"
#include "MOD/l5/cbf_h.inc"
#include "MOD/l5/cbf_p.inc"
#include "MOD/l5/sig_h.inc"
#include "MOD/l5/sig_p.inc"
#include "MOD/l5/lvl_h.inc"
#include "MOD/l5/lvl_p.inc"
#include "MOD/l5/sgn_h.inc"
#include "MOD/l5/sgn_p.inc"
#include "MOD/l5/mvg_h.inc"
#include "MOD/l5/mvg_p.inc"
#include "MOD/l5/qpg_h.inc"
#include "MOD/l5/qpg_p.inc"
#include "MOD/l5/pcm_h.inc"
#include "MOD/l5/pcm_p.inc"

struct Lv5 {
  static const int id = 5;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions, the motion vectors as
  // the decoder has them (mvp; the decoder and refs need them)
  static const bool refs = true, recon = true, search = false, proxy = false, signpred = false, skippx = false, deblock = false,
                    cabpred = false, mvp = true;
#define LV_FAM(F) typedef C5_##F F;
  H264P_FAMS(LV_FAM)
#undef LV_FAM
};
