









static const int A1_sid_rA = (253+0) * (1);
static const int A1_sid_mwA = (32+0) * (1);
static const int A1_sid_rB = (255+0) * (1);
static const int A1_sid_mwB = (4+0) * (1);
static const int A1_sid_rC = (254+0) * (1);
static const int A1_sid_mwC = (9+0) * (1);
static const int A1_sid_rD = (255+0) * (1);
static const int A1_sid_mwD = (0+0) * (1);
static const int A1_sid_lr = (15+0) * (1);
static const int A1_sid_mb = (96+0) * (1);
static const int A1_sid_rT = (128+0) * (1);
static const int A1_sid_mwT = (0+0) * (1);
static const int A1_sid_mt = (4+0) * (1);
static const int A1_sid_lrm = (31+0) * (1);
static const int A1_sid_mbm = (25+0) * (1);
static const int A1_sid_rG = (255+0) * (1);
static const int A1_sid_mwG = (188+0) * (1);
static const int A1_sid_hA = (22+0) * (1);
static const int A1_sid_hB = (22+0) * (1);
static const int A1_sid_hC = (22+0) * (1);
static const int A1_sid_hD = (22+0) * (1);
static const int A1_sid_hM = (22+0) * (1);
static const int A1_sid_hT = (22+0) * (1);
static const int A1_sid_hN = (22+0) * (1);


static const long long A1_sid_a_Volume = 1; // 1
static const long long A1_sid_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<6)); // 2048
static const long long A1_sid_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), (1<<5)), 3), 3); // 9216
static const long long A1_sid_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 2); // 8192
static const long long A1_sid_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A1_sid_m_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A1_sid_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A1_sid_t_Volume = tc_vmul(1, (1<<5)); // 32

#define A1_sid_USES(X) X(fld) X(lv) X(pr) X(lb) X(pq)

struct A1_sid_T {
  cm_cnt2s A1_sid_A[tc_rows(A1_sid_a_Volume, A1_sid_hA) * M_sid_ND];
  cm_cnt2s A1_sid_B[tc_rows(A1_sid_b_Volume, A1_sid_hB) * M_sid_ND];
  cm_cnt2s A1_sid_C[tc_rows(A1_sid_c_Volume, A1_sid_hC) * M_sid_ND];
  cm_cnt2s A1_sid_D[tc_rows(A1_sid_d_Volume, A1_sid_hD) * M_sid_ND];
  cm_cnt A1_sid_MT[tc_rows(A1_sid_t_Volume, A1_sid_hT) * M_sid_MND];
  i32 A1_sid_W[tc_rows(A1_sid_m_Volume, A1_sid_hM) * M_sid_ND * tc_nin(4)];
  u8 A1_sid_WC[tc_rows(A1_sid_m_Volume, A1_sid_hM) * M_sid_ND];
  i32 A1_sid_WM[tc_rows(A1_sid_n_Volume, A1_sid_hN) * M_sid_MND * TC_MIN];
  u8 A1_sid_WMC[tc_rows(A1_sid_n_Volume, A1_sid_hN) * M_sid_MND];
  cm_cnt A1_sid_G[A1_sid_g_Volume * M_sid_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_sid_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_sid_a_Volume, A1_sid_hA) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_sid_b_Volume, A1_sid_hB) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_sid_c_Volume, A1_sid_hC) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_sid_d_Volume, A1_sid_hD) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_sid_t_Volume, A1_sid_hT) * M_sid_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_sid_m_Volume, A1_sid_hM) * M_sid_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_sid_m_Volume, A1_sid_hM) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_sid_n_Volume, A1_sid_hN) * M_sid_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_sid_n_Volume, A1_sid_hN) * M_sid_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_sid_g_Volume * M_sid_NG);
  void A1_sid_Init( void ) {}
  void A1_sid_Quit( void ) {}
};
