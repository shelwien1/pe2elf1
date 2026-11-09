
static const byte D9_scl_a_dp[3+1]={ 0,1,2,3 };

static const byte D9_scl_b_pn[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte D9_scl_b_dp[3+1]={ 0,1,2,3 };

static const byte D9_scl_c_ab[27+1]={ 0,1,1,2,2,2,2,2,2,2,2,3,4,5,6,6,6,7,8,9,9,9,9,9,9,9,9,9 };

static const byte D9_scl_d_lp[9+1]={ 0,1,2,3,4,5,6,6,6,6 };


static const byte D9_scl_f_ab[27+1]={ 0,0,1,2,2,3,3,3,3,4,4,5,5,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };

static const byte D9_scl_g_kd[3+1]={ 0,1,2,3 };

static const byte D9_scl_h_pn[7+1]={ 0,1,2,3,3,4,5,6 };
static const byte D9_scl_h_ab[27+1]={ 0,0,0,1,1,2,2,3,4,4,5,6,6,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9 };

static const byte D9_scl_i_pn[7+1]={ 0,1,2,3,3,4,4,5 };

static const byte D9_scl_j_bd[31+1]={ 0,0,1,2,2,3,3,3,4,5,5,5,5,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8 };
static const byte D9_scl_j_nb[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte D9_scl_k_lp[9+1]={ 0,0,0,1,2,3,3,3,4,5 };

static const byte D9_scl_l_kd[3+1]={ 0,1,2,3 };
static const byte D9_scl_l_ab[27+1]={ 0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,3,3,4,5,6,6,6,6,7,7,8,8,8 };


static const byte D9_scl_n_kd[3+1]={ 0,1,2,3 };

static const byte D9_scl_s_kd[3+1]={ 0,1,2,3 };
static const byte D9_scl_s_pn[7+1]={ 0,1,2,2,2,2,2,3 };
static const byte D9_scl_s_dp[3+1]={ 0,1,2,3 };




static const int D9_scl_rA = (0+0) * (1);
static const int D9_scl_mwA = (33+0) * (1);
static const int D9_scl_rB = (33+0) * (1);
static const int D9_scl_mwB = (2+0) * (1);
static const int D9_scl_rC = (255+0) * (1);
static const int D9_scl_mwC = (66+0) * (1);
static const int D9_scl_rD = (253+0) * (1);
static const int D9_scl_mwD = (226+0) * (1);
static const int D9_scl_rE = (254+0) * (1);
static const int D9_scl_mwE = (209+0) * (1);
static const int D9_scl_rH = (144+0) * (1);
static const int D9_scl_mwH = (0+0) * (1);
static const int D9_scl_rI = (244+0) * (1);
static const int D9_scl_mwI = (0+0) * (1);
static const int D9_scl_rJ = (240+0) * (1);
static const int D9_scl_mwJ = (1+0) * (1);
static const int D9_scl_rK = (220+0) * (1);
static const int D9_scl_mwK = (129+0) * (1);
static const int D9_scl_rL = (236+0) * (1);
static const int D9_scl_mwL = (131+0) * (1);
static const int D9_scl_lr = (15+0) * (1);
static const int D9_scl_mb = (72+0) * (1);
static const int D9_scl_rS1 = (16+0) * (1);
static const int D9_scl_rS2 = (8+0) * (1);
static const int D9_scl_qs = (64+3) * (1);
static const int D9_scl_qf = (28+3) * (1);
static const int D9_scl_bw = (9+0) * (1);
static const int D9_scl_rT = (8+0) * (1);
static const int D9_scl_mwT = (161+0) * (1);
static const int D9_scl_mt = (4+0) * (1);
static const int D9_scl_lrm = (31+0) * (1);
static const int D9_scl_mbm = (25+0) * (1);
static const int D9_scl_rG = (12+0) * (1);
static const int D9_scl_mwG = (32+0) * (1);
static const int D9_scl_hA = (22+0) * (1);
static const int D9_scl_hB = (22+0) * (1);
static const int D9_scl_hC = (22+0) * (1);
static const int D9_scl_hD = (22+0) * (1);
static const int D9_scl_hE = (22+0) * (1);
static const int D9_scl_hH = (22+0) * (1);
static const int D9_scl_hI = (22+0) * (1);
static const int D9_scl_hJ = (22+0) * (1);
static const int D9_scl_hK = (22+0) * (1);
static const int D9_scl_hL = (22+0) * (1);
static const int D9_scl_hS = (22+0) * (1);
static const int D9_scl_hF = (22+0) * (1);
static const int D9_scl_hM = (22+0) * (1);
static const int D9_scl_hT = (22+0) * (1);
static const int D9_scl_hN = (22+0) * (1);


static const long long D9_scl_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 4); // 16
static const long long D9_scl_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 8), 4); // 192
static const long long D9_scl_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 10); // 40
static const long long D9_scl_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 7); // 42
static const long long D9_scl_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 2); // 12
static const long long D9_scl_f_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 8); // 48
static const long long D9_scl_g_Volume = tc_vmul(1, 4); // 4
static const long long D9_scl_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 7), 10); // 140
static const long long D9_scl_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 2), 6); // 72
static const long long D9_scl_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 9), 3), 10); // 540
static const long long D9_scl_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 6), 3); // 36
static const long long D9_scl_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 1), 9); // 72
static const long long D9_scl_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 1); // 6
static const long long D9_scl_n_Volume = tc_vmul(1, 4); // 4
static const long long D9_scl_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 4), 4), 3); // 384
static const long long D9_scl_t_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long D9_scl_w_Volume = tc_vmul(1, 3); // 3

#define D9_scl_USES(X) X(kd) X(pk) X(dp) X(pn) X(ab) X(lp) X(hm) X(pm) X(bd) X(cc) X(nb)

struct D9_scl_T {
  cm_cnt2 D9_scl_A[tc_rows(D9_scl_a_Volume, D9_scl_hA) * M_scl_ND];
  cm_cnt2 D9_scl_B[tc_rows(D9_scl_b_Volume, D9_scl_hB) * M_scl_ND];
  cm_cnt2 D9_scl_C[tc_rows(D9_scl_c_Volume, D9_scl_hC) * M_scl_ND];
  cm_cnt2 D9_scl_D[tc_rows(D9_scl_d_Volume, D9_scl_hD) * M_scl_ND];
  cm_cnt2 D9_scl_E[tc_rows(D9_scl_e_Volume, D9_scl_hE) * M_scl_ND];
  cm_cnt2 D9_scl_H[tc_rows(D9_scl_h_Volume, D9_scl_hH) * M_scl_ND];
  cm_cnt2 D9_scl_I[tc_rows(D9_scl_i_Volume, D9_scl_hI) * M_scl_ND];
  cm_cnt2 D9_scl_J[tc_rows(D9_scl_j_Volume, D9_scl_hJ) * M_scl_ND];
  cm_cnt2 D9_scl_K[tc_rows(D9_scl_k_Volume, D9_scl_hK) * M_scl_ND];
  cm_cnt2 D9_scl_L[tc_rows(D9_scl_l_Volume, D9_scl_hL) * M_scl_ND];
  cm_cnt D9_scl_MT[tc_rows(D9_scl_t_Volume, D9_scl_hT) * M_scl_MND];
  i16 D9_scl_S[tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * cm_apm_q(D9_scl_qs)];
  u8 D9_scl_SC[tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * cm_apm_q(D9_scl_qs)];
  i16 D9_scl_F[tc_rows(D9_scl_f_Volume, D9_scl_hF) * M_scl_ND * cm_apm_q(D9_scl_qf)];
  u8 D9_scl_FC[tc_rows(D9_scl_f_Volume, D9_scl_hF) * M_scl_ND * cm_apm_q(D9_scl_qf)];
  i32 D9_scl_W[tc_rows(D9_scl_m_Volume, D9_scl_hM) * M_scl_ND * tc_nin(10)];
  u8 D9_scl_WC[tc_rows(D9_scl_m_Volume, D9_scl_hM) * M_scl_ND];
  i32 D9_scl_WM[tc_rows(D9_scl_n_Volume, D9_scl_hN) * M_scl_MND * TC_MIN];
  u8 D9_scl_WMC[tc_rows(D9_scl_n_Volume, D9_scl_hN) * M_scl_MND];
  i32 D9_scl_W2[D9_scl_w_Volume * M_scl_ND * tc_nin(10)];
  u8 D9_scl_W2C[D9_scl_w_Volume * M_scl_ND];
  i32 D9_scl_W3[tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * tc_nin(10)];
  u8 D9_scl_W3C[tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND];
  i32 D9_scl_WF[M_scl_ND * HS_N * TC_FIN];
  u8 D9_scl_WFC[M_scl_ND * HS_N];
  cm_cnt D9_scl_G[D9_scl_g_Volume * M_scl_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D9_scl_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_a_Volume, D9_scl_hA) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_b_Volume, D9_scl_hB) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_c_Volume, D9_scl_hC) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_d_Volume, D9_scl_hD) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_e_Volume, D9_scl_hE) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_h_Volume, D9_scl_hH) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_i_Volume, D9_scl_hI) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_j_Volume, D9_scl_hJ) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_k_Volume, D9_scl_hK) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_scl_l_Volume, D9_scl_hL) * M_scl_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D9_scl_t_Volume, D9_scl_hT) * M_scl_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * cm_apm_q(D9_scl_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * cm_apm_q(D9_scl_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(D9_scl_f_Volume, D9_scl_hF) * M_scl_ND * cm_apm_q(D9_scl_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_scl_f_Volume, D9_scl_hF) * M_scl_ND * cm_apm_q(D9_scl_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_scl_m_Volume, D9_scl_hM) * M_scl_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_scl_m_Volume, D9_scl_hM) * M_scl_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_scl_n_Volume, D9_scl_hN) * M_scl_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_scl_n_Volume, D9_scl_hN) * M_scl_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D9_scl_w_Volume * M_scl_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D9_scl_w_Volume * M_scl_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_scl_s_Volume, D9_scl_hS) * M_scl_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_scl_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_scl_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D9_scl_g_Volume * M_scl_NG);
  void D9_scl_Init( void ) {}
  void D9_scl_Quit( void ) {}
};
