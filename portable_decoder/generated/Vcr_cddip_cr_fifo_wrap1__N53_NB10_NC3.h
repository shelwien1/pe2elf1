// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP_CR_FIFO_WRAP1__N53_NB10_NC3_H_
#define VERILATED_VCR_CDDIP_CR_FIFO_WRAP1__N53_NB10_NC3_H_  // guard

#include "verilated.h"


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3 final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst_n,0,0);
    VL_OUT8(full,0,0);
    VL_OUT8(afull,0,0);
    VL_OUT8(empty,0,0);
    VL_OUT8(aempty,0,0);
    VL_IN8(wen,0,0);
    VL_IN8(ren,0,0);
    CData/*0:0*/ __PVT__afull_r;
    CData/*0:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    CData/*0:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty;
    CData/*0:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
    CData/*0:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full;
    CData/*4:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    CData/*4:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots;
    CData/*4:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    CData/*4:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots;
    CData/*3:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    CData/*3:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr;
    CData/*3:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    CData/*3:0*/ __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr;
    CData/*0:0*/ __Vdly__afull_r;
    VL_OUTW(rdata,82,0,3);
    VL_INW(wdata,82,0,3);
    VlUnpacked<VlWide<3>/*82:0*/, 16> __PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data;

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3();
    VL_UNCOPYABLE(Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
