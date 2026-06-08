// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_dsm.h"
#include "Vcr_cddip_cr_tlvp_top.h"

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__1(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__1\n"); );
    // Init
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
    CData/*0:0*/ __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
    __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 0;
    VlWide<3>/*82:0*/ __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out;
    VL_ZERO_W(83, __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    // Body
    __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen = vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U];
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U];
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U];
    __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 
        ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
         && (IData)(((vlSymsp->TOP.cr_cddip__DOT____Vcellout__u_cr_isf__isf_ob_out[2U] 
                      >> 0x12U) & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r)))));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable 
        = vlSymsp->TOP.cr_cddip__DOT__rst_sync_n;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((4U >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((5U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen)) 
                  & (~ (IData)(vlSelf->__PVT__tlvp_ob_rd)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((2U >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((3U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr)) 
                  & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren)))));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr));
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSelf->__PVT__tlvp_ob_rd) {
            if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] = 0U;
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] = 0U;
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] = 0U;
            } else {
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
            }
        } else if (((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                     >> 0x12U) & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
            __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                = (0x3ffffU & __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U]);
        }
    } else {
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] = 0U;
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] = 0U;
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] = 0U;
    }
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U];
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U];
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U];
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((1U >= (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((2U == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen)) 
                  & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in 
        = ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full)) 
           & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((0U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
               && ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)
                    ? (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)
                    : ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
                       && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state;
        if ((0U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            if ((1U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                    if ((0x80000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert 
                            = (1U & (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                                     >> 9U));
                    }
                } else {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0U;
                }
            }
            if ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((0x20000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
                } else if ((0x40000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern 
                        = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern)));
                }
            } else if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((0x20000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
                } else if ((0x40000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern 
                        = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)));
                }
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
            }
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[0U] 
            = vlSymsp->TOP.cr_cddip__DOT____Vcellout__u_cr_isf__isf_ob_out[0U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[1U] 
            = vlSymsp->TOP.cr_cddip__DOT____Vcellout__u_cr_isf__isf_ob_out[1U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[2U] 
            = vlSymsp->TOP.cr_cddip__DOT____Vcellout__u_cr_isf__isf_ob_out[2U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        if ((0U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        } else if ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U];
        } else if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U];
        } else {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        if (vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd) {
            if ((IData)((0U != (0x80001U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U])))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern 
                    = (0x1fffU & ((vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[3U] 
                                   << 4U) | (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U] 
                                             >> 0x1cU)));
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[3U];
        }
        if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
            if ((IData)((0U != (0x80001U & vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern 
                    = (0x1fffU & ((vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] 
                                   << 4U) | (vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] 
                                             >> 0x1cU)));
            }
            if ((0U != (0x80001U & vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U]))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] 
                    = (IData)((((QData)((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2)) 
                                << 0x3eU) | (0x3fffffffffffffffULL 
                                             & (((QData)((IData)(
                                                                 vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                                 << 0x20U) 
                                                | (QData)((IData)(
                                                                  vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))))));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] 
                    = (IData)(((((QData)((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2)) 
                                 << 0x3eU) | (0x3fffffffffffffffULL 
                                              & (((QData)((IData)(
                                                                  vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(
                                                                   vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))))) 
                               >> 0x20U));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] 
                    = (IData)((0x3ffffffffffULL & (
                                                   ((QData)((IData)(
                                                                    vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                    = (IData)(((0x3ffffffffffULL & 
                                (((QData)((IData)(vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U])) 
                                  << 0x20U) | (QData)((IData)(
                                                              vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))) 
                               >> 0x20U));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U];
            }
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd) 
             & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 1U;
        } else if (((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd)) 
                    & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        }
        if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
             & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 1U;
        } else if (((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren)) 
                    & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
        }
    } else {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0x7ffU;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[0U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[1U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[2U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
    }
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen 
        = __Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
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
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
    vlSelf->__PVT__tlvp_ob_rd = (1U & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                       & ((~ (vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                                              >> 0x12U)) 
                                          | (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
           <= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern) 
           == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern));
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
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
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
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__2(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__2\n"); );
    // Body
    vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd = (1U & 
                                               ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                                & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                                                   & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                                                      | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
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
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
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
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__1(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__1\n"); );
    // Init
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
    VlWide<3>/*82:0*/ __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out;
    VL_ZERO_W(83, __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    // Body
    vlSelf->__Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen 
        = vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_wen;
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U];
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U];
    __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
        = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U];
    vlSelf->__Vdly__u_cr_axi4s_slave__DOT__axi4s_slv_wen 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(((vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im_pipe__ob_out[2U] 
                        >> 0x12U) & (~ (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r)))));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable 
        = vlSymsp->TOP.cr_cddip__DOT__rst_sync_n;
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((4U >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((5U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen)) 
                  & (~ (IData)(vlSelf->__PVT__tlvp_ob_rd)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((2U >= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((3U == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr)) 
                  & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren)))));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr));
    vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSelf->__PVT__tlvp_ob_rd) {
            if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) {
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] = 0U;
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] = 0U;
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] = 0U;
            } else {
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][0U];
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][1U];
                __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                    = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data
                    [vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr][2U];
            }
        } else if (((vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                     >> 0x12U) & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
            __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                = (0x3ffffU & __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U]);
        }
    } else {
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] = 0U;
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] = 0U;
        __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] = 0U;
    }
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[0U];
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[1U];
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
        = __Vdly__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U];
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_axi4s_mstr__DOT__axi4s_ib_in 
        = ((~ (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full)) 
           & (IData)(vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((0U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
               && ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)
                    ? (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)
                    : ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
                       && (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state;
        if ((0U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            if ((1U != vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                    if ((0x80000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert 
                            = (1U & (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                                     >> 9U));
                    }
                } else {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0U;
                }
            }
            if ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((0x20000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
                } else if ((0x40000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern 
                        = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern)));
                }
            } else if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
                if ((0x20000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
                } else if ((0x40000U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U])) {
                    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern 
                        = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)));
                }
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
            }
        }
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[0U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im_pipe__ob_out[0U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[1U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im_pipe__ob_out[1U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[2U] 
            = vlSymsp->TOP.cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im_pipe__ob_out[2U];
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        if ((0U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        } else if ((1U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U];
        } else if ((2U == vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U];
        } else {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        }
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        if (vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd) {
            if ((IData)((0U != (0x80001U & vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U])))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern 
                    = (0x1fffU & ((vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[3U] 
                                   << 4U) | (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U] 
                                             >> 0x1cU)));
            }
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[0U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[1U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[2U];
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U] 
                = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->pt_ib_tlv[3U];
        }
        if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
            if ((IData)((0U != (0x80001U & vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern 
                    = (0x1fffU & ((vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U] 
                                   << 4U) | (vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U] 
                                             >> 0x1cU)));
            }
            if ((0U != (0x80001U & vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U]))) {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] 
                    = (IData)((((QData)((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2)) 
                                << 0x3eU) | (0x3fffffffffffffffULL 
                                             & (((QData)((IData)(
                                                                 vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                                 << 0x20U) 
                                                | (QData)((IData)(
                                                                  vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))))));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] 
                    = (IData)(((((QData)((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2)) 
                                 << 0x3eU) | (0x3fffffffffffffffULL 
                                              & (((QData)((IData)(
                                                                  vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U])) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(
                                                                   vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U]))))) 
                               >> 0x20U));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] 
                    = (IData)((0x3ffffffffffULL & (
                                                   ((QData)((IData)(
                                                                    vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U])) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(
                                                                     vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))));
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                    = (IData)(((0x3ffffffffffULL & 
                                (((QData)((IData)(vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U])) 
                                  << 0x20U) | (QData)((IData)(
                                                              vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U])))) 
                               >> 0x20U));
            } else {
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[0U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[1U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[2U];
                vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] 
                    = vlSelf->u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata[3U];
            }
        }
        if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd) 
             & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 1U;
        } else if (((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd)) 
                    & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        }
        if (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
             & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 1U;
        } else if (((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren)) 
                    & (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
        }
    } else {
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0x7ffU;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 1U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[0U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[1U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__axi4s_slv_datain[2U] = 0U;
        vlSelf->__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[3U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv[3U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[0U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[1U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[2U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv[3U] = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
    }
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
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
           <= (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern));
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next 
        = ((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern) 
           == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern));
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
    __PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
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
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    if (vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__2(Vcr_cddip_cr_tlvp_top* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_tlvp_top___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__2\n"); );
    // Body
    vlSelf->__PVT__u_cr_tlvp__DOT__pt_ob_rd = (1U & 
                                               ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_dsm->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                                                & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                                                   & ((~ (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                                                      | (IData)(vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
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
