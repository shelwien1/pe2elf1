
static const byte H7_scf_a_pa[14+1]={ 0,0,0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H7_scf_a_d1[5+1]={ 0,0,1,2,3,4 };

static const byte H7_scf_b_pd[14+1]={ 0,0,1,1,1,1,1,1,2,2,3,3,4,4,5 };

static const byte H7_scf_c_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,18,18,19,19,19,20,21,22,23,24,24,25,25,26,27,28,29,29,30,31,32,33,34,35,36,37,38,39,39,40,40,40 };

static const byte H7_scf_d_d1[13+1]={ 0,1,2,3,4,5,6,7,8,8,8,8,9,10 };
static const byte H7_scf_d_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H7_scf_e_oc[13+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,6,6 };

static const byte H7_scf_g_d1[5+1]={ 0,1,2,3,4,5 };


static const byte H7_scf_i_gd[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_scf_i_pa[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_scf_j_pw[25+1]={ 0,1,2,3,4,5,6,6,6,6,6,6,6,7,8,9,10,11,12,13,13,13,13,13,13,13 };

static const byte H7_scf_k_pv[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H7_scf_k_sk[15+1]={ 0,1,1,1,2,2,2,2,3,3,3,3,3,3,3,3 };

static const byte H7_scf_l_pa[12+1]={ 0,0,0,0,1,2,3,4,5,6,7,8,9 };
static const byte H7_scf_l_pd[5+1]={ 0,1,1,2,3,4 };


static const byte H7_scf_s_pa[3+1]={ 0,1,2,3 };




static const int H7_scf_rA = (255+0) * (1);
static const int H7_scf_rB = (252+0) * (1);
static const int H7_scf_rC = (255+0) * (1);
static const int H7_scf_rD = (255+0) * (1);
static const int H7_scf_rE = (255+0) * (1);
static const int H7_scf_rH = (128+0) * (1);
static const int H7_scf_rI = (252+0) * (1);
static const int H7_scf_rJ = (75+0) * (1);
static const int H7_scf_rK = (255+0) * (1);
static const int H7_scf_rL = (253+0) * (1);
static const int H7_scf_rT = (192+0) * (1);
static const int H7_scf_rG = (126+0) * (1);
static const int H7_scf_lr = (15+0) * (1);
static const int H7_scf_mwA = (26+0) * (1);
static const int H7_scf_mwB = (0+0) * (1);
static const int H7_scf_mwC = (1+0) * (1);
static const int H7_scf_mwD = (29+0) * (1);
static const int H7_scf_mwE = (5+0) * (1);
static const int H7_scf_mwH = (128+0) * (1);
static const int H7_scf_mwI = (177+0) * (1);
static const int H7_scf_mwJ = (16+0) * (1);
static const int H7_scf_mwK = (0+0) * (1);
static const int H7_scf_mwL = (63+0) * (1);
static const int H7_scf_mwT = (17+0) * (1);
static const int H7_scf_mwG = (8+0) * (1);
static const int H7_scf_mt = (0+0) * (1);
static const int H7_scf_mb = (54+0) * (1);
static const int H7_scf_hA = (24+0) * (1);
static const int H7_scf_hB = (24+0) * (1);
static const int H7_scf_hC = (24+0) * (1);
static const int H7_scf_hD = (24+0) * (1);
static const int H7_scf_hE = (24+0) * (1);
static const int H7_scf_hH = (24+0) * (1);
static const int H7_scf_hI = (24+0) * (1);
static const int H7_scf_hJ = (24+0) * (1);
static const int H7_scf_hK = (24+0) * (1);
static const int H7_scf_hL = (24+0) * (1);
static const int H7_scf_hS = (24+0) * (1);
static const int H7_scf_hM = (24+0) * (1);
static const int H7_scf_hT = (24+0) * (1);


static const long long H7_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 13), 5); // 130
static const long long H7_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 2); // 12
static const long long H7_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 41), 2); // 164
static const long long H7_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 2), 61); // 4026
static const long long H7_scf_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 1), 3); // 21
static const long long H7_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H7_scf_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 2); // 4
static const long long H7_scf_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 8); // 192
static const long long H7_scf_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 2), 14); // 252
static const long long H7_scf_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 3), 4); // 396
static const long long H7_scf_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 5); // 150
static const long long H7_scf_m_Volume = tc_vmul(tc_vmul(1, 2), 3); // 6
static const long long H7_scf_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 4), 3); // 24
static const long long H7_scf_t_Volume = tc_vmul(1, 2); // 2
static const long long H7_scf_w_Volume = tc_vmul(1, 2); // 2

#define H7_scf_USES(X) X(kd) X(pa) X(d1) X(pd) X(sb) X(ws) X(d2) X(dm) X(oc) X(mu) X(gd) X(gr) X(pw) X(pv) X(sk) X(fb)

struct H7_scf_T {
  cm_cnt2 H7_scf_A[tc_rows(H7_scf_a_Volume, H7_scf_hA) * H_scf_ND];
  cm_cnt2 H7_scf_B[tc_rows(H7_scf_b_Volume, H7_scf_hB) * H_scf_ND];
  cm_cnt2 H7_scf_C[tc_rows(H7_scf_c_Volume, H7_scf_hC) * H_scf_ND];
  cm_cnt2 H7_scf_D[tc_rows(H7_scf_d_Volume, H7_scf_hD) * H_scf_ND];
  cm_cnt2 H7_scf_E[tc_rows(H7_scf_e_Volume, H7_scf_hE) * H_scf_ND];
  cm_cnt2 H7_scf_H[tc_rows(H7_scf_h_Volume, H7_scf_hH) * H_scf_ND];
  cm_cnt2 H7_scf_I[tc_rows(H7_scf_i_Volume, H7_scf_hI) * H_scf_ND];
  cm_cnt2 H7_scf_J[tc_rows(H7_scf_j_Volume, H7_scf_hJ) * H_scf_ND];
  cm_cnt2 H7_scf_K[tc_rows(H7_scf_k_Volume, H7_scf_hK) * H_scf_ND];
  cm_cnt2 H7_scf_L[tc_rows(H7_scf_l_Volume, H7_scf_hL) * H_scf_ND];
  cm_cnt H7_scf_MT[tc_rows(H7_scf_t_Volume, H7_scf_hT) * H_scf_MND];
  i32 H7_scf_W[tc_rows(H7_scf_m_Volume, H7_scf_hM) * H_scf_ND * tc_nin(10)];
  u8 H7_scf_WC[tc_rows(H7_scf_m_Volume, H7_scf_hM) * H_scf_ND];
  i32 H7_scf_W2[H7_scf_w_Volume * H_scf_ND * tc_nin(10)];
  u8 H7_scf_W2C[H7_scf_w_Volume * H_scf_ND];
  i32 H7_scf_W3[tc_rows(H7_scf_s_Volume, H7_scf_hS) * H_scf_ND * tc_nin(10)];
  u8 H7_scf_W3C[tc_rows(H7_scf_s_Volume, H7_scf_hS) * H_scf_ND];
  i32 H7_scf_WF[H_scf_ND * 4];
  u8 H7_scf_WFC[H_scf_ND];
  cm_cnt H7_scf_G[H7_scf_g_Volume * H_scf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_scf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_a_Volume, H7_scf_hA) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_b_Volume, H7_scf_hB) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_c_Volume, H7_scf_hC) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_d_Volume, H7_scf_hD) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_e_Volume, H7_scf_hE) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_h_Volume, H7_scf_hH) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_i_Volume, H7_scf_hI) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_j_Volume, H7_scf_hJ) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_k_Volume, H7_scf_hK) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_scf_l_Volume, H7_scf_hL) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_scf_t_Volume, H7_scf_hT) * H_scf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_scf_m_Volume, H7_scf_hM) * H_scf_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_scf_m_Volume, H7_scf_hM) * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_scf_w_Volume * H_scf_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_scf_w_Volume * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_scf_s_Volume, H7_scf_hS) * H_scf_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_scf_s_Volume, H7_scf_hS) * H_scf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_scf_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_scf_g_Volume * H_scf_NG);
  void H7_scf_Init( void ) {}
  void H7_scf_Quit( void ) {}
};
