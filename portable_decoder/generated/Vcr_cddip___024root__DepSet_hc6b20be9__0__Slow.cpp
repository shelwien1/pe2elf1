// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip___024root.h"

VL_ATTR_COLD void Vcr_cddip___024root___eval_static__TOP(Vcr_cddip___024root* vlSelf);

VL_ATTR_COLD void Vcr_cddip___024root___eval_static(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___eval_static\n"); );
    // Body
    Vcr_cddip___024root___eval_static__TOP(vlSelf);
}

VL_ATTR_COLD void Vcr_cddip___024root___eval_static__TOP(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___eval_static__TOP\n"); );
    // Body
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__stb_tog_dly = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__stb_tog_dly = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__stb_tog_dly = 0U;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__stb_tog_dly = 0U;
}

VL_ATTR_COLD void Vcr_cddip___024root___eval_final__TOP(Vcr_cddip___024root* vlSelf);

VL_ATTR_COLD void Vcr_cddip___024root___eval_final(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___eval_final\n"); );
    // Body
    Vcr_cddip___024root___eval_final__TOP(vlSelf);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcr_cddip___024root___dump_triggers__stl(Vcr_cddip___024root* vlSelf);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vcr_cddip___024root___eval_phase__stl(Vcr_cddip___024root* vlSelf);

VL_ATTR_COLD void Vcr_cddip___024root___eval_settle(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___eval_settle\n"); );
    // Init
    IData/*31:0*/ __VstlIterCount;
    CData/*0:0*/ __VstlContinue;
    // Body
    __VstlIterCount = 0U;
    vlSelf->__VstlFirstIteration = 1U;
    __VstlContinue = 1U;
    while (__VstlContinue) {
        if (VL_UNLIKELY((0x64U < __VstlIterCount))) {
#ifdef VL_DEBUG
            Vcr_cddip___024root___dump_triggers__stl(vlSelf);
#endif
            VL_FATAL_MT("/home/user/pe2elf1/project-zipline/rtl/cr_cddip/cr_cddip.v", 19, "", "Settle region did not converge.");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        __VstlContinue = 0U;
        if (Vcr_cddip___024root___eval_phase__stl(vlSelf)) {
            __VstlContinue = 1U;
        }
        vlSelf->__VstlFirstIteration = 0U;
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcr_cddip___024root___dump_triggers__stl(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VstlTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
    if ((2ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 1 is active: @([hybrid] cr_cddip.rbus_ring_i)\n");
    }
    if ((4ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 2 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_support.isf_fifo_empty_mod)\n");
    }
    if ((8ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 3 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_tlv_mods.user_footer_coding)\n");
    }
    if ((0x10ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 4 is active: @([hybrid] cr_cddip.u_cr_prefix_attach.u_cr_prefix_attach_core.prefix_attach_core.u_cr_prefix_attach_ibp.ibp_prefix_num_d)\n");
    }
    if ((0x20ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 5 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_lz77d_im.hw_cs)\n");
    }
    if ((0x40ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 6 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_xpd_im.hw_cs)\n");
    }
    if ((0x80ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 7 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_htf_bl_im.credit_available)\n");
    }
    if ((0x100ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 8 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.invalid_hdr_addr or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x200ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 9 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x400ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 10 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.bhp_inst.dflate_hdr.all_valid)\n");
    }
    if ((0x800ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 11 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.predef_bl_req_ready or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.hdr_bits_err)\n");
    }
    if ((0x1000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 12 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_hdr_fifo.hdr_fifo_ren)\n");
    }
    if ((0x2000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 13 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_symtab_dec.retro_go)\n");
    }
    if ((0x4000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 14 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_table_writer.hist_complete)\n");
    }
    if ((0x8000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 15 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_ld.pipe_dst_ready)\n");
    }
    if ((0x10000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 16 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_sp.pipe_dst_ready)\n");
    }
    if ((0x20000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 17 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.df_dbg_empty_mod)\n");
    }
    if ((0x40000ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 18 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.pf_dbg_empty_mod)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vcr_cddip___024root___stl_sequent__TOP__5(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___stl_sequent__TOP__5\n"); );
    // Body
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0U;
    if ((1U & ((vlSelf->cr_cddip__DOT__rbus_ring_i[2U][2U] 
                >> 3U) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                          [2U][1U] >> 2U)))) {
        if ((1U & (~ ((0x20000U <= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                [2U][2U] 
                                                >> 4U))) 
                      & (0x2007fU >= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                  [2U][2U] 
                                                  >> 4U))))))) {
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [2U][2U] >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                = ((vlSelf->cr_cddip__DOT__rbus_ring_i
                    [2U][2U] << 0x1dU) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                          [2U][1U] 
                                          >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [2U][1U] >> 2U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o 
                = (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                               [2U][2U] >> 4U));
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0U;
    if ((1U & ((vlSelf->cr_cddip__DOT__rbus_ring_i[5U][2U] 
                >> 3U) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                          [5U][1U] >> 2U)))) {
        if ((1U & (~ ((0x70000U <= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                [5U][2U] 
                                                >> 4U))) 
                      & (0x7007fU >= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                  [5U][2U] 
                                                  >> 4U))))))) {
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [5U][2U] >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                = ((vlSelf->cr_cddip__DOT__rbus_ring_i
                    [5U][2U] << 0x1dU) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                          [5U][1U] 
                                          >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [5U][1U] >> 2U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o 
                = (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                               [5U][2U] >> 4U));
        }
    }
}

VL_ATTR_COLD void Vcr_cddip___024root___stl_comb__TOP__14(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___stl_comb__TOP__14\n"); );
    // Init
    IData/*19:0*/ cr_cddip__DOT____Vlvbound_he8fc44d5__0;
    cr_cddip__DOT____Vlvbound_he8fc44d5__0 = 0;
    CData/*0:0*/ cr_cddip__DOT____Vlvbound_h0b575c8c__0;
    cr_cddip__DOT____Vlvbound_h0b575c8c__0 = 0;
    IData/*31:0*/ cr_cddip__DOT____Vlvbound_h2a7e5b54__0;
    cr_cddip__DOT____Vlvbound_h2a7e5b54__0 = 0;
    CData/*0:0*/ cr_cddip__DOT____Vlvbound_h0b6519c7__0;
    cr_cddip__DOT____Vlvbound_h0b6519c7__0 = 0;
    VlWide<3>/*87:0*/ cr_cddip__DOT____Vlvbound_hd3ca9d54__1;
    VL_ZERO_W(88, cr_cddip__DOT____Vlvbound_hd3ca9d54__1);
    VlWide<3>/*95:0*/ __Vtemp_1;
    VlWide<3>/*95:0*/ __Vtemp_4;
    // Body
    __Vtemp_1[1U] = ((vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                      << 3U) | (IData)(((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o)) 
                                          << 0x22U) 
                                         | (((QData)((IData)(
                                                             (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                               | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                               ? 
                                                              (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write)) 
                                                                & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack))
                                                                ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data
                                                                : 0U)
                                                               : 
                                                              ((vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                [2U][1U] 
                                                                << 0x1eU) 
                                                               | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                  [2U][0U] 
                                                                  >> 2U))))) 
                                             << 2U) 
                                            | (QData)((IData)(
                                                              ((2U 
                                                                & ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                     | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                     ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack)
                                                                     : 
                                                                    (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                     [2U][0U] 
                                                                     >> 1U)) 
                                                                   << 1U)) 
                                                               | (1U 
                                                                  & (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                      | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                      ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack)
                                                                      : 
                                                                     vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                     [2U][0U]))))))) 
                                        >> 0x20U)));
    vlSelf->cr_cddip__DOT__rbus_ring_o[2U][0U] = (IData)(
                                                         (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o)) 
                                                           << 0x22U) 
                                                          | (((QData)((IData)(
                                                                              (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                ? 
                                                                               (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write)) 
                                                                                & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack))
                                                                                 ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data
                                                                                 : 0U)
                                                                                : 
                                                                               ((vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [2U][1U] 
                                                                                << 0x1eU) 
                                                                                | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [2U][0U] 
                                                                                >> 2U))))) 
                                                              << 2U) 
                                                             | (QData)((IData)(
                                                                               ((2U 
                                                                                & ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                 ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack)
                                                                                 : 
                                                                                (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [2U][0U] 
                                                                                >> 1U)) 
                                                                                << 1U)) 
                                                                                | (1U 
                                                                                & (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                 ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack)
                                                                                 : 
                                                                                vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [2U][0U]))))))));
    vlSelf->cr_cddip__DOT__rbus_ring_o[2U][1U] = __Vtemp_1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_o[2U][2U] = ((vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o 
                                                   << 4U) 
                                                  | (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o) 
                                                      << 3U) 
                                                     | (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                                                        >> 0x1dU)));
    __Vtemp_4[1U] = ((vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                      << 3U) | (IData)(((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o)) 
                                          << 0x22U) 
                                         | (((QData)((IData)(
                                                             (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                               | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                               ? 
                                                              (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write)) 
                                                                & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack))
                                                                ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data
                                                                : 0U)
                                                               : 
                                                              ((vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                [5U][1U] 
                                                                << 0x1eU) 
                                                               | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                  [5U][0U] 
                                                                  >> 2U))))) 
                                             << 2U) 
                                            | (QData)((IData)(
                                                              ((2U 
                                                                & ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                     | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                     ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack)
                                                                     : 
                                                                    (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                     [5U][0U] 
                                                                     >> 1U)) 
                                                                   << 1U)) 
                                                               | (1U 
                                                                  & (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                      | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                      ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack)
                                                                      : 
                                                                     vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                     [5U][0U]))))))) 
                                        >> 0x20U)));
    vlSelf->cr_cddip__DOT__rbus_ring_o[5U][0U] = (IData)(
                                                         (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o)) 
                                                           << 0x22U) 
                                                          | (((QData)((IData)(
                                                                              (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                ? 
                                                                               (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write)) 
                                                                                & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack))
                                                                                 ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data
                                                                                 : 0U)
                                                                                : 
                                                                               ((vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [5U][1U] 
                                                                                << 0x1eU) 
                                                                                | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [5U][0U] 
                                                                                >> 2U))))) 
                                                              << 2U) 
                                                             | (QData)((IData)(
                                                                               ((2U 
                                                                                & ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                 ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack)
                                                                                 : 
                                                                                (vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [5U][0U] 
                                                                                >> 1U)) 
                                                                                << 1U)) 
                                                                                | (1U 
                                                                                & (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack) 
                                                                                | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack))
                                                                                 ? (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack)
                                                                                 : 
                                                                                vlSelf->cr_cddip__DOT__rbus_ring_i
                                                                                [5U][0U]))))))));
    vlSelf->cr_cddip__DOT__rbus_ring_o[5U][1U] = __Vtemp_4[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_o[5U][2U] = ((vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o 
                                                   << 4U) 
                                                  | (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o) 
                                                      << 3U) 
                                                     | (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o 
                                                        >> 0x1dU)));
    vlSelf->cr_cddip__DOT__rbus_o[0U] = ((1U & vlSelf->cr_cddip__DOT__rbus_o[0U]) 
                                         | ((IData)(
                                                    (0x3ffffffffULL 
                                                     & (((QData)((IData)(
                                                                         vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                         [9U][1U])) 
                                                         << 0x1fU) 
                                                        | ((QData)((IData)(
                                                                           vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                           [9U][0U])) 
                                                           >> 1U)))) 
                                            << 1U));
    vlSelf->cr_cddip__DOT__rbus_o[1U] = ((0xfffffff8U 
                                          & vlSelf->cr_cddip__DOT__rbus_o[1U]) 
                                         | (((IData)(
                                                     (0x3ffffffffULL 
                                                      & (((QData)((IData)(
                                                                          vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                          [9U][1U])) 
                                                          << 0x1fU) 
                                                         | ((QData)((IData)(
                                                                            vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                            [9U][0U])) 
                                                            >> 1U)))) 
                                             >> 0x1fU) 
                                            | ((IData)(
                                                       ((0x3ffffffffULL 
                                                         & (((QData)((IData)(
                                                                             vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                             [9U][1U])) 
                                                             << 0x1fU) 
                                                            | ((QData)((IData)(
                                                                               vlSelf->cr_cddip__DOT__rbus_ring_o
                                                                               [9U][0U])) 
                                                               >> 1U))) 
                                                        >> 0x20U)) 
                                               << 1U)));
    vlSelf->cr_cddip__DOT__rbus_o[2U] = ((0xfffff7U 
                                          & vlSelf->cr_cddip__DOT__rbus_o[2U]) 
                                         | (8U & vlSelf->cr_cddip__DOT__rbus_ring_o
                                            [9U][2U]));
    vlSelf->cr_cddip__DOT__rbus_o[0U] = ((0xfffffffeU 
                                          & vlSelf->cr_cddip__DOT__rbus_o[0U]) 
                                         | (1U & vlSelf->cr_cddip__DOT__rbus_ring_o
                                            [9U][0U]));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][0U] = 0U;
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][1U] = 0U;
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][2U] = 0U;
    cr_cddip__DOT____Vlvbound_he8fc44d5__0 = (0xfffffU 
                                              & (vlSelf->cr_cddip__DOT__rbus_i[2U] 
                                                 >> 4U));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][2U] = ((0xfU 
                                                   & vlSelf->cr_cddip__DOT__rbus_ring_i
                                                   [0U][2U]) 
                                                  | (0xffffffU 
                                                     & (cr_cddip__DOT____Vlvbound_he8fc44d5__0 
                                                        << 4U)));
    cr_cddip__DOT____Vlvbound_h0b575c8c__0 = (1U & 
                                              (vlSelf->cr_cddip__DOT__rbus_i[2U] 
                                               >> 3U));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][2U] = ((0xfffff7U 
                                                   & vlSelf->cr_cddip__DOT__rbus_ring_i
                                                   [0U][2U]) 
                                                  | (0xffffffU 
                                                     & ((IData)(cr_cddip__DOT____Vlvbound_h0b575c8c__0) 
                                                        << 3U)));
    cr_cddip__DOT____Vlvbound_h2a7e5b54__0 = ((vlSelf->cr_cddip__DOT__rbus_i[2U] 
                                               << 0x1dU) 
                                              | (vlSelf->cr_cddip__DOT__rbus_i[1U] 
                                                 >> 3U));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][1U] = ((7U 
                                                   & vlSelf->cr_cddip__DOT__rbus_ring_i
                                                   [0U][1U]) 
                                                  | (cr_cddip__DOT____Vlvbound_h2a7e5b54__0 
                                                     << 3U));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][2U] = ((0xfffff8U 
                                                   & vlSelf->cr_cddip__DOT__rbus_ring_i
                                                   [0U][2U]) 
                                                  | (0xffffffU 
                                                     & (cr_cddip__DOT____Vlvbound_h2a7e5b54__0 
                                                        >> 0x1dU)));
    cr_cddip__DOT____Vlvbound_h0b6519c7__0 = (1U & 
                                              (vlSelf->cr_cddip__DOT__rbus_i[1U] 
                                               >> 2U));
    vlSelf->cr_cddip__DOT__rbus_ring_i[0U][1U] = ((0xfffffffbU 
                                                   & vlSelf->cr_cddip__DOT__rbus_ring_i
                                                   [0U][1U]) 
                                                  | ((IData)(cr_cddip__DOT____Vlvbound_h0b6519c7__0) 
                                                     << 2U));
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [0U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [0U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [0U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[1U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[1U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[1U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [1U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [1U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [1U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[2U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[2U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[2U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [2U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [2U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [2U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[3U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[3U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[3U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [3U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [3U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [3U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[4U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[4U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[4U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [4U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [4U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [4U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[5U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[5U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[5U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [5U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [5U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [5U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[6U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[6U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[6U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [6U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [6U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [6U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[7U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[7U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[7U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [7U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [7U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [7U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[8U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[8U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[8U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [8U][0U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [8U][1U];
    cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U] = vlSelf->cr_cddip__DOT__rbus_ring_o
        [8U][2U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[9U][0U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[0U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[9U][1U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[1U];
    vlSelf->cr_cddip__DOT__rbus_ring_i[9U][2U] = cr_cddip__DOT____Vlvbound_hd3ca9d54__1[2U];
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_addr = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_addr = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb = 0U;
    if ((1U & ((vlSelf->cr_cddip__DOT__rbus_ring_i[2U][2U] 
                >> 3U) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                          [2U][1U] >> 2U)))) {
        if (((0x20000U <= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                       [2U][2U] >> 4U))) 
             & (0x2007fU >= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                         [2U][2U] >> 4U))))) {
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data 
                = ((vlSelf->cr_cddip__DOT__rbus_ring_i
                    [2U][2U] << 0x1dU) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                          [2U][1U] 
                                          >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_addr 
                = (0x7fU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                            [2U][2U] >> 4U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [2U][2U] >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [2U][1U] >> 2U));
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb = 0U;
    if ((1U & ((vlSelf->cr_cddip__DOT__rbus_ring_i[5U][2U] 
                >> 3U) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                          [5U][1U] >> 2U)))) {
        if (((0x70000U <= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                       [5U][2U] >> 4U))) 
             & (0x7007fU >= (0xfffffU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                                         [5U][2U] >> 4U))))) {
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data 
                = ((vlSelf->cr_cddip__DOT__rbus_ring_i
                    [5U][2U] << 0x1dU) | (vlSelf->cr_cddip__DOT__rbus_ring_i
                                          [5U][1U] 
                                          >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_addr 
                = (0x7fU & (vlSelf->cr_cddip__DOT__rbus_ring_i
                            [5U][2U] >> 4U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [5U][2U] >> 3U));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb 
                = (1U & (vlSelf->cr_cddip__DOT__rbus_ring_i
                         [5U][1U] >> 2U));
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_next_state 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb)
            ? 1U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb)
                     ? 5U : ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))
                              ? 3U : ((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))
                                       ? 7U : 0U))));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_next_state 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb)
            ? 1U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb)
                     ? 5U : ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))
                              ? 3U : ((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))
                                       ? 7U : 0U))));
}

VL_ATTR_COLD void Vcr_cddip___024root___eval_triggers__stl(Vcr_cddip___024root* vlSelf);
VL_ATTR_COLD void Vcr_cddip___024root___eval_stl(Vcr_cddip___024root* vlSelf);

VL_ATTR_COLD bool Vcr_cddip___024root___eval_phase__stl(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___eval_phase__stl\n"); );
    // Init
    CData/*0:0*/ __VstlExecute;
    // Body
    Vcr_cddip___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = vlSelf->__VstlTriggered.any();
    if (__VstlExecute) {
        Vcr_cddip___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcr_cddip___024root___dump_triggers__ico(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___dump_triggers__ico\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VicoTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VicoTriggered.word(0U))) {
        VL_DBG_MSGF("         'ico' region trigger index 0 is active: Internal 'ico' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcr_cddip___024root___dump_triggers__act(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VactTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 0 is active: @([hybrid] cr_cddip.rbus_ring_i)\n");
    }
    if ((2ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 1 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_support.isf_fifo_empty_mod)\n");
    }
    if ((4ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 2 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_tlv_mods.user_footer_coding)\n");
    }
    if ((8ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 3 is active: @([hybrid] cr_cddip.u_cr_prefix_attach.u_cr_prefix_attach_core.prefix_attach_core.u_cr_prefix_attach_ibp.ibp_prefix_num_d)\n");
    }
    if ((0x10ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 4 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_lz77d_im.hw_cs)\n");
    }
    if ((0x20ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 5 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_xpd_im.hw_cs)\n");
    }
    if ((0x40ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 6 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_htf_bl_im.credit_available)\n");
    }
    if ((0x80ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 7 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.invalid_hdr_addr or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x100ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 8 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x200ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 9 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.bhp_inst.dflate_hdr.all_valid)\n");
    }
    if ((0x400ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 10 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.predef_bl_req_ready or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.hdr_bits_err)\n");
    }
    if ((0x800ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 11 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_hdr_fifo.hdr_fifo_ren)\n");
    }
    if ((0x1000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 12 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_symtab_dec.retro_go)\n");
    }
    if ((0x2000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 13 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_table_writer.hist_complete)\n");
    }
    if ((0x4000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 14 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_ld.pipe_dst_ready)\n");
    }
    if ((0x8000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 15 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_sp.pipe_dst_ready)\n");
    }
    if ((0x10000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 16 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.df_dbg_empty_mod)\n");
    }
    if ((0x20000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 17 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.pf_dbg_empty_mod)\n");
    }
    if ((0x40000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 18 is active: @(posedge clk or negedge cr_cddip.rst_sync_n)\n");
    }
    if ((0x80000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 19 is active: @(posedge clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 20 is active: @(posedge clk or negedge cr_cddip.u_cr_xp10_decomp.rst_sync_n)\n");
    }
}
#endif  // VL_DEBUG

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcr_cddip___024root___dump_triggers__nba(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___dump_triggers__nba\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VnbaTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 0 is active: @([hybrid] cr_cddip.rbus_ring_i)\n");
    }
    if ((2ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 1 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_support.isf_fifo_empty_mod)\n");
    }
    if ((4ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 2 is active: @([hybrid] cr_cddip.u_cr_isf.u_cr_isf_core.u_cr_isf_tlv_mods.user_footer_coding)\n");
    }
    if ((8ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 3 is active: @([hybrid] cr_cddip.u_cr_prefix_attach.u_cr_prefix_attach_core.prefix_attach_core.u_cr_prefix_attach_ibp.ibp_prefix_num_d)\n");
    }
    if ((0x10ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 4 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_lz77d_im.hw_cs)\n");
    }
    if ((0x20ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 5 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_xpd_im.hw_cs)\n");
    }
    if ((0x40ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 6 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.u_cr_xp10_decomp_regfile.u_htf_bl_im.credit_available)\n");
    }
    if ((0x80ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 7 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.invalid_hdr_addr or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x100ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 8 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.lfa_inst.sdd_fifo_wbus)\n");
    }
    if ((0x200ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 9 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.fe.bhp_inst.dflate_hdr.all_valid)\n");
    }
    if ((0x400ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 10 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.predef_bl_req_ready or [hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.hdr_bits_err)\n");
    }
    if ((0x800ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 11 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_hdr_fifo.hdr_fifo_ren)\n");
    }
    if ((0x1000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 12 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_symtab_dec.retro_go)\n");
    }
    if ((0x2000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 13 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.htf.u_table_writer.hist_complete)\n");
    }
    if ((0x4000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 14 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_ld.pipe_dst_ready)\n");
    }
    if ((0x8000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 15 is active: @([hybrid] cr_cddip.u_cr_xp10_decomp.xp10_decomp_rtl_start.u_cr_xp10_decomp_core.no_stub.hufd.sdd.u_sp.pipe_dst_ready)\n");
    }
    if ((0x10000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 16 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.df_dbg_empty_mod)\n");
    }
    if ((0x20000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 17 is active: @([hybrid] cr_cddip.u_cr_osf.u_cr_osf_core.u_cr_osf_dbg2fifo_ctl.pf_dbg_empty_mod)\n");
    }
    if ((0x40000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 18 is active: @(posedge clk or negedge cr_cddip.rst_sync_n)\n");
    }
    if ((0x80000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 19 is active: @(posedge clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 20 is active: @(posedge clk or negedge cr_cddip.u_cr_xp10_decomp.rst_sync_n)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vcr_cddip___024root___ctor_var_reset(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->ib_tready = 0;
    vlSelf->ob_tvalid = 0;
    vlSelf->ob_tlast = 0;
    vlSelf->ob_tid = 0;
    vlSelf->ob_tstrb = 0;
    vlSelf->ob_tuser = 0;
    vlSelf->ob_tdata = 0;
    vlSelf->sch_update_tvalid = 0;
    vlSelf->sch_update_tlast = 0;
    vlSelf->sch_update_tuser = 0;
    vlSelf->sch_update_tdata = 0;
    vlSelf->apb_prdata = 0;
    vlSelf->apb_pready = 0;
    vlSelf->apb_pslverr = 0;
    vlSelf->cddip_int = 0;
    vlSelf->cddip_idle = 0;
    vlSelf->clk = 0;
    vlSelf->rst_n = 0;
    vlSelf->scan_en = 0;
    vlSelf->scan_mode = 0;
    vlSelf->scan_rst_n = 0;
    vlSelf->ovstb = 0;
    vlSelf->lvm = 0;
    vlSelf->mlvm = 0;
    vlSelf->ib_tvalid = 0;
    vlSelf->ib_tlast = 0;
    vlSelf->ib_tid = 0;
    vlSelf->ib_tstrb = 0;
    vlSelf->ib_tuser = 0;
    vlSelf->ib_tdata = 0;
    vlSelf->ob_tready = 0;
    vlSelf->sch_update_tready = 0;
    vlSelf->apb_paddr = 0;
    vlSelf->apb_psel = 0;
    vlSelf->apb_penable = 0;
    vlSelf->apb_pwrite = 0;
    vlSelf->apb_pwdata = 0;
    vlSelf->dbg_cmd_disable = 0;
    vlSelf->xp9_disable = 0;
    vlSelf->cr_cddip__DOT__crcc0_sa_stat_events = 0;
    vlSelf->cr_cddip__DOT__crcg0_sa_stat_events = 0;
    vlSelf->cr_cddip__DOT__isf_sup_cqe_exit = 0;
    vlSelf->cr_cddip__DOT__isf_sup_cqe_rx = 0;
    vlSelf->cr_cddip__DOT__isf_sup_rqe_rx = 0;
    vlSelf->cr_cddip__DOT__osf_sup_cqe_exit = 0;
    vlSelf->cr_cddip__DOT__rst_sync_n = 0;
    vlSelf->cr_cddip__DOT__su_ready = 0;
    vlSelf->cr_cddip__DOT__sup_osf_halt = 0;
    vlSelf->cr_cddip__DOT__top_bimc_mstr_rst_n = 0;
    for (int __Vi0 = 0; __Vi0 < 10; ++__Vi0) {
        VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__rbus_ring_i[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 10; ++__Vi0) {
        VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__rbus_ring_o[__Vi0]);
    }
    VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__rbus_i);
    VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__rbus_o);
    vlSelf->cr_cddip__DOT__top_bimc_mstr_odat = 0;
    vlSelf->cr_cddip__DOT__top_bimc_mstr_osync = 0;
    vlSelf->cr_cddip__DOT____Vcellout__u_nx_rbus_apb__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT____Vcellout__u_nx_rbus_apb__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT____Vcellout__u_nx_rbus_apb__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT____Vcellout__u_nx_rbus_apb__rbus_addr_o = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT____Vcellinp__u_cr_isf__isf_ib_in);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT____Vcellout__u_cr_isf__isf_ob_out);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT____Vcellout__u_cr_prefix_attach__prefix_attach_ob_out);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT____Vcellout__u_cr_cg__cg_ob_out);
    vlSelf->cr_cddip__DOT____Vcellinp__u_cr_su__su_ob_in = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_paddr_reg = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_penable_reg = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_psel_reg = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_pwdata_reg = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_pwrite_reg = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_active = 0;
    vlSelf->cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_active_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__pre_cddip_int = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__pipe_stat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT____Vcellout__u_cr_cddip_support_regfile__im_consumed_xpd = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT____Vcellout__u_cr_cddip_support_regfile__im_consumed_htf_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT____Vcellout__u_cr_cddip_support_regfile__im_consumed_lz77d = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_core__DOT__pipe_cmd_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_core__DOT__isf_cmd_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_core__DOT__cqe_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_monitor = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_parity_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_cddip_int_control_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_cddip_int_control_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_parity_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_soft_rst = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__top_bimc_int = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__locl_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__im_available_xpd_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__im_available_lz77d_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__im_available_htf_bl_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_cr_cddip_support_regs__o_ctl = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_cr_cddip_support_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_mask_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_valid_ram_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_valid_ram_wr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_do_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__r_next_cddip_int_control_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__r_next_cddip_int_control_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__rs_next_cddip_int_control_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_monitor_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_cmd1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_rxcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_rxrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_pollrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_dbgcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgExtracted_h0b8b3dc6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgExtracted_h516d65cc__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgExtracted_hc60374d8__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgExtracted_h23b986f3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgTmp_hb6155f85__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT____VdfgTmp_ha6d16c09__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt_uncorrectable_ecc_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt_correctable_ecc_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt_parity_errors_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__number_of_memories = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_bimc_eccpar_debug_write_notify_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_sent_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_cmd2_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_bimc_cmd2_write_notify_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_cmd2_sent_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_cmd2_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt = 0;
    VL_ZERO_RESET_W(72, vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rdat);
    VL_ZERO_RESET_W(72, vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat);
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_chk = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bm_resp = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bm_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_rstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_global_config_bimc_mem_init_done_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cmd_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__mem_wr_init_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__mem_wr_init_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__eccpar_debug_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cpu_transmit_ev = 0;
    VL_ZERO_RESET_W(73, vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send);
    VL_ZERO_RESET_W(73, vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send);
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_mem = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__auto_poll_ecc_par_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__poll_ecc_par_timer = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_hi = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_lo = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_sb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__ib_bytes_cnt_amt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__ib_bytes_cnt_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__isf_fifo_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__single_step_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__aux_cmd_match0_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__aux_cmd_match1_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__aux_cmd_match2_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__aux_cmd_match3_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__axi_slv_ovfl = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ib_cmd_cnt_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ib_frame_cnt_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ib_prot_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_sys_stall_intr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ovfl_int = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ss_rd_ok = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__trigger_hit = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_sw_wdata);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_hw_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_hw_wr = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_sw_rdata);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT____Vcellout__u_isf_axi4s_slv__axi4s_slv_out);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT____Vcellout__u_isf_dbg_fifo__fifo_status_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT____Vcellout__u_isf_dbg_fifo__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT____Vcellout__u_isf_dbg_fifo__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT____Vcellout__u_cr_isf_support__isf_stat_events = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__axi4s_slv_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__wr_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__und_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__ovr_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__fifo_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__fifo_wr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__fifo_depth_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__fifo_rd_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__fifo_wr_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__hw_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__hw_re = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__wadd = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__din);
    for (int __Vi0 = 0; __Vi0 < 1024; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_fifo_full_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_ib_tlv_word0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_ob_tlv_word0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_ib_par_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_ob_par_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_trig_frz_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_trig_frz_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__sys_stall_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__use_wmark = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__req_wmark = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__sys_stall_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__stall_limit_hit = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_fifo_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_fifo_empty_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__ib_cmd_active = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__axi_slv_ovfl_d0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__tlv_word_cnt_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__tlv_word_cnt_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trigger_hit_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__tlv_word_cnt_match = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trig_word_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__tlv_word_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trig_word_hld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__pre_tlvp_fifo_rd_d0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_term_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__xfr_user_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__xfr_rdy = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ix_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__start_footer_pulse_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__start_footer_pulse = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_ld_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_done_clr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__start_footer_clr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ix = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_data_w0_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_rqe_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_cqe_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_cmd_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_frmd_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_done_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__start_footer_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_xp10_user_prefix_size_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_eot = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_cnt_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_ld_save = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_done_hld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__start_footer_hld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_cmd_debug_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__rqe_frame_size_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_mac0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_mac1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_mac2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_mac3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_cksum = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_mac0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_mac1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_mac2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_mac3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_cksum = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_nvme_raw_cksum_crc16t = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_comp_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__fgen_user_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_done_pulse = 0;
    for (int __Vi0 = 0; __Vi0 < 13; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_frmd_reg[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_vld_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_bytes_cnt_stb_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_bytes_cnt_amt_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_frame_cnt_stb_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_cmd_cnt_stb_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__prefix_err_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__prefix_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__prefix_err_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__prefix_err_frame = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__comp_match0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__comp_match1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__comp_match2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__comp_match3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__crypto_match0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__crypto_match1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__crypto_match2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__crypto_match3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_prot_error_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__rqe_que_grp_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__bytes_in_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__bytes_in_cnt_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__bytes_in_cnt_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_stats_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_comp_mode_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_frmd_out_type_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__out_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_enc_mac_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_raw_mac_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__footer_coding = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_footer_coding = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__fgen_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__fgen_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__rqe_trace_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_tlv_tdata_w1_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_tlv_tdata_w2_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_tlv_tdata_w0 = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cqe_tlv_w0_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cqe_tlv_w1_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__data_tlv_w0_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_tlv_w1);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_tlv_w2);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__frmd_w0_adj);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__fgen);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_term_tlv_cqe);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_tlv);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_prefix_tlv_w0);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_stall_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_stall = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT____Vlvbound_h46814511__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT____Vlvbound_h46814511__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT____Vlvbound_hf2c50de9__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT____Vlvbound_h6a97bf36__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__tlvp_ob_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__ib_agg_data_bytes_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_wr_data = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__ib_agg_bytes_cnt[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_ib_agg_data_bytes_cntr__counter_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_3_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_3_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_3_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_3_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_2_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_2_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_2_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_2_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_1_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_1_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_1_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_1_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_0_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_0_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_0_crypto_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_0_comp_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_trace_ctl_limits_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_trace_ctl_en_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_mask_hi_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_mask_lo_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_match_hi_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_match_lo_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_tlv_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_system_stall_limit_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__counter_a_int[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_stb_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_by_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_id_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_stb_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_stb_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_stb_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_by_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_by_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__count_by_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__selected = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__rd_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__wr_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__counter_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__stb_tog_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_h8d97bf4d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_hf1cf4032__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_hbfef4226__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_hbfef4226__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_hd7f3132e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_he9aadcae__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT____Vlvbound_he01da8e1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__o_debug_ss_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_debug_trig_tlv_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_trace_ctl_limits_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc64e_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc64e_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc_init = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_enc_cmp_data_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_enc_cmp_data_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_nvme_raw_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_nvme_raw_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_raw_data_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_raw_data_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0);
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__chk_xp10crc64_enc = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_match = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__data_vbits = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__cceip_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__cceip_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__cddip_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT____Vcellout__u_cr_prefix_attach_regfile__phd_mem_dout = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT____Vcellout__u_cr_prefix_attach_regfile__pfd_mem_dout = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_pfd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_phd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_pfd_req = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_phd_req = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_true = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_pfd_crc_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_phd_crc_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_prefix_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_prefix_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pac_pfd_check_valid_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pac_phd_check_valid_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pac_usr_ob_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_check_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_crc_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_dout_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_eot = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_check_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_crc_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_dout_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_eot = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_inwrk = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_phd_inwrk = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_usr_ob_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ob_wr = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT____Vcellout__prefix_attach_core__DOT__u_cr_prefix_attach_pac__pac_usr_ob_tlv);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT____Vcellout__prefix_attach_core__DOT__u_cr_prefix_attach_pti__pti_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__tlvp_ob_rd = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT____Vcellout__u_cr_fifo_wrap1__rdata);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__pt_ob_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_in = 0;
    VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_pfd_crc_wait = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_phd_crc_wait = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_num_d = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_valid_d = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_num_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_r_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_dunkw1_stall = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_phd_crc_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_pfd_crc_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_base = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_eod = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_eod = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_xp10_crc32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_xp10_crc32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_inc_phd_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_inc_pfd_addr_r = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_tlv_dunk_w0);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ibp_tlv_ftr_word_0);
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_dunk_w0_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_dunk_w1_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_tlv_tdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_pfd_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_insert_phd_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_pfd_tlv_tdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_ibp_tlv_word0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellinp__PFDMEM__wr_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PFDMEM__rd_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PFDMEM__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PFDMEM__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellinp__PHDMEM__wr_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PHDMEM__rd_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PHDMEM__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__PHDMEM__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellinp__u_cr_prefix_attach_regs__i_phdmem_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellinp__u_cr_prefix_attach_regs__i_pfdmem_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_regs_error_control = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_phdmem_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_phdmem_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_phdmem_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_pfdmem_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_pfdmem_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_pfdmem_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_cr_prefix_attach_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__add = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__din = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__we = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__r_rsp = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_ram__DOT__g__DOT__dat_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8192; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_ram__DOT__g__DOT__mem[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__add = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__din = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__we = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__r_rsp = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_ram__DOT__g__DOT__dat_r = 0;
    for (int __Vi0 = 0; __Vi0 < 4096; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_ram__DOT__g__DOT__mem[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cceip_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cceip_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_pfdmem_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_phdmem_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_error_control = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cddip_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cddip_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT___hufd_stat_events = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__bimc_ecc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__bimc_interrupt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__bimc_rst_n = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__htf_bl_im_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__htf_bl_im_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__lz_bytes_decomp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__lz_hb_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__lz_hb_head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__lz_hb_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__lz_local_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__sw_IGNORE_CRC_CONFIG = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__sw_LZ_BYPASS_CONFIG = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__sw_LZ_DECOMP_OLIMIT = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__sw_TLVP_ACTION_CFG0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__sw_TLVP_ACTION_CFG1 = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_ob_out_pre);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xpd_im_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xpd_im_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__r_uncor_ecc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__comp_rst__DOT____VdfgTmp_h99843f81__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master_odat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master_osync = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_monitor = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__i_bimc_parity_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__im_rdy_lz77d = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_cmd0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_cmd1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_dbgcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_monitor_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_parity_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_pollrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_rxcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__o_bimc_rxrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__locl_wr_data = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__im_din_lz77d);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__im_din_xpd);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im_pipe__ob_out);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_htf_bl_out_im_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_htf_bl_out_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_xpd_out_im_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_xpd_out_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_lz77d_out_im_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_cr_xp10_decomp_regs__i_lz77d_out_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_htf_bl_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_htf_bl_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_htf_bl_out_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_htf_bl_out_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_htf_bl_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_xpd_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_xpd_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_xpd_out_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_xpd_out_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_xpd_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_lz77d_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_lz77d_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_lz77d_out_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_lz77d_out_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_lz77d_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_cr_xp10_decomp_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_lz77d_im__wr_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im__im_available = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im__rd_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_lz77d_im__stat_code = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_xpd_im__wr_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_xpd_im__im_available = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_xpd_im__rd_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_xpd_im__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_xpd_im__stat_code = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellinp__u_htf_bl_im__wr_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_htf_bl_im__im_available = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_htf_bl_im__rd_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_htf_bl_im__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT____Vcellout__u_htf_bl_im__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im_pipe__DOT__state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im_pipe__DOT__state_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im_pipe__DOT__im_rdy_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt_uncorrectable_ecc_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt_correctable_ecc_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt_parity_errors_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__number_of_memories = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__r_bimc_eccpar_debug_write_notify_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_sent_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_cmd2_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__r_bimc_cmd2_write_notify_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_cmd2_sent_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_cmd2_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt = 0;
    VL_ZERO_RESET_W(72, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rdat);
    VL_ZERO_RESET_W(72, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dat);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_chk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_mem = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_resp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rx_chk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bm_resp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bm_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__new_frame = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__rstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__nxt_rstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__tstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__nxt_tstate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_global_config_bimc_mem_init_done_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__sync_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cmd_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__mem_wr_init_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__mem_wr_init_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__eccpar_debug_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cpu_transmit_ev = 0;
    VL_ZERO_RESET_W(73, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__reg_send);
    VL_ZERO_RESET_W(73, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__r_reg_send);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cputx_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cputx_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cputx_mem = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cputx_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__cputx_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__auto_poll_ecc_par_ev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__poll_ecc_par_timer = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__r32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__r32_mux_2_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f32_mux_2_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_decomp_dp_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_decomp_dp_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_lz77d_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_lz77d_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_lz77d_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_xpd_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_xpd_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_xpd_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_htf_bl_out_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_htf_bl_out_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_htf_bl_out_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_htf_bl_out_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_htf_bl_out_im_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_monitor_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_eccpar_debug = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_cmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_cmd1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_rxcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_rxrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_pollrsp2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_cr_xp10_decomp_regs__DOT__w_load_bimc_dbgcmd2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_din_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_din_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_din_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_available_pre = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__we = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__dout);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__hw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__credit_return = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__bank_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__overflow = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_vld_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__im_consumed_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT____Vcellout__u_nx_credit_manager__hw_status = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__ram_din);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__ram_bwe);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT____Vlvbound_he41d1f72__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT____Vlvbound_h4fe6ae12__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__wr_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__rd_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__wr_ptr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__rd_ptr_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__mem_nxt[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__mem_r[__Vi0]);
    }
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__dout_i);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__Vstatic__cur_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_nx_credit_manager__DOT__credit_issued_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_nx_credit_manager__DOT__used_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_nx_credit_manager__DOT__return_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_nx_credit_manager__DOT__credit_issued_v = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_ram__DOT__g__DOT__dat_r);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_ram__DOT__g__DOT__dout_i);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_ram__DOT__g__DOT__din_i);
    for (int __Vi0 = 0; __Vi0 < 512; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_ram__DOT__g__DOT__mem[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_din_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_din_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_din_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_available_pre = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__we = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__dout);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__hw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__credit_return = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__bank_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__overflow = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_vld_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__im_consumed_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT____Vcellout__u_nx_credit_manager__hw_status = 0;
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__ram_din);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__ram_bwe);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT____Vlvbound_hc55cd5e2__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT____Vlvbound_h4fe6ae12__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__wr_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__rd_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__wr_ptr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__rd_ptr_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__mem_nxt[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__mem_r[__Vi0]);
    }
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__dout_i);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__Vstatic__cur_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_nx_credit_manager__DOT__credit_issued_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_nx_credit_manager__DOT__used_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_nx_credit_manager__DOT__return_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_nx_credit_manager__DOT__credit_issued_v = 0;
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_ram__DOT__g__DOT__dat_r);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_ram__DOT__g__DOT__dout_i);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_ram__DOT__g__DOT__din_i);
    for (int __Vi0 = 0; __Vi0 < 512; ++__Vi0) {
        VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_ram__DOT__g__DOT__mem[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_din_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_din_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_din_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_available_pre = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__we = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__dout);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__hw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__credit_available = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__credit_return = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__bank_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__overflow = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_vld_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__im_consumed_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT____Vcellout__u_nx_credit_manager__hw_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__ram_din = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__ram_bwe = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT____Vlvbound_hd760f79f__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT____Vlvbound_h4fe6ae12__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__wr_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__rd_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__wr_ptr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__rd_ptr_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__mem_nxt[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__mem_r[__Vi0]);
    }
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__dout_i);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__hit_flag = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__Vstatic__cur_ptr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_nx_credit_manager__DOT__credit_issued_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_nx_credit_manager__DOT__used_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_nx_credit_manager__DOT__return_err_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_nx_credit_manager__DOT__credit_issued_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_ram__DOT__g__DOT__dat_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_ram__DOT__g__DOT__dout_i = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_ram__DOT__g__DOT__din_i = 0;
    for (int __Vi0 = 0; __Vi0 < 226; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_ram__DOT__g__DOT__mem[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_ram__DOT____Vlvbound_hac24fed3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__fe_lfa_ro_uncorrectable_ecc_error_a = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__fe_lfa_ro_uncorrectable_ecc_error_b = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__lz77_hb_ro_uncorrectable_ecc_error_a = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__lz77_hb_ro_uncorrectable_ecc_error_b = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__bhp_htf_hdr_dp_valid = 0;
    VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__bhp_htf_hdrinfo_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__bhp_htf_hdrinfo_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__bhp_mtf_hdr_valid = 0;
    VL_ZERO_RESET_W(107, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_be_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_be_dp_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_be_usr_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_be_usr_valid = 0;
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_htf_bl_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_htf_bl_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_dbg_data_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_dbg_data_valid = 0;
    VL_ZERO_RESET_W(67, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_prefix_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_prefix_hdr_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_prefix_hdr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fhp_lz_prefix_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__htf_bhp_hdr_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__htf_bhp_hdrinfo_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__htf_bhp_status_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__htf_bhp_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__htf_fhp_bl_ready = 0;
    VL_ZERO_RESET_W(99, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lfa_be_crc_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lfa_be_crc_valid = 0;
    VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lfa_sdd_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lfa_sdd_dp_valid = 0;
    VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz_be_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz_be_dp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz_fhp_pre_prefix_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf_bhp_hdr_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf_sdd_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__sdd_lfa_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__sdd_mtf_dp_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__zlib_raw_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__zlib_frm_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__zlib_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp9_raw_frm_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp9_frm_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp9_crc_err_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp9_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp10_raw_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp10_frm_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp10_frm_pfx_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp10_frm_pdh_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__xp10_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__phd_crc_err_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__pfx_crc_err_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__lfa_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__gzip_raw_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__gzip_frm_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__gzip_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__fhp_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__chu8k_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__chu8k_raw_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__chu4k_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellout__no_stub__DOT__fe__chu4k_raw_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT____Vcellinp__no_stub__DOT__u_xpd_split__ready_dst = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_lfa_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_lfa_htf_status_valid = 0;
    VL_ZERO_RESET_W(120, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_lfa_status_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_lfa_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_lfa_stbl_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_clear_sof_fifo = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_dp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_eof_bus = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_sof_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_lfa_sof_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_tlvp_pt_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_tlvp_usr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_bhp_align_bits = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_bhp_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_bhp_dp_valid = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_bhp_sof_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_bhp_sof_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_usr_tlv);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_fhp_tlvp_pt_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_fhp_tlvp_usr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__frmd_coding_location = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__usr_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r1_usr_eot = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__rqe_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__data_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_phd_word0_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__prefix_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__set_prefix_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__prefix_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__usr_prefix = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_usr_prefix = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_got_usr_prefix = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__frm_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__data_frm_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__first_beat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__prefix_crc_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__phd_crc_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_phd_crc_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_phd_crc_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__chk_pfx_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__chk_phd_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__set_pfx_crc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__set_phd_crc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_set_pfx_crc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_set_phd_crc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__phd_sof_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_phd_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__xp9_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__xp10_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__gzip_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__zlib_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__chu4k = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__chu8k = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__xp9_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pt_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_phd_tlv = 0;
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__int_htf_bl_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__int_htf_bl_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__wait_for_bl_ack = 0;
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_bl_fifo_rbus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__tlv_frame_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__tlv_eng_id = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__tlv_seq_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__rqe_sched_handle = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__frm_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__eot_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__last_of_command = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__sent_bl_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__phd_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__sent_phd_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__sent_pfx_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_pfx_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_phd_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__got_prefix = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__r_hdr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__func_map_tlv_to_lz__Vstatic__lz_out = 0;
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT____Vcellout__bl_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT____VdfgTmp_hfc0d2107__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT____VdfgTmp_hfc085838__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__deflate_data_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_deflate_data_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__deflate_hdr_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__chu_hdr_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__chu_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_start_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_sob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_frm_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_frm_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_frm_last = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__int_lfa_bhp_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_deflate_start_hdr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_hdr_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_stbl_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_data_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_align_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_hdr_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_hdr_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__words_to_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_hdr_head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_hdr_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_stbl_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_words_to_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_words_to_send = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__wait_for_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_hdr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__ack_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_isize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_isize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__f_isize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__next_isize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__n_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__f_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_data_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__l_cur_data_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_data_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_trace = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_hdr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_data_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rr_lfa_hdr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rr_lfa_data_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rr_lfa_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_data_rd_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cum_data_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__got_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_rd_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_rd_ack_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sent_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_crc_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_crc_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_stbl_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_stbl_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_stbl_head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_stbl_align = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_crc_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__first_crc_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_crc_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rr_lfa_crc_rd = 0;
    VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__err_sdd_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__tmp_sdd_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_data_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_hdr_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_stbl_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_frm_fmt = 0;
    VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_wbus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__wait_for_sdd_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_align_pre_eof = 0;
    VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_sdd_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__start_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_start_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_clear = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__tmp_sdd_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_rd_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__ack_64 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_hdr_align_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__deflate_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_deflate_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_deflate_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_deflate_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__prev_cur_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_ptr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_hdr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_stbl_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_eof_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__bhp_status_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__bhp_htf_status_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_sof_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_eof_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__raw_data_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_eof_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__last_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__one_after_last_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_hdr_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_stbl_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_data_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_eof_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_eof_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_data_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__f_cum_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__f_cum_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__s_cum_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__s_cum_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_sof_trace = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__start_hdr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_start_hdr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__start_stbl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_hdr_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_eof_hdr_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rr_eof_hdr_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__clear_sdd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_sdd_ack_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sent_eof_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_missing_eof_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__rewind_ack_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eob_word_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_sob_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_sob_eob_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_set_sob_eob_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp_hdr_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__bhp_dp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp9_first_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__first_word_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__err_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp_eof_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__new_stbl_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_rewind_ack_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__last_eof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_last_eof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__pre_last_eof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__last_hdr_eof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__first_sof_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp10_crc_mode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp10_runt_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__deflate_runt_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__blk_hdr_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__runt_blk_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__pre_runt_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__at_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp9_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__set_xp9_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp9_stbl_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__runt_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__a_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__stbl_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_error_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__in_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__abort_stbl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__abort_frm_early = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__late_eof_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sent_last_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__abort_premature_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__invalid_hdr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__invalid_stbl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_ack_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__dflate_late_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__large_data_frm = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_sdd_sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_lfa_sdd_sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__ack_addr_ok = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_eof_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__late_eof_numbits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__pre_eof_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_sdd_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__Vstatic__mod_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__Vstatic__mod_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__Vstatic__mod_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__Vstatic__n_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__Vstatic__int_data_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____Vcellinp__data_inst__align_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____Vcellinp__data_inst__align_wr = 0;
    VL_ZERO_RESET_W(80, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____Vcellout__sof_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____Vcellinp__eof_fifo__wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____Vcellout__eof_fifo__rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__new_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__tmp_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__mod_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__words_consumed = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__pad_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__Vstatic__new_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgExtracted_hd48169a3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgExtracted_hd19b84d7__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_hbd0a3260__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_h224b09c1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_hf74ce5a6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_h60b4df1d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_ha8578788__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_h749f9fd0__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_h078e3479__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT____VdfgTmp_h5644d939__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__tgl = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__prev_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__r_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__r_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__r_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__new_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__cur_bits_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__eof_bytes_valid = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT____Vcellout__if_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT____VdfgTmp_haef84294__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__hdr_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__tgl = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__prev_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__r_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__r_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__r_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__new_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__cur_bits_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__eof_bytes_valid = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT____Vcellinp__if_fifo__wdata);
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT____Vcellout__if_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT____VdfgExtracted_h8d0fbe65__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT____VdfgTmp_haef84294__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        VL_ZERO_RESET_W(80, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__wr_pg = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__rd_pg = 0;
    for (int __Vi0 = 0; __Vi0 < 1024; ++__Vi0) {
        VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__lfa_inst__DOT__g__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__lfa_inst__DOT__g__DOT__douta_r[__Vi0]);
    }
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__lfa_inst__DOT__g__DOT__douta_rr);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_sof_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_r_sof_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_lfa_bhp_dp_valid = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_lfa_bhp_dp_bus);
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_dp_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_sof_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_frm_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_blk_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_out_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_in_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_last_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__wsize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_part_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_part_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_res_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_bits_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_size = 0;
    VL_ZERO_RESET_W(84, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_hdrs);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_stbl_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_stbl_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__got_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__got_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_got_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_crc_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_crc_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_crc_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_frm_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_tmp_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_crc_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__chk_crc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_xp9_crc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_xp9_seq_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_seq_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__no_xp9_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__no_xp10_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_xp10_flg_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_xp10_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_deflate_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_hdrinfo_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__wait_for_htf_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdr_adj_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo_rd = 0;
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_htf_fifo_rbus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_dp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_data_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_data_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_align_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_data_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_deflate_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_deflate_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_hdr_info = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_deflate_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_blast = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_deflate_blast = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_bits_consumed = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_invalid_hdr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__chu_mode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_chu_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_phd_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp10_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp10_raw_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__gzip_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__gzip_raw_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__zlib_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__zlib_raw_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__chu4k_raw_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__chu8k_raw_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_crc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__part_osize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp10_hdr_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__send_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__cum_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__wait_for_htf_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hold_mtf_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_frm_out_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_frm_in_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_crc_option = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_frm_last_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__int_mtf_hdr_valid = 0;
    VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__int_mtf_hdr_bus);
    VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_int_mtf_hdr_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_bhp_lfa_htf_status_valid = 0;
    VL_ZERO_RESET_W(120, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_bhp_lfa_htf_status_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__rr_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_pfx_sz = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_tlv_frame_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_tlv_eng_id = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_tlv_seq_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_rqe_sched_handle = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__chu_sz = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__no_htf_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__int_bhp_htf_hdrinfo_valid = 0;
    VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__int_bhp_htf_hdrinfo_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_clear_hdrinfo = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_wsize_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_sof_blk = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_stbl_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_xp9_stbl_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__stbl_sz_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__set_mtf_hdr_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__valid_mtf_hdr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_mtf_res_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__updated_hdr_size = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__invalid_cum_sz = 0;
    VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT____Vcellout__bhp_fifo__rdata);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__mtf_exp[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__mtf_lsb[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__cum_numbits = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__tmp_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__new_tmp_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__Vstatic__is_mtf_last_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__Vstatic__lsb_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT____VdfgTmp_h445b6a52__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT____VdfgTmp_h8cc83258__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT____VdfgTmp_h7fbbaee1__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(70, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__bhp_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__htf_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__hdr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_deflate_data_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__nxt_data_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__nxt_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_bits_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__fcrc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__fextra = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__fname = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__fcomment = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__bits_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_xlen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_xlen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_fname = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_fname = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_fcomment = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_fcomment = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_crc_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_crc_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_blast_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_blast_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_blast = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_blast = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_dfmt1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_dfmt1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_dfmt2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_dfmt2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__all_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_dfmt1_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_dfmt2_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_dfmt2_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_dfmt1_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__id_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__cm_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__xlen_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__crc_wrd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_fcrc = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__tmp_dfmt_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__len_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_len_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__u_len_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__fcheck_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__cinfo_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_deflate_data_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__nxt_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__dict_prsnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__hdr_sof_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__any_errors = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_deflate_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__zlib_len_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__runt_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__r_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__got_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__nxt_data_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__max_sz_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__frm_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__t_r_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__inv_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__t_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__tt_r_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__term_i = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__fname_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__Vstatic__b_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__Vstatic__tmp_xlen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__Vstatic__f_xlen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__Vstatic__f_xlen_6 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__Vstatic__f_xlen_8 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__Vstatic__bits_consumed = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_blast_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_dfmt1_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_dfmt2_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_blast = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_dfmt1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__f_dfmt2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__Vstatic__b_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__Vstatic__bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__Vstatic__f_crc_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__Vstatic__b_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__b_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__new_b_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__f_len_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__f_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__f_len_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__r_hdr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__r1_hdr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__Vstatic__r2_hdr_data = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__hdrinfo_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT____Vcellinp__tlvp_dsm__tlv_parse_action = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__axi4s_slv_wen = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT____Vcellout__u_cr_fifo_wrap1__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__tlvp_pt_ib_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__tlvp_usr_ib_wen = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT____Vcellout__u_cr_tlvp_spl__tlvp_usr_ib_wdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT____Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT____Vcellout__u_cr_fifo_wrap1_pt__rdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT____Vcellout__u_cr_fifo_wrap1_usr_ib__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_bits_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_bits_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__predef_bl_req_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__predef_bl_rsp_bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_waddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_waddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_word_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_word_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_wbank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_wbank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_rbank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_rbank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_prefetch_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_prefetch_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_buf_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_buf_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__r_predef_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__c_predef_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__buf_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__bl_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__genblk1__DOT__web_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__genblk1__DOT__wa_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__genblk1__DOT__din_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__genblk1__DOT__bwe_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__din_i = 0;
    for (int __Vi0 = 0; __Vi0 < 130; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__mem[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__dout_r[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_buf__DOT____Vlvbound_h46f859b1__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT____Vlvbound_hd23a2db7__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__prefetch__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(84, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__r_shiftreg);
    VL_ZERO_RESET_W(84, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__c_shiftreg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__r_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__c_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__r_num_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__c_num_last = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__r_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_predef_buf__DOT__u_predef_unpacker__DOT__c_items_valid_till_last[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_fifo_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_fifo_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_fifo_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_unpacker_src_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_unpacker_dst_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_unpacker_dst_items_consume = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__r_got_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__c_got_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__r_got_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__c_got_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__r_hdr_bits_last_prev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__c_hdr_bits_last_prev = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__r_hdr_ro_uncorrectable_ecc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__r_hdr_data_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__c_hdr_data_stall_stb = 0;
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT____Vcellinp__u_hdr_fifo__wdata);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT____Vcellout__u_hdr_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__mem_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__mem_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT___mem_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT___mem_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk1__DOT__web_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk1__DOT__wa_r = 0;
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk1__DOT__din_r);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk1__DOT__bwe_r);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__din_i);
    for (int __Vi0 = 0; __Vi0 < 125; ++__Vi0) {
        VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr);
    VL_ZERO_RESET_W(65, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__ram__DOT____Vlvbound_hce1a20ab__0);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_ren_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_ren_dly = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_prefetch_wptr_dly[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_prefetch_wptr_dly[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_mem_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_mem_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_full = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__r_prefetch_data[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT__c_prefetch_data[__Vi0]);
    }
    VL_ZERO_RESET_W(66, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_fifo__DOT__fifo_ctrl__DOT____Vlvbound_h3339264c__0);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__r_shiftreg);
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__c_shiftreg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__r_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__c_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__r_num_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__c_num_last = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__r_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__u_hdr_unpacker__DOT__c_items_valid_till_last[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_start_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_start_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_total_bit_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_total_bit_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_sched_info = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_sched_info = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_blt_hist_complete_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_blt_hist_complete_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_small_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_small_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_small_blt_hist_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_small_blt_hist_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_small_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_small_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_hclen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_hclen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_hdist = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_hdist = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_hlit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_hlit = 0;
    VL_ZERO_RESET_W(114, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_codelen_data);
    VL_ZERO_RESET_W(114, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_codelen_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__reordered_codelen_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_total_bit_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_total_bit_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_status_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_status_bus = 0;
    VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_hdrinfo);
    VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_hdrinfo);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_prev_frame_phd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_prev_frame_phd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_prev_small_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_prev_small_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_bl_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_bl_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_bl_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_bl_index = 0;
    VL_ZERO_RESET_W(80, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_prev_bl);
    VL_ZERO_RESET_W(80, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_prev_bl);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_prev_non_zero_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_prev_non_zero_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_fifo_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_fifo_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_unpacker_src_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_unpacker_dst_items_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_unpacker_dst_items_consume = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__bl_unpacker_dst_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_section_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_section_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_section_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_section_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_section_end_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_section_end_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_sub_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_sub_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_repeat_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_repeat_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp9_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp9_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp9_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp9_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp9_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp9_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp9_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp9_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_xp10_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_xp10_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu4k_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu4k_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_simple_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_retro_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_predef_short_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_simple_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_retro_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_chu8k_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_chu8k_predef_long_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_deflate_dynamic_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_deflate_dynamic_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_deflate_fixed_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_deflate_fixed_blk_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__r_hdr_info_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__c_hdr_info_stall_stb = 0;
    VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vcellout__u_hdrinfo_reg_slice__payload_dst);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__status_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__im_valid = 0;
    VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__im_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__im_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_repeat_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_sub_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_bl_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_bl_index = 0;
    VL_ZERO_RESET_W(80, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_prev_bl);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_prev_non_zero_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_hdr_bits_consume = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_blt_hist_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_status_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_status_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_go = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vcellout__u_bl_fifo__rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_shrt_bl_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_long_bl_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_shrt_simple_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_shrt_simple_split_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_long_simple_bl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk5__DOT__v_long_simple_split_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk7__DOT__v_hdr_bits_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk3__DOT__unnamedblk7__DOT__v_hdr_bits_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__unnamedblk13__DOT__v_bl_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vlvbound_hef92d5d1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vlvbound_h5782f5d2__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vlvbound_h973489c9__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____Vlvbound_h6554ed6c__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____VdfgExtracted_h7e8c8f40__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____VdfgExtracted_h7e8c91e4__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____VdfgExtracted_h7e8ba8f3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____VdfgExtracted_h7e8bbbc9__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT____VdfgExtracted_h33f26013__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(75, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_hdrinfo_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_status_reg_slice__DOT__rev__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_status_reg_slice__DOT__rev__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_status_reg_slice__DOT__rev__DOT__r_payload = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_status_reg_slice__DOT__rev__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_im_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_total_length = 0;
    VL_ZERO_RESET_W(297, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_onehot_symbol);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_blt_hist_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_repeat_count = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_sub_state[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_status_valid = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__huff_status_bus[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra2_total_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra2_repeat_count = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra2_sub_state[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra2_status_valid = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra2_status_bus[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra3_total_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra3_repeat_count = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra3_sub_state[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra3_status_valid = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra3_status_bus[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra7_total_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra7_repeat_count = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra7_sub_state[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra7_status_valid = 0;
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__extra7_status_bus[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vconcswap_1_he7b8088a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vconcswap_1_h0eccd0c6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__unnamedblk1__DOT__unnamedblk3__DOT__unnamedblk4__DOT__v_inc_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__unnamedblk1__DOT__unnamedblk3__DOT__unnamedblk4__DOT__v_carryout = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_he365457e__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_he365457e__3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_hbb38e4c0__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_hbb38e4c0__3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__4 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__5 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__6 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT____Vlvbound_h51563254__9 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huff_length = 0;
    for (int __Vi0 = 0; __Vi0 < 33; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__unnamedblk2__DOT__v_bl_constants[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 33; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__unnamedblk2__DOT__v_repeat_constants[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 33; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__unnamedblk2__DOT__v_sub_state_constants[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h35df99b2__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h55759c98__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h6c96a96e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h17bdef37__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h4f7828af__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_ha81917b9__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_hc4d3878c__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT____Vlvbound_h080be1af__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__bct_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__bct_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__svt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__svt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__svt_last = 0;
    VL_ZERO_RESET_W(272, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__r_svt);
    VL_ZERO_RESET_W(272, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__c_svt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__r_small_tables_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__c_small_tables_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__r_small_tables_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__c_small_tables_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__r_deflate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__c_deflate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__small_smt_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT____Vlvbound_h34264dce__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__r_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__c_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__r_prev_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_bct_writer__DOT__c_prev_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__r_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__c_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__r_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__c_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__bct_preload_en = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__bct_inc_onehot[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT____Vcellinp__bct_inst__inc_onehot[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__unnamedblk1__DOT__unnamedblk4__DOT__v_slt_wen_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT____Vlvbound_h8da6c55a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__bct_inst__DOT__r_array = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__bct_inst__DOT__c_array = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_svt_writer__DOT__bct_inst__DOT____Vlvbound_ha09e6dcb__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_histogram__DOT__inc_onehot[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_histogram__DOT____Vlvbound_hdb417716__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_histogram__DOT__u_histogram_array__DOT__r_array = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_histogram__DOT__u_histogram_array__DOT__c_array = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__r_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__c_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__r_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__c_blt_depth = 0;
    VL_ZERO_RESET_W(136, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__r_blt);
    VL_ZERO_RESET_W(136, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT__c_blt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_blt__DOT____Vlvbound_hd49e2827__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__r_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__c_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__r_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__c_blt_depth = 0;
    VL_ZERO_RESET_W(272, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__r_blt);
    VL_ZERO_RESET_W(272, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT__c_blt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_small_tables__DOT__u_smt__DOT____Vlvbound_hb033af64__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT____Vlvbound_h8e61eadf__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(320, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__r_shiftreg);
    VL_ZERO_RESET_W(320, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__c_shiftreg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__r_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__c_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__r_num_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__c_num_last = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__r_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_bl_unpacker__DOT__c_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_histogram__DOT__inc_onehot[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_histogram__DOT____Vlvbound_h4c55b371__0 = 0;
    VL_ZERO_RESET_W(270, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_histogram__DOT__u_histogram_array__DOT__r_array);
    VL_ZERO_RESET_W(270, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_histogram__DOT__u_histogram_array__DOT__c_array);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__r_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__c_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__r_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__c_blt_depth = 0;
    VL_ZERO_RESET_W(2880, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__r_blt);
    VL_ZERO_RESET_W(2880, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT__c_blt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_blt__DOT____Vlvbound_he61c1b69__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_sched_info = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_sched_info = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_complete_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_complete_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_bct_sat_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_bct_sat_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_bct_sat_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_bct_sat_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_bct_sat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_bct_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_bct_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_sat_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_sat_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ss_slt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ss_slt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ss_slt_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ss_slt_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ss_slt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ss_slt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ll_slt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ll_slt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ll_slt_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ll_slt_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_htf_sdd_ll_slt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_htf_sdd_ll_slt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__hist_complete = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__ss_pointer_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__ll_pointer_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__ss_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__ll_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__hist_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_blt_hist_busy = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_blt_hist_busy = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_blt_hist_complete_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_ss_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_ss_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_ll_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_ll_slt_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_at_least_1_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_at_least_1_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_ss_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_ss_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__r_ll_blt_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__c_ll_blt_depth = 0;
    VL_ZERO_RESET_W(1240, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__ll_blt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT____Vlvbound_hfcabd122__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__r_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__c_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__r_prev_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__c_prev_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__r_prev_sat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__bct_sat_writer__DOT__c_prev_sat = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__r_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__c_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__r_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__c_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__pointer_preload_en = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__pointer_inc_onehot[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT____Vcellinp__pointer_inst__inc_onehot[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__unnamedblk1__DOT__unnamedblk4__DOT__v_slt_wen_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT____Vlvbound_h430cf720__0 = 0;
    VL_ZERO_RESET_W(270, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__pointer_inst__DOT__r_array);
    VL_ZERO_RESET_W(270, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__pointer_inst__DOT__c_array);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ss_slt_writer__DOT__pointer_inst__DOT____Vlvbound_h55a64d73__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__r_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__c_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__r_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__c_blt_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__r_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__c_blt_count = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__pointer_preload_en = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__pointer_inc_onehot[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT____Vcellinp__pointer_inst__inc_onehot[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__unnamedblk1__DOT__unnamedblk4__DOT__v_slt_wen_mask = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT____Vlvbound_h430cf720__0 = 0;
    VL_ZERO_RESET_W(216, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__pointer_inst__DOT__r_array);
    VL_ZERO_RESET_W(216, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__pointer_inst__DOT__c_array);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__u_ll_slt_writer__DOT__pointer_inst__DOT____Vlvbound_h6ee0a27e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__decoder_eob_credit_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__decoder_sob_credit_used = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__lanes_wf_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__ss_ld_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__ss_sp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__wf_lanes_valid = 0;
    VL_ZERO_RESET_W(1404, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ss_bct);
    VL_ZERO_RESET_W(1404, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ss_bct);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ss_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ss_bct_valid = 0;
    VL_ZERO_RESET_W(540, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ss_sat);
    VL_ZERO_RESET_W(540, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ss_sat);
    VL_ZERO_RESET_W(11520, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ss_slt);
    VL_ZERO_RESET_W(11520, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ss_slt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ss_used_symbols = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ss_used_symbols = 0;
    VL_ZERO_RESET_W(1404, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ll_bct);
    VL_ZERO_RESET_W(1404, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ll_bct);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ll_bct_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ll_bct_valid = 0;
    VL_ZERO_RESET_W(432, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ll_sat);
    VL_ZERO_RESET_W(432, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ll_sat);
    VL_ZERO_RESET_W(3968, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ll_slt);
    VL_ZERO_RESET_W(3968, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ll_slt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_ll_used_symbols = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_ll_used_symbols = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_block_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_block_fmt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_block_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_block_min_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_block_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_block_min_mtf_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_block_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_block_error = 0;
    VL_ZERO_RESET_W(1404, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__bct_mask);
    VL_ZERO_RESET_W(540, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__ss_sat_mask);
    VL_ZERO_RESET_W(432, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__ll_sat_mask);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_first_ss_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_first_ss_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_first_ll_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_first_ll_bct = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_first_ss_slt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_first_ss_slt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__r_first_ll_slt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__c_first_ll_slt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vcellout__u_sched_info_fifo__rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h78178c98__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h4bed052e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h34942b51__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h88ea4348__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h810bef58__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h4dd76874__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h598f07a1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h00cbb320__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h35027c98__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h0a7f4d77__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h35027c98__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_hda31c06b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT____Vlvbound_h9dd1dc4b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_decoder_credit__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_htf_credit__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_dec_tables__DOT__u_sched_info_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__dp_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__unpacker_src_items_valid = 0;
    VL_ZERO_RESET_W(224, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__unpacker_src_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__unpacker_dst_items_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__unpacker_dst_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__r_input_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT____VdfgExtracted_h9549da8d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(112, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    VL_ZERO_RESET_W(784, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__r_shiftreg);
    VL_ZERO_RESET_W(784, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__c_shiftreg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__r_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__c_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__r_num_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__c_num_last = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__r_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_wf__DOT__u_bff__DOT__c_items_valid_till_last[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length_PRICOD__Vstatic__cod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__Vstatic__reversed_bit_stream = 0;
    for (int __Vi0 = 0; __Vi0 < 27; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__Vstatic__candidate_minus_base[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__Vstatic__candidate_gte_base = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__Vstatic__length_pricod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length_PRICOD__Vstatic__cod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__Vstatic__reversed_bit_stream = 0;
    for (int __Vi0 = 0; __Vi0 < 27; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__Vstatic__candidate_minus_base[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__Vstatic__candidate_gte_base = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__Vstatic__length_pricod = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_s0_table_bank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_s0_table_bank = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_s1_buf_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_s1_buf_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_s2_word_mod_8 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_s2_word_mod_8 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_s2_credited_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_s2_credited_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_buf_full_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_buf_full_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_ready = 0;
    for (int __Vi0 = 0; __Vi0 < 7; ++__Vi0) {
        VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_src_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_ready = 0;
    for (int __Vi0 = 0; __Vi0 < 7; ++__Vi0) {
        VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_data[__Vi0]);
    }
    VL_ZERO_RESET_W(960, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__r_bit_buf);
    VL_ZERO_RESET_W(960, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__c_bit_buf);
    VL_ZERO_RESET_W(5760, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__v_ss_slt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__v_ss_used_symbols = 0;
    VL_ZERO_RESET_W(1984, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__v_ll_slt);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__v_ll_used_symbols = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__N = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__v_ll_bct_valid_is_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__v_fmt_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__v_fmt_deflate = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__v_fmt_xp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk12__DOT__v_bias = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk13__DOT__v_s1_buf_idx = 0;
    VL_ZERO_RESET_W(1920, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk13__DOT__v_bit_buf);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk21__DOT__unnamedblk22__DOT__v_extra_add_term_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk21__DOT__unnamedblk22__DOT__v_extra_sub_term_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk21__DOT__unnamedblk22__DOT__v_extra_add_term_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk21__DOT__unnamedblk22__DOT__v_extra_sub_term_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk21__DOT__unnamedblk22__DOT__v_ll_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__unnamedblk9__DOT__unnamedblk23__DOT__unnamedblk24__DOT__v_eob_sym = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vcellout__reg_slices__BRA__1__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vcellout__reg_slices__BRA__2__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vcellout__reg_slices__BRA__4__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vcellout__reg_slices__BRA__5__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vcellout__reg_slices__BRA__6__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_ha1411d30__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hf9352cd6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h4274e171__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_he2df5d77__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h308fe530__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h422d001b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h5395ce4a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h42a7aee3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h42a783d4__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h308fe530__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h840e2855__0 = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h0ad3e382__0);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h94d8532b__0);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h77d4e462__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h77258406__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h76580c76__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6605d6d6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6ee5518b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h660d0239__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hc08a2163__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h538a8118__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h52e39c5b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h52ae2a54__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h5278a460__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hf36c500f__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_he608471b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h5e055bb6__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h52ae2a54__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h1c10810f__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hf3c8219a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h1c10810f__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h308fe530__3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h840e2855__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h7fd193e7__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h83c5c510__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h7183d3d5__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6de05efd__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hd1bab71f__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h7183d3d5__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6de05efd__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6b5f5e8c__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6b5f5e8c__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hd1bab71f__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6b5f5e8c__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_hd1bab71f__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6b24648d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT____Vlvbound_h6b24648d__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__4__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__4__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__4__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__4__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__5__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__5__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__5__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__5__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__6__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__6__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__6__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__reg_slices__BRA__6__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_selector_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_selector_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_selector_idx_mod_256 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_selector_idx_mod_256 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_selector_errcode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_selector_errcode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sent_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sent_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sent_sob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sent_sob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_selector_reg_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_selector_reg_valid = 0;
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_selector_reg);
    VL_ZERO_RESET_W(3872, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_selector_reg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__pipe_src_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__pipe_src_ready = 0;
    VL_ZERO_RESET_W(251, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__pipe_src_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sdd_lfa_ack_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sdd_lfa_ack_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sdd_lfa_ack_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sdd_lfa_ack_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__r_sent_eob_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__c_sent_eob_ack = 0;
    VL_ZERO_RESET_W(3680, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_lanes);
    VL_ZERO_RESET_W(352, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_start_idxs);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_total_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_window = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_idx_mod_32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__v_terminate = 0;
    VL_ZERO_RESET_W(115, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__unnamedblk2__DOT__unnamedblk4__DOT__unnamedblk5__DOT__v_lane);
    VL_ZERO_RESET_W(115, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h74543e6e__0);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h7b54864b__0 = 0;
    VL_ZERO_RESET_W(115, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h74543e6e__1);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h7b54864b__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h98efa05b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h7e44a956__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h7bfaee7b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h86bb6ec0__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h742acd8a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h98c9d0c3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h80663e2a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT____Vlvbound_h1d6af844__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(251, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ss__DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_mtf_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_mtf_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_sp_buf_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_sp_buf_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__packer_src_items_valid = 0;
    VL_ZERO_RESET_W(306, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__packer_src_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__packer_dst_items_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_frame_bits_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_frame_bits_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_bits_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_bits_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_bytes_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_bytes_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_at_least_1_byte_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_at_least_1_byte_out = 0;
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_xp10_decomp_sch_update);
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_xp10_decomp_sch_update);
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_last_sch_update);
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__c_last_sch_update);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__r_su_afull_n = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__eof_fifo_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__eof_fifo_wdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_src_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_src_ready = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        VL_ZERO_RESET_W(244, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_src_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_dst_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_dst_ready = 0;
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        VL_ZERO_RESET_W(244, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_dst_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__outreg_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__outreg_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__outreg_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vcellout__u_eof_fifo__rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__v_found_backref = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__v_stop = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__v_consume_symbol = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__v_consume_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk7__DOT__v_group = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk7__DOT__unnamedblk8__DOT__i = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk7__DOT__unnamedblk9__DOT__i = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk10__DOT__v_distance = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk11__DOT__v_extra_byte = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk11__DOT__unnamedblk12__DOT__v_data_bits = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__unnamedblk3__DOT__unnamedblk11__DOT__unnamedblk13__DOT__v_tmp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vcellout__reg_slices__BRA__1__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vcellout__reg_slices__BRA__2__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vcellout__reg_slices__BRA__3__KET____DOT__u_reg_slice__ready_src = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h151658f8__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h17ad59e3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h151658f8__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h17ad59e3__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h101094ab__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec003ee__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec6d295__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec1bcae__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha592dc42__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h5cec43dd__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbbfb570b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9f916209__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha2e606a3__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec2ee6e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha5bab6fa__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9f916209__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha4c7ca6e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ee557cd__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha7da547d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbe1facb0__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec003ee__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbe1facb0__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h56141ca7__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h5602618c__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h56141ca7__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h9ec1ac7d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h5602618c__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h101094ab__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h57074f8b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h380daef6__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h8433112d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hdf3ca886__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h90eb0403__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h4f621893__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h568587e8__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbdbf6e36__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbdbf6e36__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbdbf6e36__2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_hbdbf6e36__3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h3c4b1511__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_ha5f07365__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT____Vlvbound_h48262a05__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_eof_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(816, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__r_shiftreg);
    VL_ZERO_RESET_W(816, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__c_shiftreg);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__r_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__c_total_items_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__r_num_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__c_num_last = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__r_items_valid_till_last[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_packer__DOT__c_items_valid_till_last[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__u_outreg__DOT__full__DOT__r_payload[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(244, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__1__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(244, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__2__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__DOT__fwd__DOT__c_full = 0;
    VL_ZERO_RESET_W(244, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_payload);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__reg_slices__BRA__3__KET____DOT__u_reg_slice__DOT__fwd__DOT__r_enable = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_mtf_cache_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_mtf_cache_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_mtf_cache_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_mtf_cache_present = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_mtf_cache_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_mtf_cache_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_mtf_cache_format = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_mtf_cache_format = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_mtf_cache_ptr_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_mtf_cache_ptr_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__r_errcode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__c_errcode = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__pipe_src_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__pipe_src_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__pipe_src_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__unnamedblk1__DOT__unnamedblk2__DOT__v_mtf = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__unnamedblk1__DOT__unnamedblk2__DOT__v_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__unnamedblk1__DOT__unnamedblk2__DOT__v_shift_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__unnamedblk1__DOT__unnamedblk4__DOT__v_exp = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__unnamedblk1__DOT__unnamedblk4__DOT__v_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(88, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_head_info_reg__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__mtf__DOT__u_out_reg__DOT__full__DOT__r_payload[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__u_xpd_split__DOT__ready_int = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__u_xpd_split__DOT__u_hndshk_split__DOT__r_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__u_xpd_split__DOT__u_hndshk_split__DOT__c_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_ep_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_ep_head_moved = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_even_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_even_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_even_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_even_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_odd_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_odd_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_odd_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_do_odd_word = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_ag_hb_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_bm_pause = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_ag_hb_1st_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_ag_hb_num_words = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_ag_hb_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_copy_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_copy_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_copy_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_from_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_lit_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_lit_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_lwl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_lwrd_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_num_lit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_ptr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_bm_to_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_if_entry_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_if_load_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_if_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_ep_sym = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_ep_sym_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_ag_load_tail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_ag_tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_ep_prefix_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_ep_prefix_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_ep_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx0_pld_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx0_pld_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx0_pld_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx1_pld_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx1_pld_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx1_pld_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx2_pld_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx2_pld_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_pfx2_pld_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_usr_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_usr_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_hb_usr_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__lz_dp_bus = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__lz_dp_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__if_inst__lz77_stall_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__if_inst__frm_out_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__if_inst__frm_in_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__if_inst__ptr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__if_inst__lane_lit_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_3_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_4_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_5_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_6_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_7_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_8_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_9_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_10_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_11_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_32_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_64_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_128_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT____Vcellout__ep_inst__ptr_256_stb = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__sym[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__sym_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_rd = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__lane_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__sym_val = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__rd_data[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__wr_data[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__wr[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__rd[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_i = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__cur_index = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__rd_idx = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__read_fifo = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__frame_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__r_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__saved_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__int_ptr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__int_frm_in_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__int_frame_ready = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__int_frm_out_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__int_lane_lit_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT____Vcellout__if_fifo__rdata = 0;
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__if_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__0__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__1__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__2__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__if_inst__DOT__fifo_loop__BRA__3__KET____DOT__fifo_inst__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__lit_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__lit_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__lit_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ptr_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ptr_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__cur_pos = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__cur_pos_cur = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__his_buf_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__his_buf_cur = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__curr_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__tmp_curr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_bytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_hb_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_last_word_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__copy_in_pgrss = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_in_pgrss = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__copy_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__copy_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__copy_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_copy_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_copy_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_copy_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__words_to_read_from_hb = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__updated_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__cur_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__hb_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__last_word_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__from_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__to_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__copy_after_hb_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_copy_after_hb_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_from_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_to_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__curr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_pause = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ep_pause = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ready_to_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_lit_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_lit_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_lit_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__eof_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__eob_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__saved_sym = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__nxt_sym = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__saved_sym_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__nxt_sym_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__cp_after_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__cp_after_ptr_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__curr_cp_len = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__set_offset_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_offset_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_256 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_128 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_64 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_11 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_10 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_9 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_8 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_7 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_6 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_5 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_4 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__ptr_3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_trace_bit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__last_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word0_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word1_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__saved_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__next_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__next_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__vld_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__saved_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__new_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word_to_send);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__word_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__prev_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__part_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__part_word_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__saved_part_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__saved_part_word_valid = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__hb_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__hb_word_valid = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cur_hb_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cur_hb_word_valid = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__prev_hb_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__prev_hb_word_valid = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__p_prev_hb_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__in_copy = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__new_cp_cnt = 0;
    VL_ZERO_RESET_W(192, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__new_cp_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_bytes);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_c_bytes);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_bytes_offset = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__cp_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__to_copy = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__local_saved_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__r_bm_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__r_bm_eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__r_bm_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_lit_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_num_lit = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_lit_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_ptr_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_from_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_to_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_lwrd_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_lwl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_copy_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_copy_offset = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_copy_length = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__bm_eof_err_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__r_do_bm_pause = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__f_to_copy = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__tmp_wd);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__mask_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__tmp_copy_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__m_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__Vstatic__tmp_copy_bytes);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__Vstatic__local_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__Vstatic__mask_word);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__Vstatic__tmp_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__Vstatic__m_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__prev_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__prev_offset = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__new_prev_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__f_cp_byte_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__f_cp_bytes);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__new_cur_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__cur_offset = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__tmp_wd);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__o_i = 0;
    VL_ZERO_RESET_W(192, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__Vstatic__tmp_cur_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__Vstatic__o_i = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__Vstatic__tmp_wd);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__f_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__s_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__n_word = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__n_word_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__s_word_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__Vstatic__f_word_valid = 0;
    VL_ZERO_RESET_W(192, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__Vstatic__lword);
    VL_ZERO_RESET_W(192, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__Vstatic__lword1);
    VL_ZERO_RESET_W(192, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__Vstatic__local_word);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__Vstatic__l_cur_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__odd_sent = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__odd_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__even_rd = 0;
    VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__out_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_wr = 0;
    VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__odd_rdata);
    VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__even_rdata);
    VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__rd_data);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__odd_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__ag_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__sent_eob = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__bytes_shift = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_odd__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(69, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__outfifo_even__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__tail_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__head_ptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__base_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__curr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__prev_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__window_sz = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__r_ep_ag_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ag_inst__DOT__rr_ep_ag_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx0_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx0_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx1_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx1_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx2_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx2_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_waddr = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx0_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx0_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx1_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx1_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx2_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx2_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx0_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx1_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx2_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__rr_pfx0_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__rr_pfx1_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__rr_pfx2_rd = 0;
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx0_rdata);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx1_rdata);
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__r_pfx2_rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx0_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx1_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__pfx2_hb_wr = 0;
    for (int __Vi0 = 0; __Vi0 < 4096; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_inst__DOT__g__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_inst__DOT__g__DOT__douta_r[__Vi0]);
    }
    VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__hb_inst__DOT__g__DOT__douta_rr);
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix0__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix0__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix1__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix1__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix2__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(128, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix2__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx0_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx1_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx2_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx0_in_use = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx1_in_use = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx2_in_use = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_rd_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx0_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx1_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx2_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__space_avail = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__r_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__tgl = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__blk_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__l_cnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__rr_last = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__r_data_3 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__r_data_2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__r_data_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__r_data_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__rr_data_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx0_avail_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx1_avail_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx2_avail_load = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx0_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx1_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__pfx2_done = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_fifo__DOT__depth_1__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_fifo__DOT__depth_1__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_fifo__DOT__depth_1__DOT__c_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hdr_fifo__DOT__depth_1__DOT__r_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hold_fifo__DOT__depth_1__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hold_fifo__DOT__depth_1__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hold_fifo__DOT__depth_1__DOT__c_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__hold_fifo__DOT__depth_1__DOT__r_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_fifo__DOT__depth_1__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_fifo__DOT__depth_1__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_fifo__DOT__depth_1__DOT__c_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__data_fifo__DOT__depth_1__DOT__r_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__lz_data_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__pt_ob_rd = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__pt_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__r_lz_fifo_rd = 0;
    VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__r_lz_fifo_rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__cur_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__r_cur_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_cur_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__sent_eof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_hdr_rd = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__r_usr_hdr_rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_ob_wr = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_ob_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftr_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftr_rd = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftr_wdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__ok_to_read = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_err_code = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftr_mod_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__ftr_cnt = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_data_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_frm_bcnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_size_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_crc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__size_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__olimit_size_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_word_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__r_usr_word_0 = 0;
    VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_crc_rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__get_bip2__Vstatic__evn = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__get_bip2__Vstatic__odd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__get_bip2__Vstatic__par = 0;
    VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT____Vcellout__lz_fifo__rdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT____Vcellout__usr_fifo__rdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT____Vcellout__usr_ftf__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT____VdfgTmp_h65db311b__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__lz_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 128; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__pt_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__usr_ftf__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(74, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_lz_bus);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_lz_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_fifo_rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__frm_bcnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__bcnt = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_isize = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_c4k = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_c8k = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_crc_32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_crc_64 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_adler = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__check_gzip_crc_32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_32_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_64_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__gzip_crc_32_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_32_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__gzip_crc_32_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_64_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_crc_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__tmp_32_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__gzip_tmp_32_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__tmp_64_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_sof = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__got_eop = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__nxt_adler32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_nxt_adler32 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__r_check_adler = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__crc_crc_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_crc_error = 0;
    VL_ZERO_RESET_W(99, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT____Vcellout__usr_fifo__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__reverse_bytes__Vstatic__out_crc = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__data_arr[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__b_out = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__a_int_out[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__r_a_out = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__r_a_int_out[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__r_b_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__r_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__adler_inst__DOT__val_sof = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(99, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__usr_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi4s_mstr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_bip2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_sel = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_ob_datain);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_selector = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__current_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__next_state = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__axi4s_ib_out);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__axi4s_ib_in = 0;
    VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst);
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__axi_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(96, vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc64e_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc64e_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc_init = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_enc_cmp_data_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_enc_cmp_data_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_nvme_raw_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_nvme_raw_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_raw_data_cksum_err_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_raw_data_cksum_good_se = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_md_type = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0);
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_xp10crc64_raw = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc16t = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc64e = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__data_vbits = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__data_vbits = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__cg_ib_in_mod);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT____Vcellout__u_cr_cg_tlv_mods__cg_stat_events = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_term_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ix_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ix = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__out_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__out_sel_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_err_save = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_fin = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_tlv_w0_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_tlv_w1_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_tlv);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_tlv_w0_reg);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__stats_tlv_tdata_w0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_pulse = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd_pulse = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w0_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__guid_bip2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd_clr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short = 0;
    for (int __Vi0 = 0; __Vi0 < 20; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd_hld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_hld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w0_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__stats_val_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__stats_val_clr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_val_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_idle = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_idle_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_cmd_end = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_cmd_end_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_agg_bytes_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_agg_bytes_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_err_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_err_clr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid_ld = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__stats_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____Vlvbound_h647a6d3a__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____Vlvbound_h9cafe8f1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____Vlvbound_hcf917fa1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____VdfgExtracted_h5b9f775b__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____VdfgTmp_h593ef814__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__tlvp_ob_rd = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_bip2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT____Vcellout__u_cr_fifo_wrap1_uobf__rdata);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_out);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_in = 0;
    VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT____Vcellout__u_axi_channel_reg_slice__payload_dst);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_wr_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_cg_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_cr_cg_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_cg_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_cg_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__data_fifo_single_step_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__ob_bytes_cnt_amt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__ob_bytes_cnt_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__ob_frame_cnt_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__osf_data_fifo_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__osf_pdt_fifo_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__pdt_fifo_single_step_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_empty_mod = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_data_fifo_sw_wdata);
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_data_fifo_sw_rdata);
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_pdt_fifo_sw_wdata);
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_pdt_fifo_sw_rdata);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_ib_in_mod);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_data_fifo__fifo_status_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_data_fifo__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_data_fifo__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_pdt_fifo__fifo_status_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_pdt_fifo__stat_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_osf_dbg_pdt_fifo__stat_code = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_ctl__ob_fifo_wdata);
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_latency__axi4s_out);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellout__u_cr_osf_support__osf_stat_events = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT____Vcellout__u_cr_fifo_wrap1__rdata);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__tlvp_pt_ib_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__tlvp_usr_ib_wen = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT____Vcellout__u_cr_tlvp_spl__tlvp_pt_ib_wdata);
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT____Vcellout__u_cr_fifo_wrap1_pt__rdata);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__afull_r = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__afull_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel = 0;
    VL_ZERO_RESET_W(106, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_nxt_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__wr_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__und_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__ovr_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__fifo_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__fifo_wr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__fifo_depth_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__fifo_rd_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__fifo_wr_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__hw_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__hw_re = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__wadd = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__din);
    for (int __Vi0 = 0; __Vi0 < 512; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_datain);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__wr_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__und_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__ovr_flow = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__fifo_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__fifo_wr_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__fifo_depth_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__fifo_rd_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__fifo_wr_addr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__hw_raddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__hw_cs = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__hw_re = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__wadd = 0;
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__din);
    for (int __Vi0 = 0; __Vi0 < 256; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    VL_ZERO_RESET_W(92, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timeout = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__igrant = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__df_dbg_empty_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__pf_dbg_empty_mod = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__df_debug_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__pf_debug_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_df_rd_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_dmux_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__df_dat_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__df_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__pf_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__simp_cmd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__cmp_cmd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__pf_cqe_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__cmp_cmd_last_frame = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__pre_frmd_val = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__osf_lat_ctl_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__osf_lat_ctl_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_out_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_cnt_en = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_cnt_clr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__latency_cnt = 0;
    VL_ZERO_RESET_W(83, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__axi4s_ib_out);
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__axi4s_ib_in = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_enable = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        VL_ZERO_RESET_W(82, vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_payload[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__ob_cmd_active = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__ob_out_bytes_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__cqe_par_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__osf_ob_par_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__osf_ob_data_par_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_data_bytes_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_frame_global_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_wr_data = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_bytes_cnt[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_frame_cnt[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_ob_agg_data_bytes_cntr__counter_a[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_ob_agg_frame_cntr__counter_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_pdt_fifo_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_data_fifo_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__counter_a_int[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_stb_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_by_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_id_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_stb_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_stb_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_stb_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_by_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_by_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__count_by_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__selected = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__rd_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__wr_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__counter_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__stb_tog_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_h8d97bf4d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_hf1cf4032__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_hbfef4226__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_hbfef4226__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_hd7f3132e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_he9aadcae__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT____Vlvbound_he01da8e1__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__counter_a_int[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_stb_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_by_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_id_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_stb_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_stb_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__count_stb_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__selected = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__rd_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__wr_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__counter_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__stb_tog_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_h8d97bf4d__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_hf1cf4032__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_hbfef4226__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_hbfef4226__1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_hd7f3132e__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_he9aadcae__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT____Vlvbound_he01da8e1__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__o_data_fifo_debug_ss_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__o_pdt_fifo_debug_ss_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__r32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_tlv_parse_action_0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_tlv_parse_action_1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_data_fifo_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_pdt_fifo_debug_ctl_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_data_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_data_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_pdt_fifo_ia_wdata_part2 = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_pdt_fifo_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT____Vcellout__u_cr_cddip_sa_regfile__regs_sa_ctrl[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__big_stat_events_vec[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_events[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_snap_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_clear_live_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_wr_data = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__sa_ctrl_rst_dat[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_CTRL__rst_dat[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_CTRL__mem_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_CTRL__rd_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_CTRL__stat_code = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_SNAPSHOTR__rd_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_SNAPSHOTR__stat_code = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_COUNTR__rd_dat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_COUNTR__stat_code = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_count_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_snapshot_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_ctrl_ia_status = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_wdata_part1 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_add = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_wdat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_rdat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__yield = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__sw_we_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__sw_rdat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__yield = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__sw_rdat = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__yield = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__init_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__init_inc_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__sw_cs_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__rst_or_ini_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_issued = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__ack_error = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__unsupported_op = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__state_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__sim_tmo_r = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__badaddr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cntrlr__DOT__state_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_global_ctrl = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_ctrl_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_ctrl_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_snapshot_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_snapshot_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_count_ia_wdata_part0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_count_ia_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_agg_cnt_stb = 0;
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_ZERO_RESET_W(108, vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__axi_mstr_rd = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__in_fifo_rd = 0;
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__in_fifo_rdata);
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__in_fifo_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__out_fifo_wr = 0;
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT____Vcellout__u_cr_su_ctl__in_fifo_wdata);
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT____Vcellout__u_su_out_fifo__rdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT____Vcellout__u_su_axi4s_mstr__axi4s_ob_out = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__mem_wen = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__mem_ren = 0;
    for (int __Vi0 = 0; __Vi0 < 64; ++__Vi0) {
        VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__mem[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__dout_r[__Vi0]);
    }
    VL_ZERO_RESET_W(113, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__ram__DOT___1r1wramDxWb__DOT__genblk3__DOT__dout_rr);
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_ren_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_ren_dly = 0;
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_prefetch_wptr_dly[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_prefetch_wptr_dly[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_mem_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_mem_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_depth = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_full = 0;
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        VL_ZERO_RESET_W(114, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__r_prefetch_data[__Vi0]);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        VL_ZERO_RESET_W(114, vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_in_fifo__DOT__fifo_ctrl__DOT__c_prefetch_data[__Vi0]);
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_ctl_st = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_ctl_st_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__in_fifo_rd_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__out_fifo_wr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__serial_mux_sel_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_axi_tlast_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_axi_tuser_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_hb_wr_nxt = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__serial_mux_sel = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_axi_tlast = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_axi_tuser = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_hb_wr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_axi_tdata = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_hb_add = 0;
    for (int __Vi0 = 0; __Vi0 < 24; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT____Vlvbound_hcafaed44__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_su_out_fifo__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_rd_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_wr_strb = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__rd_stb = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_wr_data = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__agg_su_count[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_cr_su_regs__o_dbg_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_cr_su_regs__o_spare_config = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_err_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_ack_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_rd_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_data_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_wr_strb_o = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_nx_rbus_ring__rbus_addr_o = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_agg_su_cntr__counter_a[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__w_valid_rd_addr = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__n_write = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__w_next_state = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_err_ack = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f32_mux_0_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f32_mux_1_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f32_data = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_addr_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_wr_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_strb_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_rd_data_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_ack_i_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_nx_rbus_ring__DOT__sync_io__DOT__rbus_err_ack_i_reg = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__counter_a_int[__Vi0] = 0;
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_stb_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_by_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_id_reg = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_stb_int__VforceRd = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_stb_int__VforceEn = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__count_stb_int__VforceVal = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__rd_stb_valid = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__counter_v = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__stb_tog_dly = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT____Vlvbound_h9f1ee0cb__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT____Vlvbound_h9ccb65f4__0 = 0;
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT____Vlvbound_h0dba765c__0 = 0;
    VL_ZERO_RESET_W(82, vlSelf->__VdfgTmp_h95cca642__0);
    VL_ZERO_RESET_W(83, vlSelf->__VdfgTmp_hd3473a04__0);
    vlSelf->__VdfgTmp_hc036dc68__0 = 0;
    vlSelf->__VdfgTmp_h4f991360__0 = 0;
    VL_ZERO_RESET_W(66, vlSelf->__VdfgTmp_hf4db9841__0);
    vlSelf->__VdfgTmp_he518378e__0 = 0;
    vlSelf->__VdfgTmp_h4a07184e__0 = 0;
    vlSelf->__VdfgTmp_h58fbbaa8__0 = 0;
    VL_ZERO_RESET_W(80, vlSelf->__VdfgTmp_h0a2de17f__0);
    vlSelf->__VdfgTmp_heb9b4284__0 = 0;
    VL_ZERO_RESET_W(70, vlSelf->__VdfgTmp_hcfe7f827__0);
    VL_ZERO_RESET_W(70, vlSelf->__VdfgTmp_h9f5d3268__0);
    VL_ZERO_RESET_W(106, vlSelf->__VdfgTmp_hc649aea0__0);
    VL_ZERO_RESET_W(66, vlSelf->__VdfgTmp_h930585ec__0);
    vlSelf->__VdfgTmp_h0488d964__0 = 0;
    vlSelf->__VdfgTmp_h04d990b5__0 = 0;
    vlSelf->__VdfgTmp_h04f66e69__0 = 0;
    vlSelf->__VdfgTmp_h041275bb__0 = 0;
    vlSelf->__VdfgTmp_h04fd4bb8__0 = 0;
    vlSelf->__VdfgTmp_h041b8178__0 = 0;
    vlSelf->__VdfgTmp_h042dfa7f__0 = 0;
    vlSelf->__VdfgTmp_h04c9ccb5__0 = 0;
    VL_ZERO_RESET_W(66, vlSelf->__VdfgTmp_h5922ca4a__0);
    vlSelf->__VdfgTmp_h42d35f2a__0 = 0;
    vlSelf->__VdfgTmp_h42f92ea7__0 = 0;
    vlSelf->__VdfgTmp_h420f4c95__0 = 0;
    vlSelf->__VdfgTmp_h42225ad9__0 = 0;
    vlSelf->__VdfgTmp_h42456034__0 = 0;
    vlSelf->__VdfgTmp_h427adc60__0 = 0;
    vlSelf->__VdfgTmp_h5de63d8d__0 = 0;
    vlSelf->__VdfgTmp_h420a35ed__0 = 0;
    VL_ZERO_RESET_W(75, vlSelf->__VdfgTmp_h137fc90c__0);
    VL_ZERO_RESET_W(75, vlSelf->__VdfgTmp_h1373e978__0);
    vlSelf->__VdfgTmp_hb476b05a__0 = 0;
    vlSelf->__VdfgTmp_h41c787e2__0 = 0;
    vlSelf->__VdfgTmp_h41e79277__0 = 0;
    vlSelf->__VdfgTmp_h4112f00f__0 = 0;
    vlSelf->__VdfgTmp_h4107b8fa__0 = 0;
    vlSelf->__VdfgTmp_h40838ff6__0 = 0;
    vlSelf->__VdfgTmp_h44d3f3eb__0 = 0;
    vlSelf->__VdfgTmp_h44f48a4f__0 = 0;
    vlSelf->__VdfgTmp_h447f116b__0 = 0;
    VL_ZERO_RESET_W(112, vlSelf->__VdfgTmp_h50d0b174__0);
    VL_ZERO_RESET_W(112, vlSelf->__VdfgTmp_h50dc40e3__0);
    vlSelf->__VdfgTmp_h13519f20__0 = 0;
    VL_ZERO_RESET_W(112, vlSelf->__VdfgTmp_h532f22f3__0);
    vlSelf->__VdfgTmp_h6dc36e05__0 = 0;
    VL_ZERO_RESET_W(88, vlSelf->__VdfgTmp_hec80bb76__0);
    vlSelf->__VdfgTmp_h56960680__0 = 0;
    VL_ZERO_RESET_W(88, vlSelf->__VdfgTmp_hed174b9b__0);
    VL_ZERO_RESET_W(88, vlSelf->__VdfgTmp_hede01ff6__0);
    vlSelf->__VdfgTmp_hbdba66dd__0 = 0;
    vlSelf->__VdfgTmp_hb37e2670__0 = 0;
    vlSelf->__VdfgTmp_h4a264645__0 = 0;
    VL_ZERO_RESET_W(74, vlSelf->__VdfgTmp_h4b4573cc__0);
    VL_ZERO_RESET_W(106, vlSelf->__VdfgTmp_hcc2a92bf__0);
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__pack_ram__20__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__23__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__23__cur_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__24__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__24__cur_ptr = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__pack_ram__25__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__28__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__28__cur_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__29__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__29__cur_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__pack_ram__30__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__33__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__33__cur_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__34__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__u_sync_fifo__DOT__func_check_next_ptr_eq_depth__34__cur_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__40__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__40__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__40__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__40__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__40__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__41__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__41__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__41__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__41__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__41__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__42__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__42__data_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__42__align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__43__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__43__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__43__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__43__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__43__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__44__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__44__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__44__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__44__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__44__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__45__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__45__data_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_words_to_send__45__align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__46__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__46__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__46__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__46__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__46__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__47__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__47__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__47__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__47__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__47__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__48__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__48__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__48__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__48__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__48__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__49__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__49__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__49__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__49__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__49__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__50__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__50__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__50__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__50__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__50__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__51__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__51__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__51__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__51__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__51__byte_align = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__52__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__52__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__52__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__52__hd_align_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__53__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__53__hd_size = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__53__hd_ptr = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__get_alignment_n_new_ptr__53__hd_align_bits = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__56__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__56__prev_data);
    vlSelf->__Vfunc_align_data__56__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__56__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__56__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__56__n_wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__cur_data);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__prev_data);
    vlSelf->__Vfunc_align_data__57__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__wd2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__msk2);
    vlSelf->__Vfunc_align_data__57__inv_align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__wd2_msk2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__n_wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__57__n_wd2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__58__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__58__prev_data);
    vlSelf->__Vfunc_align_data__58__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__58__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__58__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__58__n_wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__59__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__59__prev_data);
    vlSelf->__Vfunc_align_data__59__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__59__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__59__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__59__n_wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__cur_data);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__prev_data);
    vlSelf->__Vfunc_align_data__60__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__wd2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__msk2);
    vlSelf->__Vfunc_align_data__60__inv_align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__wd2_msk2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__n_wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__60__n_wd2);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__61__Vfuncout);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__61__prev_data);
    vlSelf->__Vfunc_align_data__61__align_offset = 0;
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__61__wd1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__61__msk1);
    VL_ZERO_RESET_W(65, vlSelf->__Vfunc_align_data__61__n_wd1);
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__63__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__63__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__63__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__64__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__64__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__64__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__65__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__65__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__65__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__66__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__66__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__66__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__67__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__67__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__67__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__68__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__68__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__68__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__69__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__69__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__69__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__70__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__70__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__70__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__71__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__71__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__71__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__72__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__72__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__72__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__73__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__73__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__73__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__74__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__74__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__74__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__75__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__75__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__75__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__76__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__76__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__76__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__77__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__77__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__77__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__78__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__78__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__78__mtf_exp = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__79__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__79__mtf_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_header_and_numbits__79__frm_fmt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__80__Vfuncout = 0;
    VL_ZERO_RESET_W(96, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__80__new_tmp_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__get_mtf_lsb__80__mtf_exp = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__82__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__82__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__82__hdr_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__82__len_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__82__r_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__83__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__83__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__83__hdr_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__83__len_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__83__r_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__84__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__84__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__84__hdr_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__84__len_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__84__r_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__85__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__85__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__85__hdr_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__85__len_bits = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_len_fields__85__r_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__86__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__87__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__88__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__89__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__90__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__91__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__r_data = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__blast_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__dfmt1_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_fmt_fields__92__dfmt2_val = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__93__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__93__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__93__r_crc_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__94__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__94__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_crc_field__94__r_crc_len = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__95__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__95__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__95__data_in = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__96__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__96__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__96__data_in = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__97__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__97__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__97__data_in = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__98__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__98__bits_consumed = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_0_term_field__98__data_in = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__99__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__99__xlen_wrd = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__99__r_xlen = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__100__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__100__xlen_wrd = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__100__r_xlen = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__101__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__101__xlen_wrd = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__101__r_xlen = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__102__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__102__xlen_wrd = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__examine_xlen_field__102__r_xlen = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__bit_stream = 0;
    VL_ZERO_RESET_W(132, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__blt);
    VL_ZERO_RESET_W(264, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__smt);
    VL_ZERO_RESET_W(264, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__svt);
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__length = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__symbol = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__onehot_symbol = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__u_retro__DOT__u_retro_huff__DOT__huffman_decoder__103__error = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__bit_stream = 0;
    VL_ZERO_RESET_W(702, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__bct);
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__bct_valid = 0;
    VL_ZERO_RESET_W(270, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__sat);
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__length = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__base_offset = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length__104__sat_entry = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length_PRICOD__105__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__short_hufd_length_PRICOD__105__A = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__bit_stream = 0;
    VL_ZERO_RESET_W(702, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__bct);
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__bct_valid = 0;
    VL_ZERO_RESET_W(216, vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__sat);
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__length = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__base_offset = 0;
    vlSelf->__Vtask_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length__106__sat_entry = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length_PRICOD__107__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__long_hufd_length_PRICOD__107__A = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__cur_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__cur_word_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__prev_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__offset = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__110__length = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__111__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__111__word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__111__from_off = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__111__t_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__112__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__112__word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__112__t_cnt = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__113__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__113__f_cp_byte_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__113__f_cp_bytes);
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__114__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__114__f_cp_byte_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__114__f_cp_bytes);
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__cur_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__cur_word_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__prev_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__offset = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_copy_bytes__115__length = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__116__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__116__word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__116__from_off = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__116__t_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__117__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__117__word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__get_bytes_from_word__117__t_cnt = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__118__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__118__f_cp_byte_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__118__f_cp_bytes);
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__119__Vfuncout);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__119__f_cp_byte_cnt = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__fill_cp_bytes_in_word__119__f_cp_bytes);
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__120__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__120__copy_bytes);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__120__copy_length = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__121__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__121__copy_bytes);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__121__copy_length = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__122__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__122__copy_bytes);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__122__copy_offset = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__122__copy_length = 0;
    VL_ZERO_RESET_W(133, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__123__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__123__copy_bytes);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__123__copy_offset = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__copy_into_new_word__123__copy_length = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__125__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__125__cur_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__125__cur_cnt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__125__sav_word = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__125__sav_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__126__Vfuncout);
    VL_ZERO_RESET_W(128, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__126__cur_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__126__cur_cnt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__126__sav_word = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_saved_and_current__126__sav_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__127__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__127__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__127__l_cur_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__128__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__128__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__128__l_cur_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__129__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__129__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__129__l_cur_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__130__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__130__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__130__l_cur_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__131__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__131__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__131__l_cur_cnt = 0;
    VL_ZERO_RESET_W(197, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__132__Vfuncout);
    VL_ZERO_RESET_W(192, vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__132__local_word);
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__pack_into_8_bytes__132__l_cur_cnt = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__get_bip2__133__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__get_bip2__133__data_in = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__reverse_bytes__137__Vfuncout = 0;
    vlSelf->__Vfunc_cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__reverse_bytes__137__in_crc = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_nx_rbus_apb__DOT__apb_active_reg = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_core__DOT__pipe_cmd_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_frm = 0;
    VL_ZERO_RESET_W(72, vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat);
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_bimc_eccpar_debug_write_notify_ev = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_bimc_cmd2_write_notify_ev = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_axi4s_slv__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__sys_stall_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_ib_par_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_stats_en = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_word_num = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_xp10_crc32 = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_xp10_crc32 = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_error = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pac_phd_check_valid_ack = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_pfd_error = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pac_pfd_check_valid_ack = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_word_num = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_dunk_w0_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_dunk_w1_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__r_rsp = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__r_rsp = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__stats_en = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_err_save = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v0 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v0 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v1 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v1 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v2 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v2 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v3 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v3 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v4 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v4 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v5 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v5 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v6 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v6 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v7 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v7 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v8 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v8 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v9 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v9 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v10 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v10 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v11 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v11 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v12 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v12 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v13 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v13 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg__v20 = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_ordern = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_support__DOT__osf_ob_par_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_rdat = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__sw_rdat = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__rst_addr_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__sw_rdat = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__timer_r = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__rst_addr_r = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(83, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(83, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PFDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__PHDMEM__DOT__u_ram__DOT__g__DOT__mem__v0 = 0;
    VL_ZERO_RESET_W(70, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__lfa_inst__DOT__g__DOT__douta_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__lfa_inst__DOT__lfa_inst__DOT__g__DOT__douta_r__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__do_inst__DOT__hb_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix0__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix0__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix1__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix1__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    VL_ZERO_RESET_W(128, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix2__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__hb_inst__DOT__prefix2__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(83, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__tlvp_inst__DOT__rsm_inst__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    vlSelf->__Vdlyvdim0__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(83, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__r_data__v0 = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    VL_ZERO_RESET_W(92, vlSelf->__Vdlyvval__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0);
    vlSelf->__Vdlyvset__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_ram__DOT___1r1wramDxWb__DOT__dout_r__v0 = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_frm = 0;
    VL_ZERO_RESET_W(72, vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dat);
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__sync_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__r_bimc_eccpar_debug_write_notify_ev = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__bimc_master__DOT__r_bimc_cmd2_write_notify_ev = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__rqe_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pt_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__pfx_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__frm_bytes = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__fhp_inst__DOT__set_prefix_sof = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_hdr_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_stbl_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__p_data_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__one_after_last_rd = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__last_rd = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_sof_blk = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_ptr = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_align = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_words_to_send = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_deflate_raw = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_bits = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_err = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_last_blk = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_crc_option = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_fmt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_trace = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_nxt_data_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_err = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_last_blk = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__nxt_data_fmt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__f_cum_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__s_cum_size = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__s_cum_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__xp9_stbl_last = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__cur_eof_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__crc_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__first_crc_word = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_deflate_start_hdr = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_start_hdr = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__abort_frm_early = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__r_isize = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__data_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__eob_word_rd = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__next_isize = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__n_valid = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__xp9_seq_num = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__r_got_eof = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__frm_out_size = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_done = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__mtf_size = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__nxt_st = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__deflate_bits_consumed = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__tlvp_inst__DOT__axi_in__DOT__axi4s_slv_wen = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_copy_after_hb_read = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ptr_offset = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_eob = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__ep_inst__DOT__r_ep_eof = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__to_copy = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__bm_inst__DOT__last_sent = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__blk_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__l_cnt = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__lz77__DOT__pl_inst__DOT__tgl = 0;
    vlSelf->__Vdly__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__be__DOT__be_fifos__DOT__frm_chk_inst__DOT__frm_bcnt = 0;
    for (int __Vi0 = 0; __Vi0 < 10; ++__Vi0) {
        VL_ZERO_RESET_W(88, vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__rbus_ring_i__0[__Vi0]);
    }
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_fifo_empty_mod__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_footer_coding__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_num_d__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__hw_cs__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__hw_cs__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__credit_available__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__invalid_hdr_addr__0 = 0;
    VL_ZERO_RESET_W(112, vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_wbus__0);
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__all_valid__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__predef_bl_req_ready__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_bits_err__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_fifo_ren__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_go__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__hist_complete__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_ready__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_dst_ready__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__df_dbg_empty_mod__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__pf_dbg_empty_mod__0 = 0;
    vlSelf->__VstlDidInit = 0;
    for (int __Vi0 = 0; __Vi0 < 10; ++__Vi0) {
        VL_ZERO_RESET_W(88, vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__rbus_ring_i__1[__Vi0]);
    }
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__isf_fifo_empty_mod__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__user_footer_coding__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_prefix_num_d__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_lz77d_im__DOT__hw_cs__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_xpd_im__DOT__hw_cs__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__u_cr_xp10_decomp_regfile__DOT__u_htf_bl_im__DOT__credit_available__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__invalid_hdr_addr__1 = 0;
    VL_ZERO_RESET_W(112, vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__lfa_inst__DOT__sdd_fifo_wbus__1);
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__fe__DOT__bhp_inst__DOT__dflate_hdr__DOT__all_valid__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__predef_bl_req_ready__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__hdr_bits_err__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_hdr_fifo__DOT__hdr_fifo_ren__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_symtab_dec__DOT__retro_go__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__htf__DOT__u_table_writer__DOT__hist_complete__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_ld__DOT__pipe_dst_ready__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__xp10_decomp_rtl_start__DOT__u_cr_xp10_decomp_core__DOT__no_stub__DOT__hufd__DOT__sdd__DOT__u_sp__DOT__pipe_dst_ready__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__df_dbg_empty_mod__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_dbg2fifo_ctl__DOT__pf_dbg_empty_mod__1 = 0;
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__rst_sync_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cr_cddip__DOT__u_cr_xp10_decomp__DOT__rst_sync_n__0 = 0;
    vlSelf->__VactDidInit = 0;
}
