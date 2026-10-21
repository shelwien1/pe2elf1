
static const byte H3_skp_a_c9[9+1]={ 0,1,1,2,3,4,5,6,7,7 };

static const byte H3_skp_b_cnt[15+1]={ 0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8 };

static const byte H3_skp_c_c9[9+1]={ 0,1,1,1,2,2,3,4,4,5 };
static const byte H3_skp_c_run[9+1]={ 0,0,0,1,2,2,2,2,2,3 };

static const byte H3_skp_d_k9[10+1]={ 0,1,2,2,3,3,4,4,5,6,6 };





static const byte H3_skp_j_qc[17+1]={ 0,1,2,3,3,3,3,3,4,5,6,7,7,7,7,7,7,7 };









static const int H3_skp_rA = (254+0) * (1);
static const int H3_skp_rB = (255+0) * (1);
static const int H3_skp_rC = (255+0) * (1);
static const int H3_skp_rE = (255+0) * (1);
static const int H3_skp_rH = (254+0) * (1);
static const int H3_skp_rI = (0+0) * (1);
static const int H3_skp_rJ = (253+0) * (1);
static const int H3_skp_rK = (64+0) * (1);
static const int H3_skp_rL = (246+0) * (1);
static const int H3_skp_rD = (255+0) * (1);
static const int H3_skp_rT = (0+0) * (1);
static const int H3_skp_rG = (0+0) * (1);
static const int H3_skp_lr = (14+0) * (1);
static const int H3_skp_lrm = (0+0) * (1);
static const int H3_skp_mbm = (0+0) * (1);
static const int H3_skp_mwA = (70+0) * (1);
static const int H3_skp_mwB = (16+0) * (1);
static const int H3_skp_mwC = (62+0) * (1);
static const int H3_skp_mwE = (23+0) * (1);
static const int H3_skp_mwH = (241+0) * (1);
static const int H3_skp_mwI = (141+0) * (1);
static const int H3_skp_mwJ = (4+0) * (1);
static const int H3_skp_mwK = (0+0) * (1);
static const int H3_skp_mwL = (70+0) * (1);
static const int H3_skp_mwD = (1+0) * (1);
static const int H3_skp_mwT = (0+0) * (1);
static const int H3_skp_mwG = (0+0) * (1);
static const int H3_skp_mt = (0+0) * (1);
static const int H3_skp_mb = (15+0) * (1);
static const int H3_skp_hA = (24+0) * (1);
static const int H3_skp_hB = (24+0) * (1);
static const int H3_skp_hC = (24+0) * (1);
static const int H3_skp_hE = (24+0) * (1);
static const int H3_skp_hH = (24+0) * (1);
static const int H3_skp_hI = (24+0) * (1);
static const int H3_skp_hJ = (24+0) * (1);
static const int H3_skp_hK = (24+0) * (1);
static const int H3_skp_hL = (24+0) * (1);
static const int H3_skp_hD = (24+0) * (1);
static const int H3_skp_hS = (24+0) * (1);
static const int H3_skp_hM = (24+0) * (1);
static const int H3_skp_hT = (24+0) * (1);
static const int H3_skp_hN = (24+0) * (1);


static const long long H3_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3), 2), 8), 2), 1), 1); // 192
static const long long H3_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 2), 3), 9), 3); // 972
static const long long H3_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 6), 4); // 216
static const long long H3_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3), 3), 7); // 126
static const long long H3_skp_e_Volume = tc_vmul(1, 3); // 3
static const long long H3_skp_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_skp_h_Volume = tc_vmul(1, 3); // 3
static const long long H3_skp_i_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long H3_skp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 8); // 72
static const long long H3_skp_k_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long H3_skp_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 2); // 4
static const long long H3_skp_m_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long H3_skp_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_skp_s_Volume = tc_vmul(1, 1); // 1
static const long long H3_skp_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_skp_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27

#define H3_skp_USES(X) X(a3) X(b3) X(col) X(c9) X(eq) X(st) X(cd) X(cnt) X(c1) X(run) X(k9) X(qc)

struct H3_skp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_skp_A[tc_rows(H3_skp_a_Volume, H3_skp_hA) * H_skp_ND];
  cm_cnt2 H3_skp_B[tc_rows(H3_skp_b_Volume, H3_skp_hB) * H_skp_ND];
  cm_cnt2 H3_skp_C[tc_rows(H3_skp_c_Volume, H3_skp_hC) * H_skp_ND];
  cm_cnt2 H3_skp_D[tc_rows(H3_skp_d_Volume, H3_skp_hD) * H_skp_ND];
  cm_cnt2 H3_skp_E[tc_rows(H3_skp_e_Volume, H3_skp_hE) * H_skp_ND];
  cm_cnt2 H3_skp_H[tc_rows(H3_skp_h_Volume, H3_skp_hH) * H_skp_ND];
  cm_cnt2 H3_skp_I[tc_rows(H3_skp_i_Volume, H3_skp_hI) * H_skp_ND];
  cm_cnt2 H3_skp_J[tc_rows(H3_skp_j_Volume, H3_skp_hJ) * H_skp_ND];
  cm_cnt2 H3_skp_K[tc_rows(H3_skp_k_Volume, H3_skp_hK) * H_skp_ND];
  cm_cnt2 H3_skp_L[tc_rows(H3_skp_l_Volume, H3_skp_hL) * H_skp_ND];
  cm_cnt H3_skp_MT[tc_rows(H3_skp_t_Volume, H3_skp_hT) * H_skp_MND];
  i32 H3_skp_W[tc_rows(H3_skp_m_Volume, H3_skp_hM) * H_skp_ND * 24];
  u8 H3_skp_WC[tc_rows(H3_skp_m_Volume, H3_skp_hM) * H_skp_ND];
  i32 H3_skp_WM[tc_rows(H3_skp_n_Volume, H3_skp_hN) * H_skp_MND * 3];
  u8 H3_skp_WMC[tc_rows(H3_skp_n_Volume, H3_skp_hN) * H_skp_MND];
  i32 H3_skp_W2[H3_skp_w_Volume * H_skp_ND * 24];
  u8 H3_skp_W2C[H3_skp_w_Volume * H_skp_ND];
  i32 H3_skp_W3[tc_rows(H3_skp_s_Volume, H3_skp_hS) * H_skp_ND * 24];
  u8 H3_skp_W3C[tc_rows(H3_skp_s_Volume, H3_skp_hS) * H_skp_ND];
  i32 H3_skp_WF[H_skp_ND * 5 * 4];
  u8 H3_skp_WFC[H_skp_ND * 5];
  cm_cnt H3_skp_G[H3_skp_g_Volume * H_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_a_Volume, H3_skp_hA) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_b_Volume, H3_skp_hB) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_c_Volume, H3_skp_hC) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_d_Volume, H3_skp_hD) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_e_Volume, H3_skp_hE) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_h_Volume, H3_skp_hH) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_i_Volume, H3_skp_hI) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_j_Volume, H3_skp_hJ) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_k_Volume, H3_skp_hK) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_skp_l_Volume, H3_skp_hL) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_skp_t_Volume, H3_skp_hT) * H_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_skp_m_Volume, H3_skp_hM) * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_skp_m_Volume, H3_skp_hM) * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_skp_n_Volume, H3_skp_hN) * H_skp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_skp_n_Volume, H3_skp_hN) * H_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_skp_w_Volume * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_skp_w_Volume * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_skp_s_Volume, H3_skp_hS) * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_skp_s_Volume, H3_skp_hS) * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_skp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_skp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_skp_g_Volume * H_skp_NG);
  void H3_skp_Init( void ) {
  
  }
  void H3_skp_Quit( void ) {}
};
