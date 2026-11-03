
static const byte L3_flr_a_col[48+1]={ 0,0,1,1,2,2,2,3,3,3,3,4,5,6,6,7,8,9,10,11,11,11,11,12,13,14,15,15,15,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19 };
static const byte L3_flr_a_pri[32+1]={ 0,0,0,0,0,1,1,2,2,2,2,2,3,3,3,3,3,3,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte L3_flr_b_col[7+1]={ 0,0,1,2,2,2,2,3 };
static const byte L3_flr_b_p1[14+1]={ 0,0,0,0,0,1,2,3,3,4,5,6,7,7,7 };
static const byte L3_flr_b_od[9+1]={ 0,1,1,2,2,2,2,2,2,3 };
static const byte L3_flr_b_pri[32+1]={ 0,0,0,0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,3,3,4,4,4,5,5,5,5,5,5,5,5,5 };

static const byte L3_flr_c_o1[7+1]={ 0,1,2,2,2,2,3,4 };
static const byte L3_flr_c_od[9+1]={ 0,1,1,1,1,2,2,2,2,3 };
static const byte L3_flr_c_pri[32+1]={ 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3 };
static const byte L3_flr_c_ep[11+1]={ 0,0,0,1,2,3,4,4,5,6,6,7 };

static const byte L3_flr_d_pri[32+1]={ 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4 };
static const byte L3_flr_d_ep[11+1]={ 0,0,0,0,0,0,0,1,2,3,4,5 };

static const byte L3_flr_f_pri[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,3,4,4,5,5,6,6,6,6,6,6,6,6,6,6 };
static const byte L3_flr_f_ep[11+1]={ 0,0,0,0,0,0,0,1,1,2,3,3 };


static const byte L3_flr_m_col[15+1]={ 0,0,0,0,1,1,1,2,2,2,3,4,5,6,7,7 };
static const byte L3_flr_m_pri[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,5,5,5,5,6,7,7,7,7,7,7,7,7,7 };

static const byte L3_flr_n_col[48+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte L3_flr_n_p1[3+1]={ 0,1,2,3 };
static const byte L3_flr_n_ep[11+1]={ 0,0,1,2,3,4,4,5,6,6,7,8 };

static const byte L3_flr_s_col[7+1]={ 0,1,2,2,2,2,2,3 };
static const byte L3_flr_s_lov[8+1]={ 0,1,1,1,1,1,2,2,3 };
static const byte L3_flr_s_pri[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,8,9,10,11,11,11,11,11,12,12,12,12,12 };

static const byte L3_flr_t_col[48+1]={ 0,0,0,1,1,1,1,1,2,2,2,3,4,5,5,6,7,8,9,10,10,10,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,13,13,13 };
static const byte L3_flr_t_pri[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,3,3,4,4,4,4,4,4,4,4,4 };


static const int L3_flr_rA = (12+0) * (1);
static const int L3_flr_rB = (233+0) * (1);
static const int L3_flr_rC = (90+0) * (1);
static const int L3_flr_rD = (112+0) * (1);
static const int L3_flr_rT = (38+0) * (1);
static const int L3_flr_rG = (4+0) * (1);
static const int L3_flr_rS1 = (6+0) * (1);
static const int L3_flr_rS2 = (12+0) * (1);
static const int L3_flr_lr = (17+0) * (1);
static const int L3_flr_lrm = (28+0) * (1);
static const int L3_flr_mbm = (65+0) * (1);
static const int L3_flr_mwA = (33+0) * (1);
static const int L3_flr_mwB = (0+0) * (1);
static const int L3_flr_mwC = (0+0) * (1);
static const int L3_flr_mwD = (0+0) * (1);
static const int L3_flr_mwT = (2+0) * (1);
static const int L3_flr_mwG = (0+0) * (1);
static const int L3_flr_bw = (11+0) * (1);
static const int L3_flr_qs = (28+3) * (1);
static const int L3_flr_qf = (33+3) * (1);
static const int L3_flr_mb = (19+0) * (1);
static const int L3_flr_hA = (31+0) * (1);
static const int L3_flr_hB = (14+0) * (1);
static const int L3_flr_hD = (31+0) * (1);
static const int L3_flr_hS = (13+0) * (1);
static const int L3_flr_hF = (14+0) * (1);
static const int L3_flr_hM = (31+0) * (1);
static const int L3_flr_hT = (31+0) * (1);
static const int L3_flr_hN = (31+0) * (1);


static const long long L3_flr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 20), 1), 1), 1), 1), 1), 2), 1), 1), 1), 3), 1), 1), 6), 1); // 1440
static const long long L3_flr_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 8), 1), 1), 2), 1), 2), 4), 2), 1), 1), 3), 2), 6), 3); // 110592
static const long long L3_flr_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 1), 5), 1), 4), 2), 1), 1), 1), 2), 4), 8); // 7680
static const long long L3_flr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 2), 1), 3), 1), 1), 1), 3), 1), 5), 6); // 540
static const long long L3_flr_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 3), 2), 3), 3), 1), 1), 2), 2), 3), 7), 4); // 54432
static const long long L3_flr_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1); // 1
static const long long L3_flr_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 8), 1), 1), 1), 1), 1), 1), 1), 1), 2), 2), 8), 1); // 256
static const long long L3_flr_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 4), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 9); // 180
static const long long L3_flr_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3), 1), 1), 3), 1), 1), 2), 4), 2), 13), 3); // 22464
static const long long L3_flr_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 1), 1), 1), 1), 1), 1), 1), 2), 1), 1), 5), 2); // 280

struct L3_flr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L3_flr_A[tc_rows(L3_flr_a_Volume, L3_flr_hA) * L3_flr_ND];
  cm_cnt L3_flr_B[tc_rows(L3_flr_b_Volume, L3_flr_hB) * L3_flr_ND];
  cm_cnt L3_flr_C[L3_flr_c_Volume * L3_flr_ND];
  cm_cnt L3_flr_D[tc_rows(L3_flr_d_Volume, L3_flr_hD) * L3_flr_ND];
  cm_cnt L3_flr_MT[tc_rows(L3_flr_t_Volume, L3_flr_hT) * L3_flr_MND];
  i16 L3_flr_S[tc_rows(L3_flr_s_Volume, L3_flr_hS) * L3_flr_ND * L3_flr_qs];
  u8 L3_flr_SC[tc_rows(L3_flr_s_Volume, L3_flr_hS) * L3_flr_ND * L3_flr_qs];
  i16 L3_flr_F[tc_rows(L3_flr_f_Volume, L3_flr_hF) * L3_flr_ND * L3_flr_qf];
  u8 L3_flr_FC[tc_rows(L3_flr_f_Volume, L3_flr_hF) * L3_flr_ND * L3_flr_qf];
  i32 L3_flr_W[tc_rows(L3_flr_m_Volume, L3_flr_hM) * L3_flr_ND * TC_MIXN];
  u8 L3_flr_WC[tc_rows(L3_flr_m_Volume, L3_flr_hM) * L3_flr_ND];
  i32 L3_flr_WM[tc_rows(L3_flr_n_Volume, L3_flr_hN) * L3_flr_MND * 3];
  u8 L3_flr_WMC[tc_rows(L3_flr_n_Volume, L3_flr_hN) * L3_flr_MND];
  cm_cnt L3_flr_G[L3_flr_g_Volume * TC_SGN_FLR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L3_flr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_flr_a_Volume, L3_flr_hA) * L3_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_flr_b_Volume, L3_flr_hB) * L3_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_flr_c_Volume * L3_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_flr_d_Volume, L3_flr_hD) * L3_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_flr_t_Volume, L3_flr_hT) * L3_flr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_flr_s_Volume, L3_flr_hS) * L3_flr_ND * L3_flr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_flr_s_Volume, L3_flr_hS) * L3_flr_ND * L3_flr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_flr_f_Volume, L3_flr_hF) * L3_flr_ND * L3_flr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_flr_f_Volume, L3_flr_hF) * L3_flr_ND * L3_flr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_flr_m_Volume, L3_flr_hM) * L3_flr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_flr_m_Volume, L3_flr_hM) * L3_flr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_flr_n_Volume, L3_flr_hN) * L3_flr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_flr_n_Volume, L3_flr_hN) * L3_flr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_flr_g_Volume * TC_SGN_FLR);
  void L3_flr_Init( void ) {
  
  }
};
