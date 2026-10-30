
static const byte M5_esc_a_dm[3+1]={ 0,1,2,3 };


static const byte M5_esc_c_dm[3+1]={ 0,1,2,3 };


static const byte M5_esc_e_dm[3+1]={ 0,1,2,3 };



static const byte M5_esc_i_dm[3+1]={ 0,1,2,3 };


static const byte M5_esc_k_dm[3+1]={ 0,1,2,3 };


static const byte M5_esc_m_dm[3+1]={ 0,1,2,3 };




static const int M5_esc_rA = (255+0) * (1);
static const int M5_esc_mwA = (23+0) * (1);
static const int M5_esc_rB = (255+0) * (1);
static const int M5_esc_mwB = (12+0) * (1);
static const int M5_esc_rC = (252+0) * (1);
static const int M5_esc_mwC = (15+0) * (1);
static const int M5_esc_rD = (255+0) * (1);
static const int M5_esc_mwD = (224+0) * (1);
static const int M5_esc_rE = (234+0) * (1);
static const int M5_esc_mwE = (8+0) * (1);
static const int M5_esc_rH = (246+0) * (1);
static const int M5_esc_mwH = (64+0) * (1);
static const int M5_esc_rI = (130+0) * (1);
static const int M5_esc_mwI = (32+0) * (1);
static const int M5_esc_rJ = (254+0) * (1);
static const int M5_esc_mwJ = (34+0) * (1);
static const int M5_esc_rK = (221+0) * (1);
static const int M5_esc_mwK = (49+0) * (1);
static const int M5_esc_rL = (244+0) * (1);
static const int M5_esc_mwL = (17+0) * (1);
static const int M5_esc_lr = (24+0) * (1);
static const int M5_esc_mb = (15+0) * (1);
static const int M5_esc_rT = (0+0) * (1);
static const int M5_esc_mwT = (0+0) * (1);
static const int M5_esc_mt = (0+0) * (1);
static const int M5_esc_lrm = (0+0) * (1);
static const int M5_esc_mbm = (0+0) * (1);
static const int M5_esc_rG = (0+0) * (1);
static const int M5_esc_mwG = (0+0) * (1);
static const int M5_esc_hA = (22+0) * (1);
static const int M5_esc_hB = (22+0) * (1);
static const int M5_esc_hC = (22+0) * (1);
static const int M5_esc_hD = (22+0) * (1);
static const int M5_esc_hE = (22+0) * (1);
static const int M5_esc_hH = (22+0) * (1);
static const int M5_esc_hI = (22+0) * (1);
static const int M5_esc_hJ = (22+0) * (1);
static const int M5_esc_hK = (22+0) * (1);
static const int M5_esc_hL = (22+0) * (1);
static const int M5_esc_hM = (22+0) * (1);
static const int M5_esc_hT = (22+0) * (1);
static const int M5_esc_hN = (22+0) * (1);


static const long long M5_esc_a_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_b_Volume = 1; // 1
static const long long M5_esc_c_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_d_Volume = 1; // 1
static const long long M5_esc_e_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_g_Volume = 1; // 1
static const long long M5_esc_h_Volume = 1; // 1
static const long long M5_esc_i_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_j_Volume = 1; // 1
static const long long M5_esc_k_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_l_Volume = 1; // 1
static const long long M5_esc_m_Volume = tc_vmul(1, 4); // 4
static const long long M5_esc_n_Volume = 1; // 1
static const long long M5_esc_t_Volume = 1; // 1

#define M5_esc_USES(X) X(dm)

struct M5_esc_T {
  cm_cnt2 M5_esc_A[tc_rows(M5_esc_a_Volume, M5_esc_hA) * M_esc_ND];
  cm_cnt2 M5_esc_B[tc_rows(M5_esc_b_Volume, M5_esc_hB) * M_esc_ND];
  cm_cnt2 M5_esc_C[tc_rows(M5_esc_c_Volume, M5_esc_hC) * M_esc_ND];
  cm_cnt2 M5_esc_D[tc_rows(M5_esc_d_Volume, M5_esc_hD) * M_esc_ND];
  cm_cnt2 M5_esc_E[tc_rows(M5_esc_e_Volume, M5_esc_hE) * M_esc_ND];
  cm_cnt2 M5_esc_H[tc_rows(M5_esc_h_Volume, M5_esc_hH) * M_esc_ND];
  cm_cnt2 M5_esc_I[tc_rows(M5_esc_i_Volume, M5_esc_hI) * M_esc_ND];
  cm_cnt2 M5_esc_J[tc_rows(M5_esc_j_Volume, M5_esc_hJ) * M_esc_ND];
  cm_cnt2 M5_esc_K[tc_rows(M5_esc_k_Volume, M5_esc_hK) * M_esc_ND];
  cm_cnt2 M5_esc_L[tc_rows(M5_esc_l_Volume, M5_esc_hL) * M_esc_ND];
  cm_cnt M5_esc_MT[tc_rows(M5_esc_t_Volume, M5_esc_hT) * M_esc_MND];
  i32 M5_esc_W[tc_rows(M5_esc_m_Volume, M5_esc_hM) * M_esc_ND * tc_nin(10)];
  u8 M5_esc_WC[tc_rows(M5_esc_m_Volume, M5_esc_hM) * M_esc_ND];
  i32 M5_esc_WM[tc_rows(M5_esc_n_Volume, M5_esc_hN) * M_esc_MND * TC_MIN];
  u8 M5_esc_WMC[tc_rows(M5_esc_n_Volume, M5_esc_hN) * M_esc_MND];
  cm_cnt M5_esc_G[M5_esc_g_Volume * M_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_a_Volume, M5_esc_hA) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_b_Volume, M5_esc_hB) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_c_Volume, M5_esc_hC) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_d_Volume, M5_esc_hD) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_e_Volume, M5_esc_hE) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_h_Volume, M5_esc_hH) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_i_Volume, M5_esc_hI) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_j_Volume, M5_esc_hJ) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_k_Volume, M5_esc_hK) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_esc_l_Volume, M5_esc_hL) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_esc_t_Volume, M5_esc_hT) * M_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_esc_m_Volume, M5_esc_hM) * M_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_esc_m_Volume, M5_esc_hM) * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_esc_n_Volume, M5_esc_hN) * M_esc_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_esc_n_Volume, M5_esc_hN) * M_esc_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_esc_g_Volume * M_esc_NG);
  void M5_esc_Init( void ) {}
  void M5_esc_Quit( void ) {}
};
