// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_dsm.h"
#include "Vcr_cddip_cr_tlvp_top.h"

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_top___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__0(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__0\n"); );
    // Init
    IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    CData/*1:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    VlWide<3>/*81:0*/ u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst;
    VL_ZERO_W(82, u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst);
    CData/*1:0*/ __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__Vfuncout;
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in;
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in = 0;
    VlWide<3>/*95:0*/ __Vtemp_2;
    // Body
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] = 0U;
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U] = 0U;
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] = 0U;
    } else {
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
    }
    __Vtemp_2[0U] = (IData)((((QData)((IData)((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                               [(1U 
                                                 & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                    >> 7U))][1U] 
                                               >> 0x18U))) 
                              << 0x38U) | (((QData)((IData)(
                                                            (0xffU 
                                                             & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                [
                                                                (1U 
                                                                 & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                    >> 6U))][1U] 
                                                                >> 0x10U)))) 
                                            << 0x30U) 
                                           | (((QData)((IData)(
                                                               (0xffU 
                                                                & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                   [
                                                                   (1U 
                                                                    & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                       >> 5U))][1U] 
                                                                   >> 8U)))) 
                                               << 0x28U) 
                                              | (((QData)((IData)(
                                                                  (0xffU 
                                                                   & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                   [
                                                                   (1U 
                                                                    & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                       >> 4U))][1U]))) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(
                                                                   ((0xff000000U 
                                                                     & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                     [
                                                                     (1U 
                                                                      & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                         >> 3U))][0U]) 
                                                                    | ((0xff0000U 
                                                                        & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                        [
                                                                        (1U 
                                                                         & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                            >> 2U))][0U]) 
                                                                       | ((0xff00U 
                                                                           & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                           [
                                                                           (1U 
                                                                            & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                               >> 1U))][0U]) 
                                                                          | (0xffU 
                                                                             & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                             [
                                                                             (1U 
                                                                              & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr))][0U])))))))))));
    __Vtemp_2[1U] = (IData)(((((QData)((IData)((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                [(1U 
                                                  & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                     >> 7U))][1U] 
                                                >> 0x18U))) 
                               << 0x38U) | (((QData)((IData)(
                                                             (0xffU 
                                                              & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                 [
                                                                 (1U 
                                                                  & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                     >> 6U))][1U] 
                                                                 >> 0x10U)))) 
                                             << 0x30U) 
                                            | (((QData)((IData)(
                                                                (0xffU 
                                                                 & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                    [
                                                                    (1U 
                                                                     & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                        >> 5U))][1U] 
                                                                    >> 8U)))) 
                                                << 0x28U) 
                                               | (((QData)((IData)(
                                                                   (0xffU 
                                                                    & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                    [
                                                                    (1U 
                                                                     & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                        >> 4U))][1U]))) 
                                                   << 0x20U) 
                                                  | (QData)((IData)(
                                                                    ((0xff000000U 
                                                                      & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                      [
                                                                      (1U 
                                                                       & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                          >> 3U))][0U]) 
                                                                     | ((0xff0000U 
                                                                         & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                         [
                                                                         (1U 
                                                                          & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                             >> 2U))][0U]) 
                                                                        | ((0xff00U 
                                                                            & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                            [
                                                                            (1U 
                                                                             & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                                >> 1U))][0U]) 
                                                                           | (0xffU 
                                                                              & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                              [
                                                                              (1U 
                                                                               & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr))][0U])))))))))) 
                             >> 0x20U));
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[0U] 
        = __Vtemp_2[0U];
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[1U] 
        = __Vtemp_2[1U];
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[2U] 
        = ((0x30000U & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
            [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                    >> 0xaU))][2U]) | ((0xff00U & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                        [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                >> 9U))][2U]) 
                                       | (0xffU & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                          [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                  >> 8U))][2U])));
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] = 0U;
    } else {
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][3U];
    }
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in 
        = ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full)) 
           & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
           <= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern) 
           == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern));
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
    vlSelf->axi4s_ob_out[0U] = u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[0U];
    vlSelf->axi4s_ob_out[1U] = u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[1U];
    vlSelf->axi4s_ob_out[2U] = ((0x40000U & ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
                                             << 0x12U)) 
                                | u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[2U]);
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
        = (0x3fffffffffffffffULL & (((QData)((IData)(
                                                     vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn = 0U;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 2U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 4U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 6U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 8U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xaU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xcU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xeU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x10U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x12U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x14U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x16U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x18U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x20U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x22U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x24U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x26U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x28U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x30U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x32U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x34U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x36U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x38U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 1U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 3U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 5U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 7U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 9U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xbU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xdU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0xfU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x11U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x13U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x15U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x17U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x19U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x1fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x21U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x23U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x25U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x27U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x29U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x2fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x31U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x33U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x35U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x37U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x39U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__data_in 
                       >> 0x3fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par 
        = ((VL_LTS_III(32, 0U, VL_MODDIVS_III(32, __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd, (IData)(2U))) 
            << 1U) | VL_LTS_III(32, 0U, VL_MODDIVS_III(32, __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__Vfuncout 
        = __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 
        = __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__0__Vfuncout;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr;
    if ((1U & ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
               & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r))))) {
        if (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full) {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 1U;
        }
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0U;
    }
    if (((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
          >> 0x12U) & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr 
            = (1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr)));
        if (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty) {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 1U;
        }
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0U;
    }
    vlSelf->__PVT__tlvp_ob_rd = (1U & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                       & ((~ (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                                              >> 0x12U)) 
                                          | (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->__PVT__tlvp_ob_rd) {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSelf->__PVT__tlvp_ob_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren 
        = (1U & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)) 
                       | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd = (1U & 
                                               ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                                & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                                                   & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                                                      | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
}

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_top___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__0(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___stl_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__0\n"); );
    // Init
    IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    CData/*0:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    CData/*1:0*/ __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    VlWide<3>/*81:0*/ u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst;
    VL_ZERO_W(82, u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst);
    CData/*1:0*/ __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__Vfuncout;
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in;
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in = 0;
    VlWide<3>/*95:0*/ __Vtemp_2;
    // Body
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] = 0U;
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U] = 0U;
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] = 0U;
    } else {
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[0U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[1U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata[2U] 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
    }
    __Vtemp_2[0U] = (IData)((((QData)((IData)((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                               [(1U 
                                                 & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                    >> 7U))][1U] 
                                               >> 0x18U))) 
                              << 0x38U) | (((QData)((IData)(
                                                            (0xffU 
                                                             & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                [
                                                                (1U 
                                                                 & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                    >> 6U))][1U] 
                                                                >> 0x10U)))) 
                                            << 0x30U) 
                                           | (((QData)((IData)(
                                                               (0xffU 
                                                                & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                   [
                                                                   (1U 
                                                                    & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                       >> 5U))][1U] 
                                                                   >> 8U)))) 
                                               << 0x28U) 
                                              | (((QData)((IData)(
                                                                  (0xffU 
                                                                   & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                   [
                                                                   (1U 
                                                                    & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                       >> 4U))][1U]))) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(
                                                                   ((0xff000000U 
                                                                     & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                     [
                                                                     (1U 
                                                                      & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                         >> 3U))][0U]) 
                                                                    | ((0xff0000U 
                                                                        & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                        [
                                                                        (1U 
                                                                         & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                            >> 2U))][0U]) 
                                                                       | ((0xff00U 
                                                                           & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                           [
                                                                           (1U 
                                                                            & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                               >> 1U))][0U]) 
                                                                          | (0xffU 
                                                                             & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                             [
                                                                             (1U 
                                                                              & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr))][0U])))))))))));
    __Vtemp_2[1U] = (IData)(((((QData)((IData)((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                [(1U 
                                                  & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                     >> 7U))][1U] 
                                                >> 0x18U))) 
                               << 0x38U) | (((QData)((IData)(
                                                             (0xffU 
                                                              & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                 [
                                                                 (1U 
                                                                  & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                     >> 6U))][1U] 
                                                                 >> 0x10U)))) 
                                             << 0x30U) 
                                            | (((QData)((IData)(
                                                                (0xffU 
                                                                 & (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                    [
                                                                    (1U 
                                                                     & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                        >> 5U))][1U] 
                                                                    >> 8U)))) 
                                                << 0x28U) 
                                               | (((QData)((IData)(
                                                                   (0xffU 
                                                                    & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                    [
                                                                    (1U 
                                                                     & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                        >> 4U))][1U]))) 
                                                   << 0x20U) 
                                                  | (QData)((IData)(
                                                                    ((0xff000000U 
                                                                      & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                      [
                                                                      (1U 
                                                                       & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                          >> 3U))][0U]) 
                                                                     | ((0xff0000U 
                                                                         & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                         [
                                                                         (1U 
                                                                          & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                             >> 2U))][0U]) 
                                                                        | ((0xff00U 
                                                                            & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                            [
                                                                            (1U 
                                                                             & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                                                >> 1U))][0U]) 
                                                                           | (0xffU 
                                                                              & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                                                              [
                                                                              (1U 
                                                                               & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr))][0U])))))))))) 
                             >> 0x20U));
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[0U] 
        = __Vtemp_2[0U];
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[1U] 
        = __Vtemp_2[1U];
    u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[2U] 
        = ((0x30000U & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
            [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                    >> 0xaU))][2U]) | ((0xff00U & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                        [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                >> 9U))][2U]) 
                                       | (0xffU & vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload
                                          [(1U & ((IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr) 
                                                  >> 8U))][2U])));
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] = 0U;
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] = 0U;
    } else {
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
        vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
            [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][3U];
    }
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in 
        = ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full)) 
           & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
           <= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern) 
           == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern));
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
    vlSelf->axi4s_ob_out[0U] = u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[0U];
    vlSelf->axi4s_ob_out[1U] = u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[1U];
    vlSelf->axi4s_ob_out[2U] = ((0x40000U & ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
                                             << 0x12U)) 
                                | u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst[2U]);
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
        = (0x3fffffffffffffffULL & (((QData)((IData)(
                                                     vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                     << 0x20U) | (QData)((IData)(
                                                                 vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn = 0U;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 2U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 4U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 6U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 8U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xaU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xcU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xeU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x10U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x12U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x14U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x16U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x18U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x20U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x22U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x24U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x26U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x28U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x30U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x32U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x34U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x36U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x38U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3aU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3cU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3eU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 1U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 3U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 5U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 7U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 9U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xbU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xdU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0xfU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x11U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x13U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x15U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x17U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x19U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x1fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x21U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x23U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x25U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x27U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x29U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x2fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x31U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x33U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x35U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x37U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x39U)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3bU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3dU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__data_in 
                       >> 0x3fU)))) {
        __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd);
    }
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par 
        = ((VL_LTS_III(32, 0U, VL_MODDIVS_III(32, __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__odd, (IData)(2U))) 
            << 1U) | VL_LTS_III(32, 0U, VL_MODDIVS_III(32, __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__Vfuncout 
        = __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__Vstatic__par;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 
        = __Vfunc_u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__get_bip2__1__Vfuncout;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty;
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr;
    if ((1U & ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
               & ((~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__afull_r)) 
                  & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__afull_r)))))) {
        if (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full) {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 1U;
        }
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0U;
    }
    if (((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
          >> 0x12U) & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr 
            = (1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr)));
        if (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty) {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 1U;
        }
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0U;
    }
    vlSelf->__PVT__tlvp_ob_rd = (1U & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                       & ((~ (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                                              >> 0x12U)) 
                                          | (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->__PVT__tlvp_ob_rd) {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSelf->__PVT__tlvp_ob_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren 
        = (1U & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)) 
                       | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd = (1U & 
                                               ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                                & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                                                   & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                                                      | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
}
