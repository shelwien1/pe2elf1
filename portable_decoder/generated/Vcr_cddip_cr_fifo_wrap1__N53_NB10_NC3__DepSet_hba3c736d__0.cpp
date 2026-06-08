// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3.h"

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = (IData)((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                        << 0x20U) | (QData)((IData)(
                                                    vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = (IData)(((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                         << 0x20U) | (QData)((IData)(
                                                     vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))) 
                       >> 0x20U));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = (0x40000U | ((0x20000U & (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                        << 0x11U)) 
                           | (0x1ffffU & (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                          >> 1U))));
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__1\n"); );
    // Body
    vlSelf->__Vdly__afull_r = vlSelf->__PVT__afull_r;
    vlSelf->__Vdly__afull_r = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                               && ((3U >= (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                   | (((4U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                       & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr)) 
                                      & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__2\n"); );
    // Body
    vlSelf->__PVT__afull_r = vlSelf->__Vdly__afull_r;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo__1\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) {
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
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) 
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
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_wr) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = (IData)((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                        << 0x20U) | (QData)((IData)(
                                                    vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = (IData)(((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                         << 0x20U) | (QData)((IData)(
                                                     vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))) 
                       >> 0x20U));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = (0x40000U | ((0x20000U & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                        << 0x11U)) 
                           | (0x1ffffU & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                          >> 1U))));
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__1\n"); );
    // Body
    vlSelf->__Vdly__afull_r = vlSelf->__PVT__afull_r;
    vlSelf->__Vdly__afull_r = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                               && ((3U >= (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                   | (((4U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                       & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_wr)) 
                                      & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo__2\n"); );
    // Body
    vlSelf->__PVT__afull_r = vlSelf->__Vdly__afull_r;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
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

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = (IData)((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                        << 0x20U) | (QData)((IData)(
                                                    vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = (IData)(((((QData)((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[1U])) 
                         << 0x20U) | (QData)((IData)(
                                                     vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[0U]))) 
                       >> 0x20U));
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = (0x40000U | ((0x20000U & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                        << 0x11U)) 
                           | (0x1ffffU & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr[2U] 
                                          >> 1U))));
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__1\n"); );
    // Body
    vlSelf->__Vdly__afull_r = vlSelf->__PVT__afull_r;
    vlSelf->__Vdly__afull_r = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                               && ((3U >= (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                   | (((4U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                       & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr)) 
                                      & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__2\n"); );
    // Body
    vlSelf->__PVT__afull_r = vlSelf->__Vdly__afull_r;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo__1\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_rd) {
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
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_rd) 
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
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_wr) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_ctl__ob_fifo_wdata[0U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_ctl__ob_fifo_wdata[1U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_ctl__ob_fifo_wdata[2U];
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__1\n"); );
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__2\n"); );
    // Body
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

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_comb__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo__1\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
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
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd) 
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

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___act_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___act_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_wr) {
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
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_wr) 
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
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) 
                << 0x12U) | (0x3ffffU & vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U]));
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__1\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__afull_r = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                              && ((3U >= (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                  | (((4U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                      & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen)) 
                                     & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_wr)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__2\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___act_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___act_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0\n"); );
    // Body
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_wr) {
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
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_wr) 
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
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<3>/*82:0*/ __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(83, __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen) 
         & (~ (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_datain[0U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_datain[1U];
        __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_datain[2U];
        __Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__1(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__1\n"); );
    // Body
    vlSelf->__Vdly__afull_r = vlSelf->__PVT__afull_r;
    vlSelf->__Vdly__afull_r = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                               && ((3U >= (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                   | (((4U == (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                                       & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen)) 
                                      & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_wr)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
}

VL_INLINE_OPT void Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__2(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1__2\n"); );
    // Body
    vlSelf->__PVT__afull_r = vlSelf->__Vdly__afull_r;
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
