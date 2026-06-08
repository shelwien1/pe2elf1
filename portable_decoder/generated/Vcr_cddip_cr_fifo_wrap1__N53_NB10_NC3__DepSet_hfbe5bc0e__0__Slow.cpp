// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3.h"

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___ctor_var_reset(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___ctor_var_reset\n"); );
    // Body
    vlSelf->full = 0;
    vlSelf->afull = 0;
    VL_ZERO_RESET_W(83, vlSelf->rdata);
    vlSelf->empty = 0;
    vlSelf->aempty = 0;
    vlSelf->clk = 0;
    vlSelf->rst_n = 0;
    VL_ZERO_RESET_W(83, vlSelf->wdata);
    vlSelf->wen = 0;
    vlSelf->ren = 0;
    vlSelf->__PVT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->__Vdly__afull_r = 0;
}
