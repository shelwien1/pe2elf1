// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_AXI_CHANNEL_REG_SLICE__PF20_H0_H_
#define VERILATED_VCR_CDDIP_AXI_CHANNEL_REG_SLICE__PF20_H0_H_  // guard

#include "verilated.h"


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_axi_channel_reg_slice__Pf20_H0 final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(aclk,0,0);
    VL_IN8(aresetn,0,0);
    VL_OUT8(ready_src,0,0);
    VL_OUT8(valid_dst,0,0);
    VL_IN8(valid_src,0,0);
    VL_IN8(ready_dst,0,0);
    CData/*0:0*/ __PVT__full__DOT__r_wptr;
    CData/*0:0*/ __PVT__full__DOT__c_wptr;
    CData/*0:0*/ __PVT__full__DOT__r_full;
    CData/*0:0*/ __PVT__full__DOT__c_full;
    CData/*0:0*/ __PVT__full__DOT__r_empty;
    CData/*0:0*/ __PVT__full__DOT__c_empty;
    CData/*0:0*/ __PVT__full__DOT__r_enable;
    VL_OUTW(payload_dst,3871,0,121);
    VL_INW(payload_src,3871,0,121);
    VlWide<16>/*483:0*/ __PVT__full__DOT__r_rptr;
    VlWide<16>/*483:0*/ __PVT__full__DOT__c_rptr;
    VlUnpacked<VlWide<121>/*3871:0*/, 2> __PVT__full__DOT__r_payload;

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_axi_channel_reg_slice__Pf20_H0(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_axi_channel_reg_slice__Pf20_H0();
    VL_UNCOPYABLE(Vcr_cddip_axi_channel_reg_slice__Pf20_H0);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
