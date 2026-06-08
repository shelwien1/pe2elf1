// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_tlvp_top.h"

void Vcr_cddip_cr_tlvp_top___ctor_var_reset(Vcr_cddip_cr_tlvp_top* vlSelf);

Vcr_cddip_cr_tlvp_top::Vcr_cddip_cr_tlvp_top(Vcr_cddip__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vcr_cddip_cr_tlvp_top___ctor_var_reset(this);
}

void Vcr_cddip_cr_tlvp_top::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vcr_cddip_cr_tlvp_top::~Vcr_cddip_cr_tlvp_top() {
}
