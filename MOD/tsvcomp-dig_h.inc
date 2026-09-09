
static const byte TC_dig_a_band[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte TC_dig_b_band[15+1]={ 0,1,2,2,3,4,5,6,6,7,8,9,10,10,11,12 };
static const byte TC_dig_b_pq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };


static const byte TC_dig_d_q1s[13+1]={ 0,1,1,2,2,3,4,4,5,6,6,7,7,8 };

static const byte TC_dig_f_cls[9+1]={ 0,1,1,2,3,4,5,6,7,8 };


static const byte TC_dig_m_cls[15+1]={ 0,1,1,2,2,3,4,5,5,6,6,7,7,8,8,9 };





static const int TC_dig_avD = (239+0) * (1);
static const int TC_dig_avA = (7+0) * (1);
static const int TC_dig_avT1 = (2+0) * (1);
static const int TC_dig_avT2 = (1+0) * (1);
static const int TC_dig_avN1 = (9+0) * (1);
static const int TC_dig_avW1 = (3+0) * (1);
static const int TC_dig_avQ1 = (5+0) * (1);
static const int TC_dig_avQ2 = (3+0) * (1);
static const int TC_dig_avX1 = (5+0) * (1);
static const int TC_dig_avC = (431+0) * (1);
static const int TC_dig_rA = (255+0) * (1);
static const int TC_dig_rB = (254+0) * (1);
static const int TC_dig_rC = (16+0) * (1);
static const int TC_dig_rD = (227+0) * (1);
static const int TC_dig_rT = (255+0) * (1);
static const int TC_dig_rS1 = (7+0) * (1);
static const int TC_dig_rS2 = (7+0) * (1);
static const int TC_dig_lr = (11+0) * (1);
static const int TC_dig_lrm = (32+0) * (1);
static const int TC_dig_mbm = (68+0) * (1);
static const int TC_dig_mwA = (2+0) * (1);
static const int TC_dig_mwB = (2+0) * (1);
static const int TC_dig_mwC = (2+0) * (1);
static const int TC_dig_mwD = (2+0) * (1);
static const int TC_dig_mwT = (2+0) * (1);
static const int TC_dig_bw = (11+0) * (1);
static const int TC_dig_qs = (30+3) * (1);
static const int TC_dig_qf = (62+3) * (1);
static const int TC_dig_mb = (68+0) * (1);


static const long long TC_dig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 16), 5), 1), 1), 1), 2), 6), 1), 2), 1), 1), 1), 1), 1), 1), 1), 1), 2), 1), 2), 3), 1), 1), 1), 1), 1), 1); // 184320
static const long long TC_dig_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 13), 3), 4), 1), 1), 3), 2), 3), 2), 1), 1), 1), 1), 1), 1), 1), 4), 1), 2), 1), 9), 1), 1), 1), 1), 1), 1); // 1213056
static const long long TC_dig_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 3), 1), 1), 1), 1), 5), 2), 1), 1), 2), 3), 3), 1), 1), 1), 1), 1); // 1080
static const long long TC_dig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 1), 1), 1), 1), 1), 7), 9), 1), 1), 7), 3), 4), 2), 1), 1), 1), 8); // 169344
static const long long TC_dig_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 9), 4), 1), 1), 6), 4), 1), 1), 2), 1), 1), 1), 1); // 5184
static const long long TC_dig_g_Volume = 1; // 1
static const long long TC_dig_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 2), 10), 8), 1), 1), 1), 1), 1), 1), 2), 1), 3), 1), 7), 1), 2), 1), 1), 1), 1); // 26880
static const long long TC_dig_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 1), 1), 7), 1), 1), 1), 1), 1), 1), 1), 2), 1), 1), 1), 1), 1), 5), 1), 1), 1), 1); // 280
static const long long TC_dig_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 1), 3), 3), 1), 1), 1), 1), 2), 1), 1), 1), 3), 2), 1), 8), 1), 2), 2), 1), 2), 1), 1); // 27648
static const long long TC_dig_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 1), 1), 7), 1), 1), 1), 1), 1), 1), 1), 2), 1), 1), 1), 1), 1), 5), 1), 1), 1), 1); // 280

struct TC_dig_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_dig_A[TC_dig_a_Volume * TC_dig_ND];
  cm_cnt TC_dig_B[TC_dig_b_Volume * TC_dig_ND];
  cm_cnt TC_dig_C[TC_dig_c_Volume * TC_dig_ND];
  cm_cnt TC_dig_D[TC_dig_d_Volume * TC_dig_ND];
  cm_cnt TC_dig_MT[TC_dig_t_Volume * TC_dig_MND];
  i16 TC_dig_S[TC_dig_s_Volume * TC_dig_ND * TC_dig_qs];
  u8 TC_dig_SC[TC_dig_s_Volume * TC_dig_ND * TC_dig_qs];
  i16 TC_dig_F[TC_dig_f_Volume * TC_dig_ND * TC_dig_qf];
  u8 TC_dig_FC[TC_dig_f_Volume * TC_dig_ND * TC_dig_qf];
  i32 TC_dig_W[TC_dig_m_Volume * TC_dig_ND * TC_MIXN];
  u8 TC_dig_WC[TC_dig_m_Volume * TC_dig_ND];
  i32 TC_dig_WM[TC_dig_n_Volume * TC_dig_MND * 3];
  u8 TC_dig_WMC[TC_dig_n_Volume * TC_dig_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_dig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_a_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_b_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_c_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_d_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_t_Volume * TC_dig_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_s_Volume * TC_dig_ND * TC_dig_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_s_Volume * TC_dig_ND * TC_dig_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_f_Volume * TC_dig_ND * TC_dig_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_f_Volume * TC_dig_ND * TC_dig_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_m_Volume * TC_dig_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_m_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_n_Volume * TC_dig_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_n_Volume * TC_dig_MND);
  void TC_dig_Init( void ) {
  
  }
  void TC_dig_Quit( void ) {
  }
};
