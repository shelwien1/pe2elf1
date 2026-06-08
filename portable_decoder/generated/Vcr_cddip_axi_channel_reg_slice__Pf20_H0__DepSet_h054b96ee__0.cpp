// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_axi_channel_reg_slice__Pf20_H0.h"

extern const VlWide<16>/*511:0*/ Vcr_cddip__ConstPool__CONST_h8b2d9f36_0;

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___act_comb__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__0(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___act_comb__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__0\n"); );
    // Body
    vlSelf->__PVT__full__DOT__c_wptr = vlSelf->__PVT__full__DOT__r_wptr;
    vlSelf->__PVT__full__DOT__c_full = vlSelf->__PVT__full__DOT__r_full;
    vlSelf->__PVT__full__DOT__c_empty = vlSelf->__PVT__full__DOT__r_empty;
    vlSelf->__PVT__full__DOT__c_rptr[0U] = vlSelf->__PVT__full__DOT__r_rptr[0U];
    vlSelf->__PVT__full__DOT__c_rptr[1U] = vlSelf->__PVT__full__DOT__r_rptr[1U];
    vlSelf->__PVT__full__DOT__c_rptr[2U] = vlSelf->__PVT__full__DOT__r_rptr[2U];
    vlSelf->__PVT__full__DOT__c_rptr[3U] = vlSelf->__PVT__full__DOT__r_rptr[3U];
    vlSelf->__PVT__full__DOT__c_rptr[4U] = vlSelf->__PVT__full__DOT__r_rptr[4U];
    vlSelf->__PVT__full__DOT__c_rptr[5U] = vlSelf->__PVT__full__DOT__r_rptr[5U];
    vlSelf->__PVT__full__DOT__c_rptr[6U] = vlSelf->__PVT__full__DOT__r_rptr[6U];
    vlSelf->__PVT__full__DOT__c_rptr[7U] = vlSelf->__PVT__full__DOT__r_rptr[7U];
    vlSelf->__PVT__full__DOT__c_rptr[8U] = vlSelf->__PVT__full__DOT__r_rptr[8U];
    vlSelf->__PVT__full__DOT__c_rptr[9U] = vlSelf->__PVT__full__DOT__r_rptr[9U];
    vlSelf->__PVT__full__DOT__c_rptr[0xaU] = vlSelf->__PVT__full__DOT__r_rptr[0xaU];
    vlSelf->__PVT__full__DOT__c_rptr[0xbU] = vlSelf->__PVT__full__DOT__r_rptr[0xbU];
    vlSelf->__PVT__full__DOT__c_rptr[0xcU] = vlSelf->__PVT__full__DOT__r_rptr[0xcU];
    vlSelf->__PVT__full__DOT__c_rptr[0xdU] = vlSelf->__PVT__full__DOT__r_rptr[0xdU];
    vlSelf->__PVT__full__DOT__c_rptr[0xeU] = vlSelf->__PVT__full__DOT__r_rptr[0xeU];
    vlSelf->__PVT__full__DOT__c_rptr[0xfU] = vlSelf->__PVT__full__DOT__r_rptr[0xfU];
    if ((1U & ((~ (IData)(vlSelf->__PVT__full__DOT__r_empty)) 
               & ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_ready) 
                  >> 2U)))) {
        if (vlSelf->__PVT__full__DOT__r_full) {
            vlSelf->__PVT__full__DOT__c_rptr[0U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[0U]));
            vlSelf->__PVT__full__DOT__c_rptr[1U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[1U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[1U]));
            vlSelf->__PVT__full__DOT__c_rptr[2U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[2U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[2U]));
            vlSelf->__PVT__full__DOT__c_rptr[3U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[3U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[3U]));
            vlSelf->__PVT__full__DOT__c_rptr[4U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[4U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[4U]));
            vlSelf->__PVT__full__DOT__c_rptr[5U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[5U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[5U]));
            vlSelf->__PVT__full__DOT__c_rptr[6U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[6U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[6U]));
            vlSelf->__PVT__full__DOT__c_rptr[7U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[7U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[7U]));
            vlSelf->__PVT__full__DOT__c_rptr[8U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[8U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[8U]));
            vlSelf->__PVT__full__DOT__c_rptr[9U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[9U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[9U]));
            vlSelf->__PVT__full__DOT__c_rptr[0xaU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xaU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xaU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xbU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xbU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xbU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xcU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xcU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xcU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xdU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xdU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xdU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xeU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xeU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xeU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xfU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xfU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xfU]));
        } else {
            vlSelf->__PVT__full__DOT__c_empty = 1U;
        }
        vlSelf->__PVT__full__DOT__c_full = 0U;
    }
    if ((((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_valid) 
          >> 2U) & (IData)(vlSelf->ready_src))) {
        vlSelf->__PVT__full__DOT__c_wptr = (1U & (~ (IData)(vlSelf->__PVT__full__DOT__r_wptr)));
        if (vlSelf->__PVT__full__DOT__c_empty) {
            vlSelf->__PVT__full__DOT__c_rptr[0U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[0U]));
            vlSelf->__PVT__full__DOT__c_rptr[1U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[1U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[1U]));
            vlSelf->__PVT__full__DOT__c_rptr[2U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[2U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[2U]));
            vlSelf->__PVT__full__DOT__c_rptr[3U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[3U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[3U]));
            vlSelf->__PVT__full__DOT__c_rptr[4U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[4U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[4U]));
            vlSelf->__PVT__full__DOT__c_rptr[5U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[5U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[5U]));
            vlSelf->__PVT__full__DOT__c_rptr[6U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[6U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[6U]));
            vlSelf->__PVT__full__DOT__c_rptr[7U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[7U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[7U]));
            vlSelf->__PVT__full__DOT__c_rptr[8U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[8U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[8U]));
            vlSelf->__PVT__full__DOT__c_rptr[9U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[9U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[9U]));
            vlSelf->__PVT__full__DOT__c_rptr[0xaU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xaU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xaU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xbU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xbU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xbU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xcU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xcU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xcU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xdU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xdU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xdU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xeU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xeU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xeU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xfU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xfU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xfU]));
        } else {
            vlSelf->__PVT__full__DOT__c_full = 1U;
        }
        vlSelf->__PVT__full__DOT__c_empty = 0U;
    }
}

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__0(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__0\n"); );
    // Init
    CData/*0:0*/ __Vdlyvdim0__full__DOT__r_payload__v0;
    __Vdlyvdim0__full__DOT__r_payload__v0 = 0;
    VlWide<121>/*3871:0*/ __Vdlyvval__full__DOT__r_payload__v0;
    VL_ZERO_W(3872, __Vdlyvval__full__DOT__r_payload__v0);
    CData/*0:0*/ __Vdlyvset__full__DOT__r_payload__v0;
    __Vdlyvset__full__DOT__r_payload__v0 = 0;
    // Body
    __Vdlyvset__full__DOT__r_payload__v0 = 0U;
    if ((((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_valid) 
          >> 2U) & (IData)(vlSelf->ready_src))) {
        VL_ASSIGN_W(3872,__Vdlyvval__full__DOT__r_payload__v0, 
                    vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_data
                    [2U]);
        __Vdlyvset__full__DOT__r_payload__v0 = 1U;
        __Vdlyvdim0__full__DOT__r_payload__v0 = vlSelf->__PVT__full__DOT__r_wptr;
    }
    if (__Vdlyvset__full__DOT__r_payload__v0) {
        VL_ASSIGN_W(3872,vlSelf->__PVT__full__DOT__r_payload
                    [__Vdlyvdim0__full__DOT__r_payload__v0], __Vdlyvval__full__DOT__r_payload__v0);
    }
}

extern const VlWide<16>/*511:0*/ Vcr_cddip__ConstPool__CONST_h218c37d6_0;

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__1(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__1\n"); );
    // Body
    vlSelf->__PVT__full__DOT__r_empty = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n))) 
                                         || (IData)(vlSelf->__PVT__full__DOT__c_empty));
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) {
        vlSelf->__PVT__full__DOT__r_enable = 1U;
        vlSelf->__PVT__full__DOT__r_rptr[0U] = vlSelf->__PVT__full__DOT__c_rptr[0U];
        vlSelf->__PVT__full__DOT__r_rptr[1U] = vlSelf->__PVT__full__DOT__c_rptr[1U];
        vlSelf->__PVT__full__DOT__r_rptr[2U] = vlSelf->__PVT__full__DOT__c_rptr[2U];
        vlSelf->__PVT__full__DOT__r_rptr[3U] = vlSelf->__PVT__full__DOT__c_rptr[3U];
        vlSelf->__PVT__full__DOT__r_rptr[4U] = vlSelf->__PVT__full__DOT__c_rptr[4U];
        vlSelf->__PVT__full__DOT__r_rptr[5U] = vlSelf->__PVT__full__DOT__c_rptr[5U];
        vlSelf->__PVT__full__DOT__r_rptr[6U] = vlSelf->__PVT__full__DOT__c_rptr[6U];
        vlSelf->__PVT__full__DOT__r_rptr[7U] = vlSelf->__PVT__full__DOT__c_rptr[7U];
        vlSelf->__PVT__full__DOT__r_rptr[8U] = vlSelf->__PVT__full__DOT__c_rptr[8U];
        vlSelf->__PVT__full__DOT__r_rptr[9U] = vlSelf->__PVT__full__DOT__c_rptr[9U];
        vlSelf->__PVT__full__DOT__r_rptr[0xaU] = vlSelf->__PVT__full__DOT__c_rptr[0xaU];
        vlSelf->__PVT__full__DOT__r_rptr[0xbU] = vlSelf->__PVT__full__DOT__c_rptr[0xbU];
        vlSelf->__PVT__full__DOT__r_rptr[0xcU] = vlSelf->__PVT__full__DOT__c_rptr[0xcU];
        vlSelf->__PVT__full__DOT__r_rptr[0xdU] = vlSelf->__PVT__full__DOT__c_rptr[0xdU];
        vlSelf->__PVT__full__DOT__r_rptr[0xeU] = vlSelf->__PVT__full__DOT__c_rptr[0xeU];
        vlSelf->__PVT__full__DOT__r_rptr[0xfU] = vlSelf->__PVT__full__DOT__c_rptr[0xfU];
    } else {
        vlSelf->__PVT__full__DOT__r_enable = 0U;
        vlSelf->__PVT__full__DOT__r_rptr[0U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0U];
        vlSelf->__PVT__full__DOT__r_rptr[1U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[1U];
        vlSelf->__PVT__full__DOT__r_rptr[2U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[2U];
        vlSelf->__PVT__full__DOT__r_rptr[3U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[3U];
        vlSelf->__PVT__full__DOT__r_rptr[4U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[4U];
        vlSelf->__PVT__full__DOT__r_rptr[5U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[5U];
        vlSelf->__PVT__full__DOT__r_rptr[6U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[6U];
        vlSelf->__PVT__full__DOT__r_rptr[7U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[7U];
        vlSelf->__PVT__full__DOT__r_rptr[8U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[8U];
        vlSelf->__PVT__full__DOT__r_rptr[9U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[9U];
        vlSelf->__PVT__full__DOT__r_rptr[0xaU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xaU];
        vlSelf->__PVT__full__DOT__r_rptr[0xbU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xbU];
        vlSelf->__PVT__full__DOT__r_rptr[0xcU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xcU];
        vlSelf->__PVT__full__DOT__r_rptr[0xdU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xdU];
        vlSelf->__PVT__full__DOT__r_rptr[0xeU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xeU];
        vlSelf->__PVT__full__DOT__r_rptr[0xfU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xfU];
    }
    vlSelf->__PVT__full__DOT__r_full = ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) 
                                        && (IData)(vlSelf->__PVT__full__DOT__c_full));
    vlSelf->__PVT__full__DOT__r_wptr = ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) 
                                        && (IData)(vlSelf->__PVT__full__DOT__c_wptr));
    vlSelf->ready_src = ((~ (IData)(vlSelf->__PVT__full__DOT__r_full)) 
                         & (IData)(vlSelf->__PVT__full__DOT__r_enable));
}

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___act_comb__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__0(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___act_comb__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__0\n"); );
    // Body
    vlSelf->__PVT__full__DOT__c_wptr = vlSelf->__PVT__full__DOT__r_wptr;
    vlSelf->__PVT__full__DOT__c_full = vlSelf->__PVT__full__DOT__r_full;
    vlSelf->__PVT__full__DOT__c_empty = vlSelf->__PVT__full__DOT__r_empty;
    vlSelf->__PVT__full__DOT__c_rptr[0U] = vlSelf->__PVT__full__DOT__r_rptr[0U];
    vlSelf->__PVT__full__DOT__c_rptr[1U] = vlSelf->__PVT__full__DOT__r_rptr[1U];
    vlSelf->__PVT__full__DOT__c_rptr[2U] = vlSelf->__PVT__full__DOT__r_rptr[2U];
    vlSelf->__PVT__full__DOT__c_rptr[3U] = vlSelf->__PVT__full__DOT__r_rptr[3U];
    vlSelf->__PVT__full__DOT__c_rptr[4U] = vlSelf->__PVT__full__DOT__r_rptr[4U];
    vlSelf->__PVT__full__DOT__c_rptr[5U] = vlSelf->__PVT__full__DOT__r_rptr[5U];
    vlSelf->__PVT__full__DOT__c_rptr[6U] = vlSelf->__PVT__full__DOT__r_rptr[6U];
    vlSelf->__PVT__full__DOT__c_rptr[7U] = vlSelf->__PVT__full__DOT__r_rptr[7U];
    vlSelf->__PVT__full__DOT__c_rptr[8U] = vlSelf->__PVT__full__DOT__r_rptr[8U];
    vlSelf->__PVT__full__DOT__c_rptr[9U] = vlSelf->__PVT__full__DOT__r_rptr[9U];
    vlSelf->__PVT__full__DOT__c_rptr[0xaU] = vlSelf->__PVT__full__DOT__r_rptr[0xaU];
    vlSelf->__PVT__full__DOT__c_rptr[0xbU] = vlSelf->__PVT__full__DOT__r_rptr[0xbU];
    vlSelf->__PVT__full__DOT__c_rptr[0xcU] = vlSelf->__PVT__full__DOT__r_rptr[0xcU];
    vlSelf->__PVT__full__DOT__c_rptr[0xdU] = vlSelf->__PVT__full__DOT__r_rptr[0xdU];
    vlSelf->__PVT__full__DOT__c_rptr[0xeU] = vlSelf->__PVT__full__DOT__r_rptr[0xeU];
    vlSelf->__PVT__full__DOT__c_rptr[0xfU] = vlSelf->__PVT__full__DOT__r_rptr[0xfU];
    if (((~ (IData)(vlSelf->__PVT__full__DOT__r_empty)) 
         & ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_ready) 
            >> 6U))) {
        if (vlSelf->__PVT__full__DOT__r_full) {
            vlSelf->__PVT__full__DOT__c_rptr[0U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[0U]));
            vlSelf->__PVT__full__DOT__c_rptr[1U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[1U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[1U]));
            vlSelf->__PVT__full__DOT__c_rptr[2U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[2U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[2U]));
            vlSelf->__PVT__full__DOT__c_rptr[3U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[3U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[3U]));
            vlSelf->__PVT__full__DOT__c_rptr[4U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[4U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[4U]));
            vlSelf->__PVT__full__DOT__c_rptr[5U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[5U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[5U]));
            vlSelf->__PVT__full__DOT__c_rptr[6U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[6U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[6U]));
            vlSelf->__PVT__full__DOT__c_rptr[7U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[7U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[7U]));
            vlSelf->__PVT__full__DOT__c_rptr[8U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[8U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[8U]));
            vlSelf->__PVT__full__DOT__c_rptr[9U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[9U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[9U]));
            vlSelf->__PVT__full__DOT__c_rptr[0xaU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xaU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xaU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xbU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xbU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xbU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xcU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xcU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xcU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xdU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xdU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xdU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xeU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xeU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xeU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xfU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xfU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xfU]));
        } else {
            vlSelf->__PVT__full__DOT__c_empty = 1U;
        }
        vlSelf->__PVT__full__DOT__c_full = 0U;
    }
    if ((((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_valid) 
          >> 6U) & (IData)(vlSelf->ready_src))) {
        vlSelf->__PVT__full__DOT__c_wptr = (1U & (~ (IData)(vlSelf->__PVT__full__DOT__r_wptr)));
        if (vlSelf->__PVT__full__DOT__c_empty) {
            vlSelf->__PVT__full__DOT__c_rptr[0U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[0U]));
            vlSelf->__PVT__full__DOT__c_rptr[1U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[1U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[1U]));
            vlSelf->__PVT__full__DOT__c_rptr[2U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[2U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[2U]));
            vlSelf->__PVT__full__DOT__c_rptr[3U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[3U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[3U]));
            vlSelf->__PVT__full__DOT__c_rptr[4U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[4U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[4U]));
            vlSelf->__PVT__full__DOT__c_rptr[5U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[5U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[5U]));
            vlSelf->__PVT__full__DOT__c_rptr[6U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[6U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[6U]));
            vlSelf->__PVT__full__DOT__c_rptr[7U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[7U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[7U]));
            vlSelf->__PVT__full__DOT__c_rptr[8U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[8U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[8U]));
            vlSelf->__PVT__full__DOT__c_rptr[9U] = 
                (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[9U] 
                 & (~ vlSelf->__PVT__full__DOT__r_rptr[9U]));
            vlSelf->__PVT__full__DOT__c_rptr[0xaU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xaU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xaU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xbU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xbU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xbU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xcU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xcU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xcU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xdU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xdU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xdU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xeU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xeU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xeU]));
            vlSelf->__PVT__full__DOT__c_rptr[0xfU] 
                = (Vcr_cddip__ConstPool__CONST_h8b2d9f36_0[0xfU] 
                   & (~ vlSelf->__PVT__full__DOT__r_rptr[0xfU]));
        } else {
            vlSelf->__PVT__full__DOT__c_full = 1U;
        }
        vlSelf->__PVT__full__DOT__c_empty = 0U;
    }
}

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__0(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__0\n"); );
    // Init
    CData/*0:0*/ __Vdlyvdim0__full__DOT__r_payload__v0;
    __Vdlyvdim0__full__DOT__r_payload__v0 = 0;
    VlWide<121>/*3871:0*/ __Vdlyvval__full__DOT__r_payload__v0;
    VL_ZERO_W(3872, __Vdlyvval__full__DOT__r_payload__v0);
    CData/*0:0*/ __Vdlyvset__full__DOT__r_payload__v0;
    __Vdlyvset__full__DOT__r_payload__v0 = 0;
    // Body
    __Vdlyvset__full__DOT__r_payload__v0 = 0U;
    if ((((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_valid) 
          >> 6U) & (IData)(vlSelf->ready_src))) {
        VL_ASSIGN_W(3872,__Vdlyvval__full__DOT__r_payload__v0, 
                    vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_data
                    [6U]);
        __Vdlyvset__full__DOT__r_payload__v0 = 1U;
        __Vdlyvdim0__full__DOT__r_payload__v0 = vlSelf->__PVT__full__DOT__r_wptr;
    }
    if (__Vdlyvset__full__DOT__r_payload__v0) {
        VL_ASSIGN_W(3872,vlSelf->__PVT__full__DOT__r_payload
                    [__Vdlyvdim0__full__DOT__r_payload__v0], __Vdlyvval__full__DOT__r_payload__v0);
    }
}

VL_INLINE_OPT void Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__1(Vcr_cddip_axi_channel_reg_slice__Pf20_H0* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_axi_channel_reg_slice__Pf20_H0___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice__1\n"); );
    // Body
    vlSelf->__PVT__full__DOT__r_enable = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n;
    vlSelf->__PVT__full__DOT__r_full = ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) 
                                        && (IData)(vlSelf->__PVT__full__DOT__c_full));
    vlSelf->__PVT__full__DOT__r_empty = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n))) 
                                         || (IData)(vlSelf->__PVT__full__DOT__c_empty));
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) {
        vlSelf->__PVT__full__DOT__r_rptr[0U] = vlSelf->__PVT__full__DOT__c_rptr[0U];
        vlSelf->__PVT__full__DOT__r_rptr[1U] = vlSelf->__PVT__full__DOT__c_rptr[1U];
        vlSelf->__PVT__full__DOT__r_rptr[2U] = vlSelf->__PVT__full__DOT__c_rptr[2U];
        vlSelf->__PVT__full__DOT__r_rptr[3U] = vlSelf->__PVT__full__DOT__c_rptr[3U];
        vlSelf->__PVT__full__DOT__r_rptr[4U] = vlSelf->__PVT__full__DOT__c_rptr[4U];
        vlSelf->__PVT__full__DOT__r_rptr[5U] = vlSelf->__PVT__full__DOT__c_rptr[5U];
        vlSelf->__PVT__full__DOT__r_rptr[6U] = vlSelf->__PVT__full__DOT__c_rptr[6U];
        vlSelf->__PVT__full__DOT__r_rptr[7U] = vlSelf->__PVT__full__DOT__c_rptr[7U];
        vlSelf->__PVT__full__DOT__r_rptr[8U] = vlSelf->__PVT__full__DOT__c_rptr[8U];
        vlSelf->__PVT__full__DOT__r_rptr[9U] = vlSelf->__PVT__full__DOT__c_rptr[9U];
        vlSelf->__PVT__full__DOT__r_rptr[0xaU] = vlSelf->__PVT__full__DOT__c_rptr[0xaU];
        vlSelf->__PVT__full__DOT__r_rptr[0xbU] = vlSelf->__PVT__full__DOT__c_rptr[0xbU];
        vlSelf->__PVT__full__DOT__r_rptr[0xcU] = vlSelf->__PVT__full__DOT__c_rptr[0xcU];
        vlSelf->__PVT__full__DOT__r_rptr[0xdU] = vlSelf->__PVT__full__DOT__c_rptr[0xdU];
        vlSelf->__PVT__full__DOT__r_rptr[0xeU] = vlSelf->__PVT__full__DOT__c_rptr[0xeU];
        vlSelf->__PVT__full__DOT__r_rptr[0xfU] = vlSelf->__PVT__full__DOT__c_rptr[0xfU];
    } else {
        vlSelf->__PVT__full__DOT__r_rptr[0U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0U];
        vlSelf->__PVT__full__DOT__r_rptr[1U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[1U];
        vlSelf->__PVT__full__DOT__r_rptr[2U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[2U];
        vlSelf->__PVT__full__DOT__r_rptr[3U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[3U];
        vlSelf->__PVT__full__DOT__r_rptr[4U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[4U];
        vlSelf->__PVT__full__DOT__r_rptr[5U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[5U];
        vlSelf->__PVT__full__DOT__r_rptr[6U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[6U];
        vlSelf->__PVT__full__DOT__r_rptr[7U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[7U];
        vlSelf->__PVT__full__DOT__r_rptr[8U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[8U];
        vlSelf->__PVT__full__DOT__r_rptr[9U] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[9U];
        vlSelf->__PVT__full__DOT__r_rptr[0xaU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xaU];
        vlSelf->__PVT__full__DOT__r_rptr[0xbU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xbU];
        vlSelf->__PVT__full__DOT__r_rptr[0xcU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xcU];
        vlSelf->__PVT__full__DOT__r_rptr[0xdU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xdU];
        vlSelf->__PVT__full__DOT__r_rptr[0xeU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xeU];
        vlSelf->__PVT__full__DOT__r_rptr[0xfU] = Vcr_cddip__ConstPool__CONST_h218c37d6_0[0xfU];
    }
    vlSelf->__PVT__full__DOT__r_wptr = ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) 
                                        && (IData)(vlSelf->__PVT__full__DOT__c_wptr));
    vlSelf->ready_src = ((~ (IData)(vlSelf->__PVT__full__DOT__r_full)) 
                         & (IData)(vlSelf->__PVT__full__DOT__r_enable));
}
