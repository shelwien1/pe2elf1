// lv3.inc -- level -3: no decoder in the loop; in each family -9's coder without its
// APMs (h264p_levels.md
// sections 4 and 6). Its families as IDX/l3 declares them (generated into
// MOD/l3), each wrapped into its configuration class (famcfg.inc), and the
// level's class, which Model<f_DEC, Lv3> takes.
#include "MOD/l3/nal_h.inc"
#include "MOD/l3/hdr_h.inc"
#include "MOD/l3/skp_h.inc"
#include "MOD/l3/mbt_h.inc"
#include "MOD/l3/cbp_h.inc"
#include "MOD/l3/ipm_h.inc"
#include "MOD/l3/mvs_h.inc"
#include "MOD/l3/qpd_h.inc"
#include "MOD/l3/cbf_h.inc"
#include "MOD/l3/sig_h.inc"
#include "MOD/l3/lvl_h.inc"
#include "MOD/l3/sgn_h.inc"
#include "MOD/l3/mvg_h.inc"
#include "MOD/l3/qpg_h.inc"
#include "MOD/l3/pcm_h.inc"

#include "MOD/l3/nal_p.inc"
#include "MOD/l3/hdr_p.inc"
#include "MOD/l3/skp_p.inc"
#include "MOD/l3/mbt_p.inc"
#include "MOD/l3/cbp_p.inc"
#include "MOD/l3/ipm_p.inc"
#include "MOD/l3/mvs_p.inc"
#include "MOD/l3/qpd_p.inc"
#include "MOD/l3/cbf_p.inc"
#include "MOD/l3/sig_p.inc"
#include "MOD/l3/lvl_p.inc"
#include "MOD/l3/sgn_p.inc"
#include "MOD/l3/mvg_p.inc"
#include "MOD/l3/qpg_p.inc"
#include "MOD/l3/pcm_p.inc"

#define FC_NAME C3_nal
#define FC_PFX H3_nal_
#define FC_FAM nal
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_hdr
#define FC_PFX H3_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_skp
#define FC_PFX H3_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_mbt
#define FC_PFX H3_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_cbp
#define FC_PFX H3_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_ipm
#define FC_PFX H3_ipm_
#define FC_FAM ipm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_mvs
#define FC_PFX H3_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_qpd
#define FC_PFX H3_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_cbf
#define FC_PFX H3_cbf_
#define FC_FAM cbf
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_sig
#define FC_PFX H3_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_lvl
#define FC_PFX H3_lvl_
#define FC_FAM lvl
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_sgn
#define FC_PFX H3_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_mvg
#define FC_PFX H3_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_qpg
#define FC_PFX H3_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C3_pcm
#define FC_PFX H3_pcm_
#define FC_FAM pcm
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "famcfg.inc"

struct Lv3 {
  static const int id = 3;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions
  static const bool refs = true, recon = false, search = false, proxy = false, signpred = false, skippx = false, deblock = false,
                    cabpred = true;
  typedef C3_nal nal;
  typedef C3_hdr hdr;
  typedef C3_skp skp;
  typedef C3_mbt mbt;
  typedef C3_cbp cbp;
  typedef C3_ipm ipm;
  typedef C3_mvs mvs;
  typedef C3_qpd qpd;
  typedef C3_cbf cbf;
  typedef C3_sig sig;
  typedef C3_lvl lvl;
  typedef C3_sgn sgn;
  typedef C3_mvg mvg;
  typedef C3_qpg qpg;
  typedef C3_pcm pcm;
};
