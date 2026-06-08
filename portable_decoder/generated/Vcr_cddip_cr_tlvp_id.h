// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_CR_TLVP_ID_H_
#define VERILATED_VCR_CDDIP_CR_TLVP_ID_H_  // guard

#include "verilated.h"


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_cr_tlvp_id final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst_n,0,0);
    VL_OUT8(tlvp_ib_rd,0,0);
    VL_OUT8(tlvp_id_out_valid,0,0);
    VL_OUT8(tlvp_error,0,0);
    VL_IN8(tlvp_ib_empty,0,0);
    VL_IN8(tlvp_ib_aempty,0,0);
    VL_IN8(pt_ib_full,0,0);
    VL_IN8(pt_ib_afull,0,0);
    VL_IN8(usr_ib_full,0,0);
    VL_IN8(usr_ib_afull,0,0);
    VL_IN8(module_id,4,0);
    CData/*0:0*/ __PVT__tlvp_id_tlv0_valid;
    CData/*1:0*/ __PVT__tlvp_id_bip2;
    CData/*0:0*/ __PVT__tlvp_id_bip2_error;
    CData/*0:0*/ __PVT__tlvp_id_bip2_ftr_error;
    CData/*0:0*/ __PVT__tlvp_id_frame;
    CData/*0:0*/ __PVT__tlvp_id_frame_error;
    CData/*0:0*/ __PVT__tlvp_id_tm_off;
    CData/*0:0*/ __PVT__tlvp_id_corrupt_data;
    CData/*0:0*/ __PVT__tlvp_id_corrupt_eot;
    CData/*0:0*/ __PVT__tlvp_id_dp_cmd_wr;
    CData/*7:0*/ __PVT__tlvp_id_dp_type;
    CData/*0:0*/ __PVT__tlvp_id_tm_count_en;
    CData/*0:0*/ __PVT__tlvp_id_errors;
    CData/*0:0*/ __PVT__tlvp_id_errors_p1;
    CData/*0:0*/ __PVT__tlvp_id_padtlv;
    CData/*0:0*/ __PVT__tlvp_id_pad_detect;
    CData/*0:0*/ __PVT__tlvp_id_trunc_detect;
    CData/*0:0*/ __VdfgTmp_h1b62dd67__0;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_data;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_eot;
    CData/*0:0*/ __Vdly__tlvp_id_padtlv;
    CData/*0:0*/ __Vdly__tlvp_id_frame;
    SData/*10:0*/ __PVT__tlvp_id_tm_count;
    IData/*18:0*/ __PVT__tlvp_id_word_num;
    IData/*18:0*/ __PVT__tlvp_id_debug_word_num;
    IData/*31:0*/ __PVT__tlvp_id_dp_debug_cmd;
    IData/*31:0*/ __PVT__tlvp_id_bp_debug_cmd;
    IData/*18:0*/ __Vdly__tlvp_id_word_num;
    IData/*31:0*/ __Vdly__tlvp_id_dp_debug_cmd;
    VL_OUTW(tlvp_id_out,105,0,4);
    VL_INW(tlvp_ib,82,0,3);
    VlWide<4>/*105:0*/ __PVT__tlvp_id_tlv0;
    VlWide<4>/*105:0*/ __PVT__tlvp_id_tlv0_save;
    QData/*63:0*/ __PVT__tlvp_id_debuq_word_msk;
    VlWide<4>/*105:0*/ __Vdly__tlvp_id_tlv0_save;

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_cr_tlvp_id(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_cr_tlvp_id();
    VL_UNCOPYABLE(Vcr_cddip_cr_tlvp_id);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
