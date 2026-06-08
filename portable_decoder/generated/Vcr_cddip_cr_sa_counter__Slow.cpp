// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_sa_counter.h"

void Vcr_cddip_cr_sa_counter___ctor_var_reset(Vcr_cddip_cr_sa_counter* vlSelf);

Vcr_cddip_cr_sa_counter::Vcr_cddip_cr_sa_counter(Vcr_cddip__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vcr_cddip_cr_sa_counter___ctor_var_reset(this);
}

void Vcr_cddip_cr_sa_counter::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vcr_cddip_cr_sa_counter::~Vcr_cddip_cr_sa_counter() {
}
