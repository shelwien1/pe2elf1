// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_dsm.h"
#include "Vcr_cddip_cr_tlvp_id.h"

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1\n"); );
    // Init
    SData/*12:0*/ __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    // Body
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_usr_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_pt_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        if (((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                >> 0x11U))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        } else if ((((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                        >> 0x12U)) & (3U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
                = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern)));
        }
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] = 0U;
        __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
        = __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__tlvp_usr_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                      && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                          & (1U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action))));
    vlSelf->__PVT__tlvp_pt_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                     && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                         & ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)) 
                                            | (1U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2\n"); );
    // Body
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action 
        = (3U & (((((((((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U))) 
                        | (1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U)))) 
                       | (2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))) 
                      | (3U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U)))) 
                     | (4U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                        >> 0x14U)))) 
                    | (5U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U)))) 
                   | (6U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                      >> 0x14U)))) 
                  | (7U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                     >> 0x14U)))) ? 
                 ((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                   >> 0x14U))) ? vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0
                   : ((1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U))) ? 
                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                       >> 2U) : ((2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                     >> 4U) : ((3U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                   >> 6U)
                                                : (
                                                   (4U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                    >> 8U)
                                                    : 
                                                   ((5U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                     >> 0xaU)
                                                     : 
                                                    ((6U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                      >> 0xcU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                      >> 0xeU))))))))
                  : (((((((((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U))) 
                            | (9U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                           | (0xaU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                          | (0xbU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                         | (0xcU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                              >> 0x14U)))) 
                        | (0xdU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                       | (0xeU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                      | (0xfU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U))))
                      ? ((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))
                          ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                             >> 0x10U) : ((9U == (0xffU 
                                                  & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                           ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                              >> 0x12U)
                                           : ((0xaU 
                                               == (0xffU 
                                                   & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                               ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                  >> 0x14U)
                                               : ((0xbU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                   >> 0x16U)
                                                   : 
                                                  ((0xcU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                    >> 0x18U)
                                                    : 
                                                   ((0xdU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                     >> 0x1aU)
                                                     : 
                                                    ((0xeU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                      >> 0x1cU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 
                                                      >> 0x1eU))))))))
                      : (((((((((0x10U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))) 
                                | (0x11U == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x12U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x13U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                             | (0x14U == (0xffU & (
                                                   vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                            | (0x15U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                           | (0x16U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                          | (0x17U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U))))
                          ? ((0x10U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))
                              ? vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1
                              : ((0x11U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                     >> 2U) : ((0x12U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                   >> 4U)
                                                : (
                                                   (0x13U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                    >> 6U)
                                                    : 
                                                   ((0x14U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                     >> 8U)
                                                     : 
                                                    ((0x15U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                      >> 0xaU)
                                                      : 
                                                     ((0x16U 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                       >> 0xcU)
                                                       : 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                       >> 0xeU))))))))
                          : (((((((((0x18U == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U))) 
                                    | (0x19U == (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))) 
                                   | (0x1aU == (0xffU 
                                                & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                                  | (0x1bU == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                                 | (0x1cU == (0xffU 
                                              & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                                | (0x1dU == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x1eU == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x1fU == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))))
                              ? ((0x18U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                     >> 0x10U) : ((0x19U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                   >> 0x12U)
                                                   : 
                                                  ((0x1aU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                    >> 0x14U)
                                                    : 
                                                   ((0x1bU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                     >> 0x16U)
                                                     : 
                                                    ((0x1cU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                      >> 0x18U)
                                                      : 
                                                     ((0x1dU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                       >> 0x1aU)
                                                       : 
                                                      ((0x1eU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                        >> 0x1cU)
                                                        : 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 
                                                        >> 0x1eU))))))))
                              : 1U)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1\n"); );
    // Init
    SData/*12:0*/ __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    // Body
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_pt_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__pt_ob_rd)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_usr_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        if (((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                >> 0x11U))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        } else if ((((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                        >> 0x12U)) & (3U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
                = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern)));
        }
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] = 0U;
        __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
        = __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__tlvp_pt_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                     && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                         & ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)) 
                                            | (1U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))));
    vlSelf->__PVT__tlvp_usr_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                      && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                          & (1U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action))));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2\n"); );
    // Body
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__pt_ob_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__pt_ob_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action 
        = (3U & (((((((((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U))) 
                        | (1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U)))) 
                       | (2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))) 
                      | (3U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U)))) 
                     | (4U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                        >> 0x14U)))) 
                    | (5U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U)))) 
                   | (6U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                      >> 0x14U)))) 
                  | (7U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                     >> 0x14U)))) ? 
                 ((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                   >> 0x14U))) ? vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0
                   : ((1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U))) ? 
                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                       >> 2U) : ((2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                     >> 4U) : ((3U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                   >> 6U)
                                                : (
                                                   (4U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                    >> 8U)
                                                    : 
                                                   ((5U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                     >> 0xaU)
                                                     : 
                                                    ((6U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                      >> 0xcU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                      >> 0xeU))))))))
                  : (((((((((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U))) 
                            | (9U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                           | (0xaU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                          | (0xbU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                         | (0xcU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                              >> 0x14U)))) 
                        | (0xdU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                       | (0xeU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                      | (0xfU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U))))
                      ? ((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))
                          ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                             >> 0x10U) : ((9U == (0xffU 
                                                  & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                           ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                              >> 0x12U)
                                           : ((0xaU 
                                               == (0xffU 
                                                   & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                               ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                  >> 0x14U)
                                               : ((0xbU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                   >> 0x16U)
                                                   : 
                                                  ((0xcU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                    >> 0x18U)
                                                    : 
                                                   ((0xdU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                     >> 0x1aU)
                                                     : 
                                                    ((0xeU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                      >> 0x1cU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 
                                                      >> 0x1eU))))))))
                      : (((((((((0x10U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))) 
                                | (0x11U == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x12U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x13U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                             | (0x14U == (0xffU & (
                                                   vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                            | (0x15U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                           | (0x16U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                          | (0x17U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U))))
                          ? ((0x10U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))
                              ? vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1
                              : ((0x11U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                     >> 2U) : ((0x12U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                   >> 4U)
                                                : (
                                                   (0x13U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                    >> 6U)
                                                    : 
                                                   ((0x14U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                     >> 8U)
                                                     : 
                                                    ((0x15U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                      >> 0xaU)
                                                      : 
                                                     ((0x16U 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                       >> 0xcU)
                                                       : 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                       >> 0xeU))))))))
                          : (((((((((0x18U == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U))) 
                                    | (0x19U == (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))) 
                                   | (0x1aU == (0xffU 
                                                & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                                  | (0x1bU == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                                 | (0x1cU == (0xffU 
                                              & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                                | (0x1dU == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x1eU == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x1fU == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))))
                              ? ((0x18U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                     >> 0x10U) : ((0x19U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                   >> 0x12U)
                                                   : 
                                                  ((0x1aU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                    >> 0x14U)
                                                    : 
                                                   ((0x1bU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                     >> 0x16U)
                                                     : 
                                                    ((0x1cU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                      >> 0x18U)
                                                      : 
                                                     ((0x1dU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                       >> 0x1aU)
                                                       : 
                                                      ((0x1eU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                        >> 0x1cU)
                                                        : 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 
                                                        >> 0x1eU))))))))
                              : 1U)))));
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__1\n"); );
    // Init
    SData/*12:0*/ __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    // Body
    __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty 
        = ((1U & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n))) 
           || (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_usr_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__afull_r 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && ((3U >= (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
               | (((4U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)) 
                   & (IData)(vlSelf->__PVT__tlvp_pt_ib_wen)) 
                  & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd)))));
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
        if (((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                >> 0x11U))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        } else if ((((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                        >> 0x12U)) & (3U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))) {
            __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
                = (0x1fffU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern)));
        }
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[0U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[1U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[3U];
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] 
            = ((0xfffffffU & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U]) 
               | ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                  << 0x1cU));
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] 
            = ((0x200U & vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U]) 
               | (0x3ffU & ((IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern) 
                            >> 4U)));
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata[3U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[0U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[1U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[2U] = 0U;
        vlSelf->__Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata[3U] = 0U;
        __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 1U;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0x10U;
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full 
        = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full));
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern 
        = __Vdly__u_cr_tlvp_spl__DOT__tlvp_spl_ordern;
    vlSelf->__PVT__tlvp_usr_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                      && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                          & (1U != (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action))));
    vlSelf->__PVT__tlvp_pt_ib_wen = ((IData)(vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) 
                                     && ((IData)(vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out_valid) 
                                         & ((0U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)) 
                                            | (1U == (IData)(vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
    }
}

VL_INLINE_OPT void Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2(Vcr_cddip_cr_tlvp_dsm* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm___nba_sequent__TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm__2\n"); );
    // Body
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->__PVT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action 
        = (3U & (((((((((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U))) 
                        | (1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U)))) 
                       | (2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))) 
                      | (3U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                         >> 0x14U)))) 
                     | (4U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                        >> 0x14U)))) 
                    | (5U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U)))) 
                   | (6U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                      >> 0x14U)))) 
                  | (7U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                     >> 0x14U)))) ? 
                 ((0U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                   >> 0x14U))) ? vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0
                   : ((1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U))) ? 
                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                       >> 2U) : ((2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                     >> 4U) : ((3U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                   >> 6U)
                                                : (
                                                   (4U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                    >> 8U)
                                                    : 
                                                   ((5U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                     >> 0xaU)
                                                     : 
                                                    ((6U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                      >> 0xcU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                      >> 0xeU))))))))
                  : (((((((((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U))) 
                            | (9U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                           | (0xaU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                          | (0xbU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                         | (0xcU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                              >> 0x14U)))) 
                        | (0xdU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                       | (0xeU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                      | (0xfU == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                           >> 0x14U))))
                      ? ((8U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                          >> 0x14U)))
                          ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                             >> 0x10U) : ((9U == (0xffU 
                                                  & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                           ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                              >> 0x12U)
                                           : ((0xaU 
                                               == (0xffU 
                                                   & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                               ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                  >> 0x14U)
                                               : ((0xbU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                   >> 0x16U)
                                                   : 
                                                  ((0xcU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                    >> 0x18U)
                                                    : 
                                                   ((0xdU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                     >> 0x1aU)
                                                     : 
                                                    ((0xeU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                      >> 0x1cU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 
                                                      >> 0x1eU))))))))
                      : (((((((((0x10U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))) 
                                | (0x11U == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x12U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x13U == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                             | (0x14U == (0xffU & (
                                                   vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                            | (0x15U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                           | (0x16U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                          | (0x17U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U))))
                          ? ((0x10U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))
                              ? vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1
                              : ((0x11U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                     >> 2U) : ((0x12U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                   >> 4U)
                                                : (
                                                   (0x13U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                    >> 6U)
                                                    : 
                                                   ((0x14U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                     >> 8U)
                                                     : 
                                                    ((0x15U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                      >> 0xaU)
                                                      : 
                                                     ((0x16U 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                       >> 0xcU)
                                                       : 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                       >> 0xeU))))))))
                          : (((((((((0x18U == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U))) 
                                    | (0x19U == (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))) 
                                   | (0x1aU == (0xffU 
                                                & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                                  | (0x1bU == (0xffU 
                                               & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                                 | (0x1cU == (0xffU 
                                              & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                                | (0x1dU == (0xffU 
                                             & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x1eU == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x1fU == (0xffU & 
                                           (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                            >> 0x14U))))
                              ? ((0x18U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                     >> 0x10U) : ((0x19U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                   >> 0x12U)
                                                   : 
                                                  ((0x1aU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                    >> 0x14U)
                                                    : 
                                                   ((0x1bU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                     >> 0x16U)
                                                     : 
                                                    ((0x1cU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                      >> 0x18U)
                                                      : 
                                                     ((0x1dU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                       >> 0x1aU)
                                                       : 
                                                      ((0x1eU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                        >> 0x1cU)
                                                        : 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 
                                                        >> 0x1eU))))))))
                              : 1U)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->__PVT__tlvp_pt_ib_wen) {
        vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__pt_ob_rd) 
                 & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
}
