

static const byte M5_dcs_b_ls[14+1]={ 0,1,2,3,4,5,5,6,6,7,7,7,7,7,7 };
static const byte M5_dcs_b_as[14+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,9,9 };

static const byte M5_dcs_c_qs[32+1]={ 0,0,0,0,1,1,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte M5_dcs_d_lbn[17+1]={ 0,1,2,2,2,2,3,4,4,4,4,4,5,5,5,5,6,6 };
static const byte M5_dcs_d_abn[17+1]={ 0,1,2,3,3,3,4,5,6,6,6,6,6,6,6,7,8,8 };



static const byte M5_dcs_h_sz[12+1]={ 0,0,1,2,3,4,4,5,6,6,6,6,6 };

static const byte M5_dcs_i_prev[13+1]={ 0,1,2,3,4,5,6,7,8,8,8,8,8,8 };

static const byte M5_dcs_j_ls[14+1]={ 0,0,1,2,3,4,5,6,7,8,8,8,8,8,8 };
static const byte M5_dcs_j_prev[13+1]={ 0,0,1,2,3,4,5,6,7,7,7,7,7,7 };

static const byte M5_dcs_k_as[14+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5,5 };




static const byte M5_dcs_t_prev[13+1]={ 0,1,1,1,1,2,3,4,5,5,5,5,5,5 };


static const int M5_dcs_rA = (0+0) * (1);
static const int M5_dcs_mwA = (255+0) * (1);
static const int M5_dcs_rB = (250+0) * (1);
static const int M5_dcs_mwB = (8+0) * (1);
static const int M5_dcs_rC = (252+0) * (1);
static const int M5_dcs_mwC = (64+0) * (1);
static const int M5_dcs_rD = (255+0) * (1);
static const int M5_dcs_mwD = (24+0) * (1);
static const int M5_dcs_rE = (255+0) * (1);
static const int M5_dcs_mwE = (87+0) * (1);
static const int M5_dcs_rH = (255+0) * (1);
static const int M5_dcs_mwH = (127+0) * (1);
static const int M5_dcs_rI = (255+0) * (1);
static const int M5_dcs_mwI = (111+0) * (1);
static const int M5_dcs_rJ = (250+0) * (1);
static const int M5_dcs_mwJ = (109+0) * (1);
static const int M5_dcs_rK = (254+0) * (1);
static const int M5_dcs_mwK = (128+0) * (1);
static const int M5_dcs_rL = (94+0) * (1);
static const int M5_dcs_mwL = (197+0) * (1);
static const int M5_dcs_lr = (31+0) * (1);
static const int M5_dcs_mb = (32+0) * (1);
static const int M5_dcs_rT = (254+0) * (1);
static const int M5_dcs_mwT = (128+0) * (1);
static const int M5_dcs_mt = (6+0) * (1);
static const int M5_dcs_lrm = (24+0) * (1);
static const int M5_dcs_mbm = (128+0) * (1);
static const int M5_dcs_rG = (0+0) * (1);
static const int M5_dcs_mwG = (0+0) * (1);
static const int M5_dcs_hA = (22+0) * (1);
static const int M5_dcs_hB = (22+0) * (1);
static const int M5_dcs_hC = (22+0) * (1);
static const int M5_dcs_hD = (22+0) * (1);
static const int M5_dcs_hE = (22+0) * (1);
static const int M5_dcs_hH = (22+0) * (1);
static const int M5_dcs_hI = (22+0) * (1);
static const int M5_dcs_hJ = (22+0) * (1);
static const int M5_dcs_hK = (22+0) * (1);
static const int M5_dcs_hL = (22+0) * (1);
static const int M5_dcs_hM = (22+0) * (1);
static const int M5_dcs_hT = (22+0) * (1);
static const int M5_dcs_hN = (22+0) * (1);


static const long long M5_dcs_a_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M5_dcs_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 10); // 160
static const long long M5_dcs_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 6); // 24
static const long long M5_dcs_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 9); // 126
static const long long M5_dcs_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3), 3); // 54
static const long long M5_dcs_g_Volume = 1; // 1
static const long long M5_dcs_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 3); // 42
static const long long M5_dcs_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 9), 2); // 36
static const long long M5_dcs_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 9), 8); // 144
static const long long M5_dcs_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 3); // 18
static const long long M5_dcs_l_Volume = tc_vmul(1, 2); // 2
static const long long M5_dcs_m_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long M5_dcs_n_Volume = tc_vmul(1, 1); // 1
static const long long M5_dcs_t_Volume = tc_vmul(tc_vmul(1, 2), 6); // 12

#define M5_dcs_USES(X) X(lc) X(ls) X(as) X(coded) X(qs) X(lbn) X(abn) X(ls2) X(as2) X(psg) X(sz) X(prev)

struct M5_dcs_T {
  cm_cnt2 M5_dcs_A[tc_rows(M5_dcs_a_Volume, M5_dcs_hA) * M_dcs_ND];
  cm_cnt2 M5_dcs_B[tc_rows(M5_dcs_b_Volume, M5_dcs_hB) * M_dcs_ND];
  cm_cnt2 M5_dcs_C[tc_rows(M5_dcs_c_Volume, M5_dcs_hC) * M_dcs_ND];
  cm_cnt2 M5_dcs_D[tc_rows(M5_dcs_d_Volume, M5_dcs_hD) * M_dcs_ND];
  cm_cnt2 M5_dcs_E[tc_rows(M5_dcs_e_Volume, M5_dcs_hE) * M_dcs_ND];
  cm_cnt2 M5_dcs_H[tc_rows(M5_dcs_h_Volume, M5_dcs_hH) * M_dcs_ND];
  cm_cnt2 M5_dcs_I[tc_rows(M5_dcs_i_Volume, M5_dcs_hI) * M_dcs_ND];
  cm_cnt2 M5_dcs_J[tc_rows(M5_dcs_j_Volume, M5_dcs_hJ) * M_dcs_ND];
  cm_cnt2 M5_dcs_K[tc_rows(M5_dcs_k_Volume, M5_dcs_hK) * M_dcs_ND];
  cm_cnt2 M5_dcs_L[tc_rows(M5_dcs_l_Volume, M5_dcs_hL) * M_dcs_ND];
  cm_cnt M5_dcs_MT[tc_rows(M5_dcs_t_Volume, M5_dcs_hT) * M_dcs_MND];
  i32 M5_dcs_W[tc_rows(M5_dcs_m_Volume, M5_dcs_hM) * M_dcs_ND * tc_nin(10)];
  u8 M5_dcs_WC[tc_rows(M5_dcs_m_Volume, M5_dcs_hM) * M_dcs_ND];
  i32 M5_dcs_WM[tc_rows(M5_dcs_n_Volume, M5_dcs_hN) * M_dcs_MND * TC_MIN];
  u8 M5_dcs_WMC[tc_rows(M5_dcs_n_Volume, M5_dcs_hN) * M_dcs_MND];
  cm_cnt M5_dcs_G[M5_dcs_g_Volume * M_dcs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_dcs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_a_Volume, M5_dcs_hA) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_b_Volume, M5_dcs_hB) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_c_Volume, M5_dcs_hC) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_d_Volume, M5_dcs_hD) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_e_Volume, M5_dcs_hE) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_h_Volume, M5_dcs_hH) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_i_Volume, M5_dcs_hI) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_j_Volume, M5_dcs_hJ) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_k_Volume, M5_dcs_hK) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_dcs_l_Volume, M5_dcs_hL) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_dcs_t_Volume, M5_dcs_hT) * M_dcs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_dcs_m_Volume, M5_dcs_hM) * M_dcs_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_dcs_m_Volume, M5_dcs_hM) * M_dcs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_dcs_n_Volume, M5_dcs_hN) * M_dcs_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_dcs_n_Volume, M5_dcs_hN) * M_dcs_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_dcs_g_Volume * M_dcs_NG);
  void M5_dcs_Init( void ) {}
  void M5_dcs_Quit( void ) {}
};
