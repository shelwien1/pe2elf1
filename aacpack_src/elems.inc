// The syntax elements of the walk, each a value the model codes with what
// it knows of the element (model.inc): an id per field of the syntax, and
// per count or length the walk leaves to its channel where the bitstream
// has it derived (the frame's length, a fill element's count) or coded
// otherwise (a section's escapes). X(id, name) each, the name for -v.

namespace aacpack {

#define AAC_ELEMS(X)                                                                                        \
  /* ADTS */                                                                                                \
  X(ADTS_SAME, "adts.same") X(ADTS_ID, "adts.id") X(ADTS_LAYER, "adts.layer") X(ADTS_PROT, "adts.prot")    \
  X(ADTS_PROFILE, "adts.profile") X(ADTS_SFI, "adts.sfi") X(ADTS_PRIV, "adts.priv")                         \
  X(ADTS_CHCFG, "adts.chcfg") X(ADTS_ORIG, "adts.orig") X(ADTS_HOME, "adts.home") X(ADTS_CIB, "adts.cib")   \
  X(ADTS_CIS, "adts.cis") X(ADTS_FULL, "adts.fullness") X(ADTS_NRDB, "adts.nrdb")                           \
  X(ADTS_POS_OK, "adts.pos_ok") X(ADTS_POS, "adts.pos") X(ADTS_HCRC_OK, "adts.hcrc_ok")                     \
  X(ADTS_HCRC, "adts.hcrc") X(ADTS_BREST, "adts.brest") X(ADTS_BCRC_OK, "adts.bcrc_ok")                     \
  X(ADTS_BCRC, "adts.bcrc") X(TAIL_N, "tail.n") X(TAIL, "tail")                                             \
  /* LOAS, LATM */                                                                                          \
  X(LOAS_SAME, "loas.same") X(SMC_VER, "smc.ver") X(SMC_VERA, "smc.vera") X(SMC_TARA_NB, "smc.tara_nb")    \
  X(SMC_TARA, "smc.tara") X(SMC_ASTF, "smc.astf") X(SMC_NSUB, "smc.nsub") X(SMC_NPROG, "smc.nprog")        \
  X(SMC_NLAYER, "smc.nlayer") X(ASC_NBITS, "asc.nbits") X(ASC, "asc") X(ASC_LEN_NB, "asc.len_nb")           \
  X(ASC_LEN, "asc.len") X(SMCX_FLT, "smcx.flt") X(SMCX_FULL, "smcx.full") X(SMCX_ODP, "smcx.odp")          \
  X(SMCX_OLEN_NB, "smcx.olen_nb") X(SMCX_OLEN, "smcx.olen") X(SMCX_OESC_N, "smcx.oesc_n")                   \
  X(SMCX_CRCP, "smcx.crcp") X(SMCX_CRC, "smcx.crc") X(PAYRAW_N, "payraw.n") X(PAYRAW, "payraw")            \
  X(PAYPAD_N, "paypad.n") X(PAYPAD, "paypad") X(OTHER, "other") X(LOAS_ALIGN, "loas.align")                 \
  /* ADIF, AUS1 */                                                                                          \
  X(ADIF_CIDP, "adif.cidp") X(ADIF_CID, "adif.cid") X(ADIF_ORIG, "adif.orig") X(ADIF_HOME, "adif.home")    \
  X(ADIF_BST, "adif.bst") X(ADIF_RATE, "adif.rate") X(ADIF_NPCE, "adif.npce") X(ADIF_FULL, "adif.full")    \
  X(ASC_N, "asc.n") X(AU_RAW, "au.raw")                                                                     \
  /* raw_data_block */                                                                                      \
  X(ELEM, "elem") X(REST_N, "rest.n") X(REST, "rest") X(END_ALIGN, "end.align") X(SCE_TAG, "sce.tag")      \
  X(CPE_TAG, "cpe.tag") X(CCE_TAG, "cce.tag") X(LFE_TAG, "lfe.tag") X(CPE_CW, "cpe.cw")                    \
  X(MS_PRESENT, "ms.present") X(MS_USED, "ms.used") X(ER_REST, "er.rest") X(ER_MORE, "er.more")             \
  /* individual_channel_stream */                                                                           \
  X(GLOBAL_GAIN, "ics.gg") X(ICS_RES, "ics.res") X(WIN_SEQ, "ics.ws") X(WIN_SHAPE, "ics.wshape")           \
  X(MAX_SFB, "ics.max_sfb") X(GROUPING, "ics.grouping") X(PRED_PRESENT, "pred.present")                     \
  X(PRED_RESET, "pred.reset") X(PRED_RGROUP, "pred.rgroup") X(PRED_USED, "pred.used")                       \
  X(LTP_PRESENT, "ltp.present") X(LTP_LAG, "ltp.lag") X(LTP_COEF, "ltp.coef") X(LTP_USED, "ltp.used")      \
  X(SECT_CB, "sect.cb") X(SECT_LEN, "sect.len") X(SF, "sf") X(NOISE_NRG, "sf.noise9")                      \
  X(PULSE_PRESENT, "pulse.present") X(PULSE_N, "pulse.n") X(PULSE_START, "pulse.start")                     \
  X(PULSE_OFF, "pulse.off") X(PULSE_AMP, "pulse.amp") X(TNS_PRESENT, "tns.present") X(TNS_NFILT, "tns.nf") \
  X(TNS_RES, "tns.res") X(TNS_LEN, "tns.len") X(TNS_ORDER, "tns.order") X(TNS_DIR, "tns.dir")              \
  X(TNS_COMP, "tns.comp") X(TNS_COEF, "tns.coef") X(GC_PRESENT, "gc.present") X(GC_MAXBAND, "gc.maxband")   \
  X(GC_ADJN, "gc.adjn") X(GC_ALEV, "gc.alev") X(GC_ALOC, "gc.aloc") X(SPEC, "spec")                        \
  /* CCE */                                                                                                 \
  X(CCE_INDSW, "cce.indsw") X(CCE_NCOUP, "cce.ncoup") X(CCE_ISCPE, "cce.iscpe") X(CCE_TTAG, "cce.ttag")    \
  X(CCE_LR, "cce.lr") X(CCE_DOMAIN, "cce.domain") X(CCE_GSIGN, "cce.gsign") X(CCE_GSCALE, "cce.gscale")    \
  X(CCE_CGEP, "cce.cgep") X(CCE_GAIN, "cce.gain")                                                           \
  /* DSE, PCE */                                                                                            \
  X(DSE_TAG, "dse.tag") X(DSE_ALIGNF, "dse.alignf") X(DSE_N, "dse.n") X(DSE_AB, "dse.ab")                  \
  X(DSE_DATA, "dse.data") X(PCE_TAG, "pce.tag") X(PCE_OT, "pce.ot") X(PCE_SFI, "pce.sfi")                  \
  X(PCE_NF, "pce.nf") X(PCE_NS, "pce.ns") X(PCE_NB, "pce.nb") X(PCE_NL, "pce.nl") X(PCE_NA, "pce.na")      \
  X(PCE_NC, "pce.nc") X(PCE_MONO_P, "pce.mono_p") X(PCE_MONO, "pce.mono") X(PCE_STEREO_P, "pce.stereo_p")  \
  X(PCE_STEREO, "pce.stereo") X(PCE_MATRIX_P, "pce.matrix_p") X(PCE_MATRIX, "pce.matrix")                   \
  X(PCE_PSEUDO, "pce.pseudo") X(PCE_ISCPE, "pce.iscpe") X(PCE_ETAG, "pce.etag") X(PCE_LTAG, "pce.ltag")    \
  X(PCE_ATAG, "pce.atag") X(PCE_CCIND, "pce.ccind") X(PCE_CCTAG, "pce.cctag") X(PCE_AB, "pce.ab")          \
  X(PCE_COMN, "pce.comn") X(PCE_COM, "pce.com")                                                             \
  /* FIL and its extension payloads */                                                                      \
  X(FIL_ESC, "fil.esc") X(FIL_MORE, "fil.more") X(PAY_KIND, "pay.kind") X(FILL_N, "fill.n")                \
  X(FILL_TYPE, "fill.type") X(FILL_BYTE, "fill.byte") X(EXT_TYPE, "ext.type") X(EXT_N, "ext.n")            \
  X(EXT, "ext") X(DRC_PTP, "drc.ptp") X(DRC_PTAG, "drc.ptag") X(DRC_PRES, "drc.pres")                      \
  X(DRC_EXP, "drc.exp") X(DRC_EXM, "drc.exm") X(DRC_EXA, "drc.exa") X(DRC_BP, "drc.bp")                    \
  X(DRC_BINC, "drc.binc") X(DRC_INTERP, "drc.interp") X(DRC_BTOP, "drc.btop") X(DRC_PRLP, "drc.prlp")      \
  X(DRC_PRL, "drc.prl") X(DRC_PRLR, "drc.prlr") X(DRC_SGN, "drc.sgn") X(DRC_CTL, "drc.ctl")                \
  /* SBR */                                                                                                 \
  X(SBR_CRC_OK, "sbr.crc_ok") X(SBR_CRC, "sbr.crc") X(SBR_HDR, "sbr.hdr") X(SBRH_AMP, "sbrh.amp")          \
  X(SBRH_START, "sbrh.start") X(SBRH_STOP, "sbrh.stop") X(SBRH_XOVER, "sbrh.xover")                         \
  X(SBRH_RES, "sbrh.res") X(SBRH_X1, "sbrh.x1") X(SBRH_X2, "sbrh.x2") X(SBRH_FSCALE, "sbrh.fscale")        \
  X(SBRH_ALTER, "sbrh.alter") X(SBRH_NOISEB, "sbrh.noiseb") X(SBRH_LIMB, "sbrh.limb")                       \
  X(SBRH_LIMG, "sbrh.limg") X(SBRH_INTERP, "sbrh.interp") X(SBRH_SMOOTH, "sbrh.smooth")                     \
  X(SBRD_EXTRA, "sbrd.extra") X(SBRD_RES, "sbrd.res") X(SBRD_COUP, "sbrd.coup")                             \
  X(SGRID_CLASS, "sgrid.class") X(SGRID_NENV, "sgrid.nenv") X(SGRID_FR, "sgrid.fr")                         \
  X(SGRID_ABS, "sgrid.abs") X(SGRID_NREL, "sgrid.nrel") X(SGRID_REL, "sgrid.rel")                           \
  X(SGRID_PTR, "sgrid.ptr") X(SGRID_FRB, "sgrid.frb") X(SDF_ENV, "sdf.env") X(SDF_NOISE, "sdf.noise")      \
  X(SINVF, "sinvf") X(SENV_START, "senv.start") X(SENV, "senv") X(SNOISE_START, "snoise.start")            \
  X(SNOISE, "snoise") X(SHARM_F, "sharm.f") X(SHARM, "sharm") X(SBRX_P, "sbrx.p") X(SBRX_MORE, "sbrx.more") \
  X(SBRX_ID, "sbrx.id") X(SBRXE_N, "sbrxe.n") X(SBRXE, "sbrxe") X(SBRX_FILL, "sbrx.fill")                  \
  X(SBRPAD_N, "sbrpad.n") X(SBRPAD, "sbrpad")                                                               \
  /* PS */                                                                                                  \
  X(PS_HDR, "ps.hdr") X(PS_EIID, "ps.eiid") X(PS_IIDM, "ps.iidm") X(PS_EICC, "ps.eicc")                    \
  X(PS_ICCM, "ps.iccm") X(PS_EEXT, "ps.eext") X(PS_FC, "ps.fc") X(PS_NENV, "ps.nenv")                      \
  X(PS_BORDER, "ps.border") X(PS_IID_DT, "ps.iid_dt") X(PS_IID, "ps.iid") X(PS_ICC_DT, "ps.icc_dt")        \
  X(PS_ICC, "ps.icc") X(PSX_MORE, "psx.more") X(PSX_ID, "psx.id") X(PS_EIPD, "ps.eipd")                    \
  X(PS_IPD_DT, "ps.ipd_dt") X(PS_IPD, "ps.ipd") X(PS_OPD_DT, "ps.opd_dt") X(PS_OPD, "ps.opd")              \
  X(PS_RES, "ps.res") X(PSX_FILL, "psx.fill")

#define AAC_ELEM_ID(id, name) E_##id,
enum { AAC_ELEMS(AAC_ELEM_ID) E_N };
#undef AAC_ELEM_ID
#define AAC_ELEM_NAME(id, name) name,
static const char* const kElemName[E_N] = {AAC_ELEMS(AAC_ELEM_NAME)};
#undef AAC_ELEM_NAME
static_assert(E_N <= 256, "an element id is a byte (the hdr family's tag)");

// what an extension payload is (E_PAY_KIND): parsed as DRC, SBR (without
// or with its CRC), padding (the same byte over and over), or its bits
enum { PK_DRC, PK_SBR, PK_SBRC, PK_FILL, PK_EXT, PK_N };

}  // namespace aacpack
