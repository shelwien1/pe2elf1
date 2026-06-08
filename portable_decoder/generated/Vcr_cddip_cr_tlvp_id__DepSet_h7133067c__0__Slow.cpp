// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_tlvp_id.h"

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___eval_initial__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___eval_initial__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id\n"); );
    // Body
    vlSelf->__PVT__tlvp_id_tlv0[2U] = (0xfffffffU & 
                                       vlSelf->__PVT__tlvp_id_tlv0[2U]);
    vlSelf->__PVT__tlvp_id_tlv0[3U] = 0U;
}

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___ctor_var_reset(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___ctor_var_reset\n"); );
    // Body
    vlSelf->tlvp_ib_rd = 0;
    vlSelf->tlvp_id_out_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->tlvp_id_out);
    vlSelf->tlvp_error = 0;
    vlSelf->clk = 0;
    vlSelf->rst_n = 0;
    vlSelf->tlvp_ib_empty = 0;
    vlSelf->tlvp_ib_aempty = 0;
    VL_ZERO_RESET_W(83, vlSelf->tlvp_ib);
    vlSelf->pt_ib_full = 0;
    vlSelf->pt_ib_afull = 0;
    vlSelf->usr_ib_full = 0;
    vlSelf->usr_ib_afull = 0;
    vlSelf->module_id = 0;
    VL_ZERO_RESET_W(106, vlSelf->__PVT__tlvp_id_tlv0);
    vlSelf->__PVT__tlvp_id_tlv0_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->__PVT__tlvp_id_tlv0_save);
    vlSelf->__PVT__tlvp_id_bip2 = 0;
    vlSelf->__PVT__tlvp_id_bip2_error = 0;
    vlSelf->__PVT__tlvp_id_bip2_ftr_error = 0;
    vlSelf->__PVT__tlvp_id_frame = 0;
    vlSelf->__PVT__tlvp_id_frame_error = 0;
    vlSelf->__PVT__tlvp_id_word_num = 0;
    vlSelf->__PVT__tlvp_id_debug_word_num = 0;
    vlSelf->__PVT__tlvp_id_dp_debug_cmd = 0;
    vlSelf->__PVT__tlvp_id_bp_debug_cmd = 0;
    vlSelf->__PVT__tlvp_id_tm_count = 0;
    vlSelf->__PVT__tlvp_id_tm_off = 0;
    vlSelf->__PVT__tlvp_id_corrupt_data = 0;
    vlSelf->__PVT__tlvp_id_corrupt_eot = 0;
    vlSelf->__PVT__tlvp_id_debuq_word_msk = 0;
    vlSelf->__PVT__tlvp_id_dp_cmd_wr = 0;
    vlSelf->__PVT__tlvp_id_dp_type = 0;
    vlSelf->__PVT__tlvp_id_tm_count_en = 0;
    vlSelf->__PVT__tlvp_id_errors = 0;
    vlSelf->__PVT__tlvp_id_errors_p1 = 0;
    vlSelf->__PVT__tlvp_id_padtlv = 0;
    vlSelf->__PVT__tlvp_id_pad_detect = 0;
    vlSelf->__PVT__tlvp_id_trunc_detect = 0;
    vlSelf->__VdfgTmp_h1b62dd67__0 = 0;
    vlSelf->__Vdly__tlvp_id_word_num = 0;
    vlSelf->__Vdly__tlvp_id_dp_debug_cmd = 0;
    vlSelf->__Vdly__tlvp_id_corrupt_data = 0;
    vlSelf->__Vdly__tlvp_id_corrupt_eot = 0;
    VL_ZERO_RESET_W(106, vlSelf->__Vdly__tlvp_id_tlv0_save);
    vlSelf->__Vdly__tlvp_id_padtlv = 0;
    vlSelf->__Vdly__tlvp_id_frame = 0;
}
