
static const byte M7_cbp_a_lu[8+1]={ 0,1,2,2,3,3,4,5,6 };


static const byte M7_cbp_c_cnt[5+1]={ 0,1,2,3,4,5 };

static const byte M7_cbp_d_frac[4+1]={ 0,1,2,3,4 };

static const byte M7_cbp_e_qs[32+1]={ 0,0,0,1,1,1,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };




static const byte M7_cbp_i_lu[8+1]={ 0,1,2,2,3,4,5,6,6 };

static const byte M7_cbp_j_lu[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M7_cbp_j_mvc[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M7_cbp_k_lu[8+1]={ 0,1,2,3,4,5,6,7,8 };






static const byte M7_cbp_w_lu[8+1]={ 0,1,2,3,4,4,5,6,6 };


static const int M7_cbp_rA = (129+0) * (1);
static const int M7_cbp_mwA = (255+0) * (1);
static const int M7_cbp_rB = (41+0) * (1);
static const int M7_cbp_mwB = (2+0) * (1);
static const int M7_cbp_rC = (255+0) * (1);
static const int M7_cbp_mwC = (4+0) * (1);
static const int M7_cbp_rD = (255+0) * (1);
static const int M7_cbp_mwD = (16+0) * (1);
static const int M7_cbp_rE = (204+0) * (1);
static const int M7_cbp_mwE = (84+0) * (1);
static const int M7_cbp_rH = (207+0) * (1);
static const int M7_cbp_mwH = (64+0) * (1);
static const int M7_cbp_rI = (255+0) * (1);
static const int M7_cbp_mwI = (66+0) * (1);
static const int M7_cbp_rJ = (220+0) * (1);
static const int M7_cbp_mwJ = (64+0) * (1);
static const int M7_cbp_rK = (253+0) * (1);
static const int M7_cbp_mwK = (32+0) * (1);
static const int M7_cbp_rL = (0+0) * (1);
static const int M7_cbp_mwL = (128+0) * (1);
static const int M7_cbp_lr = (13+0) * (1);
static const int M7_cbp_mb = (31+0) * (1);
static const int M7_cbp_rS1 = (8+0) * (1);
static const int M7_cbp_rS2 = (16+0) * (1);
static const int M7_cbp_qs = (1+3) * (1);
static const int M7_cbp_qf = (64+3) * (1);
static const int M7_cbp_bw = (14+0) * (1);
static const int M7_cbp_rT = (0+0) * (1);
static const int M7_cbp_mwT = (0+0) * (1);
static const int M7_cbp_mt = (0+0) * (1);
static const int M7_cbp_lrm = (0+0) * (1);
static const int M7_cbp_mbm = (0+0) * (1);
static const int M7_cbp_rG = (0+0) * (1);
static const int M7_cbp_mwG = (0+0) * (1);
static const int M7_cbp_hA = (22+0) * (1);
static const int M7_cbp_hB = (22+0) * (1);
static const int M7_cbp_hC = (22+0) * (1);
static const int M7_cbp_hD = (22+0) * (1);
static const int M7_cbp_hE = (22+0) * (1);
static const int M7_cbp_hH = (22+0) * (1);
static const int M7_cbp_hI = (22+0) * (1);
static const int M7_cbp_hJ = (22+0) * (1);
static const int M7_cbp_hK = (22+0) * (1);
static const int M7_cbp_hL = (22+0) * (1);
static const int M7_cbp_hM = (22+0) * (1);
static const int M7_cbp_hT = (22+0) * (1);
static const int M7_cbp_hN = (22+0) * (1);
static const int M7_cbp_hS = (22+0) * (1);
static const int M7_cbp_hF = (22+0) * (1);


static const long long M7_cbp_a_Volume = tc_vmul(tc_vmul(1, 1), 7); // 7
static const long long M7_cbp_b_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long M7_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 6); // 54
static const long long M7_cbp_d_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long M7_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 3); // 36
static const long long M7_cbp_f_Volume = tc_vmul(1, 3); // 3
static const long long M7_cbp_g_Volume = 1; // 1
static const long long M7_cbp_h_Volume = tc_vmul(tc_vmul(1, 2), (1<<5)); // 64
static const long long M7_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 3); // 63
static const long long M7_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 9), 7); // 189
static const long long M7_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 9); // 27
static const long long M7_cbp_l_Volume = tc_vmul(1, 1); // 1
static const long long M7_cbp_m_Volume = tc_vmul(1, 2); // 2
static const long long M7_cbp_n_Volume = 1; // 1
static const long long M7_cbp_s_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long M7_cbp_t_Volume = 1; // 1
static const long long M7_cbp_w_Volume = tc_vmul(tc_vmul(1, 2), 7); // 14

#define M7_cbp_USES(X) X(lu) X(c3) X(mvc) X(rbit) X(cnt) X(frac) X(qs) X(pat) X(lq)

struct M7_cbp_T {
  cm_cnt2 M7_cbp_A[tc_rows(M7_cbp_a_Volume, M7_cbp_hA) * M_cbp_ND];
  cm_cnt2 M7_cbp_B[tc_rows(M7_cbp_b_Volume, M7_cbp_hB) * M_cbp_ND];
  cm_cnt2 M7_cbp_C[tc_rows(M7_cbp_c_Volume, M7_cbp_hC) * M_cbp_ND];
  cm_cnt2 M7_cbp_D[tc_rows(M7_cbp_d_Volume, M7_cbp_hD) * M_cbp_ND];
  cm_cnt2 M7_cbp_E[tc_rows(M7_cbp_e_Volume, M7_cbp_hE) * M_cbp_ND];
  cm_cnt2 M7_cbp_H[tc_rows(M7_cbp_h_Volume, M7_cbp_hH) * M_cbp_ND];
  cm_cnt2 M7_cbp_I[tc_rows(M7_cbp_i_Volume, M7_cbp_hI) * M_cbp_ND];
  cm_cnt2 M7_cbp_J[tc_rows(M7_cbp_j_Volume, M7_cbp_hJ) * M_cbp_ND];
  cm_cnt2 M7_cbp_K[tc_rows(M7_cbp_k_Volume, M7_cbp_hK) * M_cbp_ND];
  cm_cnt2 M7_cbp_L[tc_rows(M7_cbp_l_Volume, M7_cbp_hL) * M_cbp_ND];
  cm_cnt M7_cbp_MT[tc_rows(M7_cbp_t_Volume, M7_cbp_hT) * M_cbp_MND];
  i16 M7_cbp_S[tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * cm_apm_q(M7_cbp_qs)];
  u8 M7_cbp_SC[tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * cm_apm_q(M7_cbp_qs)];
  i16 M7_cbp_F[tc_rows(M7_cbp_f_Volume, M7_cbp_hF) * M_cbp_ND * cm_apm_q(M7_cbp_qf)];
  u8 M7_cbp_FC[tc_rows(M7_cbp_f_Volume, M7_cbp_hF) * M_cbp_ND * cm_apm_q(M7_cbp_qf)];
  i32 M7_cbp_W[tc_rows(M7_cbp_m_Volume, M7_cbp_hM) * M_cbp_ND * tc_nin(10)];
  u8 M7_cbp_WC[tc_rows(M7_cbp_m_Volume, M7_cbp_hM) * M_cbp_ND];
  i32 M7_cbp_WM[tc_rows(M7_cbp_n_Volume, M7_cbp_hN) * M_cbp_MND * TC_MIN];
  u8 M7_cbp_WMC[tc_rows(M7_cbp_n_Volume, M7_cbp_hN) * M_cbp_MND];
  i32 M7_cbp_W2[M7_cbp_w_Volume * M_cbp_ND * tc_nin(10)];
  u8 M7_cbp_W2C[M7_cbp_w_Volume * M_cbp_ND];
  i32 M7_cbp_W3[tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * tc_nin(10)];
  u8 M7_cbp_W3C[tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND];
  i32 M7_cbp_WF[M_cbp_ND * HS_N * TC_FIN];
  u8 M7_cbp_WFC[M_cbp_ND * HS_N];
  cm_cnt M7_cbp_G[M7_cbp_g_Volume * M_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M7_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_a_Volume, M7_cbp_hA) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_b_Volume, M7_cbp_hB) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_c_Volume, M7_cbp_hC) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_d_Volume, M7_cbp_hD) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_e_Volume, M7_cbp_hE) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_h_Volume, M7_cbp_hH) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_i_Volume, M7_cbp_hI) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_j_Volume, M7_cbp_hJ) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_k_Volume, M7_cbp_hK) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_cbp_l_Volume, M7_cbp_hL) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M7_cbp_t_Volume, M7_cbp_hT) * M_cbp_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * cm_apm_q(M7_cbp_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * cm_apm_q(M7_cbp_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_cbp_f_Volume, M7_cbp_hF) * M_cbp_ND * cm_apm_q(M7_cbp_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_cbp_f_Volume, M7_cbp_hF) * M_cbp_ND * cm_apm_q(M7_cbp_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_cbp_m_Volume, M7_cbp_hM) * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_cbp_m_Volume, M7_cbp_hM) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_cbp_n_Volume, M7_cbp_hN) * M_cbp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_cbp_n_Volume, M7_cbp_hN) * M_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M7_cbp_w_Volume * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M7_cbp_w_Volume * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_cbp_s_Volume, M7_cbp_hS) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_cbp_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_cbp_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M7_cbp_g_Volume * M_cbp_NG);
  void M7_cbp_Init( void ) {}
  void M7_cbp_Quit( void ) {}
};
