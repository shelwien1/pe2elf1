// lv9.inc -- level -9, the strongest: every part of the model, and in
// each family the ten counters, the three mixers and the final one, and
// the two APMs (h264p_levels.md sections 4 and 6).
// Its families as IDX/l9 declares them (generated into MOD/l9), each
// wrapped into its configuration class (famcfg.inc), and the level's class,
// which Model<f_DEC, Lv9> takes.
#include "MOD/l9/nal_h.inc"
#include "MOD/l9/hdr_h.inc"
#include "MOD/l9/skp_h.inc"
#include "MOD/l9/mbt_h.inc"
#include "MOD/l9/cbp_h.inc"
#include "MOD/l9/ipm_h.inc"
#include "MOD/l9/mvs_h.inc"
#include "MOD/l9/qpd_h.inc"
#include "MOD/l9/cbf_h.inc"
#include "MOD/l9/sig_h.inc"
#include "MOD/l9/lvl_h.inc"
#include "MOD/l9/sgn_h.inc"
#include "MOD/l9/mvg_h.inc"
#include "MOD/l9/qpg_h.inc"
#include "MOD/l9/pcm_h.inc"

#include "MOD/l9/nal_p.inc"
#include "MOD/l9/hdr_p.inc"
#include "MOD/l9/skp_p.inc"
#include "MOD/l9/mbt_p.inc"
#include "MOD/l9/cbp_p.inc"
#include "MOD/l9/ipm_p.inc"
#include "MOD/l9/mvs_p.inc"
#include "MOD/l9/qpd_p.inc"
#include "MOD/l9/cbf_p.inc"
#include "MOD/l9/sig_p.inc"
#include "MOD/l9/lvl_p.inc"
#include "MOD/l9/sgn_p.inc"
#include "MOD/l9/mvg_p.inc"
#include "MOD/l9/qpg_p.inc"
#include "MOD/l9/pcm_p.inc"

#define FC_NAME C9_nal
#define FC_PFX H9_nal_
#define FC_FAM nal
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_hdr
#define FC_PFX H9_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_skp
#define FC_PFX H9_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_mbt
#define FC_PFX H9_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_cbp
#define FC_PFX H9_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_ipm
#define FC_PFX H9_ipm_
#define FC_FAM ipm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_mvs
#define FC_PFX H9_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_qpd
#define FC_PFX H9_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_cbf
#define FC_PFX H9_cbf_
#define FC_FAM cbf
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_sig
#define FC_PFX H9_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_lvl
#define FC_PFX H9_lvl_
#define FC_FAM lvl
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_sgn
#define FC_PFX H9_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_mvg
#define FC_PFX H9_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_qpg
#define FC_PFX H9_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

#define FC_NAME C9_pcm
#define FC_PFX H9_pcm_
#define FC_FAM pcm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "famcfg.inc"

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
  typedef C9_nal nal;
  typedef C9_hdr hdr;
  typedef C9_skp skp;
  typedef C9_mbt mbt;
  typedef C9_cbp cbp;
  typedef C9_ipm ipm;
  typedef C9_mvs mvs;
  typedef C9_qpd qpd;
  typedef C9_cbf cbf;
  typedef C9_sig sig;
  typedef C9_lvl lvl;
  typedef C9_sgn sgn;
  typedef C9_mvg mvg;
  typedef C9_qpg qpg;
  typedef C9_pcm pcm;
};
