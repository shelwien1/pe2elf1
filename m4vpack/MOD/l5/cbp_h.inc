
static const byte M5_cbp_a_lu[8+1]={ 0,0,1,2,2,3,4,4,4 };


static const byte M5_cbp_c_cnt[5+1]={ 0,1,2,3,4,5 };

static const byte M5_cbp_d_frac[4+1]={ 0,1,2,3,4 };

static const byte M5_cbp_e_qs[32+1]={ 0,0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };



static const byte M5_cbp_i_lu[8+1]={ 0,1,2,3,4,5,6,7,7 };

static const byte M5_cbp_j_mvc[17+1]={ 0,1,2,2,2,3,3,4,4,4,4,4,4,4,4,5,6,6 };

static const byte M5_cbp_k_lu[8+1]={ 0,1,2,3,4,4,5,6,7 };






static const int M5_cbp_rA = (255+0) * (1);
static const int M5_cbp_mwA = (191+0) * (1);
static const int M5_cbp_rB = (0+0) * (1);
static const int M5_cbp_mwB = (94+0) * (1);
static const int M5_cbp_rC = (254+0) * (1);
static const int M5_cbp_mwC = (6+0) * (1);
static const int M5_cbp_rD = (42+0) * (1);
static const int M5_cbp_mwD = (63+0) * (1);
static const int M5_cbp_rE = (132+0) * (1);
static const int M5_cbp_mwE = (112+0) * (1);
static const int M5_cbp_rH = (244+0) * (1);
static const int M5_cbp_mwH = (61+0) * (1);
static const int M5_cbp_rI = (184+0) * (1);
static const int M5_cbp_mwI = (66+0) * (1);
static const int M5_cbp_rJ = (126+0) * (1);
static const int M5_cbp_mwJ = (54+0) * (1);
static const int M5_cbp_rK = (255+0) * (1);
static const int M5_cbp_mwK = (64+0) * (1);
static const int M5_cbp_rL = (255+0) * (1);
static const int M5_cbp_mwL = (4+0) * (1);
static const int M5_cbp_lr = (12+0) * (1);
static const int M5_cbp_mb = (5+0) * (1);
static const int M5_cbp_rT = (0+0) * (1);
static const int M5_cbp_mwT = (0+0) * (1);
static const int M5_cbp_mt = (0+0) * (1);
static const int M5_cbp_lrm = (0+0) * (1);
static const int M5_cbp_mbm = (0+0) * (1);
static const int M5_cbp_rG = (0+0) * (1);
static const int M5_cbp_mwG = (0+0) * (1);
static const int M5_cbp_hA = (22+0) * (1);
static const int M5_cbp_hB = (22+0) * (1);
static const int M5_cbp_hC = (22+0) * (1);
static const int M5_cbp_hD = (22+0) * (1);
static const int M5_cbp_hE = (22+0) * (1);
static const int M5_cbp_hH = (22+0) * (1);
static const int M5_cbp_hI = (22+0) * (1);
static const int M5_cbp_hJ = (22+0) * (1);
static const int M5_cbp_hK = (22+0) * (1);
static const int M5_cbp_hL = (22+0) * (1);
static const int M5_cbp_hM = (22+0) * (1);
static const int M5_cbp_hT = (22+0) * (1);
static const int M5_cbp_hN = (22+0) * (1);


static const long long M5_cbp_a_Volume = tc_vmul(tc_vmul(1, 1), 5); // 5
static const long long M5_cbp_b_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long M5_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 6); // 36
static const long long M5_cbp_d_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M5_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 4), 2); // 16
static const long long M5_cbp_g_Volume = 1; // 1
static const long long M5_cbp_h_Volume = tc_vmul(tc_vmul(1, 3), (1<<5)); // 96
static const long long M5_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 3); // 48
static const long long M5_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 7); // 14
static const long long M5_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 8); // 48
static const long long M5_cbp_l_Volume = tc_vmul(1, 2); // 2
static const long long M5_cbp_m_Volume = tc_vmul(1, 3); // 3
static const long long M5_cbp_n_Volume = 1; // 1
static const long long M5_cbp_t_Volume = 1; // 1

#define M5_cbp_USES(X) X(lu) X(c3) X(mvc) X(rbit) X(cnt) X(frac) X(qs) X(pat) X(lq)

struct M5_cbp_T {
  cm_cnt2 M5_cbp_A[tc_rows(M5_cbp_a_Volume, M5_cbp_hA) * M_cbp_ND];
  cm_cnt2 M5_cbp_B[tc_rows(M5_cbp_b_Volume, M5_cbp_hB) * M_cbp_ND];
  cm_cnt2 M5_cbp_C[tc_rows(M5_cbp_c_Volume, M5_cbp_hC) * M_cbp_ND];
  cm_cnt2 M5_cbp_D[tc_rows(M5_cbp_d_Volume, M5_cbp_hD) * M_cbp_ND];
  cm_cnt2 M5_cbp_E[tc_rows(M5_cbp_e_Volume, M5_cbp_hE) * M_cbp_ND];
  cm_cnt2 M5_cbp_H[tc_rows(M5_cbp_h_Volume, M5_cbp_hH) * M_cbp_ND];
  cm_cnt2 M5_cbp_I[tc_rows(M5_cbp_i_Volume, M5_cbp_hI) * M_cbp_ND];
  cm_cnt2 M5_cbp_J[tc_rows(M5_cbp_j_Volume, M5_cbp_hJ) * M_cbp_ND];
  cm_cnt2 M5_cbp_K[tc_rows(M5_cbp_k_Volume, M5_cbp_hK) * M_cbp_ND];
  cm_cnt2 M5_cbp_L[tc_rows(M5_cbp_l_Volume, M5_cbp_hL) * M_cbp_ND];
  cm_cnt M5_cbp_MT[tc_rows(M5_cbp_t_Volume, M5_cbp_hT) * M_cbp_MND];
  i32 M5_cbp_W[tc_rows(M5_cbp_m_Volume, M5_cbp_hM) * M_cbp_ND * tc_nin(10)];
  u8 M5_cbp_WC[tc_rows(M5_cbp_m_Volume, M5_cbp_hM) * M_cbp_ND];
  i32 M5_cbp_WM[tc_rows(M5_cbp_n_Volume, M5_cbp_hN) * M_cbp_MND * TC_MIN];
  u8 M5_cbp_WMC[tc_rows(M5_cbp_n_Volume, M5_cbp_hN) * M_cbp_MND];
  cm_cnt M5_cbp_G[M5_cbp_g_Volume * M_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_a_Volume, M5_cbp_hA) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_b_Volume, M5_cbp_hB) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_c_Volume, M5_cbp_hC) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_d_Volume, M5_cbp_hD) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_e_Volume, M5_cbp_hE) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_h_Volume, M5_cbp_hH) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_i_Volume, M5_cbp_hI) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_j_Volume, M5_cbp_hJ) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_k_Volume, M5_cbp_hK) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_cbp_l_Volume, M5_cbp_hL) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_cbp_t_Volume, M5_cbp_hT) * M_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_cbp_m_Volume, M5_cbp_hM) * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_cbp_m_Volume, M5_cbp_hM) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_cbp_n_Volume, M5_cbp_hN) * M_cbp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_cbp_n_Volume, M5_cbp_hN) * M_cbp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_cbp_g_Volume * M_cbp_NG);
  void M5_cbp_Init( void ) {}
  void M5_cbp_Quit( void ) {}
};
