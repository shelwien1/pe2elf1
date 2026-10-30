

static const byte M5_sgn_b_lsa[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M5_sgn_b_pb[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,48,49,50,51,52,53,54,55,55,55,56,57,57,57,57,57 };

static const byte M5_sgn_c_gh[26+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26 };
static const byte M5_sgn_c_pb[64+1]={ 0,1,2,3,4,5,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };


static const byte M5_sgn_e_flu[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const byte M5_sgn_h_flu[8+1]={ 0,1,1,2,3,3,4,5,5 };
static const byte M5_sgn_h_flu2[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const byte M5_sgn_j_fdlu[26+1]={ 0,1,2,2,3,4,5,5,5,6,6,6,6,7,8,9,10,11,12,12,12,12,13,14,15,16,17 };


static const byte M5_sgn_l_lam[24+1]={ 0,1,1,2,2,3,4,4,5,5,6,7,7,8,8,9,10,10,11,11,12,13,13,14,14 };

static const byte M5_sgn_m_pb[64+1]={ 0,1,2,2,3,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };




static const int M5_sgn_rA = (16+0) * (1);
static const int M5_sgn_mwA = (11+0) * (1);
static const int M5_sgn_rB = (255+0) * (1);
static const int M5_sgn_mwB = (4+0) * (1);
static const int M5_sgn_rC = (255+0) * (1);
static const int M5_sgn_mwC = (3+0) * (1);
static const int M5_sgn_rD = (251+0) * (1);
static const int M5_sgn_mwD = (98+0) * (1);
static const int M5_sgn_rE = (252+0) * (1);
static const int M5_sgn_mwE = (14+0) * (1);
static const int M5_sgn_rH = (255+0) * (1);
static const int M5_sgn_mwH = (2+0) * (1);
static const int M5_sgn_rI = (32+0) * (1);
static const int M5_sgn_mwI = (63+0) * (1);
static const int M5_sgn_rJ = (255+0) * (1);
static const int M5_sgn_mwJ = (8+0) * (1);
static const int M5_sgn_rK = (2+0) * (1);
static const int M5_sgn_mwK = (2+0) * (1);
static const int M5_sgn_rL = (254+0) * (1);
static const int M5_sgn_mwL = (2+0) * (1);
static const int M5_sgn_lr = (31+0) * (1);
static const int M5_sgn_mb = (20+0) * (1);
static const int M5_sgn_rT = (0+0) * (1);
static const int M5_sgn_mwT = (0+0) * (1);
static const int M5_sgn_mt = (0+0) * (1);
static const int M5_sgn_lrm = (0+0) * (1);
static const int M5_sgn_mbm = (0+0) * (1);
static const int M5_sgn_rG = (0+0) * (1);
static const int M5_sgn_mwG = (0+0) * (1);
static const int M5_sgn_hA = (22+0) * (1);
static const int M5_sgn_hB = (22+0) * (1);
static const int M5_sgn_hC = (22+0) * (1);
static const int M5_sgn_hD = (22+0) * (1);
static const int M5_sgn_hE = (22+0) * (1);
static const int M5_sgn_hH = (22+0) * (1);
static const int M5_sgn_hI = (22+0) * (1);
static const int M5_sgn_hJ = (22+0) * (1);
static const int M5_sgn_hK = (22+0) * (1);
static const int M5_sgn_hL = (22+0) * (1);
static const int M5_sgn_hM = (22+0) * (1);
static const int M5_sgn_hT = (22+0) * (1);
static const int M5_sgn_hN = (22+0) * (1);


static const long long M5_sgn_a_Volume = tc_vmul(tc_vmul(1, (1<<2)), 1); // 4
static const long long M5_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), 58); // 4176
static const long long M5_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 27), 9); // 972
static const long long M5_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), (1<<6)), 2); // 512
static const long long M5_sgn_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), (1<<6)); // 4608
static const long long M5_sgn_g_Volume = 1; // 1
static const long long M5_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 6), 9); // 432
static const long long M5_sgn_i_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long M5_sgn_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 18), 2); // 144
static const long long M5_sgn_k_Volume = tc_vmul(1, 3); // 3
static const long long M5_sgn_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 15), 3); // 90
static const long long M5_sgn_m_Volume = tc_vmul(tc_vmul(1, (1<<3)), 6); // 48
static const long long M5_sgn_n_Volume = 1; // 1
static const long long M5_sgn_t_Volume = 1; // 1

#define M5_sgn_USES(X) X(cls) X(lsa) X(pos) X(gh) X(ps) X(flu) X(flu2) X(vote) X(fdlu) X(xs) X(lam)

struct M5_sgn_T {
  cm_cnt2 M5_sgn_A[tc_rows(M5_sgn_a_Volume, M5_sgn_hA) * M_sgn_ND];
  cm_cnt2 M5_sgn_B[tc_rows(M5_sgn_b_Volume, M5_sgn_hB) * M_sgn_ND];
  cm_cnt2 M5_sgn_C[tc_rows(M5_sgn_c_Volume, M5_sgn_hC) * M_sgn_ND];
  cm_cnt2 M5_sgn_D[tc_rows(M5_sgn_d_Volume, M5_sgn_hD) * M_sgn_ND];
  cm_cnt2 M5_sgn_E[tc_rows(M5_sgn_e_Volume, M5_sgn_hE) * M_sgn_ND];
  cm_cnt2 M5_sgn_H[tc_rows(M5_sgn_h_Volume, M5_sgn_hH) * M_sgn_ND];
  cm_cnt2 M5_sgn_I[tc_rows(M5_sgn_i_Volume, M5_sgn_hI) * M_sgn_ND];
  cm_cnt2 M5_sgn_J[tc_rows(M5_sgn_j_Volume, M5_sgn_hJ) * M_sgn_ND];
  cm_cnt2 M5_sgn_K[tc_rows(M5_sgn_k_Volume, M5_sgn_hK) * M_sgn_ND];
  cm_cnt2 M5_sgn_L[tc_rows(M5_sgn_l_Volume, M5_sgn_hL) * M_sgn_ND];
  cm_cnt M5_sgn_MT[tc_rows(M5_sgn_t_Volume, M5_sgn_hT) * M_sgn_MND];
  i32 M5_sgn_W[tc_rows(M5_sgn_m_Volume, M5_sgn_hM) * M_sgn_ND * tc_nin(10)];
  u8 M5_sgn_WC[tc_rows(M5_sgn_m_Volume, M5_sgn_hM) * M_sgn_ND];
  i32 M5_sgn_WM[tc_rows(M5_sgn_n_Volume, M5_sgn_hN) * M_sgn_MND * TC_MIN];
  u8 M5_sgn_WMC[tc_rows(M5_sgn_n_Volume, M5_sgn_hN) * M_sgn_MND];
  cm_cnt M5_sgn_G[M5_sgn_g_Volume * M_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_a_Volume, M5_sgn_hA) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_b_Volume, M5_sgn_hB) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_c_Volume, M5_sgn_hC) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_d_Volume, M5_sgn_hD) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_e_Volume, M5_sgn_hE) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_h_Volume, M5_sgn_hH) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_i_Volume, M5_sgn_hI) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_j_Volume, M5_sgn_hJ) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_k_Volume, M5_sgn_hK) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sgn_l_Volume, M5_sgn_hL) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_sgn_t_Volume, M5_sgn_hT) * M_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sgn_m_Volume, M5_sgn_hM) * M_sgn_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sgn_m_Volume, M5_sgn_hM) * M_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sgn_n_Volume, M5_sgn_hN) * M_sgn_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sgn_n_Volume, M5_sgn_hN) * M_sgn_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_sgn_g_Volume * M_sgn_NG);
  void M5_sgn_Init( void ) {}
  void M5_sgn_Quit( void ) {}
};
