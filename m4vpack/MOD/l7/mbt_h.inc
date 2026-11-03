


static const byte M7_mbt_c_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };



static const byte M7_mbt_f_st[3+1]={ 0,1,2,3 };


static const byte M7_mbt_h_fsn[3+1]={ 0,1,2,3 };

static const byte M7_mbt_i_c4[4+1]={ 0,1,2,3,4 };

static const byte M7_mbt_j_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };







static const byte M7_mbt_w_rt[3+1]={ 0,1,2,3 };


static const int M7_mbt_rA = (31+0) * (1);
static const int M7_mbt_mwA = (122+0) * (1);
static const int M7_mbt_rB = (0+0) * (1);
static const int M7_mbt_mwB = (65+0) * (1);
static const int M7_mbt_rC = (31+0) * (1);
static const int M7_mbt_mwC = (128+0) * (1);
static const int M7_mbt_rD = (255+0) * (1);
static const int M7_mbt_mwD = (10+0) * (1);
static const int M7_mbt_rE = (0+0) * (1);
static const int M7_mbt_mwE = (135+0) * (1);
static const int M7_mbt_rH = (191+0) * (1);
static const int M7_mbt_mwH = (250+0) * (1);
static const int M7_mbt_rI = (253+0) * (1);
static const int M7_mbt_mwI = (78+0) * (1);
static const int M7_mbt_rJ = (255+0) * (1);
static const int M7_mbt_mwJ = (14+0) * (1);
static const int M7_mbt_rK = (247+0) * (1);
static const int M7_mbt_mwK = (38+0) * (1);
static const int M7_mbt_rL = (178+0) * (1);
static const int M7_mbt_mwL = (0+0) * (1);
static const int M7_mbt_lr = (12+0) * (1);
static const int M7_mbt_mb = (15+0) * (1);
static const int M7_mbt_rS1 = (8+0) * (1);
static const int M7_mbt_rS2 = (3+0) * (1);
static const int M7_mbt_qs = (90+3) * (1);
static const int M7_mbt_qf = (62+3) * (1);
static const int M7_mbt_bw = (16+0) * (1);
static const int M7_mbt_rT = (0+0) * (1);
static const int M7_mbt_mwT = (0+0) * (1);
static const int M7_mbt_mt = (0+0) * (1);
static const int M7_mbt_lrm = (0+0) * (1);
static const int M7_mbt_mbm = (0+0) * (1);
static const int M7_mbt_rG = (0+0) * (1);
static const int M7_mbt_mwG = (0+0) * (1);
static const int M7_mbt_hA = (22+0) * (1);
static const int M7_mbt_hB = (22+0) * (1);
static const int M7_mbt_hC = (22+0) * (1);
static const int M7_mbt_hD = (22+0) * (1);
static const int M7_mbt_hE = (22+0) * (1);
static const int M7_mbt_hH = (22+0) * (1);
static const int M7_mbt_hI = (22+0) * (1);
static const int M7_mbt_hJ = (22+0) * (1);
static const int M7_mbt_hK = (22+0) * (1);
static const int M7_mbt_hL = (22+0) * (1);
static const int M7_mbt_hM = (22+0) * (1);
static const int M7_mbt_hT = (22+0) * (1);
static const int M7_mbt_hN = (22+0) * (1);
static const int M7_mbt_hS = (22+0) * (1);
static const int M7_mbt_hF = (22+0) * (1);


static const long long M7_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 2); // 4
static const long long M7_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 1); // 3
static const long long M7_mbt_c_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M7_mbt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 1); // 3
static const long long M7_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 1); // 4
static const long long M7_mbt_f_Volume = tc_vmul(1, 4); // 4
static const long long M7_mbt_g_Volume = 1; // 1
static const long long M7_mbt_h_Volume = tc_vmul(tc_vmul(1, 1), 4); // 4
static const long long M7_mbt_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 5); // 30
static const long long M7_mbt_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 3); // 15
static const long long M7_mbt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3); // 18
static const long long M7_mbt_l_Volume = tc_vmul(1, 1); // 1
static const long long M7_mbt_m_Volume = tc_vmul(1, 1); // 1
static const long long M7_mbt_n_Volume = 1; // 1
static const long long M7_mbt_s_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3
static const long long M7_mbt_t_Volume = 1; // 1
static const long long M7_mbt_w_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8

#define M7_mbt_USES(X) X(st) X(rt) X(rc) X(cd) X(intra) X(fsn) X(cl) X(c4)

struct M7_mbt_T {
  cm_cnt2 M7_mbt_A[tc_rows(M7_mbt_a_Volume, M7_mbt_hA) * M_mbt_ND];
  cm_cnt2 M7_mbt_B[tc_rows(M7_mbt_b_Volume, M7_mbt_hB) * M_mbt_ND];
  cm_cnt2 M7_mbt_C[tc_rows(M7_mbt_c_Volume, M7_mbt_hC) * M_mbt_ND];
  cm_cnt2 M7_mbt_D[tc_rows(M7_mbt_d_Volume, M7_mbt_hD) * M_mbt_ND];
  cm_cnt2 M7_mbt_E[tc_rows(M7_mbt_e_Volume, M7_mbt_hE) * M_mbt_ND];
  cm_cnt2 M7_mbt_H[tc_rows(M7_mbt_h_Volume, M7_mbt_hH) * M_mbt_ND];
  cm_cnt2 M7_mbt_I[tc_rows(M7_mbt_i_Volume, M7_mbt_hI) * M_mbt_ND];
  cm_cnt2 M7_mbt_J[tc_rows(M7_mbt_j_Volume, M7_mbt_hJ) * M_mbt_ND];
  cm_cnt2 M7_mbt_K[tc_rows(M7_mbt_k_Volume, M7_mbt_hK) * M_mbt_ND];
  cm_cnt2 M7_mbt_L[tc_rows(M7_mbt_l_Volume, M7_mbt_hL) * M_mbt_ND];
  cm_cnt M7_mbt_MT[tc_rows(M7_mbt_t_Volume, M7_mbt_hT) * M_mbt_MND];
  i16 M7_mbt_S[tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * cm_apm_q(M7_mbt_qs)];
  u8 M7_mbt_SC[tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * cm_apm_q(M7_mbt_qs)];
  i16 M7_mbt_F[tc_rows(M7_mbt_f_Volume, M7_mbt_hF) * M_mbt_ND * cm_apm_q(M7_mbt_qf)];
  u8 M7_mbt_FC[tc_rows(M7_mbt_f_Volume, M7_mbt_hF) * M_mbt_ND * cm_apm_q(M7_mbt_qf)];
  i32 M7_mbt_W[tc_rows(M7_mbt_m_Volume, M7_mbt_hM) * M_mbt_ND * tc_nin(10)];
  u8 M7_mbt_WC[tc_rows(M7_mbt_m_Volume, M7_mbt_hM) * M_mbt_ND];
  i32 M7_mbt_WM[tc_rows(M7_mbt_n_Volume, M7_mbt_hN) * M_mbt_MND * TC_MIN];
  u8 M7_mbt_WMC[tc_rows(M7_mbt_n_Volume, M7_mbt_hN) * M_mbt_MND];
  i32 M7_mbt_W2[M7_mbt_w_Volume * M_mbt_ND * tc_nin(10)];
  u8 M7_mbt_W2C[M7_mbt_w_Volume * M_mbt_ND];
  i32 M7_mbt_W3[tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * tc_nin(10)];
  u8 M7_mbt_W3C[tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND];
  i32 M7_mbt_WF[M_mbt_ND * HS_N * TC_FIN];
  u8 M7_mbt_WFC[M_mbt_ND * HS_N];
  cm_cnt M7_mbt_G[M7_mbt_g_Volume * M_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M7_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_a_Volume, M7_mbt_hA) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_b_Volume, M7_mbt_hB) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_c_Volume, M7_mbt_hC) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_d_Volume, M7_mbt_hD) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_e_Volume, M7_mbt_hE) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_h_Volume, M7_mbt_hH) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_i_Volume, M7_mbt_hI) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_j_Volume, M7_mbt_hJ) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_k_Volume, M7_mbt_hK) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_mbt_l_Volume, M7_mbt_hL) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M7_mbt_t_Volume, M7_mbt_hT) * M_mbt_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * cm_apm_q(M7_mbt_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * cm_apm_q(M7_mbt_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_mbt_f_Volume, M7_mbt_hF) * M_mbt_ND * cm_apm_q(M7_mbt_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_mbt_f_Volume, M7_mbt_hF) * M_mbt_ND * cm_apm_q(M7_mbt_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_mbt_m_Volume, M7_mbt_hM) * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_mbt_m_Volume, M7_mbt_hM) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_mbt_n_Volume, M7_mbt_hN) * M_mbt_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_mbt_n_Volume, M7_mbt_hN) * M_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M7_mbt_w_Volume * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M7_mbt_w_Volume * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_mbt_s_Volume, M7_mbt_hS) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_mbt_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_mbt_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M7_mbt_g_Volume * M_mbt_NG);
  void M7_mbt_Init( void ) {}
  void M7_mbt_Quit( void ) {}
};
