// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_sa_counter.h"

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [0U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [0U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [0U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [0U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [0U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [0U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [0U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [0U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [1U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [1U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [1U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [1U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [1U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [1U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [1U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [1U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [1U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [1U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [1U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [1U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [1U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [1U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [1U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [1U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [2U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [2U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [2U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [2U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [2U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [2U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [2U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [2U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [2U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [2U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [2U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [2U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [2U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [2U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [2U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [2U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [3U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [3U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [3U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [3U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [3U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [3U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [3U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [3U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [3U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [3U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [3U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [3U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [3U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [3U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [3U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [3U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [4U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [4U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [4U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [4U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [4U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [4U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [4U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [4U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [4U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [4U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [4U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [4U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [4U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [4U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [4U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [4U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [5U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [5U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [5U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [5U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [5U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [5U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [5U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [5U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [5U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [5U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [5U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [5U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [5U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [5U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [5U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [5U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [6U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [6U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [6U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [6U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [6U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [6U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [6U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [6U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [6U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [6U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [6U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [6U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [6U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [6U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [6U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [6U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [7U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [7U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [7U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [7U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [7U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [7U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [7U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [7U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [7U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [7U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [7U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [7U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [7U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [7U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [7U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [7U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [8U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [8U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [8U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [8U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [8U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [8U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [8U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [8U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [8U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [8U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [8U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [8U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [8U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [8U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [8U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [8U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [9U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [9U]) ? ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                         [9U]) ? ((0x80U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [9U])
                                                   ? 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [9U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                   : 
                                                  ((0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [9U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                         : ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [9U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [9U])
                                              ? vlSelf->sa_events
                                             [0xbU]
                                              : vlSelf->sa_events
                                             [0xaU])
                                             : ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [9U])
                                                 ? 
                                                vlSelf->sa_events
                                                [9U]
                                                 : 
                                                vlSelf->sa_events
                                                [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [9U]) ? ((0x80U 
                                             & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                             [9U]) ? 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [9U])
                                              ? vlSelf->sa_events
                                             [7U] : 
                                             vlSelf->sa_events
                                             [6U]) : 
                                            ((0x40U 
                                              & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                              [9U])
                                              ? vlSelf->sa_events
                                             [5U] : 
                                             vlSelf->sa_events
                                             [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [9U]) ? ((0x40U 
                                                 & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                 [9U])
                                                 ? 
                                                vlSelf->sa_events
                                                [3U]
                                                 : 
                                                vlSelf->sa_events
                                                [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [9U]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xaU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xaU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xaU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xaU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xaU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xaU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xaU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xaU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xaU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xaU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xaU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xaU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xaU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xaU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xaU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xaU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xbU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xbU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xbU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xbU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xbU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xbU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xbU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xbU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xbU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xbU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xbU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xbU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xbU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xbU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xbU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xbU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xcU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xcU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xcU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xcU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xcU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xcU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xcU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xcU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xcU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xcU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xcU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xcU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xcU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xcU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xcU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xcU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xdU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xdU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xdU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xdU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xdU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xdU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xdU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xdU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xdU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xdU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xdU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xdU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xdU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xdU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xdU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xdU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xeU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xeU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xeU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xeU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xeU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xeU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xeU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xeU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xeU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xeU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xeU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xeU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xeU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xeU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xeU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xeU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0xfU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0xfU]) ? ((0x100U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xfU]) ? 
                                          ((0x80U & 
                                            vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0xfU])
                                            ? ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xfU])
                                                ? vlSelf->sa_events
                                               [0xfU]
                                                : vlSelf->sa_events
                                               [0xeU])
                                            : ((0x40U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0xfU])
                                                ? vlSelf->sa_events
                                               [0xdU]
                                                : vlSelf->sa_events
                                               [0xcU]))
                                           : ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xfU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xfU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [0xbU]
                                                   : 
                                                  vlSelf->sa_events
                                                  [0xaU])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xfU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [9U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0xfU]) ? ((0x80U 
                                               & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                               [0xfU])
                                               ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xfU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [7U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [6U])
                                               : ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xfU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [5U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0xfU]) ? ((0x40U 
                                                   & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                   [0xfU])
                                                   ? 
                                                  vlSelf->sa_events
                                                  [3U]
                                                   : 
                                                  vlSelf->sa_events
                                                  [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0xfU]) ? 
                                          vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x10U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x10U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x10U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x10U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x10U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x10U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x10U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x10U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x10U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x10U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x11U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x11U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x11U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x11U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x11U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x11U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x11U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x11U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x11U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x11U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x12U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x12U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x12U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x12U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x12U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x12U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x12U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x12U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x12U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x12U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x13U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x13U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x13U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x13U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x13U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x13U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x13U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x13U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x13U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x13U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x14U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x14U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x14U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x14U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x14U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x14U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x14U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x14U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x14U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x14U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x15U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x15U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x15U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x15U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x15U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x15U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x15U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x15U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x15U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x15U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x16U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x16U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x16U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x16U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x16U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x16U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x16U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x16U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x16U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x16U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x17U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x17U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x17U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x17U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x17U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x17U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x17U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x17U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x17U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x17U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x18U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x18U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x18U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x18U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x18U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x18U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x18U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x18U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x18U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x18U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x19U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x19U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x19U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x19U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x19U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x19U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x19U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x19U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x19U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x19U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1aU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1aU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1aU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1aU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1aU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1aU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1bU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1bU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1bU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1bU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1bU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1bU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1cU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1cU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1cU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1cU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1cU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1cU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1dU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1dU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1dU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1dU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1dU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1dU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1eU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1eU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1eU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1eU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1eU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1eU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x1fU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x1fU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x1fU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x1fU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x1fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x1fU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x1fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x1fU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x20U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x20U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x20U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x20U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x20U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x20U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x20U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x20U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x20U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x20U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x21U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x21U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x21U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x21U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x21U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x21U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x21U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x21U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x21U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x21U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x22U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x22U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x22U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x22U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x22U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x22U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x22U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x22U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x22U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x22U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x23U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x23U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x23U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x23U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x23U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x23U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x23U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x23U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x23U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x23U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x24U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x24U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x24U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x24U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x24U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x24U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x24U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x24U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x24U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x24U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x25U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x25U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x25U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x25U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x25U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x25U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x25U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x25U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x25U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x25U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x26U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x26U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x26U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x26U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x26U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x26U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x26U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x26U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x26U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x26U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x27U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x27U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x27U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x27U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x27U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x27U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x27U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x27U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x27U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x27U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x28U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x28U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x28U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x28U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x28U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x28U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x28U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x28U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x28U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x28U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x29U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x29U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x29U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x29U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x29U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x29U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x29U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x29U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x29U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x29U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2aU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2aU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2aU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2aU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2aU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2aU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2bU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2bU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2bU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2bU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2bU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2bU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2cU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2cU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2cU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2cU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2cU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2cU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2dU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2dU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2dU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2dU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2dU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2dU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2eU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2eU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2eU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2eU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2eU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2eU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x2fU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x2fU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x2fU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x2fU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x2fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x2fU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x2fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x2fU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x30U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x30U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x30U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x30U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x30U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x30U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x30U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x30U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x30U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x30U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x31U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x31U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x31U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x31U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x31U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x31U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x31U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x31U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x31U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x31U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x32U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x32U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x32U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x32U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x32U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x32U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x32U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x32U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x32U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x32U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x33U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x33U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x33U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x33U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x33U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x33U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x33U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x33U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x33U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x33U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x34U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x34U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x34U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x34U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x34U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x34U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x34U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x34U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x34U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x34U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x35U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x35U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x35U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x35U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x35U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x35U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x35U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x35U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x35U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x35U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x36U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x36U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x36U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x36U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x36U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x36U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x36U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x36U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x36U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x36U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x37U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x37U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x37U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x37U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x37U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x37U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x37U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x37U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x37U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x37U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x38U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x38U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x38U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x38U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x38U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x38U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x38U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x38U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x38U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x38U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x39U]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x39U]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x39U])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x39U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x39U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x39U]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x39U])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x39U]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x39U])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x39U])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3aU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3aU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3aU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3aU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3aU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3aU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3aU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3aU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3bU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3bU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3bU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3bU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3bU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3bU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3bU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3bU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3cU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3cU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3cU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3cU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3cU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3cU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3cU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3cU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3dU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3dU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3dU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3dU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3dU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3dU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3dU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3dU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3eU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3eU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3eU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3eU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3eU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3eU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3eU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3eU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i__0(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i__0\n"); );
    // Init
    QData/*49:0*/ __Vdly__sa_count;
    __Vdly__sa_count = 0;
    // Body
    __Vdly__sa_count = vlSelf->sa_count;
    if (vlSymsp->TOP.cr_cddip__DOT__rst_sync_n) {
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear) {
            __Vdly__sa_count = 0ULL;
        } else if ((1U & (IData)((vlSelf->__PVT__sa_mux1 
                                  >> (0x3fU & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                      [0x3fU]))))) {
            __Vdly__sa_count = (0x3ffffffffffffULL 
                                & (1ULL + vlSelf->sa_count));
        }
        if (vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap) {
            vlSelf->sa_snapshot = vlSelf->sa_count;
        }
    } else {
        __Vdly__sa_count = 0ULL;
        vlSelf->sa_snapshot = 0ULL;
    }
    vlSelf->sa_count = __Vdly__sa_count;
}

VL_INLINE_OPT void Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i__1(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___nba_sequent__TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i__1\n"); );
    // Body
    vlSelf->__PVT__sa_mux1 = ((0x200U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                               [0x3fU]) ? ((0x100U 
                                            & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                            [0x3fU])
                                            ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xfU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xeU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xdU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xcU]))
                                            : ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [0xbU]
                                                    : 
                                                   vlSelf->sa_events
                                                   [0xaU])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [9U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [8U])))
                               : ((0x100U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                   [0x3fU]) ? ((0x80U 
                                                & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                [0x3fU])
                                                ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [7U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [6U])
                                                : (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [5U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [4U]))
                                   : ((0x80U & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                       [0x3fU]) ? (
                                                   (0x40U 
                                                    & vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                                    [0x3fU])
                                                    ? 
                                                   vlSelf->sa_events
                                                   [3U]
                                                    : 
                                                   vlSelf->sa_events
                                                   [2U])
                                       : ((0x40U & 
                                           vlSymsp->TOP.cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl
                                           [0x3fU])
                                           ? vlSelf->sa_events
                                          [1U] : vlSelf->sa_events
                                          [0U]))));
}
