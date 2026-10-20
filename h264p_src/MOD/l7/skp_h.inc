
static const byte H7_skp_a_c9[9+1]={ 0,1,2,2,3,3,4,4,4,5 };

static const byte H7_skp_b_cnt[15+1]={ 0,0,0,0,0,0,0,0,1,2,2,2,3,4,4,5 };

static const byte H7_skp_c_c9[9+1]={ 0,1,2,2,3,4,5,6,7,8 };
static const byte H7_skp_c_en[14+1]={ 0,0,1,2,3,4,5,5,5,6,6,6,6,6,7 };

static const byte H7_skp_d_k9[10+1]={ 0,1,1,2,3,4,5,6,7,8,9 };

static const byte H7_skp_e_td[16+1]={ 0,1,2,2,3,3,3,3,3,3,4,4,4,5,5,5,6 };
static const byte H7_skp_e_en[14+1]={ 0,0,1,1,2,2,2,2,2,3,3,4,4,4,4 };
static const byte H7_skp_e_sm[16+1]={ 0,0,1,1,1,2,3,3,4,4,4,4,5,5,5,5,5 };
static const byte H7_skp_e_tx[14+1]={ 0,0,1,2,3,3,4,4,5,5,5,6,6,6,7 };


static const byte H7_skp_h_tm[16+1]={ 0,0,1,2,2,3,3,4,4,5,5,6,6,6,6,6,6 };
static const byte H7_skp_h_sm[16+1]={ 0,0,1,2,3,3,4,4,5,5,5,6,6,6,6,7,7 };
static const byte H7_skp_h_bd[16+1]={ 0,1,2,2,3,3,4,5,6,6,7,7,8,8,8,8,8 };

static const byte H7_skp_i_en[14+1]={ 0,0,1,1,2,2,3,3,3,3,3,4,4,4,4 };

static const byte H7_skp_j_qc[17+1]={ 0,0,0,1,2,3,4,5,6,7,7,7,7,8,8,9,9,9 };

static const byte H7_skp_k_td[16+1]={ 0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3 };
static const byte H7_skp_k_tm[16+1]={ 0,0,0,0,1,1,2,2,3,3,4,4,4,4,4,4,4 };

static const byte H7_skp_l_bd[16+1]={ 0,1,2,3,3,3,4,4,4,5,5,6,6,7,7,7,7 };



static const byte H7_skp_s_bd[16+1]={ 0,1,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4 };


static const byte H7_skp_w_td[16+1]={ 0,0,0,0,1,1,1,1,1,2,2,3,3,3,3,3,4 };


static const int H7_skp_rA = (255+0) * (1);
static const int H7_skp_rB = (254+0) * (1);
static const int H7_skp_rC = (254+0) * (1);
static const int H7_skp_rE = (254+0) * (1);
static const int H7_skp_rH = (253+0) * (1);
static const int H7_skp_rI = (255+0) * (1);
static const int H7_skp_rJ = (255+0) * (1);
static const int H7_skp_rK = (228+0) * (1);
static const int H7_skp_rL = (255+0) * (1);
static const int H7_skp_rD = (254+0) * (1);
static const int H7_skp_rT = (0+0) * (1);
static const int H7_skp_rG = (0+0) * (1);
static const int H7_skp_lr = (15+0) * (1);
static const int H7_skp_lrm = (0+0) * (1);
static const int H7_skp_mbm = (0+0) * (1);
static const int H7_skp_mwA = (128+0) * (1);
static const int H7_skp_mwB = (17+0) * (1);
static const int H7_skp_mwC = (1+0) * (1);
static const int H7_skp_mwE = (16+0) * (1);
static const int H7_skp_mwH = (7+0) * (1);
static const int H7_skp_mwI = (4+0) * (1);
static const int H7_skp_mwJ = (59+0) * (1);
static const int H7_skp_mwK = (30+0) * (1);
static const int H7_skp_mwL = (16+0) * (1);
static const int H7_skp_mwD = (3+0) * (1);
static const int H7_skp_mwT = (0+0) * (1);
static const int H7_skp_mwG = (0+0) * (1);
static const int H7_skp_mt = (0+0) * (1);
static const int H7_skp_mb = (24+0) * (1);
static const int H7_skp_hA = (24+0) * (1);
static const int H7_skp_hB = (24+0) * (1);
static const int H7_skp_hC = (24+0) * (1);
static const int H7_skp_hE = (24+0) * (1);
static const int H7_skp_hH = (24+0) * (1);
static const int H7_skp_hI = (24+0) * (1);
static const int H7_skp_hJ = (24+0) * (1);
static const int H7_skp_hK = (24+0) * (1);
static const int H7_skp_hL = (24+0) * (1);
static const int H7_skp_hD = (24+0) * (1);
static const int H7_skp_hS = (24+0) * (1);
static const int H7_skp_hM = (24+0) * (1);
static const int H7_skp_hT = (24+0) * (1);
static const int H7_skp_hN = (24+0) * (1);


static const long long H7_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 2), 2), 6), 2), 1), 1); // 96
static const long long H7_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3), 3), 6), 3); // 972
static const long long H7_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 9), 3), 8); // 1296
static const long long H7_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3), 2), 2), 10); // 240
static const long long H7_skp_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 5), 6), 8); // 3360
static const long long H7_skp_g_Volume = tc_vmul(1, 1); // 1
static const long long H7_skp_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 8), 9); // 1008
static const long long H7_skp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 5), 1); // 15
static const long long H7_skp_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 10); // 90
static const long long H7_skp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 5), 3); // 60
static const long long H7_skp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 1), 1); // 24
static const long long H7_skp_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long H7_skp_n_Volume = tc_vmul(1, 1); // 1
static const long long H7_skp_s_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long H7_skp_t_Volume = tc_vmul(1, 1); // 1
static const long long H7_skp_w_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3), 5); // 135

#define H7_skp_USES(X) X(a3) X(b3) X(col) X(c9) X(eq) X(st) X(cd) X(cnt) X(c1) X(run) X(en) X(td) X(k9) X(sm) X(tx) X(tm) X(bd) X(qc)

struct H7_skp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H7_skp_A[tc_rows(H7_skp_a_Volume, H7_skp_hA) * H_skp_ND];
  cm_cnt2 H7_skp_B[tc_rows(H7_skp_b_Volume, H7_skp_hB) * H_skp_ND];
  cm_cnt2 H7_skp_C[tc_rows(H7_skp_c_Volume, H7_skp_hC) * H_skp_ND];
  cm_cnt2 H7_skp_D[tc_rows(H7_skp_d_Volume, H7_skp_hD) * H_skp_ND];
  cm_cnt2 H7_skp_E[tc_rows(H7_skp_e_Volume, H7_skp_hE) * H_skp_ND];
  cm_cnt2 H7_skp_H[tc_rows(H7_skp_h_Volume, H7_skp_hH) * H_skp_ND];
  cm_cnt2 H7_skp_I[tc_rows(H7_skp_i_Volume, H7_skp_hI) * H_skp_ND];
  cm_cnt2 H7_skp_J[tc_rows(H7_skp_j_Volume, H7_skp_hJ) * H_skp_ND];
  cm_cnt2 H7_skp_K[tc_rows(H7_skp_k_Volume, H7_skp_hK) * H_skp_ND];
  cm_cnt2 H7_skp_L[tc_rows(H7_skp_l_Volume, H7_skp_hL) * H_skp_ND];
  cm_cnt H7_skp_MT[tc_rows(H7_skp_t_Volume, H7_skp_hT) * H_skp_MND];
  i32 H7_skp_W[tc_rows(H7_skp_m_Volume, H7_skp_hM) * H_skp_ND * 24];
  u8 H7_skp_WC[tc_rows(H7_skp_m_Volume, H7_skp_hM) * H_skp_ND];
  i32 H7_skp_WM[tc_rows(H7_skp_n_Volume, H7_skp_hN) * H_skp_MND * 3];
  u8 H7_skp_WMC[tc_rows(H7_skp_n_Volume, H7_skp_hN) * H_skp_MND];
  i32 H7_skp_W2[H7_skp_w_Volume * H_skp_ND * 24];
  u8 H7_skp_W2C[H7_skp_w_Volume * H_skp_ND];
  i32 H7_skp_W3[tc_rows(H7_skp_s_Volume, H7_skp_hS) * H_skp_ND * 24];
  u8 H7_skp_W3C[tc_rows(H7_skp_s_Volume, H7_skp_hS) * H_skp_ND];
  i32 H7_skp_WF[H_skp_ND * 5 * 4];
  u8 H7_skp_WFC[H_skp_ND * 5];
  cm_cnt H7_skp_G[H7_skp_g_Volume * H_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_a_Volume, H7_skp_hA) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_b_Volume, H7_skp_hB) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_c_Volume, H7_skp_hC) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_d_Volume, H7_skp_hD) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_e_Volume, H7_skp_hE) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_h_Volume, H7_skp_hH) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_i_Volume, H7_skp_hI) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_j_Volume, H7_skp_hJ) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_k_Volume, H7_skp_hK) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_skp_l_Volume, H7_skp_hL) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_skp_t_Volume, H7_skp_hT) * H_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_skp_m_Volume, H7_skp_hM) * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_skp_m_Volume, H7_skp_hM) * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_skp_n_Volume, H7_skp_hN) * H_skp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_skp_n_Volume, H7_skp_hN) * H_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_skp_w_Volume * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_skp_w_Volume * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_skp_s_Volume, H7_skp_hS) * H_skp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_skp_s_Volume, H7_skp_hS) * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_skp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_skp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_skp_g_Volume * H_skp_NG);
  void H7_skp_Init( void ) {
  
  }
};
