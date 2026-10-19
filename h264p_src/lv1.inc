// lv1.inc -- level -1, the fastest: no decoder in the loop, and in each
// family four counters and one mixer, no APMs (h264p_levels.md sections 4
// and 5); its families as IDX/l1 declares them (generated into MOD/l1),
// each wrapped into its configuration class (famcfg.inc), and the level's
// class, which Model<f_DEC, Lv1> takes.
#include "MOD/l1/nal_h.inc"
#include "MOD/l1/hdr_h.inc"
#include "MOD/l1/skp_h.inc"
#include "MOD/l1/mbt_h.inc"
#include "MOD/l1/cbp_h.inc"
#include "MOD/l1/ipm_h.inc"
#include "MOD/l1/mvs_h.inc"
#include "MOD/l1/qpd_h.inc"
#include "MOD/l1/cbf_h.inc"
#include "MOD/l1/sig_h.inc"
#include "MOD/l1/lvl_h.inc"
#include "MOD/l1/sgn_h.inc"
#include "MOD/l1/mvg_h.inc"
#include "MOD/l1/qpg_h.inc"
#include "MOD/l1/pcm_h.inc"

#include "MOD/l1/nal_p.inc"
#include "MOD/l1/hdr_p.inc"
#include "MOD/l1/skp_p.inc"
#include "MOD/l1/mbt_p.inc"
#include "MOD/l1/cbp_p.inc"
#include "MOD/l1/ipm_p.inc"
#include "MOD/l1/mvs_p.inc"
#include "MOD/l1/qpd_p.inc"
#include "MOD/l1/cbf_p.inc"
#include "MOD/l1/sig_p.inc"
#include "MOD/l1/lvl_p.inc"
#include "MOD/l1/sgn_p.inc"
#include "MOD/l1/mvg_p.inc"
#include "MOD/l1/qpg_p.inc"
#include "MOD/l1/pcm_p.inc"

#define FC_NAME C1_nal
#define FC_PFX H1_nal_
#define FC_FAM nal
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_hdr
#define FC_PFX H1_hdr_
#define FC_FAM hdr
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_skp
#define FC_PFX H1_skp_
#define FC_FAM skp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_mbt
#define FC_PFX H1_mbt_
#define FC_FAM mbt
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_cbp
#define FC_PFX H1_cbp_
#define FC_FAM cbp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_ipm
#define FC_PFX H1_ipm_
#define FC_FAM ipm
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_mvs
#define FC_PFX H1_mvs_
#define FC_FAM mvs
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_qpd
#define FC_PFX H1_qpd_
#define FC_FAM qpd
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_cbf
#define FC_PFX H1_cbf_
#define FC_FAM cbf
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_sig
#define FC_PFX H1_sig_
#define FC_FAM sig
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_lvl
#define FC_PFX H1_lvl_
#define FC_FAM lvl
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_sgn
#define FC_PFX H1_sgn_
#define FC_FAM sgn
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_mvg
#define FC_PFX H1_mvg_
#define FC_FAM mvg
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_qpg
#define FC_PFX H1_qpg_
#define FC_FAM qpg
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C1_pcm
#define FC_PFX H1_pcm_
#define FC_FAM pcm
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

struct Lv1 {
  static const int id = 1;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): none of the decoder's
  static const bool recon = false, search = false, proxy = false, signpred = false, skippx = false, deblock = false,
                    cabpred = false;
  typedef C1_nal nal;
  typedef C1_hdr hdr;
  typedef C1_skp skp;
  typedef C1_mbt mbt;
  typedef C1_cbp cbp;
  typedef C1_ipm ipm;
  typedef C1_mvs mvs;
  typedef C1_qpd qpd;
  typedef C1_cbf cbf;
  typedef C1_sig sig;
  typedef C1_lvl lvl;
  typedef C1_sgn sgn;
  typedef C1_mvg mvg;
  typedef C1_qpg qpg;
  typedef C1_pcm pcm;
};
