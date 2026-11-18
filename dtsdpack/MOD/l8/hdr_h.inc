



static const byte D8_hdr_d_pos[31+1]={ 0,0,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3 };




static const byte D8_hdr_h_pos[31+1]={ 0,1,1,1,1,2,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,6,6,6,6,7,7,7,7,7,7,7 };



static const byte D8_hdr_k_pos[31+1]={ 0,0,0,0,0,1,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,6,6,7,7,7,7,7,7,7 };








static const int D8_hdr_rA = (0+0) * (1);
static const int D8_hdr_mwA = (143+0) * (1);
static const int D8_hdr_rB = (0+0) * (1);
static const int D8_hdr_mwB = (0+0) * (1);
static const int D8_hdr_rC = (90+0) * (1);
static const int D8_hdr_mwC = (125+0) * (1);
static const int D8_hdr_rD = (0+0) * (1);
static const int D8_hdr_mwD = (0+0) * (1);
static const int D8_hdr_rE = (54+0) * (1);
static const int D8_hdr_mwE = (31+0) * (1);
static const int D8_hdr_rH = (95+0) * (1);
static const int D8_hdr_mwH = (0+0) * (1);
static const int D8_hdr_rI = (97+0) * (1);
static const int D8_hdr_mwI = (140+0) * (1);
static const int D8_hdr_rJ = (102+0) * (1);
static const int D8_hdr_mwJ = (28+0) * (1);
static const int D8_hdr_rK = (130+0) * (1);
static const int D8_hdr_mwK = (36+0) * (1);
static const int D8_hdr_rL = (0+0) * (1);
static const int D8_hdr_mwL = (133+0) * (1);
static const int D8_hdr_lr = (15+0) * (1);
static const int D8_hdr_mb = (212+0) * (1);
static const int D8_hdr_wp = (19+0) * (1);
static const int D8_hdr_wq = (2+0) * (1);
static const int D8_hdr_wo = (64+0) * (1);
static const int D8_hdr_wa = (16+0) * (1);
static const int D8_hdr_wf = (77+0) * (1);
static const int D8_hdr_lf = (16+0) * (1);
static const int D8_hdr_bf = (16+0) * (1);
static const int D8_hdr_l2 = (16+0) * (1);
static const int D8_hdr_l3 = (16+0) * (1);
static const int D8_hdr_b2 = (16+0) * (1);
static const int D8_hdr_b3 = (16+0) * (1);
static const int D8_hdr_rS1 = (3+0) * (1);
static const int D8_hdr_rS2 = (8+0) * (1);
static const int D8_hdr_qs = (115+3) * (1);
static const int D8_hdr_qf = (33+3) * (1);
static const int D8_hdr_bw = (5+0) * (1);
static const int D8_hdr_rT = (0+0) * (1);
static const int D8_hdr_mwT = (0+0) * (1);
static const int D8_hdr_mt = (6+0) * (1);
static const int D8_hdr_lrm = (31+0) * (1);
static const int D8_hdr_mbm = (25+0) * (1);
static const int D8_hdr_rG = (0+0) * (1);
static const int D8_hdr_mwG = (0+0) * (1);
static const int D8_hdr_hA = (22+0) * (1);
static const int D8_hdr_hB = (22+0) * (1);
static const int D8_hdr_hC = (22+0) * (1);
static const int D8_hdr_hD = (22+0) * (1);
static const int D8_hdr_hE = (22+0) * (1);
static const int D8_hdr_hH = (22+0) * (1);
static const int D8_hdr_hI = (22+0) * (1);
static const int D8_hdr_hJ = (22+0) * (1);
static const int D8_hdr_hK = (22+0) * (1);
static const int D8_hdr_hL = (22+0) * (1);
static const int D8_hdr_hS = (22+0) * (1);
static const int D8_hdr_hF = (22+0) * (1);
static const int D8_hdr_hM = (22+0) * (1);
static const int D8_hdr_hT = (22+0) * (1);
static const int D8_hdr_hN = (22+0) * (1);


static const long long D8_hdr_a_Volume = tc_vmul(1, (1<<2)); // 4
static const long long D8_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<3)); // 256
static const long long D8_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), (1<<1)), 2); // 8
static const long long D8_hdr_d_Volume = tc_vmul(tc_vmul(1, (1<<5)), 4); // 128
static const long long D8_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 3), 3); // 18
static const long long D8_hdr_f_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D8_hdr_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D8_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<4)), (1<<2)), 8); // 512
static const long long D8_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 3), 3); // 72
static const long long D8_hdr_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), (1<<1)), 3); // 24
static const long long D8_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<4)), 8), 1); // 128
static const long long D8_hdr_l_Volume = tc_vmul(tc_vmul(1, (1<<4)), 3); // 48
static const long long D8_hdr_m_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D8_hdr_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D8_hdr_s_Volume = tc_vmul(tc_vmul(1, (1<<6)), 2); // 128
static const long long D8_hdr_t_Volume = tc_vmul(1, (1<<3)); // 8
static const long long D8_hdr_w_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3

#define D8_hdr_USES(X) X(fld) X(lv) X(l2) X(pos) X(st) X(nch)

struct D8_hdr_T {
  cm_cnt2 D8_hdr_A[tc_rows(D8_hdr_a_Volume, D8_hdr_hA) * M_hdr_ND];
  cm_cnt2 D8_hdr_B[tc_rows(D8_hdr_b_Volume, D8_hdr_hB) * M_hdr_ND];
  cm_cnt2 D8_hdr_C[tc_rows(D8_hdr_c_Volume, D8_hdr_hC) * M_hdr_ND];
  cm_cnt2 D8_hdr_D[tc_rows(D8_hdr_d_Volume, D8_hdr_hD) * M_hdr_ND];
  cm_cnt2 D8_hdr_E[tc_rows(D8_hdr_e_Volume, D8_hdr_hE) * M_hdr_ND];
  cm_cnt2 D8_hdr_H[tc_rows(D8_hdr_h_Volume, D8_hdr_hH) * M_hdr_ND];
  cm_cnt2 D8_hdr_I[tc_rows(D8_hdr_i_Volume, D8_hdr_hI) * M_hdr_ND];
  cm_cnt2 D8_hdr_J[tc_rows(D8_hdr_j_Volume, D8_hdr_hJ) * M_hdr_ND];
  cm_cnt2 D8_hdr_K[tc_rows(D8_hdr_k_Volume, D8_hdr_hK) * M_hdr_ND];
  cm_cnt2 D8_hdr_L[tc_rows(D8_hdr_l_Volume, D8_hdr_hL) * M_hdr_ND];
  cm_cnt D8_hdr_MT[tc_rows(D8_hdr_t_Volume, D8_hdr_hT) * M_hdr_MND];
  cm_apme D8_hdr_S[tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND * cm_apm_q(D8_hdr_qs)];
  cm_apme D8_hdr_F[tc_rows(D8_hdr_f_Volume, D8_hdr_hF) * M_hdr_ND * cm_apm_q(D8_hdr_qf)];
  i32 D8_hdr_W[tc_rows(D8_hdr_m_Volume, D8_hdr_hM) * M_hdr_ND * tc_nin(10)];
  u8 D8_hdr_WC[tc_rows(D8_hdr_m_Volume, D8_hdr_hM) * M_hdr_ND];
  i32 D8_hdr_WM[tc_rows(D8_hdr_n_Volume, D8_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 D8_hdr_WMC[tc_rows(D8_hdr_n_Volume, D8_hdr_hN) * M_hdr_MND];
  i32 D8_hdr_W2[D8_hdr_w_Volume * M_hdr_ND * tc_nin(10)];
  u8 D8_hdr_W2C[D8_hdr_w_Volume * M_hdr_ND];
  i32 D8_hdr_W3[tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND * tc_nin(10)];
  u8 D8_hdr_W3C[tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND];
  i32 D8_hdr_WF[M_hdr_ND * HS_N * TC_FIN];
  u8 D8_hdr_WFC[M_hdr_ND * HS_N];
  cm_cnt D8_hdr_G[D8_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D8_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_a_Volume, D8_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_b_Volume, D8_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_c_Volume, D8_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_d_Volume, D8_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_e_Volume, D8_hdr_hE) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_h_Volume, D8_hdr_hH) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_i_Volume, D8_hdr_hI) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_j_Volume, D8_hdr_hJ) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_k_Volume, D8_hdr_hK) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D8_hdr_l_Volume, D8_hdr_hL) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D8_hdr_t_Volume, D8_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND * cm_apm_q(D8_hdr_qs))
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D8_hdr_f_Volume, D8_hdr_hF) * M_hdr_ND * cm_apm_q(D8_hdr_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_hdr_m_Volume, D8_hdr_hM) * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_hdr_m_Volume, D8_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_hdr_n_Volume, D8_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_hdr_n_Volume, D8_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D8_hdr_w_Volume * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D8_hdr_w_Volume * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D8_hdr_s_Volume, D8_hdr_hS) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_hdr_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_hdr_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D8_hdr_g_Volume * M_hdr_NG);
  void D8_hdr_Init( void ) {}
  void D8_hdr_Quit( void ) {}
};
