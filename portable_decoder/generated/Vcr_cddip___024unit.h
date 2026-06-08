// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcr_cddip.h for the primary calling header

#ifndef VERILATED_VCR_CDDIP___024UNIT_H_
#define VERILATED_VCR_CDDIP___024UNIT_H_  // guard

#include "verilated.h"


class Vcr_cddip__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcr_cddip___024unit final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    static VlUnpacked<std::string, 8> __Venumtab_enum_name0;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name1;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name2;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name3;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name4;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name5;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name6;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name7;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name8;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name9;
    static VlUnpacked<std::string, 8> __Venumtab_enum_name10;

    // INTERNAL VARIABLES
    Vcr_cddip__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vcr_cddip___024unit(Vcr_cddip__Syms* symsp, const char* v__name);
    ~Vcr_cddip___024unit();
    VL_UNCOPYABLE(Vcr_cddip___024unit);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
