// lv5.inc -- level -5: the decoder only for the maps of the last picture,
// the textures and the edge misses, the pictures unfiltered, no CABAC
// states; in each family the ten counters and one mixer, no APMs
// (h264p_levels.md sections 4 and 6).
// Its families as IDX/l5 declares them (generated into MOD/l5), each
// wrapped into its configuration class (famcfg.inc), and the level's class,
// which Model<f_DEC, Lv5> takes.
#include "MOD/l5/nal_h.inc"
#include "MOD/l5/hdr_h.inc"
#include "MOD/l5/skp_h.inc"
#include "MOD/l5/mbt_h.inc"
#include "MOD/l5/cbp_h.inc"
#include "MOD/l5/ipm_h.inc"
#include "MOD/l5/mvs_h.inc"
#include "MOD/l5/qpd_h.inc"
#include "MOD/l5/cbf_h.inc"
#include "MOD/l5/sig_h.inc"
#include "MOD/l5/lvl_h.inc"
#include "MOD/l5/sgn_h.inc"
#include "MOD/l5/mvg_h.inc"
#include "MOD/l5/qpg_h.inc"
#include "MOD/l5/pcm_h.inc"

#include "MOD/l5/nal_p.inc"
#include "MOD/l5/hdr_p.inc"
#include "MOD/l5/skp_p.inc"
#include "MOD/l5/mbt_p.inc"
#include "MOD/l5/cbp_p.inc"
#include "MOD/l5/ipm_p.inc"
#include "MOD/l5/mvs_p.inc"
#include "MOD/l5/qpd_p.inc"
#include "MOD/l5/cbf_p.inc"
#include "MOD/l5/sig_p.inc"
#include "MOD/l5/lvl_p.inc"
#include "MOD/l5/sgn_p.inc"
#include "MOD/l5/mvg_p.inc"
#include "MOD/l5/qpg_p.inc"
#include "MOD/l5/pcm_p.inc"

#define FC_NAME C5_nal
#define FC_PFX H5_nal_
#define FC_FAM nal
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_hdr
#define FC_PFX H5_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_skp
#define FC_PFX H5_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_mbt
#define FC_PFX H5_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_cbp
#define FC_PFX H5_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_ipm
#define FC_PFX H5_ipm_
#define FC_FAM ipm
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_mvs
#define FC_PFX H5_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_qpd
#define FC_PFX H5_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_cbf
#define FC_PFX H5_cbf_
#define FC_FAM cbf
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_sig
#define FC_PFX H5_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_lvl
#define FC_PFX H5_lvl_
#define FC_FAM lvl
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_sgn
#define FC_PFX H5_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_mvg
#define FC_PFX H5_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_qpg
#define FC_PFX H5_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

#define FC_NAME C5_pcm
#define FC_PFX H5_pcm_
#define FC_FAM pcm
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "famcfg.inc"

struct Lv5 {
  static const int id = 5;  // the header's level byte
  // the model's parts (h264p_levels.md section 6): the decoder in the loop
  // (or without it the reference lists and the pictures' motion, refs), its
  // template searches, the intra proxy, sign prediction, the skip
  // prediction's pixels, deblocking (or the contexts see the pictures
  // unfiltered), the CABAC states as predictions
  static const bool refs = true, recon = true, search = false, proxy = false, signpred = false, skippx = false, deblock = false,
                    cabpred = false;
  typedef C5_nal nal;
  typedef C5_hdr hdr;
  typedef C5_skp skp;
  typedef C5_mbt mbt;
  typedef C5_cbp cbp;
  typedef C5_ipm ipm;
  typedef C5_mvs mvs;
  typedef C5_qpd qpd;
  typedef C5_cbf cbf;
  typedef C5_sig sig;
  typedef C5_lvl lvl;
  typedef C5_sgn sgn;
  typedef C5_mvg mvg;
  typedef C5_qpg qpg;
  typedef C5_pcm pcm;
};
