
// -------------------------------------------------------------
// mix2cfg.hpp -- parameter bundle for Mix2<CP> (sh_mix2.inc).
//
//   #define CP_NAME CP_M0     // bundle struct to define
//   #define CP_PFX  M0_       // prefix of the IDX constants (M0_W0, ...)
//   #include "mix2cfg.hpp"
//
// Config_W / Config_B are ParamUpdater configs with the scalings of
// newton.inc (F_W_M1_F = 1 - I_W_M1/(SCALE<<9), ...); the rest are the
// mixer's seeds and boxes.  Constant expressions in the shipping build,
// runtime reads of the patchable knobs in the tuning build.
// -------------------------------------------------------------

#define CP_CAT2(a,b) a##b
#define CP_CAT(a,b)  CP_CAT2(a,b)
#define CPX(n)       CP_CAT(CP_PFX, n)

struct CP_NAME {
  def_Config(Config_W)
  def_Config(Config_B)
  static const int   ON, BIAS, WDOM;
  static const float W0, Wclip, Bclip, Pmin, Wlin;
};

#define set  const float CP_NAME::Config_W::
set momentum_D = 1.0f - float(CPX(M1w)) / (SCALE<<9);
set momentum_R = 1.0f - float(CPX(M2w)) / (SCALE<<11);
set NW         = float(CPX(NWw)) / (SCALE<<9);
set inc        = fmaxf( float(CPX(INCw)) / (SCALE<<11), 1.0f/(SCALE<<11) );   // never 0: R+inc divides
set stepMax    = float(CPX(STEPw)) / (SCALE<<8);
set minVal     = 0.0f;
set maxVal     = 0.0f;
set grad1_clip = 0.0f;
set grad2_clip = float(CPX(G2w)) / (1<<5);
set D_clip     = float(CPX(DCw)) / (1<<4);
set R_clip     = float(CPX(RCw)) / (1<<4);
set R0         = 0.0f;
#undef set

#define set  const float CP_NAME::Config_B::
set momentum_D = 1.0f - float(CPX(M1b)) / (SCALE<<9);
set momentum_R = 1.0f - float(CPX(M2b)) / (SCALE<<11);
set NW         = float(CPX(NWb)) / (SCALE<<9);
set inc        = fmaxf( float(CPX(INCb)) / (SCALE<<11), 1.0f/(SCALE<<11) );
set stepMax    = float(CPX(STEPb)) / (SCALE<<8);
set minVal     = 0.0f;
set maxVal     = 0.0f;
set grad1_clip = 0.0f;
set grad2_clip = float(CPX(G2b)) / (1<<5);
set D_clip     = float(CPX(DCb)) / (1<<4);
set R_clip     = float(CPX(RCb)) / (1<<4);
set R0         = 0.0f;
#undef set

const int   CP_NAME::ON    = CPX(ON);
const int   CP_NAME::BIAS  = CPX(BIAS);
const int   CP_NAME::WDOM  = CPX(WDOM);
const float CP_NAME::W0    = float(CPX(W0)) / SCALE;
const float CP_NAME::Wclip = float(CPX(Wclip)) / (1<<8);
const float CP_NAME::Bclip = float(CPX(Bclip)) / (1<<8);
const float CP_NAME::Wlin  = float(CPX(Wlin)) / (1<<8);
const float CP_NAME::Pmin  = fmaxf( float(CPX(Pmin)) / (float(SCALE) * 65536.0f), 1.0f/(1<<24) );   // never 0: st() would be infinite

#undef CPX
#undef CP_CAT
#undef CP_CAT2
#undef CP_NAME
#undef CP_PFX
