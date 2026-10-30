
static const byte M3_mvg_a_mag[16+1]={ 0,0,1,2,3,3,3,3,3,4,4,4,4,4,4,4,4 };

static const byte M3_mvg_b_kind[5+1]={ 0,0,1,2,2,3 };
static const byte M3_mvg_b_hm[3+1]={ 0,1,2,3 };

static const byte M3_mvg_c_kind[5+1]={ 0,1,2,3,3,3 };
static const byte M3_mvg_c_dz[17+1]={ 0,0,0,1,1,2,2,3,3,4,5,6,6,7,7,8,8,8 };

static const byte M3_mvg_d_kind[5+1]={ 0,0,1,2,2,3 };




static const byte M3_mvg_m_kind[5+1]={ 0,0,1,2,3,4 };




static const int M3_mvg_rA = (128+0) * (1);
static const int M3_mvg_mwA = (0+0) * (1);
static const int M3_mvg_rB = (255+0) * (1);
static const int M3_mvg_mwB = (0+0) * (1);
static const int M3_mvg_rC = (0+0) * (1);
static const int M3_mvg_mwC = (64+0) * (1);
static const int M3_mvg_rD = (255+0) * (1);
static const int M3_mvg_mwD = (2+0) * (1);
static const int M3_mvg_rE = (0+0) * (1);
static const int M3_mvg_mwE = (75+0) * (1);
static const int M3_mvg_rH = (252+0) * (1);
static const int M3_mvg_mwH = (51+0) * (1);
static const int M3_mvg_lr = (19+0) * (1);
static const int M3_mvg_mb = (44+0) * (1);
static const int M3_mvg_rT = (0+0) * (1);
static const int M3_mvg_mwT = (0+0) * (1);
static const int M3_mvg_mt = (0+0) * (1);
static const int M3_mvg_lrm = (0+0) * (1);
static const int M3_mvg_mbm = (0+0) * (1);
static const int M3_mvg_rG = (0+0) * (1);
static const int M3_mvg_mwG = (0+0) * (1);
static const int M3_mvg_hA = (22+0) * (1);
static const int M3_mvg_hB = (22+0) * (1);
static const int M3_mvg_hC = (22+0) * (1);
static const int M3_mvg_hD = (22+0) * (1);
static const int M3_mvg_hE = (22+0) * (1);
static const int M3_mvg_hH = (22+0) * (1);
static const int M3_mvg_hM = (22+0) * (1);
static const int M3_mvg_hT = (22+0) * (1);
static const int M3_mvg_hN = (22+0) * (1);


static const long long M3_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 5); // 90
static const long long M3_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3), 4); // 48
static const long long M3_mvg_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3), 9); // 216
static const long long M3_mvg_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3), 3); // 72
static const long long M3_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long M3_mvg_g_Volume = 1; // 1
static const long long M3_mvg_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3), 3); // 81
static const long long M3_mvg_m_Volume = tc_vmul(tc_vmul(1, 5), 1); // 5
static const long long M3_mvg_n_Volume = 1; // 1
static const long long M3_mvg_t_Volume = 1; // 1

#define M3_mvg_USES(X) X(kind) X(comp) X(ds) X(mag) X(hm) X(dz) X(ls) X(as)

struct M3_mvg_T {
  cm_cnt2 M3_mvg_A[tc_rows(M3_mvg_a_Volume, M3_mvg_hA) * M_mvg_ND];
  cm_cnt2 M3_mvg_B[tc_rows(M3_mvg_b_Volume, M3_mvg_hB) * M_mvg_ND];
  cm_cnt2 M3_mvg_C[tc_rows(M3_mvg_c_Volume, M3_mvg_hC) * M_mvg_ND];
  cm_cnt2 M3_mvg_D[tc_rows(M3_mvg_d_Volume, M3_mvg_hD) * M_mvg_ND];
  cm_cnt2 M3_mvg_E[tc_rows(M3_mvg_e_Volume, M3_mvg_hE) * M_mvg_ND];
  cm_cnt2 M3_mvg_H[tc_rows(M3_mvg_h_Volume, M3_mvg_hH) * M_mvg_ND];
  cm_cnt M3_mvg_MT[tc_rows(M3_mvg_t_Volume, M3_mvg_hT) * M_mvg_MND];
  i32 M3_mvg_W[tc_rows(M3_mvg_m_Volume, M3_mvg_hM) * M_mvg_ND * tc_nin(6)];
  u8 M3_mvg_WC[tc_rows(M3_mvg_m_Volume, M3_mvg_hM) * M_mvg_ND];
  i32 M3_mvg_WM[tc_rows(M3_mvg_n_Volume, M3_mvg_hN) * M_mvg_MND * TC_MIN];
  u8 M3_mvg_WMC[tc_rows(M3_mvg_n_Volume, M3_mvg_hN) * M_mvg_MND];
  cm_cnt M3_mvg_G[M3_mvg_g_Volume * M_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_a_Volume, M3_mvg_hA) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_b_Volume, M3_mvg_hB) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_c_Volume, M3_mvg_hC) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_d_Volume, M3_mvg_hD) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_e_Volume, M3_mvg_hE) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvg_h_Volume, M3_mvg_hH) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_mvg_t_Volume, M3_mvg_hT) * M_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mvg_m_Volume, M3_mvg_hM) * M_mvg_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mvg_m_Volume, M3_mvg_hM) * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mvg_n_Volume, M3_mvg_hN) * M_mvg_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mvg_n_Volume, M3_mvg_hN) * M_mvg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_mvg_g_Volume * M_mvg_NG);
  void M3_mvg_Init( void ) {}
  void M3_mvg_Quit( void ) {}
};
