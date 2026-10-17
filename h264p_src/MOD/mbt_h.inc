
static const byte H_mbt_a_ca[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mbt_a_cb[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mbt_a_en[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };

static const byte H_mbt_b_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_b_cc[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mbt_b_nsk[4+1]={ 0,1,1,2,3 };
static const byte H_mbt_b_td[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_mbt_c_ca[8+1]={ 0,1,1,2,3,4,5,5,5 };
static const byte H_mbt_c_cc[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mbt_d_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_d_sm[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_mbt_d_tm[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_mbt_e_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_e_ca[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mbt_e_td[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_mbt_e_sm[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_mbt_f_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H_mbt_h_dh[3+1]={ 0,1,2,3 };
static const byte H_mbt_h_tb[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_mbt_h_nsk[4+1]={ 0,0,1,2,3 };
static const byte H_mbt_h_bd[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_mbt_m_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_m_bd[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };


static const byte H_mbt_s_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_s_tb[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_mbt_s_dh[3+1]={ 0,1,2,3 };


static const byte H_mbt_w_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_mbt_w_sm[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_mbt_w_tm[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };


static const int H_mbt_rA = (64+0) * (1);
static const int H_mbt_rB = (0+0) * (1);
static const int H_mbt_rC = (253+0) * (1);
static const int H_mbt_rE = (64+0) * (1);
static const int H_mbt_rH = (64+0) * (1);
static const int H_mbt_rD = (239+0) * (1);
static const int H_mbt_rT = (55+0) * (1);
static const int H_mbt_rG = (90+0) * (1);
static const int H_mbt_rS1 = (0+0) * (1);
static const int H_mbt_rS2 = (3+0) * (1);
static const int H_mbt_lr = (12+0) * (1);
static const int H_mbt_lrm = (12+0) * (1);
static const int H_mbt_mbm = (0+0) * (1);
static const int H_mbt_mwA = (16+0) * (1);
static const int H_mbt_mwB = (1+0) * (1);
static const int H_mbt_mwC = (0+0) * (1);
static const int H_mbt_mwE = (0+0) * (1);
static const int H_mbt_mwH = (0+0) * (1);
static const int H_mbt_mwD = (0+0) * (1);
static const int H_mbt_mwT = (0+0) * (1);
static const int H_mbt_mwG = (0+0) * (1);
static const int H_mbt_bw = (8+0) * (1);
static const int H_mbt_qs = (16+3) * (1);
static const int H_mbt_qf = (22+3) * (1);
static const int H_mbt_mb = (22+0) * (1);
static const int H_mbt_hA = (24+0) * (1);
static const int H_mbt_hB = (24+0) * (1);
static const int H_mbt_hE = (24+0) * (1);
static const int H_mbt_hH = (24+0) * (1);
static const int H_mbt_hD = (24+0) * (1);
static const int H_mbt_hS = (24+0) * (1);
static const int H_mbt_hF = (24+0) * (1);
static const int H_mbt_hM = (24+0) * (1);
static const int H_mbt_hT = (24+0) * (1);
static const int H_mbt_hN = (24+0) * (1);


static const long long H_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 9), 9), 15); // 2430
static const long long H_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 10), 9), 4), 17); // 6120
static const long long H_mbt_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 6), 9); // 162
static const long long H_mbt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 17), 17); // 2890
static const long long H_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 10), 9), 17), 17); // 26010
static const long long H_mbt_f_Volume = tc_vmul(tc_vmul(1, 10), 3); // 30
static const long long H_mbt_g_Volume = tc_vmul(1, 1); // 1
static const long long H_mbt_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 4), 17), 4), 17); // 9248
static const long long H_mbt_m_Volume = tc_vmul(tc_vmul(1, 10), 17); // 170
static const long long H_mbt_n_Volume = tc_vmul(1, 1); // 1
static const long long H_mbt_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 17), 4); // 680
static const long long H_mbt_t_Volume = tc_vmul(1, 1); // 1
static const long long H_mbt_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 17), 17); // 2890

struct H_mbt_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt H_mbt_A[tc_rows(H_mbt_a_Volume, H_mbt_hA) * H_mbt_ND];
  cm_cnt H_mbt_B[tc_rows(H_mbt_b_Volume, H_mbt_hB) * H_mbt_ND];
  cm_cnt H_mbt_C[H_mbt_c_Volume * H_mbt_ND];
  cm_cnt H_mbt_D[tc_rows(H_mbt_d_Volume, H_mbt_hD) * H_mbt_ND];
  cm_cnt H_mbt_E[tc_rows(H_mbt_e_Volume, H_mbt_hE) * H_mbt_ND];
  cm_cnt H_mbt_H[tc_rows(H_mbt_h_Volume, H_mbt_hH) * H_mbt_ND];
  cm_cnt H_mbt_MT[tc_rows(H_mbt_t_Volume, H_mbt_hT) * H_mbt_MND];
  i16 H_mbt_S[tc_rows(H_mbt_s_Volume, H_mbt_hS) * H_mbt_ND * H_mbt_qs];
  u8 H_mbt_SC[tc_rows(H_mbt_s_Volume, H_mbt_hS) * H_mbt_ND * H_mbt_qs];
  i16 H_mbt_F[tc_rows(H_mbt_f_Volume, H_mbt_hF) * H_mbt_ND * H_mbt_qf];
  u8 H_mbt_FC[tc_rows(H_mbt_f_Volume, H_mbt_hF) * H_mbt_ND * H_mbt_qf];
  i32 H_mbt_W[tc_rows(H_mbt_m_Volume, H_mbt_hM) * H_mbt_ND * TC_MIXN];
  u8 H_mbt_WC[tc_rows(H_mbt_m_Volume, H_mbt_hM) * H_mbt_ND];
  i32 H_mbt_WM[tc_rows(H_mbt_n_Volume, H_mbt_hN) * H_mbt_MND * 3];
  u8 H_mbt_WMC[tc_rows(H_mbt_n_Volume, H_mbt_hN) * H_mbt_MND];
  i32 H_mbt_W2[H_mbt_w_Volume * H_mbt_ND * TC_MIXN];
  u8 H_mbt_W2C[H_mbt_w_Volume * H_mbt_ND];
  i32 H_mbt_WF[H_mbt_ND * 3];
  u8 H_mbt_WFC[H_mbt_ND];
  cm_cnt H_mbt_G[H_mbt_g_Volume * H_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_a_Volume, H_mbt_hA) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_b_Volume, H_mbt_hB) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_mbt_c_Volume * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_d_Volume, H_mbt_hD) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_e_Volume, H_mbt_hE) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_h_Volume, H_mbt_hH) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mbt_t_Volume, H_mbt_hT) * H_mbt_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mbt_s_Volume, H_mbt_hS) * H_mbt_ND * H_mbt_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mbt_s_Volume, H_mbt_hS) * H_mbt_ND * H_mbt_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mbt_f_Volume, H_mbt_hF) * H_mbt_ND * H_mbt_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mbt_f_Volume, H_mbt_hF) * H_mbt_ND * H_mbt_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mbt_m_Volume, H_mbt_hM) * H_mbt_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mbt_m_Volume, H_mbt_hM) * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mbt_n_Volume, H_mbt_hN) * H_mbt_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mbt_n_Volume, H_mbt_hN) * H_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mbt_w_Volume * H_mbt_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mbt_w_Volume * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mbt_ND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_mbt_g_Volume * H_mbt_NG);
  void H_mbt_Init( void ) {
  
  }
};
