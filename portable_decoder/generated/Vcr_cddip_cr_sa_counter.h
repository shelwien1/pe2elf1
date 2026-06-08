// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_CR_SA_COUNTER_H_
#define VERILATED_VCR_CDDIP_CR_SA_COUNTER_H_  // guard

#include "verilated.h"


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_cr_sa_counter final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst_n,0,0);
    VL_IN8(sa_clear,0,0);
    VL_IN8(sa_snap,0,0);
    VL_IN16(sa_event_sel,9,0);
    VL_OUT64(sa_count,49,0);
    VL_OUT64(sa_snapshot,49,0);
    QData/*63:0*/ __PVT__sa_mux1;
    VL_IN64(sa_events[16],63,0);

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_cr_sa_counter(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_cr_sa_counter();
    VL_UNCOPYABLE(Vcr_cddip_cr_sa_counter);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
