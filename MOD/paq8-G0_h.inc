





static const int G0_MIXA_pt = (48+0) * (P_SCALE/256);
static const int G0_MIXA_pu = (8+0) * (P_SCALE/256);
static const int G0_MIXA_pv = (88+0) * (P_SCALE/256);
static const int G0_MIXB_pt = (32+0) * (P_SCALE/256);
static const int G0_MIXB_pu = (40+0) * (P_SCALE/256);
static const int G0_MIXB_pv = (96+0) * (P_SCALE/256);
static const int G0_SM_RATE_0 = (7+0) * (1);
static const int G0_SM_RATE_1 = (8+0) * (1);
static const int G0_SM_RATE_2 = (9+0) * (1);
static const int G0_SM_SW1 = (8+0) * (65536);
static const int G0_SM_SW2 = (16+0) * (65536);
static const int G0_SM_PRIOR_ADD = (1+0) * (1);
static const int G0_A1_RATE = (3+0) * (1);
static const int G0_A1_NH = (15+1) * (1);
static const int G0_A1_w = (32+0) * (P_SCALE/256);
static const int G0_A2_RATE = (7+0) * (1);
static const int G0_A2_SW1 = (14+0) * (262144);
static const int G0_A2_SW2 = (28+0) * (524288);
static const int G0_A2_NH = (15+1) * (1);
static const int G0_A3_RATE = (6+0) * (1);
static const int G0_A3_SW1 = (14+0) * (262144);
static const int G0_A3_SW2 = (28+0) * (524288);
static const int G0_A3_NH = (15+1) * (1);
static const int G0_A4_RATE = (6+0) * (1);
static const int G0_A4_SW1 = (14+0) * (262144);
static const int G0_A4_SW2 = (28+0) * (524288);
static const int G0_A4_NH = (15+1) * (1);
static const int G0_A5_RATE = (6+0) * (1);
static const int G0_A5_SW1 = (14+0) * (262144);
static const int G0_A5_SW2 = (28+0) * (524288);
static const int G0_A5_NH = (15+1) * (1);
static const int G0_A6_RATE = (6+0) * (1);
static const int G0_A6_SW1 = (14+0) * (262144);
static const int G0_A6_SW2 = (28+0) * (524288);
static const int G0_A6_NH = (15+1) * (1);
static const int G0_SCM_RATE_EARLY = (9+0) * (1);
static const int G0_SCM_RATE_LATE = (10+0) * (1);
static const int G0_SCM_SW = (4000+0) * (1000);
static const int G0_MIX_LR_NUM = (7+0) * (1);
static const int G0_MIX_LR_DEN = (0+1) * (1);
static const int G0_MIX2_LR_NUM = (3+0) * (1);
static const int G0_MIX2_LR_DEN = (1+1) * (1);
static const int G0_MIX_W_INIT = (512+0) * (1);
static const int G0_MIX2_W_INIT = (32767+0) * (1);
static const int G0_MIX_DP_MUL = (9+0) * (1);
static const int G0_MIX_DP_SHIFT = (9+0) * (1);
static const int G0_MIX2_Z_SHIFT = (9+0) * (1);
static const int G0_MIX2_ZB_MUL = (15+0) * (1);
static const int G0_MIX2_ZB_SHIFT = (13+0) * (1);
static const int G0_MIX_MUL_0 = (5+0) * (1);
static const int G0_MIX_MUL_1 = (6+0) * (1);
static const int G0_MIX_MUL_2 = (9+0) * (1);
static const int G0_MIX_MUL_DEN = (3+1) * (1);
static const int G0_CM_DECAY_FROM = (204+0) * (1);
static const int G0_CM_DECAY_BIAS = (452+0) * (1);
static const int G0_CM_DECAY_SHIFT = (3+0) * (1);
static const int G0_CM_DECAY_STEP = (4+0) * (1);
static const int G0_RCM_RUN_MAX = (255+0) * (1);
static const int G0_CM_RUN_MAX = (254+0) * (1);
static const int G0_CM_RUN_STEP = (2+0) * (1);
static const int G0_FAIL_TH = (1820+0) * (1);
static const int G0_FAILZ_TH = (848+0) * (1);


static const int G0_MX0_Volume = 1* 7* (1<<4)* (1<<4); // 1792
static const int G0_MX1_Volume = 1* 7* (1<<2)* (1<<6); // 1792
static const int G0_MX2_Volume = 1* 8* (1<<8); // 2048
static const int G0_MX4_Volume = 1* 7* (1<<5)* 8; // 1792

struct G0_T { 
#undef USE_NEW
#define USE_NEW 0


  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long G0_Size = 0;
  void G0_Init( void ) {
  
  }
  void G0_Quit( void ) {
  }
};
