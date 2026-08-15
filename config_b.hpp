
// -------------------------------------------------------------
// config_b.hpp -- ParamUpdater configuration for bmpc's LPC mixer,
// the B0 counterpart of config.hpp's Config_MW / Config_K / Config_U.
//
// One class, Config_LW, is shared by every weight of every mixer (and
// by the bias input, which is just input #n with the constant value 1 --
// the Newton denominator normalises its much larger E[x^2] on its own,
// so it needs no separate learning rate).
//
// The scalings differ from config.hpp's because the quantities differ:
// the counter's wr/mw/K live on (0,1)-ish boxes reached through log and
// logit maps, while a mix weight is an unbounded real of order 1 whose
// gradient is e*x and whose curvature is x^2.
// -------------------------------------------------------------

def_Config(Config_LW)
#define set  const float Config_LW::
set momentum_D = float(B0_M1_w) / 1024.0f;
set momentum_R = float(B0_M2_w) / 65536.0f;
set NW         = float(B0_NW_w) / 4096.0f;
set inc        = float(B0_INC_w) / float(1<<20);
set stepMax    = float(B0_STEP_w) / 65536.0f;
set minVal     = -float(B0_WMAX_w) / 1024.0f;
set maxVal     =  float(B0_WMAX_w) / 1024.0f;
set grad1_clip = float(B0_G1_w) / 1024.0f;
set grad2_clip = float(B0_G2_w) / 1024.0f;
set D_clip     = float(B0_D_w) / 64.0f;
set R_clip     = float(B0_R_w);
set R0         = float(B0_R0_w) / float(1<<20);
#undef set

// The second mix of MIX_DUAL: same shape, its own pace.
def_Config(Config_LW2)
#define set  const float Config_LW2::
set momentum_D = float(B0_M1_w) / 1024.0f;
set momentum_R = float(B0_M2_w) / 65536.0f;
set NW         = float(B0_NW2_w) / 4096.0f;
set inc        = float(B0_INC2_w) / float(1<<20);
set stepMax    = float(B0_STEP_w) / 65536.0f;
set minVal     = -float(B0_WMAX_w) / 1024.0f;
set maxVal     =  float(B0_WMAX_w) / 1024.0f;
set grad1_clip = float(B0_G1_w) / 1024.0f;
set grad2_clip = float(B0_G2_w) / 1024.0f;
set D_clip     = float(B0_D_w) / 64.0f;
set R_clip     = float(B0_R_w);
set R0         = float(B0_R0_w) / float(1<<20);
#undef set

// The convex-combination coordinate z (logistic domain, unbounded box).
def_Config(Config_LM)
#define set  const float Config_LM::
set momentum_D = 0.0f;
set momentum_R = float(B0_MM2)/65536.0f;
set NW         = float(B0_NWL)/4096.0f;
set inc        = float(B0_INCL)/float(1<<20);
set stepMax    = float(B0_STEPL)/65536.0f;
set minVal     = -16.0f;
set maxVal     =  16.0f;
set grad1_clip = 1e9f;
set grad2_clip = 1e9f;
set D_clip     = 1e9f;
set R_clip     = 1e9f;
set R0         = 0.0f;
#undef set

// The residual-model mixer: weights on stretched probabilities.
def_Config(Config_BM)
#define set  const float Config_BM::
set momentum_D = 0.0f;
set momentum_R = 0.0f;
set NW         = float(B0_NWB)/4096.0f;
set inc        = float(B0_INCB)/1024.0f;
set stepMax    = float(B0_STEPB)/65536.0f;
set minVal     = -8.0f;
set maxVal     =  8.0f;
set grad1_clip = 1e9f;
set grad2_clip = 1e9f;
set D_clip     = float(B0_DB)/1024.0f;
set R_clip     = 1e9f;
set R0         = 0.0f;
#undef set

// Weight-space box and seeds (shared by both parameterisations).
static const float LW_LO   = float(B0_WLO) / 1024.0f;
static const float LW_HI   = float(B0_WHI) / 1024.0f;
static const float LW_SPAN = LW_HI - LW_LO;
static const float LW_W0   = float(B0_W0) / 1024.0f;
static const float LW_SQC  = float(B0_SQC) / 1024.0f;
static const float LW_HUB  = float(B0_HUB) / 1024.0f;   // MIX_LOSS knee
static const float LW_HUB2 = float(B0_HUB2) / 1024.0f;  // knee of the 2nd mix
static const float BIAS_R  = float(B0_BR) / 1024.0f;    // MIX_BIAS==2 EMA rate
static const float LW_HUBA = float(B0_HUBA) / 256.0f;   // MIX_LOSS==3 knee, in EMA(|e|)
static const float MAD_R   = float(B0_MADR) / 1024.0f;
static const float LW_MAD0 = float(B0_MAD0) / 1024.0f;

static const float BM_W0   = float(B0_BMW0) / 1024.0f;  // mixer weight seed
static const float SSEW    = float(B0_SSEW) / 1024.0f;  // APM blend weight
