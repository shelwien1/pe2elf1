// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Primary model header
//
// This header should be included by all source files instantiating the design.
// The class here is then constructed to instantiate the design.
// See the Verilator manual for examples.

#ifndef VERILATED_VCR_CDDIP_H_
#define VERILATED_VCR_CDDIP_H_  // guard

#include "verilated.h"

class Vcr_cddip__Syms;
class Vcr_cddip___024root;
class Vcr_cddip___024unit;
class Vcr_cddip_axi_channel_reg_slice__Pf20_H0;
class Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3;
class Vcr_cddip_cr_sa_counter;
class Vcr_cddip_cr_tlvp_dsm;
class Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8;
class Vcr_cddip_cr_tlvp_id;
class Vcr_cddip_cr_tlvp_top;
class Vcr_cddip_cr_xp10_decompPKG;


// This class is the main interface to the Verilated model
class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip VL_NOT_FINAL : public VerilatedModel {
  private:
    // Symbol table holding complete model state (owned by this class)
    Vcr_cddip__Syms* const vlSymsp;

  public:

    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(&clk,0,0);
    VL_OUT8(&ib_tready,0,0);
    VL_OUT8(&ob_tvalid,0,0);
    VL_OUT8(&ob_tlast,0,0);
    VL_OUT8(&ob_tid,0,0);
    VL_OUT8(&ob_tstrb,7,0);
    VL_OUT8(&ob_tuser,7,0);
    VL_OUT8(&sch_update_tvalid,0,0);
    VL_OUT8(&sch_update_tlast,0,0);
    VL_OUT8(&sch_update_tuser,1,0);
    VL_OUT8(&sch_update_tdata,7,0);
    VL_OUT8(&apb_pready,0,0);
    VL_OUT8(&apb_pslverr,0,0);
    VL_OUT8(&cddip_int,0,0);
    VL_OUT8(&cddip_idle,0,0);
    VL_IN8(&rst_n,0,0);
    VL_IN8(&scan_en,0,0);
    VL_IN8(&scan_mode,0,0);
    VL_IN8(&scan_rst_n,0,0);
    VL_IN8(&ovstb,0,0);
    VL_IN8(&lvm,0,0);
    VL_IN8(&mlvm,0,0);
    VL_IN8(&ib_tvalid,0,0);
    VL_IN8(&ib_tlast,0,0);
    VL_IN8(&ib_tid,0,0);
    VL_IN8(&ib_tstrb,7,0);
    VL_IN8(&ib_tuser,7,0);
    VL_IN8(&ob_tready,0,0);
    VL_IN8(&sch_update_tready,0,0);
    VL_IN8(&apb_psel,0,0);
    VL_IN8(&apb_penable,0,0);
    VL_IN8(&apb_pwrite,0,0);
    VL_IN8(&dbg_cmd_disable,0,0);
    VL_IN8(&xp9_disable,0,0);
    VL_OUT(&apb_prdata,31,0);
    VL_IN(&apb_paddr,19,0);
    VL_IN(&apb_pwdata,31,0);
    VL_OUT64(&ob_tdata,63,0);
    VL_IN64(&ib_tdata,63,0);

    // CELLS
    // Public to allow access to /* verilator public */ items.
    // Otherwise the application code can consider these internals.
    Vcr_cddip_cr_xp10_decompPKG* const __PVT__cr_xp10_decompPKG;
    Vcr_cddip___024unit* const __PVT____024unit;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo;
    Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8* const __PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm;
    Vcr_cddip_cr_tlvp_top* const __PVT__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top;
    Vcr_cddip_cr_tlvp_dsm* const __PVT__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm;
    Vcr_cddip_cr_tlvp_id* const __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id;
    Vcr_cddip_axi_channel_reg_slice__Pf20_H0* const __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice;
    Vcr_cddip_axi_channel_reg_slice__Pf20_H0* const __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice;
    Vcr_cddip_cr_tlvp_top* const __PVT__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top;
    Vcr_cddip_cr_tlvp_dsm__N0_NB0_NC0_ND8* const __PVT__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo;
    Vcr_cddip_cr_tlvp_id* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob;
    Vcr_cddip_cr_fifo_wrap1__N53_NB10_NC3* const __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i;
    Vcr_cddip_cr_sa_counter* const __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i;

    // Root instance pointer to allow access to model internals,
    // including inlined /* verilator public_flat_* */ items.
    Vcr_cddip___024root* const rootp;

    // CONSTRUCTORS
    /// Construct the model; called by application code
    /// If contextp is null, then the model will use the default global context
    /// If name is "", then makes a wrapper with a
    /// single model invisible with respect to DPI scope names.
    explicit Vcr_cddip(VerilatedContext* contextp, const char* name = "TOP");
    explicit Vcr_cddip(const char* name = "TOP");
    /// Destroy the model; called (often implicitly) by application code
    virtual ~Vcr_cddip();
  private:
    VL_UNCOPYABLE(Vcr_cddip);  ///< Copying not allowed

  public:
    // API METHODS
    /// Evaluate the model.  Application must call when inputs change.
    void eval() { eval_step(); }
    /// Evaluate when calling multiple units/models per time step.
    void eval_step();
    /// Evaluate at end of a timestep for tracing, when using eval_step().
    /// Application must call after all eval() and before time changes.
    void eval_end_step() {}
    /// Simulation complete, run final blocks.  Application must call on completion.
    void final();
    /// Are there scheduled events to handle?
    bool eventsPending();
    /// Returns time at next time slot. Aborts if !eventsPending()
    uint64_t nextTimeSlot();
    /// Trace signals in the model; called by application code
    void trace(VerilatedVcdC* tfp, int levels, int options = 0);
    /// Retrieve name of this model instance (as passed to constructor).
    const char* name() const;

    // Abstract methods from VerilatedModel
    const char* hierName() const override final;
    const char* modelName() const override final;
    unsigned threads() const override final;
    /// Prepare for cloning the model at the process level (e.g. fork in Linux)
    /// Release necessary resources. Called before cloning.
    void prepareClone() const;
    /// Re-init after cloning the model at the process level (e.g. fork in Linux)
    /// Re-allocate necessary resources. Called after cloning.
    void atClone() const;
};

#endif  // guard
