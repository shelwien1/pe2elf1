
static const byte H9_scf_a_pa[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H9_scf_a_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H9_scf_b_pd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H9_scf_b_d1[3+1]={ 0,1,2,3 };

static const byte H9_scf_c_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50 };

static const byte H9_scf_d_d1[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H9_scf_d_d2[5+1]={ 0,1,2,3,4,5 };

static const byte H9_scf_e_oc[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H9_scf_e_pa[5+1]={ 0,1,2,3,4,5 };

static const byte H9_scf_f_d1[3+1]={ 0,1,2,3 };

static const byte H9_scf_g_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H9_scf_h_cb[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H9_scf_h_pa[3+1]={ 0,1,2,3 };

static const byte H9_scf_i_gd[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H9_scf_i_pa[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H9_scf_j_gr[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H9_scf_k_pv[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H9_scf_k_sb[5+1]={ 0,1,2,3,4,5 };

static const byte H9_scf_l_pa[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H9_scf_l_pd[5+1]={ 0,1,2,3,4,5 };



static const byte H9_scf_s_pa[3+1]={ 0,1,2,3 };




static const int H9_scf_rA = (130+0) * (1);
static const int H9_scf_rB = (242+0) * (1);
static const int H9_scf_rC = (255+0) * (1);
static const int H9_scf_rD = (255+0) * (1);
static const int H9_scf_rE = (255+0) * (1);
static const int H9_scf_rH = (255+0) * (1);
static const int H9_scf_rI = (137+0) * (1);
static const int H9_scf_rJ = (254+0) * (1);
static const int H9_scf_rK = (255+0) * (1);
static const int H9_scf_rL = (253+0) * (1);
static const int H9_scf_rT = (128+0) * (1);
static const int H9_scf_rG = (64+0) * (1);
static const int H9_scf_rS1 = (8+0) * (1);
static const int H9_scf_rS2 = (6+0) * (1);
static const int H9_scf_lr = (15+0) * (1);
static const int H9_scf_mwA = (128+0) * (1);
static const int H9_scf_mwB = (4+0) * (1);
static const int H9_scf_mwC = (4+0) * (1);
static const int H9_scf_mwD = (27+0) * (1);
static const int H9_scf_mwE = (16+0) * (1);
static const int H9_scf_mwH = (8+0) * (1);
static const int H9_scf_mwI = (1+0) * (1);
static const int H9_scf_mwJ = (20+0) * (1);
static const int H9_scf_mwK = (51+0) * (1);
static const int H9_scf_mwL = (62+0) * (1);
static const int H9_scf_mwT = (0+0) * (1);
static const int H9_scf_mwG = (0+0) * (1);
static const int H9_scf_bw = (12+0) * (1);
static const int H9_scf_mt = (2+0) * (1);
static const int H9_scf_qs = (23+3) * (1);
static const int H9_scf_qf = (17+3) * (1);
static const int H9_scf_mb = (33+0) * (1);
static const int H9_scf_lrm = (0+0) * (1);
static const int H9_scf_mbm = (0+0) * (1);
static const int H9_scf_hA = (24+0) * (1);
static const int H9_scf_hB = (24+0) * (1);
static const int H9_scf_hC = (24+0) * (1);
static const int H9_scf_hD = (24+0) * (1);
static const int H9_scf_hE = (24+0) * (1);
static const int H9_scf_hH = (24+0) * (1);
static const int H9_scf_hI = (24+0) * (1);
static const int H9_scf_hJ = (24+0) * (1);
static const int H9_scf_hK = (24+0) * (1);
static const int H9_scf_hL = (24+0) * (1);
static const int H9_scf_hS = (24+0) * (1);
static const int H9_scf_hF = (24+0) * (1);
static const int H9_scf_hM = (24+0) * (1);
static const int H9_scf_hT = (24+0) * (1);
static const int H9_scf_hN = (24+0) * (1);


static const long long H9_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 6); // 270
static const long long H9_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 4); // 180
static const long long H9_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 51), 2); // 306
static const long long H9_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 14), 6); // 252
static const long long H9_scf_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 14), 6); // 252
static const long long H9_scf_f_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long H9_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H9_scf_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 16), 4); // 192
static const long long H9_scf_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 8); // 192
static const long long H9_scf_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 2); // 48
static const long long H9_scf_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 6); // 198
static const long long H9_scf_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 13), 6); // 234
static const long long H9_scf_m_Volume = tc_vmul(1, 3); // 3
static const long long H9_scf_n_Volume = 1; // 1
static const long long H9_scf_s_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long H9_scf_t_Volume = tc_vmul(1, 3); // 3
static const long long H9_scf_w_Volume = tc_vmul(1, 2); // 2

#define H9_scf_USES(X) X(kd) X(pa) X(d1) X(pd) X(sb) X(ws) X(d2) X(oc) X(cb) X(gd) X(gr) X(pv)

struct H9_scf_T {
  cm_cnt2 H9_scf_A[tc_rows(H9_scf_a_Volume, H9_scf_hA) * H_scf_ND];
  cm_cnt2 H9_scf_B[tc_rows(H9_scf_b_Volume, H9_scf_hB) * H_scf_ND];
  cm_cnt2 H9_scf_C[tc_rows(H9_scf_c_Volume, H9_scf_hC) * H_scf_ND];
  cm_cnt2 H9_scf_D[tc_rows(H9_scf_d_Volume, H9_scf_hD) * H_scf_ND];
  cm_cnt2 H9_scf_E[tc_rows(H9_scf_e_Volume, H9_scf_hE) * H_scf_ND];
  cm_cnt2 H9_scf_H[tc_rows(H9_scf_h_Volume, H9_scf_hH) * H_scf_ND];
  cm_cnt2 H9_scf_I[tc_rows(H9_scf_i_Volume, H9_scf_hI) * H_scf_ND];
  cm_cnt2 H9_scf_J[tc_rows(H9_scf_j_Volume, H9_scf_hJ) * H_scf_ND];
  cm_cnt2 H9_scf_K[tc_rows(H9_scf_k_Volume, H9_scf_hK) * H_scf_ND];
  cm_cnt2 H9_scf_L[tc_rows(H9_scf_l_Volume, H9_scf_hL) * H_scf_ND];
  cm_cnt H9_scf_MT[tc_rows(H9_scf_t_Volume, H9_scf_hT) * H_scf_MND];
  i16 H9_scf_S[tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * cm_apm_q(H9_scf_qs)];
  u8 H9_scf_SC[tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * cm_apm_q(H9_scf_qs)];
  i16 H9_scf_F[tc_rows(H9_scf_f_Volume, H9_scf_hF) * H_scf_ND * cm_apm_q(H9_scf_qf)];
  u8 H9_scf_FC[tc_rows(H9_scf_f_Volume, H9_scf_hF) * H_scf_ND * cm_apm_q(H9_scf_qf)];
  i32 H9_scf_W[tc_rows(H9_scf_m_Volume, H9_scf_hM) * H_scf_ND * TC_MIXN];
  u8 H9_scf_WC[tc_rows(H9_scf_m_Volume, H9_scf_hM) * H_scf_ND];
  i32 H9_scf_WM[tc_rows(H9_scf_n_Volume, H9_scf_hN) * H_scf_MND * 3];
  u8 H9_scf_WMC[tc_rows(H9_scf_n_Volume, H9_scf_hN) * H_scf_MND];
  i32 H9_scf_W2[H9_scf_w_Volume * H_scf_ND * TC_MIXN];
  u8 H9_scf_W2C[H9_scf_w_Volume * H_scf_ND];
  i32 H9_scf_W3[tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * TC_MIXN];
  u8 H9_scf_W3C[tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND];
  i32 H9_scf_WF[H_scf_ND * 5 * 4];
  u8 H9_scf_WFC[H_scf_ND * 5];
  cm_cnt H9_scf_G[H9_scf_g_Volume * H_scf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H9_scf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_a_Volume, H9_scf_hA) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_b_Volume, H9_scf_hB) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_c_Volume, H9_scf_hC) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_d_Volume, H9_scf_hD) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_e_Volume, H9_scf_hE) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_h_Volume, H9_scf_hH) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_i_Volume, H9_scf_hI) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_j_Volume, H9_scf_hJ) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_k_Volume, H9_scf_hK) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_scf_l_Volume, H9_scf_hL) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H9_scf_t_Volume, H9_scf_hT) * H_scf_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * cm_apm_q(H9_scf_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * cm_apm_q(H9_scf_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_scf_f_Volume, H9_scf_hF) * H_scf_ND * cm_apm_q(H9_scf_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_scf_f_Volume, H9_scf_hF) * H_scf_ND * cm_apm_q(H9_scf_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_scf_m_Volume, H9_scf_hM) * H_scf_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_scf_m_Volume, H9_scf_hM) * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_scf_n_Volume, H9_scf_hN) * H_scf_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_scf_n_Volume, H9_scf_hN) * H_scf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H9_scf_w_Volume * H_scf_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H9_scf_w_Volume * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_scf_s_Volume, H9_scf_hS) * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_scf_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_scf_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H9_scf_g_Volume * H_scf_NG);
  void H9_scf_Init( void ) {}
  void H9_scf_Quit( void ) {}
};
