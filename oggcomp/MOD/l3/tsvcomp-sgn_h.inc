
static const byte L3_sgn_a_cls[9+1]={ 0,0,1,2,3,4,5,6,7,7 };
static const byte L3_sgn_a_p0s[7+1]={ 0,1,1,1,2,2,2,3 };
static const byte L3_sgn_a_mg[9+1]={ 0,1,1,1,2,2,3,4,4,4 };
static const byte L3_sgn_a_band[5+1]={ 0,1,1,2,3,4 };
static const byte L3_sgn_a_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
static const byte L3_sgn_a_sq[32+1]={ 0,1,1,2,2,2,2,2,3,3,3,4,5,5,5,5,5,5,5,6,7,7,7,7,8,9,9,9,10,10,10,10,10 };
static const byte L3_sgn_a_ps[13+1]={ 0,0,0,1,1,2,3,3,4,5,5,6,6,6 };
static const byte L3_sgn_a_x1s[13+1]={ 0,1,1,1,2,2,3,3,3,4,5,5,6,7 };

static const byte L3_sgn_b_cls[15+1]={ 0,0,0,1,2,2,3,4,4,5,5,6,6,6,6,7 };
static const byte L3_sgn_b_p0s[7+1]={ 0,0,0,1,2,3,4,4 };
static const byte L3_sgn_b_q1s[7+1]={ 0,0,1,1,2,3,3,4 };
static const byte L3_sgn_b_t1s[7+1]={ 0,0,1,1,2,3,3,3 };
static const byte L3_sgn_b_pq[32+1]={ 0,0,0,0,0,0,1,1,1,1,2,3,4,4,4,5,5,5,5,5,5,5,6,7,8,8,8,9,9,9,9,10,11 };
static const byte L3_sgn_b_chn[7+1]={ 0,1,1,2,3,3,4,4 };
static const byte L3_sgn_b_x1s[13+1]={ 0,0,1,2,2,3,4,5,5,5,6,6,6,6 };

static const byte L3_sgn_c_sq[32+1]={ 0,0,0,0,0,0,1,2,2,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte L3_sgn_c_ps[13+1]={ 0,0,1,1,1,1,1,1,1,1,1,1,2,3 };

static const byte L3_sgn_d_pq[32+1]={ 0,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,5,6,6,6,6,6,6,6,6,6,6 };

static const byte L3_sgn_f_p0s[7+1]={ 0,0,1,2,2,3,3,4 };
static const byte L3_sgn_f_sq[32+1]={ 0,0,0,0,0,1,1,1,2,2,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,7,7 };
static const byte L3_sgn_f_ps[13+1]={ 0,0,0,0,1,1,1,2,2,3,3,4,4,4 };
static const byte L3_sgn_f_chn[7+1]={ 0,1,1,1,2,3,3,3 };


static const byte L3_sgn_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte L3_sgn_m_sq[32+1]={ 0,0,0,0,0,1,2,2,2,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,5,6,6,7,7,7,7,7 };
static const byte L3_sgn_m_ps[13+1]={ 0,0,0,0,0,0,0,1,2,2,2,2,2,3 };
static const byte L3_sgn_m_x1s[13+1]={ 0,1,1,1,1,2,3,4,5,6,6,7,7,8 };


static const byte L3_sgn_s_pq[32+1]={ 0,0,0,0,0,0,0,1,1,1,1,1,1,1,2,3,3,3,3,3,3,3,4,4,4,4,4,4,4,5,5,5,6 };
static const byte L3_sgn_s_ps[13+1]={ 0,0,0,0,0,1,2,2,2,2,2,3,3,3 };
static const byte L3_sgn_s_chn[7+1]={ 0,1,1,2,2,3,3,4 };



static const int L3_sgn_rA = (254+0) * (1);
static const int L3_sgn_rB = (255+0) * (1);
static const int L3_sgn_rC = (10+0) * (1);
static const int L3_sgn_rD = (8+0) * (1);
static const int L3_sgn_rS1 = (4+0) * (1);
static const int L3_sgn_rS2 = (7+0) * (1);
static const int L3_sgn_lr = (18+0) * (1);
static const int L3_sgn_mwA = (2+0) * (1);
static const int L3_sgn_mwB = (0+0) * (1);
static const int L3_sgn_mwC = (0+0) * (1);
static const int L3_sgn_mwD = (1+0) * (1);
static const int L3_sgn_bw = (12+0) * (1);
static const int L3_sgn_qs = (37+3) * (1);
static const int L3_sgn_qf = (96+3) * (1);
static const int L3_sgn_mb = (16+0) * (1);
static const int L3_sgn_hA = (15+0) * (1);
static const int L3_sgn_hB = (16+0) * (1);
static const int L3_sgn_hD = (31+0) * (1);
static const int L3_sgn_hS = (31+0) * (1);
static const int L3_sgn_hF = (14+0) * (1);
static const int L3_sgn_hM = (31+0) * (1);
static const int L3_sgn_hT = (31+0) * (1);
static const int L3_sgn_hN = (31+0) * (1);


static const long long L3_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 8), 4), 1), 5), 1), 1), 5), 4), 11), 7), 2), 2), 1), 1), 8); // 15769600
static const long long L3_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 8), 5), 5), 2), 4), 3), 2), 12), 3), 1), 5), 1), 1), 2), 7); // 72576000
static const long long L3_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 3), 1), 1), 1), 2), 5), 4), 2), 1), 1), 1), 1); // 480
static const long long L3_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 1), 3), 1), 7), 2), 3), 2), 1), 1), 1), 1); // 504
static const long long L3_sgn_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 5), 3), 3), 8), 5), 4), 2), 2), 1), 1); // 86400
static const long long L3_sgn_g_Volume = 1; // 1
static const long long L3_sgn_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 1), 2), 5), 8), 4), 2), 1), 1), 1), 9); // 34560
static const long long L3_sgn_n_Volume = 1; // 1
static const long long L3_sgn_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 2), 1), 7), 2), 4), 5), 1), 1), 1), 1); // 1120
static const long long L3_sgn_t_Volume = 1; // 1

struct L3_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L3_sgn_A[tc_rows(L3_sgn_a_Volume, L3_sgn_hA) * L3_sgn_ND];
  cm_cnt L3_sgn_B[tc_rows(L3_sgn_b_Volume, L3_sgn_hB) * L3_sgn_ND];
  cm_cnt L3_sgn_C[L3_sgn_c_Volume * L3_sgn_ND];
  cm_cnt L3_sgn_D[tc_rows(L3_sgn_d_Volume, L3_sgn_hD) * L3_sgn_ND];
  cm_cnt L3_sgn_MT[tc_rows(L3_sgn_t_Volume, L3_sgn_hT) * L3_sgn_MND];
  i16 L3_sgn_S[tc_rows(L3_sgn_s_Volume, L3_sgn_hS) * L3_sgn_ND * L3_sgn_qs];
  u8 L3_sgn_SC[tc_rows(L3_sgn_s_Volume, L3_sgn_hS) * L3_sgn_ND * L3_sgn_qs];
  i16 L3_sgn_F[tc_rows(L3_sgn_f_Volume, L3_sgn_hF) * L3_sgn_ND * L3_sgn_qf];
  u8 L3_sgn_FC[tc_rows(L3_sgn_f_Volume, L3_sgn_hF) * L3_sgn_ND * L3_sgn_qf];
  i32 L3_sgn_W[tc_rows(L3_sgn_m_Volume, L3_sgn_hM) * L3_sgn_ND * TC_MIXN];
  u8 L3_sgn_WC[tc_rows(L3_sgn_m_Volume, L3_sgn_hM) * L3_sgn_ND];
  i32 L3_sgn_WM[tc_rows(L3_sgn_n_Volume, L3_sgn_hN) * L3_sgn_MND * 3];
  u8 L3_sgn_WMC[tc_rows(L3_sgn_n_Volume, L3_sgn_hN) * L3_sgn_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L3_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_sgn_a_Volume, L3_sgn_hA) * L3_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_sgn_b_Volume, L3_sgn_hB) * L3_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_sgn_c_Volume * L3_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_sgn_d_Volume, L3_sgn_hD) * L3_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_sgn_t_Volume, L3_sgn_hT) * L3_sgn_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_sgn_s_Volume, L3_sgn_hS) * L3_sgn_ND * L3_sgn_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_sgn_s_Volume, L3_sgn_hS) * L3_sgn_ND * L3_sgn_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_sgn_f_Volume, L3_sgn_hF) * L3_sgn_ND * L3_sgn_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_sgn_f_Volume, L3_sgn_hF) * L3_sgn_ND * L3_sgn_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_sgn_m_Volume, L3_sgn_hM) * L3_sgn_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_sgn_m_Volume, L3_sgn_hM) * L3_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_sgn_n_Volume, L3_sgn_hN) * L3_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_sgn_n_Volume, L3_sgn_hN) * L3_sgn_MND);
  void L3_sgn_Init( void ) {
  
  }
};
