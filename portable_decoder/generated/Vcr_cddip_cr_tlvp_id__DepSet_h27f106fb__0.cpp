// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_id.h"

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
    // Init
    SData/*10:0*/ __Vdly__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_count = 0;
    CData/*0:0*/ __Vdly__tlvp_id_tm_off;
    __Vdly__tlvp_id_tm_off = 0;
    // Body
    vlSelf->__Vdly__tlvp_id_frame = vlSelf->__PVT__tlvp_id_frame;
    vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
    vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
    vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
    vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
    vlSelf->__Vdly__tlvp_id_corrupt_data = vlSelf->__PVT__tlvp_id_corrupt_data;
    vlSelf->__Vdly__tlvp_id_padtlv = vlSelf->__PVT__tlvp_id_padtlv;
    vlSelf->__Vdly__tlvp_id_corrupt_eot = vlSelf->__PVT__tlvp_id_corrupt_eot;
    vlSelf->__Vdly__tlvp_id_word_num = vlSelf->__PVT__tlvp_id_word_num;
    vlSelf->__Vdly__tlvp_id_dp_debug_cmd = vlSelf->__PVT__tlvp_id_dp_debug_cmd;
    __Vdly__tlvp_id_tm_count = vlSelf->__PVT__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_off = vlSelf->__PVT__tlvp_id_tm_off;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSelf->__PVT__tlvp_id_tm_count_en) {
            if ((1U >= (IData)(vlSelf->__PVT__tlvp_id_tm_count))) {
                if (((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                     & (0U < (0x7ffU & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                        >> 0xbU))))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                                   >> 0xbU));
                    __Vdly__tlvp_id_tm_off = 0U;
                } else if (((~ (IData)(vlSelf->__PVT__tlvp_id_tm_off)) 
                            & (0U < (0x7ffU & vlSelf->__PVT__tlvp_id_bp_debug_cmd)))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & vlSelf->__PVT__tlvp_id_bp_debug_cmd);
                    __Vdly__tlvp_id_tm_off = 1U;
                }
            } else {
                __Vdly__tlvp_id_tm_count = (0x7ffU 
                                            & ((IData)(vlSelf->__PVT__tlvp_id_tm_count) 
                                               - (IData)(1U)));
            }
        } else {
            __Vdly__tlvp_id_tm_count = 0U;
            __Vdly__tlvp_id_tm_off = 0U;
        }
        vlSelf->tlvp_error = ((IData)(vlSelf->__PVT__tlvp_id_errors) 
                              & (~ (IData)(vlSelf->__PVT__tlvp_id_errors_p1)));
        vlSelf->__PVT__tlvp_id_errors_p1 = vlSelf->__PVT__tlvp_id_errors;
        if ((1U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                          >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 1U;
        } else if ((2U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                 >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
        }
    } else {
        __Vdly__tlvp_id_tm_count = 0U;
        __Vdly__tlvp_id_tm_off = 0U;
        vlSelf->__PVT__tlvp_id_errors_p1 = 0U;
        vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
    }
    vlSelf->__PVT__tlvp_id_tm_count = __Vdly__tlvp_id_tm_count;
    vlSelf->__PVT__tlvp_id_tm_off = __Vdly__tlvp_id_tm_off;
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__0__Vfuncout;
    __Vfunc_get_bip2__0__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__0__data_in;
    __Vfunc_get_bip2__0__data_in = 0;
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U])) {
            vlSelf->__Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            vlSelf->__Vdly__tlvp_id_word_num = (0x7ffffU 
                                                & ((IData)(1U) 
                                                   + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x80000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                vlSelf->__Vdly__tlvp_id_tlv0_save[0U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[0U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[1U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[1U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[2U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[2U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[3U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                vlSelf->__Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                vlSelf->__Vdly__tlvp_id_word_num = 
                    (0x7ffffU & ((IData)(1U) + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.rdata[2U]));
    } else {
        vlSelf->__Vdly__tlvp_id_frame = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
        vlSelf->__Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        vlSelf->__Vdly__tlvp_id_word_num = 0U;
        vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_corrupt_data = vlSelf->__Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = vlSelf->__Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = vlSelf->__Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = vlSelf->__Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = vlSelf->__Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = vlSelf->__Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = vlSelf->__Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = vlSelf->__Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = vlSelf->__Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = vlSelf->__Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x80000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__0__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__0__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__0__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__0__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__0__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_comb__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
    // Body
    vlSelf->tlvp_ib_rd = (1U & (~ (((4U == (7U & vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config))
                                     ? ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trigger_hit_nxt) 
                                        | (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty))
                                     : ((3U == (7U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config))
                                         ? ((~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__single_step_rd)) 
                                            | (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty))
                                         : (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty))) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                      | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                            | (IData)(vlSelf->__PVT__tlvp_id_padtlv)))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__1__Vfuncout;
    __Vfunc_get_bip2__1__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__1__data_in;
    __Vfunc_get_bip2__1__data_in = 0;
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            vlSelf->__Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            vlSelf->__Vdly__tlvp_id_word_num = (0x7ffffU 
                                                & ((IData)(1U) 
                                                   + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x81000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                vlSelf->__Vdly__tlvp_id_tlv0_save[0U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[0U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[1U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[1U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[2U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[2U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[3U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                vlSelf->__Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                vlSelf->__Vdly__tlvp_id_word_num = 
                    (0x7ffffU & ((IData)(1U) + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        vlSelf->__Vdly__tlvp_id_frame = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
        vlSelf->__Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        vlSelf->__Vdly__tlvp_id_word_num = 0U;
        vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_corrupt_data = vlSelf->__Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = vlSelf->__Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = vlSelf->__Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = vlSelf->__Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = vlSelf->__Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = vlSelf->__Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = vlSelf->__Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = vlSelf->__Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = vlSelf->__Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = vlSelf->__Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x81000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__1__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__1__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__1__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__1__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__1__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__2__Vfuncout;
    __Vfunc_get_bip2__2__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__2__data_in;
    __Vfunc_get_bip2__2__data_in = 0;
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            vlSelf->__Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            vlSelf->__Vdly__tlvp_id_word_num = (0x7ffffU 
                                                & ((IData)(1U) 
                                                   + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x82000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                vlSelf->__Vdly__tlvp_id_tlv0_save[0U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[0U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[1U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[1U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[2U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[2U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[3U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                vlSelf->__Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                vlSelf->__Vdly__tlvp_id_word_num = 
                    (0x7ffffU & ((IData)(1U) + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        vlSelf->__Vdly__tlvp_id_frame = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
        vlSelf->__Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        vlSelf->__Vdly__tlvp_id_word_num = 0U;
        vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_corrupt_data = vlSelf->__Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = vlSelf->__Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = vlSelf->__Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = vlSelf->__Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = vlSelf->__Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = vlSelf->__Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = vlSelf->__Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = vlSelf->__Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = vlSelf->__Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = vlSelf->__Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x82000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__2__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__2__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__2__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__2__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__2__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id__0\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__3__Vfuncout;
    __Vfunc_get_bip2__3__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__3__data_in;
    __Vfunc_get_bip2__3__data_in = 0;
    SData/*10:0*/ __Vdly__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_count = 0;
    CData/*0:0*/ __Vdly__tlvp_id_tm_off;
    __Vdly__tlvp_id_tm_off = 0;
    IData/*18:0*/ __Vdly__tlvp_id_word_num;
    __Vdly__tlvp_id_word_num = 0;
    IData/*31:0*/ __Vdly__tlvp_id_dp_debug_cmd;
    __Vdly__tlvp_id_dp_debug_cmd = 0;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_data;
    __Vdly__tlvp_id_corrupt_data = 0;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_eot;
    __Vdly__tlvp_id_corrupt_eot = 0;
    VlWide<4>/*105:0*/ __Vdly__tlvp_id_tlv0_save;
    VL_ZERO_W(106, __Vdly__tlvp_id_tlv0_save);
    CData/*0:0*/ __Vdly__tlvp_id_padtlv;
    __Vdly__tlvp_id_padtlv = 0;
    CData/*0:0*/ __Vdly__tlvp_id_frame;
    __Vdly__tlvp_id_frame = 0;
    // Body
    __Vdly__tlvp_id_frame = vlSelf->__PVT__tlvp_id_frame;
    __Vdly__tlvp_id_tlv0_save[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
    __Vdly__tlvp_id_tlv0_save[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
    __Vdly__tlvp_id_tlv0_save[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
    __Vdly__tlvp_id_tlv0_save[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
    __Vdly__tlvp_id_corrupt_data = vlSelf->__PVT__tlvp_id_corrupt_data;
    __Vdly__tlvp_id_padtlv = vlSelf->__PVT__tlvp_id_padtlv;
    __Vdly__tlvp_id_corrupt_eot = vlSelf->__PVT__tlvp_id_corrupt_eot;
    __Vdly__tlvp_id_word_num = vlSelf->__PVT__tlvp_id_word_num;
    __Vdly__tlvp_id_dp_debug_cmd = vlSelf->__PVT__tlvp_id_dp_debug_cmd;
    __Vdly__tlvp_id_tm_count = vlSelf->__PVT__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_off = vlSelf->__PVT__tlvp_id_tm_off;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) {
        if (vlSelf->__PVT__tlvp_id_tm_count_en) {
            if ((1U >= (IData)(vlSelf->__PVT__tlvp_id_tm_count))) {
                if (((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                     & (0U < (0x7ffU & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                        >> 0xbU))))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                                   >> 0xbU));
                    __Vdly__tlvp_id_tm_off = 0U;
                } else if (((~ (IData)(vlSelf->__PVT__tlvp_id_tm_off)) 
                            & (0U < (0x7ffU & vlSelf->__PVT__tlvp_id_bp_debug_cmd)))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & vlSelf->__PVT__tlvp_id_bp_debug_cmd);
                    __Vdly__tlvp_id_tm_off = 1U;
                }
            } else {
                __Vdly__tlvp_id_tm_count = (0x7ffU 
                                            & ((IData)(vlSelf->__PVT__tlvp_id_tm_count) 
                                               - (IData)(1U)));
            }
        } else {
            __Vdly__tlvp_id_tm_count = 0U;
            __Vdly__tlvp_id_tm_off = 0U;
        }
        vlSelf->tlvp_error = ((IData)(vlSelf->__PVT__tlvp_id_errors) 
                              & (~ (IData)(vlSelf->__PVT__tlvp_id_errors_p1)));
        vlSelf->__PVT__tlvp_id_errors_p1 = vlSelf->__PVT__tlvp_id_errors;
        if ((1U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                          >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 1U;
        } else if ((2U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                 >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
        }
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            __Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            __Vdly__tlvp_id_word_num = (0x7ffffU & 
                                        ((IData)(1U) 
                                         + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x84000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        __Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    __Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    __Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                __Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                __Vdly__tlvp_id_tlv0_save[0U] = vlSelf->__PVT__tlvp_id_tlv0[0U];
                __Vdly__tlvp_id_tlv0_save[1U] = vlSelf->__PVT__tlvp_id_tlv0[1U];
                __Vdly__tlvp_id_tlv0_save[2U] = vlSelf->__PVT__tlvp_id_tlv0[2U];
                __Vdly__tlvp_id_tlv0_save[3U] = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                __Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    __Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    __Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                __Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                __Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                __Vdly__tlvp_id_word_num = (0x7ffffU 
                                            & ((IData)(1U) 
                                               + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        __Vdly__tlvp_id_tm_count = 0U;
        __Vdly__tlvp_id_tm_off = 0U;
        vlSelf->__PVT__tlvp_id_errors_p1 = 0U;
        vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
        __Vdly__tlvp_id_frame = 0U;
        __Vdly__tlvp_id_corrupt_data = 0U;
        __Vdly__tlvp_id_corrupt_eot = 0U;
        __Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        __Vdly__tlvp_id_word_num = 0U;
        __Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        __Vdly__tlvp_id_tlv0_save[0U] = 0U;
        __Vdly__tlvp_id_tlv0_save[1U] = 0U;
        __Vdly__tlvp_id_tlv0_save[2U] = 0U;
        __Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_tm_count = __Vdly__tlvp_id_tm_count;
    vlSelf->__PVT__tlvp_id_tm_off = __Vdly__tlvp_id_tm_off;
    vlSelf->__PVT__tlvp_id_corrupt_data = __Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = __Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = __Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = __Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = __Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = __Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = __Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = __Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = __Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = __Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x84000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__3__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__3__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__3__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__3__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__3__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__4__Vfuncout;
    __Vfunc_get_bip2__4__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__4__data_in;
    __Vfunc_get_bip2__4__data_in = 0;
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            vlSelf->__Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            vlSelf->__Vdly__tlvp_id_word_num = (0x7ffffU 
                                                & ((IData)(1U) 
                                                   + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x86000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                vlSelf->__Vdly__tlvp_id_tlv0_save[0U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[0U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[1U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[1U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[2U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[2U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[3U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                vlSelf->__Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                vlSelf->__Vdly__tlvp_id_word_num = 
                    (0x7ffffU & ((IData)(1U) + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        vlSelf->__Vdly__tlvp_id_frame = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
        vlSelf->__Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        vlSelf->__Vdly__tlvp_id_word_num = 0U;
        vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_corrupt_data = vlSelf->__Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = vlSelf->__Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = vlSelf->__Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = vlSelf->__Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = vlSelf->__Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = vlSelf->__Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = vlSelf->__Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = vlSelf->__Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = vlSelf->__Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = vlSelf->__Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x86000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__4__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__4__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__4__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__4__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__4__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__2(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__2\n"); );
    // Body
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__1\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__5__Vfuncout;
    __Vfunc_get_bip2__5__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__5__data_in;
    __Vfunc_get_bip2__5__data_in = 0;
    // Body
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            vlSelf->__Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            vlSelf->__Vdly__tlvp_id_word_num = (0x7ffffU 
                                                & ((IData)(1U) 
                                                   + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x87000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        vlSelf->__Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                vlSelf->__Vdly__tlvp_id_tlv0_save[0U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[0U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[1U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[1U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[2U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[2U];
                vlSelf->__Vdly__tlvp_id_tlv0_save[3U] 
                    = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    vlSelf->__Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                vlSelf->__Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                vlSelf->__Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                vlSelf->__Vdly__tlvp_id_word_num = 
                    (0x7ffffU & ((IData)(1U) + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        vlSelf->__Vdly__tlvp_id_frame = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_data = 0U;
        vlSelf->__Vdly__tlvp_id_corrupt_eot = 0U;
        vlSelf->__Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        vlSelf->__Vdly__tlvp_id_word_num = 0U;
        vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[0U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[1U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[2U] = 0U;
        vlSelf->__Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_corrupt_data = vlSelf->__Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = vlSelf->__Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = vlSelf->__Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = vlSelf->__Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = vlSelf->__Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = vlSelf->__Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = vlSelf->__Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = vlSelf->__Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = vlSelf->__Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = vlSelf->__Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x87000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__5__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__5__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__5__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__5__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__5__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                      | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                            | (IData)(vlSelf->__PVT__tlvp_id_padtlv)))))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___nba_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id__0\n"); );
    // Init
    IData/*31:0*/ __PVT__get_bip2__Vstatic__evn;
    __PVT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__get_bip2__Vstatic__odd;
    __PVT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__get_bip2__Vstatic__par;
    __PVT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __VdfgTmp_hcd0dc4ac__0;
    __VdfgTmp_hcd0dc4ac__0 = 0;
    CData/*1:0*/ __Vfunc_get_bip2__6__Vfuncout;
    __Vfunc_get_bip2__6__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_get_bip2__6__data_in;
    __Vfunc_get_bip2__6__data_in = 0;
    SData/*10:0*/ __Vdly__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_count = 0;
    CData/*0:0*/ __Vdly__tlvp_id_tm_off;
    __Vdly__tlvp_id_tm_off = 0;
    IData/*18:0*/ __Vdly__tlvp_id_word_num;
    __Vdly__tlvp_id_word_num = 0;
    IData/*31:0*/ __Vdly__tlvp_id_dp_debug_cmd;
    __Vdly__tlvp_id_dp_debug_cmd = 0;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_data;
    __Vdly__tlvp_id_corrupt_data = 0;
    CData/*0:0*/ __Vdly__tlvp_id_corrupt_eot;
    __Vdly__tlvp_id_corrupt_eot = 0;
    VlWide<4>/*105:0*/ __Vdly__tlvp_id_tlv0_save;
    VL_ZERO_W(106, __Vdly__tlvp_id_tlv0_save);
    CData/*0:0*/ __Vdly__tlvp_id_padtlv;
    __Vdly__tlvp_id_padtlv = 0;
    CData/*0:0*/ __Vdly__tlvp_id_frame;
    __Vdly__tlvp_id_frame = 0;
    // Body
    __Vdly__tlvp_id_frame = vlSelf->__PVT__tlvp_id_frame;
    __Vdly__tlvp_id_tlv0_save[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
    __Vdly__tlvp_id_tlv0_save[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
    __Vdly__tlvp_id_tlv0_save[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
    __Vdly__tlvp_id_tlv0_save[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
    __Vdly__tlvp_id_corrupt_data = vlSelf->__PVT__tlvp_id_corrupt_data;
    __Vdly__tlvp_id_padtlv = vlSelf->__PVT__tlvp_id_padtlv;
    __Vdly__tlvp_id_corrupt_eot = vlSelf->__PVT__tlvp_id_corrupt_eot;
    __Vdly__tlvp_id_word_num = vlSelf->__PVT__tlvp_id_word_num;
    __Vdly__tlvp_id_dp_debug_cmd = vlSelf->__PVT__tlvp_id_dp_debug_cmd;
    __Vdly__tlvp_id_tm_count = vlSelf->__PVT__tlvp_id_tm_count;
    __Vdly__tlvp_id_tm_off = vlSelf->__PVT__tlvp_id_tm_off;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSelf->__PVT__tlvp_id_tm_count_en) {
            if ((1U >= (IData)(vlSelf->__PVT__tlvp_id_tm_count))) {
                if (((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                     & (0U < (0x7ffU & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                        >> 0xbU))))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                                   >> 0xbU));
                    __Vdly__tlvp_id_tm_off = 0U;
                } else if (((~ (IData)(vlSelf->__PVT__tlvp_id_tm_off)) 
                            & (0U < (0x7ffU & vlSelf->__PVT__tlvp_id_bp_debug_cmd)))) {
                    __Vdly__tlvp_id_tm_count = (0x7ffU 
                                                & vlSelf->__PVT__tlvp_id_bp_debug_cmd);
                    __Vdly__tlvp_id_tm_off = 1U;
                }
            } else {
                __Vdly__tlvp_id_tm_count = (0x7ffU 
                                            & ((IData)(vlSelf->__PVT__tlvp_id_tm_count) 
                                               - (IData)(1U)));
            }
        } else {
            __Vdly__tlvp_id_tm_count = 0U;
            __Vdly__tlvp_id_tm_off = 0U;
        }
        vlSelf->tlvp_error = ((IData)(vlSelf->__PVT__tlvp_id_errors) 
                              & (~ (IData)(vlSelf->__PVT__tlvp_id_errors_p1)));
        vlSelf->__PVT__tlvp_id_errors_p1 = vlSelf->__PVT__tlvp_id_errors;
        if ((1U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                          >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 1U;
        } else if ((2U == (3U & (vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                                 >> 0x1dU)))) {
            vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
        }
        vlSelf->__PVT__tlvp_id_errors = ((IData)(vlSelf->__PVT__tlvp_id_frame_error) 
                                         | (IData)(vlSelf->__PVT__tlvp_id_bip2_error));
        vlSelf->tlvp_id_out[2U] = (IData)((0x3ffffffffffULL 
                                           & (((QData)((IData)(
                                                               vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                               << 0x20U) 
                                              | (QData)((IData)(
                                                                vlSelf->__PVT__tlvp_id_tlv0[2U])))));
        vlSelf->tlvp_id_out[3U] = (0x3ffU & (IData)(
                                                    ((0x3ffffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->__PVT__tlvp_id_tlv0[3U])) 
                                                          << 0x20U) 
                                                         | (QData)((IData)(
                                                                           vlSelf->__PVT__tlvp_id_tlv0[2U])))) 
                                                     >> 0x20U)));
        if (((IData)(vlSelf->tlvp_ib_rd) & vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])) {
            __Vdly__tlvp_id_word_num = 0U;
        } else if (vlSelf->tlvp_ib_rd) {
            __Vdly__tlvp_id_word_num = (0x7ffffU & 
                                        ((IData)(1U) 
                                         + vlSelf->__PVT__tlvp_id_word_num));
        }
        if (vlSelf->__PVT__tlvp_id_tlv0_valid) {
            if ((1U == (0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                 >> 0x14U)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   ((QData)((IData)(
                                                                    vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[0U]))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   >> 0x20U));
                if ((1U == vlSelf->__PVT__tlvp_id_word_num)) {
                    if ((IData)((0x88000000U == (0x9f800000U 
                                                 & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_dp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__VdfgTmp_h1b62dd67__0) 
                         & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                            >> 0x17U))) {
                        vlSelf->__PVT__tlvp_id_bp_debug_cmd 
                            = vlSelf->__PVT__tlvp_id_tlv0[1U];
                    }
                    if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        __Vdly__tlvp_id_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_corrupt_data = 0U;
                    } else if (((IData)(vlSelf->__PVT__tlvp_id_dp_cmd_wr) 
                                & (0x60000000U == (0x60000000U 
                                                   & vlSelf->__PVT__tlvp_id_tlv0[1U])))) {
                        __Vdly__tlvp_id_corrupt_eot = 1U;
                    }
                }
            } else {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (6U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   ((((0x15U 
                                                       == vlSelf->__PVT__tlvp_id_word_num) 
                                                      & (0U 
                                                         == 
                                                         (0xffffU 
                                                          & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                     & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                     ? 
                                                    (0xffULL 
                                                     | (0xffffffffffff00ULL 
                                                        & (((QData)((IData)(
                                                                            vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                            << 0x18U) 
                                                           | (0xffffffffffff00ULL 
                                                              & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                 >> 8U)))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                    : 
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U])))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    ((((0x15U 
                                                        == vlSelf->__PVT__tlvp_id_word_num) 
                                                       & (0U 
                                                          == 
                                                          (0xffffU 
                                                           & vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                      & (IData)(vlSelf->__PVT__tlvp_id_bip2_ftr_error))
                                                      ? 
                                                     (0xffULL 
                                                      | (0xffffffffffff00ULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                             << 0x18U) 
                                                            | (0xffffffffffff00ULL 
                                                               & ((QData)((IData)(
                                                                                vlSelf->__PVT__tlvp_id_tlv0[0U])) 
                                                                  >> 8U)))))
                                                      : 
                                                     (((QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                       << 0x20U) 
                                                      | (QData)((IData)(
                                                                        vlSelf->__PVT__tlvp_id_tlv0[0U]))))
                                                     : 
                                                    (((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U])))) 
                                                   >> 0x20U));
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                  & (vlSelf->__PVT__tlvp_id_debug_word_num 
                     == vlSelf->__PVT__tlvp_id_word_num)) 
                 & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ vlSelf->__PVT__tlvp_id_debuq_word_msk));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ vlSelf->__PVT__tlvp_id_debuq_word_msk) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    __Vdly__tlvp_id_corrupt_data = 0U;
                }
            } else if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_data) 
                         & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                      >> 0x14U)) == (IData)(vlSelf->__PVT__tlvp_id_dp_type))) 
                        & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                           >> 0x12U))) {
                vlSelf->tlvp_id_out[0U] = (IData)((
                                                   (((QData)((IData)(
                                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                     << 0x20U) 
                                                    | (QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                   ^ (QData)((IData)(
                                                                     (0xffU 
                                                                      & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))));
                vlSelf->tlvp_id_out[1U] = (IData)((
                                                   ((((QData)((IData)(
                                                                      vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                                      << 0x20U) 
                                                     | (QData)((IData)(
                                                                       vlSelf->__PVT__tlvp_id_tlv0[0U]))) 
                                                    ^ (QData)((IData)(
                                                                      (0xffU 
                                                                       & vlSelf->__PVT__tlvp_id_dp_debug_cmd)))) 
                                                   >> 0x20U));
                if ((0U == (3U & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    __Vdly__tlvp_id_corrupt_data = 0U;
                }
            }
            if ((((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                  & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                     >> 0x12U)) & ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x14U)) 
                                   == (IData)(vlSelf->__PVT__tlvp_id_dp_type)))) {
                __Vdly__tlvp_id_corrupt_eot = 0U;
            }
            if (vlSelf->__PVT__tlvp_id_pad_detect) {
                __Vdly__tlvp_id_tlv0_save[0U] = vlSelf->__PVT__tlvp_id_tlv0[0U];
                __Vdly__tlvp_id_tlv0_save[1U] = vlSelf->__PVT__tlvp_id_tlv0[1U];
                __Vdly__tlvp_id_tlv0_save[2U] = vlSelf->__PVT__tlvp_id_tlv0[2U];
                __Vdly__tlvp_id_tlv0_save[3U] = vlSelf->__PVT__tlvp_id_tlv0[3U];
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                __Vdly__tlvp_id_padtlv = 1U;
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
            if ((2U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = vlSelf->__PVT__tlvp_id_frame;
                    __Vdly__tlvp_id_frame = 0U;
                } else {
                    vlSelf->__PVT__tlvp_id_frame_error 
                        = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
                    __Vdly__tlvp_id_frame = 0U;
                }
            } else if ((1U & vlSelf->__PVT__tlvp_id_tlv0[2U])) {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = vlSelf->__PVT__tlvp_id_frame;
                __Vdly__tlvp_id_frame = 1U;
            } else {
                vlSelf->__PVT__tlvp_id_frame_error 
                    = (1U & (~ (IData)(vlSelf->__PVT__tlvp_id_frame)));
            }
        }
        if (vlSelf->__PVT__tlvp_id_padtlv) {
            vlSelf->tlvp_id_out[0U] = vlSelf->__PVT__tlvp_id_tlv0_save[0U];
            vlSelf->tlvp_id_out[1U] = vlSelf->__PVT__tlvp_id_tlv0_save[1U];
            vlSelf->tlvp_id_out[2U] = vlSelf->__PVT__tlvp_id_tlv0_save[2U];
            vlSelf->tlvp_id_out[3U] = vlSelf->__PVT__tlvp_id_tlv0_save[3U];
            vlSelf->tlvp_id_out[0U] = 0U;
            vlSelf->tlvp_id_out[1U] = 0U;
            if ((vlSelf->__PVT__tlvp_id_debug_word_num 
                 == vlSelf->__PVT__tlvp_id_word_num)) {
                vlSelf->tlvp_id_out[2U] = (0x40000U 
                                           | vlSelf->tlvp_id_out[2U]);
                __Vdly__tlvp_id_padtlv = 0U;
                vlSelf->tlvp_id_out[2U] = (2U | vlSelf->tlvp_id_out[2U]);
            } else {
                __Vdly__tlvp_id_word_num = (0x7ffffU 
                                            & ((IData)(1U) 
                                               + vlSelf->__PVT__tlvp_id_word_num));
                vlSelf->tlvp_id_out[2U] = (0xfff9ffffU 
                                           & vlSelf->tlvp_id_out[2U]);
                vlSelf->tlvp_id_out[2U] = (0xfffffffdU 
                                           & vlSelf->tlvp_id_out[2U]);
            }
        }
        if (((vlSelf->__PVT__tlvp_id_tlv0[2U] & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
             & (~ (IData)(vlSelf->__PVT__tlvp_id_bip2_error)))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 
                (0U != (IData)(vlSelf->__PVT__tlvp_id_bip2));
        } else if (((IData)(vlSelf->__PVT__tlvp_id_tlv0_valid) 
                    & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                       >> 0x11U))) {
            vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        }
        vlSelf->tlvp_id_out_valid = ((IData)(vlSelf->__PVT__tlvp_id_padtlv) 
                                     || ((1U & (~ (IData)(vlSelf->__PVT__tlvp_id_trunc_detect))) 
                                         && (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)));
        vlSelf->__PVT__tlvp_id_bip2_error = ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                              & (IData)(vlSelf->__PVT__tlvp_id_tlv0_valid)) 
                                             && (0U 
                                                 != (IData)(vlSelf->__PVT__tlvp_id_bip2)));
        if ((vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
             & (IData)(vlSelf->tlvp_ib_rd))) {
            vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xf00fffffU 
                                                & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                               | (0xff00000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
                                                     << 0x14U)));
        }
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfff1ffffU 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0xfffe0000U 
                                              & ((0x80000U 
                                                  & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                     << 0x13U)) 
                                                 | ((0x40000U 
                                                     & (vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
                                                        << 0x11U)) 
                                                    | (0x20000U 
                                                       & vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U])))));
        vlSelf->__PVT__tlvp_id_tlv0[0U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U];
        vlSelf->__PVT__tlvp_id_tlv0[1U] = vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U];
        vlSelf->__PVT__tlvp_id_tlv0[2U] = ((0xfffe0000U 
                                            & vlSelf->__PVT__tlvp_id_tlv0[2U]) 
                                           | (0x1ffffU 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U]));
    } else {
        __Vdly__tlvp_id_tm_count = 0U;
        __Vdly__tlvp_id_tm_off = 0U;
        vlSelf->__PVT__tlvp_id_errors_p1 = 0U;
        vlSelf->__PVT__tlvp_id_tm_count_en = 0U;
        __Vdly__tlvp_id_frame = 0U;
        __Vdly__tlvp_id_corrupt_data = 0U;
        __Vdly__tlvp_id_corrupt_eot = 0U;
        __Vdly__tlvp_id_padtlv = 0U;
        vlSelf->tlvp_id_out[0U] = 0U;
        vlSelf->tlvp_id_out[1U] = 0U;
        vlSelf->tlvp_id_out[2U] = 0U;
        vlSelf->tlvp_id_out[3U] = 0U;
        vlSelf->tlvp_id_out_valid = 0U;
        vlSelf->__PVT__tlvp_id_bip2_error = 0U;
        vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0U;
        vlSelf->__PVT__tlvp_id_frame_error = 0U;
        vlSelf->__PVT__tlvp_id_errors = 0U;
        __Vdly__tlvp_id_word_num = 0U;
        __Vdly__tlvp_id_dp_debug_cmd = 0U;
        vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0U;
        __Vdly__tlvp_id_tlv0_save[0U] = 0U;
        __Vdly__tlvp_id_tlv0_save[1U] = 0U;
        __Vdly__tlvp_id_tlv0_save[2U] = 0U;
        __Vdly__tlvp_id_tlv0_save[3U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[0U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[1U] = 0U;
        vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xf0000000U 
                                           & vlSelf->__PVT__tlvp_id_tlv0[2U]);
    }
    vlSelf->__PVT__tlvp_id_tm_count = __Vdly__tlvp_id_tm_count;
    vlSelf->__PVT__tlvp_id_tm_off = __Vdly__tlvp_id_tm_off;
    vlSelf->__PVT__tlvp_id_corrupt_data = __Vdly__tlvp_id_corrupt_data;
    vlSelf->__PVT__tlvp_id_tlv0_save[0U] = __Vdly__tlvp_id_tlv0_save[0U];
    vlSelf->__PVT__tlvp_id_tlv0_save[1U] = __Vdly__tlvp_id_tlv0_save[1U];
    vlSelf->__PVT__tlvp_id_tlv0_save[2U] = __Vdly__tlvp_id_tlv0_save[2U];
    vlSelf->__PVT__tlvp_id_tlv0_save[3U] = __Vdly__tlvp_id_tlv0_save[3U];
    vlSelf->__PVT__tlvp_id_frame = __Vdly__tlvp_id_frame;
    vlSelf->__PVT__tlvp_id_padtlv = __Vdly__tlvp_id_padtlv;
    vlSelf->__PVT__tlvp_id_word_num = __Vdly__tlvp_id_word_num;
    vlSelf->__PVT__tlvp_id_corrupt_eot = __Vdly__tlvp_id_corrupt_eot;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = __Vdly__tlvp_id_dp_debug_cmd;
    vlSelf->__PVT__tlvp_id_debug_word_num = (0x3ffU 
                                             & VL_SHIFTR_III(10,10,32, 
                                                             (0x3ffU 
                                                              & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                 >> 8U)), 3U));
    vlSelf->__PVT__tlvp_id_dp_type = (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                               >> 0x12U));
    vlSelf->__PVT__tlvp_id_debuq_word_msk = ((QData)((IData)(
                                                             (0xffU 
                                                              & vlSelf->__PVT__tlvp_id_dp_debug_cmd))) 
                                             << (0x38U 
                                                 & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                    >> 5U)));
    vlSelf->__PVT__tlvp_id_tlv0_valid = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                         && (IData)(vlSelf->tlvp_ib_rd));
    vlSelf->__VdfgTmp_h1b62dd67__0 = (IData)((0x88000000U 
                                              == (0x9f000000U 
                                                  & vlSelf->__PVT__tlvp_id_tlv0[1U])));
    __Vfunc_get_bip2__6__data_in = (((QData)((IData)(
                                                     vlSelf->__PVT__tlvp_id_tlv0[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->__PVT__tlvp_id_tlv0[0U])));
    __PVT__get_bip2__Vstatic__evn = 0U;
    __PVT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_get_bip2__6__data_in))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 2U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 4U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 6U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 8U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xaU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xcU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xeU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x10U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x12U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x14U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x16U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x18U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x20U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x22U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x24U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x26U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x28U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x30U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x32U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x34U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x36U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x38U)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3aU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3cU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3eU)))) {
        __PVT__get_bip2__Vstatic__evn = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 1U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 3U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 5U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 7U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 9U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xbU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xdU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0xfU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x11U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x13U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x15U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x17U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x19U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x1fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x21U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x23U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x25U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x27U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x29U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x2fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x31U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x33U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x35U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x37U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x39U)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3bU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3dU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_get_bip2__6__data_in 
                       >> 0x3fU)))) {
        __PVT__get_bip2__Vstatic__odd = ((IData)(1U) 
                                         + __PVT__get_bip2__Vstatic__odd);
    }
    __PVT__get_bip2__Vstatic__par = ((VL_LTS_III(32, 0U, 
                                                 VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__odd, (IData)(2U))) 
                                      << 1U) | VL_LTS_III(32, 0U, 
                                                          VL_MODDIVS_III(32, __PVT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_get_bip2__6__Vfuncout = __PVT__get_bip2__Vstatic__par;
    vlSelf->__PVT__tlvp_id_bip2 = __Vfunc_get_bip2__6__Vfuncout;
    __VdfgTmp_hcd0dc4ac__0 = ((0xffU & (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                        >> 0x14U)) 
                              == (0x1fU & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                           >> 0x12U)));
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = ((~ (vlSelf->__PVT__tlvp_id_tlv0[1U] 
                                            >> 0x17U)) 
                                        & (IData)(vlSelf->__VdfgTmp_h1b62dd67__0));
    vlSelf->__PVT__tlvp_id_trunc_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                           & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                              & ((~ 
                                                  (vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                                   >> 0x12U)) 
                                                 & ((0x3ffU 
                                                     & VL_SHIFTR_III(10,10,32, 
                                                                     (0x3ffU 
                                                                      & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                         >> 8U)), 3U)) 
                                                    <= vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->__PVT__tlvp_id_pad_detect = ((IData)(vlSelf->__PVT__tlvp_id_corrupt_eot) 
                                         & ((vlSelf->__PVT__tlvp_id_tlv0[2U] 
                                             >> 0x12U) 
                                            & ((IData)(__VdfgTmp_hcd0dc4ac__0) 
                                               & ((0x3ffU 
                                                   & VL_SHIFTR_III(10,10,32, 
                                                                   (0x3ffU 
                                                                    & (vlSelf->__PVT__tlvp_id_dp_debug_cmd 
                                                                       >> 8U)), 3U)) 
                                                  > vlSelf->__PVT__tlvp_id_word_num))));
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}
