

static const byte M9_skp_b_run[16+1]={ 0,1,1,2,3,3,3,4,5,5,5,5,5,5,6,6,6 };

static const byte M9_skp_c_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };






static const byte M9_skp_i_st[3+1]={ 0,1,2,3 };
static const byte M9_skp_i_run[16+1]={ 0,1,2,2,2,2,2,2,2,2,2,2,2,3,3,4,4 };
static const byte M9_skp_i_c4[4+1]={ 0,1,2,3,4 };

static const byte M9_skp_j_st[3+1]={ 0,1,2,3 };
static const byte M9_skp_j_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte M9_skp_j_run[16+1]={ 0,1,2,3,4,5,6,7,8,8,9,10,10,11,12,13,13 };

static const byte M9_skp_k_c4[4+1]={ 0,1,2,3,4 };

static const byte M9_skp_l_st[3+1]={ 0,1,2,3 };
static const byte M9_skp_l_zt[24+1]={ 0,0,0,0,0,1,1,1,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
static const byte M9_skp_l_ze[24+1]={ 0,1,2,3,4,4,5,6,6,6,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8 };

static const byte M9_skp_m_ze[24+1]={ 0,1,2,3,4,5,6,7,7,7,7,8,8,8,8,9,10,11,12,13,13,13,13,13,13 };






static const int M9_skp_rA = (0+0) * (1);
static const int M9_skp_rB = (157+0) * (1);
static const int M9_skp_rC = (254+0) * (1);
static const int M9_skp_rD = (254+0) * (1);
static const int M9_skp_rE = (153+0) * (1);
static const int M9_skp_rH = (255+0) * (1);
static const int M9_skp_rI = (249+0) * (1);
static const int M9_skp_rJ = (253+0) * (1);
static const int M9_skp_rK = (253+0) * (1);
static const int M9_skp_rL = (254+0) * (1);
static const int M9_skp_rS1 = (6+0) * (1);
static const int M9_skp_rS2 = (6+0) * (1);
static const int M9_skp_lr = (15+0) * (1);
static const int M9_skp_mwA = (203+0) * (1);
static const int M9_skp_mwB = (80+0) * (1);
static const int M9_skp_mwC = (45+0) * (1);
static const int M9_skp_mwD = (1+0) * (1);
static const int M9_skp_mwE = (5+0) * (1);
static const int M9_skp_mwH = (128+0) * (1);
static const int M9_skp_mwI = (65+0) * (1);
static const int M9_skp_mwJ = (16+0) * (1);
static const int M9_skp_mwK = (31+0) * (1);
static const int M9_skp_mwL = (1+0) * (1);
static const int M9_skp_bw = (16+0) * (1);
static const int M9_skp_qs = (106+3) * (1);
static const int M9_skp_qf = (65+3) * (1);
static const int M9_skp_mb = (41+0) * (1);
static const int M9_skp_rT = (0+0) * (1);
static const int M9_skp_mwT = (0+0) * (1);
static const int M9_skp_mt = (0+0) * (1);
static const int M9_skp_lrm = (0+0) * (1);
static const int M9_skp_mbm = (0+0) * (1);
static const int M9_skp_rG = (0+0) * (1);
static const int M9_skp_mwG = (0+0) * (1);
static const int M9_skp_hA = (22+0) * (1);
static const int M9_skp_hB = (22+0) * (1);
static const int M9_skp_hC = (22+0) * (1);
static const int M9_skp_hD = (22+0) * (1);
static const int M9_skp_hE = (22+0) * (1);
static const int M9_skp_hH = (22+0) * (1);
static const int M9_skp_hI = (22+0) * (1);
static const int M9_skp_hJ = (22+0) * (1);
static const int M9_skp_hK = (22+0) * (1);
static const int M9_skp_hL = (22+0) * (1);
static const int M9_skp_hS = (22+0) * (1);
static const int M9_skp_hF = (22+0) * (1);
static const int M9_skp_hM = (22+0) * (1);
static const int M9_skp_hT = (22+0) * (1);
static const int M9_skp_hN = (22+0) * (1);


static const long long M9_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M9_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 7); // 7
static const long long M9_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 1); // 22
static const long long M9_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M9_skp_e_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3
static const long long M9_skp_f_Volume = tc_vmul(1, 1); // 1
static const long long M9_skp_g_Volume = 1; // 1
static const long long M9_skp_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M9_skp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 5), 3); // 300
static const long long M9_skp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 11), 14); // 616
static const long long M9_skp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 5), 3); // 45
static const long long M9_skp_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 9); // 144
static const long long M9_skp_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 14); // 42
static const long long M9_skp_n_Volume = 1; // 1
static const long long M9_skp_s_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M9_skp_t_Volume = 1; // 1
static const long long M9_skp_w_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3

#define M9_skp_USES(X) X(cl) X(rt) X(run) X(st) X(rc) X(pz) X(rz) X(bsk) X(rr) X(c4) X(zt) X(ze)

struct M9_skp_T {
  cm_cnt2 M9_skp_A[tc_rows(M9_skp_a_Volume, M9_skp_hA) * M_skp_ND];
  cm_cnt2 M9_skp_B[tc_rows(M9_skp_b_Volume, M9_skp_hB) * M_skp_ND];
  cm_cnt2 M9_skp_C[tc_rows(M9_skp_c_Volume, M9_skp_hC) * M_skp_ND];
  cm_cnt2 M9_skp_D[tc_rows(M9_skp_d_Volume, M9_skp_hD) * M_skp_ND];
  cm_cnt2 M9_skp_E[tc_rows(M9_skp_e_Volume, M9_skp_hE) * M_skp_ND];
  cm_cnt2 M9_skp_H[tc_rows(M9_skp_h_Volume, M9_skp_hH) * M_skp_ND];
  cm_cnt2 M9_skp_I[tc_rows(M9_skp_i_Volume, M9_skp_hI) * M_skp_ND];
  cm_cnt2 M9_skp_J[tc_rows(M9_skp_j_Volume, M9_skp_hJ) * M_skp_ND];
  cm_cnt2 M9_skp_K[tc_rows(M9_skp_k_Volume, M9_skp_hK) * M_skp_ND];
  cm_cnt2 M9_skp_L[tc_rows(M9_skp_l_Volume, M9_skp_hL) * M_skp_ND];
  cm_cnt M9_skp_MT[tc_rows(M9_skp_t_Volume, M9_skp_hT) * M_skp_MND];
  i16 M9_skp_S[tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * cm_apm_q(M9_skp_qs)];
  u8 M9_skp_SC[tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * cm_apm_q(M9_skp_qs)];
  i16 M9_skp_F[tc_rows(M9_skp_f_Volume, M9_skp_hF) * M_skp_ND * cm_apm_q(M9_skp_qf)];
  u8 M9_skp_FC[tc_rows(M9_skp_f_Volume, M9_skp_hF) * M_skp_ND * cm_apm_q(M9_skp_qf)];
  i32 M9_skp_W[tc_rows(M9_skp_m_Volume, M9_skp_hM) * M_skp_ND * tc_nin(10)];
  u8 M9_skp_WC[tc_rows(M9_skp_m_Volume, M9_skp_hM) * M_skp_ND];
  i32 M9_skp_WM[tc_rows(M9_skp_n_Volume, M9_skp_hN) * M_skp_MND * TC_MIN];
  u8 M9_skp_WMC[tc_rows(M9_skp_n_Volume, M9_skp_hN) * M_skp_MND];
  i32 M9_skp_W2[M9_skp_w_Volume * M_skp_ND * tc_nin(10)];
  u8 M9_skp_W2C[M9_skp_w_Volume * M_skp_ND];
  i32 M9_skp_W3[tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * tc_nin(10)];
  u8 M9_skp_W3C[tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND];
  i32 M9_skp_WF[M_skp_ND * HS_N * TC_FIN];
  u8 M9_skp_WFC[M_skp_ND * HS_N];
  cm_cnt M9_skp_G[M9_skp_g_Volume * M_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_a_Volume, M9_skp_hA) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_b_Volume, M9_skp_hB) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_c_Volume, M9_skp_hC) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_d_Volume, M9_skp_hD) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_e_Volume, M9_skp_hE) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_h_Volume, M9_skp_hH) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_i_Volume, M9_skp_hI) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_j_Volume, M9_skp_hJ) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_k_Volume, M9_skp_hK) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_skp_l_Volume, M9_skp_hL) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_skp_t_Volume, M9_skp_hT) * M_skp_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * cm_apm_q(M9_skp_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * cm_apm_q(M9_skp_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_skp_f_Volume, M9_skp_hF) * M_skp_ND * cm_apm_q(M9_skp_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_skp_f_Volume, M9_skp_hF) * M_skp_ND * cm_apm_q(M9_skp_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_skp_m_Volume, M9_skp_hM) * M_skp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_skp_m_Volume, M9_skp_hM) * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_skp_n_Volume, M9_skp_hN) * M_skp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_skp_n_Volume, M9_skp_hN) * M_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_skp_w_Volume * M_skp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_skp_w_Volume * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_skp_s_Volume, M9_skp_hS) * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_skp_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_skp_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_skp_g_Volume * M_skp_NG);
  void M9_skp_Init( void ) {}
  void M9_skp_Quit( void ) {}
};
