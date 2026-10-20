// lv7.inc -- level -7: -9 without the decoder's template searches, and in
// each family without the APMs (h264p_levels.md sections 4 and 6).
// Its families as IDX/l7 declares them (generated into MOD/l7), each
// wrapped into its configuration class (famcfg.inc), and the level's class,
// which Model<f_DEC, Lv7> takes.
#include "MOD/l7/nal_h.inc"
#include "MOD/l7/hdr_h.inc"
#include "MOD/l7/skp_h.inc"
#include "MOD/l7/mbt_h.inc"
#include "MOD/l7/cbp_h.inc"
#include "MOD/l7/ipm_h.inc"
#include "MOD/l7/mvs_h.inc"
#include "MOD/l7/qpd_h.inc"
#include "MOD/l7/cbf_h.inc"
#include "MOD/l7/sig_h.inc"
#include "MOD/l7/lvl_h.inc"
#include "MOD/l7/sgn_h.inc"
#include "MOD/l7/mvg_h.inc"
#include "MOD/l7/qpg_h.inc"
#include "MOD/l7/pcm_h.inc"

#include "MOD/l7/nal_p.inc"
#include "MOD/l7/hdr_p.inc"
#include "MOD/l7/skp_p.inc"
#include "MOD/l7/mbt_p.inc"
#include "MOD/l7/cbp_p.inc"
#include "MOD/l7/ipm_p.inc"
#include "MOD/l7/mvs_p.inc"
#include "MOD/l7/qpd_p.inc"
#include "MOD/l7/cbf_p.inc"
#include "MOD/l7/sig_p.inc"
#include "MOD/l7/lvl_p.inc"
#include "MOD/l7/sgn_p.inc"
#include "MOD/l7/mvg_p.inc"
#include "MOD/l7/qpg_p.inc"
#include "MOD/l7/pcm_p.inc"

#define FC_NAME C7_nal
#define FC_PFX H7_nal_
#define FC_FAM nal
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_hdr
#define FC_PFX H7_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_skp
#define FC_PFX H7_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_mbt
#define FC_PFX H7_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_cbp
#define FC_PFX H7_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_ipm
#define FC_PFX H7_ipm_
#define FC_FAM ipm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_mvs
#define FC_PFX H7_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_qpd
#define FC_PFX H7_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_cbf
#define FC_PFX H7_cbf_
#define FC_FAM cbf
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_sig
#define FC_PFX H7_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_lvl
#define FC_PFX H7_lvl_
#define FC_FAM lvl
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_sgn
#define FC_PFX H7_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_mvg
#define FC_PFX H7_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_qpg
#define FC_PFX H7_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C7_pcm
#define FC_PFX H7_pcm_
#define FC_FAM pcm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

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
  typedef C7_nal nal;
  typedef C7_hdr hdr;
  typedef C7_skp skp;
  typedef C7_mbt mbt;
  typedef C7_cbp cbp;
  typedef C7_ipm ipm;
  typedef C7_mvs mvs;
  typedef C7_qpd qpd;
  typedef C7_cbf cbf;
  typedef C7_sig sig;
  typedef C7_lvl lvl;
  typedef C7_sgn sgn;
  typedef C7_mvg mvg;
  typedef C7_qpg qpg;
  typedef C7_pcm pcm;
};
