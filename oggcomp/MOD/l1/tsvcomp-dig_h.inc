
static const byte L1_dig_a_pass[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte L1_dig_a_band[15+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,9,9,9 };
static const byte L1_dig_a_col[7+1]={ 0,0,1,1,2,2,3,4 };
static const byte L1_dig_a_bkq[4+1]={ 0,0,1,2,3 };
static const byte L1_dig_a_cls[15+1]={ 0,1,1,2,3,4,5,6,7,8,8,8,8,8,8,8 };

static const byte L1_dig_b_band[15+1]={ 0,1,2,2,3,4,4,4,5,5,6,7,8,9,10,11 };
static const byte L1_dig_b_col[7+1]={ 0,1,2,2,2,3,3,4 };
static const byte L1_dig_b_q1[5+1]={ 0,1,2,3,4,5 };
static const byte L1_dig_b_ax[12+1]={ 0,0,1,1,1,1,1,1,1,1,2,3,3 };
static const byte L1_dig_b_pq[32+1]={ 0,1,1,1,1,1,2,2,2,3,4,4,5,5,6,6,7,8,9,10,10,10,11,11,12,12,12,13,13,14,14,14,14 };
static const byte L1_dig_b_ps[13+1]={ 0,1,1,1,1,2,2,3,4,4,5,6,6,6 };
static const byte L1_dig_b_chn[7+1]={ 0,1,2,2,2,3,3,3 };

static const byte L1_dig_c_ax[12+1]={ 0,0,0,0,0,0,0,0,0,1,2,3,4 };
static const byte L1_dig_c_p0s[13+1]={ 0,0,0,0,0,0,0,0,0,0,1,1,2,3 };
static const byte L1_dig_c_ps[13+1]={ 0,0,0,0,0,1,1,1,2,3,4,4,4,4 };

static const byte L1_dig_d_t1s[13+1]={ 0,1,1,1,2,2,3,3,3,3,3,3,3,3 };
static const byte L1_dig_d_ax[12+1]={ 0,0,0,0,1,1,2,3,4,4,4,5,5 };
static const byte L1_dig_d_p0s[13+1]={ 0,0,1,1,1,1,1,1,2,2,3,3,3,3 };
static const byte L1_dig_d_x1s[13+1]={ 0,0,0,1,1,2,2,2,2,2,3,3,3,3 };

static const byte L1_dig_f_cls[9+1]={ 0,0,0,1,2,2,3,4,5,6 };
static const byte L1_dig_f_ax[12+1]={ 0,1,2,2,2,3,3,4,5,6,6,6,6 };
static const byte L1_dig_f_band[5+1]={ 0,1,2,3,4,5 };
static const byte L1_dig_f_pq[32+1]={ 0,1,2,3,3,3,3,3,4,4,5,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };
static const byte L1_dig_f_x1s[13+1]={ 0,0,0,0,0,1,1,1,2,2,2,2,3,3 };


static const byte L1_dig_m_cls[15+1]={ 0,1,1,2,2,2,3,4,5,6,6,7,7,7,7,7 };
static const byte L1_dig_m_p0[3+1]={ 0,1,2,3 };
static const byte L1_dig_m_pq[32+1]={ 0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,3,3,3,4,4,4,4 };
static const byte L1_dig_m_chn[7+1]={ 0,1,1,2,2,3,3,4 };

static const byte L1_dig_n_pass[3+1]={ 0,1,2,3 };
static const byte L1_dig_n_cls[15+1]={ 0,1,1,1,2,2,2,3,4,5,5,5,5,5,5,6 };

static const byte L1_dig_s_pq[32+1]={ 0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,4,4,4,5,5,5,5,5,5,5,6,6,6,6,6,6 };
static const byte L1_dig_s_x1s[13+1]={ 0,1,2,3,3,4,4,5,5,5,5,5,6,6 };

static const byte L1_dig_t_cls[15+1]={ 0,1,1,2,2,2,2,3,4,5,6,6,6,6,6,6 };
static const byte L1_dig_t_p0s[13+1]={ 0,0,1,1,2,3,3,3,3,3,3,3,3,4 };
static const byte L1_dig_t_ax[12+1]={ 0,0,1,2,3,3,3,4,4,5,5,5,5 };
static const byte L1_dig_t_pq[32+1]={ 0,1,1,1,1,1,1,2,2,2,2,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6 };
static const byte L1_dig_t_chn[7+1]={ 0,1,1,2,2,3,3,4 };


static const int L1_dig_avD = (238+0) * (1);
static const int L1_dig_avA = (7+0) * (1);
static const int L1_dig_avT1 = (2+0) * (1);
static const int L1_dig_avT2 = (1+0) * (1);
static const int L1_dig_avN1 = (5+0) * (1);
static const int L1_dig_avW1 = (3+0) * (1);
static const int L1_dig_avQ1 = (7+0) * (1);
static const int L1_dig_avQ2 = (3+0) * (1);
static const int L1_dig_avX1 = (1+0) * (1);
static const int L1_dig_avC = (356+0) * (1);
static const int L1_dig_rA = (253+0) * (1);
static const int L1_dig_rB = (252+0) * (1);
static const int L1_dig_rC = (16+0) * (1);
static const int L1_dig_rD = (107+0) * (1);
static const int L1_dig_rT = (247+0) * (1);
static const int L1_dig_rS1 = (15+0) * (1);
static const int L1_dig_rS2 = (7+0) * (1);
static const int L1_dig_lr = (10+0) * (1);
static const int L1_dig_lrm = (27+0) * (1);
static const int L1_dig_mbm = (68+0) * (1);
static const int L1_dig_mwA = (1+0) * (1);
static const int L1_dig_mwB = (58+0) * (1);
static const int L1_dig_mwC = (116+0) * (1);
static const int L1_dig_mwD = (126+0) * (1);
static const int L1_dig_mwT = (0+0) * (1);
static const int L1_dig_bw = (11+0) * (1);
static const int L1_dig_qs = (22+3) * (1);
static const int L1_dig_qf = (42+3) * (1);
static const int L1_dig_mb = (75+0) * (1);
static const int L1_dig_hA = (17+0) * (1);
static const int L1_dig_hB = (16+0) * (1);
static const int L1_dig_hD = (17+0) * (1);
static const int L1_dig_hS = (15+0) * (1);
static const int L1_dig_hF = (15+0) * (1);
static const int L1_dig_hM = (31+0) * (1);
static const int L1_dig_hT = (14+0) * (1);
static const int L1_dig_hN = (31+0) * (1);


static const long long L1_dig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 10), 5), 1), 1), 1), 4), 9), 2), 2), 1), 1), 1), 1), 1), 1), 2), 2), 2), 2), 3), 3), 1), 3), 2), 1), 1), 3); // 149299200
static const long long L1_dig_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 12), 5), 6), 1), 1), 2), 2), 3), 2), 1), 1), 1), 1), 1), 1), 1), 2), 2), 2), 4), 15), 7), 4), 1), 1), 1), 1); // 464486400
static const long long L1_dig_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 1), 1), 1), 5), 1), 3), 1), 4), 2), 5), 1), 1), 1), 1), 1); // 1200
static const long long L1_dig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 4), 2), 1), 1), 6), 3), 1), 1), 4), 2), 2), 3), 1), 1), 2), 4); // 55296
static const long long L1_dig_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 7), 3), 2), 1), 7), 6), 9), 1), 3), 3), 1), 2), 4); // 3429216
static const long long L1_dig_g_Volume = 1; // 1
static const long long L1_dig_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 2), 8), 2), 1), 1), 1), 1), 1), 1), 4), 1), 3), 1), 5), 1), 5), 1), 1), 1), 1); // 19200
static const long long L1_dig_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 1), 1), 1), 7), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 2), 1), 2), 2), 1), 1), 1), 1); // 448
static const long long L1_dig_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 3), 1), 3), 1), 2), 1), 1), 1), 1), 1), 2), 1), 2), 3), 2), 7), 2), 2), 1), 2), 2), 7); // 1016064
static const long long L1_dig_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 1), 1), 7), 1), 2), 2), 1), 1), 1), 1), 2), 5), 1), 6), 7), 1), 5), 1), 1), 1), 1); // 1058400

struct L1_dig_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L1_dig_A[tc_rows(L1_dig_a_Volume, L1_dig_hA) * L1_dig_ND];
  cm_cnt L1_dig_B[tc_rows(L1_dig_b_Volume, L1_dig_hB) * L1_dig_ND];
  cm_cnt L1_dig_C[L1_dig_c_Volume * L1_dig_ND];
  cm_cnt L1_dig_D[tc_rows(L1_dig_d_Volume, L1_dig_hD) * L1_dig_ND];
  cm_cnt L1_dig_MT[tc_rows(L1_dig_t_Volume, L1_dig_hT) * L1_dig_MND];
  i16 L1_dig_S[tc_rows(L1_dig_s_Volume, L1_dig_hS) * L1_dig_ND * L1_dig_qs];
  u8 L1_dig_SC[tc_rows(L1_dig_s_Volume, L1_dig_hS) * L1_dig_ND * L1_dig_qs];
  i16 L1_dig_F[tc_rows(L1_dig_f_Volume, L1_dig_hF) * L1_dig_ND * L1_dig_qf];
  u8 L1_dig_FC[tc_rows(L1_dig_f_Volume, L1_dig_hF) * L1_dig_ND * L1_dig_qf];
  i32 L1_dig_W[tc_rows(L1_dig_m_Volume, L1_dig_hM) * L1_dig_ND * TC_MIXN];
  u8 L1_dig_WC[tc_rows(L1_dig_m_Volume, L1_dig_hM) * L1_dig_ND];
  i32 L1_dig_WM[tc_rows(L1_dig_n_Volume, L1_dig_hN) * L1_dig_MND * 3];
  u8 L1_dig_WMC[tc_rows(L1_dig_n_Volume, L1_dig_hN) * L1_dig_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L1_dig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_dig_a_Volume, L1_dig_hA) * L1_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_dig_b_Volume, L1_dig_hB) * L1_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L1_dig_c_Volume * L1_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_dig_d_Volume, L1_dig_hD) * L1_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_dig_t_Volume, L1_dig_hT) * L1_dig_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L1_dig_s_Volume, L1_dig_hS) * L1_dig_ND * L1_dig_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_dig_s_Volume, L1_dig_hS) * L1_dig_ND * L1_dig_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L1_dig_f_Volume, L1_dig_hF) * L1_dig_ND * L1_dig_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_dig_f_Volume, L1_dig_hF) * L1_dig_ND * L1_dig_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L1_dig_m_Volume, L1_dig_hM) * L1_dig_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_dig_m_Volume, L1_dig_hM) * L1_dig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L1_dig_n_Volume, L1_dig_hN) * L1_dig_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_dig_n_Volume, L1_dig_hN) * L1_dig_MND);
  void L1_dig_Init( void ) {
  
  }
};
