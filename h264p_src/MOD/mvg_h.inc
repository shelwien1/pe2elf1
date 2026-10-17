
static const byte H_mvg_a_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_a_mag[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H_mvg_b_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvg_b_mag[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H_mvg_c_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_c_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mvg_d_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_d_mag[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H_mvg_e_ns[6+1]={ 0,1,2,3,4,5,6 };

static const byte H_mvg_f_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H_mvg_h_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_h_tq[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H_mvg_i_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_i_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvg_i_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_mvg_j_ox[3+1]={ 0,1,2,3 };
static const byte H_mvg_j_ns[6+1]={ 0,1,2,3,4,5,6 };

static const byte H_mvg_k_mag[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_mvg_k_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_mvg_l_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvg_l_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_mvg_m_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H_mvg_s_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mvg_s_mag[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H_mvg_w_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvg_w_ts[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const int H_mvg_rA = (235+0) * (1);
static const int H_mvg_rB = (205+0) * (1);
static const int H_mvg_rC = (3+0) * (1);
static const int H_mvg_rE = (64+0) * (1);
static const int H_mvg_rH = (64+0) * (1);
static const int H_mvg_rI = (64+0) * (1);
static const int H_mvg_rJ = (224+0) * (1);
static const int H_mvg_rK = (64+0) * (1);
static const int H_mvg_rL = (235+0) * (1);
static const int H_mvg_rD = (111+0) * (1);
static const int H_mvg_rT = (55+0) * (1);
static const int H_mvg_rG = (90+0) * (1);
static const int H_mvg_rS1 = (6+0) * (1);
static const int H_mvg_rS2 = (6+0) * (1);
static const int H_mvg_lr = (12+0) * (1);
static const int H_mvg_lrm = (12+0) * (1);
static const int H_mvg_mbm = (0+0) * (1);
static const int H_mvg_mwA = (29+0) * (1);
static const int H_mvg_mwB = (0+0) * (1);
static const int H_mvg_mwC = (0+0) * (1);
static const int H_mvg_mwE = (0+0) * (1);
static const int H_mvg_mwH = (32+0) * (1);
static const int H_mvg_mwI = (0+0) * (1);
static const int H_mvg_mwJ = (0+0) * (1);
static const int H_mvg_mwK = (0+0) * (1);
static const int H_mvg_mwL = (0+0) * (1);
static const int H_mvg_mwD = (31+0) * (1);
static const int H_mvg_mwT = (0+0) * (1);
static const int H_mvg_mwG = (0+0) * (1);
static const int H_mvg_bw = (14+0) * (1);
static const int H_mvg_qs = (0+3) * (1);
static const int H_mvg_qf = (122+3) * (1);
static const int H_mvg_mb = (160+0) * (1);
static const int H_mvg_hA = (24+0) * (1);
static const int H_mvg_hB = (24+0) * (1);
static const int H_mvg_hE = (24+0) * (1);
static const int H_mvg_hH = (24+0) * (1);
static const int H_mvg_hI = (24+0) * (1);
static const int H_mvg_hJ = (24+0) * (1);
static const int H_mvg_hK = (24+0) * (1);
static const int H_mvg_hL = (24+0) * (1);
static const int H_mvg_hD = (24+0) * (1);
static const int H_mvg_hS = (24+0) * (1);
static const int H_mvg_hF = (24+0) * (1);
static const int H_mvg_hM = (24+0) * (1);
static const int H_mvg_hT = (24+0) * (1);
static const int H_mvg_hN = (24+0) * (1);


static const long long H_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 10), 8); // 160
static const long long H_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 8), 2); // 144
static const long long H_mvg_c_Volume = tc_vmul(tc_vmul(1, 10), 9); // 90
static const long long H_mvg_d_Volume = tc_vmul(tc_vmul(1, 10), 8); // 80
static const long long H_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 2), 7); // 126
static const long long H_mvg_f_Volume = tc_vmul(tc_vmul(1, 2), 10); // 20
static const long long H_mvg_g_Volume = tc_vmul(1, 1); // 1
static const long long H_mvg_h_Volume = tc_vmul(tc_vmul(1, 10), 13); // 130
static const long long H_mvg_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 9), 10); // 900
static const long long H_mvg_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 4), 7); // 112
static const long long H_mvg_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 8), 10), 3); // 240
static const long long H_mvg_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 10), 3); // 270
static const long long H_mvg_m_Volume = tc_vmul(1, 10); // 10
static const long long H_mvg_n_Volume = tc_vmul(1, 1); // 1
static const long long H_mvg_s_Volume = tc_vmul(tc_vmul(1, 10), 8); // 80
static const long long H_mvg_t_Volume = tc_vmul(1, 1); // 1
static const long long H_mvg_w_Volume = tc_vmul(tc_vmul(1, 9), 10); // 90

struct H_mvg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H_mvg_A[tc_rows(H_mvg_a_Volume, H_mvg_hA) * H_mvg_ND];
  cm_cnt2 H_mvg_B[tc_rows(H_mvg_b_Volume, H_mvg_hB) * H_mvg_ND];
  cm_cnt2 H_mvg_C[H_mvg_c_Volume * H_mvg_ND];
  cm_cnt2 H_mvg_D[tc_rows(H_mvg_d_Volume, H_mvg_hD) * H_mvg_ND];
  cm_cnt2 H_mvg_E[tc_rows(H_mvg_e_Volume, H_mvg_hE) * H_mvg_ND];
  cm_cnt2 H_mvg_H[tc_rows(H_mvg_h_Volume, H_mvg_hH) * H_mvg_ND];
  cm_cnt2 H_mvg_I[tc_rows(H_mvg_i_Volume, H_mvg_hI) * H_mvg_ND];
  cm_cnt2 H_mvg_J[tc_rows(H_mvg_j_Volume, H_mvg_hJ) * H_mvg_ND];
  cm_cnt2 H_mvg_K[tc_rows(H_mvg_k_Volume, H_mvg_hK) * H_mvg_ND];
  cm_cnt2 H_mvg_L[tc_rows(H_mvg_l_Volume, H_mvg_hL) * H_mvg_ND];
  cm_cnt H_mvg_MT[tc_rows(H_mvg_t_Volume, H_mvg_hT) * H_mvg_MND];
  i16 H_mvg_S[tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * H_mvg_qs];
  u8 H_mvg_SC[tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * H_mvg_qs];
  i16 H_mvg_F[tc_rows(H_mvg_f_Volume, H_mvg_hF) * H_mvg_ND * H_mvg_qf];
  u8 H_mvg_FC[tc_rows(H_mvg_f_Volume, H_mvg_hF) * H_mvg_ND * H_mvg_qf];
  i32 H_mvg_W[tc_rows(H_mvg_m_Volume, H_mvg_hM) * H_mvg_ND * TC_MIXN];
  u8 H_mvg_WC[tc_rows(H_mvg_m_Volume, H_mvg_hM) * H_mvg_ND];
  i32 H_mvg_WM[tc_rows(H_mvg_n_Volume, H_mvg_hN) * H_mvg_MND * 3];
  u8 H_mvg_WMC[tc_rows(H_mvg_n_Volume, H_mvg_hN) * H_mvg_MND];
  i32 H_mvg_W2[H_mvg_w_Volume * H_mvg_ND * TC_MIXN];
  u8 H_mvg_W2C[H_mvg_w_Volume * H_mvg_ND];
  i32 H_mvg_W3[tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * TC_MIXN];
  u8 H_mvg_W3C[tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND];
  i32 H_mvg_WF[H_mvg_ND * 5 * 4];
  u8 H_mvg_WFC[H_mvg_ND * 5];
  cm_cnt H_mvg_G[H_mvg_g_Volume * H_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_a_Volume, H_mvg_hA) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_b_Volume, H_mvg_hB) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(H_mvg_c_Volume * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_d_Volume, H_mvg_hD) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_e_Volume, H_mvg_hE) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_h_Volume, H_mvg_hH) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_i_Volume, H_mvg_hI) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_j_Volume, H_mvg_hJ) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_k_Volume, H_mvg_hK) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_mvg_l_Volume, H_mvg_hL) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mvg_t_Volume, H_mvg_hT) * H_mvg_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * H_mvg_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * H_mvg_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mvg_f_Volume, H_mvg_hF) * H_mvg_ND * H_mvg_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvg_f_Volume, H_mvg_hF) * H_mvg_ND * H_mvg_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvg_m_Volume, H_mvg_hM) * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvg_m_Volume, H_mvg_hM) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvg_n_Volume, H_mvg_hN) * H_mvg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvg_n_Volume, H_mvg_hN) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mvg_w_Volume * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mvg_w_Volume * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvg_s_Volume, H_mvg_hS) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mvg_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mvg_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_mvg_g_Volume * H_mvg_NG);
  void H_mvg_Init( void ) {
  
  }
};
