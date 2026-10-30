

static const byte M9_dcs_b_ls[14+1]={ 0,1,2,3,4,5,5,5,6,7,7,7,7,7,7 };
static const byte M9_dcs_b_as[14+1]={ 0,1,2,3,4,5,5,6,6,7,7,7,7,7,7 };


static const byte M9_dcs_d_lbn[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5 };
static const byte M9_dcs_d_abn[17+1]={ 0,1,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4 };




static const byte M9_dcs_h_sz[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte M9_dcs_i_prev[13+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,7,7 };

static const byte M9_dcs_j_ls[14+1]={ 0,0,1,2,3,3,4,5,5,5,5,5,5,5,5 };
static const byte M9_dcs_j_prev[13+1]={ 0,0,0,1,1,2,3,4,4,4,4,4,4,4 };

static const byte M9_dcs_k_as[14+1]={ 0,0,1,2,3,3,4,4,4,5,5,5,5,5,5 };
static const byte M9_dcs_k_prev[13+1]={ 0,0,1,1,2,2,3,4,4,4,4,4,4,4 };

static const byte M9_dcs_l_ie[26+1]={ 0,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,6,6,6,6 };
static const byte M9_dcs_l_tq[64+1]={ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };







static const int M9_dcs_rA = (211+0) * (1);
static const int M9_dcs_rB = (255+0) * (1);
static const int M9_dcs_rC = (255+0) * (1);
static const int M9_dcs_rD = (255+0) * (1);
static const int M9_dcs_rE = (148+0) * (1);
static const int M9_dcs_rH = (254+0) * (1);
static const int M9_dcs_rI = (250+0) * (1);
static const int M9_dcs_rJ = (232+0) * (1);
static const int M9_dcs_rK = (240+0) * (1);
static const int M9_dcs_rL = (254+0) * (1);
static const int M9_dcs_rS1 = (8+0) * (1);
static const int M9_dcs_rS2 = (0+0) * (1);
static const int M9_dcs_lr = (31+0) * (1);
static const int M9_dcs_mwA = (129+0) * (1);
static const int M9_dcs_mwB = (8+0) * (1);
static const int M9_dcs_mwC = (87+0) * (1);
static const int M9_dcs_mwD = (23+0) * (1);
static const int M9_dcs_mwE = (79+0) * (1);
static const int M9_dcs_mwH = (64+0) * (1);
static const int M9_dcs_mwI = (112+0) * (1);
static const int M9_dcs_mwJ = (95+0) * (1);
static const int M9_dcs_mwK = (46+0) * (1);
static const int M9_dcs_mwL = (4+0) * (1);
static const int M9_dcs_bw = (16+0) * (1);
static const int M9_dcs_qs = (97+3) * (1);
static const int M9_dcs_qf = (0+3) * (1);
static const int M9_dcs_mb = (32+0) * (1);
static const int M9_dcs_rT = (247+0) * (1);
static const int M9_dcs_mwT = (0+0) * (1);
static const int M9_dcs_mt = (0+0) * (1);
static const int M9_dcs_lrm = (24+0) * (1);
static const int M9_dcs_mbm = (128+0) * (1);
static const int M9_dcs_rG = (0+0) * (1);
static const int M9_dcs_mwG = (0+0) * (1);
static const int M9_dcs_hA = (22+0) * (1);
static const int M9_dcs_hB = (22+0) * (1);
static const int M9_dcs_hC = (22+0) * (1);
static const int M9_dcs_hD = (22+0) * (1);
static const int M9_dcs_hE = (22+0) * (1);
static const int M9_dcs_hH = (22+0) * (1);
static const int M9_dcs_hI = (22+0) * (1);
static const int M9_dcs_hJ = (22+0) * (1);
static const int M9_dcs_hK = (22+0) * (1);
static const int M9_dcs_hL = (22+0) * (1);
static const int M9_dcs_hS = (22+0) * (1);
static const int M9_dcs_hF = (22+0) * (1);
static const int M9_dcs_hM = (22+0) * (1);
static const int M9_dcs_hT = (22+0) * (1);
static const int M9_dcs_hN = (22+0) * (1);


static const long long M9_dcs_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 3); // 6
static const long long M9_dcs_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 8); // 128
static const long long M9_dcs_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long M9_dcs_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 5); // 30
static const long long M9_dcs_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3), 3); // 54
static const long long M9_dcs_f_Volume = tc_vmul(1, 2); // 2
static const long long M9_dcs_g_Volume = 1; // 1
static const long long M9_dcs_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 13), 3); // 78
static const long long M9_dcs_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 2); // 32
static const long long M9_dcs_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 5); // 60
static const long long M9_dcs_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 5); // 60
static const long long M9_dcs_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 6); // 42
static const long long M9_dcs_m_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long M9_dcs_n_Volume = tc_vmul(1, 1); // 1
static const long long M9_dcs_s_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long M9_dcs_t_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M9_dcs_w_Volume = tc_vmul(tc_vmul(1, 2), 3); // 6

#define M9_dcs_USES(X) X(lc) X(es) X(ls) X(as) X(coded) X(qs) X(lbn) X(abn) X(ls2) X(as2) X(psg) X(sz) X(prev) X(ie) X(tq)

struct M9_dcs_T {
  cm_cnt2 M9_dcs_A[tc_rows(M9_dcs_a_Volume, M9_dcs_hA) * M_dcs_ND];
  cm_cnt2 M9_dcs_B[tc_rows(M9_dcs_b_Volume, M9_dcs_hB) * M_dcs_ND];
  cm_cnt2 M9_dcs_C[tc_rows(M9_dcs_c_Volume, M9_dcs_hC) * M_dcs_ND];
  cm_cnt2 M9_dcs_D[tc_rows(M9_dcs_d_Volume, M9_dcs_hD) * M_dcs_ND];
  cm_cnt2 M9_dcs_E[tc_rows(M9_dcs_e_Volume, M9_dcs_hE) * M_dcs_ND];
  cm_cnt2 M9_dcs_H[tc_rows(M9_dcs_h_Volume, M9_dcs_hH) * M_dcs_ND];
  cm_cnt2 M9_dcs_I[tc_rows(M9_dcs_i_Volume, M9_dcs_hI) * M_dcs_ND];
  cm_cnt2 M9_dcs_J[tc_rows(M9_dcs_j_Volume, M9_dcs_hJ) * M_dcs_ND];
  cm_cnt2 M9_dcs_K[tc_rows(M9_dcs_k_Volume, M9_dcs_hK) * M_dcs_ND];
  cm_cnt2 M9_dcs_L[tc_rows(M9_dcs_l_Volume, M9_dcs_hL) * M_dcs_ND];
  cm_cnt M9_dcs_MT[tc_rows(M9_dcs_t_Volume, M9_dcs_hT) * M_dcs_MND];
  i16 M9_dcs_S[tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * cm_apm_q(M9_dcs_qs)];
  u8 M9_dcs_SC[tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * cm_apm_q(M9_dcs_qs)];
  i16 M9_dcs_F[tc_rows(M9_dcs_f_Volume, M9_dcs_hF) * M_dcs_ND * cm_apm_q(M9_dcs_qf)];
  u8 M9_dcs_FC[tc_rows(M9_dcs_f_Volume, M9_dcs_hF) * M_dcs_ND * cm_apm_q(M9_dcs_qf)];
  i32 M9_dcs_W[tc_rows(M9_dcs_m_Volume, M9_dcs_hM) * M_dcs_ND * tc_nin(10)];
  u8 M9_dcs_WC[tc_rows(M9_dcs_m_Volume, M9_dcs_hM) * M_dcs_ND];
  i32 M9_dcs_WM[tc_rows(M9_dcs_n_Volume, M9_dcs_hN) * M_dcs_MND * TC_MIN];
  u8 M9_dcs_WMC[tc_rows(M9_dcs_n_Volume, M9_dcs_hN) * M_dcs_MND];
  i32 M9_dcs_W2[M9_dcs_w_Volume * M_dcs_ND * tc_nin(10)];
  u8 M9_dcs_W2C[M9_dcs_w_Volume * M_dcs_ND];
  i32 M9_dcs_W3[tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * tc_nin(10)];
  u8 M9_dcs_W3C[tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND];
  i32 M9_dcs_WF[M_dcs_ND * HS_N * TC_FIN];
  u8 M9_dcs_WFC[M_dcs_ND * HS_N];
  cm_cnt M9_dcs_G[M9_dcs_g_Volume * M_dcs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_dcs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_a_Volume, M9_dcs_hA) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_b_Volume, M9_dcs_hB) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_c_Volume, M9_dcs_hC) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_d_Volume, M9_dcs_hD) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_e_Volume, M9_dcs_hE) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_h_Volume, M9_dcs_hH) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_i_Volume, M9_dcs_hI) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_j_Volume, M9_dcs_hJ) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_k_Volume, M9_dcs_hK) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_dcs_l_Volume, M9_dcs_hL) * M_dcs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_dcs_t_Volume, M9_dcs_hT) * M_dcs_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * cm_apm_q(M9_dcs_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * cm_apm_q(M9_dcs_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_dcs_f_Volume, M9_dcs_hF) * M_dcs_ND * cm_apm_q(M9_dcs_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_dcs_f_Volume, M9_dcs_hF) * M_dcs_ND * cm_apm_q(M9_dcs_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_dcs_m_Volume, M9_dcs_hM) * M_dcs_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_dcs_m_Volume, M9_dcs_hM) * M_dcs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_dcs_n_Volume, M9_dcs_hN) * M_dcs_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_dcs_n_Volume, M9_dcs_hN) * M_dcs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_dcs_w_Volume * M_dcs_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_dcs_w_Volume * M_dcs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_dcs_s_Volume, M9_dcs_hS) * M_dcs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_dcs_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_dcs_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_dcs_g_Volume * M_dcs_NG);
  void M9_dcs_Init( void ) {}
  void M9_dcs_Quit( void ) {}
};
