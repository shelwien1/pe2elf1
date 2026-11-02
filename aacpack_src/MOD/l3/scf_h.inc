
static const byte H3_scf_a_oc[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_scf_a_pa[5+1]={ 0,1,2,3,4,5 };

static const byte H3_scf_b_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50 };

static const byte H3_scf_c_pa[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H3_scf_c_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H3_scf_d_pv[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H3_scf_d_sb[5+1]={ 0,1,2,3,4,5 };
static const byte H3_scf_d_sk[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_scf_e_d1[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_scf_e_d2[5+1]={ 0,1,2,3,4,5 };
static const byte H3_scf_e_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H3_scf_g_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H3_scf_h_pa[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H3_scf_h_pd[5+1]={ 0,1,2,3,4,5 };




static const int H3_scf_rA = (255+0) * (1);
static const int H3_scf_rB = (255+0) * (1);
static const int H3_scf_rC = (130+0) * (1);
static const int H3_scf_rD = (255+0) * (1);
static const int H3_scf_rE = (255+0) * (1);
static const int H3_scf_rH = (253+0) * (1);
static const int H3_scf_rT = (128+0) * (1);
static const int H3_scf_rG = (64+0) * (1);
static const int H3_scf_lr = (15+0) * (1);
static const int H3_scf_mwA = (16+0) * (1);
static const int H3_scf_mwB = (4+0) * (1);
static const int H3_scf_mwC = (128+0) * (1);
static const int H3_scf_mwD = (51+0) * (1);
static const int H3_scf_mwE = (27+0) * (1);
static const int H3_scf_mwH = (62+0) * (1);
static const int H3_scf_mwT = (0+0) * (1);
static const int H3_scf_mwG = (0+0) * (1);
static const int H3_scf_mt = (2+0) * (1);
static const int H3_scf_mb = (8+0) * (1);
static const int H3_scf_hA = (24+0) * (1);
static const int H3_scf_hB = (24+0) * (1);
static const int H3_scf_hC = (24+0) * (1);
static const int H3_scf_hD = (24+0) * (1);
static const int H3_scf_hE = (24+0) * (1);
static const int H3_scf_hH = (24+0) * (1);
static const int H3_scf_hM = (24+0) * (1);
static const int H3_scf_hT = (24+0) * (1);


static const long long H3_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 14), 6), 3); // 756
static const long long H3_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 51), 2); // 306
static const long long H3_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 6); // 270
static const long long H3_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 6), 16); // 3168
static const long long H3_scf_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 14), 6), 61); // 15372
static const long long H3_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H3_scf_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 13), 6); // 234
static const long long H3_scf_m_Volume = tc_vmul(tc_vmul(1, 3), 3); // 9
static const long long H3_scf_t_Volume = tc_vmul(1, 3); // 3

#define H3_scf_USES(X) X(kd) X(oc) X(pa) X(mu) X(sb) X(ws) X(d1) X(pv) X(sk) X(d2) X(dm) X(pd)

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
