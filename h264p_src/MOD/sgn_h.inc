
static const byte H_sgn_a_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_a_pos[3+1]={ 0,1,2,3 };
static const byte H_sgn_a_sa[3+1]={ 0,1,2,3 };
static const byte H_sgn_a_sb[3+1]={ 0,1,2,3 };
static const byte H_sgn_a_gh[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_sgn_b_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_b_scb[3+1]={ 0,1,2,3 };
static const byte H_sgn_b_pos[3+1]={ 0,1,2,3 };

static const byte H_sgn_c_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_c_md[19+1]={ 0,1,2,2,2,3,4,4,4,4,4,4,5,5,6,6,7,8,8,8 };

static const byte H_sgn_d_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_d_mg[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H_sgn_e_cg[13+1]={ 0,1,1,2,3,3,3,3,3,3,3,3,3,3 };
static const byte H_sgn_e_mg[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H_sgn_f_cg[13+1]={ 0,0,0,1,2,3,3,3,3,3,3,3,3,3 };


static const byte H_sgn_h_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_h_pos[3+1]={ 0,1,2,3 };
static const byte H_sgn_h_ez[5+1]={ 0,1,2,3,4,5 };
static const byte H_sgn_h_ts[3+1]={ 0,1,2,3 };
static const byte H_sgn_h_pq[24+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };

static const byte H_sgn_m_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };


static const byte H_sgn_s_se[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H_sgn_w_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H_sgn_w_mg[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int H_sgn_rA = (235+0) * (1);
static const int H_sgn_rB = (16+0) * (1);
static const int H_sgn_rC = (3+0) * (1);
static const int H_sgn_rE = (64+0) * (1);
static const int H_sgn_rH = (64+0) * (1);
static const int H_sgn_rD = (111+0) * (1);
static const int H_sgn_rT = (55+0) * (1);
static const int H_sgn_rG = (90+0) * (1);
static const int H_sgn_rS1 = (6+0) * (1);
static const int H_sgn_rS2 = (6+0) * (1);
static const int H_sgn_lr = (12+0) * (1);
static const int H_sgn_lrm = (12+0) * (1);
static const int H_sgn_mbm = (0+0) * (1);
static const int H_sgn_mwA = (1+0) * (1);
static const int H_sgn_mwB = (0+0) * (1);
static const int H_sgn_mwC = (0+0) * (1);
static const int H_sgn_mwE = (0+0) * (1);
static const int H_sgn_mwH = (0+0) * (1);
static const int H_sgn_mwD = (0+0) * (1);
static const int H_sgn_mwT = (0+0) * (1);
static const int H_sgn_mwG = (0+0) * (1);
static const int H_sgn_bw = (14+0) * (1);
static const int H_sgn_qs = (0+3) * (1);
static const int H_sgn_qf = (122+3) * (1);
static const int H_sgn_mb = (168+0) * (1);
static const int H_sgn_hA = (24+0) * (1);
static const int H_sgn_hB = (24+0) * (1);
static const int H_sgn_hE = (24+0) * (1);
static const int H_sgn_hH = (24+0) * (1);
static const int H_sgn_hD = (24+0) * (1);
static const int H_sgn_hS = (24+0) * (1);
static const int H_sgn_hF = (24+0) * (1);
static const int H_sgn_hM = (24+0) * (1);
static const int H_sgn_hT = (24+0) * (1);
static const int H_sgn_hN = (24+0) * (1);


static const long long H_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4), 4), 9); // 8064
static const long long H_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4); // 224
static const long long H_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 2), 9); // 252
static const long long H_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 3), 16); // 672
static const long long H_sgn_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 16), 1); // 64
static const long long H_sgn_f_Volume = tc_vmul(1, 4); // 4
static const long long H_sgn_g_Volume = tc_vmul(1, 1); // 1
static const long long H_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 6), 3), 4), 25); // 100800
static const long long H_sgn_m_Volume = tc_vmul(tc_vmul(1, 14), 1); // 14
static const long long H_sgn_n_Volume = tc_vmul(1, 1); // 1
static const long long H_sgn_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 8); // 48
static const long long H_sgn_t_Volume = tc_vmul(1, 1); // 1
static const long long H_sgn_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 1), 16); // 224

struct H_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H_sgn_A[tc_rows(H_sgn_a_Volume, H_sgn_hA) * H_sgn_ND];
  cm_cnt2 H_sgn_B[tc_rows(H_sgn_b_Volume, H_sgn_hB) * H_sgn_ND];
  cm_cnt2 H_sgn_C[H_sgn_c_Volume * H_sgn_ND];
  cm_cnt2 H_sgn_D[tc_rows(H_sgn_d_Volume, H_sgn_hD) * H_sgn_ND];
  cm_cnt2 H_sgn_E[tc_rows(H_sgn_e_Volume, H_sgn_hE) * H_sgn_ND];
  cm_cnt2 H_sgn_H[tc_rows(H_sgn_h_Volume, H_sgn_hH) * H_sgn_ND];
  cm_cnt H_sgn_MT[tc_rows(H_sgn_t_Volume, H_sgn_hT) * H_sgn_MND];
  i16 H_sgn_S[tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * H_sgn_qs];
  u8 H_sgn_SC[tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * H_sgn_qs];
  i16 H_sgn_F[tc_rows(H_sgn_f_Volume, H_sgn_hF) * H_sgn_ND * H_sgn_qf];
  u8 H_sgn_FC[tc_rows(H_sgn_f_Volume, H_sgn_hF) * H_sgn_ND * H_sgn_qf];
  i32 H_sgn_W[tc_rows(H_sgn_m_Volume, H_sgn_hM) * H_sgn_ND * TC_MIXN];
  u8 H_sgn_WC[tc_rows(H_sgn_m_Volume, H_sgn_hM) * H_sgn_ND];
  i32 H_sgn_WM[tc_rows(H_sgn_n_Volume, H_sgn_hN) * H_sgn_MND * 3];
  u8 H_sgn_WMC[tc_rows(H_sgn_n_Volume, H_sgn_hN) * H_sgn_MND];
  i32 H_sgn_W2[H_sgn_w_Volume * H_sgn_ND * TC_MIXN];
  u8 H_sgn_W2C[H_sgn_w_Volume * H_sgn_ND];
  i32 H_sgn_W3[tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * TC_MIXN];
  u8 H_sgn_W3C[tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND];
  i32 H_sgn_WF[H_sgn_ND * 4];
  u8 H_sgn_WFC[H_sgn_ND];
  cm_cnt H_sgn_G[H_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_sgn_a_Volume, H_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_sgn_b_Volume, H_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(H_sgn_c_Volume * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_sgn_d_Volume, H_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_sgn_e_Volume, H_sgn_hE) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_sgn_h_Volume, H_sgn_hH) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_sgn_t_Volume, H_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * H_sgn_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * H_sgn_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_sgn_f_Volume, H_sgn_hF) * H_sgn_ND * H_sgn_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_sgn_f_Volume, H_sgn_hF) * H_sgn_ND * H_sgn_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_sgn_m_Volume, H_sgn_hM) * H_sgn_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_sgn_m_Volume, H_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_sgn_n_Volume, H_sgn_hN) * H_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_sgn_n_Volume, H_sgn_hN) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_sgn_w_Volume * H_sgn_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_sgn_w_Volume * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_sgn_s_Volume, H_sgn_hS) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_sgn_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_sgn_g_Volume * H_sgn_NG);
  void H_sgn_Init( void ) {
  
  }
};
