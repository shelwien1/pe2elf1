



static const byte M7_qpd_d_qs[32+1]={ 0,0,0,0,0,1,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte M7_qpd_e_qs[32+1]={ 0,0,0,0,0,1,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };




static const byte M7_qpd_i_qs[32+1]={ 0,0,0,0,0,1,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };



static const byte M7_qpd_l_qs[32+1]={ 0,0,0,0,0,1,1,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };







static const int M7_qpd_rA = (94+0) * (1);
static const int M7_qpd_mwA = (21+0) * (1);
static const int M7_qpd_rB = (183+0) * (1);
static const int M7_qpd_mwB = (11+0) * (1);
static const int M7_qpd_rC = (243+0) * (1);
static const int M7_qpd_mwC = (7+0) * (1);
static const int M7_qpd_rD = (0+0) * (1);
static const int M7_qpd_mwD = (10+0) * (1);
static const int M7_qpd_rE = (81+0) * (1);
static const int M7_qpd_mwE = (9+0) * (1);
static const int M7_qpd_rH = (163+0) * (1);
static const int M7_qpd_mwH = (132+0) * (1);
static const int M7_qpd_rI = (2+0) * (1);
static const int M7_qpd_mwI = (0+0) * (1);
static const int M7_qpd_rJ = (166+0) * (1);
static const int M7_qpd_mwJ = (0+0) * (1);
static const int M7_qpd_rK = (201+0) * (1);
static const int M7_qpd_mwK = (130+0) * (1);
static const int M7_qpd_rL = (0+0) * (1);
static const int M7_qpd_mwL = (0+0) * (1);
static const int M7_qpd_lr = (29+0) * (1);
static const int M7_qpd_mb = (15+0) * (1);
static const int M7_qpd_rS1 = (8+0) * (1);
static const int M7_qpd_rS2 = (4+0) * (1);
static const int M7_qpd_qs = (5+3) * (1);
static const int M7_qpd_qf = (57+3) * (1);
static const int M7_qpd_bw = (8+0) * (1);
static const int M7_qpd_rT = (0+0) * (1);
static const int M7_qpd_mwT = (0+0) * (1);
static const int M7_qpd_mt = (0+0) * (1);
static const int M7_qpd_lrm = (0+0) * (1);
static const int M7_qpd_mbm = (0+0) * (1);
static const int M7_qpd_rG = (0+0) * (1);
static const int M7_qpd_mwG = (0+0) * (1);
static const int M7_qpd_hA = (22+0) * (1);
static const int M7_qpd_hB = (22+0) * (1);
static const int M7_qpd_hC = (22+0) * (1);
static const int M7_qpd_hD = (22+0) * (1);
static const int M7_qpd_hE = (22+0) * (1);
static const int M7_qpd_hH = (22+0) * (1);
static const int M7_qpd_hI = (22+0) * (1);
static const int M7_qpd_hJ = (22+0) * (1);
static const int M7_qpd_hK = (22+0) * (1);
static const int M7_qpd_hL = (22+0) * (1);
static const int M7_qpd_hM = (22+0) * (1);
static const int M7_qpd_hT = (22+0) * (1);
static const int M7_qpd_hN = (22+0) * (1);
static const int M7_qpd_hS = (22+0) * (1);
static const int M7_qpd_hF = (22+0) * (1);


static const long long M7_qpd_a_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M7_qpd_b_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M7_qpd_c_Volume = tc_vmul(1, 1); // 1
static const long long M7_qpd_d_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long M7_qpd_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 4); // 24
static const long long M7_qpd_f_Volume = tc_vmul(1, 1); // 1
static const long long M7_qpd_g_Volume = 1; // 1
static const long long M7_qpd_h_Volume = tc_vmul(1, 2); // 2
static const long long M7_qpd_i_Volume = tc_vmul(1, 4); // 4
static const long long M7_qpd_j_Volume = 1; // 1
static const long long M7_qpd_k_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M7_qpd_l_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long M7_qpd_m_Volume = tc_vmul(1, 3); // 3
static const long long M7_qpd_n_Volume = 1; // 1
static const long long M7_qpd_s_Volume = tc_vmul(1, 1); // 1
static const long long M7_qpd_t_Volume = 1; // 1
static const long long M7_qpd_w_Volume = tc_vmul(1, 2); // 2

#define M7_qpd_USES(X) X(c3) X(pdq) X(qs)

struct M7_qpd_T {
  cm_cnt2 M7_qpd_A[tc_rows(M7_qpd_a_Volume, M7_qpd_hA) * M_qpd_ND];
  cm_cnt2 M7_qpd_B[tc_rows(M7_qpd_b_Volume, M7_qpd_hB) * M_qpd_ND];
  cm_cnt2 M7_qpd_C[tc_rows(M7_qpd_c_Volume, M7_qpd_hC) * M_qpd_ND];
  cm_cnt2 M7_qpd_D[tc_rows(M7_qpd_d_Volume, M7_qpd_hD) * M_qpd_ND];
  cm_cnt2 M7_qpd_E[tc_rows(M7_qpd_e_Volume, M7_qpd_hE) * M_qpd_ND];
  cm_cnt2 M7_qpd_H[tc_rows(M7_qpd_h_Volume, M7_qpd_hH) * M_qpd_ND];
  cm_cnt2 M7_qpd_I[tc_rows(M7_qpd_i_Volume, M7_qpd_hI) * M_qpd_ND];
  cm_cnt2 M7_qpd_J[tc_rows(M7_qpd_j_Volume, M7_qpd_hJ) * M_qpd_ND];
  cm_cnt2 M7_qpd_K[tc_rows(M7_qpd_k_Volume, M7_qpd_hK) * M_qpd_ND];
  cm_cnt2 M7_qpd_L[tc_rows(M7_qpd_l_Volume, M7_qpd_hL) * M_qpd_ND];
  cm_cnt M7_qpd_MT[tc_rows(M7_qpd_t_Volume, M7_qpd_hT) * M_qpd_MND];
  i16 M7_qpd_S[tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * cm_apm_q(M7_qpd_qs)];
  u8 M7_qpd_SC[tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * cm_apm_q(M7_qpd_qs)];
  i16 M7_qpd_F[tc_rows(M7_qpd_f_Volume, M7_qpd_hF) * M_qpd_ND * cm_apm_q(M7_qpd_qf)];
  u8 M7_qpd_FC[tc_rows(M7_qpd_f_Volume, M7_qpd_hF) * M_qpd_ND * cm_apm_q(M7_qpd_qf)];
  i32 M7_qpd_W[tc_rows(M7_qpd_m_Volume, M7_qpd_hM) * M_qpd_ND * tc_nin(10)];
  u8 M7_qpd_WC[tc_rows(M7_qpd_m_Volume, M7_qpd_hM) * M_qpd_ND];
  i32 M7_qpd_WM[tc_rows(M7_qpd_n_Volume, M7_qpd_hN) * M_qpd_MND * TC_MIN];
  u8 M7_qpd_WMC[tc_rows(M7_qpd_n_Volume, M7_qpd_hN) * M_qpd_MND];
  i32 M7_qpd_W2[M7_qpd_w_Volume * M_qpd_ND * tc_nin(10)];
  u8 M7_qpd_W2C[M7_qpd_w_Volume * M_qpd_ND];
  i32 M7_qpd_W3[tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * tc_nin(10)];
  u8 M7_qpd_W3C[tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND];
  i32 M7_qpd_WF[M_qpd_ND * HS_N * TC_FIN];
  u8 M7_qpd_WFC[M_qpd_ND * HS_N];
  cm_cnt M7_qpd_G[M7_qpd_g_Volume * M_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M7_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_a_Volume, M7_qpd_hA) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_b_Volume, M7_qpd_hB) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_c_Volume, M7_qpd_hC) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_d_Volume, M7_qpd_hD) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_e_Volume, M7_qpd_hE) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_h_Volume, M7_qpd_hH) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_i_Volume, M7_qpd_hI) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_j_Volume, M7_qpd_hJ) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_k_Volume, M7_qpd_hK) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_qpd_l_Volume, M7_qpd_hL) * M_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M7_qpd_t_Volume, M7_qpd_hT) * M_qpd_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * cm_apm_q(M7_qpd_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * cm_apm_q(M7_qpd_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_qpd_f_Volume, M7_qpd_hF) * M_qpd_ND * cm_apm_q(M7_qpd_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_qpd_f_Volume, M7_qpd_hF) * M_qpd_ND * cm_apm_q(M7_qpd_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_qpd_m_Volume, M7_qpd_hM) * M_qpd_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_qpd_m_Volume, M7_qpd_hM) * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_qpd_n_Volume, M7_qpd_hN) * M_qpd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_qpd_n_Volume, M7_qpd_hN) * M_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M7_qpd_w_Volume * M_qpd_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M7_qpd_w_Volume * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_qpd_s_Volume, M7_qpd_hS) * M_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_qpd_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_qpd_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M7_qpd_g_Volume * M_qpd_NG);
  void M7_qpd_Init( void ) {}
  void M7_qpd_Quit( void ) {}
};
