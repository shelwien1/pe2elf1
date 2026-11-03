
static const byte L7_dig_a_pass[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte L7_dig_a_band[15+1]={ 0,1,1,1,2,2,3,4,5,6,7,8,8,9,10,11 };
static const byte L7_dig_a_cls[15+1]={ 0,0,1,2,2,3,4,5,6,7,8,8,8,8,8,8 };
static const byte L7_dig_a_ax[12+1]={ 0,1,2,2,2,2,2,2,2,3,3,3,4 };
static const byte L7_dig_a_pq[32+1]={ 0,0,1,2,3,3,3,3,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,6,7,8,8,9,9,10,11,11,11 };

static const byte L7_dig_b_band[15+1]={ 0,0,1,2,2,2,2,3,4,4,5,6,6,6,7,8 };
static const byte L7_dig_b_q1[5+1]={ 0,1,2,2,3,4 };
static const byte L7_dig_b_bkq[4+1]={ 0,1,2,3,4 };
static const byte L7_dig_b_p0[3+1]={ 0,1,2,3 };
static const byte L7_dig_b_p0s[13+1]={ 0,0,0,1,2,2,2,3,4,5,6,6,7,7 };
static const byte L7_dig_b_ax[12+1]={ 0,0,0,0,0,1,1,2,2,2,2,3,3 };
static const byte L7_dig_b_pq[32+1]={ 0,1,1,1,1,1,2,2,2,3,4,4,5,5,6,6,7,8,9,10,10,10,11,11,12,12,12,13,13,14,14,14,14 };
static const byte L7_dig_b_ps[13+1]={ 0,1,1,1,1,2,2,3,4,4,5,6,6,6 };
static const byte L7_dig_b_chn[7+1]={ 0,1,2,2,2,3,3,3 };

static const byte L7_dig_c_ax[12+1]={ 0,1,1,1,1,2,2,2,3,3,4,5,6 };
static const byte L7_dig_c_p0s[13+1]={ 0,0,0,0,0,0,0,0,0,0,1,1,2,3 };
static const byte L7_dig_c_pq[32+1]={ 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,3,4,4,4,4,4,4,4,4 };
static const byte L7_dig_c_ps[13+1]={ 0,0,1,1,1,1,2,2,3,3,3,4,4,4 };
static const byte L7_dig_c_chn[7+1]={ 0,1,1,2,2,3,3,4 };
static const byte L7_dig_c_crun[9+1]={ 0,0,0,0,0,0,0,1,2,3 };
static const byte L7_dig_c_x1s[13+1]={ 0,0,0,1,1,1,1,1,1,1,1,1,2,3 };

static const byte L7_dig_d_t1s[13+1]={ 0,0,0,0,0,1,2,2,2,3,3,4,4,4 };
static const byte L7_dig_d_ax[12+1]={ 0,0,0,1,2,2,3,4,5,5,5,6,7 };
static const byte L7_dig_d_p0s[13+1]={ 0,0,1,1,1,1,1,1,2,2,3,3,3,3 };

static const byte L7_dig_f_cls[9+1]={ 0,1,1,2,3,4,5,6,7,8 };
static const byte L7_dig_f_ax[12+1]={ 0,1,1,1,2,3,3,4,5,6,6,6,7 };
static const byte L7_dig_f_pq[32+1]={ 0,1,2,2,2,2,2,3,3,4,4,5,6,6,6,6,6,6,6,6,6,6,7,7,7,8,9,10,11,12,13,13,13 };
static const byte L7_dig_f_ps[13+1]={ 0,1,1,2,2,2,2,3,3,3,3,3,3,3 };


static const byte L7_dig_m_cls[15+1]={ 0,0,0,1,1,1,1,2,3,4,4,4,4,4,4,4 };
static const byte L7_dig_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
static const byte L7_dig_m_chn[7+1]={ 0,1,1,2,2,3,3,4 };

static const byte L7_dig_n_pass[3+1]={ 0,1,2,3 };

static const byte L7_dig_s_ax[12+1]={ 0,1,1,1,1,1,2,2,2,2,3,3,3 };
static const byte L7_dig_s_pq[32+1]={ 0,1,2,2,2,2,3,3,3,3,3,3,4,4,5,5,5,6,6,6,6,6,6,7,8,8,8,8,8,9,10,10,10 };

static const byte L7_dig_t_q1[5+1]={ 0,1,2,3,4,4 };
static const byte L7_dig_t_cls[15+1]={ 0,1,1,2,2,3,3,4,5,6,7,7,7,7,7,7 };
static const byte L7_dig_t_p0s[13+1]={ 0,0,0,0,1,1,2,3,3,4,4,4,4,4 };
static const byte L7_dig_t_pq[32+1]={ 0,1,1,1,1,1,1,2,2,2,2,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6 };
static const byte L7_dig_t_chn[7+1]={ 0,1,1,2,2,3,3,4 };


static const int L7_dig_avD = (241+0) * (1);
static const int L7_dig_avA = (7+0) * (1);
static const int L7_dig_avT1 = (7+0) * (1);
static const int L7_dig_avT2 = (1+0) * (1);
static const int L7_dig_avN1 = (5+0) * (1);
static const int L7_dig_avW1 = (3+0) * (1);
static const int L7_dig_avQ1 = (0+0) * (1);
static const int L7_dig_avQ2 = (1+0) * (1);
static const int L7_dig_avX1 = (0+0) * (1);
static const int L7_dig_avC = (382+0) * (1);
static const int L7_dig_rA = (253+0) * (1);
static const int L7_dig_rB = (252+0) * (1);
static const int L7_dig_rC = (16+0) * (1);
static const int L7_dig_rD = (223+0) * (1);
static const int L7_dig_rT = (247+0) * (1);
static const int L7_dig_rS1 = (15+0) * (1);
static const int L7_dig_rS2 = (7+0) * (1);
static const int L7_dig_lr = (13+0) * (1);
static const int L7_dig_lrm = (30+0) * (1);
static const int L7_dig_mbm = (47+0) * (1);
static const int L7_dig_mwA = (1+0) * (1);
static const int L7_dig_mwB = (2+0) * (1);
static const int L7_dig_mwC = (116+0) * (1);
static const int L7_dig_mwD = (67+0) * (1);
static const int L7_dig_mwT = (0+0) * (1);
static const int L7_dig_bw = (12+0) * (1);
static const int L7_dig_qs = (10+3) * (1);
static const int L7_dig_qf = (34+3) * (1);
static const int L7_dig_mb = (75+0) * (1);
static const int L7_dig_hA = (17+0) * (1);
static const int L7_dig_hB = (16+0) * (1);
static const int L7_dig_hD = (17+0) * (1);
static const int L7_dig_hS = (15+0) * (1);
static const int L7_dig_hF = (15+0) * (1);
static const int L7_dig_hM = (31+0) * (1);
static const int L7_dig_hT = (14+0) * (1);
static const int L7_dig_hN = (31+0) * (1);


static const long long L7_dig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 12), 2), 1), 1), 1), 3), 9), 2), 2), 1), 1), 1), 1), 1), 1), 1), 2), 2), 2), 5), 12), 1), 3), 2), 1), 1), 1); // 119439360
static const long long L7_dig_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 9), 1), 5), 1), 1), 5), 2), 3), 2), 2), 1), 1), 1), 1), 1), 1), 4), 8), 2), 4), 15), 7), 4), 1), 1), 1), 2); // 3483648000
static const long long L7_dig_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 1), 1), 1), 7), 1), 3), 1), 4), 5), 5), 5), 4), 1), 1), 4); // 336000
static const long long L7_dig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 5), 3), 1), 1), 8), 2), 1), 1), 4), 2), 3), 3), 1), 1), 2), 3); // 103680
static const long long L7_dig_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 9), 2), 3), 1), 8), 3), 14), 4), 3), 1), 1), 2), 1); // 1306368
static const long long L7_dig_g_Volume = 1; // 1
static const long long L7_dig_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 3), 5), 2), 1), 1), 1), 1), 1), 1), 3), 1), 3), 1), 4), 1), 5), 1), 1), 1), 1); // 10800
static const long long L7_dig_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 1), 1), 3), 1), 1), 1), 1), 1), 1), 1), 2), 1), 1), 2), 3), 2), 2), 1), 1), 1), 1); // 576
static const long long L7_dig_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 2), 1), 1), 1), 2), 1), 1), 1), 1), 1), 2), 3), 3), 2), 4), 11), 2), 2), 1), 2), 3), 1); // 304128
static const long long L7_dig_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 5), 1), 3), 8), 1), 3), 2), 1), 1), 1), 1), 2), 5), 2), 2), 7), 1), 5), 1), 1), 1), 1); // 6048000

struct L7_dig_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L7_dig_A[tc_rows(L7_dig_a_Volume, L7_dig_hA) * L7_dig_ND];
  cm_cnt L7_dig_B[tc_rows(L7_dig_b_Volume, L7_dig_hB) * L7_dig_ND];
  cm_cnt L7_dig_C[L7_dig_c_Volume * L7_dig_ND];
  cm_cnt L7_dig_D[tc_rows(L7_dig_d_Volume, L7_dig_hD) * L7_dig_ND];
  cm_cnt L7_dig_MT[tc_rows(L7_dig_t_Volume, L7_dig_hT) * L7_dig_MND];
  i16 L7_dig_S[tc_rows(L7_dig_s_Volume, L7_dig_hS) * L7_dig_ND * L7_dig_qs];
  u8 L7_dig_SC[tc_rows(L7_dig_s_Volume, L7_dig_hS) * L7_dig_ND * L7_dig_qs];
  i16 L7_dig_F[tc_rows(L7_dig_f_Volume, L7_dig_hF) * L7_dig_ND * L7_dig_qf];
  u8 L7_dig_FC[tc_rows(L7_dig_f_Volume, L7_dig_hF) * L7_dig_ND * L7_dig_qf];
  i32 L7_dig_W[tc_rows(L7_dig_m_Volume, L7_dig_hM) * L7_dig_ND * TC_MIXN];
  u8 L7_dig_WC[tc_rows(L7_dig_m_Volume, L7_dig_hM) * L7_dig_ND];
  i32 L7_dig_WM[tc_rows(L7_dig_n_Volume, L7_dig_hN) * L7_dig_MND * 3];
  u8 L7_dig_WMC[tc_rows(L7_dig_n_Volume, L7_dig_hN) * L7_dig_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L7_dig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_dig_a_Volume, L7_dig_hA) * L7_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_dig_b_Volume, L7_dig_hB) * L7_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L7_dig_c_Volume * L7_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_dig_d_Volume, L7_dig_hD) * L7_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_dig_t_Volume, L7_dig_hT) * L7_dig_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_dig_s_Volume, L7_dig_hS) * L7_dig_ND * L7_dig_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_dig_s_Volume, L7_dig_hS) * L7_dig_ND * L7_dig_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_dig_f_Volume, L7_dig_hF) * L7_dig_ND * L7_dig_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_dig_f_Volume, L7_dig_hF) * L7_dig_ND * L7_dig_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_dig_m_Volume, L7_dig_hM) * L7_dig_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_dig_m_Volume, L7_dig_hM) * L7_dig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_dig_n_Volume, L7_dig_hN) * L7_dig_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_dig_n_Volume, L7_dig_hN) * L7_dig_MND);
  void L7_dig_Init( void ) {
  
  }
};
