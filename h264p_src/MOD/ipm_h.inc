
static const byte H_ipm_a_ma[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H_ipm_a_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte H_ipm_b_mp[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_b_pv[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_ipm_c_mp[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_c_col[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_ipm_d_mp[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_d_dm[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_d_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_ipm_e_pm[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_e_mp[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_ipm_f_dr[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const byte H_ipm_h_pr[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_ipm_h_dr[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_ipm_h_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };



static const byte H_ipm_s_pr[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const byte H_ipm_w_mp[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_w_dm[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_ipm_w_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const int H_ipm_rA = (228+0) * (1);
static const int H_ipm_rB = (17+0) * (1);
static const int H_ipm_rC = (0+0) * (1);
static const int H_ipm_rE = (64+0) * (1);
static const int H_ipm_rH = (96+0) * (1);
static const int H_ipm_rD = (255+0) * (1);
static const int H_ipm_rT = (55+0) * (1);
static const int H_ipm_rG = (90+0) * (1);
static const int H_ipm_rS1 = (6+0) * (1);
static const int H_ipm_rS2 = (6+0) * (1);
static const int H_ipm_lr = (12+0) * (1);
static const int H_ipm_lrm = (12+0) * (1);
static const int H_ipm_mbm = (0+0) * (1);
static const int H_ipm_mwA = (0+0) * (1);
static const int H_ipm_mwB = (10+0) * (1);
static const int H_ipm_mwC = (0+0) * (1);
static const int H_ipm_mwE = (0+0) * (1);
static const int H_ipm_mwH = (0+0) * (1);
static const int H_ipm_mwD = (224+0) * (1);
static const int H_ipm_mwT = (0+0) * (1);
static const int H_ipm_mwG = (0+0) * (1);
static const int H_ipm_bw = (8+0) * (1);
static const int H_ipm_qs = (33+3) * (1);
static const int H_ipm_qf = (16+3) * (1);
static const int H_ipm_mb = (44+0) * (1);
static const int H_ipm_hA = (24+0) * (1);
static const int H_ipm_hB = (24+0) * (1);
static const int H_ipm_hE = (24+0) * (1);
static const int H_ipm_hH = (24+0) * (1);
static const int H_ipm_hD = (24+0) * (1);
static const int H_ipm_hS = (24+0) * (1);
static const int H_ipm_hF = (24+0) * (1);
static const int H_ipm_hM = (24+0) * (1);
static const int H_ipm_hT = (24+0) * (1);
static const int H_ipm_hN = (24+0) * (1);


static const long long H_ipm_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 11); // 363
static const long long H_ipm_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 10); // 300
static const long long H_ipm_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 10); // 300
static const long long H_ipm_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 10), 10); // 3000
static const long long H_ipm_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 10); // 300
static const long long H_ipm_f_Volume = tc_vmul(tc_vmul(1, 3), 9); // 27
static const long long H_ipm_g_Volume = tc_vmul(1, 1); // 1
static const long long H_ipm_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 9), 9), 10); // 2430
static const long long H_ipm_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 3); // 3
static const long long H_ipm_n_Volume = tc_vmul(1, 1); // 1
static const long long H_ipm_s_Volume = tc_vmul(tc_vmul(1, 3), 9); // 27
static const long long H_ipm_t_Volume = tc_vmul(1, 1); // 1
static const long long H_ipm_w_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 10), 10), 10); // 3000

struct H_ipm_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt H_ipm_A[tc_rows(H_ipm_a_Volume, H_ipm_hA) * H_ipm_ND];
  cm_cnt H_ipm_B[tc_rows(H_ipm_b_Volume, H_ipm_hB) * H_ipm_ND];
  cm_cnt H_ipm_C[H_ipm_c_Volume * H_ipm_ND];
  cm_cnt H_ipm_D[tc_rows(H_ipm_d_Volume, H_ipm_hD) * H_ipm_ND];
  cm_cnt H_ipm_E[tc_rows(H_ipm_e_Volume, H_ipm_hE) * H_ipm_ND];
  cm_cnt H_ipm_H[tc_rows(H_ipm_h_Volume, H_ipm_hH) * H_ipm_ND];
  cm_cnt H_ipm_MT[tc_rows(H_ipm_t_Volume, H_ipm_hT) * H_ipm_MND];
  i16 H_ipm_S[tc_rows(H_ipm_s_Volume, H_ipm_hS) * H_ipm_ND * H_ipm_qs];
  u8 H_ipm_SC[tc_rows(H_ipm_s_Volume, H_ipm_hS) * H_ipm_ND * H_ipm_qs];
  i16 H_ipm_F[tc_rows(H_ipm_f_Volume, H_ipm_hF) * H_ipm_ND * H_ipm_qf];
  u8 H_ipm_FC[tc_rows(H_ipm_f_Volume, H_ipm_hF) * H_ipm_ND * H_ipm_qf];
  i32 H_ipm_W[tc_rows(H_ipm_m_Volume, H_ipm_hM) * H_ipm_ND * TC_MIXN];
  u8 H_ipm_WC[tc_rows(H_ipm_m_Volume, H_ipm_hM) * H_ipm_ND];
  i32 H_ipm_WM[tc_rows(H_ipm_n_Volume, H_ipm_hN) * H_ipm_MND * 3];
  u8 H_ipm_WMC[tc_rows(H_ipm_n_Volume, H_ipm_hN) * H_ipm_MND];
  i32 H_ipm_W2[H_ipm_w_Volume * H_ipm_ND * TC_MIXN];
  u8 H_ipm_W2C[H_ipm_w_Volume * H_ipm_ND];
  i32 H_ipm_WF[H_ipm_ND * 3];
  u8 H_ipm_WFC[H_ipm_ND];
  cm_cnt H_ipm_G[H_ipm_g_Volume * H_ipm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_ipm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_a_Volume, H_ipm_hA) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_b_Volume, H_ipm_hB) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_ipm_c_Volume * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_d_Volume, H_ipm_hD) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_e_Volume, H_ipm_hE) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_h_Volume, H_ipm_hH) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_ipm_t_Volume, H_ipm_hT) * H_ipm_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_ipm_s_Volume, H_ipm_hS) * H_ipm_ND * H_ipm_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_ipm_s_Volume, H_ipm_hS) * H_ipm_ND * H_ipm_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_ipm_f_Volume, H_ipm_hF) * H_ipm_ND * H_ipm_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_ipm_f_Volume, H_ipm_hF) * H_ipm_ND * H_ipm_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_ipm_m_Volume, H_ipm_hM) * H_ipm_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_ipm_m_Volume, H_ipm_hM) * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_ipm_n_Volume, H_ipm_hN) * H_ipm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_ipm_n_Volume, H_ipm_hN) * H_ipm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_ipm_w_Volume * H_ipm_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_ipm_w_Volume * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_ipm_ND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_ipm_g_Volume * H_ipm_NG);
  void H_ipm_Init( void ) {
  
  }
};
