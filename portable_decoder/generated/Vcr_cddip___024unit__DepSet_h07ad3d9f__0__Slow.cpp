// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcr_cddip.h for the primary calling header

#include "Vcr_cddip__pch.h"
#include "Vcr_cddip___024unit.h"

VL_ATTR_COLD void Vcr_cddip___024unit___ctor_var_reset(Vcr_cddip___024unit* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vcr_cddip__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+  Vcr_cddip___024unit___ctor_var_reset\n"); );
    // Body
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name0[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name0[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name0[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name0[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name0[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name0[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name0[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name0[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name1[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name1[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name1[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name1[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name1[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name1[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name1[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name1[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name2[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name2[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name2[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name2[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name2[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name2[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name2[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name2[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name3[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name3[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name3[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name3[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name3[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name3[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name3[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name3[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name4[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name4[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name4[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name4[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name4[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name4[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name4[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name4[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name5[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name5[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name5[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name5[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name5[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name5[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name5[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name5[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name6[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name6[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name6[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name6[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name6[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name6[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name6[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name6[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name7[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name7[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name7[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name7[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name7[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name7[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name7[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name7[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name8[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name8[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name8[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name8[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name8[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name8[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name8[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name8[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name9[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name9[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name9[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name9[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name9[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name9[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name9[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name9[7] = std::string{"PDN"};
    for (int __Vi = 0; __Vi < 8; ++__Vi) {
        vlSelf->__Venumtab_enum_name10[__Vi] = std::string{""};
    }
    vlSelf->__Venumtab_enum_name10[0] = std::string{"RDY"};
    vlSelf->__Venumtab_enum_name10[1] = std::string{"BSY"};
    vlSelf->__Venumtab_enum_name10[2] = std::string{"TMO"};
    vlSelf->__Venumtab_enum_name10[3] = std::string{"OVR"};
    vlSelf->__Venumtab_enum_name10[4] = std::string{"NXM"};
    vlSelf->__Venumtab_enum_name10[5] = std::string{"UOP"};
    vlSelf->__Venumtab_enum_name10[7] = std::string{"PDN"};
}
