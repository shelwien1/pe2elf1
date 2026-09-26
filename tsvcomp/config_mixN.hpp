// -------------------------------------------------------------
// config_mixN.hpp -- parameter bundle for MixN<CP,N> (sh_mixN.inc): a Mix2
// bundle (config_mix2.hpp) plus what N > 2 and the gain add.
//
//   #define CP_NAME CP_M0m    // the Mix2 part first
//   #define CP_PFX  M0_
//   #include "config_mix2.hpp"
//   #define CP_NAME CP_M0     // then the MixN bundle, derived from it
//   #define CP_BASE CP_M0m
//   #define CP_PFX  M0_
//   #include "config_mixN.hpp"
//
// Knobs (with the prefix): XW (cross-curvature weight, /1024), GON (gain
// on), G0 (seed of the log gain, (G0-1024)/256), Glo/Ghi (its box, -Glo/256
// .. Ghi/256), and the gain's ParamUpdater M1g, M2g, NWg, INCg, STEPg, G2g,
// DCg, RCg, NAGg, scaled like the bias's (Config_B).
// -------------------------------------------------------------

#define CP_CAT2(a,b) a##b
#define CP_CAT(a,b)  CP_CAT2(a,b)
#define CPX(n)       CP_CAT(CP_PFX, n)

struct CP_NAME : CP_BASE {
  def_Config(Config_G)
  static const float XW, G0, Glo, Ghi;
  static const int GON;
};

#define set  const float CP_NAME::Config_G::
set momentum_D = 1.0f - float(CPX(M1g)) / (SCALE<<9);
set momentum_R = 1.0f - float(CPX(M2g)) / (SCALE<<11);
set NW         = float(CPX(NWg)) / (SCALE<<9);
set inc        = fmaxf( float(CPX(INCg)) / (SCALE<<11), 1.0f/(SCALE<<11) );   // never 0: R+inc divides
set stepMax    = float(CPX(STEPg)) / (SCALE<<8);
set minVal     = 0.0f;
set maxVal     = 0.0f;
set grad2_clip = float(CPX(G2g)) / (1<<5);
set D_clip     = float(CPX(DCg)) / (1<<4);
set R_clip     = float(CPX(RCg)) / (1<<4);
set R0         = 0.0f;
#undef set
const int   CP_NAME::Config_G::NAG = CPX(NAGg);

const float CP_NAME::XW  = float(CPX(XW)) / 1024;
const int   CP_NAME::GON = CPX(GON) ? 1 : 0;
const float CP_NAME::Glo = -float(CPX(Glo)) / 256;
const float CP_NAME::Ghi = float(CPX(Ghi)) / 256;
const float CP_NAME::G0  = clamp( float(CPX(G0) - 1024) / 256, CP_NAME::Glo, CP_NAME::Ghi );

#undef CPX
#undef CP_CAT
#undef CP_CAT2
#undef CP_NAME
#undef CP_BASE
#undef CP_PFX
