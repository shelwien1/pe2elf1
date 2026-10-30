


static const byte M5_skp_c_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };





static const byte M5_skp_i_st[3+1]={ 0,1,2,3 };
static const byte M5_skp_i_run[16+1]={ 0,1,2,3,4,5,6,6,6,7,8,9,10,11,12,13,13 };
static const byte M5_skp_i_c4[4+1]={ 0,1,2,3,4 };

static const byte M5_skp_j_st[3+1]={ 0,1,2,3 };
static const byte M5_skp_j_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte M5_skp_j_run[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,10,11,11,11,12,12 };

static const byte M5_skp_k_c4[4+1]={ 0,1,2,3,3 };






static const int M5_skp_rA = (0+0) * (1);
static const int M5_skp_mwA = (147+0) * (1);
static const int M5_skp_rB = (0+0) * (1);
static const int M5_skp_mwB = (126+0) * (1);
static const int M5_skp_rC = (255+0) * (1);
static const int M5_skp_mwC = (32+0) * (1);
static const int M5_skp_rD = (252+0) * (1);
static const int M5_skp_mwD = (3+0) * (1);
static const int M5_skp_rE = (128+0) * (1);
static const int M5_skp_mwE = (0+0) * (1);
static const int M5_skp_rH = (255+0) * (1);
static const int M5_skp_mwH = (108+0) * (1);
static const int M5_skp_rI = (248+0) * (1);
static const int M5_skp_mwI = (64+0) * (1);
static const int M5_skp_rJ = (251+0) * (1);
static const int M5_skp_mwJ = (81+0) * (1);
static const int M5_skp_rK = (179+0) * (1);
static const int M5_skp_mwK = (16+0) * (1);
static const int M5_skp_rL = (253+0) * (1);
static const int M5_skp_mwL = (254+0) * (1);
static const int M5_skp_lr = (15+0) * (1);
static const int M5_skp_mb = (47+0) * (1);
static const int M5_skp_rT = (0+0) * (1);
static const int M5_skp_mwT = (0+0) * (1);
static const int M5_skp_mt = (0+0) * (1);
static const int M5_skp_lrm = (0+0) * (1);
static const int M5_skp_mbm = (0+0) * (1);
static const int M5_skp_rG = (0+0) * (1);
static const int M5_skp_mwG = (0+0) * (1);
static const int M5_skp_hA = (22+0) * (1);
static const int M5_skp_hB = (22+0) * (1);
static const int M5_skp_hC = (22+0) * (1);
static const int M5_skp_hD = (22+0) * (1);
static const int M5_skp_hE = (22+0) * (1);
static const int M5_skp_hH = (22+0) * (1);
static const int M5_skp_hI = (22+0) * (1);
static const int M5_skp_hJ = (22+0) * (1);
static const int M5_skp_hK = (22+0) * (1);
static const int M5_skp_hL = (22+0) * (1);
static const int M5_skp_hM = (22+0) * (1);
static const int M5_skp_hT = (22+0) * (1);
static const int M5_skp_hN = (22+0) * (1);


static const long long M5_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 2); // 2
static const long long M5_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 2); // 6
static const long long M5_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 1); // 22
static const long long M5_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M5_skp_e_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3
static const long long M5_skp_g_Volume = 1; // 1
static const long long M5_skp_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 3); // 9
static const long long M5_skp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 14), 5), 3); // 840
static const long long M5_skp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 11), 13); // 572
static const long long M5_skp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 4), 3); // 36
static const long long M5_skp_l_Volume = tc_vmul(1, 1); // 1
static const long long M5_skp_m_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M5_skp_n_Volume = 1; // 1
static const long long M5_skp_t_Volume = 1; // 1

#define M5_skp_USES(X) X(rt) X(c4) X(run) X(st) X(rc) X(pz) X(rz) X(bsk) X(cl) X(rr)

struct M5_skp_T {
  cm_cnt2 M5_skp_A[tc_rows(M5_skp_a_Volume, M5_skp_hA) * M_skp_ND];
  cm_cnt2 M5_skp_B[tc_rows(M5_skp_b_Volume, M5_skp_hB) * M_skp_ND];
  cm_cnt2 M5_skp_C[tc_rows(M5_skp_c_Volume, M5_skp_hC) * M_skp_ND];
  cm_cnt2 M5_skp_D[tc_rows(M5_skp_d_Volume, M5_skp_hD) * M_skp_ND];
  cm_cnt2 M5_skp_E[tc_rows(M5_skp_e_Volume, M5_skp_hE) * M_skp_ND];
  cm_cnt2 M5_skp_H[tc_rows(M5_skp_h_Volume, M5_skp_hH) * M_skp_ND];
  cm_cnt2 M5_skp_I[tc_rows(M5_skp_i_Volume, M5_skp_hI) * M_skp_ND];
  cm_cnt2 M5_skp_J[tc_rows(M5_skp_j_Volume, M5_skp_hJ) * M_skp_ND];
  cm_cnt2 M5_skp_K[tc_rows(M5_skp_k_Volume, M5_skp_hK) * M_skp_ND];
  cm_cnt2 M5_skp_L[tc_rows(M5_skp_l_Volume, M5_skp_hL) * M_skp_ND];
  cm_cnt M5_skp_MT[tc_rows(M5_skp_t_Volume, M5_skp_hT) * M_skp_MND];
  i32 M5_skp_W[tc_rows(M5_skp_m_Volume, M5_skp_hM) * M_skp_ND * tc_nin(10)];
  u8 M5_skp_WC[tc_rows(M5_skp_m_Volume, M5_skp_hM) * M_skp_ND];
  i32 M5_skp_WM[tc_rows(M5_skp_n_Volume, M5_skp_hN) * M_skp_MND * TC_MIN];
  u8 M5_skp_WMC[tc_rows(M5_skp_n_Volume, M5_skp_hN) * M_skp_MND];
  cm_cnt M5_skp_G[M5_skp_g_Volume * M_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_a_Volume, M5_skp_hA) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_b_Volume, M5_skp_hB) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_c_Volume, M5_skp_hC) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_d_Volume, M5_skp_hD) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_e_Volume, M5_skp_hE) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_h_Volume, M5_skp_hH) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_i_Volume, M5_skp_hI) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_j_Volume, M5_skp_hJ) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_k_Volume, M5_skp_hK) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_skp_l_Volume, M5_skp_hL) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_skp_t_Volume, M5_skp_hT) * M_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_skp_m_Volume, M5_skp_hM) * M_skp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_skp_m_Volume, M5_skp_hM) * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_skp_n_Volume, M5_skp_hN) * M_skp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_skp_n_Volume, M5_skp_hN) * M_skp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_skp_g_Volume * M_skp_NG);
  void M5_skp_Init( void ) {}
  void M5_skp_Quit( void ) {}
};
