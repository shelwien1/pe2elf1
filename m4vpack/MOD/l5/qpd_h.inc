

static const byte M5_qpd_b_qs[32+1]={ 0,0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };


static const byte M5_qpd_d_qs[32+1]={ 0,0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };

static const byte M5_qpd_e_qs[32+1]={ 0,0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };



static const byte M5_qpd_i_qs[32+1]={ 0,0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };



static const byte M5_qpd_l_qs[32+1]={ 0,0,0,1,1,2,2,3,3,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };





static const int M5_qpd_rA = (255+0) * (1);
static const int M5_qpd_mwA = (23+0) * (1);
static const int M5_qpd_rB = (255+0) * (1);
static const int M5_qpd_mwB = (12+0) * (1);
static const int M5_qpd_rC = (252+0) * (1);
static const int M5_qpd_mwC = (15+0) * (1);
static const int M5_qpd_rD = (255+0) * (1);
static const int M5_qpd_mwD = (224+0) * (1);
static const int M5_qpd_rE = (234+0) * (1);
static const int M5_qpd_mwE = (8+0) * (1);
static const int M5_qpd_rH = (246+0) * (1);
static const int M5_qpd_mwH = (64+0) * (1);
static const int M5_qpd_rI = (130+0) * (1);
static const int M5_qpd_mwI = (32+0) * (1);
static const int M5_qpd_rJ = (254+0) * (1);
static const int M5_qpd_mwJ = (34+0) * (1);
static const int M5_qpd_rK = (221+0) * (1);
static const int M5_qpd_mwK = (49+0) * (1);
static const int M5_qpd_rL = (244+0) * (1);
static const int M5_qpd_mwL = (17+0) * (1);
static const int M5_qpd_lr = (24+0) * (1);
static const int M5_qpd_mb = (15+0) * (1);
static const int M5_qpd_rT = (0+0) * (1);
static const int M5_qpd_mwT = (0+0) * (1);
static const int M5_qpd_mt = (0+0) * (1);
static const int M5_qpd_lrm = (0+0) * (1);
static const int M5_qpd_mbm = (0+0) * (1);
static const int M5_qpd_rG = (0+0) * (1);
static const int M5_qpd_mwG = (0+0) * (1);
static const int M5_qpd_hA = (22+0) * (1);
static const int M5_qpd_hB = (22+0) * (1);
static const int M5_qpd_hC = (22+0) * (1);
static const int M5_qpd_hD = (22+0) * (1);
static const int M5_qpd_hE = (22+0) * (1);
static const int M5_qpd_hH = (22+0) * (1);
static const int M5_qpd_hI = (22+0) * (1);
static const int M5_qpd_hJ = (22+0) * (1);
static const int M5_qpd_hK = (22+0) * (1);
static const int M5_qpd_hL = (22+0) * (1);
static const int M5_qpd_hM = (22+0) * (1);
static const int M5_qpd_hT = (22+0) * (1);
static const int M5_qpd_hN = (22+0) * (1);


static const long long M5_qpd_a_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M5_qpd_b_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long M5_qpd_c_Volume = tc_vmul(1, 3); // 3
static const long long M5_qpd_d_Volume = tc_vmul(tc_vmul(1, 2), 8); // 16
static const long long M5_qpd_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 8); // 48
static const long long M5_qpd_g_Volume = 1; // 1
static const long long M5_qpd_h_Volume = tc_vmul(1, 2); // 2
static const long long M5_qpd_i_Volume = tc_vmul(1, 8); // 8
static const long long M5_qpd_j_Volume = 1; // 1
static const long long M5_qpd_k_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M5_qpd_l_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long M5_qpd_m_Volume = tc_vmul(1, 3); // 3
static const long long M5_qpd_n_Volume = 1; // 1
static const long long M5_qpd_t_Volume = 1; // 1

#define M5_qpd_USES(X) X(c3) X(pdq) X(qs)

struct M5_qpd_T {
  cm_cnt2 M5_qpd_A[tc_rows(M5_qpd_a_Volume, M5_qpd_hA) * M_qpd_ND];
  cm_cnt2 M5_qpd_B[tc_rows(M5_qpd_b_Volume, M5_qpd_hB) * M_qpd_ND];
  cm_cnt2 M5_qpd_C[tc_rows(M5_qpd_c_Volume, M5_qpd_hC) * M_qpd_ND];
  cm_cnt2 M5_qpd_D[tc_rows(M5_qpd_d_Volume, M5_qpd_hD) * M_qpd_ND];
  cm_cnt2 M5_qpd_E[tc_rows(M5_qpd_e_Volume, M5_qpd_hE) * M_qpd_ND];
  cm_cnt2 M5_qpd_H[tc_rows(M5_qpd_h_Volume, M5_qpd_hH) * M_qpd_ND];
  cm_cnt2 M5_qpd_I[tc_rows(M5_qpd_i_Volume, M5_qpd_hI) * M_qpd_ND];
  cm_cnt2 M5_qpd_J[tc_rows(M5_qpd_j_Volume, M5_qpd_hJ) * M_qpd_ND];
  cm_cnt2 M5_qpd_K[tc_rows(M5_qpd_k_Volume, M5_qpd_hK) * M_qpd_ND];
  cm_cnt2 M5_qpd_L[tc_rows(M5_qpd_l_Volume, M5_qpd_hL) * M_qpd_ND];
  cm_cnt M5_qpd_MT[tc_rows(M5_qpd_t_Volume, M5_qpd_hT) * M_qpd_MND];
  i32 M5_qpd_W[tc_rows(M5_qpd_m_Volume, M5_qpd_hM) * M_qpd_ND * tc_nin(10)];
  u8 M5_qpd_WC[tc_rows(M5_qpd_m_Volume, M5_qpd_hM) * M_qpd_ND];
  i32 M5_qpd_WM[tc_rows(M5_qpd_n_Volume, M5_qpd_hN) * M_qpd_MND * TC_MIN];
  u8 M5_qpd_WMC[tc_rows(M5_qpd_n_Volume, M5_qpd_hN) * M_qpd_MND];
  cm_cnt M5_qpd_G[M5_qpd_g_Volume * M_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_a_Volume, M5_qpd_hA) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_b_Volume, M5_qpd_hB) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_c_Volume, M5_qpd_hC) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_d_Volume, M5_qpd_hD) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_e_Volume, M5_qpd_hE) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_h_Volume, M5_qpd_hH) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_i_Volume, M5_qpd_hI) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_j_Volume, M5_qpd_hJ) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_k_Volume, M5_qpd_hK) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_qpd_l_Volume, M5_qpd_hL) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_qpd_t_Volume, M5_qpd_hT) * M_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_qpd_m_Volume, M5_qpd_hM) * M_qpd_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_qpd_m_Volume, M5_qpd_hM) * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_qpd_n_Volume, M5_qpd_hN) * M_qpd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_qpd_n_Volume, M5_qpd_hN) * M_qpd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_qpd_g_Volume * M_qpd_NG);
  void M5_qpd_Init( void ) {}
  void M5_qpd_Quit( void ) {}
};
