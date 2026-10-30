

static const byte M9_mbt_b_rt[3+1]={ 0,1,2,3 };

static const byte M9_mbt_c_st[3+1]={ 0,1,2,3 };
static const byte M9_mbt_c_rc[10+1]={ 0,1,1,2,2,3,3,3,3,3,3 };



static const byte M9_mbt_f_st[3+1]={ 0,1,2,3 };


static const byte M9_mbt_h_fsn[3+1]={ 0,1,2,3 };

static const byte M9_mbt_i_st[3+1]={ 0,1,2,3 };
static const byte M9_mbt_i_c4[4+1]={ 0,1,2,3,4 };

static const byte M9_mbt_j_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte M9_mbt_k_c4[4+1]={ 0,1,2,3,3 };

static const byte M9_mbt_l_zt[24+1]={ 0,0,0,0,0,1,1,2,3,3,4,5,5,6,6,6,7,8,8,8,8,8,8,8,8 };
static const byte M9_mbt_l_ze[24+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,11,12,12,13,13,13,13,13,13,13,13,13,13 };



static const byte M9_mbt_s_st[3+1]={ 0,1,2,3 };


static const byte M9_mbt_w_rt[3+1]={ 0,1,2,3 };


static const int M9_mbt_rA = (242+0) * (1);
static const int M9_mbt_rB = (0+0) * (1);
static const int M9_mbt_rC = (0+0) * (1);
static const int M9_mbt_rD = (255+0) * (1);
static const int M9_mbt_rE = (244+0) * (1);
static const int M9_mbt_rH = (240+0) * (1);
static const int M9_mbt_rI = (255+0) * (1);
static const int M9_mbt_rJ = (255+0) * (1);
static const int M9_mbt_rK = (255+0) * (1);
static const int M9_mbt_rL = (254+0) * (1);
static const int M9_mbt_rS1 = (8+0) * (1);
static const int M9_mbt_rS2 = (3+0) * (1);
static const int M9_mbt_lr = (15+0) * (1);
static const int M9_mbt_mwA = (251+0) * (1);
static const int M9_mbt_mwB = (30+0) * (1);
static const int M9_mbt_mwC = (32+0) * (1);
static const int M9_mbt_mwD = (128+0) * (1);
static const int M9_mbt_mwE = (238+0) * (1);
static const int M9_mbt_mwH = (67+0) * (1);
static const int M9_mbt_mwI = (63+0) * (1);
static const int M9_mbt_mwJ = (7+0) * (1);
static const int M9_mbt_mwK = (39+0) * (1);
static const int M9_mbt_mwL = (0+0) * (1);
static const int M9_mbt_bw = (16+0) * (1);
static const int M9_mbt_qs = (88+3) * (1);
static const int M9_mbt_qf = (62+3) * (1);
static const int M9_mbt_mb = (29+0) * (1);
static const int M9_mbt_rT = (0+0) * (1);
static const int M9_mbt_mwT = (0+0) * (1);
static const int M9_mbt_mt = (0+0) * (1);
static const int M9_mbt_lrm = (0+0) * (1);
static const int M9_mbt_mbm = (0+0) * (1);
static const int M9_mbt_rG = (0+0) * (1);
static const int M9_mbt_mwG = (0+0) * (1);
static const int M9_mbt_hA = (22+0) * (1);
static const int M9_mbt_hB = (22+0) * (1);
static const int M9_mbt_hC = (22+0) * (1);
static const int M9_mbt_hD = (22+0) * (1);
static const int M9_mbt_hE = (22+0) * (1);
static const int M9_mbt_hH = (22+0) * (1);
static const int M9_mbt_hI = (22+0) * (1);
static const int M9_mbt_hJ = (22+0) * (1);
static const int M9_mbt_hK = (22+0) * (1);
static const int M9_mbt_hL = (22+0) * (1);
static const int M9_mbt_hS = (22+0) * (1);
static const int M9_mbt_hF = (22+0) * (1);
static const int M9_mbt_hM = (22+0) * (1);
static const int M9_mbt_hT = (22+0) * (1);
static const int M9_mbt_hN = (22+0) * (1);


static const long long M9_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long M9_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 4); // 16
static const long long M9_mbt_c_Volume = tc_vmul(tc_vmul(1, 4), 4); // 16
static const long long M9_mbt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 1); // 9
static const long long M9_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 2); // 12
static const long long M9_mbt_f_Volume = tc_vmul(1, 4); // 4
static const long long M9_mbt_g_Volume = 1; // 1
static const long long M9_mbt_h_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long M9_mbt_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 5); // 60
static const long long M9_mbt_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 3); // 66
static const long long M9_mbt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 4); // 24
static const long long M9_mbt_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 9), 14); // 126
static const long long M9_mbt_m_Volume = tc_vmul(1, 2); // 2
static const long long M9_mbt_n_Volume = 1; // 1
static const long long M9_mbt_s_Volume = tc_vmul(tc_vmul(1, 4), 3); // 12
static const long long M9_mbt_t_Volume = 1; // 1
static const long long M9_mbt_w_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8

#define M9_mbt_USES(X) X(st) X(cl) X(rt) X(c4) X(rc) X(cd) X(intra) X(fsn) X(zt) X(ze)

struct M9_mbt_T {
  cm_cnt2 M9_mbt_A[tc_rows(M9_mbt_a_Volume, M9_mbt_hA) * M_mbt_ND];
  cm_cnt2 M9_mbt_B[tc_rows(M9_mbt_b_Volume, M9_mbt_hB) * M_mbt_ND];
  cm_cnt2 M9_mbt_C[tc_rows(M9_mbt_c_Volume, M9_mbt_hC) * M_mbt_ND];
  cm_cnt2 M9_mbt_D[tc_rows(M9_mbt_d_Volume, M9_mbt_hD) * M_mbt_ND];
  cm_cnt2 M9_mbt_E[tc_rows(M9_mbt_e_Volume, M9_mbt_hE) * M_mbt_ND];
  cm_cnt2 M9_mbt_H[tc_rows(M9_mbt_h_Volume, M9_mbt_hH) * M_mbt_ND];
  cm_cnt2 M9_mbt_I[tc_rows(M9_mbt_i_Volume, M9_mbt_hI) * M_mbt_ND];
  cm_cnt2 M9_mbt_J[tc_rows(M9_mbt_j_Volume, M9_mbt_hJ) * M_mbt_ND];
  cm_cnt2 M9_mbt_K[tc_rows(M9_mbt_k_Volume, M9_mbt_hK) * M_mbt_ND];
  cm_cnt2 M9_mbt_L[tc_rows(M9_mbt_l_Volume, M9_mbt_hL) * M_mbt_ND];
  cm_cnt M9_mbt_MT[tc_rows(M9_mbt_t_Volume, M9_mbt_hT) * M_mbt_MND];
  i16 M9_mbt_S[tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * cm_apm_q(M9_mbt_qs)];
  u8 M9_mbt_SC[tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * cm_apm_q(M9_mbt_qs)];
  i16 M9_mbt_F[tc_rows(M9_mbt_f_Volume, M9_mbt_hF) * M_mbt_ND * cm_apm_q(M9_mbt_qf)];
  u8 M9_mbt_FC[tc_rows(M9_mbt_f_Volume, M9_mbt_hF) * M_mbt_ND * cm_apm_q(M9_mbt_qf)];
  i32 M9_mbt_W[tc_rows(M9_mbt_m_Volume, M9_mbt_hM) * M_mbt_ND * tc_nin(10)];
  u8 M9_mbt_WC[tc_rows(M9_mbt_m_Volume, M9_mbt_hM) * M_mbt_ND];
  i32 M9_mbt_WM[tc_rows(M9_mbt_n_Volume, M9_mbt_hN) * M_mbt_MND * TC_MIN];
  u8 M9_mbt_WMC[tc_rows(M9_mbt_n_Volume, M9_mbt_hN) * M_mbt_MND];
  i32 M9_mbt_W2[M9_mbt_w_Volume * M_mbt_ND * tc_nin(10)];
  u8 M9_mbt_W2C[M9_mbt_w_Volume * M_mbt_ND];
  i32 M9_mbt_W3[tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * tc_nin(10)];
  u8 M9_mbt_W3C[tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND];
  i32 M9_mbt_WF[M_mbt_ND * HS_N * TC_FIN];
  u8 M9_mbt_WFC[M_mbt_ND * HS_N];
  cm_cnt M9_mbt_G[M9_mbt_g_Volume * M_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_a_Volume, M9_mbt_hA) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_b_Volume, M9_mbt_hB) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_c_Volume, M9_mbt_hC) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_d_Volume, M9_mbt_hD) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_e_Volume, M9_mbt_hE) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_h_Volume, M9_mbt_hH) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_i_Volume, M9_mbt_hI) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_j_Volume, M9_mbt_hJ) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_k_Volume, M9_mbt_hK) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mbt_l_Volume, M9_mbt_hL) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_mbt_t_Volume, M9_mbt_hT) * M_mbt_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * cm_apm_q(M9_mbt_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * cm_apm_q(M9_mbt_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_mbt_f_Volume, M9_mbt_hF) * M_mbt_ND * cm_apm_q(M9_mbt_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mbt_f_Volume, M9_mbt_hF) * M_mbt_ND * cm_apm_q(M9_mbt_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mbt_m_Volume, M9_mbt_hM) * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mbt_m_Volume, M9_mbt_hM) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mbt_n_Volume, M9_mbt_hN) * M_mbt_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mbt_n_Volume, M9_mbt_hN) * M_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_mbt_w_Volume * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_mbt_w_Volume * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mbt_s_Volume, M9_mbt_hS) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_mbt_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_mbt_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_mbt_g_Volume * M_mbt_NG);
  void M9_mbt_Init( void ) {}
  void M9_mbt_Quit( void ) {}
};
