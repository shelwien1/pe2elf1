
static const byte H3_pcm_a_dp[6+1]={ 0,1,2,3,4,5,6 };
static const byte H3_pcm_a_pd[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_pcm_b_pd[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H3_pcm_b_pd2[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H3_pcm_d_dp[6+1]={ 0,1,2,3,4,5,6 };

static const byte H3_pcm_e_pd[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_pcm_e_pd2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_pcm_g_pd[3+1]={ 0,1,2,3 };

static const byte H3_pcm_h_dp[6+1]={ 0,1,2,3,4,5,6 };

static const byte H3_pcm_i_pd[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_pcm_j_dp[6+1]={ 0,1,2,3,4,5,6 };
static const byte H3_pcm_j_pd2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_pcm_k_pd2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_pcm_l_pd[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_pcm_l_dp[6+1]={ 0,1,2,3,4,5,6 };




static const byte H3_pcm_t_dp[6+1]={ 0,1,2,3,4,5,6 };

static const byte H3_pcm_w_dp[6+1]={ 0,1,2,3,4,5,6 };


static const int H3_pcm_rA = (64+0) * (1);
static const int H3_pcm_rB = (16+0) * (1);
static const int H3_pcm_rC = (64+0) * (1);
static const int H3_pcm_rE = (64+0) * (1);
static const int H3_pcm_rH = (64+0) * (1);
static const int H3_pcm_rI = (64+0) * (1);
static const int H3_pcm_rJ = (64+0) * (1);
static const int H3_pcm_rK = (64+0) * (1);
static const int H3_pcm_rL = (64+0) * (1);
static const int H3_pcm_rD = (255+0) * (1);
static const int H3_pcm_rT = (55+0) * (1);
static const int H3_pcm_rG = (90+0) * (1);
static const int H3_pcm_lr = (12+0) * (1);
static const int H3_pcm_lrm = (12+0) * (1);
static const int H3_pcm_mbm = (0+0) * (1);
static const int H3_pcm_mwA = (0+0) * (1);
static const int H3_pcm_mwB = (0+0) * (1);
static const int H3_pcm_mwC = (0+0) * (1);
static const int H3_pcm_mwE = (0+0) * (1);
static const int H3_pcm_mwH = (0+0) * (1);
static const int H3_pcm_mwI = (0+0) * (1);
static const int H3_pcm_mwJ = (0+0) * (1);
static const int H3_pcm_mwK = (0+0) * (1);
static const int H3_pcm_mwL = (0+0) * (1);
static const int H3_pcm_mwD = (0+0) * (1);
static const int H3_pcm_mwT = (0+0) * (1);
static const int H3_pcm_mwG = (0+0) * (1);
static const int H3_pcm_mt = (0+0) * (1);
static const int H3_pcm_mb = (5+0) * (1);
static const int H3_pcm_hA = (24+0) * (1);
static const int H3_pcm_hB = (24+0) * (1);
static const int H3_pcm_hC = (24+0) * (1);
static const int H3_pcm_hE = (24+0) * (1);
static const int H3_pcm_hH = (24+0) * (1);
static const int H3_pcm_hI = (24+0) * (1);
static const int H3_pcm_hJ = (24+0) * (1);
static const int H3_pcm_hK = (24+0) * (1);
static const int H3_pcm_hL = (24+0) * (1);
static const int H3_pcm_hD = (24+0) * (1);
static const int H3_pcm_hS = (24+0) * (1);
static const int H3_pcm_hM = (24+0) * (1);
static const int H3_pcm_hT = (24+0) * (1);
static const int H3_pcm_hN = (24+0) * (1);


static const long long H3_pcm_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 16); // 336
static const long long H3_pcm_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 8); // 192
static const long long H3_pcm_c_Volume = tc_vmul(1, 3); // 3
static const long long H3_pcm_d_Volume = tc_vmul(1, 7); // 7
static const long long H3_pcm_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 16), 8); // 384
static const long long H3_pcm_g_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long H3_pcm_h_Volume = tc_vmul(tc_vmul(1, 3), 7); // 21
static const long long H3_pcm_i_Volume = tc_vmul(tc_vmul(1, 3), 16); // 48
static const long long H3_pcm_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 8); // 168
static const long long H3_pcm_k_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long H3_pcm_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 16), 7); // 336
static const long long H3_pcm_m_Volume = tc_vmul(1, 3); // 3
static const long long H3_pcm_n_Volume = tc_vmul(1, 3); // 3
static const long long H3_pcm_s_Volume = tc_vmul(1, 3); // 3
static const long long H3_pcm_t_Volume = tc_vmul(tc_vmul(1, 3), 7); // 21
static const long long H3_pcm_w_Volume = tc_vmul(1, 7); // 7

#define H3_pcm_USES(X) X(pl) X(dp) X(pd) X(pd2)

struct H3_pcm_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_pcm_A[tc_rows(H3_pcm_a_Volume, H3_pcm_hA) * H_pcm_ND];
  cm_cnt2 H3_pcm_B[tc_rows(H3_pcm_b_Volume, H3_pcm_hB) * H_pcm_ND];
  cm_cnt2 H3_pcm_C[tc_rows(H3_pcm_c_Volume, H3_pcm_hC) * H_pcm_ND];
  cm_cnt2 H3_pcm_D[tc_rows(H3_pcm_d_Volume, H3_pcm_hD) * H_pcm_ND];
  cm_cnt2 H3_pcm_E[tc_rows(H3_pcm_e_Volume, H3_pcm_hE) * H_pcm_ND];
  cm_cnt2 H3_pcm_H[tc_rows(H3_pcm_h_Volume, H3_pcm_hH) * H_pcm_ND];
  cm_cnt2 H3_pcm_I[tc_rows(H3_pcm_i_Volume, H3_pcm_hI) * H_pcm_ND];
  cm_cnt2 H3_pcm_J[tc_rows(H3_pcm_j_Volume, H3_pcm_hJ) * H_pcm_ND];
  cm_cnt2 H3_pcm_K[tc_rows(H3_pcm_k_Volume, H3_pcm_hK) * H_pcm_ND];
  cm_cnt2 H3_pcm_L[tc_rows(H3_pcm_l_Volume, H3_pcm_hL) * H_pcm_ND];
  cm_cnt H3_pcm_MT[tc_rows(H3_pcm_t_Volume, H3_pcm_hT) * H_pcm_MND];
  i32 H3_pcm_W[tc_rows(H3_pcm_m_Volume, H3_pcm_hM) * H_pcm_ND * 24];
  u8 H3_pcm_WC[tc_rows(H3_pcm_m_Volume, H3_pcm_hM) * H_pcm_ND];
  i32 H3_pcm_WM[tc_rows(H3_pcm_n_Volume, H3_pcm_hN) * H_pcm_MND * 3];
  u8 H3_pcm_WMC[tc_rows(H3_pcm_n_Volume, H3_pcm_hN) * H_pcm_MND];
  i32 H3_pcm_W2[H3_pcm_w_Volume * H_pcm_ND * 24];
  u8 H3_pcm_W2C[H3_pcm_w_Volume * H_pcm_ND];
  i32 H3_pcm_W3[tc_rows(H3_pcm_s_Volume, H3_pcm_hS) * H_pcm_ND * 24];
  u8 H3_pcm_W3C[tc_rows(H3_pcm_s_Volume, H3_pcm_hS) * H_pcm_ND];
  i32 H3_pcm_WF[H_pcm_ND * 5 * 4];
  u8 H3_pcm_WFC[H_pcm_ND * 5];
  cm_cnt H3_pcm_G[H3_pcm_g_Volume * H_pcm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_pcm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_a_Volume, H3_pcm_hA) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_b_Volume, H3_pcm_hB) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_c_Volume, H3_pcm_hC) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_d_Volume, H3_pcm_hD) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_e_Volume, H3_pcm_hE) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_h_Volume, H3_pcm_hH) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_i_Volume, H3_pcm_hI) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_j_Volume, H3_pcm_hJ) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_k_Volume, H3_pcm_hK) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_pcm_l_Volume, H3_pcm_hL) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_pcm_t_Volume, H3_pcm_hT) * H_pcm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_pcm_m_Volume, H3_pcm_hM) * H_pcm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_pcm_m_Volume, H3_pcm_hM) * H_pcm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_pcm_n_Volume, H3_pcm_hN) * H_pcm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_pcm_n_Volume, H3_pcm_hN) * H_pcm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_pcm_w_Volume * H_pcm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_pcm_w_Volume * H_pcm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_pcm_s_Volume, H3_pcm_hS) * H_pcm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_pcm_s_Volume, H3_pcm_hS) * H_pcm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_pcm_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_pcm_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_pcm_g_Volume * H_pcm_NG);
  void H3_pcm_Init( void ) {
  
  }
  void H3_pcm_Quit( void ) {}
};
