


static const byte M5_mbt_c_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };




static const byte M5_mbt_h_fsn[3+1]={ 0,1,2,3 };

static const byte M5_mbt_i_c4[4+1]={ 0,1,2,3,4 };

static const byte M5_mbt_j_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };







static const int M5_mbt_rA = (31+0) * (1);
static const int M5_mbt_mwA = (124+0) * (1);
static const int M5_mbt_rB = (0+0) * (1);
static const int M5_mbt_mwB = (102+0) * (1);
static const int M5_mbt_rC = (27+0) * (1);
static const int M5_mbt_mwC = (128+0) * (1);
static const int M5_mbt_rD = (255+0) * (1);
static const int M5_mbt_mwD = (10+0) * (1);
static const int M5_mbt_rE = (0+0) * (1);
static const int M5_mbt_mwE = (197+0) * (1);
static const int M5_mbt_rH = (0+0) * (1);
static const int M5_mbt_mwH = (255+0) * (1);
static const int M5_mbt_rI = (253+0) * (1);
static const int M5_mbt_mwI = (78+0) * (1);
static const int M5_mbt_rJ = (253+0) * (1);
static const int M5_mbt_mwJ = (1+0) * (1);
static const int M5_mbt_rK = (253+0) * (1);
static const int M5_mbt_mwK = (36+0) * (1);
static const int M5_mbt_rL = (171+0) * (1);
static const int M5_mbt_mwL = (0+0) * (1);
static const int M5_mbt_lr = (12+0) * (1);
static const int M5_mbt_mb = (15+0) * (1);
static const int M5_mbt_rT = (0+0) * (1);
static const int M5_mbt_mwT = (0+0) * (1);
static const int M5_mbt_mt = (0+0) * (1);
static const int M5_mbt_lrm = (0+0) * (1);
static const int M5_mbt_mbm = (0+0) * (1);
static const int M5_mbt_rG = (0+0) * (1);
static const int M5_mbt_mwG = (0+0) * (1);
static const int M5_mbt_hA = (22+0) * (1);
static const int M5_mbt_hB = (22+0) * (1);
static const int M5_mbt_hC = (22+0) * (1);
static const int M5_mbt_hD = (22+0) * (1);
static const int M5_mbt_hE = (22+0) * (1);
static const int M5_mbt_hH = (22+0) * (1);
static const int M5_mbt_hI = (22+0) * (1);
static const int M5_mbt_hJ = (22+0) * (1);
static const int M5_mbt_hK = (22+0) * (1);
static const int M5_mbt_hL = (22+0) * (1);
static const int M5_mbt_hM = (22+0) * (1);
static const int M5_mbt_hT = (22+0) * (1);
static const int M5_mbt_hN = (22+0) * (1);


static const long long M5_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 3); // 6
static const long long M5_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 1); // 3
static const long long M5_mbt_c_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M5_mbt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 1); // 3
static const long long M5_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1); // 2
static const long long M5_mbt_g_Volume = 1; // 1
static const long long M5_mbt_h_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long M5_mbt_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 5); // 30
static const long long M5_mbt_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 3); // 15
static const long long M5_mbt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3); // 18
static const long long M5_mbt_l_Volume = tc_vmul(1, 1); // 1
static const long long M5_mbt_m_Volume = tc_vmul(1, 1); // 1
static const long long M5_mbt_n_Volume = 1; // 1
static const long long M5_mbt_t_Volume = 1; // 1

#define M5_mbt_USES(X) X(st) X(rt) X(rc) X(cd) X(fsn) X(cl) X(c4)

struct M5_mbt_T {
  cm_cnt2 M5_mbt_A[tc_rows(M5_mbt_a_Volume, M5_mbt_hA) * M_mbt_ND];
  cm_cnt2 M5_mbt_B[tc_rows(M5_mbt_b_Volume, M5_mbt_hB) * M_mbt_ND];
  cm_cnt2 M5_mbt_C[tc_rows(M5_mbt_c_Volume, M5_mbt_hC) * M_mbt_ND];
  cm_cnt2 M5_mbt_D[tc_rows(M5_mbt_d_Volume, M5_mbt_hD) * M_mbt_ND];
  cm_cnt2 M5_mbt_E[tc_rows(M5_mbt_e_Volume, M5_mbt_hE) * M_mbt_ND];
  cm_cnt2 M5_mbt_H[tc_rows(M5_mbt_h_Volume, M5_mbt_hH) * M_mbt_ND];
  cm_cnt2 M5_mbt_I[tc_rows(M5_mbt_i_Volume, M5_mbt_hI) * M_mbt_ND];
  cm_cnt2 M5_mbt_J[tc_rows(M5_mbt_j_Volume, M5_mbt_hJ) * M_mbt_ND];
  cm_cnt2 M5_mbt_K[tc_rows(M5_mbt_k_Volume, M5_mbt_hK) * M_mbt_ND];
  cm_cnt2 M5_mbt_L[tc_rows(M5_mbt_l_Volume, M5_mbt_hL) * M_mbt_ND];
  cm_cnt M5_mbt_MT[tc_rows(M5_mbt_t_Volume, M5_mbt_hT) * M_mbt_MND];
  i32 M5_mbt_W[tc_rows(M5_mbt_m_Volume, M5_mbt_hM) * M_mbt_ND * tc_nin(10)];
  u8 M5_mbt_WC[tc_rows(M5_mbt_m_Volume, M5_mbt_hM) * M_mbt_ND];
  i32 M5_mbt_WM[tc_rows(M5_mbt_n_Volume, M5_mbt_hN) * M_mbt_MND * TC_MIN];
  u8 M5_mbt_WMC[tc_rows(M5_mbt_n_Volume, M5_mbt_hN) * M_mbt_MND];
  cm_cnt M5_mbt_G[M5_mbt_g_Volume * M_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_a_Volume, M5_mbt_hA) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_b_Volume, M5_mbt_hB) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_c_Volume, M5_mbt_hC) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_d_Volume, M5_mbt_hD) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_e_Volume, M5_mbt_hE) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_h_Volume, M5_mbt_hH) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_i_Volume, M5_mbt_hI) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_j_Volume, M5_mbt_hJ) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_k_Volume, M5_mbt_hK) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mbt_l_Volume, M5_mbt_hL) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_mbt_t_Volume, M5_mbt_hT) * M_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mbt_m_Volume, M5_mbt_hM) * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mbt_m_Volume, M5_mbt_hM) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mbt_n_Volume, M5_mbt_hN) * M_mbt_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mbt_n_Volume, M5_mbt_hN) * M_mbt_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_mbt_g_Volume * M_mbt_NG);
  void M5_mbt_Init( void ) {}
  void M5_mbt_Quit( void ) {}
};
