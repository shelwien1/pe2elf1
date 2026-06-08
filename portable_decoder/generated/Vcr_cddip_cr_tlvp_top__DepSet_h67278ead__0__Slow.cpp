// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_tlvp_top.h"

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_top___ctor_var_reset(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___ctor_var_reset\n"); );
    // Body
    vlSelf->axi4s_ib_out = 0;
    vlSelf->usr_ib_empty = 0;
    vlSelf->usr_ib_aempty = 0;
    VL_ZERO_RESET_W(106, vlSelf->usr_ib_tlv);
    vlSelf->usr_ob_full = 0;
    vlSelf->usr_ob_afull = 0;
    vlSelf->tlvp_error = 0;
    VL_ZERO_RESET_W(83, vlSelf->axi4s_ob_out);
    vlSelf->clk = 0;
    vlSelf->rst_n = 0;
    VL_ZERO_RESET_W(83, vlSelf->axi4s_ib_in);
    vlSelf->tlv_parse_action = 0;
    vlSelf->module_id = 0;
    vlSelf->usr_ib_rd = 0;
    vlSelf->usr_ob_wr = 0;
    VL_ZERO_RESET_W(106, vlSelf->usr_ob_tlv);
    vlSelf->axi4s_ob_in = 0;
    vlSelf->__PVT__tlvp_ob_rd = 0;
    VL_ZERO_RESET_W(83, vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain);
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 0;
    VL_ZERO_RESET_W(83, vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata);
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv);
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen = 0;
    VL_ZERO_RESET_W(106, vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain);
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state = 0;
    VL_ZERO_RESET_W(106, vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata);
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(83, vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->__Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdlyvdim0__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(83, vlSelf->__Vdlyvval__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    vlSelf->__Vdlyvset__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
}
