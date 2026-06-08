// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip_cr_xp10_decompPKG.h"

// Parameter definitions for Vcr_cddip_cr_xp10_decompPKG
constexpr VlUnpacked<CData/*2:0*/, 32> Vcr_cddip_cr_xp10_decompPKG::HLIT_EXTRA_BITS;
constexpr VlUnpacked<SData/*8:0*/, 32> Vcr_cddip_cr_xp10_decompPKG::HLIT_BASE_LENGTH;
constexpr VlUnpacked<CData/*3:0*/, 32> Vcr_cddip_cr_xp10_decompPKG::HDIST_EXTRA_BITS;
constexpr VlUnpacked<SData/*14:0*/, 32> Vcr_cddip_cr_xp10_decompPKG::HDIST_BASE_DISTANCE;


void Vcr_cddip_cr_xp10_decompPKG___ctor_var_reset(Vcr_cddip_cr_xp10_decompPKG* vlSelf);

Vcr_cddip_cr_xp10_decompPKG::Vcr_cddip_cr_xp10_decompPKG(Vcr_cddip__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vcr_cddip_cr_xp10_decompPKG___ctor_var_reset(this);
}

void Vcr_cddip_cr_xp10_decompPKG::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vcr_cddip_cr_xp10_decompPKG::~Vcr_cddip_cr_xp10_decompPKG() {
}
