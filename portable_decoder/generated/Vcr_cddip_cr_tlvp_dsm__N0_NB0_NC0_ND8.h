// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_CR_TLVP_DSM__N0_NB0_NC0_ND8_H_
#define VERILATED_VCR_CDDIP_CR_TLVP_DSM__N0_NB0_NC0_ND8_H_  // guard

#include "verilated.h"
class Vcr_cddip_cr_tlvp_id;


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8 final : public VerilatedModule {
  public:
    // CELLS
    Vcr_cddip_cr_tlvp_id* __PVT__u_cr_tlvp_id;

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst_n,0,0);
    VL_OUT8(tlvp_ib_rd,0,0);
    VL_OUT8(usr_ib_empty,0,0);
    VL_OUT8(usr_ib_aempty,0,0);
    VL_OUT8(pt_ib_empty,0,0);
    VL_OUT8(pt_ib_aempty,0,0);
    VL_OUT8(tlvp_error,0,0);
    VL_IN8(tlvp_ib_empty,0,0);
    VL_IN8(tlvp_ib_aempty,0,0);
    VL_IN8(module_id,4,0);
    VL_IN8(usr_ib_rd,0,0);
    VL_IN8(pt_ib_rd,0,0);
    CData/*0:0*/ __PVT__tlvp_usr_ib_wen;
    CData/*1:0*/ __PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action;
    CData/*0:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r;
    CData/*0:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    CData/*0:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty;
    CData/*0:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
    CData/*0:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full;
    CData/*3:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    CData/*3:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
    CData/*3:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    CData/*3:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    CData/*2:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    CData/*2:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
    CData/*2:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    CData/*2:0*/ __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
    SData/*12:0*/ __PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    VL_OUTW(usr_ib_tlv,105,0,4);
    VL_OUTW(pt_ib_tlv,105,0,4);
    VL_INW(tlvp_ib,82,0,3);
    VL_IN64(tlv_parse_action,63,0);
    VlWide<4>/*105:0*/ __Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata;
    VlUnpacked<VlWide<4>/*105:0*/, 8> __PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data;

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8();
    VL_UNCOPYABLE(Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
