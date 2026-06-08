// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_id.h"

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id__0\n"); );
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
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
    vlSelf->tlvp_ib_rd = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                                   | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r) 
                                      | ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r) 
                                         | ((IData)(vlSelf->__PVT__tlvp_id_tm_off) 
                                            | ((IData)(vlSelf->__PVT__tlvp_id_pad_detect) 
                                               | (IData)(vlSelf->__PVT__tlvp_id_padtlv))))))));
}

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id__0\n"); );
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id__0(Vcr_cddip_cr_tlvp_id* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                    Vcr_cddip_cr_tlvp_id___stl_sequent__TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id__0\n"); );
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
