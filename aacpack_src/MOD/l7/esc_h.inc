
static const byte H7_esc_a_kq[19+1]={ 0,1,2,2,3,3,4,5,6,7,8,8,9,9,10,10,10,10,11,11 };
static const byte H7_esc_a_a1[10+1]={ 0,1,1,2,2,3,3,4,5,6,7 };


static const byte H7_esc_c_oc[12+1]={ 0,1,2,3,3,3,4,4,4,5,5,6,7 };

static const byte H7_esc_d_la[12+1]={ 0,0,0,0,0,0,0,0,0,1,2,2,3 };

static const byte H7_esc_e_sb[50+1]={ 0,0,1,2,2,3,3,4,5,6,7,7,8,9,10,11,12,13,14,14,15,15,15,15,16,17,18,19,19,19,19,19,19,19,19,19,19,20,21,22,22,22,22,22,22,22,22,22,22,22,22 };


static const byte H7_esc_h_a1[12+1]={ 0,1,2,3,4,5,6,7,8,8,8,9,9 };
static const byte H7_esc_h_la[5+1]={ 0,1,2,3,4,5 };
static const byte H7_esc_h_xp[39+1]={ 0,0,0,1,2,2,2,2,2,2,2,2,2,3,4,5,6,7,8,9,10,10,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12 };

static const byte H7_esc_i_pf[12+1]={ 0,0,1,1,2,2,3,4,5,6,7,7,8 };
static const byte H7_esc_i_oc[5+1]={ 0,1,1,2,3,4 };

static const byte H7_esc_j_kq[19+1]={ 0,0,0,1,2,3,4,5,6,6,7,8,8,8,8,9,10,10,10,11 };
static const byte H7_esc_j_pf[5+1]={ 0,1,2,3,4,4 };




static const byte H7_esc_s_av[31+1]={ 0,1,2,3,3,3,3,4,4,5,5,6,7,7,7,8,9,10,11,12,13,13,13,13,13,13,13,13,14,14,14,14 };




static const int H7_esc_rA = (130+0) * (1);
static const int H7_esc_rB = (248+0) * (1);
static const int H7_esc_rC = (251+0) * (1);
static const int H7_esc_rD = (186+0) * (1);
static const int H7_esc_rE = (255+0) * (1);
static const int H7_esc_rH = (0+0) * (1);
static const int H7_esc_rI = (192+0) * (1);
static const int H7_esc_rJ = (254+0) * (1);
static const int H7_esc_rK = (254+0) * (1);
static const int H7_esc_rL = (253+0) * (1);
static const int H7_esc_rT = (128+0) * (1);
static const int H7_esc_rG = (64+0) * (1);
static const int H7_esc_lr = (29+0) * (1);
static const int H7_esc_mwA = (0+0) * (1);
static const int H7_esc_mwB = (1+0) * (1);
static const int H7_esc_mwC = (5+0) * (1);
static const int H7_esc_mwD = (0+0) * (1);
static const int H7_esc_mwE = (129+0) * (1);
static const int H7_esc_mwH = (0+0) * (1);
static const int H7_esc_mwI = (0+0) * (1);
static const int H7_esc_mwJ = (0+0) * (1);
static const int H7_esc_mwK = (3+0) * (1);
static const int H7_esc_mwL = (0+0) * (1);
static const int H7_esc_mwT = (111+0) * (1);
static const int H7_esc_mwG = (0+0) * (1);
static const int H7_esc_mt = (2+0) * (1);
static const int H7_esc_mb = (11+0) * (1);
static const int H7_esc_hA = (24+0) * (1);
static const int H7_esc_hB = (24+0) * (1);
static const int H7_esc_hC = (24+0) * (1);
static const int H7_esc_hD = (24+0) * (1);
static const int H7_esc_hE = (24+0) * (1);
static const int H7_esc_hH = (24+0) * (1);
static const int H7_esc_hI = (24+0) * (1);
static const int H7_esc_hJ = (24+0) * (1);
static const int H7_esc_hK = (24+0) * (1);
static const int H7_esc_hL = (24+0) * (1);
static const int H7_esc_hS = (24+0) * (1);
static const int H7_esc_hM = (24+0) * (1);
static const int H7_esc_hT = (24+0) * (1);


static const long long H7_esc_a_Volume = tc_vmul(tc_vmul(1, 12), 8); // 96
static const long long H7_esc_b_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H7_esc_c_Volume = tc_vmul(tc_vmul(1, 8), 2); // 16
static const long long H7_esc_d_Volume = tc_vmul(tc_vmul(1, 4), 3); // 12
static const long long H7_esc_e_Volume = tc_vmul(1, 23); // 23
static const long long H7_esc_g_Volume = 1; // 1
static const long long H7_esc_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 6), 13); // 780
static const long long H7_esc_i_Volume = tc_vmul(tc_vmul(1, 9), 5); // 45
static const long long H7_esc_j_Volume = tc_vmul(tc_vmul(1, 12), 5); // 60
static const long long H7_esc_k_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H7_esc_l_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long H7_esc_m_Volume = tc_vmul(1, 2); // 2
static const long long H7_esc_s_Volume = tc_vmul(tc_vmul(1, 1), 15); // 15
static const long long H7_esc_t_Volume = tc_vmul(1, 1); // 1
static const long long H7_esc_w_Volume = tc_vmul(1, 2); // 2

#define H7_esc_USES(X) X(kq) X(a1) X(pf) X(oc) X(ws) X(la) X(sb) X(xp) X(av)

struct H7_esc_T {
  cm_cnt2 H7_esc_A[tc_rows(H7_esc_a_Volume, H7_esc_hA) * H_esc_ND];
  cm_cnt2 H7_esc_B[tc_rows(H7_esc_b_Volume, H7_esc_hB) * H_esc_ND];
  cm_cnt2 H7_esc_C[tc_rows(H7_esc_c_Volume, H7_esc_hC) * H_esc_ND];
  cm_cnt2 H7_esc_D[tc_rows(H7_esc_d_Volume, H7_esc_hD) * H_esc_ND];
  cm_cnt2 H7_esc_E[tc_rows(H7_esc_e_Volume, H7_esc_hE) * H_esc_ND];
  cm_cnt2 H7_esc_H[tc_rows(H7_esc_h_Volume, H7_esc_hH) * H_esc_ND];
  cm_cnt2 H7_esc_I[tc_rows(H7_esc_i_Volume, H7_esc_hI) * H_esc_ND];
  cm_cnt2 H7_esc_J[tc_rows(H7_esc_j_Volume, H7_esc_hJ) * H_esc_ND];
  cm_cnt2 H7_esc_K[tc_rows(H7_esc_k_Volume, H7_esc_hK) * H_esc_ND];
  cm_cnt2 H7_esc_L[tc_rows(H7_esc_l_Volume, H7_esc_hL) * H_esc_ND];
  cm_cnt H7_esc_MT[tc_rows(H7_esc_t_Volume, H7_esc_hT) * H_esc_MND];
  i32 H7_esc_W[tc_rows(H7_esc_m_Volume, H7_esc_hM) * H_esc_ND * tc_nin(10)];
  u8 H7_esc_WC[tc_rows(H7_esc_m_Volume, H7_esc_hM) * H_esc_ND];
  i32 H7_esc_W2[H7_esc_w_Volume * H_esc_ND * tc_nin(10)];
  u8 H7_esc_W2C[H7_esc_w_Volume * H_esc_ND];
  i32 H7_esc_W3[tc_rows(H7_esc_s_Volume, H7_esc_hS) * H_esc_ND * tc_nin(10)];
  u8 H7_esc_W3C[tc_rows(H7_esc_s_Volume, H7_esc_hS) * H_esc_ND];
  i32 H7_esc_WF[H_esc_ND * 4];
  u8 H7_esc_WFC[H_esc_ND];
  cm_cnt H7_esc_G[H7_esc_g_Volume * H_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_a_Volume, H7_esc_hA) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_b_Volume, H7_esc_hB) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_c_Volume, H7_esc_hC) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_d_Volume, H7_esc_hD) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_e_Volume, H7_esc_hE) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_h_Volume, H7_esc_hH) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_i_Volume, H7_esc_hI) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_j_Volume, H7_esc_hJ) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_k_Volume, H7_esc_hK) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_esc_l_Volume, H7_esc_hL) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_esc_t_Volume, H7_esc_hT) * H_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_esc_m_Volume, H7_esc_hM) * H_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_esc_m_Volume, H7_esc_hM) * H_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_esc_w_Volume * H_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_esc_w_Volume * H_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_esc_s_Volume, H7_esc_hS) * H_esc_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_esc_s_Volume, H7_esc_hS) * H_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_esc_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_esc_g_Volume * H_esc_NG);
  void H7_esc_Init( void ) {}
  void H7_esc_Quit( void ) {}
};
