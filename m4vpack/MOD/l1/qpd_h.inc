
static const byte M1_qpd_a_qs[32+1]={ 0,0,0,0,0,1,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte M1_qpd_b_qs[32+1]={ 0,0,0,0,0,1,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M1_qpd_c_qs[32+1]={ 0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };







static const int M1_qpd_rA = (107+0) * (1);
static const int M1_qpd_mwA = (1+0) * (1);
static const int M1_qpd_rB = (0+0) * (1);
static const int M1_qpd_mwB = (0+0) * (1);
static const int M1_qpd_rC = (0+0) * (1);
static const int M1_qpd_mwC = (14+0) * (1);
static const int M1_qpd_rD = (0+0) * (1);
static const int M1_qpd_mwD = (0+0) * (1);
static const int M1_qpd_lr = (31+0) * (1);
static const int M1_qpd_mb = (43+0) * (1);
static const int M1_qpd_rT = (0+0) * (1);
static const int M1_qpd_mwT = (0+0) * (1);
static const int M1_qpd_mt = (0+0) * (1);
static const int M1_qpd_lrm = (0+0) * (1);
static const int M1_qpd_mbm = (0+0) * (1);
static const int M1_qpd_rG = (0+0) * (1);
static const int M1_qpd_mwG = (0+0) * (1);
static const int M1_qpd_hA = (22+0) * (1);
static const int M1_qpd_hB = (22+0) * (1);
static const int M1_qpd_hC = (22+0) * (1);
static const int M1_qpd_hD = (22+0) * (1);
static const int M1_qpd_hM = (22+0) * (1);
static const int M1_qpd_hT = (22+0) * (1);
static const int M1_qpd_hN = (22+0) * (1);


static const long long M1_qpd_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 6); // 36
static const long long M1_qpd_b_Volume = tc_vmul(1, 5); // 5
static const long long M1_qpd_c_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long M1_qpd_d_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long M1_qpd_g_Volume = 1; // 1
static const long long M1_qpd_m_Volume = tc_vmul(1, 1); // 1
static const long long M1_qpd_n_Volume = 1; // 1
static const long long M1_qpd_t_Volume = 1; // 1

#define M1_qpd_USES(X) X(c3) X(pdq) X(qs)

struct M1_qpd_T {
  cm_cnt2s M1_qpd_A[tc_rows(M1_qpd_a_Volume, M1_qpd_hA) * M_qpd_ND];
  cm_cnt2s M1_qpd_B[tc_rows(M1_qpd_b_Volume, M1_qpd_hB) * M_qpd_ND];
  cm_cnt2s M1_qpd_C[tc_rows(M1_qpd_c_Volume, M1_qpd_hC) * M_qpd_ND];
  cm_cnt2s M1_qpd_D[tc_rows(M1_qpd_d_Volume, M1_qpd_hD) * M_qpd_ND];
  cm_cnt M1_qpd_MT[tc_rows(M1_qpd_t_Volume, M1_qpd_hT) * M_qpd_MND];
  i32 M1_qpd_W[tc_rows(M1_qpd_m_Volume, M1_qpd_hM) * M_qpd_ND * tc_nin(4)];
  u8 M1_qpd_WC[tc_rows(M1_qpd_m_Volume, M1_qpd_hM) * M_qpd_ND];
  i32 M1_qpd_WM[tc_rows(M1_qpd_n_Volume, M1_qpd_hN) * M_qpd_MND * TC_MIN];
  u8 M1_qpd_WMC[tc_rows(M1_qpd_n_Volume, M1_qpd_hN) * M_qpd_MND];
  cm_cnt M1_qpd_G[M1_qpd_g_Volume * M_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_qpd_a_Volume, M1_qpd_hA) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_qpd_b_Volume, M1_qpd_hB) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_qpd_c_Volume, M1_qpd_hC) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_qpd_d_Volume, M1_qpd_hD) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_qpd_t_Volume, M1_qpd_hT) * M_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_qpd_m_Volume, M1_qpd_hM) * M_qpd_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_qpd_m_Volume, M1_qpd_hM) * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_qpd_n_Volume, M1_qpd_hN) * M_qpd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_qpd_n_Volume, M1_qpd_hN) * M_qpd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_qpd_g_Volume * M_qpd_NG);
  void M1_qpd_Init( void ) {}
  void M1_qpd_Quit( void ) {}
};
