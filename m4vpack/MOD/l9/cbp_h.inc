
static const byte M9_cbp_a_lu[8+1]={ 0,1,1,1,2,2,3,4,4 };


static const byte M9_cbp_c_cnt[5+1]={ 0,1,2,3,3,4 };

static const byte M9_cbp_d_frac[4+1]={ 0,1,2,3,4 };





static const byte M9_cbp_i_lu[8+1]={ 0,1,2,2,3,4,5,6,6 };

static const byte M9_cbp_j_lu[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M9_cbp_j_mvc[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M9_cbp_k_lu[8+1]={ 0,1,2,3,4,5,5,6,6 };

static const byte M9_cbp_l_ie[17+1]={ 0,0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };
static const byte M9_cbp_l_tq[64+1]={ 0,0,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
static const byte M9_cbp_l_tx[24+1]={ 0,0,1,2,2,3,3,4,4,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };
static const byte M9_cbp_l_em[24+1]={ 0,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M9_cbp_m_em[24+1]={ 0,0,1,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };




static const byte M9_cbp_w_lu[8+1]={ 0,1,2,3,3,3,4,4,4 };


static const int M9_cbp_rA = (129+0) * (1);
static const int M9_cbp_rB = (48+0) * (1);
static const int M9_cbp_rC = (255+0) * (1);
static const int M9_cbp_rD = (252+0) * (1);
static const int M9_cbp_rE = (248+0) * (1);
static const int M9_cbp_rH = (255+0) * (1);
static const int M9_cbp_rI = (254+0) * (1);
static const int M9_cbp_rJ = (253+0) * (1);
static const int M9_cbp_rK = (255+0) * (1);
static const int M9_cbp_rL = (255+0) * (1);
static const int M9_cbp_rS1 = (8+0) * (1);
static const int M9_cbp_rS2 = (16+0) * (1);
static const int M9_cbp_lr = (15+0) * (1);
static const int M9_cbp_mwA = (254+0) * (1);
static const int M9_cbp_mwB = (6+0) * (1);
static const int M9_cbp_mwC = (6+0) * (1);
static const int M9_cbp_mwD = (62+0) * (1);
static const int M9_cbp_mwE = (112+0) * (1);
static const int M9_cbp_mwH = (64+0) * (1);
static const int M9_cbp_mwI = (118+0) * (1);
static const int M9_cbp_mwJ = (98+0) * (1);
static const int M9_cbp_mwK = (45+0) * (1);
static const int M9_cbp_mwL = (4+0) * (1);
static const int M9_cbp_bw = (15+0) * (1);
static const int M9_cbp_qs = (118+3) * (1);
static const int M9_cbp_qf = (78+3) * (1);
static const int M9_cbp_mb = (31+0) * (1);
static const int M9_cbp_rT = (0+0) * (1);
static const int M9_cbp_mwT = (0+0) * (1);
static const int M9_cbp_mt = (0+0) * (1);
static const int M9_cbp_lrm = (0+0) * (1);
static const int M9_cbp_mbm = (0+0) * (1);
static const int M9_cbp_rG = (0+0) * (1);
static const int M9_cbp_mwG = (0+0) * (1);
static const int M9_cbp_hA = (22+0) * (1);
static const int M9_cbp_hB = (22+0) * (1);
static const int M9_cbp_hC = (22+0) * (1);
static const int M9_cbp_hD = (22+0) * (1);
static const int M9_cbp_hE = (22+0) * (1);
static const int M9_cbp_hH = (22+0) * (1);
static const int M9_cbp_hI = (22+0) * (1);
static const int M9_cbp_hJ = (22+0) * (1);
static const int M9_cbp_hK = (22+0) * (1);
static const int M9_cbp_hL = (22+0) * (1);
static const int M9_cbp_hS = (22+0) * (1);
static const int M9_cbp_hF = (22+0) * (1);
static const int M9_cbp_hM = (22+0) * (1);
static const int M9_cbp_hT = (22+0) * (1);
static const int M9_cbp_hN = (22+0) * (1);


static const long long M9_cbp_a_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M9_cbp_b_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long M9_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 5); // 45
static const long long M9_cbp_d_Volume = tc_vmul(tc_vmul(1, 1), 5); // 5
static const long long M9_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27
static const long long M9_cbp_f_Volume = tc_vmul(1, 3); // 3
static const long long M9_cbp_g_Volume = 1; // 1
static const long long M9_cbp_h_Volume = tc_vmul(tc_vmul(1, 2), (1<<5)); // 64
static const long long M9_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 3); // 42
static const long long M9_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 9), 7); // 189
static const long long M9_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 7); // 21
static const long long M9_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 4), 7), 5); // 1680
static const long long M9_cbp_m_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long M9_cbp_n_Volume = 1; // 1
static const long long M9_cbp_s_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long M9_cbp_t_Volume = 1; // 1
static const long long M9_cbp_w_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10

#define M9_cbp_USES(X) X(c3) X(lu) X(mvc) X(rbit) X(cnt) X(frac) X(qs) X(pat) X(lq) X(ie) X(tq) X(tx) X(em)

struct M9_cbp_T {
  cm_cnt2 M9_cbp_A[tc_rows(M9_cbp_a_Volume, M9_cbp_hA) * M_cbp_ND];
  cm_cnt2 M9_cbp_B[tc_rows(M9_cbp_b_Volume, M9_cbp_hB) * M_cbp_ND];
  cm_cnt2 M9_cbp_C[tc_rows(M9_cbp_c_Volume, M9_cbp_hC) * M_cbp_ND];
  cm_cnt2 M9_cbp_D[tc_rows(M9_cbp_d_Volume, M9_cbp_hD) * M_cbp_ND];
  cm_cnt2 M9_cbp_E[tc_rows(M9_cbp_e_Volume, M9_cbp_hE) * M_cbp_ND];
  cm_cnt2 M9_cbp_H[tc_rows(M9_cbp_h_Volume, M9_cbp_hH) * M_cbp_ND];
  cm_cnt2 M9_cbp_I[tc_rows(M9_cbp_i_Volume, M9_cbp_hI) * M_cbp_ND];
  cm_cnt2 M9_cbp_J[tc_rows(M9_cbp_j_Volume, M9_cbp_hJ) * M_cbp_ND];
  cm_cnt2 M9_cbp_K[tc_rows(M9_cbp_k_Volume, M9_cbp_hK) * M_cbp_ND];
  cm_cnt2 M9_cbp_L[tc_rows(M9_cbp_l_Volume, M9_cbp_hL) * M_cbp_ND];
  cm_cnt M9_cbp_MT[tc_rows(M9_cbp_t_Volume, M9_cbp_hT) * M_cbp_MND];
  i16 M9_cbp_S[tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * cm_apm_q(M9_cbp_qs)];
  u8 M9_cbp_SC[tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * cm_apm_q(M9_cbp_qs)];
  i16 M9_cbp_F[tc_rows(M9_cbp_f_Volume, M9_cbp_hF) * M_cbp_ND * cm_apm_q(M9_cbp_qf)];
  u8 M9_cbp_FC[tc_rows(M9_cbp_f_Volume, M9_cbp_hF) * M_cbp_ND * cm_apm_q(M9_cbp_qf)];
  i32 M9_cbp_W[tc_rows(M9_cbp_m_Volume, M9_cbp_hM) * M_cbp_ND * tc_nin(10)];
  u8 M9_cbp_WC[tc_rows(M9_cbp_m_Volume, M9_cbp_hM) * M_cbp_ND];
  i32 M9_cbp_WM[tc_rows(M9_cbp_n_Volume, M9_cbp_hN) * M_cbp_MND * TC_MIN];
  u8 M9_cbp_WMC[tc_rows(M9_cbp_n_Volume, M9_cbp_hN) * M_cbp_MND];
  i32 M9_cbp_W2[M9_cbp_w_Volume * M_cbp_ND * tc_nin(10)];
  u8 M9_cbp_W2C[M9_cbp_w_Volume * M_cbp_ND];
  i32 M9_cbp_W3[tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * tc_nin(10)];
  u8 M9_cbp_W3C[tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND];
  i32 M9_cbp_WF[M_cbp_ND * HS_N * TC_FIN];
  u8 M9_cbp_WFC[M_cbp_ND * HS_N];
  cm_cnt M9_cbp_G[M9_cbp_g_Volume * M_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_a_Volume, M9_cbp_hA) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_b_Volume, M9_cbp_hB) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_c_Volume, M9_cbp_hC) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_d_Volume, M9_cbp_hD) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_e_Volume, M9_cbp_hE) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_h_Volume, M9_cbp_hH) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_i_Volume, M9_cbp_hI) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_j_Volume, M9_cbp_hJ) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_k_Volume, M9_cbp_hK) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_cbp_l_Volume, M9_cbp_hL) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_cbp_t_Volume, M9_cbp_hT) * M_cbp_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * cm_apm_q(M9_cbp_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * cm_apm_q(M9_cbp_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_cbp_f_Volume, M9_cbp_hF) * M_cbp_ND * cm_apm_q(M9_cbp_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_cbp_f_Volume, M9_cbp_hF) * M_cbp_ND * cm_apm_q(M9_cbp_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_cbp_m_Volume, M9_cbp_hM) * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_cbp_m_Volume, M9_cbp_hM) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_cbp_n_Volume, M9_cbp_hN) * M_cbp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_cbp_n_Volume, M9_cbp_hN) * M_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_cbp_w_Volume * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_cbp_w_Volume * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_cbp_s_Volume, M9_cbp_hS) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_cbp_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_cbp_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_cbp_g_Volume * M_cbp_NG);
  void M9_cbp_Init( void ) {}
  void M9_cbp_Quit( void ) {}
};
