
#if 0
def_Config(Config_WR)
#define set  const float Config_WR::
set momentum_D = 1.0f-float(C0_M1_w)/(SCALE<<8);
set momentum_R = 1.0f-float(C0_M2_w)/(SCALE<<8);
set NW         = float(C0_NWw)/(SCALE<<8);
set inc        = float(C0_RWinc) / (SCALE<<8);
set stepMax    = float(C0_wrStep) / (SCALE<<8);
set minVal     = float(C0_wrMin) / (SCALE<<3);
set maxVal     = float(C0_wrMax) / (SCALE<<8);
set grad1_clip = float(C0_G1_w);
set grad2_clip = float(C0_G2_w)/(1<<5);
set D_clip = float(C0_G3_w);
set R_clip = float(C0_G4_w);
set R0 = 0.0f;
#undef set
#endif

def_Config(Config_MW)
#define set  const float Config_MW::
set momentum_D = 1.0f-float(C0_M1_m)/(SCALE<<8);
set momentum_R = 1.0f-float(C0_M2_m)/(SCALE<<8);
set NW         = float(C0_NWm)/(SCALE<<8);
set inc        = float(C0_MWinc) / (SCALE<<8);
set stepMax    = float(C0_mwStep) / (SCALE<<8);
set minVal     = float(C0_mwMin) / (SCALE<<3);
set maxVal     = float(C0_mwMax) / (SCALE<<3);
set grad1_clip = float(C0_G1_m);
set grad2_clip = float(C0_G2_m)/(1<<5);
set D_clip = float(C0_G3_m);
set R_clip = float(C0_G4_m);
set R0 = 0.0f;
#undef set

def_Config(Config_K)
#define set  const float Config_K::
set momentum_D = 1.0f-float(C0_M1_k)/(SCALE<<8);
set momentum_R = 1.0f-float(C0_M2_k)/(SCALE<<8);
set NW         = float(C0_NWk) / (SCALE<<8);
set inc        = float(C0_RKinc) / (SCALE<<8);
set stepMax    = float(C0_kStep) / (SCALE<<8);
set minVal     = float(C0_kMin) / (SCALE<<8);
set maxVal     = float(C0_kMax) / (SCALE>>2);
set grad1_clip = float(C0_G1_k);
set grad2_clip = float(C0_G2_k)/(1<<5);
set D_clip = float(C0_G3_k);
set R_clip = float(C0_G4_k);
set R0 = 0.0f;
#undef set

// -------------------------------------------------------------
// LOGWR/UVROT: wr is optimized in log space, in the rotated basis
//   u = (ln wr0 + ln wr1)/2   (memory length)
//   v = (ln wr0 - ln wr1)/2   (hit/miss asymmetry)
// wr0_state tracks u (Config_U), wr1_state tracks v (Config_V, a
// slower dedicated optimizer). Effective wr = exp(clamp(u+-v)).
// RTRL traces stay in linear wr space; the chain to log coordinates
// (dp/du = dp/dwr*wr, d2p/du2 = d2p/dwr2*wr^2 + dp/dwr*wr) is applied
// at gradient-assembly time each step.
// New constants in sh_model-C0_h.inc style, staged for .idx migration.
// -------------------------------------------------------------

def_Config(Config_U)
#define set  const float Config_U::
set momentum_D = 1.0f-float(C0_M1_u)/(SCALE<<8);
set momentum_R = 1.0f-float(C0_M2_u)/(SCALE<<8);
set NW         = float(C0_NWu)/(SCALE<<8);
set inc        = float(C0_RUinc) / (SCALE<<8);
set stepMax    = float(C0_uStep) / (SCALE<<8);
set minVal     = float(C0_uMin) / (SCALE<<3);
set maxVal     = float(C0_uMax) / (SCALE<<8);
set grad1_clip = float(C0_G1_u);
set grad2_clip = float(C0_G2_u)/(1<<5);
set D_clip = float(C0_G3_u);
set R_clip = float(C0_G4_u);
set R0 = float(C0_R0_u)/256;
#undef set

def_Config(Config_V)
#define set  const float Config_V::
set momentum_D = 1.0f-float(C0_M1_v)/(SCALE<<8);
set momentum_R = 1.0f-float(C0_M2_v)/(SCALE<<8);
set NW         = float(C0_NWv)/(SCALE<<8);
set inc        = float(C0_RVinc) / (SCALE<<8);
set stepMax    = float(C0_vStep) / (SCALE<<8);
set minVal     = float(C0_uMin) / (SCALE<<3);
set maxVal     = float(C0_uMax) / (SCALE<<8);
set grad1_clip = float(C0_G1_v);
set grad2_clip = float(C0_G2_v)/(1<<5);
set D_clip = float(C0_G3_v);
set R_clip = float(C0_G4_v);
set R0 = float(C0_R0_v)/256;
#undef set
