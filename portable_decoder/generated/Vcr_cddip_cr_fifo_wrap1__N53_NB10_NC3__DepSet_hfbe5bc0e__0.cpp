// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3.h"

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0\n"); );
    // Body
    if (vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->rdata[0U] = 0U;
        vlSelf->rdata[1U] = 0U;
        vlSelf->rdata[2U] = 0U;
    } else {
        vlSelf->rdata[0U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->rdata[1U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->rdata[2U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
    }
}
