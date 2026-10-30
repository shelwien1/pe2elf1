
static const byte M5_sig_a_nc[17+1]={ 0,1,2,2,3,3,3,4,4,4,4,4,5,5,5,5,5,5 };

static const byte M5_sig_b_kb[16+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,7 };

static const byte M5_sig_c_nz[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M5_sig_d_past[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M5_sig_e_run[16+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };



static const byte M5_sig_i_mvc[30+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,7,8,9,10,11,12,13,14,15,15,15,15,15,15 };

static const byte M5_sig_j_rnz[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6 };
static const byte M5_sig_j_xm[9+1]={ 0,1,2,2,2,2,2,2,3,3 };







static const int M5_sig_rA = (254+0) * (1);
static const int M5_sig_mwA = (31+0) * (1);
static const int M5_sig_rB = (123+0) * (1);
static const int M5_sig_mwB = (128+0) * (1);
static const int M5_sig_rC = (250+0) * (1);
static const int M5_sig_mwC = (4+0) * (1);
static const int M5_sig_rD = (255+0) * (1);
static const int M5_sig_mwD = (64+0) * (1);
static const int M5_sig_rE = (255+0) * (1);
static const int M5_sig_mwE = (4+0) * (1);
static const int M5_sig_rH = (248+0) * (1);
static const int M5_sig_mwH = (63+0) * (1);
static const int M5_sig_rI = (254+0) * (1);
static const int M5_sig_mwI = (33+0) * (1);
static const int M5_sig_rJ = (253+0) * (1);
static const int M5_sig_mwJ = (65+0) * (1);
static const int M5_sig_rK = (196+0) * (1);
static const int M5_sig_mwK = (7+0) * (1);
static const int M5_sig_rL = (243+0) * (1);
static const int M5_sig_mwL = (32+0) * (1);
static const int M5_sig_lr = (12+0) * (1);
static const int M5_sig_mb = (88+0) * (1);
static const int M5_sig_rT = (0+0) * (1);
static const int M5_sig_mwT = (0+0) * (1);
static const int M5_sig_mt = (0+0) * (1);
static const int M5_sig_lrm = (0+0) * (1);
static const int M5_sig_mbm = (0+0) * (1);
static const int M5_sig_rG = (0+0) * (1);
static const int M5_sig_mwG = (0+0) * (1);
static const int M5_sig_hA = (22+0) * (1);
static const int M5_sig_hB = (22+0) * (1);
static const int M5_sig_hC = (22+0) * (1);
static const int M5_sig_hD = (22+0) * (1);
static const int M5_sig_hE = (22+0) * (1);
static const int M5_sig_hH = (22+0) * (1);
static const int M5_sig_hI = (22+0) * (1);
static const int M5_sig_hJ = (22+0) * (1);
static const int M5_sig_hK = (22+0) * (1);
static const int M5_sig_hL = (20+0) * (1);
static const int M5_sig_hM = (22+0) * (1);
static const int M5_sig_hT = (22+0) * (1);
static const int M5_sig_hN = (22+0) * (1);


static const long long M5_sig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 6), 3), (1<<6)); // 9216
static const long long M5_sig_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 8), (1<<6)); // 4096
static const long long M5_sig_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), (1<<6)); // 4608
static const long long M5_sig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), 3), (1<<6)); // 13824
static const long long M5_sig_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 6), (1<<6)); // 3072
static const long long M5_sig_g_Volume = 1; // 1
static const long long M5_sig_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<5)), (1<<6)); // 16384
static const long long M5_sig_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 16), (1<<6)); // 8192
static const long long M5_sig_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 7), 4), (1<<6)); // 14336
static const long long M5_sig_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<8)), (1<<6)); // 131072
static const long long M5_sig_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<6)), (1<<16)); // 33554432
static const long long M5_sig_m_Volume = tc_vmul(tc_vmul(1, (1<<3)), (1<<6)); // 512
static const long long M5_sig_n_Volume = 1; // 1
static const long long M5_sig_t_Volume = 1; // 1

#define M5_sig_USES(X) X(cls) X(nc) X(qs) X(pos) X(kb) X(nz) X(past) X(run) X(fp) X(mvc) X(rnz) X(xm) X(h8) X(h16)

struct M5_sig_T {
  cm_cnt2 M5_sig_A[tc_rows(M5_sig_a_Volume, M5_sig_hA) * M_sig_ND];
  cm_cnt2 M5_sig_B[tc_rows(M5_sig_b_Volume, M5_sig_hB) * M_sig_ND];
  cm_cnt2 M5_sig_C[tc_rows(M5_sig_c_Volume, M5_sig_hC) * M_sig_ND];
  cm_cnt2 M5_sig_D[tc_rows(M5_sig_d_Volume, M5_sig_hD) * M_sig_ND];
  cm_cnt2 M5_sig_E[tc_rows(M5_sig_e_Volume, M5_sig_hE) * M_sig_ND];
  cm_cnt2 M5_sig_H[tc_rows(M5_sig_h_Volume, M5_sig_hH) * M_sig_ND];
  cm_cnt2 M5_sig_I[tc_rows(M5_sig_i_Volume, M5_sig_hI) * M_sig_ND];
  cm_cnt2 M5_sig_J[tc_rows(M5_sig_j_Volume, M5_sig_hJ) * M_sig_ND];
  cm_cnt2 M5_sig_K[tc_rows(M5_sig_k_Volume, M5_sig_hK) * M_sig_ND];
  cm_cnt2 M5_sig_L[tc_rows(M5_sig_l_Volume, M5_sig_hL) * M_sig_ND];
  cm_cnt M5_sig_MT[tc_rows(M5_sig_t_Volume, M5_sig_hT) * M_sig_MND];
  i32 M5_sig_W[tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND * tc_nin(10)];
  u8 M5_sig_WC[tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND];
  i32 M5_sig_WM[tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND * TC_MIN];
  u8 M5_sig_WMC[tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND];
  cm_cnt M5_sig_G[M5_sig_g_Volume * M_sig_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_sig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_a_Volume, M5_sig_hA) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_b_Volume, M5_sig_hB) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_c_Volume, M5_sig_hC) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_d_Volume, M5_sig_hD) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_e_Volume, M5_sig_hE) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_h_Volume, M5_sig_hH) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_i_Volume, M5_sig_hI) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_j_Volume, M5_sig_hJ) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_k_Volume, M5_sig_hK) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_l_Volume, M5_sig_hL) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_sig_t_Volume, M5_sig_hT) * M_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_sig_g_Volume * M_sig_NG);
  void M5_sig_Init( void ) {}
  void M5_sig_Quit( void ) {}
};
