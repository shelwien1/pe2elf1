



static const byte D9_hdr_d_pos[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

static const byte D9_hdr_e_nch[3+1]={ 0,1,2,3 };



static const byte D9_hdr_h_pos[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };



static const byte D9_hdr_k_pos[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

static const byte D9_hdr_l_nch[3+1]={ 0,1,2,3 };







static const int D9_hdr_rA = (0+0) * (1);
static const int D9_hdr_mwA = (12+0) * (1);
static const int D9_hdr_rB = (255+0) * (1);
static const int D9_hdr_mwB = (20+0) * (1);
static const int D9_hdr_rC = (254+0) * (1);
static const int D9_hdr_mwC = (8+0) * (1);
static const int D9_hdr_rD = (255+0) * (1);
static const int D9_hdr_mwD = (5+0) * (1);
static const int D9_hdr_rE = (164+0) * (1);
static const int D9_hdr_mwE = (0+0) * (1);
static const int D9_hdr_rH = (191+0) * (1);
static const int D9_hdr_mwH = (4+0) * (1);
static const int D9_hdr_rI = (223+0) * (1);
static const int D9_hdr_mwI = (4+0) * (1);
static const int D9_hdr_rJ = (252+0) * (1);
static const int D9_hdr_mwJ = (1+0) * (1);
static const int D9_hdr_rK = (255+0) * (1);
static const int D9_hdr_mwK = (32+0) * (1);
static const int D9_hdr_rL = (251+0) * (1);
static const int D9_hdr_mwL = (4+0) * (1);
static const int D9_hdr_lr = (14+0) * (1);
static const int D9_hdr_mb = (129+0) * (1);
static const int D9_hdr_rS1 = (8+0) * (1);
static const int D9_hdr_rS2 = (8+0) * (1);
static const int D9_hdr_qs = (47+3) * (1);
static const int D9_hdr_qf = (17+3) * (1);
static const int D9_hdr_bw = (8+0) * (1);
static const int D9_hdr_rT = (128+0) * (1);
static const int D9_hdr_mwT = (31+0) * (1);
static const int D9_hdr_mt = (6+0) * (1);
static const int D9_hdr_lrm = (31+0) * (1);
static const int D9_hdr_mbm = (25+0) * (1);
static const int D9_hdr_rG = (255+0) * (1);
static const int D9_hdr_mwG = (96+0) * (1);
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


static const long long D9_hdr_a_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<6)), (1<<6)); // 4096
static const long long D9_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 3); // 12288
static const long long D9_hdr_d_Volume = tc_vmul(tc_vmul(1, (1<<6)), 32); // 2048
static const long long D9_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 3), 4); // 768
static const long long D9_hdr_f_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 32); // 131072
static const long long D9_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 3), 3); // 576
static const long long D9_hdr_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 3); // 12288
static const long long D9_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 32), 3); // 6144
static const long long D9_hdr_l_Volume = tc_vmul(tc_vmul(1, (1<<6)), 4); // 256
static const long long D9_hdr_m_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_s_Volume = tc_vmul(tc_vmul(1, (1<<6)), 3); // 192
static const long long D9_hdr_t_Volume = tc_vmul(1, (1<<6)); // 64
static const long long D9_hdr_w_Volume = tc_vmul(tc_vmul(1, 3), 3); // 9

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
