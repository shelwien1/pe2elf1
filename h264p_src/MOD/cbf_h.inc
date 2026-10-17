
static const byte H_cbf_a_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_a_na[9+1]={ 0,1,2,2,3,3,3,4,4,5 };
static const byte H_cbf_a_nb[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_cbf_b_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_b_np[3+1]={ 0,1,2,3 };
static const byte H_cbf_b_pos[3+1]={ 0,1,2,3 };
static const byte H_cbf_b_em[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_cbf_c_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbf_c_en[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };

static const byte H_cbf_d_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_d_nc[5+1]={ 0,1,2,3,4,5 };
static const byte H_cbf_d_md[19+1]={ 0,0,1,2,2,2,2,2,2,2,2,2,2,2,3,3,3,4,4,4 };
static const byte H_cbf_d_pg[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_cbf_e_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_e_pz[5+1]={ 0,1,2,3,4,5 };
static const byte H_cbf_e_na[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_cbf_e_nb[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H_cbf_e_tn[5+1]={ 0,1,2,3,4,5 };

static const byte H_cbf_f_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };


static const byte H_cbf_h_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_h_pg[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbf_h_md[19+1]={ 0,1,2,2,2,2,2,3,3,3,4,5,6,6,7,7,7,8,8,9 };
static const byte H_cbf_h_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_cbf_m_cat[13+1]={ 0,1,2,3,4,4,4,4,4,4,4,4,4,4 };

static const byte H_cbf_n_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H_cbf_s_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H_cbf_t_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H_cbf_w_cat[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_cbf_w_nc[5+1]={ 0,1,2,3,4,5 };
static const byte H_cbf_w_md[19+1]={ 0,0,1,2,2,2,2,2,2,2,2,2,2,2,3,3,3,4,4,4 };


static const int H_cbf_rA = (64+0) * (1);
static const int H_cbf_rB = (16+0) * (1);
static const int H_cbf_rC = (64+0) * (1);
static const int H_cbf_rE = (64+0) * (1);
static const int H_cbf_rH = (64+0) * (1);
static const int H_cbf_rD = (242+0) * (1);
static const int H_cbf_rT = (55+0) * (1);
static const int H_cbf_rG = (90+0) * (1);
static const int H_cbf_rS1 = (6+0) * (1);
static const int H_cbf_rS2 = (6+0) * (1);
static const int H_cbf_lr = (12+0) * (1);
static const int H_cbf_lrm = (12+0) * (1);
static const int H_cbf_mbm = (0+0) * (1);
static const int H_cbf_mwA = (0+0) * (1);
static const int H_cbf_mwB = (0+0) * (1);
static const int H_cbf_mwC = (0+0) * (1);
static const int H_cbf_mwE = (0+0) * (1);
static const int H_cbf_mwH = (0+0) * (1);
static const int H_cbf_mwD = (200+0) * (1);
static const int H_cbf_mwT = (8+0) * (1);
static const int H_cbf_mwG = (0+0) * (1);
static const int H_cbf_bw = (16+0) * (1);
static const int H_cbf_qs = (0+3) * (1);
static const int H_cbf_qf = (16+3) * (1);
static const int H_cbf_mb = (130+0) * (1);
static const int H_cbf_hA = (24+0) * (1);
static const int H_cbf_hB = (24+0) * (1);
static const int H_cbf_hE = (24+0) * (1);
static const int H_cbf_hH = (24+0) * (1);
static const int H_cbf_hD = (24+0) * (1);
static const int H_cbf_hS = (24+0) * (1);
static const int H_cbf_hF = (24+0) * (1);
static const int H_cbf_hM = (24+0) * (1);
static const int H_cbf_hT = (24+0) * (1);
static const int H_cbf_hN = (24+0) * (1);


static const long long H_cbf_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 10), 2); // 1680
static const long long H_cbf_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4), 3), 2), 17); // 22848
static const long long H_cbf_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 9), 3), 2), 15); // 1620
static const long long H_cbf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 5), 2), 17); // 14280
static const long long H_cbf_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 10), 10), 6); // 50400
static const long long H_cbf_f_Volume = tc_vmul(tc_vmul(1, 14), 2); // 28
static const long long H_cbf_g_Volume = tc_vmul(1, 1); // 1
static const long long H_cbf_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 17), 10), 9); // 21420
static const long long H_cbf_m_Volume = tc_vmul(tc_vmul(1, 5), 2); // 10
static const long long H_cbf_n_Volume = tc_vmul(1, 14); // 14
static const long long H_cbf_s_Volume = tc_vmul(tc_vmul(1, 14), 2); // 28
static const long long H_cbf_t_Volume = tc_vmul(1, 14); // 14
static const long long H_cbf_w_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 5), 2); // 840

struct H_cbf_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt H_cbf_A[tc_rows(H_cbf_a_Volume, H_cbf_hA) * H_cbf_ND];
  cm_cnt H_cbf_B[tc_rows(H_cbf_b_Volume, H_cbf_hB) * H_cbf_ND];
  cm_cnt H_cbf_C[H_cbf_c_Volume * H_cbf_ND];
  cm_cnt H_cbf_D[tc_rows(H_cbf_d_Volume, H_cbf_hD) * H_cbf_ND];
  cm_cnt H_cbf_E[tc_rows(H_cbf_e_Volume, H_cbf_hE) * H_cbf_ND];
  cm_cnt H_cbf_H[tc_rows(H_cbf_h_Volume, H_cbf_hH) * H_cbf_ND];
  cm_cnt H_cbf_MT[tc_rows(H_cbf_t_Volume, H_cbf_hT) * H_cbf_MND];
  i16 H_cbf_S[tc_rows(H_cbf_s_Volume, H_cbf_hS) * H_cbf_ND * H_cbf_qs];
  u8 H_cbf_SC[tc_rows(H_cbf_s_Volume, H_cbf_hS) * H_cbf_ND * H_cbf_qs];
  i16 H_cbf_F[tc_rows(H_cbf_f_Volume, H_cbf_hF) * H_cbf_ND * H_cbf_qf];
  u8 H_cbf_FC[tc_rows(H_cbf_f_Volume, H_cbf_hF) * H_cbf_ND * H_cbf_qf];
  i32 H_cbf_W[tc_rows(H_cbf_m_Volume, H_cbf_hM) * H_cbf_ND * TC_MIXN];
  u8 H_cbf_WC[tc_rows(H_cbf_m_Volume, H_cbf_hM) * H_cbf_ND];
  i32 H_cbf_WM[tc_rows(H_cbf_n_Volume, H_cbf_hN) * H_cbf_MND * 3];
  u8 H_cbf_WMC[tc_rows(H_cbf_n_Volume, H_cbf_hN) * H_cbf_MND];
  i32 H_cbf_W2[H_cbf_w_Volume * H_cbf_ND * TC_MIXN];
  u8 H_cbf_W2C[H_cbf_w_Volume * H_cbf_ND];
  i32 H_cbf_WF[H_cbf_ND * 3];
  u8 H_cbf_WFC[H_cbf_ND];
  cm_cnt H_cbf_G[H_cbf_g_Volume * H_cbf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_cbf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_a_Volume, H_cbf_hA) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_b_Volume, H_cbf_hB) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_cbf_c_Volume * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_d_Volume, H_cbf_hD) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_e_Volume, H_cbf_hE) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_h_Volume, H_cbf_hH) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbf_t_Volume, H_cbf_hT) * H_cbf_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_cbf_s_Volume, H_cbf_hS) * H_cbf_ND * H_cbf_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbf_s_Volume, H_cbf_hS) * H_cbf_ND * H_cbf_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_cbf_f_Volume, H_cbf_hF) * H_cbf_ND * H_cbf_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbf_f_Volume, H_cbf_hF) * H_cbf_ND * H_cbf_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_cbf_m_Volume, H_cbf_hM) * H_cbf_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbf_m_Volume, H_cbf_hM) * H_cbf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_cbf_n_Volume, H_cbf_hN) * H_cbf_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbf_n_Volume, H_cbf_hN) * H_cbf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbf_w_Volume * H_cbf_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbf_w_Volume * H_cbf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbf_ND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_cbf_g_Volume * H_cbf_NG);
  void H_cbf_Init( void ) {
  
  }
};
