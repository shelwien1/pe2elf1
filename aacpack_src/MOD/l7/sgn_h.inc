
static const byte H7_sgn_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H7_sgn_b_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H7_sgn_c_mg[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_sgn_d_mg[4+1]={ 0,1,2,3,4 };

static const byte H7_sgn_e_kq[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };


static const byte H7_sgn_h_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H7_sgn_h_mg[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_sgn_h_kq[6+1]={ 0,1,2,3,4,5,6 };


static const byte H7_sgn_j_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H7_sgn_k_mg[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H7_sgn_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H7_sgn_s_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };




static const int H7_sgn_rA = (254+0) * (1);
static const int H7_sgn_rB = (159+0) * (1);
static const int H7_sgn_rC = (255+0) * (1);
static const int H7_sgn_rD = (255+0) * (1);
static const int H7_sgn_rE = (255+0) * (1);
static const int H7_sgn_rH = (189+0) * (1);
static const int H7_sgn_rI = (234+0) * (1);
static const int H7_sgn_rJ = (254+0) * (1);
static const int H7_sgn_rK = (255+0) * (1);
static const int H7_sgn_rL = (254+0) * (1);
static const int H7_sgn_lr = (15+0) * (1);
static const int H7_sgn_mwA = (8+0) * (1);
static const int H7_sgn_mwB = (4+0) * (1);
static const int H7_sgn_mwC = (131+0) * (1);
static const int H7_sgn_mwD = (31+0) * (1);
static const int H7_sgn_mwE = (20+0) * (1);
static const int H7_sgn_mwH = (32+0) * (1);
static const int H7_sgn_mwI = (0+0) * (1);
static const int H7_sgn_mwJ = (14+0) * (1);
static const int H7_sgn_mwK = (29+0) * (1);
static const int H7_sgn_mwL = (2+0) * (1);
static const int H7_sgn_mb = (32+0) * (1);
static const int H7_sgn_rT = (0+0) * (1);
static const int H7_sgn_mwT = (0+0) * (1);
static const int H7_sgn_mt = (0+0) * (1);
static const int H7_sgn_rG = (0+0) * (1);
static const int H7_sgn_mwG = (0+0) * (1);
static const int H7_sgn_hA = (24+0) * (1);
static const int H7_sgn_hB = (24+0) * (1);
static const int H7_sgn_hC = (24+0) * (1);
static const int H7_sgn_hD = (24+0) * (1);
static const int H7_sgn_hE = (24+0) * (1);
static const int H7_sgn_hH = (24+0) * (1);
static const int H7_sgn_hI = (24+0) * (1);
static const int H7_sgn_hJ = (24+0) * (1);
static const int H7_sgn_hK = (24+0) * (1);
static const int H7_sgn_hL = (24+0) * (1);
static const int H7_sgn_hS = (24+0) * (1);
static const int H7_sgn_hM = (24+0) * (1);
static const int H7_sgn_hT = (24+0) * (1);


static const long long H7_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 3), 3); // 108
static const long long H7_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 12); // 108
static const long long H7_sgn_c_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long H7_sgn_d_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long H7_sgn_e_Volume = tc_vmul(1, 20); // 20
static const long long H7_sgn_g_Volume = 1; // 1
static const long long H7_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 8), 7); // 672
static const long long H7_sgn_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27
static const long long H7_sgn_j_Volume = tc_vmul(tc_vmul(1, 2), 12); // 24
static const long long H7_sgn_k_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long H7_sgn_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27
static const long long H7_sgn_m_Volume = tc_vmul(1, 12); // 12
static const long long H7_sgn_s_Volume = tc_vmul(1, 12); // 12
static const long long H7_sgn_t_Volume = 1; // 1
static const long long H7_sgn_w_Volume = tc_vmul(1, 3); // 3

#define H7_sgn_USES(X) X(cb) X(oc) X(ms) X(s1) X(s2) X(pf) X(mg) X(kq) X(ws)

struct H7_sgn_T {
  cm_cnt2 H7_sgn_A[tc_rows(H7_sgn_a_Volume, H7_sgn_hA) * H_sgn_ND];
  cm_cnt2 H7_sgn_B[tc_rows(H7_sgn_b_Volume, H7_sgn_hB) * H_sgn_ND];
  cm_cnt2 H7_sgn_C[tc_rows(H7_sgn_c_Volume, H7_sgn_hC) * H_sgn_ND];
  cm_cnt2 H7_sgn_D[tc_rows(H7_sgn_d_Volume, H7_sgn_hD) * H_sgn_ND];
  cm_cnt2 H7_sgn_E[tc_rows(H7_sgn_e_Volume, H7_sgn_hE) * H_sgn_ND];
  cm_cnt2 H7_sgn_H[tc_rows(H7_sgn_h_Volume, H7_sgn_hH) * H_sgn_ND];
  cm_cnt2 H7_sgn_I[tc_rows(H7_sgn_i_Volume, H7_sgn_hI) * H_sgn_ND];
  cm_cnt2 H7_sgn_J[tc_rows(H7_sgn_j_Volume, H7_sgn_hJ) * H_sgn_ND];
  cm_cnt2 H7_sgn_K[tc_rows(H7_sgn_k_Volume, H7_sgn_hK) * H_sgn_ND];
  cm_cnt2 H7_sgn_L[tc_rows(H7_sgn_l_Volume, H7_sgn_hL) * H_sgn_ND];
  cm_cnt H7_sgn_MT[tc_rows(H7_sgn_t_Volume, H7_sgn_hT) * H_sgn_MND];
  i32 H7_sgn_W[tc_rows(H7_sgn_m_Volume, H7_sgn_hM) * H_sgn_ND * tc_nin(10)];
  u8 H7_sgn_WC[tc_rows(H7_sgn_m_Volume, H7_sgn_hM) * H_sgn_ND];
  i32 H7_sgn_W2[H7_sgn_w_Volume * H_sgn_ND * tc_nin(10)];
  u8 H7_sgn_W2C[H7_sgn_w_Volume * H_sgn_ND];
  i32 H7_sgn_W3[tc_rows(H7_sgn_s_Volume, H7_sgn_hS) * H_sgn_ND * tc_nin(10)];
  u8 H7_sgn_W3C[tc_rows(H7_sgn_s_Volume, H7_sgn_hS) * H_sgn_ND];
  i32 H7_sgn_WF[H_sgn_ND * 4];
  u8 H7_sgn_WFC[H_sgn_ND];
  cm_cnt H7_sgn_G[H7_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_a_Volume, H7_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_b_Volume, H7_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_c_Volume, H7_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_d_Volume, H7_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_e_Volume, H7_sgn_hE) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_h_Volume, H7_sgn_hH) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_i_Volume, H7_sgn_hI) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_j_Volume, H7_sgn_hJ) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_k_Volume, H7_sgn_hK) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_sgn_l_Volume, H7_sgn_hL) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_sgn_t_Volume, H7_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_sgn_m_Volume, H7_sgn_hM) * H_sgn_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_sgn_m_Volume, H7_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_sgn_w_Volume * H_sgn_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_sgn_w_Volume * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_sgn_s_Volume, H7_sgn_hS) * H_sgn_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_sgn_s_Volume, H7_sgn_hS) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_sgn_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_sgn_g_Volume * H_sgn_NG);
  void H7_sgn_Init( void ) {}
  void H7_sgn_Quit( void ) {}
};
