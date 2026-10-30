
static const byte M5_mvg_a_kind[5+1]={ 0,0,1,2,2,3 };

static const byte M5_mvg_b_kind[5+1]={ 0,1,2,3,3,3 };
static const byte M5_mvg_b_dz[17+1]={ 0,0,0,1,1,2,3,4,5,6,7,8,8,9,9,10,10,10 };


static const byte M5_mvg_d_kind[5+1]={ 0,1,1,2,3,3 };

static const byte M5_mvg_e_kind[5+1]={ 0,0,1,2,2,3 };
static const byte M5_mvg_e_hm[3+1]={ 0,1,2,3 };


static const byte M5_mvg_h_kind[5+1]={ 0,1,1,2,2,3 };


static const byte M5_mvg_j_mag[16+1]={ 0,0,1,2,3,3,3,3,3,4,4,4,4,4,4,4,4 };


static const byte M5_mvg_l_kind[5+1]={ 0,0,1,1,2,3 };

static const byte M5_mvg_m_kind[5+1]={ 0,0,0,1,2,3 };




static const int M5_mvg_rA = (253+0) * (1);
static const int M5_mvg_mwA = (31+0) * (1);
static const int M5_mvg_rB = (0+0) * (1);
static const int M5_mvg_mwB = (92+0) * (1);
static const int M5_mvg_rC = (0+0) * (1);
static const int M5_mvg_mwC = (127+0) * (1);
static const int M5_mvg_rD = (135+0) * (1);
static const int M5_mvg_mwD = (0+0) * (1);
static const int M5_mvg_rE = (206+0) * (1);
static const int M5_mvg_mwE = (0+0) * (1);
static const int M5_mvg_rH = (44+0) * (1);
static const int M5_mvg_mwH = (64+0) * (1);
static const int M5_mvg_rI = (245+0) * (1);
static const int M5_mvg_mwI = (254+0) * (1);
static const int M5_mvg_rJ = (128+0) * (1);
static const int M5_mvg_mwJ = (0+0) * (1);
static const int M5_mvg_rK = (225+0) * (1);
static const int M5_mvg_mwK = (102+0) * (1);
static const int M5_mvg_rL = (0+0) * (1);
static const int M5_mvg_mwL = (107+0) * (1);
static const int M5_mvg_lr = (16+0) * (1);
static const int M5_mvg_mb = (128+0) * (1);
static const int M5_mvg_rT = (0+0) * (1);
static const int M5_mvg_mwT = (0+0) * (1);
static const int M5_mvg_mt = (0+0) * (1);
static const int M5_mvg_lrm = (0+0) * (1);
static const int M5_mvg_mbm = (0+0) * (1);
static const int M5_mvg_rG = (0+0) * (1);
static const int M5_mvg_mwG = (0+0) * (1);
static const int M5_mvg_hA = (22+0) * (1);
static const int M5_mvg_hB = (22+0) * (1);
static const int M5_mvg_hC = (22+0) * (1);
static const int M5_mvg_hD = (22+0) * (1);
static const int M5_mvg_hE = (22+0) * (1);
static const int M5_mvg_hH = (22+0) * (1);
static const int M5_mvg_hI = (22+0) * (1);
static const int M5_mvg_hJ = (22+0) * (1);
static const int M5_mvg_hK = (22+0) * (1);
static const int M5_mvg_hL = (22+0) * (1);
static const int M5_mvg_hM = (22+0) * (1);
static const int M5_mvg_hT = (22+0) * (1);
static const int M5_mvg_hN = (22+0) * (1);


static const long long M5_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3), 3); // 72
static const long long M5_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3), 11); // 264
static const long long M5_mvg_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 2); // 12
static const long long M5_mvg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 2); // 8
static const long long M5_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3), 4); // 48
static const long long M5_mvg_g_Volume = 1; // 1
static const long long M5_mvg_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3); // 12
static const long long M5_mvg_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M5_mvg_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 5); // 90
static const long long M5_mvg_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3), 3); // 81
static const long long M5_mvg_l_Volume = tc_vmul(tc_vmul(1, 4), 2); // 8
static const long long M5_mvg_m_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long M5_mvg_n_Volume = 1; // 1
static const long long M5_mvg_t_Volume = 1; // 1

#define M5_mvg_USES(X) X(kind) X(comp) X(ls) X(as) X(ds) X(dz) X(mag) X(hm)

struct M5_mvg_T {
  cm_cnt2 M5_mvg_A[tc_rows(M5_mvg_a_Volume, M5_mvg_hA) * M_mvg_ND];
  cm_cnt2 M5_mvg_B[tc_rows(M5_mvg_b_Volume, M5_mvg_hB) * M_mvg_ND];
  cm_cnt2 M5_mvg_C[tc_rows(M5_mvg_c_Volume, M5_mvg_hC) * M_mvg_ND];
  cm_cnt2 M5_mvg_D[tc_rows(M5_mvg_d_Volume, M5_mvg_hD) * M_mvg_ND];
  cm_cnt2 M5_mvg_E[tc_rows(M5_mvg_e_Volume, M5_mvg_hE) * M_mvg_ND];
  cm_cnt2 M5_mvg_H[tc_rows(M5_mvg_h_Volume, M5_mvg_hH) * M_mvg_ND];
  cm_cnt2 M5_mvg_I[tc_rows(M5_mvg_i_Volume, M5_mvg_hI) * M_mvg_ND];
  cm_cnt2 M5_mvg_J[tc_rows(M5_mvg_j_Volume, M5_mvg_hJ) * M_mvg_ND];
  cm_cnt2 M5_mvg_K[tc_rows(M5_mvg_k_Volume, M5_mvg_hK) * M_mvg_ND];
  cm_cnt2 M5_mvg_L[tc_rows(M5_mvg_l_Volume, M5_mvg_hL) * M_mvg_ND];
  cm_cnt M5_mvg_MT[tc_rows(M5_mvg_t_Volume, M5_mvg_hT) * M_mvg_MND];
  i32 M5_mvg_W[tc_rows(M5_mvg_m_Volume, M5_mvg_hM) * M_mvg_ND * tc_nin(10)];
  u8 M5_mvg_WC[tc_rows(M5_mvg_m_Volume, M5_mvg_hM) * M_mvg_ND];
  i32 M5_mvg_WM[tc_rows(M5_mvg_n_Volume, M5_mvg_hN) * M_mvg_MND * TC_MIN];
  u8 M5_mvg_WMC[tc_rows(M5_mvg_n_Volume, M5_mvg_hN) * M_mvg_MND];
  cm_cnt M5_mvg_G[M5_mvg_g_Volume * M_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_a_Volume, M5_mvg_hA) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_b_Volume, M5_mvg_hB) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_c_Volume, M5_mvg_hC) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_d_Volume, M5_mvg_hD) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_e_Volume, M5_mvg_hE) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_h_Volume, M5_mvg_hH) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_i_Volume, M5_mvg_hI) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_j_Volume, M5_mvg_hJ) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_k_Volume, M5_mvg_hK) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvg_l_Volume, M5_mvg_hL) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_mvg_t_Volume, M5_mvg_hT) * M_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mvg_m_Volume, M5_mvg_hM) * M_mvg_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mvg_m_Volume, M5_mvg_hM) * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mvg_n_Volume, M5_mvg_hN) * M_mvg_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mvg_n_Volume, M5_mvg_hN) * M_mvg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_mvg_g_Volume * M_mvg_NG);
  void M5_mvg_Init( void ) {}
  void M5_mvg_Quit( void ) {}
};
