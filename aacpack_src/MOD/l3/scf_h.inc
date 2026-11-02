
static const byte H3_scf_a_oc[13+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,6,6 };

static const byte H3_scf_b_sb[50+1]={ 0,1,2,3,4,4,5,6,7,8,9,10,11,12,13,14,15,16,17,17,17,17,18,19,19,20,21,22,22,22,23,23,24,24,25,25,26,27,28,29,30,31,32,33,34,35,36,36,37,37,37 };

static const byte H3_scf_c_pa[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H3_scf_c_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H3_scf_d_pv[10+1]={ 0,1,2,3,4,5,6,7,8,8,9 };
static const byte H3_scf_d_sk[15+1]={ 0,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3 };

static const byte H3_scf_e_d1[13+1]={ 0,0,1,2,3,4,5,6,7,7,7,7,7,8 };
static const byte H3_scf_e_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H3_scf_g_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H3_scf_h_pa[12+1]={ 0,0,1,2,3,4,5,6,7,8,9,9,10 };
static const byte H3_scf_h_pd[5+1]={ 0,1,1,2,3,4 };




static const int H3_scf_rA = (255+0) * (1);
static const int H3_scf_rB = (253+0) * (1);
static const int H3_scf_rC = (255+0) * (1);
static const int H3_scf_rD = (255+0) * (1);
static const int H3_scf_rE = (253+0) * (1);
static const int H3_scf_rH = (255+0) * (1);
static const int H3_scf_rT = (140+0) * (1);
static const int H3_scf_rG = (122+0) * (1);
static const int H3_scf_lr = (25+0) * (1);
static const int H3_scf_mwA = (8+0) * (1);
static const int H3_scf_mwB = (0+0) * (1);
static const int H3_scf_mwC = (0+0) * (1);
static const int H3_scf_mwD = (30+0) * (1);
static const int H3_scf_mwE = (2+0) * (1);
static const int H3_scf_mwH = (27+0) * (1);
static const int H3_scf_mwT = (17+0) * (1);
static const int H3_scf_mwG = (8+0) * (1);
static const int H3_scf_mt = (0+0) * (1);
static const int H3_scf_mb = (10+0) * (1);
static const int H3_scf_hA = (24+0) * (1);
static const int H3_scf_hB = (24+0) * (1);
static const int H3_scf_hC = (24+0) * (1);
static const int H3_scf_hD = (24+0) * (1);
static const int H3_scf_hE = (24+0) * (1);
static const int H3_scf_hH = (24+0) * (1);
static const int H3_scf_hM = (24+0) * (1);
static const int H3_scf_hT = (24+0) * (1);


static const long long H3_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 1), 3); // 21
static const long long H3_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 38), 2); // 152
static const long long H3_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 6); // 270
static const long long H3_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 3), 4); // 360
static const long long H3_scf_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 9), 2), 61); // 1098
static const long long H3_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H3_scf_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 5); // 165
static const long long H3_scf_m_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long H3_scf_t_Volume = tc_vmul(1, 2); // 2

#define H3_scf_USES(X) X(oc) X(mu) X(kd) X(sb) X(ws) X(pa) X(d1) X(pv) X(sk) X(d2) X(dm) X(pd)

struct H3_scf_T {
  cm_cnt2 H3_scf_A[tc_rows(H3_scf_a_Volume, H3_scf_hA) * H_scf_ND];
  cm_cnt2 H3_scf_B[tc_rows(H3_scf_b_Volume, H3_scf_hB) * H_scf_ND];
  cm_cnt2 H3_scf_C[tc_rows(H3_scf_c_Volume, H3_scf_hC) * H_scf_ND];
  cm_cnt2 H3_scf_D[tc_rows(H3_scf_d_Volume, H3_scf_hD) * H_scf_ND];
  cm_cnt2 H3_scf_E[tc_rows(H3_scf_e_Volume, H3_scf_hE) * H_scf_ND];
  cm_cnt2 H3_scf_H[tc_rows(H3_scf_h_Volume, H3_scf_hH) * H_scf_ND];
  cm_cnt H3_scf_MT[tc_rows(H3_scf_t_Volume, H3_scf_hT) * H_scf_MND];
  i32 H3_scf_W[tc_rows(H3_scf_m_Volume, H3_scf_hM) * H_scf_ND * tc_nin(6)];
  u8 H3_scf_WC[tc_rows(H3_scf_m_Volume, H3_scf_hM) * H_scf_ND];
  cm_cnt H3_scf_G[H3_scf_g_Volume * H_scf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_scf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_a_Volume, H3_scf_hA) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_b_Volume, H3_scf_hB) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_c_Volume, H3_scf_hC) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_d_Volume, H3_scf_hD) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_e_Volume, H3_scf_hE) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_scf_h_Volume, H3_scf_hH) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_scf_t_Volume, H3_scf_hT) * H_scf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_scf_m_Volume, H3_scf_hM) * H_scf_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_scf_m_Volume, H3_scf_hM) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_scf_g_Volume * H_scf_NG);
  void H3_scf_Init( void ) {}
  void H3_scf_Quit( void ) {}
};
