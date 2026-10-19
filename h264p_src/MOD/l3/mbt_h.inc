
static const byte H3_mbt_a_ca[8+1]={ 0,1,2,3,3,4,5,6,6 };
static const byte H3_mbt_a_cb[8+1]={ 0,1,2,3,3,4,5,6,6 };

static const byte H3_mbt_b_cc[8+1]={ 0,1,2,3,3,4,5,6,6 };
static const byte H3_mbt_b_c1[9+1]={ 0,1,1,2,2,3,3,4,5,5 };

static const byte H3_mbt_c_ca[8+1]={ 0,1,1,2,3,4,5,6,6 };
static const byte H3_mbt_c_cc[8+1]={ 0,0,0,1,2,2,3,4,4 };


static const byte H3_mbt_e_ca[8+1]={ 0,1,1,1,1,1,2,3,3 };




static const byte H3_mbt_j_cb[8+1]={ 0,1,2,2,3,3,4,5,5 };

static const byte H3_mbt_k_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };
static const byte H3_mbt_k_nsk[4+1]={ 0,1,2,3,4 };
static const byte H3_mbt_k_cc[8+1]={ 0,0,1,1,2,2,3,4,4 };

static const byte H3_mbt_l_ca[8+1]={ 0,1,2,3,4,4,5,6,6 };
static const byte H3_mbt_l_cb[8+1]={ 0,1,2,3,4,4,5,6,6 };

static const byte H3_mbt_m_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };






static const int H3_mbt_rA = (253+0) * (1);
static const int H3_mbt_rB = (250+0) * (1);
static const int H3_mbt_rC = (251+0) * (1);
static const int H3_mbt_rE = (134+0) * (1);
static const int H3_mbt_rH = (186+0) * (1);
static const int H3_mbt_rI = (0+0) * (1);
static const int H3_mbt_rJ = (254+0) * (1);
static const int H3_mbt_rK = (160+0) * (1);
static const int H3_mbt_rL = (254+0) * (1);
static const int H3_mbt_rD = (119+0) * (1);
static const int H3_mbt_rT = (0+0) * (1);
static const int H3_mbt_rG = (0+0) * (1);
static const int H3_mbt_lr = (8+0) * (1);
static const int H3_mbt_lrm = (0+0) * (1);
static const int H3_mbt_mbm = (0+0) * (1);
static const int H3_mbt_mwA = (33+0) * (1);
static const int H3_mbt_mwB = (7+0) * (1);
static const int H3_mbt_mwC = (58+0) * (1);
static const int H3_mbt_mwE = (93+0) * (1);
static const int H3_mbt_mwH = (0+0) * (1);
static const int H3_mbt_mwI = (33+0) * (1);
static const int H3_mbt_mwJ = (128+0) * (1);
static const int H3_mbt_mwK = (2+0) * (1);
static const int H3_mbt_mwL = (0+0) * (1);
static const int H3_mbt_mwD = (71+0) * (1);
static const int H3_mbt_mwT = (0+0) * (1);
static const int H3_mbt_mwG = (0+0) * (1);
static const int H3_mbt_mt = (0+0) * (1);
static const int H3_mbt_mb = (64+0) * (1);
static const int H3_mbt_hA = (24+0) * (1);
static const int H3_mbt_hB = (24+0) * (1);
static const int H3_mbt_hC = (24+0) * (1);
static const int H3_mbt_hE = (15+0) * (1);
static const int H3_mbt_hH = (24+0) * (1);
static const int H3_mbt_hI = (24+0) * (1);
static const int H3_mbt_hJ = (24+0) * (1);
static const int H3_mbt_hK = (24+0) * (1);
static const int H3_mbt_hL = (24+0) * (1);
static const int H3_mbt_hD = (24+0) * (1);
static const int H3_mbt_hS = (24+0) * (1);
static const int H3_mbt_hM = (24+0) * (1);
static const int H3_mbt_hT = (24+0) * (1);
static const int H3_mbt_hN = (24+0) * (1);


static const long long H3_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 7); // 49
static const long long H3_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 1), 6); // 84
static const long long H3_mbt_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 5); // 105
static const long long H3_mbt_d_Volume = tc_vmul(1, 2); // 2
static const long long H3_mbt_e_Volume = tc_vmul(tc_vmul(1, 1), 4); // 4
static const long long H3_mbt_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_mbt_h_Volume = tc_vmul(tc_vmul(1, 2), 1); // 2
static const long long H3_mbt_i_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long H3_mbt_j_Volume = tc_vmul(tc_vmul(1, 1), 6); // 6
static const long long H3_mbt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 5); // 100
static const long long H3_mbt_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 7); // 147
static const long long H3_mbt_m_Volume = tc_vmul(1, 4); // 4
static const long long H3_mbt_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_mbt_s_Volume = tc_vmul(1, 2); // 2
static const long long H3_mbt_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_mbt_w_Volume = tc_vmul(1, 1); // 1

#define H3_mbt_USES(X) X(ca) X(cb) X(st) X(cc) X(c1) X(nsk)

struct H3_mbt_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_mbt_A[tc_rows(H3_mbt_a_Volume, H3_mbt_hA) * H_mbt_ND];
  cm_cnt2 H3_mbt_B[tc_rows(H3_mbt_b_Volume, H3_mbt_hB) * H_mbt_ND];
  cm_cnt2 H3_mbt_C[tc_rows(H3_mbt_c_Volume, H3_mbt_hC) * H_mbt_ND];
  cm_cnt2 H3_mbt_D[tc_rows(H3_mbt_d_Volume, H3_mbt_hD) * H_mbt_ND];
  cm_cnt2 H3_mbt_E[tc_rows(H3_mbt_e_Volume, H3_mbt_hE) * H_mbt_ND];
  cm_cnt2 H3_mbt_H[tc_rows(H3_mbt_h_Volume, H3_mbt_hH) * H_mbt_ND];
  cm_cnt2 H3_mbt_I[tc_rows(H3_mbt_i_Volume, H3_mbt_hI) * H_mbt_ND];
  cm_cnt2 H3_mbt_J[tc_rows(H3_mbt_j_Volume, H3_mbt_hJ) * H_mbt_ND];
  cm_cnt2 H3_mbt_K[tc_rows(H3_mbt_k_Volume, H3_mbt_hK) * H_mbt_ND];
  cm_cnt2 H3_mbt_L[tc_rows(H3_mbt_l_Volume, H3_mbt_hL) * H_mbt_ND];
  cm_cnt H3_mbt_MT[tc_rows(H3_mbt_t_Volume, H3_mbt_hT) * H_mbt_MND];
  i32 H3_mbt_W[tc_rows(H3_mbt_m_Volume, H3_mbt_hM) * H_mbt_ND * 24];
  u8 H3_mbt_WC[tc_rows(H3_mbt_m_Volume, H3_mbt_hM) * H_mbt_ND];
  i32 H3_mbt_WM[tc_rows(H3_mbt_n_Volume, H3_mbt_hN) * H_mbt_MND * 3];
  u8 H3_mbt_WMC[tc_rows(H3_mbt_n_Volume, H3_mbt_hN) * H_mbt_MND];
  i32 H3_mbt_W2[H3_mbt_w_Volume * H_mbt_ND * 24];
  u8 H3_mbt_W2C[H3_mbt_w_Volume * H_mbt_ND];
  i32 H3_mbt_W3[tc_rows(H3_mbt_s_Volume, H3_mbt_hS) * H_mbt_ND * 24];
  u8 H3_mbt_W3C[tc_rows(H3_mbt_s_Volume, H3_mbt_hS) * H_mbt_ND];
  i32 H3_mbt_WF[H_mbt_ND * 5 * 4];
  u8 H3_mbt_WFC[H_mbt_ND * 5];
  cm_cnt H3_mbt_G[H3_mbt_g_Volume * H_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_a_Volume, H3_mbt_hA) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_b_Volume, H3_mbt_hB) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_c_Volume, H3_mbt_hC) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_d_Volume, H3_mbt_hD) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_e_Volume, H3_mbt_hE) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_h_Volume, H3_mbt_hH) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_i_Volume, H3_mbt_hI) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_j_Volume, H3_mbt_hJ) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_k_Volume, H3_mbt_hK) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_mbt_l_Volume, H3_mbt_hL) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_mbt_t_Volume, H3_mbt_hT) * H_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_mbt_m_Volume, H3_mbt_hM) * H_mbt_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_mbt_m_Volume, H3_mbt_hM) * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_mbt_n_Volume, H3_mbt_hN) * H_mbt_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_mbt_n_Volume, H3_mbt_hN) * H_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_mbt_w_Volume * H_mbt_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_mbt_w_Volume * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_mbt_s_Volume, H3_mbt_hS) * H_mbt_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_mbt_s_Volume, H3_mbt_hS) * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mbt_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mbt_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_mbt_g_Volume * H_mbt_NG);
  void H3_mbt_Init( void ) {
  
  }
};
