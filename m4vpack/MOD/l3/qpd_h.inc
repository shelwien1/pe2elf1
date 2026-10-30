
static const byte M3_qpd_a_qs[32+1]={ 0,0,0,0,0,1,2,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M3_qpd_b_qs[32+1]={ 0,0,0,0,0,1,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M3_qpd_c_qs[32+1]={ 0,0,0,0,0,0,1,2,3,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };




static const byte M3_qpd_h_qs[32+1]={ 0,0,0,0,0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };





static const int M3_qpd_rA = (248+0) * (1);
static const int M3_qpd_mwA = (0+0) * (1);
static const int M3_qpd_rB = (0+0) * (1);
static const int M3_qpd_mwB = (2+0) * (1);
static const int M3_qpd_rC = (0+0) * (1);
static const int M3_qpd_mwC = (36+0) * (1);
static const int M3_qpd_rD = (0+0) * (1);
static const int M3_qpd_mwD = (1+0) * (1);
static const int M3_qpd_rE = (193+0) * (1);
static const int M3_qpd_mwE = (0+0) * (1);
static const int M3_qpd_rH = (168+0) * (1);
static const int M3_qpd_mwH = (112+0) * (1);
static const int M3_qpd_lr = (31+0) * (1);
static const int M3_qpd_mb = (32+0) * (1);
static const int M3_qpd_rT = (0+0) * (1);
static const int M3_qpd_mwT = (0+0) * (1);
static const int M3_qpd_mt = (0+0) * (1);
static const int M3_qpd_lrm = (0+0) * (1);
static const int M3_qpd_mbm = (0+0) * (1);
static const int M3_qpd_rG = (0+0) * (1);
static const int M3_qpd_mwG = (0+0) * (1);
static const int M3_qpd_hA = (22+0) * (1);
static const int M3_qpd_hB = (22+0) * (1);
static const int M3_qpd_hC = (22+0) * (1);
static const int M3_qpd_hD = (22+0) * (1);
static const int M3_qpd_hE = (22+0) * (1);
static const int M3_qpd_hH = (22+0) * (1);
static const int M3_qpd_hM = (22+0) * (1);
static const int M3_qpd_hT = (22+0) * (1);
static const int M3_qpd_hN = (22+0) * (1);


static const long long M3_qpd_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 5); // 30
static const long long M3_qpd_b_Volume = tc_vmul(1, 5); // 5
static const long long M3_qpd_c_Volume = tc_vmul(tc_vmul(1, 2), 6); // 12
static const long long M3_qpd_d_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long M3_qpd_e_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M3_qpd_g_Volume = 1; // 1
static const long long M3_qpd_h_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long M3_qpd_m_Volume = tc_vmul(1, 1); // 1
static const long long M3_qpd_n_Volume = 1; // 1
static const long long M3_qpd_t_Volume = 1; // 1

#define M3_qpd_USES(X) X(c3) X(pdq) X(qs)

struct M3_qpd_T {
  cm_cnt2 M3_qpd_A[tc_rows(M3_qpd_a_Volume, M3_qpd_hA) * M_qpd_ND];
  cm_cnt2 M3_qpd_B[tc_rows(M3_qpd_b_Volume, M3_qpd_hB) * M_qpd_ND];
  cm_cnt2 M3_qpd_C[tc_rows(M3_qpd_c_Volume, M3_qpd_hC) * M_qpd_ND];
  cm_cnt2 M3_qpd_D[tc_rows(M3_qpd_d_Volume, M3_qpd_hD) * M_qpd_ND];
  cm_cnt2 M3_qpd_E[tc_rows(M3_qpd_e_Volume, M3_qpd_hE) * M_qpd_ND];
  cm_cnt2 M3_qpd_H[tc_rows(M3_qpd_h_Volume, M3_qpd_hH) * M_qpd_ND];
  cm_cnt M3_qpd_MT[tc_rows(M3_qpd_t_Volume, M3_qpd_hT) * M_qpd_MND];
  i32 M3_qpd_W[tc_rows(M3_qpd_m_Volume, M3_qpd_hM) * M_qpd_ND * tc_nin(6)];
  u8 M3_qpd_WC[tc_rows(M3_qpd_m_Volume, M3_qpd_hM) * M_qpd_ND];
  i32 M3_qpd_WM[tc_rows(M3_qpd_n_Volume, M3_qpd_hN) * M_qpd_MND * TC_MIN];
  u8 M3_qpd_WMC[tc_rows(M3_qpd_n_Volume, M3_qpd_hN) * M_qpd_MND];
  cm_cnt M3_qpd_G[M3_qpd_g_Volume * M_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_a_Volume, M3_qpd_hA) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_b_Volume, M3_qpd_hB) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_c_Volume, M3_qpd_hC) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_d_Volume, M3_qpd_hD) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_e_Volume, M3_qpd_hE) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_qpd_h_Volume, M3_qpd_hH) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_qpd_t_Volume, M3_qpd_hT) * M_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_qpd_m_Volume, M3_qpd_hM) * M_qpd_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_qpd_m_Volume, M3_qpd_hM) * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_qpd_n_Volume, M3_qpd_hN) * M_qpd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_qpd_n_Volume, M3_qpd_hN) * M_qpd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_qpd_g_Volume * M_qpd_NG);
  void M3_qpd_Init( void ) {}
  void M3_qpd_Quit( void ) {}
};
