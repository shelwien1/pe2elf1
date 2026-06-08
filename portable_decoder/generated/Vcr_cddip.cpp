// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vcr_cddip__pch.h"

//============================================================
// Constructors

Vcr_cddip::Vcr_cddip(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vcr_cddip__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , ib_tready{vlSymsp->TOP.ib_tready}
    , ob_tvalid{vlSymsp->TOP.ob_tvalid}
    , ob_tlast{vlSymsp->TOP.ob_tlast}
    , ob_tid{vlSymsp->TOP.ob_tid}
    , ob_tstrb{vlSymsp->TOP.ob_tstrb}
    , ob_tuser{vlSymsp->TOP.ob_tuser}
    , sch_update_tvalid{vlSymsp->TOP.sch_update_tvalid}
    , sch_update_tlast{vlSymsp->TOP.sch_update_tlast}
    , sch_update_tuser{vlSymsp->TOP.sch_update_tuser}
    , sch_update_tdata{vlSymsp->TOP.sch_update_tdata}
    , apb_pready{vlSymsp->TOP.apb_pready}
    , apb_pslverr{vlSymsp->TOP.apb_pslverr}
    , cddip_int{vlSymsp->TOP.cddip_int}
    , cddip_idle{vlSymsp->TOP.cddip_idle}
    , rst_n{vlSymsp->TOP.rst_n}
    , scan_en{vlSymsp->TOP.scan_en}
    , scan_mode{vlSymsp->TOP.scan_mode}
    , scan_rst_n{vlSymsp->TOP.scan_rst_n}
    , ovstb{vlSymsp->TOP.ovstb}
    , lvm{vlSymsp->TOP.lvm}
    , mlvm{vlSymsp->TOP.mlvm}
    , ib_tvalid{vlSymsp->TOP.ib_tvalid}
    , ib_tlast{vlSymsp->TOP.ib_tlast}
    , ib_tid{vlSymsp->TOP.ib_tid}
    , ib_tstrb{vlSymsp->TOP.ib_tstrb}
    , ib_tuser{vlSymsp->TOP.ib_tuser}
    , ob_tready{vlSymsp->TOP.ob_tready}
    , sch_update_tready{vlSymsp->TOP.sch_update_tready}
    , apb_psel{vlSymsp->TOP.apb_psel}
    , apb_penable{vlSymsp->TOP.apb_penable}
    , apb_pwrite{vlSymsp->TOP.apb_pwrite}
    , dbg_cmd_disable{vlSymsp->TOP.dbg_cmd_disable}
    , xp9_disable{vlSymsp->TOP.xp9_disable}
    , apb_prdata{vlSymsp->TOP.apb_prdata}
    , apb_paddr{vlSymsp->TOP.apb_paddr}
    , apb_pwdata{vlSymsp->TOP.apb_pwdata}
    , ob_tdata{vlSymsp->TOP.ob_tdata}
    , ib_tdata{vlSymsp->TOP.ib_tdata}
    , __PVT__cr_xp10_decompPKG{vlSymsp->TOP.__PVT__cr_xp10_decompPKG}
    , __PVT____024unit{vlSymsp->TOP.__PVT____024unit}
    , __PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo}
    , __PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm}
    , __PVT__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top}
    , __PVT__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm}
    , __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_id}
    , __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice}
    , __PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__7__KET____DOT__u_reg_slice}
    , __PVT__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top}
    , __PVT__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob}
    , __PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i}
    , __PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i{vlSymsp->TOP.__PVT__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vcr_cddip::Vcr_cddip(const char* _vcname__)
    : Vcr_cddip(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vcr_cddip::~Vcr_cddip() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vcr_cddip___024root___eval_debug_assertions(Vcr_cddip___024root* vlSelf);
#endif  // VL_DEBUG
void Vcr_cddip___024root___eval_static(Vcr_cddip___024root* vlSelf);
void Vcr_cddip___024root___eval_initial(Vcr_cddip___024root* vlSelf);
void Vcr_cddip___024root___eval_settle(Vcr_cddip___024root* vlSelf);
void Vcr_cddip___024root___eval(Vcr_cddip___024root* vlSelf);

void Vcr_cddip::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vcr_cddip::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vcr_cddip___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vcr_cddip___024root___eval_static(&(vlSymsp->TOP));
        Vcr_cddip___024root___eval_initial(&(vlSymsp->TOP));
        Vcr_cddip___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vcr_cddip___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vcr_cddip::eventsPending() { return false; }

uint64_t Vcr_cddip::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "%Error: No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vcr_cddip::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vcr_cddip___024root___eval_final(Vcr_cddip___024root* vlSelf);

VL_ATTR_COLD void Vcr_cddip::final() {
    Vcr_cddip___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vcr_cddip::hierName() const { return vlSymsp->name(); }
const char* Vcr_cddip::modelName() const { return "Vcr_cddip"; }
unsigned Vcr_cddip::threads() const { return 1; }
void Vcr_cddip::prepareClone() const { contextp()->prepareClone(); }
void Vcr_cddip::atClone() const {
    contextp()->threadPoolpOnClone();
}

//============================================================
// Trace configuration

VL_ATTR_COLD void Vcr_cddip::trace(VerilatedVcdC* tfp, int levels, int options) {
    vl_fatal(__FILE__, __LINE__, __FILE__,"'Vcr_cddip::trace()' called on model that was Verilated without --trace option");
}
