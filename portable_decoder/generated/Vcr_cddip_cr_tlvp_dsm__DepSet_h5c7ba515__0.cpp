// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_tlvp_dsm.h"

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<4>/*105:0*/ __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(106, __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<4>/*105:0*/ __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(106, __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSelf->__PVT__tlvp_usr_ib_wen) & 
         (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U];
        __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (((IData)(vlSelf->__PVT__tlvp_pt_ib_wen) & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U];
        __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][3U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U];
    }
    if (__Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][3U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_comb__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_comb__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0\n"); );
    // Body
    if (vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->pt_ib_tlv[0U] = 0U;
        vlSelf->pt_ib_tlv[1U] = 0U;
        vlSelf->pt_ib_tlv[2U] = 0U;
        vlSelf->pt_ib_tlv[3U] = 0U;
    } else {
        vlSelf->pt_ib_tlv[0U] = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->pt_ib_tlv[1U] = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->pt_ib_tlv[2U] = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
        vlSelf->pt_ib_tlv[3U] = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][3U];
    }
    if (vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->usr_ib_tlv[0U] = 0U;
        vlSelf->usr_ib_tlv[1U] = 0U;
        vlSelf->usr_ib_tlv[2U] = 0U;
        vlSelf->usr_ib_tlv[3U] = 0U;
    } else {
        vlSelf->usr_ib_tlv[0U] = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->usr_ib_tlv[1U] = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->usr_ib_tlv[2U] = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
        vlSelf->usr_ib_tlv[3U] = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][3U];
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0\n"); );
    // Init
    CData/*3:0*/ __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<4>/*105:0*/ __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(106, __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VlWide<4>/*105:0*/ __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    VL_ZERO_W(106, __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    CData/*0:0*/ __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0;
    __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    // Body
    __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0U;
    if (((IData)(vlSelf->__PVT__tlvp_pt_ib_wen) & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U];
        __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U];
        __Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (((IData)(vlSelf->__PVT__tlvp_usr_ib_wen) & 
         (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U];
        __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U] 
            = vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U];
        __Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 1U;
        __Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    }
    if (__Vdlyvset__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][3U] 
            = __Vdlyvval__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U];
    }
    if (__Vdlyvset__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][0U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[0U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][1U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[1U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][2U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[2U];
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vdlyvdim0__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0][3U] 
            = __Vdlyvval__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0[3U];
    }
}
