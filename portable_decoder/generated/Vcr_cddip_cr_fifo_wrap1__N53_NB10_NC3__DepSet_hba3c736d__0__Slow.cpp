// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3.h"

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
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

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->rdata[0U] = 0U;
        vlSelf->rdata[1U] = 0U;
        vlSelf->rdata[2U] = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
    } else {
        vlSelf->rdata[0U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->rdata[1U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->rdata[2U] = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_rd) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_rd) 
                 & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
}

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
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

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__0\n"); );
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
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
        }
    }
}

VL_ATTR_COLD void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}
