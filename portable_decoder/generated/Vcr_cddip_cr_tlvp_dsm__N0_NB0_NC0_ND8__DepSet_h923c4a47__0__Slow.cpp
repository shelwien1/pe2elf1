// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8.h"
#include "Vcr_cddip_cr_tlvp_id.h"

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0(Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8___stl_sequent__TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0\n"); );
    // Body
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
                                   >> 0x14U))) ? vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0
                   : ((1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U))) ? 
                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                       >> 2U) : ((2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                     >> 4U) : ((3U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                   >> 6U)
                                                : (
                                                   (4U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                    >> 8U)
                                                    : 
                                                   ((5U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                     >> 0xaU)
                                                     : 
                                                    ((6U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                      >> 0xcU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
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
                          ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                             >> 0x10U) : ((9U == (0xffU 
                                                  & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                           ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                              >> 0x12U)
                                           : ((0xaU 
                                               == (0xffU 
                                                   & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                               ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                  >> 0x14U)
                                               : ((0xbU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                   >> 0x16U)
                                                   : 
                                                  ((0xcU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                    >> 0x18U)
                                                    : 
                                                   ((0xdU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                     >> 0x1aU)
                                                     : 
                                                    ((0xeU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
                                                      >> 0x1cU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 
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
                              ? vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1
                              : ((0x11U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                     >> 2U) : ((0x12U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                   >> 4U)
                                                : (
                                                   (0x13U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                    >> 6U)
                                                    : 
                                                   ((0x14U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                     >> 8U)
                                                     : 
                                                    ((0x15U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                      >> 0xaU)
                                                      : 
                                                     ((0x16U 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                       >> 0xcU)
                                                       : 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
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
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                     >> 0x10U) : ((0x19U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                   >> 0x12U)
                                                   : 
                                                  ((0x1aU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                    >> 0x14U)
                                                    : 
                                                   ((0x1bU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                     >> 0x16U)
                                                     : 
                                                    ((0x1cU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                      >> 0x18U)
                                                      : 
                                                     ((0x1dU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                       >> 0x1aU)
                                                       : 
                                                      ((0x1eU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                        >> 0x1cU)
                                                        : 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 
                                                        >> 0x1eU))))))))
                              : 1U)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((7U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
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

VL_ATTR_COLD void Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8___stl_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0(Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+                  Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8___stl_sequent__TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__0\n"); );
    // Body
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
                                   >> 0x14U))) ? vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0
                   : ((1U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                       >> 0x14U))) ? 
                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                       >> 2U) : ((2U == (0xffU & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                  >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                     >> 4U) : ((3U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                   >> 6U)
                                                : (
                                                   (4U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                    >> 8U)
                                                    : 
                                                   ((5U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                     >> 0xaU)
                                                     : 
                                                    ((6U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                      >> 0xcU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
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
                          ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                             >> 0x10U) : ((9U == (0xffU 
                                                  & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                           ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                              >> 0x12U)
                                           : ((0xaU 
                                               == (0xffU 
                                                   & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                               ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                  >> 0x14U)
                                               : ((0xbU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                   >> 0x16U)
                                                   : 
                                                  ((0xcU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                    >> 0x18U)
                                                    : 
                                                   ((0xdU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                     >> 0x1aU)
                                                     : 
                                                    ((0xeU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
                                                      >> 0x1cU)
                                                      : 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 
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
                              ? vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1
                              : ((0x11U == (0xffU & 
                                            (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                     >> 2U) : ((0x12U 
                                                == 
                                                (0xffU 
                                                 & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                    >> 0x14U)))
                                                ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                   >> 4U)
                                                : (
                                                   (0x13U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                    >> 6U)
                                                    : 
                                                   ((0x14U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                     >> 8U)
                                                     : 
                                                    ((0x15U 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                      >> 0xaU)
                                                      : 
                                                     ((0x16U 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                       >> 0xcU)
                                                       : 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
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
                                  ? (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                     >> 0x10U) : ((0x19U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? 
                                                  (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                   >> 0x12U)
                                                   : 
                                                  ((0x1aU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? 
                                                   (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                    >> 0x14U)
                                                    : 
                                                   ((0x1bU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? 
                                                    (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                     >> 0x16U)
                                                     : 
                                                    ((0x1cU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? 
                                                     (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                      >> 0x18U)
                                                      : 
                                                     ((0x1dU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? 
                                                      (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                       >> 0x1aU)
                                                       : 
                                                      ((0x1eU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSelf->__PVT__u_cr_tlvp_id->tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                        >> 0x1cU)
                                                        : 
                                                       (vlSymsp->TOP.cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 
                                                        >> 0x1eU))))))))
                              : 1U)))));
    vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    if (vlSelf->__PVT__tlvp_usr_ib_wen) {
        if ((1U & (~ (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((7U == (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
        }
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
