

static const byte M3_skp_b_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte M3_skp_c_c4[4+1]={ 0,1,2,3,3 };

static const byte M3_skp_d_st[3+1]={ 0,1,2,3 };
static const byte M3_skp_d_run[16+1]={ 0,1,2,3,4,5,5,5,5,6,7,7,8,9,10,11,11 };
static const byte M3_skp_d_c4[4+1]={ 0,1,2,3,4 };

static const byte M3_skp_e_st[3+1]={ 0,1,2,3 };
static const byte M3_skp_e_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte M3_skp_e_run[16+1]={ 0,1,2,3,4,5,6,7,8,8,9,10,10,10,10,11,11 };







static const int M3_skp_rA = (255+0) * (1);
static const int M3_skp_mwA = (16+0) * (1);
static const int M3_skp_rB = (255+0) * (1);
static const int M3_skp_mwB = (35+0) * (1);
static const int M3_skp_rC = (175+0) * (1);
static const int M3_skp_mwC = (8+0) * (1);
static const int M3_skp_rD = (248+0) * (1);
static const int M3_skp_mwD = (64+0) * (1);
static const int M3_skp_rE = (241+0) * (1);
static const int M3_skp_mwE = (80+0) * (1);
static const int M3_skp_rH = (139+0) * (1);
static const int M3_skp_mwH = (0+0) * (1);
static const int M3_skp_lr = (15+0) * (1);
static const int M3_skp_mb = (51+0) * (1);
static const int M3_skp_rT = (0+0) * (1);
static const int M3_skp_mwT = (0+0) * (1);
static const int M3_skp_mt = (0+0) * (1);
static const int M3_skp_lrm = (0+0) * (1);
static const int M3_skp_mbm = (0+0) * (1);
static const int M3_skp_rG = (0+0) * (1);
static const int M3_skp_mwG = (0+0) * (1);
static const int M3_skp_hA = (22+0) * (1);
static const int M3_skp_hB = (22+0) * (1);
static const int M3_skp_hC = (22+0) * (1);
static const int M3_skp_hD = (22+0) * (1);
static const int M3_skp_hE = (22+0) * (1);
static const int M3_skp_hH = (22+0) * (1);
static const int M3_skp_hM = (22+0) * (1);
static const int M3_skp_hT = (22+0) * (1);
static const int M3_skp_hN = (22+0) * (1);


static const long long M3_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M3_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 1); // 22
static const long long M3_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 4), 3); // 36
static const long long M3_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 12), 5), 3); // 720
static const long long M3_skp_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 11), 12); // 528
static const long long M3_skp_g_Volume = 1; // 1
static const long long M3_skp_h_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3
static const long long M3_skp_m_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M3_skp_n_Volume = 1; // 1
static const long long M3_skp_t_Volume = 1; // 1

#define M3_skp_USES(X) X(pz) X(rz) X(st) X(rc) X(cl) X(c4) X(rt) X(run) X(bsk)

struct M3_skp_T {
  cm_cnt2 M3_skp_A[tc_rows(M3_skp_a_Volume, M3_skp_hA) * M_skp_ND];
  cm_cnt2 M3_skp_B[tc_rows(M3_skp_b_Volume, M3_skp_hB) * M_skp_ND];
  cm_cnt2 M3_skp_C[tc_rows(M3_skp_c_Volume, M3_skp_hC) * M_skp_ND];
  cm_cnt2 M3_skp_D[tc_rows(M3_skp_d_Volume, M3_skp_hD) * M_skp_ND];
  cm_cnt2 M3_skp_E[tc_rows(M3_skp_e_Volume, M3_skp_hE) * M_skp_ND];
  cm_cnt2 M3_skp_H[tc_rows(M3_skp_h_Volume, M3_skp_hH) * M_skp_ND];
  cm_cnt M3_skp_MT[tc_rows(M3_skp_t_Volume, M3_skp_hT) * M_skp_MND];
  i32 M3_skp_W[tc_rows(M3_skp_m_Volume, M3_skp_hM) * M_skp_ND * tc_nin(6)];
  u8 M3_skp_WC[tc_rows(M3_skp_m_Volume, M3_skp_hM) * M_skp_ND];
  i32 M3_skp_WM[tc_rows(M3_skp_n_Volume, M3_skp_hN) * M_skp_MND * TC_MIN];
  u8 M3_skp_WMC[tc_rows(M3_skp_n_Volume, M3_skp_hN) * M_skp_MND];
  cm_cnt M3_skp_G[M3_skp_g_Volume * M_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_a_Volume, M3_skp_hA) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_b_Volume, M3_skp_hB) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_c_Volume, M3_skp_hC) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_d_Volume, M3_skp_hD) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_e_Volume, M3_skp_hE) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_skp_h_Volume, M3_skp_hH) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_skp_t_Volume, M3_skp_hT) * M_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_skp_m_Volume, M3_skp_hM) * M_skp_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_skp_m_Volume, M3_skp_hM) * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_skp_n_Volume, M3_skp_hN) * M_skp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_skp_n_Volume, M3_skp_hN) * M_skp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_skp_g_Volume * M_skp_NG);
  void M3_skp_Init( void ) {}
  void M3_skp_Quit( void ) {}
};
