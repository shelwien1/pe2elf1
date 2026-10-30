
static const byte M3_mbt_a_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };

static const byte M3_mbt_b_c4[4+1]={ 0,1,2,3,4 };

static const byte M3_mbt_c_st[3+1]={ 0,1,2,3 };
static const byte M3_mbt_c_c4[4+1]={ 0,1,2,3,3 };

static const byte M3_mbt_d_rc[10+1]={ 0,1,1,2,2,3,3,3,3,3,3 };

static const byte M3_mbt_e_rt[3+1]={ 0,1,2,3 };







static const int M3_mbt_rA = (254+0) * (1);
static const int M3_mbt_mwA = (1+0) * (1);
static const int M3_mbt_rB = (255+0) * (1);
static const int M3_mbt_mwB = (32+0) * (1);
static const int M3_mbt_rC = (249+0) * (1);
static const int M3_mbt_mwC = (38+0) * (1);
static const int M3_mbt_rD = (0+0) * (1);
static const int M3_mbt_mwD = (57+0) * (1);
static const int M3_mbt_rE = (0+0) * (1);
static const int M3_mbt_mwE = (31+0) * (1);
static const int M3_mbt_rH = (255+0) * (1);
static const int M3_mbt_mwH = (140+0) * (1);
static const int M3_mbt_lr = (12+0) * (1);
static const int M3_mbt_mb = (31+0) * (1);
static const int M3_mbt_rT = (0+0) * (1);
static const int M3_mbt_mwT = (0+0) * (1);
static const int M3_mbt_mt = (0+0) * (1);
static const int M3_mbt_lrm = (0+0) * (1);
static const int M3_mbt_mbm = (0+0) * (1);
static const int M3_mbt_rG = (0+0) * (1);
static const int M3_mbt_mwG = (0+0) * (1);
static const int M3_mbt_hA = (22+0) * (1);
static const int M3_mbt_hB = (22+0) * (1);
static const int M3_mbt_hC = (22+0) * (1);
static const int M3_mbt_hD = (22+0) * (1);
static const int M3_mbt_hE = (22+0) * (1);
static const int M3_mbt_hH = (22+0) * (1);
static const int M3_mbt_hM = (22+0) * (1);
static const int M3_mbt_hT = (22+0) * (1);
static const int M3_mbt_hN = (22+0) * (1);


static const long long M3_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 5), 3); // 30
static const long long M3_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 5); // 30
static const long long M3_mbt_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 4); // 48
static const long long M3_mbt_d_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long M3_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 4); // 8
static const long long M3_mbt_g_Volume = 1; // 1
static const long long M3_mbt_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 3); // 12
static const long long M3_mbt_m_Volume = tc_vmul(1, 1); // 1
static const long long M3_mbt_n_Volume = 1; // 1
static const long long M3_mbt_t_Volume = 1; // 1

#define M3_mbt_USES(X) X(st) X(rc) X(cl) X(rt) X(c4)

struct M3_mbt_T {
  cm_cnt2 M3_mbt_A[tc_rows(M3_mbt_a_Volume, M3_mbt_hA) * M_mbt_ND];
  cm_cnt2 M3_mbt_B[tc_rows(M3_mbt_b_Volume, M3_mbt_hB) * M_mbt_ND];
  cm_cnt2 M3_mbt_C[tc_rows(M3_mbt_c_Volume, M3_mbt_hC) * M_mbt_ND];
  cm_cnt2 M3_mbt_D[tc_rows(M3_mbt_d_Volume, M3_mbt_hD) * M_mbt_ND];
  cm_cnt2 M3_mbt_E[tc_rows(M3_mbt_e_Volume, M3_mbt_hE) * M_mbt_ND];
  cm_cnt2 M3_mbt_H[tc_rows(M3_mbt_h_Volume, M3_mbt_hH) * M_mbt_ND];
  cm_cnt M3_mbt_MT[tc_rows(M3_mbt_t_Volume, M3_mbt_hT) * M_mbt_MND];
  i32 M3_mbt_W[tc_rows(M3_mbt_m_Volume, M3_mbt_hM) * M_mbt_ND * tc_nin(6)];
  u8 M3_mbt_WC[tc_rows(M3_mbt_m_Volume, M3_mbt_hM) * M_mbt_ND];
  i32 M3_mbt_WM[tc_rows(M3_mbt_n_Volume, M3_mbt_hN) * M_mbt_MND * TC_MIN];
  u8 M3_mbt_WMC[tc_rows(M3_mbt_n_Volume, M3_mbt_hN) * M_mbt_MND];
  cm_cnt M3_mbt_G[M3_mbt_g_Volume * M_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_a_Volume, M3_mbt_hA) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_b_Volume, M3_mbt_hB) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_c_Volume, M3_mbt_hC) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_d_Volume, M3_mbt_hD) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_e_Volume, M3_mbt_hE) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mbt_h_Volume, M3_mbt_hH) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_mbt_t_Volume, M3_mbt_hT) * M_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mbt_m_Volume, M3_mbt_hM) * M_mbt_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mbt_m_Volume, M3_mbt_hM) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mbt_n_Volume, M3_mbt_hN) * M_mbt_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mbt_n_Volume, M3_mbt_hN) * M_mbt_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_mbt_g_Volume * M_mbt_NG);
  void M3_mbt_Init( void ) {}
  void M3_mbt_Quit( void ) {}
};
