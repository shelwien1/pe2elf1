










static const byte M9_esc_k_dm[3+1]={ 0,1,2,3 };








static const int M9_esc_rA = (254+0) * (1);
static const int M9_esc_rB = (255+0) * (1);
static const int M9_esc_rC = (199+0) * (1);
static const int M9_esc_rD = (239+0) * (1);
static const int M9_esc_rE = (234+0) * (1);
static const int M9_esc_rH = (246+0) * (1);
static const int M9_esc_rI = (0+0) * (1);
static const int M9_esc_rJ = (245+0) * (1);
static const int M9_esc_rK = (20+0) * (1);
static const int M9_esc_rL = (247+0) * (1);
static const int M9_esc_rS1 = (6+0) * (1);
static const int M9_esc_rS2 = (6+0) * (1);
static const int M9_esc_lr = (24+0) * (1);
static const int M9_esc_mwA = (21+0) * (1);
static const int M9_esc_mwB = (12+0) * (1);
static const int M9_esc_mwC = (12+0) * (1);
static const int M9_esc_mwD = (225+0) * (1);
static const int M9_esc_mwE = (10+0) * (1);
static const int M9_esc_mwH = (64+0) * (1);
static const int M9_esc_mwI = (33+0) * (1);
static const int M9_esc_mwJ = (32+0) * (1);
static const int M9_esc_mwK = (243+0) * (1);
static const int M9_esc_mwL = (155+0) * (1);
static const int M9_esc_bw = (3+0) * (1);
static const int M9_esc_qs = (35+3) * (1);
static const int M9_esc_qf = (34+3) * (1);
static const int M9_esc_mb = (15+0) * (1);
static const int M9_esc_rT = (0+0) * (1);
static const int M9_esc_mwT = (0+0) * (1);
static const int M9_esc_mt = (0+0) * (1);
static const int M9_esc_lrm = (0+0) * (1);
static const int M9_esc_mbm = (0+0) * (1);
static const int M9_esc_rG = (0+0) * (1);
static const int M9_esc_mwG = (0+0) * (1);
static const int M9_esc_hA = (22+0) * (1);
static const int M9_esc_hB = (22+0) * (1);
static const int M9_esc_hC = (22+0) * (1);
static const int M9_esc_hD = (22+0) * (1);
static const int M9_esc_hE = (22+0) * (1);
static const int M9_esc_hH = (22+0) * (1);
static const int M9_esc_hI = (22+0) * (1);
static const int M9_esc_hJ = (22+0) * (1);
static const int M9_esc_hK = (22+0) * (1);
static const int M9_esc_hL = (22+0) * (1);
static const int M9_esc_hS = (22+0) * (1);
static const int M9_esc_hF = (22+0) * (1);
static const int M9_esc_hM = (22+0) * (1);
static const int M9_esc_hT = (22+0) * (1);
static const int M9_esc_hN = (22+0) * (1);


static const long long M9_esc_a_Volume = tc_vmul(1, 3); // 3
static const long long M9_esc_b_Volume = 1; // 1
static const long long M9_esc_c_Volume = tc_vmul(1, 3); // 3
static const long long M9_esc_d_Volume = 1; // 1
static const long long M9_esc_e_Volume = tc_vmul(1, 2); // 2
static const long long M9_esc_f_Volume = 1; // 1
static const long long M9_esc_g_Volume = 1; // 1
static const long long M9_esc_h_Volume = 1; // 1
static const long long M9_esc_i_Volume = tc_vmul(1, 2); // 2
static const long long M9_esc_j_Volume = 1; // 1
static const long long M9_esc_k_Volume = tc_vmul(1, 4); // 4
static const long long M9_esc_l_Volume = 1; // 1
static const long long M9_esc_m_Volume = tc_vmul(1, 3); // 3
static const long long M9_esc_n_Volume = 1; // 1
static const long long M9_esc_s_Volume = tc_vmul(1, 3); // 3
static const long long M9_esc_t_Volume = 1; // 1
static const long long M9_esc_w_Volume = tc_vmul(1, 3); // 3

#define M9_esc_USES(X) X(dm)

struct M9_esc_T {
  cm_cnt2 M9_esc_A[tc_rows(M9_esc_a_Volume, M9_esc_hA) * M_esc_ND];
  cm_cnt2 M9_esc_B[tc_rows(M9_esc_b_Volume, M9_esc_hB) * M_esc_ND];
  cm_cnt2 M9_esc_C[tc_rows(M9_esc_c_Volume, M9_esc_hC) * M_esc_ND];
  cm_cnt2 M9_esc_D[tc_rows(M9_esc_d_Volume, M9_esc_hD) * M_esc_ND];
  cm_cnt2 M9_esc_E[tc_rows(M9_esc_e_Volume, M9_esc_hE) * M_esc_ND];
  cm_cnt2 M9_esc_H[tc_rows(M9_esc_h_Volume, M9_esc_hH) * M_esc_ND];
  cm_cnt2 M9_esc_I[tc_rows(M9_esc_i_Volume, M9_esc_hI) * M_esc_ND];
  cm_cnt2 M9_esc_J[tc_rows(M9_esc_j_Volume, M9_esc_hJ) * M_esc_ND];
  cm_cnt2 M9_esc_K[tc_rows(M9_esc_k_Volume, M9_esc_hK) * M_esc_ND];
  cm_cnt2 M9_esc_L[tc_rows(M9_esc_l_Volume, M9_esc_hL) * M_esc_ND];
  cm_cnt M9_esc_MT[tc_rows(M9_esc_t_Volume, M9_esc_hT) * M_esc_MND];
  i16 M9_esc_S[tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * cm_apm_q(M9_esc_qs)];
  u8 M9_esc_SC[tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * cm_apm_q(M9_esc_qs)];
  i16 M9_esc_F[tc_rows(M9_esc_f_Volume, M9_esc_hF) * M_esc_ND * cm_apm_q(M9_esc_qf)];
  u8 M9_esc_FC[tc_rows(M9_esc_f_Volume, M9_esc_hF) * M_esc_ND * cm_apm_q(M9_esc_qf)];
  i32 M9_esc_W[tc_rows(M9_esc_m_Volume, M9_esc_hM) * M_esc_ND * tc_nin(10)];
  u8 M9_esc_WC[tc_rows(M9_esc_m_Volume, M9_esc_hM) * M_esc_ND];
  i32 M9_esc_WM[tc_rows(M9_esc_n_Volume, M9_esc_hN) * M_esc_MND * TC_MIN];
  u8 M9_esc_WMC[tc_rows(M9_esc_n_Volume, M9_esc_hN) * M_esc_MND];
  i32 M9_esc_W2[M9_esc_w_Volume * M_esc_ND * tc_nin(10)];
  u8 M9_esc_W2C[M9_esc_w_Volume * M_esc_ND];
  i32 M9_esc_W3[tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * tc_nin(10)];
  u8 M9_esc_W3C[tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND];
  i32 M9_esc_WF[M_esc_ND * HS_N * TC_FIN];
  u8 M9_esc_WFC[M_esc_ND * HS_N];
  cm_cnt M9_esc_G[M9_esc_g_Volume * M_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_a_Volume, M9_esc_hA) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_b_Volume, M9_esc_hB) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_c_Volume, M9_esc_hC) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_d_Volume, M9_esc_hD) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_e_Volume, M9_esc_hE) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_h_Volume, M9_esc_hH) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_i_Volume, M9_esc_hI) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_j_Volume, M9_esc_hJ) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_k_Volume, M9_esc_hK) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_esc_l_Volume, M9_esc_hL) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_esc_t_Volume, M9_esc_hT) * M_esc_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * cm_apm_q(M9_esc_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * cm_apm_q(M9_esc_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_esc_f_Volume, M9_esc_hF) * M_esc_ND * cm_apm_q(M9_esc_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_esc_f_Volume, M9_esc_hF) * M_esc_ND * cm_apm_q(M9_esc_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_esc_m_Volume, M9_esc_hM) * M_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_esc_m_Volume, M9_esc_hM) * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_esc_n_Volume, M9_esc_hN) * M_esc_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_esc_n_Volume, M9_esc_hN) * M_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_esc_w_Volume * M_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_esc_w_Volume * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_esc_s_Volume, M9_esc_hS) * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_esc_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_esc_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_esc_g_Volume * M_esc_NG);
  void M9_esc_Init( void ) {}
  void M9_esc_Quit( void ) {}
};
