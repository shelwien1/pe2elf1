




static const byte D9_hdr_e_nch[3+1]={ 0,1,2,3 };



static const byte D9_hdr_h_pos[31+1]={ 0,1,1,1,1,2,2,3,4,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7 };



static const byte D9_hdr_k_pos[31+1]={ 0,1,1,1,1,1,1,2,3,4,4,4,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,8 };








static const int D9_hdr_rA = (0+0) * (1);
static const int D9_hdr_mwA = (207+0) * (1);
static const int D9_hdr_rB = (0+0) * (1);
static const int D9_hdr_mwB = (33+0) * (1);
static const int D9_hdr_rC = (254+0) * (1);
static const int D9_hdr_mwC = (107+0) * (1);
static const int D9_hdr_rD = (0+0) * (1);
static const int D9_hdr_mwD = (8+0) * (1);
static const int D9_hdr_rE = (234+0) * (1);
static const int D9_hdr_mwE = (63+0) * (1);
static const int D9_hdr_rH = (254+0) * (1);
static const int D9_hdr_mwH = (0+0) * (1);
static const int D9_hdr_rI = (16+0) * (1);
static const int D9_hdr_mwI = (4+0) * (1);
static const int D9_hdr_rJ = (92+0) * (1);
static const int D9_hdr_mwJ = (235+0) * (1);
static const int D9_hdr_rK = (255+0) * (1);
static const int D9_hdr_mwK = (0+0) * (1);
static const int D9_hdr_rL = (0+0) * (1);
static const int D9_hdr_mwL = (169+0) * (1);
static const int D9_hdr_lr = (15+0) * (1);
static const int D9_hdr_mb = (145+0) * (1);
static const int D9_hdr_rS1 = (8+0) * (1);
static const int D9_hdr_rS2 = (8+0) * (1);
static const int D9_hdr_qs = (101+3) * (1);
static const int D9_hdr_qf = (43+3) * (1);
static const int D9_hdr_bw = (16+0) * (1);
static const int D9_hdr_rT = (0+0) * (1);
static const int D9_hdr_mwT = (0+0) * (1);
static const int D9_hdr_mt = (6+0) * (1);
static const int D9_hdr_lrm = (31+0) * (1);
static const int D9_hdr_mbm = (25+0) * (1);
static const int D9_hdr_rG = (224+0) * (1);
static const int D9_hdr_mwG = (0+0) * (1);
static const int D9_hdr_hA = (22+0) * (1);
static const int D9_hdr_hB = (22+0) * (1);
static const int D9_hdr_hC = (22+0) * (1);
static const int D9_hdr_hD = (22+0) * (1);
static const int D9_hdr_hE = (22+0) * (1);
static const int D9_hdr_hH = (22+0) * (1);
static const int D9_hdr_hI = (22+0) * (1);
static const int D9_hdr_hJ = (22+0) * (1);
static const int D9_hdr_hK = (22+0) * (1);
static const int D9_hdr_hL = (22+0) * (1);
static const int D9_hdr_hS = (22+0) * (1);
static const int D9_hdr_hF = (22+0) * (1);
static const int D9_hdr_hM = (22+0) * (1);
static const int D9_hdr_hT = (22+0) * (1);
static const int D9_hdr_hN = (22+0) * (1);


static const long long D9_hdr_a_Volume = tc_vmul(1, (1<<3)); // 8
static const long long D9_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<6)), (1<<2)); // 256
static const long long D9_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<4)), 3); // 384
static const long long D9_hdr_d_Volume = tc_vmul(tc_vmul(1, (1<<5)), 3); // 96
static const long long D9_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 2), 4); // 16
static const long long D9_hdr_f_Volume = tc_vmul(1, (1<<5)); // 32
static const long long D9_hdr_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<4)), (1<<3)), 8); // 1024
static const long long D9_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 2), 2); // 32
static const long long D9_hdr_j_Volume = tc_vmul(tc_vmul(1, (1<<4)), 3); // 48
static const long long D9_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), 9), 1); // 288
static const long long D9_hdr_l_Volume = tc_vmul(tc_vmul(1, (1<<3)), 1); // 8
static const long long D9_hdr_m_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_s_Volume = tc_vmul(tc_vmul(1, (1<<5)), 3); // 96
static const long long D9_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16
static const long long D9_hdr_w_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3

#define D9_hdr_USES(X) X(fld) X(lv) X(l2) X(pos) X(st) X(nch)

struct D9_hdr_T {
  cm_cnt2 D9_hdr_A[tc_rows(D9_hdr_a_Volume, D9_hdr_hA) * M_hdr_ND];
  cm_cnt2 D9_hdr_B[tc_rows(D9_hdr_b_Volume, D9_hdr_hB) * M_hdr_ND];
  cm_cnt2 D9_hdr_C[tc_rows(D9_hdr_c_Volume, D9_hdr_hC) * M_hdr_ND];
  cm_cnt2 D9_hdr_D[tc_rows(D9_hdr_d_Volume, D9_hdr_hD) * M_hdr_ND];
  cm_cnt2 D9_hdr_E[tc_rows(D9_hdr_e_Volume, D9_hdr_hE) * M_hdr_ND];
  cm_cnt2 D9_hdr_H[tc_rows(D9_hdr_h_Volume, D9_hdr_hH) * M_hdr_ND];
  cm_cnt2 D9_hdr_I[tc_rows(D9_hdr_i_Volume, D9_hdr_hI) * M_hdr_ND];
  cm_cnt2 D9_hdr_J[tc_rows(D9_hdr_j_Volume, D9_hdr_hJ) * M_hdr_ND];
  cm_cnt2 D9_hdr_K[tc_rows(D9_hdr_k_Volume, D9_hdr_hK) * M_hdr_ND];
  cm_cnt2 D9_hdr_L[tc_rows(D9_hdr_l_Volume, D9_hdr_hL) * M_hdr_ND];
  cm_cnt D9_hdr_MT[tc_rows(D9_hdr_t_Volume, D9_hdr_hT) * M_hdr_MND];
  i16 D9_hdr_S[tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * cm_apm_q(D9_hdr_qs)];
  u8 D9_hdr_SC[tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * cm_apm_q(D9_hdr_qs)];
  i16 D9_hdr_F[tc_rows(D9_hdr_f_Volume, D9_hdr_hF) * M_hdr_ND * cm_apm_q(D9_hdr_qf)];
  u8 D9_hdr_FC[tc_rows(D9_hdr_f_Volume, D9_hdr_hF) * M_hdr_ND * cm_apm_q(D9_hdr_qf)];
  i32 D9_hdr_W[tc_rows(D9_hdr_m_Volume, D9_hdr_hM) * M_hdr_ND * tc_nin(10)];
  u8 D9_hdr_WC[tc_rows(D9_hdr_m_Volume, D9_hdr_hM) * M_hdr_ND];
  i32 D9_hdr_WM[tc_rows(D9_hdr_n_Volume, D9_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 D9_hdr_WMC[tc_rows(D9_hdr_n_Volume, D9_hdr_hN) * M_hdr_MND];
  i32 D9_hdr_W2[D9_hdr_w_Volume * M_hdr_ND * tc_nin(10)];
  u8 D9_hdr_W2C[D9_hdr_w_Volume * M_hdr_ND];
  i32 D9_hdr_W3[tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * tc_nin(10)];
  u8 D9_hdr_W3C[tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND];
  i32 D9_hdr_WF[M_hdr_ND * HS_N * TC_FIN];
  u8 D9_hdr_WFC[M_hdr_ND * HS_N];
  cm_cnt D9_hdr_G[D9_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D9_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_a_Volume, D9_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_b_Volume, D9_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_c_Volume, D9_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_d_Volume, D9_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_e_Volume, D9_hdr_hE) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_h_Volume, D9_hdr_hH) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_i_Volume, D9_hdr_hI) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_j_Volume, D9_hdr_hJ) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_k_Volume, D9_hdr_hK) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_hdr_l_Volume, D9_hdr_hL) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D9_hdr_t_Volume, D9_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * cm_apm_q(D9_hdr_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * cm_apm_q(D9_hdr_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(D9_hdr_f_Volume, D9_hdr_hF) * M_hdr_ND * cm_apm_q(D9_hdr_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_hdr_f_Volume, D9_hdr_hF) * M_hdr_ND * cm_apm_q(D9_hdr_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_hdr_m_Volume, D9_hdr_hM) * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_hdr_m_Volume, D9_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_hdr_n_Volume, D9_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_hdr_n_Volume, D9_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D9_hdr_w_Volume * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D9_hdr_w_Volume * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_hdr_s_Volume, D9_hdr_hS) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_hdr_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_hdr_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D9_hdr_g_Volume * M_hdr_NG);
  void D9_hdr_Init( void ) {}
  void D9_hdr_Quit( void ) {}
};
