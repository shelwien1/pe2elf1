// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip_cr_sa_counter.h"

VL_ATTR_COLD void Vcr_cddip_cr_sa_counter___ctor_var_reset(Vcr_cddip_cr_sa_counter* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vcr_cddip_cr_sa_counter___ctor_var_reset\n"); );
    // Body
    vlSelf->sa_count = 0;
    vlSelf->sa_snapshot = 0;
    vlSelf->clk = 0;
    vlSelf->rst_n = 0;
    vlSelf->sa_event_sel = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->sa_events[__Vi0] = 0;
    }
    vlSelf->sa_clear = 0;
    vlSelf->sa_snap = 0;
    vlSelf->__PVT__sa_mux1 = 0;
}
