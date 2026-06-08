// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip__Syms.h"
#include "Vcr_cddip___024root.h"

extern const VlUnpacked<CData/*6:0*/, 8> Vcr_cddip__ConstPool__TABLE_h6faa2b41_0;

VL_INLINE_OPT void Vcr_cddip___024root___nba_sequent__TOP__3(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___nba_sequent__TOP__3\n"); );
    // Init
    CData/*1:0*/ cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2 = 0;
    IData/*31:0*/ cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn = 0;
    IData/*31:0*/ cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd = 0;
    CData/*1:0*/ cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__par;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__par = 0;
    CData/*1:0*/ __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__Vfuncout;
    __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__Vfuncout = 0;
    QData/*63:0*/ __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in;
    __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in = 0;
    CData/*2:0*/ __Vtableidx1;
    __Vtableidx1 = 0;
    CData/*2:0*/ __Vtableidx2;
    __Vtableidx2 = 0;
    VlWide<3>/*95:0*/ __Vtemp_120;
    VlWide<3>/*95:0*/ __Vtemp_137;
    // Body
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr))) 
           & ((4U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)) 
              | ((0xcU <= (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)) 
                 & (0x14U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)))));
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_agg_su_cntr__DOT__rd_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__rd_stb) 
           & ((0x94U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
              & (0x98U > (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))));
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data = 0U;
    if (((((((((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
               | (0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
              | (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
             | (0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
            | (0x60U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
           | (0x64U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
          | (0x68U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
         | (0x6cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)))) {
        if ((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [3U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [3U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [3U][3U] << 0x14U)));
        } else if ((0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                      [4U][0U]));
        } else if ((0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                    [4U][1U] << 8U) 
                                   | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [4U][0U] >> 0x18U))));
        } else if ((0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                    [4U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                          [4U][1U] 
                                          >> 0x10U));
        } else if ((0x60U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [4U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [4U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [4U][3U] << 0x14U)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x64U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))
                    ? ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                       | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                          [5U][0U])) : ((0x68U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))
                                         ? ((0xff000000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                                            | (0xffffffU 
                                               & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                                   [5U][1U] 
                                                   << 8U) 
                                                  | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                                     [5U][0U] 
                                                     >> 0x18U))))
                                         : ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                             [5U][2U] 
                                             << 0x10U) 
                                            | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                               [5U][1U] 
                                               >> 0x10U))));
        }
    } else if (((((((((0x70U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
                      | (0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                     | (0x78U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                    | (0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                   | (0x80U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                  | (0x84U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                 | (0x88U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                | (0x8cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)))) {
        if ((0x70U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [5U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [5U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [5U][3U] << 0x14U)));
        } else if ((0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                      [6U][0U]));
        } else if ((0x78U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                    [6U][1U] << 8U) 
                                   | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [6U][0U] >> 0x18U))));
        } else if ((0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                    [6U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                          [6U][1U] 
                                          >> 0x10U));
        } else if ((0x80U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [6U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [6U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [6U][3U] << 0x14U)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
                = ((0x84U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))
                    ? ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                       | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                          [7U][0U])) : ((0x88U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))
                                         ? ((0xff000000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
                                            | (0xffffffU 
                                               & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                                   [7U][1U] 
                                                   << 8U) 
                                                  | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                                     [7U][0U] 
                                                     >> 0x18U))))
                                         : ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                             [7U][2U] 
                                             << 0x10U) 
                                            | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                               [7U][1U] 
                                               >> 0x10U))));
        }
    } else if ((0x90U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
            = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
               | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                 [7U][3U] << 0x10U) 
                                | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                   [7U][2U] >> 0x10U))));
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
            = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data) 
               | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                 [7U][3U] << 0x14U)));
    } else if ((0x94U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_1_data 
            = vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__agg_su_count
            [0U];
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data = 0U;
    if (((((((((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
               | (4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
              | (8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
             | (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
            | (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
           | (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
          | (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
         | (0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)))) {
        if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data);
        } else if ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_cr_su_regs__o_spare_config;
        } else if ((8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT____Vcellout__u_cr_su_regs__o_dbg_config));
        } else if ((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xfffffff8U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_core__DOT__u_cr_su_ctl__DOT__su_hb_add));
        } else if ((0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                      [0U][0U]));
        } else if ((0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                    [0U][1U] << 8U) 
                                   | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [0U][0U] >> 0x18U))));
        } else if ((0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                    [0U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                          [0U][1U] 
                                          >> 0x10U));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [0U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [0U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [0U][3U] << 0x14U)));
        }
    } else if (((((((((0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
                      | (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                     | (0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                    | (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                   | (0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                  | (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                 | (0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
                | (0x40U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)))) {
        if ((0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                      [1U][0U]));
        } else if ((0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                    [1U][1U] << 8U) 
                                   | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [1U][0U] >> 0x18U))));
        } else if ((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                    [1U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                          [1U][1U] 
                                          >> 0x10U));
        } else if ((0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [1U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [1U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [1U][3U] << 0x14U)));
        } else if ((0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                      [2U][0U]));
        } else if ((0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                    [2U][1U] << 8U) 
                                   | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [2U][0U] >> 0x18U))));
        } else if ((0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                    [2U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                          [2U][1U] 
                                          >> 0x10U));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0xf8000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x7ffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [2U][3U] << 0x10U) 
                                    | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                       [2U][2U] >> 0x10U))));
            vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                     [2U][3U] << 0x14U)));
        }
    } else if ((0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
            = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
               | (0xffffffU & vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                  [3U][0U]));
    } else if ((0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
            = ((0xff000000U & vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data) 
               | (0xffffffU & ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                [3U][1U] << 8U) | (
                                                   vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                                   [3U][0U] 
                                                   >> 0x18U))));
    } else if ((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__r32_mux_0_data 
            = ((vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                [3U][2U] << 0x10U) | (vlSelf->cr_cddip__DOT__u_cr_su__DOT__su_hb
                                      [3U][1U] >> 0x10U));
    }
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr))) 
           & ((0xcU >= (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
              | ((0x14U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)) 
                 & (0x94U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__reg_addr)))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr))) 
           & ((4U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)) 
              | (((0xcU <= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)) 
                  & (0x4cU >= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr))) 
                 | ((0x58U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)) 
                    & (0x5cU >= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr))))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
           & (0x54U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__wr_stb = 
        ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
         && (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
              & (0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))) 
             & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_valid_rd_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__mem_wr_init_dly 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                     >> 3U)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_cmd2_send 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2) 
                     >> 8U)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_send 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                     >> 0x16U)));
    if (vlSelf->cr_cddip__DOT__rst_sync_n) {
        if ((1U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (1U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((1U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffffeU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((2U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (2U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((2U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffffdU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((4U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (4U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((4U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffffbU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((8U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (8U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((8U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffff7U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x10U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x10U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x10U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fffefU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x20U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x20U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x20U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fffdfU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x40U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x40U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x40U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fffbfU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x80U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x80U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x80U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fff7fU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x100U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffeffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x200U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffdffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x400U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x400U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x400U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ffbffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x800U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x800U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x800U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1ff7ffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x1000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x1000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fefffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x2000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x2000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x2000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fdfffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x4000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x4000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x4000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1fbfffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x8000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x8000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x8000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1f7fffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x10000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x10000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x10000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1effffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x20000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x20000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x20000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1dffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x40000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x40000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x40000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x1bffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x80000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x17ffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__cddip_int_stb)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0x100000U | vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        } else if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid)) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                = (0xfffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg);
        }
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr_stb_valid) 
             & (2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_mask_reg 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_data;
        }
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_next_state;
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_wr_strb) 
             | (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_rd_strb))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr 
                = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_addr;
        }
        if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_ctl_config) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x7f8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (7U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x7c7U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x38U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x7bfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x40U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data 
                               >> 2U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x77fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x80U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data 
                               >> 5U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x6ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x100U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data 
                                >> 5U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x5ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x200U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data 
                                >> 5U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config 
                = ((0x3ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)) 
                   | (0x400U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f32_data 
                                >> 5U)));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_cmd1) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1 
                = ((0xffff0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1) 
                   | (0xffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1 
                = ((0xf000ffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1) 
                   | (0xfff0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1 
                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1) 
                   | (0xf0000000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_state)) 
             & (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__reg_addr)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd0 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data;
        }
        if (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_state)) 
             & (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__reg_addr)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_ecc_uncorrectable_error_cnt 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data;
        }
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2) 
                   >> 9U)) & (((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                               & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din)));
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2) 
                   >> 9U)) & ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din)));
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2) 
                   >> 9U)) & (((6U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                               & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din)));
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2) 
                   >> 9U)) & (((0xaU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                               & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__number_of_memories 
            = (0xfffU & (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                          & (0xf02fff0001ULL == (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type)) 
                                                  << 0x24U) 
                                                 | (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op)) 
                                                     << 0x1cU) 
                                                    | (QData)((IData)(
                                                                      (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem) 
                                                                        << 0x10U) 
                                                                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))))))))
                          ? ((0xfffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat) 
                             - (IData)(1U)) : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__number_of_memories)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_global_config_bimc_mem_init_done_din 
            = ((0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate)) 
               & ((((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                    & (2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                   & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_global_config_bimc_mem_init_done_din)));
        if ((((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
              & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat;
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din;
        }
        if (((3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat;
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din;
        }
        if ((((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
              & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din 
                = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type) 
                    << 0x1cU) | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem) 
                                  << 0x10U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat;
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din;
        }
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask) 
                   >> 4U)) & ((((((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                                  | (8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))) 
                                 | (2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))) 
                                & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm))) 
                               & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame)) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din)));
        if ((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
              & (0U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din 
                = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type) 
                    << 0x1cU) | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem) 
                                  << 0x10U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat;
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din;
        }
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask) 
                   >> 1U)) & (((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                                 & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                                & (0xeccU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
                               & (3U == (3U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask) 
                   >> 2U)) & (((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                                 & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                                & (0x9a4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
                               & (0U != vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat)) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din 
            = ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
               & (((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                     & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                    & (0xeccU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
                   & ((vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat 
                       >> 2U) & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat)) 
                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask) 
                   >> 5U)) & ((((3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                                & (1U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                               | ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                                  & (3U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op)))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din 
            = ((~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask) 
                   >> 6U)) & (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                               & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                              | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt_uncorrectable_ecc_en 
            = ((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                 & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                & (0xeccU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
               & ((vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat 
                   >> 2U) & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt_correctable_ecc_en 
            = ((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                 & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                & (0xeccU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
               & (3U == (3U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt_parity_errors_en 
            = ((((9U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)) 
                 & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))) 
                & (0x9a4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr))) 
               & (0U != vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat));
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_phd_crc_addr) 
             | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter = 0U;
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_eod = 0U;
        } else {
            if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_phd_addr) {
                vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter 
                    = (0xffU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter)));
            }
            if ((0x41U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_eod = 1U;
            }
        }
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_pfd_crc_addr) 
             | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter = 0U;
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_eod = 0U;
        } else {
            if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_pfd_addr) {
                vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter 
                    = (0xffU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter)));
            }
            if ((0x80U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_eod = 1U;
            }
        }
        if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd) {
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[0U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U];
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[1U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[1U];
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[2U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U];
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[3U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[3U];
            if ((1U & (~ (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                          >> 0x1bU)))) {
                if ((1U & (~ (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                              >> 0x1aU)))) {
                    if ((1U & (~ (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                  >> 0x19U)))) {
                        if ((0x1000000U & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                            if ((1U & (~ (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                          >> 0x17U)))) {
                                if ((0x400000U & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                    if ((0x200000U 
                                         & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                        if ((1U & (~ 
                                                   (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                    >> 0x14U)))) {
                                            if ((0x40000U 
                                                 & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding 
                                                    = 
                                                    (3U 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U] 
                                                        >> 0x18U));
                                            }
                                        }
                                    }
                                } else if ((0x200000U 
                                            & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                    if ((1U & (~ (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                  >> 0x14U)))) {
                                        if ((0x40000U 
                                             & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding 
                                                = (3U 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U] 
                                                      >> 0x18U));
                                        }
                                    }
                                } else if ((0x40000U 
                                            & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding 
                                        = (3U & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U] 
                                                 >> 0x18U));
                                }
                            }
                        } else if ((0x800000U & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                            if ((0x400000U & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                if ((0x200000U & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                    if ((0x100000U 
                                         & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                        if ((0x40000U 
                                             & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])) {
                                            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding 
                                                = (3U 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U] 
                                                      >> 0x18U));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__rs_next_cddip_int_control_addr;
        if (((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f_state)) 
             & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__r_next_cddip_int_control_write) 
                & (0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__reg_addr)))))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_data 
                = (0x1fffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data);
        }
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_next_state;
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_wr_strb) 
             | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_rd_strb))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr 
                = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_addr;
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_global_config) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (1U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xfffffffdU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (2U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xfffffffbU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (4U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xfffffff7U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (8U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xffffffefU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (0x10U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0xffffffdfU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (0x20U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config 
                = ((0x3fU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config) 
                   | (0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_cmd2) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 
                = ((0x700U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2)) 
                   | (0xffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 
                = ((0x6ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2)) 
                   | (0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 
                = ((0x5ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2)) 
                   | (0x200U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 
                = ((0x3ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2)) 
                   | (0x400U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_eccpar_debug) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1ffff000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0xfffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1fff0fffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0xf000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1ffcffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0x30000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1ff3ffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0xc0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1fcfffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0x300000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1fbfffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0x400000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x1f7fffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0x800000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0x10ffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0xf000000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                   | (0x10000000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type 
            = (0xfU & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[2U] 
                       >> 4U));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem 
            = (0xfffU & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[1U] 
                         >> 0x10U));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr 
            = (0xffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[1U]);
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[0U];
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_rstate;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op 
            = (0xffU & ((vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[2U] 
                         << 4U) | (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[1U] 
                                   >> 0x1cU)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate;
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_monitor_mask) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x7eU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (1U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x7dU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (2U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x7bU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (4U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x77U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (8U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x6fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (0x10U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x5fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (0x20U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask 
                = ((0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask)) 
                   | (0x40U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_dbgcmd2) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2 
                = ((0x300U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2)) 
                   | (0xffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2 
                = ((0x2ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2)) 
                   | (0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_rxcmd2) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2 
                = ((0x300U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2)) 
                   | (0xffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2 
                = ((0x2ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2)) 
                   | (0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_rxrsp2) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2 
                = ((0x300U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2)) 
                   | (0xffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2 
                = ((0x2ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2)) 
                   | (0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__w_load_bimc_pollrsp2) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2 
                = ((0x300U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2)) 
                   | (0xffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2 
                = ((0x2ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2)) 
                   | (0x100U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cr_cddip_support_regs__DOT__f32_data));
        }
    } else {
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_mask_reg = 0U;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state = 0U;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd1 = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd0 = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_ecc_uncorrectable_error_cnt = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__number_of_memories = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_global_config_bimc_mem_init_done_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_opcode_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_memtype_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_mem_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd1_addr_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd0_data_din = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_opcode_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_memtype_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_mem_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd1_addr_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd0_data_din = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp0_data_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp1_data_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_data_din = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp0_data_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp1_data_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_data_din = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt_uncorrectable_ecc_en = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_correctable_error_cnt_correctable_ecc_en = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_parity_error_cnt_parity_errors_en = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phd_eod = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfd_eod = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[0U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[1U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[2U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0[3U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_frmd_coding = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state = 0U;
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_global_config = 1U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2 = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_type = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_mem = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_addr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_dat = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate = 2U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_monitor_mask = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2 
            = (0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_dbgcmd2));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2 
            = (0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxcmd2));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2 
            = (0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_rxrsp2));
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2 
            = (0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_pollrsp2));
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_ecc_uncorrectable_error_cnt 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_ecc_uncorrectable_error_cnt;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add 
        = (0xffU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r)
                     ? (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r)
                     : (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add 
        = (0x1ffU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r)
                      ? (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r)
                      : (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__sw_add 
        = (0x3ffU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_or_ini_r)
                      ? (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__rst_addr_r)
                      : (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_inc_phd_addr_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_phd_addr));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_inc_pfd_addr_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_pfd_addr));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__tlvp_ob_rd) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__tlvp_ob_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__tlvp_ob_rd) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__tlvp_ob_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__tlvp_ob_rd) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__tlvp_ob_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x1fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__timer_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__timer_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_word_num 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_word_num;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__timer_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_clr = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state))));
    __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
        = (((QData)((IData)((0x7ffU & (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                               [0U] 
                                               >> 0x20U))))) 
            << 0x20U) | (QData)((IData)((((IData)((
                                                   vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                   [0U] 
                                                   >> 0x1cU)) 
                                          << 0x1cU) 
                                         | ((0xf000000U 
                                             & ((IData)(
                                                        (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                         [0U] 
                                                         >> 0x18U)) 
                                                << 0x18U)) 
                                            | ((0xff0000U 
                                                & ((IData)(
                                                           (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                            [0U] 
                                                            >> 0x10U)) 
                                                   << 0x10U)) 
                                               | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux) 
                                                   << 8U) 
                                                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))))))));
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn = 0U;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd = 0U;
    if ((1U & (IData)(__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 2U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 4U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 6U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 8U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xaU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xcU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xeU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x10U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x12U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x14U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x16U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x18U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1aU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1cU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1eU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x20U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x22U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x24U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x26U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x28U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2aU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2cU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2eU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x30U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x32U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x34U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x36U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x38U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3aU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3cU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3eU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 1U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 3U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 5U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 7U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 9U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xbU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xdU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0xfU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x11U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x13U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x15U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x17U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x19U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1bU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1dU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x1fU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x21U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x23U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x25U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x27U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x29U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2bU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2dU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x2fU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x31U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x33U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x35U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x37U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x39U)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3bU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3dU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    if ((1U & (IData)((__Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__data_in 
                       >> 0x3fU)))) {
        cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd 
            = ((IData)(1U) + cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd);
    }
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__par 
        = ((VL_LTS_III(32, 0U, VL_MODDIVS_III(32, cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__odd, (IData)(2U))) 
            << 1U) | VL_LTS_III(32, 0U, VL_MODDIVS_III(32, cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__evn, (IData)(2U))));
    __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__Vfuncout 
        = cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__Vstatic__par;
    cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2 
        = __Vfunc_cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__get_bip2__145__Vfuncout;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame 
        = ((2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dbgcmd2_rxflag_din;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxcmd2_rxflag_din;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_rxrsp2_rxflag_din;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_pollrsp2_rxflag_din;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_phd_addr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_phd_inwrk) 
           & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_phdcounter;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_inc_pfd_addr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_inwrk) 
           & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_pfdcounter;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__yield 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__wr_stb) 
         & (0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)))) {
        if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                              >> 7U)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                          >> 8U)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                              >> 6U)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                      >> 9U)))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                          >> 8U)))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__yield 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__wr_stb) 
         & (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)))) {
        if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                              >> 7U)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                          >> 8U)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                              >> 6U)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                      >> 9U)))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                          >> 8U)))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv0_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd) 
               && ((1U & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                          >> 0x1bU)) || ((1U & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                >> 0x1aU)) 
                                         || ((1U & 
                                              (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                               >> 0x19U)) 
                                             || ((0x1000000U 
                                                  & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])
                                                  ? 
                                                 ((1U 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                      >> 0x17U)) 
                                                  || ((0x400000U 
                                                       & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])
                                                       ? 
                                                      ((1U 
                                                        & (~ 
                                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                            >> 0x15U))) 
                                                       || (1U 
                                                           & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                              >> 0x14U)))
                                                       : 
                                                      ((1U 
                                                        & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                           >> 0x15U)) 
                                                       && (1U 
                                                           & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                              >> 0x14U)))))
                                                  : 
                                                 ((0x800000U 
                                                   & vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U])
                                                   ? 
                                                  ((1U 
                                                    & (~ 
                                                       (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                        >> 0x16U))) 
                                                   || ((1U 
                                                        & (~ 
                                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                            >> 0x15U))) 
                                                       || (1U 
                                                           & (~ 
                                                              (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                               >> 0x14U)))))
                                                   : 
                                                  ((1U 
                                                    & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                       >> 0x16U)) 
                                                   || ((1U 
                                                        & (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                           >> 0x15U)) 
                                                       || (1U 
                                                           & (~ 
                                                              (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                                                               >> 0x14U))))))))))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_tlv_tdata 
        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[1U])) 
            << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[0U])));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_tlv_tdata 
        = (0xc00007ffffffffffULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_tlv_tdata);
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1_valid) 
         & (0x580000U == (0xff80000U & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[2U])))) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata 
            = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[1U])) 
                << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[0U])));
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata 
            = ((0xc000000000000000ULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata) 
               | (((QData)((IData)((0x7fffU & ((vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[1U] 
                                                << 4U) 
                                               | (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[0U] 
                                                  >> 0x1cU))))) 
                   << 0x1cU) | (QData)((IData)((0x8603U 
                                                | (0xfff0000U 
                                                   & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[0U]))))));
    } else {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_ibp_tlv_word0_data;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata 
            = ((0xc00000000fffffffULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata) 
               | (((QData)((IData)((0x7ffU & (IData)(
                                                     (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_ibp_tlv_word0_data 
                                                      >> 0x20U))))) 
                   << 0x20U) | ((QData)((IData)((0xfU 
                                                 & (IData)(
                                                           (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_ibp_tlv_word0_data 
                                                            >> 0x1cU))))) 
                                << 0x1cU)));
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata 
            = ((0xfffffffff0000000ULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_phd_tlv_tdata) 
               | (IData)((IData)((0x8603U | (0xfff0000U 
                                             & ((IData)(
                                                        (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pti__DOT__pti_ibp_tlv_word0_data 
                                                         >> 0x10U)) 
                                                << 0x10U))))));
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err 
        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[1U])) 
            << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[0U])));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err 
        = ((0xfffffffffffff800ULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err) 
           | (IData)((IData)((0x7ffU & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ibp_tlv_ftr_word_0[1U]))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err 
        = ((0xfffffffffff00fffULL & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_ftr_word13err) 
           | ((QData)((IData)(((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_phd_error)
                                ? 0x9bU : ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_pfd_error)
                                            ? 0x9cU
                                            : 0U)))) 
              << 0xcU));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_pfd_crc_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_pfd_crc_addr_r));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pmc_phd_crc_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_phd_crc_addr_r));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__yield 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__timer_r;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_wdat 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_r)
            ? 0U : vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_wdata_part0);
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__sw_add 
        = (0x3fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_or_ini_r)
                     ? (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__rst_addr_r)
                     : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__wr_stb) 
         & (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)))) {
        if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                              >> 7U)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                          >> 8U)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                              >> 6U)))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                      >> 9U)))) {
            if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                          >> 8U)))) {
                if ((0x80U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                              >> 7U)))) {
                    if ((0x40U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                  >> 6U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_cg_tlv_parse_action_0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state)) 
           & (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_cg_tlv_parse_action_1 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state)) 
           & (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_load_debug_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state)) 
           & (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__o_reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_regfile__DOT__u_cr_cg_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_ctrl_ia_wdata_part0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_ctrl_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_snapshot_ia_wdata_part0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_snapshot_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_count_ia_wdata_part0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_count_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cceip_tlv_parse_action_0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)) 
           & (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cceip_tlv_parse_action_1 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)) 
           & (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_error_control 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)) 
           & (0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cddip_tlv_parse_action_0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)) 
           & (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_load_regs_cddip_tlv_parse_action_1 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state)) 
           & (0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_regfile__DOT__u_cr_prefix_attach_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_su__DOT__u_cr_su_regfile__DOT__u_cr_su_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
        = ((0xfffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
           | (0xfff00000U & ((0xf0000000U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U]) 
                             | (0xff00000U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_tlv_w0_reg[2U]))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
        = (0x1ffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U]);
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
        = ((0xfff000ffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
           | (0xffffff00U & (0xff00U | (0x10000U & 
                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ftr_tlv_w0_reg[2U]))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] = 0U;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
        = (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
    if ((0x10U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
        if ((8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                              >> 1U)))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                        if ((1U & (~ (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w0_val) 
                                       & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                                      & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen)))))) {
                            if ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen) 
                                  & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                                 & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)) {
                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_clr = 1U;
                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 0U;
                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 0U;
                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 0U;
                                if ((1U & (~ (((0xfU 
                                                == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                                               || (0x10U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                                              || (0x11U 
                                                  == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))) {
                                    if ((0x12U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        if ((0x16U 
                                             != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            if ((0xcU 
                                                 != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                if (
                                                    (0xdU 
                                                     != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                    if (
                                                        (0xeU 
                                                         == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 1U;
                                                    }
                                                }
                                            }
                                        }
                                        if ((0x16U 
                                             == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 1U;
                                        }
                                    }
                                    if ((0x12U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 1U;
                                    }
                                }
                            }
                        }
                        if ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w0_val) 
                              & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen)))) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                                   | (((IData)(1U) 
                                       + ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                           << 4U) | 
                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                           >> 0x1cU))) 
                                      << 0x1cU));
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                                = ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]) 
                                   | (0x1ffU & (((IData)(1U) 
                                                 + 
                                                 ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                   << 4U) 
                                                  | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                     >> 0x1cU))) 
                                                >> 4U)));
                        } else if ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen) 
                                     & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                                    & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                                = (IData)((((QData)((IData)(
                                                            (((IData)(cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2) 
                                                              << 0x1eU) 
                                                             | (0x7ffU 
                                                                & (IData)(
                                                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                           [0U] 
                                                                           >> 0x20U)))))) 
                                            << 0x20U) 
                                           | (QData)((IData)(
                                                             (((IData)(
                                                                       (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                        [0U] 
                                                                        >> 0x1cU)) 
                                                               << 0x1cU) 
                                                              | ((0xf000000U 
                                                                  & ((IData)(
                                                                             (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                              [0U] 
                                                                              >> 0x18U)) 
                                                                     << 0x18U)) 
                                                                 | ((0xff0000U 
                                                                     & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x10U)) 
                                                                        << 0x10U)) 
                                                                    | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux) 
                                                                        << 8U) 
                                                                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))))));
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                                = (IData)(((((QData)((IData)(
                                                             (((IData)(cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2) 
                                                               << 0x1eU) 
                                                              | (0x7ffU 
                                                                 & (IData)(
                                                                           (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                            [0U] 
                                                                            >> 0x20U)))))) 
                                             << 0x20U) 
                                            | (QData)((IData)(
                                                              (((IData)(
                                                                        (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                         [0U] 
                                                                         >> 0x1cU)) 
                                                                << 0x1cU) 
                                                               | ((0xf000000U 
                                                                   & ((IData)(
                                                                              (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                               [0U] 
                                                                               >> 0x18U)) 
                                                                      << 0x18U)) 
                                                                  | ((0xff0000U 
                                                                      & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x10U)) 
                                                                         << 0x10U)) 
                                                                     | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux) 
                                                                         << 8U) 
                                                                        | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))))) 
                                           >> 0x20U));
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                = (0x80000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                = (1U | (0xffffff00U 
                                         & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                                   | (((IData)(1U) 
                                       + ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                           << 4U) | 
                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                           >> 0x1cU))) 
                                      << 0x1cU));
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                                = ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]) 
                                   | (0x1ffU & (((IData)(1U) 
                                                 + 
                                                 ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                   << 4U) 
                                                  | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                     >> 0x1cU))) 
                                                >> 4U)));
                            if ((1U & (~ (((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                                           || (0x10U 
                                               == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                                          || (0x11U 
                                              == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))) {
                                if ((0x12U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                    if ((0x16U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        if ((0xcU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            if ((0xdU 
                                                 != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                if (
                                                    (0xeU 
                                                     != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                    if (
                                                        (0xbU 
                                                         == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                            = 
                                                            (0x40000U 
                                                             | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                            = 
                                                            (3U 
                                                             | (0xffffff00U 
                                                                & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                                                    } else {
                                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                            = 
                                                            (0x40000U 
                                                             | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                                                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                            = 
                                                            (3U 
                                                             | (0xffffff00U 
                                                                & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                    = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))
                        ? 0U : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))
                                 ? 0U : ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w0_val) 
                                           & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                                          & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen)))
                                          ? 0x16U : 
                                         ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen) 
                                            & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                                           & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)
                                           ? ((((0xfU 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                                                || (0x10U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                                               || (0x11U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))
                                               ? 5U
                                               : ((0x12U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                   ? 5U
                                                   : 
                                                  ((0x16U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                    ? 5U
                                                    : 
                                                   ((0xcU 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                     ? 0x11U
                                                     : 
                                                    ((0xdU 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                      ? 0x12U
                                                      : 
                                                     ((0xeU 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                       ? 0x13U
                                                       : 0x1cU))))))
                                           : 0x1cU))));
            } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____VdfgExtracted_h5b9f775b__0) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_fgen_clr = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 0U;
                        if ((1U & (~ (((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                                       || (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                                      || (0x11U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))) {
                            if ((0x12U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                if ((0x16U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                    if ((0xcU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        if ((0xdU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            if ((0xeU 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 1U;
                                            }
                                        }
                                    }
                                }
                                if ((0x16U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 1U;
                                }
                            }
                            if ((0x12U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 1U;
                            }
                        }
                    }
                }
                if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                               & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                                  >> 0xdU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1cU;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                            = (0x40000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                            = (0xf02U | (0xffff0000U 
                                         & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)((QData)((IData)(
                                                      ((0x3000000U 
                                                        & ((IData)(
                                                                   (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                    [0U] 
                                                                    >> 0x2cU)) 
                                                           << 0x18U)) 
                                                       | (0xffffffU 
                                                          & (IData)(
                                                                    (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                     [0xdU] 
                                                                     >> 0x14U)))))));
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)(((QData)((IData)(
                                                       ((0x3000000U 
                                                         & ((IData)(
                                                                    (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                     [0U] 
                                                                     >> 0x2cU)) 
                                                            << 0x18U)) 
                                                        | (0xffffffU 
                                                           & (IData)(
                                                                     (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                      [0xdU] 
                                                                      >> 0x14U)))))) 
                                       >> 0x20U));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1bU;
                    }
                } else if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____VdfgExtracted_h5b9f775b__0) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                             || (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                            || (0x11U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))
                            ? 5U : ((0x12U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                     ? 5U : ((0x16U 
                                              == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                              ? 5U : 
                                             ((0xcU 
                                               == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                               ? 0x11U
                                               : ((0xdU 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                   ? 0x12U
                                                   : 
                                                  ((0xeU 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                    ? 0x13U
                                                    : 0x1cU))))));
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)((((QData)((IData)(
                                                    (((IData)(cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2) 
                                                      << 0x1eU) 
                                                     | (0x7ffU 
                                                        & (IData)(
                                                                  (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                   [0U] 
                                                                   >> 0x20U)))))) 
                                    << 0x20U) | (QData)((IData)(
                                                                (((IData)(
                                                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                           [0U] 
                                                                           >> 0x1cU)) 
                                                                  << 0x1cU) 
                                                                 | ((0xf000000U 
                                                                     & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x18U)) 
                                                                        << 0x18U)) 
                                                                    | ((0xff0000U 
                                                                        & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x10U)) 
                                                                           << 0x10U)) 
                                                                       | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux) 
                                                                           << 8U) 
                                                                          | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))))));
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)(((((QData)((IData)(
                                                     (((IData)(cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_bip2) 
                                                       << 0x1eU) 
                                                      | (0x7ffU 
                                                         & (IData)(
                                                                   (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                    [0U] 
                                                                    >> 0x20U)))))) 
                                     << 0x20U) | (QData)((IData)(
                                                                 (((IData)(
                                                                           (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                            [0U] 
                                                                            >> 0x1cU)) 
                                                                   << 0x1cU) 
                                                                  | ((0xf000000U 
                                                                      & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x18U)) 
                                                                         << 0x18U)) 
                                                                     | ((0xff0000U 
                                                                         & ((IData)(
                                                                                (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                                [0U] 
                                                                                >> 0x10U)) 
                                                                            << 0x10U)) 
                                                                        | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_len_mux) 
                                                                            << 8U) 
                                                                           | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))))) 
                                   >> 0x20U));
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                        = (0x80000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                        = (1U | (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                        = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                           | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)
                                ? (0x1fffU & ((IData)(1U) 
                                              + ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                  << 4U) 
                                                 | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                    >> 0x1cU))))
                                : 1U) << 0x1cU));
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                        = ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]) 
                           | (0x3ffU & (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)
                                          ? (0x1fffU 
                                             & ((IData)(1U) 
                                                + (
                                                   (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                    << 4U) 
                                                   | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                      >> 0x1cU))))
                                          : 1U) >> 4U)));
                    if ((1U & (~ (((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)) 
                                   || (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) 
                                  || (0x11U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type)))))) {
                        if ((0x12U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                            if ((0x16U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                if ((0xcU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                    if ((0xdU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        if ((0xeU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            if ((0xbU 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                    = 
                                                    (0x40000U 
                                                     | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                    = 
                                                    (3U 
                                                     | (0xffffff00U 
                                                        & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                                            } else {
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                    = 
                                                    (0x40000U 
                                                     | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                                                    = 
                                                    (3U 
                                                     | (0xffffff00U 
                                                        & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1aU;
                }
            } else {
                if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0x19U : 0U);
                } else {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0x18U : 0x19U);
                }
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                            = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                               | (((IData)(1U) + ((
                                                   vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                   << 4U) 
                                                  | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                     >> 0x1cU))) 
                                  << 0x1cU));
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                            = ((0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]) 
                               | (0x1ffU & (((IData)(1U) 
                                             + ((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[3U] 
                                                 << 4U) 
                                                | (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out[2U] 
                                                   >> 0x1cU))) 
                                            >> 4U)));
                    }
                }
            }
        } else if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                         & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_val))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x18U;
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x17U;
                    }
                } else {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0x16U : 0x17U);
                }
            } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                           & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                              >> 5U)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0xdU;
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x15U;
                }
            } else if (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                        & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                           >> 0x13U))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x15U;
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x14U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                          >> 1U)))) {
                if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                               & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                                  >> 5U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                      [5U]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [5U] >> 0x20U));
                    }
                } else if (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                            & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                               >> 0x13U))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [0x13U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [0x13U] >> 0x20U));
                }
            }
        } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                     & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                        >> 0x13U))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x14U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [0x13U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [0x13U] >> 0x20U));
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x13U;
                }
            } else if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                              & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                                 >> 5U)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1cU;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                              [5U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                               [5U] >> 0x20U));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (0x40000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (2U | (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x12U;
            }
        } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                       & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                          >> 0xcU)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1cU;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)((QData)((IData)((0xffffU 
                                               & (IData)(
                                                         (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                          [0xcU] 
                                                          >> 0x30U))))));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)(((QData)((IData)((0xffffU 
                                                & (IData)(
                                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                           [0xcU] 
                                                           >> 0x30U))))) 
                               >> 0x20U));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (0x40000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (2U | (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x11U;
            }
        } else {
            if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                              [4U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                               [4U] >> 0x20U));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = ((0xfffbffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                       | ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm) 
                          << 0x12U));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = ((0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]) 
                       | ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm)
                           ? 2U : 0U));
            }
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                    ? 0x10U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm)
                                ? 0xbU : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm)
                                           ? 0x1cU : 0x1bU)));
        }
    } else {
        if ((8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                                = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                          [3U]);
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                                = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                           [3U] >> 0x20U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                            = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                                ? 0xfU : 0x10U);
                    } else {
                        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                                = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                          [2U]);
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                                = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                           [2U] >> 0x20U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                            = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                                ? 0xeU : 0xfU);
                    }
                } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                      [1U]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [1U] >> 0x20U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0xdU : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short)
                                       ? 0xbU : 0xeU));
                } else {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                      [0x13U]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [0x13U] >> 0x20U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0xcU : 0x1bU);
                }
            } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                      [0x13U]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [0x13U] >> 0x20U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0xbU : 0xcU);
                } else {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                      [0xaU]);
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [0xaU] >> 0x20U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                            ? 0xaU : (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm) 
                                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short))
                                       ? 0xdU : 0xbU));
                }
            } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [9U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [9U] >> 0x20U));
                }
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                    = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                        ? 9U : 0xaU);
            } else {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [8U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [8U] >> 0x20U));
                }
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                    = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                        ? 8U : (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm) 
                                 | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short))
                                 ? 9U : 0xbU));
            }
        } else if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                            = (IData)(((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                        ? vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                       [7U] : ((0x10U 
                                                == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                ? vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                               [1U]
                                                : (
                                                   (0x11U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                   [1U]
                                                    : 
                                                   ((0x16U 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                     ? 
                                                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                    [7U]
                                                     : 
                                                    ((0x12U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                      ? 
                                                     vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                     [7U]
                                                      : 
                                                     vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                     [1U]))))));
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                            = (IData)((((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                         ? vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                        [7U] : ((0x10U 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                 ? 
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                [1U]
                                                 : 
                                                ((0x11U 
                                                  == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                  ? 
                                                 vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                 [1U]
                                                  : 
                                                 ((0x16U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                   ? 
                                                  vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                  [7U]
                                                   : 
                                                  ((0x12U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                   [7U]
                                                    : 
                                                   vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                   [1U]))))) 
                                       >> 0x20U));
                    }
                }
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt 
                    = ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))
                        ? 0U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)
                                 ? 6U : ((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                          ? 8U : ((0x10U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                   ? 0x1bU
                                                   : 
                                                  ((0x11U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                    ? 0xeU
                                                    : 
                                                   ((0x16U 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                     ? 8U
                                                     : 
                                                    ((0x12U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))
                                                      ? 8U
                                                      : 0x1bU)))))));
            } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                           & (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val 
                              >> 0xbU)))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 6U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [0xbU]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [0xbU] >> 0x20U));
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 5U;
                }
            } else if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                              & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1aU;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                              [0U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                               [0U] >> 0x20U));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (0x40000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                    = (2U | (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 4U;
            }
        } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                           & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val))) {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 4U;
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                        = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                  [0U]);
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                        = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                   [0U] >> 0x20U));
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 3U;
                }
            } else if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                              & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 3U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                              [0U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                               [0U] >> 0x20U));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 2U;
            }
        } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
            if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)) 
                       & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 2U;
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                    = (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                              [0U]);
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                    = (IData)((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                               [0U] >> 0x20U));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 1U;
            }
        } else if (((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                      & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                     & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)) 
                    & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr_nxt = 1U;
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 1U;
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[0U] 
                = (IData)((((QData)((IData)((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__guid_bip2) 
                                              << 0x1eU) 
                                             | (0x7ffU 
                                                & (IData)(
                                                          (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                           [0U] 
                                                           >> 0x20U)))))) 
                            << 0x20U) | (QData)((IData)(
                                                        (0xa0aU 
                                                         | ((IData)(
                                                                    (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                     [0U] 
                                                                     >> 0x10U)) 
                                                            << 0x10U))))));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[1U] 
                = (IData)(((((QData)((IData)((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__guid_bip2) 
                                               << 0x1eU) 
                                              | (0x7ffU 
                                                 & (IData)(
                                                           (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                            [0U] 
                                                            >> 0x20U)))))) 
                             << 0x20U) | (QData)((IData)(
                                                         (0xa0aU 
                                                          | ((IData)(
                                                                     (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_reg
                                                                      [0U] 
                                                                      >> 0x10U)) 
                                                             << 0x10U))))) 
                           >> 0x20U));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                = (0x80000U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]);
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                = (1U | (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                = (0x10000000U | (0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                = (0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]);
        } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                    & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)))) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0x1aU;
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U] 
                = (0x10000000U | (0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[2U]));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U] 
                = (0x200U & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_out_nxt[3U]);
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st_nxt = 0U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                      >> 3U)))) {
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                          >> 2U)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                              >> 1U)))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                        if (((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                               & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                              & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)) 
                             & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 0U;
                        } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                                    & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)))) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__user_vm_nxt = 0U;
                        }
                    }
                }
            }
            if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st))) {
                    if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r)))) {
                            if ((0xfU != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                if ((0x10U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                    if ((0x11U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                        if ((0x16U 
                                             != (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            if ((0x12U 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 1U;
                                            }
                                        }
                                        if ((0x16U 
                                             == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__adj_frmd_out_type))) {
                                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 1U;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st) 
                                 >> 1U)))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_st)))) {
                    if (((((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                           & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))) 
                          & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)) 
                         & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_val)) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 0U;
                    } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__start_new_cmd) 
                                & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__gen_guid)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_nxt = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__frmd_vm_short_nxt = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_tlv_parse_action_0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_tlv_parse_action_1 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_debug_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_data_fifo_debug_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_pdt_fifo_debug_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_data_fifo_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_load_osf_pdt_fifo_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state)) 
           & (0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr))) 
           & ((4U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
              | (((0xcU <= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
                  & (0x50U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr))) 
                 | (((0x58U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
                     & (0x80U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr))) 
                    | ((0x88U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
                       & (0xacU >= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)))))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__wr_stb) 
         & (0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)))) {
        if ((0x1000U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
            if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                              >> 0xaU)))) {
                    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                          >> 0xbU)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                              >> 0xaU)))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                              >> 9U)))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                      >> 0xcU)))) {
            if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                              >> 0xaU)))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                          >> 0xbU)))) {
                if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                              >> 0xaU)))) {
                    if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config) 
                                  >> 9U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__wr_stb) 
         & (0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)))) {
        if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
            if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                              >> 9U)))) {
                    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                          >> 0xaU)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                              >> 9U)))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                              >> 8U)))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                      >> 0xbU)))) {
            if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                              >> 9U)))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                          >> 0xaU)))) {
                if ((0x200U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                              >> 9U)))) {
                    if ((0x100U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config) 
                                  >> 8U)))) {
                        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__selected 
        = ((0x90U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
           & (0x94U > (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__selected 
        = ((0xa0U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)) 
           & (0xa4U > (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)));
    __Vtableidx1 = (7U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config) 
                          >> 3U));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__use_wmark 
        = Vcr_cddip__ConstPool__TABLE_h6faa2b41_0[__Vtableidx1];
    __Vtableidx2 = (7U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__req_wmark 
        = Vcr_cddip__ConstPool__TABLE_h6faa2b41_0[__Vtableidx2];
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_monitor 
        = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_unanswered_read_din) 
            << 6U) | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_rcv_invalid_opcode_din) 
                       << 5U) | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_bimc_chain_rcv_error_din) 
                                  << 4U) | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_parity_error_din) 
                                             << 2U) 
                                            | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_correctable_ecc_error_din) 
                                                << 1U) 
                                               | (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_monitor_uncorrectable_ecc_error_din))))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_frm));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_true 
        = ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__pti_insert_pfd_ack)) 
           & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_pfd_req) 
              | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_phd_req)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_COUNTR__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0x3fU)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_SNAPSHOTR__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0x3fU)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__SA_CTRL__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0x3fU)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_cr_osf_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0x1ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_data_fifo_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_data_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0x1ffU)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0xffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_osf_pdt_fifo_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_dbg_pdt_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0xffU)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__wr_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__wr_stb) 
           & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__selected));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__rd_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_data_bytes_global_config)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__rd_stb) 
               & (0x98U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)))
            : ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__rd_stb) 
               & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_data_bytes_cntr__DOT__selected)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__wr_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__wr_stb) 
           & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__selected));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__rd_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__ob_agg_frame_global_config)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__rd_stb) 
               & (0xa8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__reg_addr)))
            : ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__rd_stb) 
               & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT__u_ob_agg_frame_cntr__DOT__selected)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_frm 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_frm;
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[0U] 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[0U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[1U] 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[1U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[2U] 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_dat[2U];
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_pfd_crc_addr_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_pfd_crc_addr));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pmc__DOT__pmc_ld_phd_crc_addr_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_ld_phd_crc_addr));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_write) 
           & (2U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_cddip_int_control_data 
        = ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr))
            ? vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg
            : ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr))
                ? (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_raw_reg 
                   & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_mask_reg)
                : vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__int_mask_reg));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_ack) 
           | (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_tlv_parse_action_0 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_tlv_parse_action_1 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_debug_ctl_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_debug_trig_tlv_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_trace_ctl_limits_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_load_isf_fifo_ia_config 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state)) 
           & (0xacU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 0U;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_valid_rd_addr 
        = ((0U == (3U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
           & ((4U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
              | (((0xcU <= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
                  & (0xb8U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                 | ((0xc0U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
                    & (0xd4U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__wr_stb) 
         & (0xacU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))) {
        if ((0x2000U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
            if ((0x1000U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                              >> 0xbU)))) {
                    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
                }
                if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__ack_error = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_tmo_stb = 1U;
                    }
                }
            } else if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__unsupported_op = 1U;
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                          >> 0xcU)))) {
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                              >> 0xbU)))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_cmp_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_sis_stb = 1U;
                    }
                }
            }
        }
        if ((0xeU != (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                              >> 0xaU)))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 1U;
        }
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                      >> 0xdU)))) {
            if ((0x1000U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_inc_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ini_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                              >> 0xbU)))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rst_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_dis_stb = 1U;
                    }
                }
            }
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                          >> 0xcU)))) {
                if ((0x800U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_ena_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_wr_stb = 1U;
                    }
                }
                if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                              >> 0xbU)))) {
                    if ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_rd_stb = 1U;
                    }
                    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config) 
                                  >> 0xaU)))) {
                        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued = 0U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__selected 
        = ((0xc8U <= (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
           & (0xccU > (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data = 0U;
    if (((((((((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
               | (4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
              | (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
             | (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
            | (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
           | (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
          | (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
         | (0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))) {
        if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data);
        } else if ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_spare_config;
        } else if ((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_0;
        } else if ((0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_tlv_parse_action_1;
        } else if ((0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xfffffeffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x100U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config) 
                                << 2U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xffff0fffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0xf000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_ctl_config) 
                                 << 5U)));
        } else if ((0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_system_stall_limit_config;
        } else if ((0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xffffc000U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x3fffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xf800ffffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x7ff0000U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config 
                                    << 2U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xefffffffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x10000000U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config 
                                     << 3U)));
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0x7fffffffU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (0x80000000U & (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_ctl_config 
                                     << 5U)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__o_debug_ss_ctl_config));
        }
    } else if (((((((((0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
                      | (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                     | (0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                    | (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                   | (0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                  | (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                 | (0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                | (0x40U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
            = ((0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                ? ((0xfffffffcU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | (((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__ss_rd_ok) 
                       << 1U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__trigger_hit)))
                : ((0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                    ? ((0xe0000000U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                       | vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_tlv_config)
                    : ((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                        ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_match_lo_config
                        : ((0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_match_hi_config
                            : ((0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_mask_lo_config
                                : ((0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                    ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_debug_trig_mask_hi_config
                                    : ((0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                        ? (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trig_word_hld)
                                        : (IData)((vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_support__DOT__trig_word_hld 
                                                   >> 0x20U)))))))));
    } else if (((((((((0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)) 
                      | (0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                     | (0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                    | (0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                   | (0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                  | (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                 | (0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) 
                | (0x60U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
            = ((0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                ? ((0xf0000000U & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                   | vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_sb)
                : ((0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                    ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_lo
                    : ((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                        ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__debug_ss_cap_hi
                        : ((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                            ? ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data) 
                               | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_trace_ctl_en_config))
                            : ((0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_trace_ctl_limits_config
                                : ((0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                    ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_0_comp_config
                                    : ((0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))
                                        ? vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_0_crypto_config
                                        : vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_0_comp_config)))))));
    } else if ((0x64U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_mask_val_0_crypto_config;
    } else if ((0x68U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_aux_cmd_ev_match_val_1_comp_config;
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_cmd2 
        = ((0x400U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2)) 
           | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_cmd2_sent) 
               << 9U) | (0x1ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_cmd2))));
    vlSelf->__VdfgTmp_hc036dc68__0 = ((0xf000000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
                                      | ((0xcU & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                                  >> 0xeU)) 
                                         | (3U & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                                  >> 0x14U))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__i_bimc_eccpar_debug 
        = ((0x1f000000U & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug) 
           | (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__bimc_eccpar_debug_sent) 
               << 0x17U) | (0x7fffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug)));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_rstate 
        = ((8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
            ? ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                ? 2U : ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                         ? 2U : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                                  ? 0xaU : ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                              & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp)) 
                                             & ((2U 
                                                 == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                | (1U 
                                                   == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                             ? 9U : 
                                            ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                               & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp))) 
                                              & ((2U 
                                                  == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                 | (1U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                              ? 0xaU
                                              : (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm)) 
                                                  & ((2U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                     | (1U 
                                                        == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                                  ? 0xaU
                                                  : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate)))))))
            : ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                ? ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                    ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                        ? ((3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))
                            ? 8U : 0U) : 2U) : ((1U 
                                                 & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                                                 ? 6U
                                                 : 
                                                ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                                   & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp)) 
                                                  & ((2U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                     | (1U 
                                                        == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                                  ? 5U
                                                  : 
                                                 ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                                    & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp))) 
                                                   & ((2U 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                      | (1U 
                                                         == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                                   ? 6U
                                                   : 
                                                  (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm)) 
                                                    & ((2U 
                                                        == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk)) 
                                                       | (1U 
                                                          == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_chk))))
                                                    ? 6U
                                                    : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))))))
                : ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                    ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                        ? ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))
                            ? 4U : ((3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op))
                                     ? 4U : 0U)) : 
                       ((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                          & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp))) 
                         & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame))
                         ? 1U : (((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                    & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp)) 
                                   & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame)) 
                                  & (3U != (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op)))
                                  ? 3U : (((((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm) 
                                             & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_resp)) 
                                            & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame)) 
                                           & (3U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_op)))
                                           ? 7U : (
                                                   ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rx_frm)) 
                                                    & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__new_frame))
                                                    ? 2U
                                                    : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))))))
                    : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__rstate))
                        ? 0U : 2U))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr1tc_valid 
        = ((- (IData)(((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__u_cddip_support_interrupt__DOT__wr_stb_valid) 
                       & (0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_addr))))) 
           & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_cddip_int_control_data);
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__locl_err_ack 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_err_ack) 
           | ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__w_valid_rd_addr)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_cr_isf_regs__DOT__f_state))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__badaddr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__cmnd_issued) 
           & ((0x3ffU & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT____Vcellout__u_cr_isf_regs__o_isf_fifo_ia_config)) 
              > ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_dbg_fifo__DOT__u_nx_fifo_1r1w_indirect_access_debug_cntrl__DOT__u_cntrl__DOT__init_r)
                  ? 0U : 0x3ffU)));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__wr_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__wr_stb) 
           & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__selected));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__rd_stb_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__ib_agg_data_bytes_global_config)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__rd_stb) 
               & (0xd0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__reg_addr)))
            : ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__rd_stb) 
               & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_regfile__DOT__u_ib_agg_data_bytes_cntr__DOT__selected)));
    __Vtemp_120[1U] = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                        ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                            ? ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))
                                ? vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U]
                                : 0x3fffffffU) : vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U])
                        : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                            ? ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))
                                ? vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U]
                                : ((0xfU == (0xfU & 
                                             (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                              >> 0xcU)))
                                    ? (0x20000000U 
                                       | (IData)(((
                                                   ((QData)((IData)(
                                                                    (0xfffU 
                                                                     & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug))) 
                                                    << 0x20U) 
                                                   | (QData)((IData)(vlSelf->__VdfgTmp_hc036dc68__0))) 
                                                  >> 0x20U)))
                                    : (0x20000000U 
                                       | (IData)(((
                                                   ((QData)((IData)(
                                                                    (0xa0U 
                                                                     | (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                                                        << 0x14U)))) 
                                                    << 0x1cU) 
                                                   | (QData)((IData)(vlSelf->__VdfgTmp_hc036dc68__0))) 
                                                  >> 0x20U)))))
                            : vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U]));
    if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
        if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
                if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
                    __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
                    __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
                } else {
                    __Vtemp_137[0U] = 0U;
                    __Vtemp_137[2U] = 0xf0U;
                }
            } else {
                __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
                __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
            }
        } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
                __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
                __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
            } else if ((0xfU == (0xfU & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                         >> 0xcU)))) {
                __Vtemp_137[0U] = (IData)((((QData)((IData)(
                                                            (0xfffU 
                                                             & vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug))) 
                                            << 0x20U) 
                                           | (QData)((IData)(vlSelf->__VdfgTmp_hc036dc68__0))));
                __Vtemp_137[2U] = (0xf0U & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                            >> 8U));
            } else {
                __Vtemp_137[0U] = (IData)((((QData)((IData)(
                                                            (0xa0U 
                                                             | (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                                                << 0x14U)))) 
                                            << 0x1cU) 
                                           | (QData)((IData)(vlSelf->__VdfgTmp_hc036dc68__0))));
                __Vtemp_137[2U] = (0xf0U & (vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__o_bimc_eccpar_debug 
                                            >> 8U));
            }
        } else {
            __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        }
    } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
        if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
            __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else {
            __Vtemp_137[0U] = 0U;
            __Vtemp_137[2U] = 0U;
        }
    } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
        if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
            __Vtemp_137[0U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            __Vtemp_137[2U] = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else {
            __Vtemp_137[0U] = (IData)((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_mem)) 
                                        << 0x30U) | 
                                       (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_addr)) 
                                         << 0x20U) 
                                        | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_dat)))));
            __Vtemp_137[2U] = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_type) 
                                << 4U) | ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_op) 
                                          >> 4U));
        }
    } else {
        __Vtemp_137[0U] = 0U;
        __Vtemp_137[2U] = 0U;
    }
    if ((8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
        if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
                if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate = 3U;
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
                        = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                        = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
                        = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                        = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] = 1U;
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] = 0x2fff0001U;
                    vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] = 0xf0U;
                }
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
            }
        } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate = 3U;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[0U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[2U];
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] = 1U;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] = 0xffff0008U;
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] = 0xffU;
        }
    } else {
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[0U] 
            = __Vtemp_137[0U];
        if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                    ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                        ? ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))
                            ? 3U : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                        : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                    : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                        ? ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))
                            ? 3U : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))
                        : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                = __Vtemp_120[1U];
        } else if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                    = (((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt)) 
                        & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cmd_cnt))
                        ? ((IData)(vlSelf->cr_cddip__DOT__top_bimc_mstr_rst_n)
                            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cpu_transmit_ev)
                                ? 1U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__auto_poll_ecc_par_ev)
                                         ? 7U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__mem_wr_init_ev)
                                                  ? 8U
                                                  : 
                                                 ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__eccpar_debug_ev)
                                                   ? 5U
                                                   : 2U))))
                            : 0U) : 2U);
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            } else if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate = 3U;
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] = 0U;
            }
        } else if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate))) {
            if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt))) {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate = 3U;
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__r_reg_send[1U];
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                    = vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate;
                vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] 
                    = (((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_op) 
                        << 0x1cU) | (IData)(((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_mem)) 
                                               << 0x30U) 
                                              | (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_addr)) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__cputx_dat)))) 
                                             >> 0x20U)));
            }
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__nxt_tstate 
                = (((IData)(vlSelf->cr_cddip__DOT__top_bimc_mstr_rst_n) 
                    & (0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__sync_cnt)))
                    ? 0xbU : (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__tstate));
            vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[1U] = 0U;
        }
        vlSelf->cr_cddip__DOT__u_cr_cddip_support__DOT__u_cr_cddip_support_regfile__DOT__bimc_master__DOT__reg_send[2U] 
            = __Vtemp_137[2U];
    }
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren 
        = (1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)) 
                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))));
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren 
        = (1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)) 
                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren 
        = (1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)) 
                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen)))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                           - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__tgen_user_wr) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                               - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0x1fU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x3fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x3fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ob_wr) {
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_wptr 
                = ((0x1fU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))
                    ? 0U : (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_wptr))));
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_ren) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x3fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x3fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
    }
}

extern const VlUnpacked<CData/*7:0*/, 256> Vcr_cddip__ConstPool__TABLE_hc2752745_0;

VL_INLINE_OPT void Vcr_cddip___024root___nba_sequent__TOP__4(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___nba_sequent__TOP__4\n"); );
    // Init
    CData/*7:0*/ __Vtableidx5;
    __Vtableidx5 = 0;
    IData/*31:0*/ __Vilp;
    CData/*7:0*/ __Vtableidx13;
    __Vtableidx13 = 0;
    CData/*7:0*/ __Vtableidx15;
    __Vtableidx15 = 0;
    // Body
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr;
    if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
               & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__afull_r))))) {
        if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 1U;
        }
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0U;
    }
    if (((vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
          >> 0x12U) & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
        if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty) {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 1U;
        }
        vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0U;
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_spl__DOT__tlvp_spl_id_out_action 
        = (3U & (((((((((0U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                         >> 0x14U))) 
                        | (1U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                           >> 0x14U)))) 
                       | (2U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                          >> 0x14U)))) 
                      | (3U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                         >> 0x14U)))) 
                     | (4U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                        >> 0x14U)))) 
                    | (5U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                       >> 0x14U)))) 
                   | (6U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                      >> 0x14U)))) 
                  | (7U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                     >> 0x14U)))) ? 
                 ((0U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                   >> 0x14U))) ? (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action)
                   : ((1U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                       >> 0x14U))) ? (IData)(
                                                             (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                              >> 2U))
                       : ((2U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                           >> 0x14U)))
                           ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                      >> 4U)) : ((3U 
                                                  == 
                                                  (0xffU 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                                  ? (IData)(
                                                            (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                             >> 6U))
                                                  : 
                                                 ((4U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? (IData)(
                                                             (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                              >> 8U))
                                                   : 
                                                  ((5U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? (IData)(
                                                              (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                               >> 0xaU))
                                                    : 
                                                   ((6U 
                                                     == 
                                                     (0xffU 
                                                      & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? (IData)(
                                                               (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                >> 0xcU))
                                                     : (IData)(
                                                               (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                >> 0xeU)))))))))
                  : (((((((((8U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U))) 
                            | (9U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                           | (0xaU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                          | (0xbU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                               >> 0x14U)))) 
                         | (0xcU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                              >> 0x14U)))) 
                        | (0xdU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                       | (0xeU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                      | (0xfU == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                           >> 0x14U))))
                      ? ((8U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                          >> 0x14U)))
                          ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                     >> 0x10U)) : (
                                                   (9U 
                                                    == 
                                                    (0xffU 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? (IData)(
                                                              (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                               >> 0x12U))
                                                    : 
                                                   ((0xaU 
                                                     == 
                                                     (0xffU 
                                                      & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                         >> 0x14U)))
                                                     ? (IData)(
                                                               (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                >> 0x14U))
                                                     : 
                                                    ((0xbU 
                                                      == 
                                                      (0xffU 
                                                       & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                          >> 0x14U)))
                                                      ? (IData)(
                                                                (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                 >> 0x16U))
                                                      : 
                                                     ((0xcU 
                                                       == 
                                                       (0xffU 
                                                        & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                           >> 0x14U)))
                                                       ? (IData)(
                                                                 (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                  >> 0x18U))
                                                       : 
                                                      ((0xdU 
                                                        == 
                                                        (0xffU 
                                                         & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                            >> 0x14U)))
                                                        ? (IData)(
                                                                  (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                   >> 0x1aU))
                                                        : 
                                                       ((0xeU 
                                                         == 
                                                         (0xffU 
                                                          & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                             >> 0x14U)))
                                                         ? (IData)(
                                                                   (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                    >> 0x1cU))
                                                         : (IData)(
                                                                   (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                                    >> 0x1eU)))))))))
                      : (((((((((0x10U == (0xffU & 
                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U))) 
                                | (0x11U == (0xffU 
                                             & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x12U == (0xffU & 
                                            (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x13U == (0xffU & 
                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U)))) 
                             | (0x14U == (0xffU & (
                                                   vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                            | (0x15U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                           | (0x16U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                          | (0x17U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                >> 0x14U))))
                          ? ((0x10U == (0xffU & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                 >> 0x14U)))
                              ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                         >> 0x20U))
                              : ((0x11U == (0xffU & 
                                            (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                             >> 0x22U))
                                  : ((0x12U == (0xffU 
                                                & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                   >> 0x14U)))
                                      ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                 >> 0x24U))
                                      : ((0x13U == 
                                          (0xffU & 
                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U)))
                                          ? (IData)(
                                                    (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                     >> 0x26U))
                                          : ((0x14U 
                                              == (0xffU 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                              ? (IData)(
                                                        (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                         >> 0x28U))
                                              : ((0x15U 
                                                  == 
                                                  (0xffU 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                                  ? (IData)(
                                                            (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                             >> 0x2aU))
                                                  : 
                                                 ((0x16U 
                                                   == 
                                                   (0xffU 
                                                    & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? (IData)(
                                                             (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                              >> 0x2cU))
                                                   : (IData)(
                                                             (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                              >> 0x2eU)))))))))
                          : (((((((((0x18U == (0xffU 
                                               & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                  >> 0x14U))) 
                                    | (0x19U == (0xffU 
                                                 & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                    >> 0x14U)))) 
                                   | (0x1aU == (0xffU 
                                                & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                   >> 0x14U)))) 
                                  | (0x1bU == (0xffU 
                                               & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                  >> 0x14U)))) 
                                 | (0x1cU == (0xffU 
                                              & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                 >> 0x14U)))) 
                                | (0x1dU == (0xffU 
                                             & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                >> 0x14U)))) 
                               | (0x1eU == (0xffU & 
                                            (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U)))) 
                              | (0x1fU == (0xffU & 
                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U))))
                              ? ((0x18U == (0xffU & 
                                            (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                             >> 0x14U)))
                                  ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                             >> 0x30U))
                                  : ((0x19U == (0xffU 
                                                & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                   >> 0x14U)))
                                      ? (IData)((vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                 >> 0x32U))
                                      : ((0x1aU == 
                                          (0xffU & 
                                           (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                            >> 0x14U)))
                                          ? (IData)(
                                                    (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                     >> 0x34U))
                                          : ((0x1bU 
                                              == (0xffU 
                                                  & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                     >> 0x14U)))
                                              ? (IData)(
                                                        (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                         >> 0x36U))
                                              : ((0x1cU 
                                                  == 
                                                  (0xffU 
                                                   & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                      >> 0x14U)))
                                                  ? (IData)(
                                                            (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                             >> 0x38U))
                                                  : 
                                                 ((0x1dU 
                                                   == 
                                                   (0xffU 
                                                    & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                       >> 0x14U)))
                                                   ? (IData)(
                                                             (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                              >> 0x3aU))
                                                   : 
                                                  ((0x1eU 
                                                    == 
                                                    (0xffU 
                                                     & (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_id_out[2U] 
                                                        >> 0x14U)))
                                                    ? (IData)(
                                                              (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                               >> 0x3cU))
                                                    : (IData)(
                                                              (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT____Vcellinp__u_cr_osf_tlvp__tlv_parse_action 
                                                               >> 0x3eU)))))))))
                              : 1U)))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_ib_rd) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                           - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_tlvp_id.tlvp_ib_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                               - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_df_rd_sel = 0U;
    if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_st))) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_df_rd_sel 
            = (1U & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)));
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_ld = 0U;
    if ((8U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st))) {
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st) 
                      >> 2U)))) {
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st) 
                          >> 1U)))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st)))) {
                    if ((1U & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cqe_w1_ld = 1U;
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld = 0U;
    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st) 
                  >> 3U)))) {
        if ((4U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st))) {
            if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st))) {
                if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_user_xfr_st)))) {
                    if ((1U & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____Vlvbound_h647a6d3a__0 = 1U;
                        if ((0x13U >= (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ix))) {
                            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld 
                                = (((~ ((IData)(1U) 
                                        << (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ix))) 
                                    & vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld) 
                                   | (0xfffffU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT____Vlvbound_h647a6d3a__0) 
                                                  << (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__ix))));
                        }
                    }
                }
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld 
                    = (1U | vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__cg_ftr_ld);
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__pt_ob_rd 
        = (1U & ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob__DOT__afull_r)) 
                    & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__usr_ib_rd 
        = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                    | ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r) 
                       | ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_phd_crc_wait) 
                          | ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_pfd_crc_wait) 
                             | ((((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1_valid) 
                                  & (IData)(((0x500000U 
                                              == (0xff00000U 
                                                  & vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_tlv1[2U])) 
                                             & (1U 
                                                == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_word_num))))) 
                                 | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_pac__DOT__pac_dunk_w1_valid)) 
                                | ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__ibp_insert_true) 
                                   | (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_prefix_attach_ibp__DOT__ibp_dunkw1_stall)))))))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__xfr_rdy 
        = (1U & ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__u_cr_isf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__0__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[1U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[1U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__1__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[2U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[2U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__2__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[3U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[3U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__3__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[4U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[4U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__4__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[5U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[5U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__5__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[6U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[6U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__6__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[7U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[7U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__7__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[8U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[8U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__8__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[9U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[9U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__9__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xaU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xaU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__10__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xbU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xbU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__11__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xcU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xcU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__12__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xdU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xdU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__13__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xeU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xeU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__14__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0xfU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0xfU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__15__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x10U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x10U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__16__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x11U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x11U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__17__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x12U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x12U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__18__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x13U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x13U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__19__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x14U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x14U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__20__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x15U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x15U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__21__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x16U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x16U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__22__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x17U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x17U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__23__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x18U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x18U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__24__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x19U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x19U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__25__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__26__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__27__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__28__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__29__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__30__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x1fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x1fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__31__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x20U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x20U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__32__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x21U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x21U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__33__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x22U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x22U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__34__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x23U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x23U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__35__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x24U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x24U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__36__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x25U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x25U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__37__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x26U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x26U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__38__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x27U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x27U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__39__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x28U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x28U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__40__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x29U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x29U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__41__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__42__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__43__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__44__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__45__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__46__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x2fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x2fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__47__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x30U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x30U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__48__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x31U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x31U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__49__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x32U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x32U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__50__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x33U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x33U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__51__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x34U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x34U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__52__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x35U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x35U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__53__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x36U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x36U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__54__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x37U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x37U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__55__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x38U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x38U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__56__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x39U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x39U] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__57__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3aU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__58__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3bU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__59__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3cU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__60__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3dU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__61__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3eU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__62__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    __Vilp = 0U;
    while ((__Vilp <= 0x3fU)) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__regs_sa_ctrl[__Vilp] 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT____Vcellout__u_cr_cddip_sa_regfile__regs_sa_ctrl
            [__Vilp];
        __Vilp = ((IData)(1U) + __Vilp);
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count[0x3fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i.sa_count;
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot[0x3fU] 
        = vlSymsp->TOP__cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_counters__BRA__63__KET____DOT__u_cr_sa_counter_i.sa_snapshot;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_empty_mod 
        = (1U & (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_regfile__DOT____Vcellout__u_cr_osf_regs__o_debug_ctl_config) 
                  >> 2U) | ((IData)(vlSelf->cr_cddip__DOT__sup_osf_halt) 
                            | (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__axi4s_slv_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(((vlSelf->cr_cddip__DOT____Vcellout__u_cr_cg__cg_ob_out[2U] 
                        >> 0x12U) & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1.__PVT__afull_r)))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_pdt_fifo_wr 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_pdt_fifo.__PVT__afull_r)) 
               & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_pdt_fifo_hw_rd)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_wr 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo.__PVT__afull_r)) 
               & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__osf_dbg_data_fifo_hw_rd)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_data_fifo_rd 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_ctl__DOT__ob_df_rd_sel) 
           & ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_data_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
              & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_ob_fifo.__PVT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__pre_tlvp_fifo_wr 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__isf_fifo_hw_rd) 
               & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_isf_pre_tlvp_fifo.__PVT__afull_r))));
    vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_cmd_cnt_stb_nxt = 0U;
    if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st) 
                  >> 4U)))) {
        if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st) 
                      >> 3U)))) {
            if ((1U & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st) 
                          >> 2U)))) {
                if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st))) {
                    if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__isf_user_xfr_st))) {
                        if (vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__xfr_rdy) {
                            vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ib_cmd_cnt_stb_nxt 
                                = ((2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__ix)) 
                                   & (IData)(vlSelf->cr_cddip__DOT__u_cr_isf__DOT__u_cr_isf_core__DOT__u_cr_isf_tlv_mods__DOT__cmd_stats_en));
                        }
                    }
                }
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_clear 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & (vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                     & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_clear_live_r)))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__sa_snap 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & ((vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                      >> 1U) & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_snap_r)))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[1U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[2U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[3U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[4U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[5U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x3aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[6U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x39U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[7U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x38U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[8U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x37U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[9U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x36U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xaU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x35U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xbU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x34U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xcU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x33U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xdU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x32U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xeU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x31U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0xfU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x30U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x10U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x11U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x12U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x13U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x14U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x15U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x2aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x16U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x29U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x17U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x28U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x18U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x27U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x19U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x26U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x25U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x24U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x23U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x22U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x21U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x1fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x20U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x20U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x21U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x22U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x23U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x24U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x25U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x1aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x26U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x19U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x27U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x18U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x28U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x17U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x29U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x16U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x15U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x14U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x13U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x12U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x11U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x2fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0x10U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x30U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xfU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x31U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xeU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x32U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xdU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x33U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xcU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x34U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xbU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x35U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0xaU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x36U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [9U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x37U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [8U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x38U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [7U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x39U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [6U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [5U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [4U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [3U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [2U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [1U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_COUNTR__mem_a[0x3fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_count
        [0U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[1U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[2U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[3U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[4U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[5U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x3aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[6U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x39U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[7U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x38U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[8U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x37U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[9U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x36U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xaU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x35U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xbU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x34U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xcU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x33U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xdU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x32U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xeU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x31U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0xfU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x30U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x10U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x11U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x12U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x13U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x14U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x15U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x2aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x16U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x29U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x17U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x28U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x18U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x27U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x19U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x26U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x25U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x24U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x23U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x22U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x21U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x1fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x20U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x20U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1fU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x21U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1eU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x22U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1dU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x23U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1cU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x24U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1bU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x25U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x1aU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x26U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x19U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x27U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x18U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x28U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x17U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x29U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x16U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x15U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x14U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x13U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x12U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x11U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x2fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0x10U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x30U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xfU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x31U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xeU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x32U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xdU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x33U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xcU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x34U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xbU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x35U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0xaU];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x36U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [9U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x37U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [8U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x38U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [7U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x39U] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [6U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3aU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [5U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3bU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [4U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3cU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [3U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3dU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [2U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3eU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [1U];
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__SA_SNAPSHOTR__mem_a[0x3fU] 
        = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__sa_snapshot
        [0U];
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd 
        = (1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__ob_fifo_empty_mod)) 
                 & ((~ (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__axi4s_ib_out[2U] 
                        >> 0x12U)) | (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_osf_axi4s_mstr__DOT__axi4s_ib_in))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && ((0U != vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
               && ((1U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state)
                    ? (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)
                    : ((2U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state) 
                       && (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid)))));
    if (vlSelf->cr_cddip__DOT__rst_sync_n) {
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
             & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                >> 0x13U))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0U;
        } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
                    & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num) 
                          >> 0xdU)))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num 
                = (0x3fffU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid) {
            if ((0x8000000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x4000000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x2000000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x1000000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x800000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                } else if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                            >> 0x14U));
                        }
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    }
                } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                            = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x13U)));
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                            = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                     >> 0x13U));
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                            >> 0x10U));
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                            >> 8U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data 
                            = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                << 0x20U) | (QData)((IData)(
                                                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x14U));
                    }
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type 
                        = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                    >> 0x14U));
                }
            } else if ((0x800000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type 
                        = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                    >> 0x14U));
                } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x14U));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    }
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                }
            } else if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                            = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x13U)));
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                            = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                     >> 0x13U));
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                            >> 0x10U));
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                            >> 8U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data 
                            = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                << 0x20U) | (QData)((IData)(
                                                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 1U;
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                        }
                        if (((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U];
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U];
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U];
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[3U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U];
                        if (((0xbU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err 
                                = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__chk_xp10crc64_enc) 
                                   & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_match)));
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good 
                                = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__chk_xp10crc64_enc) 
                                   & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_match));
                        }
                        if (((IData)(((0U == (0xff000U 
                                              & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])) 
                                      & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                         >> 0x12U))) 
                             & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err))) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                                = (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err);
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                                = (IData)((vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
                                           >> 0x20U));
                        }
                        if (((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
                        }
                        if ((0x40000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
                        }
                    }
                } else if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                        = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                    >> 0x13U)));
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                        = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                 >> 0x13U));
                    if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                        >> 0x10U));
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                            = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 8U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data 
                        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                            << 0x20U) | (QData)((IData)(
                                                        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                }
            } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num))) {
                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on 
                        = (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                           >> 0x1fU);
                    if ((IData)((0x81000000U == (0x9f800000U 
                                                 & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U];
                    }
                    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
                    }
                }
            } else {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            }
            if ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data) 
                  & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num) 
                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num))) 
                 & ((0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type)))) {
                vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data 
                    = ((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                         << 0x20U) | (QData)((IData)(
                                                     vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U]))) 
                       ^ vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk);
                if ((0U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
                }
            }
        } else {
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
        }
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd;
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
             & (vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U] 
                >> 0x13U))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0U;
        } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) 
                    & (~ ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num) 
                          >> 0xdU)))) {
            vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num 
                = (0x3fffU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid) {
            if ((0x8000000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x4000000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x2000000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x1000000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x800000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                } else if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        }
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    }
                } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                            = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x13U)));
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                            = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                     >> 0x13U));
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                            >> 0x10U));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                            >> 8U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data 
                            = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                << 0x20U) | (QData)((IData)(
                                                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    }
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                }
            } else if ((0x800000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    }
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                }
            } else if ((0x400000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                            = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 0x13U)));
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                            = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                     >> 0x13U));
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                            >> 0x10U));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        } else {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                                = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                            >> 8U));
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data 
                            = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                << 0x20U) | (QData)((IData)(
                                                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 1U;
                        if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number 
                                = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                        }
                        if (((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
                        }
                        if (((0xbU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0U;
                        }
                        if (((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                             & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl)) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
                        }
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U];
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U];
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U];
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[3U] 
                            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U];
                        if ((((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                              & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                                 >> 1U)) & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_xp10crc64_raw))) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                                = (IData)((~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                                = (IData)(((~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r) 
                                           >> 0x20U));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result 
                                = (~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r);
                        } else if ((((5U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                                     & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                                        >> 1U)) & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc64e))) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                                = (IData)((~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                                = (IData)(((~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r) 
                                           >> 0x20U));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result 
                                = (~ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r);
                        } else if ((((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num)) 
                                     & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                                        >> 1U)) & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc16t))) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                                = (IData)((((QData)((IData)(VL_STREAML_FAST_III(16, (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r), 0))) 
                                            << 0x30U) 
                                           | (0xffffffffffffULL 
                                              & (((QData)((IData)(
                                                                  vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                                  << 0x20U) 
                                                 | (QData)((IData)(
                                                                   vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U]))))));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                                = (IData)(((((QData)((IData)(VL_STREAML_FAST_III(16, (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r), 0))) 
                                             << 0x30U) 
                                            | (0xffffffffffffULL 
                                               & (((QData)((IData)(
                                                                   vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                                                   << 0x20U) 
                                                  | (QData)((IData)(
                                                                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U]))))) 
                                           >> 0x20U));
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result 
                                = (QData)((IData)(VL_STREAML_FAST_III(16, (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r), 0)));
                        } else if (((IData)(((0U == 
                                              (0xff000U 
                                               & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])) 
                                             & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                                >> 0x12U))) 
                                    & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err))) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] 
                                = (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err);
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] 
                                = (IData)((vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
                                           >> 0x20U));
                        }
                        if ((0x40000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
                            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
                        }
                    }
                } else if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid 
                        = (1U & (~ (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                    >> 0x13U)));
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc_init 
                        = (1U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                 >> 0x13U));
                    if ((0x80000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                        >> 0x10U));
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num 
                            = (0x7ffU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U]);
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    } else {
                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes 
                            = (0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                                        >> 8U));
                    }
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data 
                        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                            << 0x20U) | (QData)((IData)(
                                                        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
                } else {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                }
            } else if ((0x200000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            } else if ((0x100000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U])) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
                if ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num))) {
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_md_type 
                        = (3U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                 >> 0x13U));
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type 
                        = (0x7fU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                                    >> 0x17U));
                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on 
                        = (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                           >> 0x1fU);
                    if ((IData)((0x86000000U == (0x9f800000U 
                                                 & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U];
                    }
                    if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr) 
                         & ((0U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                                          >> 0x1dU))) 
                            | (1U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                                            >> 0x1dU)))))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 1U;
                    } else if (((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr) 
                                & (0x40000000U == (0x60000000U 
                                                   & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])))) {
                        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
                    }
                }
            } else {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
            }
            if ((((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data) 
                  & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num) 
                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num))) 
                 & ((0xffU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                              >> 0x14U)) == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type)))) {
                vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data 
                    = ((((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                         << 0x20U) | (QData)((IData)(
                                                     vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U]))) 
                       ^ vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk);
                if ((0U == (3U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                                  >> 0x1dU)))) {
                    vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
                }
            }
        } else {
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
        }
    } else {
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data = 0ULL;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_crc_init = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[3U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
            = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data = 0ULL;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_valid = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_crc_init = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_md_type = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type = 0U;
        vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc_result = 0ULL;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_seq_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_num = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[0U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[1U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[3U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_good = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_trace_on = 0U;
    }
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_word_num;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__crc_r;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__crc_r;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_corrupt_data;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
        = vlSelf->__Vdly__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_out_sel = 0U;
    if ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__osf_lat_ctl_st))) {
        if ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__osf_lat_ctl_st))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_out_sel = 1U;
            if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__axi_mstr_rd)))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_latency__DOT__lat_out_sel = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__chk_xp10crc64_enc 
        = ((0xfU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type)) 
           | ((0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type)) 
              | ((0x11U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type)) 
                 | ((0x12U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type)) 
                    | (0x16U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frmd_type))))));
    __Vtableidx5 = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes;
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__data_vbits 
        = Vcr_cddip__ConstPool__TABLE_hc2752745_0[__Vtableidx5];
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err) 
           | ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err) 
              | ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err) 
                 | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err))));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num 
        = (0x3ffU & VL_SHIFTR_III(10,10,32, (0x3ffU 
                                             & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                                                >> 8U)), 3U));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type 
        = (0x1fU & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                    >> 0x12U));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk 
        = ((QData)((IData)((0xffU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd))) 
           << (0x38U & (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                        >> 5U)));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_wr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr) 
           & (IData)((0x640000U == (0xff40000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U]))));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crcc_err 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_err) 
           | ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_enc_cmp_data_cksum_err) 
              | ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_crc64e_cksum_err) 
                 | (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_nvme_raw_cksum_err))));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debug_word_num 
        = (0x3ffU & VL_SHIFTR_III(10,10,32, (0x3ffU 
                                             & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                                                >> 8U)), 3U));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_type 
        = (0x1fU & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                    >> 0x12U));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_debuq_word_msk 
        = ((QData)((IData)((0xffU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd))) 
           << (0x38U & (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_debug_cmd 
                        >> 5U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc16t 
        = (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_xp10crc64_raw 
        = ((0xeU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type)) 
           | ((0xdU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type)) 
              & (1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_md_type))));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__gen_crc64e 
        = ((0xdU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_frmd_out_type)) 
           & (2U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_cmd_md_type)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & VL_SHIFTL_III(7,32,32, (1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes)), 3U));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 1U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 2U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 3U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 4U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 5U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 6U)), 3U)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits 
        = (0x7fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc16t__DOT__data_vbits) 
                    + VL_SHIFTL_III(7,32,32, (1U & 
                                              ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes) 
                                               >> 7U)), 3U)));
    __Vtableidx13 = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__data_vbits 
        = Vcr_cddip__ConstPool__TABLE_hc2752745_0[__Vtableidx13];
    __Vtableidx15 = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__cts_data_vbytes;
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_crc64e__DOT__data_vbits 
        = Vcr_cddip__ConstPool__TABLE_hc2752745_0[__Vtableidx15];
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_hb_wr 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ob_wr) 
           & (IData)((0x640000U == (0xff40000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT____Vcellout__u_cr_crcgc_cts__usr_ob_tlv[2U]))));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0_valid 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_clear_live_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_core__DOT__regs_sa_snap_r 
        = ((IData)(vlSelf->cr_cddip__DOT__rst_sync_n) 
           && (1U & (vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                     >> 1U)));
    if (vlSelf->cr_cddip__DOT__rst_sync_n) {
        if (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) {
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U];
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[1U];
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U];
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[3U];
        }
        if (vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl) {
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (1U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((0xfffffffdU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (2U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((3U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (0xfffffffcU & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd) {
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[0U];
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[1U];
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[2U];
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U] 
                = vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.usr_ib_tlv[3U];
        }
        if (vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl) {
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (1U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((0xfffffffdU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (2U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl 
                = ((3U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl) 
                   | (0xfffffffcU & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f32_data));
        }
        if (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
        }
        if (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd) 
             & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 1U;
        } else if (((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd)) 
                    & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        }
        if (vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_global_ctrl) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                = ((0xfffffffeU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl) 
                   | (1U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                = ((0xfffffffdU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl) 
                   | (2U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f32_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl 
                = ((3U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl) 
                   | (0xfffffffcU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f32_data));
        }
    } else {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl = 3U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[2U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[3U] = 0U;
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl = 3U;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid = 0U;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid = 0U;
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl = 0U;
    }
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd 
        = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                    | (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state)) 
           & (0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__usr_ib_rd 
        = (1U & (~ ((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top__u_cr_tlvp__DOT__u_cr_tlvp_dsm.__PVT__u_cr_fifo_wrap1_usr_ib__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty) 
                    | (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_tlvp_top.__PVT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_uobf__DOT__afull_r))));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__w_load_regs_crcgc_ctrl 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__f_state)) 
           & (0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr 
        = (IData)((0x1000000U == (0x9f800000U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
            << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = (0x38000ULL | (0xfffffffffff00fffULL & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = ((0xfffffffffffff800ULL & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err) 
           | (IData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number)));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_raw_data_cksum_match 
        = ((~ vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_xp10crc64__DOT__crc_r) 
           == (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
                << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U]))));
    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data = 0U;
    if (((((((((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
               | (4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
              | (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
             | (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
            | (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
           | (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
          | (0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
         | (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data)
                : ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                    ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_spare_config
                    : ((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                        ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_0
                        : ((0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__tlv_parse_action_1
                            : ((0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                               [0U][2U] : ((0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                            ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                           [0U][1U]
                                            : ((0x20U 
                                                == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                               [0U][0U]
                                                : vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                               [1U][2U])))))));
    } else if (((((((((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
                      | (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                     | (0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                    | (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                   | (0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                  | (0x40U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                 | (0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                | (0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
               [1U][1U] : ((0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                           [1U][0U] : ((0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                        ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                       [2U][2U] : (
                                                   (0x38U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                   [2U][1U]
                                                    : 
                                                   ((0x3cU 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                     ? 
                                                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                    [2U][0U]
                                                     : 
                                                    ((0x40U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                      ? 
                                                     vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                     [3U][2U]
                                                      : 
                                                     ((0x44U 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                       ? 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                      [3U][1U]
                                                       : 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                      [3U][0U])))))));
    } else if (((((((((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
                      | (0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                     | (0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                    | (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                   | (0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                  | (0x60U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                 | (0x64U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                | (0x68U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
               [4U][2U] : ((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                           [4U][1U] : ((0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                        ? vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                       [4U][0U] : (
                                                   (0x58U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                   [5U][2U]
                                                    : 
                                                   ((0x5cU 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                     ? 
                                                    vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                    [5U][1U]
                                                     : 
                                                    ((0x60U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                      ? 
                                                     vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                     [5U][0U]
                                                      : 
                                                     ((0x64U 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                       ? 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                      [6U][2U]
                                                       : 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
                                                      [6U][1U])))))));
    } else if ((0x6cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
            [6U][0U];
    } else if ((0x70U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
            [7U][2U];
    } else if ((0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
            [7U][1U];
    } else if ((0x78U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__cts_hb
            [7U][0U];
    } else if ((0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcc0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl;
    }
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_dp_cmd_wr 
        = (IData)((0x6000000U == (0x9f800000U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = (((QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[1U])) 
            << 0x20U) | (QData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_usr_ib_tlv0[0U])));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = (0x37000ULL | (0xfffffffffff00fffULL & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err 
        = ((0xfffffffffffff800ULL & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_ftr_word13err) 
           | (IData)((IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_core__DOT__u_cr_crcgc_cts__DOT__cts_frame_number)));
    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data = 0U;
    if (((((((((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
               | (4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
              | (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
             | (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
            | (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
           | (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
          | (0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
         | (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data)
                : ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                    ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_spare_config
                    : ((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                        ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_0
                        : ((0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__tlv_parse_action_1
                            : ((0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                               [0U][2U] : ((0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                            ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                           [0U][1U]
                                            : ((0x20U 
                                                == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                               [0U][0U]
                                                : vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                               [1U][2U])))))));
    } else if (((((((((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
                      | (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                     | (0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                    | (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                   | (0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                  | (0x40U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                 | (0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                | (0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
               [1U][1U] : ((0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                           [1U][0U] : ((0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                        ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                       [2U][2U] : (
                                                   (0x38U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                   [2U][1U]
                                                    : 
                                                   ((0x3cU 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                     ? 
                                                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                    [2U][0U]
                                                     : 
                                                    ((0x40U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                      ? 
                                                     vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                     [3U][2U]
                                                      : 
                                                     ((0x44U 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                       ? 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                      [3U][1U]
                                                       : 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                      [3U][0U])))))));
    } else if (((((((((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)) 
                      | (0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                     | (0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                    | (0x58U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                   | (0x5cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                  | (0x60U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                 | (0x64U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) 
                | (0x68U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr)))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = ((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
               [4U][2U] : ((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                            ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                           [4U][1U] : ((0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                        ? vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                       [4U][0U] : (
                                                   (0x58U 
                                                    == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                    ? 
                                                   vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                   [5U][2U]
                                                    : 
                                                   ((0x5cU 
                                                     == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                     ? 
                                                    vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                    [5U][1U]
                                                     : 
                                                    ((0x60U 
                                                      == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                      ? 
                                                     vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                     [5U][0U]
                                                      : 
                                                     ((0x64U 
                                                       == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))
                                                       ? 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                      [6U][2U]
                                                       : 
                                                      vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
                                                      [6U][1U])))))));
    } else if ((0x6cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
            [6U][0U];
    } else if ((0x70U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
            [7U][2U];
    } else if ((0x74U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
            [7U][1U];
    } else if ((0x78U == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__cts_hb
            [7U][0U];
    } else if ((0x7cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__o_reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT__u_cr_crcgc_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_crcg0__DOT__u_cr_crcgc_regfile__DOT____Vcellout__u_cr_crcgc_regs__o_regs_crcgc_ctrl;
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector 
        = (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
            << 1U) | (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__w_load_sa_global_ctrl 
        = ((1U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__f_state)) 
           & (8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel 
        = ((2U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
            ? ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))) 
                   && ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next))) 
                       && ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                           || ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                               < (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern))))))
            : ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector)) 
               && ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next) 
                   & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert)))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel 
        = ((1U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector) 
                  >> 1U)) && ((1U & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_selector))
                               ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert) 
                                  || ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                      || ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_next))) 
                                          && ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_tlv_ordern) 
                                              >= (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_tlv_ordern)))))
                               : ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_next) 
                                  | (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_insert))));
    vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data = 0U;
    if (((((((((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)) 
               | (4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
              | (8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
             | (0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
            | (0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
           | (0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
          | (0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
         | (0x1cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)))) {
        if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0xffffff00U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data);
        } else if ((4U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_spare_config;
        } else if ((8U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_global_ctrl;
        } else if ((0xcU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x8027U | (0xffff0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x20000000U | (0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
        } else if ((0x10U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_ctrl_ia_status)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0xff000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_ctrl_ia_status) 
                                     << 0x12U)));
        } else if ((0x14U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_wdata_part0;
        } else if ((0x18U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0xf0000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_ctrl_ia_config) 
                                     << 0x16U)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_CTRL__rd_dat;
        }
    } else if (((((((((0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)) 
                      | (0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                     | (0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                    | (0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                   | (0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                  | (0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                 | (0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) 
                | (0x3cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr)))) {
        if ((0x20U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x8023U | (0xffff0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x20000000U | (0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
        } else if ((0x24U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_snapshot_ia_status)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0xff000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_snapshot_ia_status) 
                                     << 0x12U)));
        } else if ((0x28U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_wdata_part0;
        } else if ((0x2cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_wdata_part1;
        } else if ((0x30U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config)));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
                   | (0xf0000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_snapshot_ia_config) 
                                     << 0x16U)));
        } else if ((0x34U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_SNAPSHOTR__rd_dat);
        } else if ((0x38U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x3ffffU & (IData)((vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_SNAPSHOTR__rd_dat 
                                       >> 0x20U)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x8023U | (0xffff0000U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
            vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
                = (0x20000000U | (0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data));
        }
    } else if ((0x40U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
               | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_count_ia_status)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = ((0xffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
               | (0xff000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellinp__u_cr_cddip_sa_regs__i_sa_count_ia_status) 
                                 << 0x12U)));
    } else if ((0x44U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_wdata_part0;
    } else if ((0x48U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_wdata_part1;
    } else if ((0x4cU == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = ((0xffffffc0U & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
               | (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config)));
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = ((0xfffffffU & vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data) 
               | (0xf0000000U & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__u_cr_cddip_sa_regs__o_sa_count_ia_config) 
                                 << 0x16U)));
    } else if ((0x50U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_COUNTR__rd_dat);
    } else if ((0x54U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__reg_addr))) {
        vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT__u_cr_cddip_sa_regs__DOT__r32_mux_0_data 
            = (0x3ffffU & (IData)((vlSelf->cr_cddip__DOT__u_cr_cddip_sa__DOT__u_cr_cddip_sa_regfile__DOT____Vcellout__SA_COUNTR__rd_dat 
                                   >> 0x20U)));
    }
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state 
        = ((0U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
            ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                ? 2U : ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                         ? 1U : 0U)) : ((1U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                         ? ((0x40000U 
                                             & vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                             ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                 ? 2U
                                                 : 
                                                ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                  ? 1U
                                                  : 0U))
                                             : 1U) : 
                                        ((2U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__current_state)
                                          ? ((0x40000U 
                                              & vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_ob_datain[2U])
                                              ? ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_sel)
                                                  ? 2U
                                                  : 
                                                 ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_sel)
                                                   ? 1U
                                                   : 0U))
                                              : 2U)
                                          : 0U)));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_usr_ob_valid) 
           & (2U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen 
        = ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid) 
           & (1U == vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__next_state));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd 
        = (1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)) 
                 & ((~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__u_cr_fifo_wrap1_tob.__PVT__afull_r)) 
                    & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_valid)) 
                       | (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_rsm__DOT__tlvp_rsm_pt_wen)))));
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                           - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__tlvp_pt_ib_wen) {
        vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__pt_ob_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                               - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_cr_osf_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__DOT__u_cr_fifo_wrap1_pt__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
}

VL_INLINE_OPT void Vcr_cddip___024root___nba_sequent__TOP__5(Vcr_cddip___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcr_cddip___024root___nba_sequent__TOP__5\n"); );
    // Body
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((7U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (7U & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                           - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__axi4s_slv_wen) {
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0xfU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0xfU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                               - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_slv__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
        = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots;
    if (vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_rptr 
                = ((0x1fU == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))
                    ? 0U : (0x1fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_rptr))));
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                = (0x3fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots) 
                            - (IData)(1U)));
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 1U;
            }
            vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                = (0x3fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots)));
        }
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 0U;
    } else {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full;
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
            = vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots;
    }
    if (vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__axi4s_slv_wen) {
        vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_empty = 0U;
        if ((1U & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_full)))) {
            if (((IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_tlvp__DOT__u_cr_tlvp_dsm__u_cr_tlvp_id.tlvp_ib_rd) 
                 & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_empty)))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x3fU & (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots));
            } else {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_used_slots 
                    = (0x3fU & ((IData)(1U) + (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_used_slots)));
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots 
                    = (0x3fU & ((IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__r_free_slots) 
                                - (IData)(1U)));
            }
            if ((0U == (IData)(vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_free_slots))) {
                vlSelf->cr_cddip__DOT__u_cr_prefix_attach__DOT__u_cr_prefix_attach_core__DOT__prefix_attach_core__DOT__u_cr_tlvp_top__DOT__u_cr_axi4s_slave__DOT__u_cr_fifo_wrap1__DOT__entires_gt0__DOT__u_nx_fifo__DOT__depth_n__DOT__fifo_ctrl__DOT__c_full = 1U;
            }
        }
    }
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty;
    vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
        = vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr;
    if ((1U & ((~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_empty)) 
               & (~ (IData)(vlSymsp->TOP__cr_cddip__DOT__u_cr_osf__DOT__u_cr_osf_core__DOT__u_pdt_fifo_axi4s_slv__DOT__u_cr_fifo_wrap1.__PVT__afull_r))))) {
        if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_full) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 1U;
        }
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 0U;
    }
    if (((vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_out[2U] 
          >> 0x12U) & (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__axi4s_ib_in))) {
        if (vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty) {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_rptr 
                = (0x7ffU & (~ (IData)(vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__r_rptr)));
        } else {
            vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_full = 1U;
        }
        vlSelf->cr_cddip__DOT__u_cr_cg__DOT__u_cr_cg_core__DOT__u_cr_cg_tlv_mods__DOT__u_cr_cg_tlvp__DOT__u_cr_axi4s_mstr__DOT__u_axi_channel_reg_slice__DOT__full__DOT__c_empty = 0U;
    }
}
