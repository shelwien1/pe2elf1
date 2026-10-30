
static const byte M3_dcs_a_lbn[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5 };
static const byte M3_dcs_a_abn[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5 };

static const byte M3_dcs_b_ls[14+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,9,9 };
static const byte M3_dcs_b_as[14+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,9,9 };


static const byte M3_dcs_d_sz[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };



static const byte M3_dcs_h_as[14+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,9,9 };
static const byte M3_dcs_h_prev[13+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,7,7 };



static const byte M3_dcs_t_prev[13+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,7,7 };


static const int M3_dcs_rA = (255+0) * (1);
static const int M3_dcs_mwA = (27+0) * (1);
static const int M3_dcs_rB = (242+0) * (1);
static const int M3_dcs_mwB = (4+0) * (1);
static const int M3_dcs_rC = (255+0) * (1);
static const int M3_dcs_mwC = (16+0) * (1);
static const int M3_dcs_rD = (255+0) * (1);
static const int M3_dcs_mwD = (8+0) * (1);
static const int M3_dcs_rE = (255+0) * (1);
static const int M3_dcs_mwE = (4+0) * (1);
static const int M3_dcs_rH = (255+0) * (1);
static const int M3_dcs_mwH = (51+0) * (1);
static const int M3_dcs_lr = (15+0) * (1);
static const int M3_dcs_mb = (33+0) * (1);
static const int M3_dcs_rT = (128+0) * (1);
static const int M3_dcs_mwT = (0+0) * (1);
static const int M3_dcs_mt = (0+0) * (1);
static const int M3_dcs_lrm = (0+0) * (1);
static const int M3_dcs_mbm = (0+0) * (1);
static const int M3_dcs_rG = (0+0) * (1);
static const int M3_dcs_mwG = (0+0) * (1);
static const int M3_dcs_hA = (22+0) * (1);
static const int M3_dcs_hB = (22+0) * (1);
static const int M3_dcs_hC = (22+0) * (1);
static const int M3_dcs_hD = (22+0) * (1);
static const int M3_dcs_hE = (22+0) * (1);
static const int M3_dcs_hH = (22+0) * (1);
static const int M3_dcs_hM = (22+0) * (1);
static const int M3_dcs_hT = (22+0) * (1);
static const int M3_dcs_hN = (22+0) * (1);


static const long long M3_dcs_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 6); // 72
static const long long M3_dcs_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 10), 10); // 200
static const long long M3_dcs_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3), 3); // 54
static const long long M3_dcs_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 13), 3); // 78
static const long long M3_dcs_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 3); // 12
static const long long M3_dcs_g_Volume = 1; // 1
static const long long M3_dcs_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 10), 8); // 160
static const long long M3_dcs_m_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long M3_dcs_n_Volume = tc_vmul(1, 2); // 2
static const long long M3_dcs_t_Volume = tc_vmul(tc_vmul(1, 2), 8); // 16

#define M3_dcs_USES(X) X(lc) X(lbn) X(abn) X(ls) X(as) X(ls2) X(as2) X(psg) X(sz) X(coded) X(qs) X(prev)

struct M3_dcs_T {
  cm_cnt2 M3_dcs_A[tc_rows(M3_dcs_a_Volume, M3_dcs_hA) * M_dcs_ND];
  cm_cnt2 M3_dcs_B[tc_rows(M3_dcs_b_Volume, M3_dcs_hB) * M_dcs_ND];
  cm_cnt2 M3_dcs_C[tc_rows(M3_dcs_c_Volume, M3_dcs_hC) * M_dcs_ND];
  cm_cnt2 M3_dcs_D[tc_rows(M3_dcs_d_Volume, M3_dcs_hD) * M_dcs_ND];
  cm_cnt2 M3_dcs_E[tc_rows(M3_dcs_e_Volume, M3_dcs_hE) * M_dcs_ND];
  cm_cnt2 M3_dcs_H[tc_rows(M3_dcs_h_Volume, M3_dcs_hH) * M_dcs_ND];
  cm_cnt M3_dcs_MT[tc_rows(M3_dcs_t_Volume, M3_dcs_hT) * M_dcs_MND];
  i32 M3_dcs_W[tc_rows(M3_dcs_m_Volume, M3_dcs_hM) * M_dcs_ND * tc_nin(6)];
  u8 M3_dcs_WC[tc_rows(M3_dcs_m_Volume, M3_dcs_hM) * M_dcs_ND];
  i32 M3_dcs_WM[tc_rows(M3_dcs_n_Volume, M3_dcs_hN) * M_dcs_MND * TC_MIN];
  u8 M3_dcs_WMC[tc_rows(M3_dcs_n_Volume, M3_dcs_hN) * M_dcs_MND];
  cm_cnt M3_dcs_G[M3_dcs_g_Volume * M_dcs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_dcs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_a_Volume, M3_dcs_hA) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_b_Volume, M3_dcs_hB) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_c_Volume, M3_dcs_hC) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_d_Volume, M3_dcs_hD) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_e_Volume, M3_dcs_hE) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_dcs_h_Volume, M3_dcs_hH) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_dcs_t_Volume, M3_dcs_hT) * M_dcs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_dcs_m_Volume, M3_dcs_hM) * M_dcs_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_dcs_m_Volume, M3_dcs_hM) * M_dcs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_dcs_n_Volume, M3_dcs_hN) * M_dcs_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_dcs_n_Volume, M3_dcs_hN) * M_dcs_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_dcs_g_Volume * M_dcs_NG);
  void M3_dcs_Init( void ) {}
  void M3_dcs_Quit( void ) {}
};
