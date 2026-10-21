
static const byte H3_ipm_a_ma[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H3_ipm_a_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H3_ipm_a_mc[10+1]={ 0,1,2,3,4,4,5,6,7,8,9 };

static const byte H3_ipm_b_mp[9+1]={ 0,1,2,2,3,3,4,5,6,6 };
static const byte H3_ipm_b_pv[9+1]={ 0,1,2,3,4,5,6,7,8,8 };
static const byte H3_ipm_b_mh[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15 };

static const byte H3_ipm_c_mp[9+1]={ 0,1,2,3,4,4,5,6,7,7 };
static const byte H3_ipm_c_col[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H3_ipm_d_mp[9+1]={ 0,1,2,3,3,3,3,3,3,3 };

static const byte H3_ipm_e_mp[9+1]={ 0,1,2,3,4,4,5,6,7,7 };



static const byte H3_ipm_i_ma[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte H3_ipm_j_mh[16+1]={ 0,1,2,3,4,4,5,6,7,7,8,9,10,11,12,13,14 };
static const byte H3_ipm_j_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte H3_ipm_k_pv[9+1]={ 0,0,0,1,2,3,4,5,5,6 };

static const byte H3_ipm_l_mc[10+1]={ 0,1,1,2,3,3,3,4,5,6,7 };
static const byte H3_ipm_l_ma[10+1]={ 0,0,0,1,2,2,2,3,4,5,6 };

static const byte H3_ipm_m_mb[10+1]={ 0,0,0,1,2,2,3,4,5,6,7 };




static const byte H3_ipm_w_mp[9+1]={ 0,1,2,3,4,5,5,6,7,7 };


static const int H3_ipm_rA = (255+0) * (1);
static const int H3_ipm_rB = (254+0) * (1);
static const int H3_ipm_rC = (255+0) * (1);
static const int H3_ipm_rE = (89+0) * (1);
static const int H3_ipm_rH = (78+0) * (1);
static const int H3_ipm_rI = (253+0) * (1);
static const int H3_ipm_rJ = (255+0) * (1);
static const int H3_ipm_rK = (255+0) * (1);
static const int H3_ipm_rL = (252+0) * (1);
static const int H3_ipm_rD = (253+0) * (1);
static const int H3_ipm_rT = (0+0) * (1);
static const int H3_ipm_rG = (0+0) * (1);
static const int H3_ipm_lr = (15+0) * (1);
static const int H3_ipm_lrm = (0+0) * (1);
static const int H3_ipm_mbm = (0+0) * (1);
static const int H3_ipm_mwA = (22+0) * (1);
static const int H3_ipm_mwB = (148+0) * (1);
static const int H3_ipm_mwC = (8+0) * (1);
static const int H3_ipm_mwE = (0+0) * (1);
static const int H3_ipm_mwH = (8+0) * (1);
static const int H3_ipm_mwI = (58+0) * (1);
static const int H3_ipm_mwJ = (34+0) * (1);
static const int H3_ipm_mwK = (128+0) * (1);
static const int H3_ipm_mwL = (64+0) * (1);
static const int H3_ipm_mwD = (134+0) * (1);
static const int H3_ipm_mwT = (0+0) * (1);
static const int H3_ipm_mwG = (0+0) * (1);
static const int H3_ipm_mt = (0+0) * (1);
static const int H3_ipm_mb = (40+0) * (1);
static const int H3_ipm_hA = (24+0) * (1);
static const int H3_ipm_hB = (24+0) * (1);
static const int H3_ipm_hC = (24+0) * (1);
static const int H3_ipm_hE = (24+0) * (1);
static const int H3_ipm_hH = (24+0) * (1);
static const int H3_ipm_hI = (24+0) * (1);
static const int H3_ipm_hJ = (24+0) * (1);
static const int H3_ipm_hK = (24+0) * (1);
static const int H3_ipm_hL = (24+0) * (1);
static const int H3_ipm_hD = (24+0) * (1);
static const int H3_ipm_hS = (24+0) * (1);
static const int H3_ipm_hM = (24+0) * (1);
static const int H3_ipm_hT = (24+0) * (1);
static const int H3_ipm_hN = (24+0) * (1);


static const long long H3_ipm_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 11), 10); // 2420
static const long long H3_ipm_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 9), 16); // 2016
static const long long H3_ipm_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 10); // 80
static const long long H3_ipm_d_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long H3_ipm_e_Volume = tc_vmul(tc_vmul(1, 1), 8); // 8
static const long long H3_ipm_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_ipm_h_Volume = tc_vmul(1, 1); // 1
static const long long H3_ipm_i_Volume = tc_vmul(tc_vmul(1, 1), 11); // 11
static const long long H3_ipm_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 15), 11); // 330
static const long long H3_ipm_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 7); // 28
static const long long H3_ipm_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 7); // 56
static const long long H3_ipm_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 8); // 32
static const long long H3_ipm_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_ipm_s_Volume = tc_vmul(1, 1); // 1
static const long long H3_ipm_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_ipm_w_Volume = tc_vmul(tc_vmul(1, 2), 8); // 16

#define H3_ipm_USES(X) X(k8) X(ma) X(mb) X(mc) X(mp) X(pv) X(mh) X(col)

struct H3_ipm_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_ipm_A[tc_rows(H3_ipm_a_Volume, H3_ipm_hA) * H_ipm_ND];
  cm_cnt2 H3_ipm_B[tc_rows(H3_ipm_b_Volume, H3_ipm_hB) * H_ipm_ND];
  cm_cnt2 H3_ipm_C[tc_rows(H3_ipm_c_Volume, H3_ipm_hC) * H_ipm_ND];
  cm_cnt2 H3_ipm_D[tc_rows(H3_ipm_d_Volume, H3_ipm_hD) * H_ipm_ND];
  cm_cnt2 H3_ipm_E[tc_rows(H3_ipm_e_Volume, H3_ipm_hE) * H_ipm_ND];
  cm_cnt2 H3_ipm_H[tc_rows(H3_ipm_h_Volume, H3_ipm_hH) * H_ipm_ND];
  cm_cnt2 H3_ipm_I[tc_rows(H3_ipm_i_Volume, H3_ipm_hI) * H_ipm_ND];
  cm_cnt2 H3_ipm_J[tc_rows(H3_ipm_j_Volume, H3_ipm_hJ) * H_ipm_ND];
  cm_cnt2 H3_ipm_K[tc_rows(H3_ipm_k_Volume, H3_ipm_hK) * H_ipm_ND];
  cm_cnt2 H3_ipm_L[tc_rows(H3_ipm_l_Volume, H3_ipm_hL) * H_ipm_ND];
  cm_cnt H3_ipm_MT[tc_rows(H3_ipm_t_Volume, H3_ipm_hT) * H_ipm_MND];
  i32 H3_ipm_W[tc_rows(H3_ipm_m_Volume, H3_ipm_hM) * H_ipm_ND * 24];
  u8 H3_ipm_WC[tc_rows(H3_ipm_m_Volume, H3_ipm_hM) * H_ipm_ND];
  i32 H3_ipm_WM[tc_rows(H3_ipm_n_Volume, H3_ipm_hN) * H_ipm_MND * 3];
  u8 H3_ipm_WMC[tc_rows(H3_ipm_n_Volume, H3_ipm_hN) * H_ipm_MND];
  i32 H3_ipm_W2[H3_ipm_w_Volume * H_ipm_ND * 24];
  u8 H3_ipm_W2C[H3_ipm_w_Volume * H_ipm_ND];
  i32 H3_ipm_W3[tc_rows(H3_ipm_s_Volume, H3_ipm_hS) * H_ipm_ND * 24];
  u8 H3_ipm_W3C[tc_rows(H3_ipm_s_Volume, H3_ipm_hS) * H_ipm_ND];
  i32 H3_ipm_WF[H_ipm_ND * 5 * 4];
  u8 H3_ipm_WFC[H_ipm_ND * 5];
  cm_cnt H3_ipm_G[H3_ipm_g_Volume * H_ipm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_ipm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_a_Volume, H3_ipm_hA) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_b_Volume, H3_ipm_hB) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_c_Volume, H3_ipm_hC) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_d_Volume, H3_ipm_hD) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_e_Volume, H3_ipm_hE) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_h_Volume, H3_ipm_hH) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_i_Volume, H3_ipm_hI) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_j_Volume, H3_ipm_hJ) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_k_Volume, H3_ipm_hK) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_ipm_l_Volume, H3_ipm_hL) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_ipm_t_Volume, H3_ipm_hT) * H_ipm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_ipm_m_Volume, H3_ipm_hM) * H_ipm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_ipm_m_Volume, H3_ipm_hM) * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_ipm_n_Volume, H3_ipm_hN) * H_ipm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_ipm_n_Volume, H3_ipm_hN) * H_ipm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_ipm_w_Volume * H_ipm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_ipm_w_Volume * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_ipm_s_Volume, H3_ipm_hS) * H_ipm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_ipm_s_Volume, H3_ipm_hS) * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_ipm_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_ipm_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_ipm_g_Volume * H_ipm_NG);
  void H3_ipm_Init( void ) {
  
  }
  void H3_ipm_Quit( void ) {}
};
