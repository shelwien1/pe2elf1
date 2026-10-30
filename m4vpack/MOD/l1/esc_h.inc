









static const int M1_esc_rA = (255+0) * (1);
static const int M1_esc_mwA = (49+0) * (1);
static const int M1_esc_rB = (68+0) * (1);
static const int M1_esc_mwB = (0+0) * (1);
static const int M1_esc_rC = (149+0) * (1);
static const int M1_esc_mwC = (1+0) * (1);
static const int M1_esc_rD = (75+0) * (1);
static const int M1_esc_mwD = (2+0) * (1);
static const int M1_esc_lr = (18+0) * (1);
static const int M1_esc_mb = (96+0) * (1);
static const int M1_esc_rT = (0+0) * (1);
static const int M1_esc_mwT = (0+0) * (1);
static const int M1_esc_mt = (0+0) * (1);
static const int M1_esc_lrm = (0+0) * (1);
static const int M1_esc_mbm = (0+0) * (1);
static const int M1_esc_rG = (0+0) * (1);
static const int M1_esc_mwG = (0+0) * (1);
static const int M1_esc_hA = (22+0) * (1);
static const int M1_esc_hB = (22+0) * (1);
static const int M1_esc_hC = (22+0) * (1);
static const int M1_esc_hD = (22+0) * (1);
static const int M1_esc_hM = (22+0) * (1);
static const int M1_esc_hT = (22+0) * (1);
static const int M1_esc_hN = (22+0) * (1);


static const long long M1_esc_a_Volume = tc_vmul(1, 1); // 1
static const long long M1_esc_b_Volume = tc_vmul(1, 2); // 2
static const long long M1_esc_c_Volume = tc_vmul(1, 3); // 3
static const long long M1_esc_d_Volume = tc_vmul(1, 3); // 3
static const long long M1_esc_g_Volume = 1; // 1
static const long long M1_esc_m_Volume = tc_vmul(1, 3); // 3
static const long long M1_esc_n_Volume = 1; // 1
static const long long M1_esc_t_Volume = 1; // 1

#define M1_esc_USES(X) X(dm)

struct M1_esc_T {
  cm_cnt2s M1_esc_A[tc_rows(M1_esc_a_Volume, M1_esc_hA) * M_esc_ND];
  cm_cnt2s M1_esc_B[tc_rows(M1_esc_b_Volume, M1_esc_hB) * M_esc_ND];
  cm_cnt2s M1_esc_C[tc_rows(M1_esc_c_Volume, M1_esc_hC) * M_esc_ND];
  cm_cnt2s M1_esc_D[tc_rows(M1_esc_d_Volume, M1_esc_hD) * M_esc_ND];
  cm_cnt M1_esc_MT[tc_rows(M1_esc_t_Volume, M1_esc_hT) * M_esc_MND];
  i32 M1_esc_W[tc_rows(M1_esc_m_Volume, M1_esc_hM) * M_esc_ND * tc_nin(4)];
  u8 M1_esc_WC[tc_rows(M1_esc_m_Volume, M1_esc_hM) * M_esc_ND];
  i32 M1_esc_WM[tc_rows(M1_esc_n_Volume, M1_esc_hN) * M_esc_MND * TC_MIN];
  u8 M1_esc_WMC[tc_rows(M1_esc_n_Volume, M1_esc_hN) * M_esc_MND];
  cm_cnt M1_esc_G[M1_esc_g_Volume * M_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_esc_a_Volume, M1_esc_hA) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_esc_b_Volume, M1_esc_hB) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_esc_c_Volume, M1_esc_hC) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_esc_d_Volume, M1_esc_hD) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_esc_t_Volume, M1_esc_hT) * M_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_esc_m_Volume, M1_esc_hM) * M_esc_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_esc_m_Volume, M1_esc_hM) * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_esc_n_Volume, M1_esc_hN) * M_esc_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_esc_n_Volume, M1_esc_hN) * M_esc_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_esc_g_Volume * M_esc_NG);
  void M1_esc_Init( void ) {}
  void M1_esc_Quit( void ) {}
};
