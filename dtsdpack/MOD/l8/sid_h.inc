


static const byte D8_sid_c_bd[31+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,4,4,4,4,4,5,5,6,6,6,6,6,6,6,6,7,7,8,8 };

static const byte D8_sid_d_ac[8+1]={ 0,0,0,0,1,2,2,2,3 };

static const byte D8_sid_e_bd[31+1]={ 0,1,1,1,1,2,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,6,6,7 };

static const byte D8_sid_f_ac[8+1]={ 0,1,2,3,4,5,6,7,7 };

static const byte D8_sid_g_fld[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte D8_sid_h_ab[27+1]={ 0,0,0,1,2,3,3,3,3,3,4,5,5,5,5,6,7,8,8,8,8,8,9,10,11,12,12,12 };



static const byte D8_sid_k_bd[31+1]={ 0,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,4,5,5,5,5,6 };
static const byte D8_sid_k_ac[8+1]={ 0,1,2,3,3,4,5,6,6 };

static const byte D8_sid_l_ab[27+1]={ 0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,3,3,4,5,5,5,5,6,7,7,7 };
static const byte D8_sid_l_ac[8+1]={ 0,1,2,3,4,5,6,6,6 };


static const byte D8_sid_n_fld[8+1]={ 0,1,2,3,4,5,6,7,8 };





static const int D8_sid_rA = (162+0) * (1);
static const int D8_sid_mwA = (252+0) * (1);
static const int D8_sid_rB = (123+0) * (1);
static const int D8_sid_mwB = (65+0) * (1);
static const int D8_sid_rC = (37+0) * (1);
static const int D8_sid_mwC = (57+0) * (1);
static const int D8_sid_rD = (103+0) * (1);
static const int D8_sid_mwD = (69+0) * (1);
static const int D8_sid_rE = (82+0) * (1);
static const int D8_sid_mwE = (191+0) * (1);
static const int D8_sid_rH = (57+0) * (1);
static const int D8_sid_mwH = (59+0) * (1);
static const int D8_sid_rI = (255+0) * (1);
static const int D8_sid_mwI = (9+0) * (1);
static const int D8_sid_rJ = (0+0) * (1);
static const int D8_sid_mwJ = (30+0) * (1);
static const int D8_sid_rK = (245+0) * (1);
static const int D8_sid_mwK = (1+0) * (1);
static const int D8_sid_rL = (87+0) * (1);
static const int D8_sid_mwL = (37+0) * (1);
static const int D8_sid_lr = (15+0) * (1);
static const int D8_sid_mb = (65+0) * (1);
static const int D8_sid_wp = (16+0) * (1);
static const int D8_sid_wq = (0+0) * (1);
static const int D8_sid_wo = (64+0) * (1);
static const int D8_sid_wa = (1+0) * (1);
static const int D8_sid_wf = (31+0) * (1);
static const int D8_sid_lf = (27+0) * (1);
static const int D8_sid_bf = (19+0) * (1);
static const int D8_sid_l2 = (0+0) * (1);
static const int D8_sid_l3 = (128+0) * (1);
static const int D8_sid_b2 = (128+0) * (1);
static const int D8_sid_b3 = (31+0) * (1);
static const int D8_sid_rS1 = (8+0) * (1);
static const int D8_sid_rS2 = (4+0) * (1);
static const int D8_sid_qs = (88+3) * (1);
static const int D8_sid_qf = (63+3) * (1);
static const int D8_sid_bw = (16+0) * (1);
static const int D8_sid_rT = (255+0) * (1);
static const int D8_sid_mwT = (32+0) * (1);
static const int D8_sid_mt = (0+0) * (1);
static const int D8_sid_lrm = (31+0) * (1);
static const int D8_sid_mbm = (25+0) * (1);
static const int D8_sid_rG = (0+0) * (1);
static const int D8_sid_mwG = (32+0) * (1);
static const int D8_sid_hA = (22+0) * (1);
static const int D8_sid_hB = (22+0) * (1);
static const int D8_sid_hC = (22+0) * (1);
static const int D8_sid_hD = (22+0) * (1);
static const int D8_sid_hE = (22+0) * (1);
static const int D8_sid_hH = (22+0) * (1);
static const int D8_sid_hI = (22+0) * (1);
static const int D8_sid_hJ = (22+0) * (1);
static const int D8_sid_hK = (22+0) * (1);
static const int D8_sid_hL = (22+0) * (1);
static const int D8_sid_hS = (22+0) * (1);
static const int D8_sid_hF = (22+0) * (1);
static const int D8_sid_hM = (22+0) * (1);
static const int D8_sid_hT = (22+0) * (1);
static const int D8_sid_hN = (22+0) * (1);


static const long long D8_sid_a_Volume = tc_vmul(tc_vmul(1, 3), (1<<2)); // 12
static const long long D8_sid_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), (1<<2)), (1<<6)); // 512
static const long long D8_sid_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 9), (1<<2)); // 72
static const long long D8_sid_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 4), (1<<2)); // 16
static const long long D8_sid_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 8); // 16
static const long long D8_sid_f_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long D8_sid_g_Volume = tc_vmul(1, 9); // 9
static const long long D8_sid_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 13), 3); // 39
static const long long D8_sid_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long D8_sid_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), (1<<1)), 3); // 6
static const long long D8_sid_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 7); // 98
static const long long D8_sid_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 7); // 112
static const long long D8_sid_m_Volume = tc_vmul(1, 2); // 2
static const long long D8_sid_n_Volume = tc_vmul(1, 9); // 9
static const long long D8_sid_s_Volume = tc_vmul(tc_vmul(1, 1), (1<<1)); // 2
static const long long D8_sid_t_Volume = tc_vmul(1, 1); // 1
static const long long D8_sid_w_Volume = tc_vmul(1, 2); // 2

#define D8_sid_USES(X) X(fld) X(lv) X(pq) X(bd) X(ac) X(cc) X(ab) X(tr) X(ec) X(st)

struct D8_sid_T {
  cm_cnt2 D8_sid_A[tc_rows(D8_sid_a_Volume, D8_sid_hA) * M_sid_ND];
  cm_cnt2 D8_sid_B[tc_rows(D8_sid_b_Volume, D8_sid_hB) * M_sid_ND];
  cm_cnt2 D8_sid_C[tc_rows(D8_sid_c_Volume, D8_sid_hC) * M_sid_ND];
  cm_cnt2 D8_sid_D[tc_rows(D8_sid_d_Volume, D8_sid_hD) * M_sid_ND];
  cm_cnt2 D8_sid_E[tc_rows(D8_sid_e_Volume, D8_sid_hE) * M_sid_ND];
  cm_cnt2 D8_sid_H[tc_rows(D8_sid_h_Volume, D8_sid_hH) * M_sid_ND];
  cm_cnt2 D8_sid_I[tc_rows(D8_sid_i_Volume, D8_sid_hI) * M_sid_ND];
  cm_cnt2 D8_sid_J[tc_rows(D8_sid_j_Volume, D8_sid_hJ) * M_sid_ND];
  cm_cnt2 D8_sid_K[tc_rows(D8_sid_k_Volume, D8_sid_hK) * M_sid_ND];
  cm_cnt2 D8_sid_L[tc_rows(D8_sid_l_Volume, D8_sid_hL) * M_sid_ND];
  cm_cnt D8_sid_MT[tc_rows(D8_sid_t_Volume, D8_sid_hT) * M_sid_MND];
  cm_apme D8_sid_S[tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND * cm_apm_q(D8_sid_qs)];
  cm_apme D8_sid_F[tc_rows(D8_sid_f_Volume, D8_sid_hF) * M_sid_ND * cm_apm_q(D8_sid_qf)];
  i32 D8_sid_W[tc_rows(D8_sid_m_Volume, D8_sid_hM) * M_sid_ND * tc_nin(10)];
  u8 D8_sid_WC[tc_rows(D8_sid_m_Volume, D8_sid_hM) * M_sid_ND];
  i32 D8_sid_WM[tc_rows(D8_sid_n_Volume, D8_sid_hN) * M_sid_MND * TC_MIN];
  u8 D8_sid_WMC[tc_rows(D8_sid_n_Volume, D8_sid_hN) * M_sid_MND];
  i32 D8_sid_W2[D8_sid_w_Volume * M_sid_ND * tc_nin(10)];
  u8 D8_sid_W2C[D8_sid_w_Volume * M_sid_ND];
  i32 D8_sid_W3[tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND * tc_nin(10)];
  u8 D8_sid_W3C[tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND];
  i32 D8_sid_WF[M_sid_ND * HS_N * TC_FIN];
  u8 D8_sid_WFC[M_sid_ND * HS_N];
  cm_cnt D8_sid_G[D8_sid_g_Volume * M_sid_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D8_sid_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_a_Volume, D8_sid_hA) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_b_Volume, D8_sid_hB) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_c_Volume, D8_sid_hC) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_d_Volume, D8_sid_hD) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_e_Volume, D8_sid_hE) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_h_Volume, D8_sid_hH) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_i_Volume, D8_sid_hI) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_j_Volume, D8_sid_hJ) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_k_Volume, D8_sid_hK) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_sid_l_Volume, D8_sid_hL) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D8_sid_t_Volume, D8_sid_hT) * M_sid_MND)
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND * cm_apm_q(D8_sid_qs))
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D8_sid_f_Volume, D8_sid_hF) * M_sid_ND * cm_apm_q(D8_sid_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_sid_m_Volume, D8_sid_hM) * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_sid_m_Volume, D8_sid_hM) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_sid_n_Volume, D8_sid_hN) * M_sid_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_sid_n_Volume, D8_sid_hN) * M_sid_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D8_sid_w_Volume * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D8_sid_w_Volume * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_sid_s_Volume, D8_sid_hS) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_sid_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_sid_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D8_sid_g_Volume * M_sid_NG);
  void D8_sid_Init( void ) {}
  void D8_sid_Quit( void ) {}
};
