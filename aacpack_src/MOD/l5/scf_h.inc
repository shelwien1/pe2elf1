
static const byte H5_scf_a_pa[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H5_scf_a_d1[5+1]={ 0,0,1,2,3,4 };

static const byte H5_scf_b_pd[14+1]={ 0,1,1,2,2,2,2,3,3,3,4,4,5,5,6 };

static const byte H5_scf_c_sb[50+1]={ 0,1,1,2,2,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15,16,16,17,17,18,19,20,21,21,22,22,23,24,25,26,26,27,28,29,30,31,32,33,34,35,36,36,37,37,37 };

static const byte H5_scf_d_d1[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H5_scf_d_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H5_scf_e_oc[13+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,6,6 };

static const byte H5_scf_g_d1[5+1]={ 0,1,2,3,4,5 };


static const byte H5_scf_i_gd[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H5_scf_i_pa[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H5_scf_j_gr[7+1]={ 0,0,1,1,2,3,4,5 };
static const byte H5_scf_j_pw[25+1]={ 0,1,1,1,2,3,4,4,4,5,5,5,6,7,8,9,10,11,12,13,14,15,15,15,15,15 };

static const byte H5_scf_k_pv[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H5_scf_k_sb[5+1]={ 0,0,1,2,3,4 };
static const byte H5_scf_k_sk[15+1]={ 0,1,1,1,1,1,2,2,3,3,3,3,3,3,3,3 };

static const byte H5_scf_l_pa[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H5_scf_l_pd[5+1]={ 0,0,0,1,2,3 };




static const int H5_scf_rA = (255+0) * (1);
static const int H5_scf_rB = (172+0) * (1);
static const int H5_scf_rC = (251+0) * (1);
static const int H5_scf_rD = (255+0) * (1);
static const int H5_scf_rE = (255+0) * (1);
static const int H5_scf_rH = (90+0) * (1);
static const int H5_scf_rI = (255+0) * (1);
static const int H5_scf_rJ = (63+0) * (1);
static const int H5_scf_rK = (255+0) * (1);
static const int H5_scf_rL = (254+0) * (1);
static const int H5_scf_rT = (162+0) * (1);
static const int H5_scf_rG = (128+0) * (1);
static const int H5_scf_lr = (15+0) * (1);
static const int H5_scf_mwA = (7+0) * (1);
static const int H5_scf_mwB = (0+0) * (1);
static const int H5_scf_mwC = (0+0) * (1);
static const int H5_scf_mwD = (8+0) * (1);
static const int H5_scf_mwE = (4+0) * (1);
static const int H5_scf_mwH = (189+0) * (1);
static const int H5_scf_mwI = (156+0) * (1);
static const int H5_scf_mwJ = (2+0) * (1);
static const int H5_scf_mwK = (0+0) * (1);
static const int H5_scf_mwL = (0+0) * (1);
static const int H5_scf_mwT = (14+0) * (1);
static const int H5_scf_mwG = (7+0) * (1);
static const int H5_scf_mt = (0+0) * (1);
static const int H5_scf_mb = (27+0) * (1);
static const int H5_scf_hA = (24+0) * (1);
static const int H5_scf_hB = (24+0) * (1);
static const int H5_scf_hC = (24+0) * (1);
static const int H5_scf_hD = (24+0) * (1);
static const int H5_scf_hE = (24+0) * (1);
static const int H5_scf_hH = (24+0) * (1);
static const int H5_scf_hI = (24+0) * (1);
static const int H5_scf_hJ = (24+0) * (1);
static const int H5_scf_hK = (24+0) * (1);
static const int H5_scf_hL = (24+0) * (1);
static const int H5_scf_hM = (24+0) * (1);
static const int H5_scf_hT = (24+0) * (1);


static const long long H5_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 5); // 225
static const long long H5_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 1); // 7
static const long long H5_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 38), 2); // 152
static const long long H5_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 14), 1), 61); // 854
static const long long H5_scf_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 1), 3); // 21
static const long long H5_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H5_scf_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 2); // 6
static const long long H5_scf_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 8); // 192
static const long long H5_scf_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 6), 2), 16); // 576
static const long long H5_scf_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 5), 4); // 660
static const long long H5_scf_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 13), 4); // 52
static const long long H5_scf_m_Volume = tc_vmul(tc_vmul(1, 2), 3); // 6
static const long long H5_scf_t_Volume = tc_vmul(1, 2); // 2

#define H5_scf_USES(X) X(kd) X(pa) X(d1) X(pd) X(sb) X(ws) X(dm) X(oc) X(mu) X(gd) X(gr) X(pw) X(pv) X(sk)

struct H5_scf_T {
  cm_cnt2 H5_scf_A[tc_rows(H5_scf_a_Volume, H5_scf_hA) * H_scf_ND];
  cm_cnt2 H5_scf_B[tc_rows(H5_scf_b_Volume, H5_scf_hB) * H_scf_ND];
  cm_cnt2 H5_scf_C[tc_rows(H5_scf_c_Volume, H5_scf_hC) * H_scf_ND];
  cm_cnt2 H5_scf_D[tc_rows(H5_scf_d_Volume, H5_scf_hD) * H_scf_ND];
  cm_cnt2 H5_scf_E[tc_rows(H5_scf_e_Volume, H5_scf_hE) * H_scf_ND];
  cm_cnt2 H5_scf_H[tc_rows(H5_scf_h_Volume, H5_scf_hH) * H_scf_ND];
  cm_cnt2 H5_scf_I[tc_rows(H5_scf_i_Volume, H5_scf_hI) * H_scf_ND];
  cm_cnt2 H5_scf_J[tc_rows(H5_scf_j_Volume, H5_scf_hJ) * H_scf_ND];
  cm_cnt2 H5_scf_K[tc_rows(H5_scf_k_Volume, H5_scf_hK) * H_scf_ND];
  cm_cnt2 H5_scf_L[tc_rows(H5_scf_l_Volume, H5_scf_hL) * H_scf_ND];
  cm_cnt H5_scf_MT[tc_rows(H5_scf_t_Volume, H5_scf_hT) * H_scf_MND];
  i32 H5_scf_W[tc_rows(H5_scf_m_Volume, H5_scf_hM) * H_scf_ND * tc_nin(10)];
  u8 H5_scf_WC[tc_rows(H5_scf_m_Volume, H5_scf_hM) * H_scf_ND];
  cm_cnt H5_scf_G[H5_scf_g_Volume * H_scf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_scf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_a_Volume, H5_scf_hA) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_b_Volume, H5_scf_hB) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_c_Volume, H5_scf_hC) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_d_Volume, H5_scf_hD) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_e_Volume, H5_scf_hE) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_h_Volume, H5_scf_hH) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_i_Volume, H5_scf_hI) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_j_Volume, H5_scf_hJ) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_k_Volume, H5_scf_hK) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_scf_l_Volume, H5_scf_hL) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_scf_t_Volume, H5_scf_hT) * H_scf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_scf_m_Volume, H5_scf_hM) * H_scf_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_scf_m_Volume, H5_scf_hM) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_scf_g_Volume * H_scf_NG);
  void H5_scf_Init( void ) {}
  void H5_scf_Quit( void ) {}
};
