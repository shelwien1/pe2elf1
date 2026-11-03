
static const byte L9_sgn_a_cls[9+1]={ 0,0,1,1,1,1,2,3,4,4 };
static const byte L9_sgn_a_p0s[7+1]={ 0,1,1,1,2,2,2,3 };
static const byte L9_sgn_a_mg[9+1]={ 0,0,1,2,3,3,4,4,5,5 };
static const byte L9_sgn_a_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,3,4,4,4,4,4,4,4,4,4,4,4 };
static const byte L9_sgn_a_sq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };
static const byte L9_sgn_a_ps[13+1]={ 0,0,0,0,0,1,2,2,3,4,4,5,5,5 };
static const byte L9_sgn_a_x1s[13+1]={ 0,1,1,1,2,2,3,3,3,4,5,5,6,7 };

static const byte L9_sgn_b_cls[15+1]={ 0,0,0,1,2,2,3,4,4,5,5,6,6,6,6,7 };
static const byte L9_sgn_b_p0s[7+1]={ 0,0,0,1,2,3,4,4 };
static const byte L9_sgn_b_t1s[7+1]={ 0,0,0,1,2,3,4,4 };
static const byte L9_sgn_b_sq[32+1]={ 0,0,0,1,1,1,1,2,2,2,2,2,3,3,4,4,4,4,4,5,6,6,6,7,8,8,9,9,9,9,9,10,10 };
static const byte L9_sgn_b_chn[7+1]={ 0,1,1,2,3,3,4,4 };
static const byte L9_sgn_b_x1s[13+1]={ 0,0,0,0,0,1,2,3,4,5,5,5,5,5 };

static const byte L9_sgn_c_sq[32+1]={ 0,0,0,0,0,0,1,2,2,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte L9_sgn_d_p0s[7+1]={ 0,0,0,1,1,2,2,3 };
static const byte L9_sgn_d_ps[13+1]={ 0,0,0,1,1,1,1,2,2,2,2,3,3,3 };

static const byte L9_sgn_f_p0s[7+1]={ 0,1,2,3,3,4,5,6 };
static const byte L9_sgn_f_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,5,6,7,8,9,10,10,10,10,10,10,10,10,10,10,10 };
static const byte L9_sgn_f_sq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,3,3,3,3,3,3,4,4,4,4,5,5,5,6,6,6 };


static const byte L9_sgn_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,5,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7 };
static const byte L9_sgn_m_sq[32+1]={ 0,0,0,0,0,1,2,2,2,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,5,6,6,7,7,7,7,7 };
static const byte L9_sgn_m_x1s[13+1]={ 0,1,1,1,1,2,3,4,5,6,6,7,7,8 };


static const byte L9_sgn_s_p0s[7+1]={ 0,0,0,0,1,2,3,4 };
static const byte L9_sgn_s_ps[13+1]={ 0,0,0,0,0,1,2,2,2,2,2,3,3,3 };
static const byte L9_sgn_s_chn[7+1]={ 0,1,1,2,2,3,3,4 };



static const int L9_sgn_rA = (254+0) * (1);
static const int L9_sgn_rB = (255+0) * (1);
static const int L9_sgn_rC = (10+0) * (1);
static const int L9_sgn_rD = (13+0) * (1);
static const int L9_sgn_rS1 = (7+0) * (1);
static const int L9_sgn_rS2 = (7+0) * (1);
static const int L9_sgn_lr = (18+0) * (1);
static const int L9_sgn_mwA = (0+0) * (1);
static const int L9_sgn_mwB = (0+0) * (1);
static const int L9_sgn_mwC = (0+0) * (1);
static const int L9_sgn_mwD = (0+0) * (1);
static const int L9_sgn_bw = (12+0) * (1);
static const int L9_sgn_qs = (46+3) * (1);
static const int L9_sgn_qf = (62+3) * (1);
static const int L9_sgn_mb = (16+0) * (1);
static const int L9_sgn_hA = (15+0) * (1);
static const int L9_sgn_hB = (16+0) * (1);
static const int L9_sgn_hD = (31+0) * (1);
static const int L9_sgn_hS = (31+0) * (1);
static const int L9_sgn_hF = (14+0) * (1);
static const int L9_sgn_hM = (31+0) * (1);
static const int L9_sgn_hT = (31+0) * (1);
static const int L9_sgn_hN = (31+0) * (1);


static const long long L9_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 5), 4), 1), 6), 1), 1), 3), 5), 9), 6), 2), 1), 1), 1), 8); // 6220800
static const long long L9_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 8), 5), 1), 2), 5), 3), 3), 2), 11), 1), 5), 1), 1), 1), 6); // 14256000
static const long long L9_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 3), 1), 1), 1), 1), 1), 5), 1), 2), 1), 1), 1), 1); // 120
static const long long L9_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 4), 1), 1), 2), 1), 1), 2), 4), 2), 3), 1), 1), 1); // 384
static const long long L9_sgn_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 7), 3), 11), 7), 3), 2), 2), 2), 1), 1); // 232848
static const long long L9_sgn_g_Volume = 1; // 1
static const long long L9_sgn_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 2), 8), 8), 1), 2), 1), 1), 1), 9); // 41472
static const long long L9_sgn_n_Volume = 1; // 1
static const long long L9_sgn_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 5), 1), 3), 2), 4), 5), 2), 1), 1), 1); // 2400
static const long long L9_sgn_t_Volume = 1; // 1

struct L9_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L9_sgn_A[tc_rows(L9_sgn_a_Volume, L9_sgn_hA) * L9_sgn_ND];
  cm_cnt L9_sgn_B[tc_rows(L9_sgn_b_Volume, L9_sgn_hB) * L9_sgn_ND];
  cm_cnt L9_sgn_C[L9_sgn_c_Volume * L9_sgn_ND];
  cm_cnt L9_sgn_D[tc_rows(L9_sgn_d_Volume, L9_sgn_hD) * L9_sgn_ND];
  cm_cnt L9_sgn_MT[tc_rows(L9_sgn_t_Volume, L9_sgn_hT) * L9_sgn_MND];
  i16 L9_sgn_S[tc_rows(L9_sgn_s_Volume, L9_sgn_hS) * L9_sgn_ND * L9_sgn_qs];
  u8 L9_sgn_SC[tc_rows(L9_sgn_s_Volume, L9_sgn_hS) * L9_sgn_ND * L9_sgn_qs];
  i16 L9_sgn_F[tc_rows(L9_sgn_f_Volume, L9_sgn_hF) * L9_sgn_ND * L9_sgn_qf];
  u8 L9_sgn_FC[tc_rows(L9_sgn_f_Volume, L9_sgn_hF) * L9_sgn_ND * L9_sgn_qf];
  i32 L9_sgn_W[tc_rows(L9_sgn_m_Volume, L9_sgn_hM) * L9_sgn_ND * TC_MIXN];
  u8 L9_sgn_WC[tc_rows(L9_sgn_m_Volume, L9_sgn_hM) * L9_sgn_ND];
  i32 L9_sgn_WM[tc_rows(L9_sgn_n_Volume, L9_sgn_hN) * L9_sgn_MND * 3];
  u8 L9_sgn_WMC[tc_rows(L9_sgn_n_Volume, L9_sgn_hN) * L9_sgn_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L9_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L9_sgn_a_Volume, L9_sgn_hA) * L9_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L9_sgn_b_Volume, L9_sgn_hB) * L9_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L9_sgn_c_Volume * L9_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L9_sgn_d_Volume, L9_sgn_hD) * L9_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L9_sgn_t_Volume, L9_sgn_hT) * L9_sgn_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L9_sgn_s_Volume, L9_sgn_hS) * L9_sgn_ND * L9_sgn_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L9_sgn_s_Volume, L9_sgn_hS) * L9_sgn_ND * L9_sgn_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L9_sgn_f_Volume, L9_sgn_hF) * L9_sgn_ND * L9_sgn_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L9_sgn_f_Volume, L9_sgn_hF) * L9_sgn_ND * L9_sgn_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L9_sgn_m_Volume, L9_sgn_hM) * L9_sgn_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L9_sgn_m_Volume, L9_sgn_hM) * L9_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L9_sgn_n_Volume, L9_sgn_hN) * L9_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L9_sgn_n_Volume, L9_sgn_hN) * L9_sgn_MND);
  void L9_sgn_Init( void ) {
  
  }
};
