// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_CR_TLVP_TOP_H_
#define VERILATED_VCR_CDDIP_CR_TLVP_TOP_H_  // guard

#include "verilated.h"
class Vcr_cddip_cr_tlvp_dsm;


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_cr_tlvp_top final : public VerilatedModule {
  public:
    // CELLS
    Vcr_cddip_cr_tlvp_dsm* __PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm;

    // DESIGN SPECIFIC STATE
    // Anonymous structures to workaround compiler member-count bugs
    struct {
        VL_IN8(clk,0,0);
        VL_IN8(rst_n,0,0);
        VL_OUT8(axi4s_ib_out,0,0);
        VL_OUT8(usr_ib_empty,0,0);
        VL_OUT8(usr_ib_aempty,0,0);
        VL_OUT8(usr_ob_full,0,0);
        VL_OUT8(usr_ob_afull,0,0);
        VL_OUT8(tlvp_error,0,0);
        VL_IN8(module_id,4,0);
        VL_IN8(usr_ib_rd,0,0);
        VL_IN8(usr_ob_wr,0,0);
        VL_IN8(axi4s_ob_in,0,0);
        CData/*0:0*/ __PVT__tlvp_ob_rd;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        CData/*0:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full;
        CData/*4:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
        CData/*4:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        CData/*4:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        CData/*4:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        CData/*3:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
        CData/*3:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        CData/*3:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
        CData/*3:0*/ __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__pt_ob_rd;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren;
        CData/*1:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        CData/*4:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
        CData/*3:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr;
    };
    struct {
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty;
        CData/*0:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable;
        CData/*0:0*/ __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
        CData/*3:0*/ __Vdlyvdim0__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
        CData/*0:0*/ __Vdlyvset__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
        SData/*12:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern;
        SData/*12:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern;
        SData/*12:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern;
        SData/*10:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr;
        SData/*10:0*/ __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr;
        VlWide<3>/*82:0*/ __PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain;
        VlWide<3>/*82:0*/ u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata;
        IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state;
        IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state;
        VlWide<4>/*105:0*/ u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata;
        VlWide<3>/*82:0*/ __Vdlyvval__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
        VL_OUTW(usr_ib_tlv,105,0,4);
        VL_OUTW(axi4s_ob_out,82,0,3);
        VL_INW(axi4s_ib_in,82,0,3);
        VL_IN64(tlv_parse_action,63,0);
        VL_INW(usr_ob_tlv,105,0,4);
        VlWide<4>/*105:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv;
        VlWide<4>/*105:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv;
        VlWide<4>/*105:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain;
        VlWide<3>/*82:0*/ __PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out;
        VlUnpacked<VlWide<3>/*82:0*/, 16> __PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data;
        VlUnpacked<VlWide<3>/*82:0*/, 16> __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data;
        VlUnpacked<VlWide<4>/*105:0*/, 16> __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data;
        VlUnpacked<VlWide<3>/*81:0*/, 2> __PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload;
    };

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_cr_tlvp_top(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_cr_tlvp_top();
    VL_UNCOPYABLE(Vcr_cddip_cr_tlvp_top);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
