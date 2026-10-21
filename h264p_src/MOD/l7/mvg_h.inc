

static const byte H7_mvg_b_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };
static const byte H7_mvg_b_mag[7+1]={ 0,0,0,0,1,1,2,3 };

static const byte H7_mvg_c_zs[8+1]={ 0,0,0,1,2,2,3,4,4 };

static const byte H7_mvg_d_ds[9+1]={ 0,0,0,1,2,3,4,5,6,7 };
static const byte H7_mvg_d_mag[7+1]={ 0,0,1,1,2,2,2,3 };

static const byte H7_mvg_e_ns[6+1]={ 0,0,1,2,3,4,4 };



static const byte H7_mvg_i_ds[9+1]={ 0,0,0,1,1,1,2,2,2,3 };



static const byte H7_mvg_l_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };
static const byte H7_mvg_l_ds[9+1]={ 0,0,0,1,2,3,4,5,6,7 };





static const byte H7_mvg_w_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };


static const int H7_mvg_rA = (128+0) * (1);
static const int H7_mvg_rB = (0+0) * (1);
static const int H7_mvg_rC = (64+0) * (1);
static const int H7_mvg_rE = (253+0) * (1);
static const int H7_mvg_rH = (65+0) * (1);
static const int H7_mvg_rI = (253+0) * (1);
static const int H7_mvg_rJ = (128+0) * (1);
static const int H7_mvg_rK = (255+0) * (1);
static const int H7_mvg_rL = (254+0) * (1);
static const int H7_mvg_rD = (255+0) * (1);
static const int H7_mvg_lr = (14+0) * (1);
static const int H7_mvg_mwA = (6+0) * (1);
static const int H7_mvg_mwB = (66+0) * (1);
static const int H7_mvg_mwC = (10+0) * (1);
static const int H7_mvg_mwE = (8+0) * (1);
static const int H7_mvg_mwH = (2+0) * (1);
static const int H7_mvg_mwI = (20+0) * (1);
static const int H7_mvg_mwJ = (15+0) * (1);
static const int H7_mvg_mwK = (145+0) * (1);
static const int H7_mvg_mwL = (64+0) * (1);
static const int H7_mvg_mwD = (8+0) * (1);
static const int H7_mvg_mb = (130+0) * (1);
static const int H7_mvg_rT = (0+0) * (1);
static const int H7_mvg_mwT = (0+0) * (1);
static const int H7_mvg_mt = (0+0) * (1);
static const int H7_mvg_lrm = (0+0) * (1);
static const int H7_mvg_mbm = (0+0) * (1);
static const int H7_mvg_rG = (0+0) * (1);
static const int H7_mvg_mwG = (0+0) * (1);
static const int H7_mvg_hA = (24+0) * (1);
static const int H7_mvg_hB = (24+0) * (1);
static const int H7_mvg_hC = (24+0) * (1);
static const int H7_mvg_hE = (24+0) * (1);
static const int H7_mvg_hH = (24+0) * (1);
static const int H7_mvg_hI = (24+0) * (1);
static const int H7_mvg_hJ = (24+0) * (1);
static const int H7_mvg_hK = (24+0) * (1);
static const int H7_mvg_hL = (24+0) * (1);
static const int H7_mvg_hD = (24+0) * (1);
static const int H7_mvg_hS = (24+0) * (1);
static const int H7_mvg_hM = (24+0) * (1);
static const int H7_mvg_hT = (24+0) * (1);
static const int H7_mvg_hN = (24+0) * (1);


static const long long H7_mvg_a_Volume = tc_vmul(tc_vmul(1, 2), 1); // 2
static const long long H7_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 4), 2); // 56
static const long long H7_mvg_c_Volume = tc_vmul(1, 5); // 5
static const long long H7_mvg_d_Volume = tc_vmul(tc_vmul(1, 8), 4); // 32
static const long long H7_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 2), 5); // 90
static const long long H7_mvg_g_Volume = tc_vmul(1, 1); // 1
static const long long H7_mvg_h_Volume = 1; // 1
static const long long H7_mvg_i_Volume = tc_vmul(tc_vmul(1, 1), 4); // 4
static const long long H7_mvg_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3), 3); // 18
static const long long H7_mvg_k_Volume = tc_vmul(tc_vmul(1, 3), 3); // 9
static const long long H7_mvg_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 8), 3); // 168
static const long long H7_mvg_m_Volume = 1; // 1
static const long long H7_mvg_n_Volume = tc_vmul(1, 1); // 1
static const long long H7_mvg_s_Volume = tc_vmul(1, 3); // 3
static const long long H7_mvg_t_Volume = tc_vmul(1, 1); // 1
static const long long H7_mvg_w_Volume = tc_vmul(1, 7); // 7

#define H7_mvg_USES(X) X(comp) X(zs) X(mag) X(ds) X(sa) X(sb) X(ns) X(lst) X(ox)

struct H7_mvg_T {
  cm_cnt2 H7_mvg_A[tc_rows(H7_mvg_a_Volume, H7_mvg_hA) * H_mvg_ND];
  cm_cnt2 H7_mvg_B[tc_rows(H7_mvg_b_Volume, H7_mvg_hB) * H_mvg_ND];
  cm_cnt2 H7_mvg_C[tc_rows(H7_mvg_c_Volume, H7_mvg_hC) * H_mvg_ND];
  cm_cnt2 H7_mvg_D[tc_rows(H7_mvg_d_Volume, H7_mvg_hD) * H_mvg_ND];
  cm_cnt2 H7_mvg_E[tc_rows(H7_mvg_e_Volume, H7_mvg_hE) * H_mvg_ND];
  cm_cnt2 H7_mvg_H[tc_rows(H7_mvg_h_Volume, H7_mvg_hH) * H_mvg_ND];
  cm_cnt2 H7_mvg_I[tc_rows(H7_mvg_i_Volume, H7_mvg_hI) * H_mvg_ND];
  cm_cnt2 H7_mvg_J[tc_rows(H7_mvg_j_Volume, H7_mvg_hJ) * H_mvg_ND];
  cm_cnt2 H7_mvg_K[tc_rows(H7_mvg_k_Volume, H7_mvg_hK) * H_mvg_ND];
  cm_cnt2 H7_mvg_L[tc_rows(H7_mvg_l_Volume, H7_mvg_hL) * H_mvg_ND];
  cm_cnt H7_mvg_MT[tc_rows(H7_mvg_t_Volume, H7_mvg_hT) * H_mvg_MND];
  i32 H7_mvg_W[tc_rows(H7_mvg_m_Volume, H7_mvg_hM) * H_mvg_ND * 24];
  u8 H7_mvg_WC[tc_rows(H7_mvg_m_Volume, H7_mvg_hM) * H_mvg_ND];
  i32 H7_mvg_WM[tc_rows(H7_mvg_n_Volume, H7_mvg_hN) * H_mvg_MND * 3];
  u8 H7_mvg_WMC[tc_rows(H7_mvg_n_Volume, H7_mvg_hN) * H_mvg_MND];
  i32 H7_mvg_W2[H7_mvg_w_Volume * H_mvg_ND * 24];
  u8 H7_mvg_W2C[H7_mvg_w_Volume * H_mvg_ND];
  i32 H7_mvg_W3[tc_rows(H7_mvg_s_Volume, H7_mvg_hS) * H_mvg_ND * 24];
  u8 H7_mvg_W3C[tc_rows(H7_mvg_s_Volume, H7_mvg_hS) * H_mvg_ND];
  i32 H7_mvg_WF[H_mvg_ND * 5 * 4];
  u8 H7_mvg_WFC[H_mvg_ND * 5];
  cm_cnt H7_mvg_G[H7_mvg_g_Volume * H_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_a_Volume, H7_mvg_hA) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_b_Volume, H7_mvg_hB) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_c_Volume, H7_mvg_hC) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_d_Volume, H7_mvg_hD) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_e_Volume, H7_mvg_hE) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_h_Volume, H7_mvg_hH) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_i_Volume, H7_mvg_hI) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_j_Volume, H7_mvg_hJ) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_k_Volume, H7_mvg_hK) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_mvg_l_Volume, H7_mvg_hL) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_mvg_t_Volume, H7_mvg_hT) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_mvg_m_Volume, H7_mvg_hM) * H_mvg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_mvg_m_Volume, H7_mvg_hM) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_mvg_n_Volume, H7_mvg_hN) * H_mvg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_mvg_n_Volume, H7_mvg_hN) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_mvg_w_Volume * H_mvg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_mvg_w_Volume * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_mvg_s_Volume, H7_mvg_hS) * H_mvg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_mvg_s_Volume, H7_mvg_hS) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mvg_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mvg_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_mvg_g_Volume * H_mvg_NG);
  void H7_mvg_Init( void ) {}
  void H7_mvg_Quit( void ) {}
};
