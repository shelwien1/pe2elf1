



static const byte D9_jsc_d_ac[8+1]={ 0,1,1,1,2,2,2,2,3 };


static const byte D9_jsc_f_ac[8+1]={ 0,1,2,3,4,5,6,7,7 };

static const byte D9_jsc_g_fld[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte D9_jsc_h_ab[27+1]={ 0,0,1,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,5,5,6,6,6 };
static const byte D9_jsc_h_tr[5+1]={ 0,1,2,2,3,3 };

static const byte D9_jsc_i_ec[9+1]={ 0,0,1,2,3,4,5,5,5,6 };
static const byte D9_jsc_i_tr[5+1]={ 0,1,2,2,3,3 };


static const byte D9_jsc_k_ac[8+1]={ 0,1,2,2,3,3,3,3,3 };

static const byte D9_jsc_l_ab[27+1]={ 0,0,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,4,5,5,5,5,5,5 };
static const byte D9_jsc_l_ac[8+1]={ 0,0,1,2,3,4,5,6,7 };

static const byte D9_jsc_m_fld[8+1]={ 0,0,0,1,2,2,3,3,3 };

static const byte D9_jsc_n_fld[8+1]={ 0,1,2,3,4,5,6,7,8 };





static const int D9_jsc_rA = (2+0) * (1);
static const int D9_jsc_mwA = (211+0) * (1);
static const int D9_jsc_rB = (242+0) * (1);
static const int D9_jsc_mwB = (102+0) * (1);
static const int D9_jsc_rC = (232+0) * (1);
static const int D9_jsc_mwC = (255+0) * (1);
static const int D9_jsc_rD = (253+0) * (1);
static const int D9_jsc_mwD = (96+0) * (1);
static const int D9_jsc_rE = (242+0) * (1);
static const int D9_jsc_mwE = (229+0) * (1);
static const int D9_jsc_rH = (251+0) * (1);
static const int D9_jsc_mwH = (252+0) * (1);
static const int D9_jsc_rI = (252+0) * (1);
static const int D9_jsc_mwI = (248+0) * (1);
static const int D9_jsc_rJ = (251+0) * (1);
static const int D9_jsc_mwJ = (252+0) * (1);
static const int D9_jsc_rK = (248+0) * (1);
static const int D9_jsc_mwK = (61+0) * (1);
static const int D9_jsc_rL = (254+0) * (1);
static const int D9_jsc_mwL = (0+0) * (1);
static const int D9_jsc_lr = (8+0) * (1);
static const int D9_jsc_mb = (253+0) * (1);
static const int D9_jsc_wp = (30+0) * (1);
static const int D9_jsc_wq = (0+0) * (1);
static const int D9_jsc_wo = (64+0) * (1);
static const int D9_jsc_wa = (0+0) * (1);
static const int D9_jsc_wf = (95+0) * (1);
static const int D9_jsc_lf = (18+0) * (1);
static const int D9_jsc_bf = (3+0) * (1);
static const int D9_jsc_l2 = (8+0) * (1);
static const int D9_jsc_l3 = (0+0) * (1);
static const int D9_jsc_b2 = (64+0) * (1);
static const int D9_jsc_b3 = (0+0) * (1);
static const int D9_jsc_rS1 = (8+0) * (1);
static const int D9_jsc_rS2 = (4+0) * (1);
static const int D9_jsc_qs = (117+3) * (1);
static const int D9_jsc_qf = (63+3) * (1);
static const int D9_jsc_bw = (16+0) * (1);
static const int D9_jsc_rT = (255+0) * (1);
static const int D9_jsc_mwT = (32+0) * (1);
static const int D9_jsc_mt = (0+0) * (1);
static const int D9_jsc_lrm = (31+0) * (1);
static const int D9_jsc_mbm = (25+0) * (1);
static const int D9_jsc_rG = (95+0) * (1);
static const int D9_jsc_mwG = (159+0) * (1);
static const int D9_jsc_hA = (22+0) * (1);
static const int D9_jsc_hB = (22+0) * (1);
static const int D9_jsc_hC = (22+0) * (1);
static const int D9_jsc_hD = (22+0) * (1);
static const int D9_jsc_hE = (22+0) * (1);
static const int D9_jsc_hH = (22+0) * (1);
static const int D9_jsc_hI = (22+0) * (1);
static const int D9_jsc_hJ = (22+0) * (1);
static const int D9_jsc_hK = (22+0) * (1);
static const int D9_jsc_hL = (22+0) * (1);
static const int D9_jsc_hS = (22+0) * (1);
static const int D9_jsc_hF = (22+0) * (1);
static const int D9_jsc_hM = (22+0) * (1);
static const int D9_jsc_hT = (22+0) * (1);
static const int D9_jsc_hN = (22+0) * (1);


static const long long D9_jsc_a_Volume = tc_vmul(tc_vmul(1, 2), (1<<1)); // 4
static const long long D9_jsc_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), (1<<1)), (1<<6)); // 256
static const long long D9_jsc_c_Volume = tc_vmul(tc_vmul(1, 2), 1); // 2
static const long long D9_jsc_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 4), (1<<3)); // 64
static const long long D9_jsc_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 1); // 6
static const long long D9_jsc_f_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long D9_jsc_g_Volume = tc_vmul(1, 9); // 9
static const long long D9_jsc_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), (1<<1)), 4); // 112
static const long long D9_jsc_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), (1<<1)), 4); // 112
static const long long D9_jsc_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), (1<<1)), 3); // 18
static const long long D9_jsc_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 4); // 36
static const long long D9_jsc_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), (1<<1)), 6), 8); // 192
static const long long D9_jsc_m_Volume = tc_vmul(1, 4); // 4
static const long long D9_jsc_n_Volume = tc_vmul(1, 9); // 9
static const long long D9_jsc_s_Volume = tc_vmul(tc_vmul(1, 3), (1<<3)); // 24
static const long long D9_jsc_t_Volume = tc_vmul(1, 1); // 1
static const long long D9_jsc_w_Volume = tc_vmul(1, 3); // 3

#define D9_jsc_USES(X) X(fld) X(lv) X(pq) X(ac) X(cc) X(ab) X(tr) X(ec) X(st) X(bd)

struct D9_jsc_T {
  cm_cnt2 D9_jsc_A[tc_rows(D9_jsc_a_Volume, D9_jsc_hA) * M_jsc_ND];
  cm_cnt2 D9_jsc_B[tc_rows(D9_jsc_b_Volume, D9_jsc_hB) * M_jsc_ND];
  cm_cnt2 D9_jsc_C[tc_rows(D9_jsc_c_Volume, D9_jsc_hC) * M_jsc_ND];
  cm_cnt2 D9_jsc_D[tc_rows(D9_jsc_d_Volume, D9_jsc_hD) * M_jsc_ND];
  cm_cnt2 D9_jsc_E[tc_rows(D9_jsc_e_Volume, D9_jsc_hE) * M_jsc_ND];
  cm_cnt2 D9_jsc_H[tc_rows(D9_jsc_h_Volume, D9_jsc_hH) * M_jsc_ND];
  cm_cnt2 D9_jsc_I[tc_rows(D9_jsc_i_Volume, D9_jsc_hI) * M_jsc_ND];
  cm_cnt2 D9_jsc_J[tc_rows(D9_jsc_j_Volume, D9_jsc_hJ) * M_jsc_ND];
  cm_cnt2 D9_jsc_K[tc_rows(D9_jsc_k_Volume, D9_jsc_hK) * M_jsc_ND];
  cm_cnt2 D9_jsc_L[tc_rows(D9_jsc_l_Volume, D9_jsc_hL) * M_jsc_ND];
  cm_cnt D9_jsc_MT[tc_rows(D9_jsc_t_Volume, D9_jsc_hT) * M_jsc_MND];
  cm_apme D9_jsc_S[tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND * cm_apm_q(D9_jsc_qs)];
  cm_apme D9_jsc_F[tc_rows(D9_jsc_f_Volume, D9_jsc_hF) * M_jsc_ND * cm_apm_q(D9_jsc_qf)];
  i32 D9_jsc_W[tc_rows(D9_jsc_m_Volume, D9_jsc_hM) * M_jsc_ND * tc_nin(10)];
  u8 D9_jsc_WC[tc_rows(D9_jsc_m_Volume, D9_jsc_hM) * M_jsc_ND];
  i32 D9_jsc_WM[tc_rows(D9_jsc_n_Volume, D9_jsc_hN) * M_jsc_MND * TC_MIN];
  u8 D9_jsc_WMC[tc_rows(D9_jsc_n_Volume, D9_jsc_hN) * M_jsc_MND];
  i32 D9_jsc_W2[D9_jsc_w_Volume * M_jsc_ND * tc_nin(10)];
  u8 D9_jsc_W2C[D9_jsc_w_Volume * M_jsc_ND];
  i32 D9_jsc_W3[tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND * tc_nin(10)];
  u8 D9_jsc_W3C[tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND];
  i32 D9_jsc_WF[M_jsc_ND * HS_N * TC_FIN];
  u8 D9_jsc_WFC[M_jsc_ND * HS_N];
  cm_cnt D9_jsc_G[D9_jsc_g_Volume * M_jsc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D9_jsc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_a_Volume, D9_jsc_hA) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_b_Volume, D9_jsc_hB) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_c_Volume, D9_jsc_hC) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_d_Volume, D9_jsc_hD) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_e_Volume, D9_jsc_hE) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_h_Volume, D9_jsc_hH) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_i_Volume, D9_jsc_hI) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_j_Volume, D9_jsc_hJ) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_k_Volume, D9_jsc_hK) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_jsc_l_Volume, D9_jsc_hL) * M_jsc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D9_jsc_t_Volume, D9_jsc_hT) * M_jsc_MND)
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND * cm_apm_q(D9_jsc_qs))
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D9_jsc_f_Volume, D9_jsc_hF) * M_jsc_ND * cm_apm_q(D9_jsc_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_jsc_m_Volume, D9_jsc_hM) * M_jsc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_jsc_m_Volume, D9_jsc_hM) * M_jsc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_jsc_n_Volume, D9_jsc_hN) * M_jsc_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_jsc_n_Volume, D9_jsc_hN) * M_jsc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D9_jsc_w_Volume * M_jsc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D9_jsc_w_Volume * M_jsc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_jsc_s_Volume, D9_jsc_hS) * M_jsc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_jsc_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_jsc_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D9_jsc_g_Volume * M_jsc_NG);
  void D9_jsc_Init( void ) {}
  void D9_jsc_Quit( void ) {}
};
