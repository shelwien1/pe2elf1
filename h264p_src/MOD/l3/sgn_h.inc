
static const byte H3_sgn_a_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_a_pos[3+1]={ 0,1,2,3 };
static const byte H3_sgn_a_sa[3+1]={ 0,1,2,3 };
static const byte H3_sgn_a_sb[3+1]={ 0,1,2,3 };
static const byte H3_sgn_a_gh[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_sgn_b_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_b_scb[3+1]={ 0,1,2,3 };
static const byte H3_sgn_b_pos[3+1]={ 0,1,2,3 };

static const byte H3_sgn_c_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_c_md[19+1]={ 0,1,2,2,2,3,4,4,4,4,4,4,5,5,6,6,7,8,8,8 };

static const byte H3_sgn_d_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H3_sgn_e_cg[13+1]={ 0,1,1,2,3,3,3,3,3,3,3,3,3,3 };


static const byte H3_sgn_h_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_h_pos[3+1]={ 0,1,2,3 };

static const byte H3_sgn_i_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H3_sgn_j_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_j_sa[3+1]={ 0,1,2,3 };
static const byte H3_sgn_j_sb[3+1]={ 0,1,2,3 };
static const byte H3_sgn_j_gh[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_sgn_k_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte H3_sgn_l_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H3_sgn_l_pos[3+1]={ 0,1,2,3 };
static const byte H3_sgn_l_gh[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_sgn_m_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };




static const byte H3_sgn_w_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };


static const int H3_sgn_rA = (233+0) * (1);
static const int H3_sgn_rB = (205+0) * (1);
static const int H3_sgn_rC = (3+0) * (1);
static const int H3_sgn_rE = (64+0) * (1);
static const int H3_sgn_rH = (64+0) * (1);
static const int H3_sgn_rI = (64+0) * (1);
static const int H3_sgn_rJ = (224+0) * (1);
static const int H3_sgn_rK = (224+0) * (1);
static const int H3_sgn_rL = (235+0) * (1);
static const int H3_sgn_rD = (246+0) * (1);
static const int H3_sgn_rT = (55+0) * (1);
static const int H3_sgn_rG = (90+0) * (1);
static const int H3_sgn_lr = (12+0) * (1);
static const int H3_sgn_lrm = (12+0) * (1);
static const int H3_sgn_mbm = (0+0) * (1);
static const int H3_sgn_mwA = (29+0) * (1);
static const int H3_sgn_mwB = (5+0) * (1);
static const int H3_sgn_mwC = (0+0) * (1);
static const int H3_sgn_mwE = (0+0) * (1);
static const int H3_sgn_mwH = (32+0) * (1);
static const int H3_sgn_mwI = (0+0) * (1);
static const int H3_sgn_mwJ = (0+0) * (1);
static const int H3_sgn_mwK = (0+0) * (1);
static const int H3_sgn_mwL = (0+0) * (1);
static const int H3_sgn_mwD = (31+0) * (1);
static const int H3_sgn_mwT = (0+0) * (1);
static const int H3_sgn_mwG = (0+0) * (1);
static const int H3_sgn_mt = (6+0) * (1);
static const int H3_sgn_mb = (160+0) * (1);
static const int H3_sgn_hA = (24+0) * (1);
static const int H3_sgn_hB = (24+0) * (1);
static const int H3_sgn_hC = (24+0) * (1);
static const int H3_sgn_hE = (24+0) * (1);
static const int H3_sgn_hH = (24+0) * (1);
static const int H3_sgn_hI = (24+0) * (1);
static const int H3_sgn_hJ = (24+0) * (1);
static const int H3_sgn_hK = (24+0) * (1);
static const int H3_sgn_hL = (24+0) * (1);
static const int H3_sgn_hD = (24+0) * (1);
static const int H3_sgn_hS = (24+0) * (1);
static const int H3_sgn_hM = (24+0) * (1);
static const int H3_sgn_hT = (24+0) * (1);
static const int H3_sgn_hN = (24+0) * (1);


static const long long H3_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4), 4), 9); // 8064
static const long long H3_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4); // 224
static const long long H3_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 2), 9); // 252
static const long long H3_sgn_d_Volume = tc_vmul(1, 14); // 14
static const long long H3_sgn_e_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H3_sgn_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_sgn_h_Volume = tc_vmul(tc_vmul(1, 14), 4); // 56
static const long long H3_sgn_i_Volume = tc_vmul(tc_vmul(1, 14), 1); // 14
static const long long H3_sgn_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4), 9), 2); // 4032
static const long long H3_sgn_k_Volume = tc_vmul(1, 14); // 14
static const long long H3_sgn_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 9); // 504
static const long long H3_sgn_m_Volume = tc_vmul(1, 14); // 14
static const long long H3_sgn_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_sgn_s_Volume = tc_vmul(1, 2); // 2
static const long long H3_sgn_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_sgn_w_Volume = tc_vmul(1, 14); // 14

#define H3_sgn_USES(X) X(cg) X(pos) X(sa) X(sb) X(gh) X(scb) X(mag) X(md)

struct H3_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_sgn_A[tc_rows(H3_sgn_a_Volume, H3_sgn_hA) * H_sgn_ND];
  cm_cnt2 H3_sgn_B[tc_rows(H3_sgn_b_Volume, H3_sgn_hB) * H_sgn_ND];
  cm_cnt2 H3_sgn_C[tc_rows(H3_sgn_c_Volume, H3_sgn_hC) * H_sgn_ND];
  cm_cnt2 H3_sgn_D[tc_rows(H3_sgn_d_Volume, H3_sgn_hD) * H_sgn_ND];
  cm_cnt2 H3_sgn_E[tc_rows(H3_sgn_e_Volume, H3_sgn_hE) * H_sgn_ND];
  cm_cnt2 H3_sgn_H[tc_rows(H3_sgn_h_Volume, H3_sgn_hH) * H_sgn_ND];
  cm_cnt2 H3_sgn_I[tc_rows(H3_sgn_i_Volume, H3_sgn_hI) * H_sgn_ND];
  cm_cnt2 H3_sgn_J[tc_rows(H3_sgn_j_Volume, H3_sgn_hJ) * H_sgn_ND];
  cm_cnt2 H3_sgn_K[tc_rows(H3_sgn_k_Volume, H3_sgn_hK) * H_sgn_ND];
  cm_cnt2 H3_sgn_L[tc_rows(H3_sgn_l_Volume, H3_sgn_hL) * H_sgn_ND];
  cm_cnt H3_sgn_MT[tc_rows(H3_sgn_t_Volume, H3_sgn_hT) * H_sgn_MND];
  i32 H3_sgn_W[tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND * 24];
  u8 H3_sgn_WC[tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND];
  i32 H3_sgn_WM[tc_rows(H3_sgn_n_Volume, H3_sgn_hN) * H_sgn_MND * 3];
  u8 H3_sgn_WMC[tc_rows(H3_sgn_n_Volume, H3_sgn_hN) * H_sgn_MND];
  i32 H3_sgn_W2[H3_sgn_w_Volume * H_sgn_ND * 24];
  u8 H3_sgn_W2C[H3_sgn_w_Volume * H_sgn_ND];
  i32 H3_sgn_W3[tc_rows(H3_sgn_s_Volume, H3_sgn_hS) * H_sgn_ND * 24];
  u8 H3_sgn_W3C[tc_rows(H3_sgn_s_Volume, H3_sgn_hS) * H_sgn_ND];
  i32 H3_sgn_WF[H_sgn_ND * 5 * 4];
  u8 H3_sgn_WFC[H_sgn_ND * 5];
  cm_cnt H3_sgn_G[H3_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_a_Volume, H3_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_b_Volume, H3_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_c_Volume, H3_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_d_Volume, H3_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_e_Volume, H3_sgn_hE) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_h_Volume, H3_sgn_hH) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_i_Volume, H3_sgn_hI) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_j_Volume, H3_sgn_hJ) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_k_Volume, H3_sgn_hK) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_l_Volume, H3_sgn_hL) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_sgn_t_Volume, H3_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sgn_n_Volume, H3_sgn_hN) * H_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sgn_n_Volume, H3_sgn_hN) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_sgn_w_Volume * H_sgn_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_sgn_w_Volume * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sgn_s_Volume, H3_sgn_hS) * H_sgn_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sgn_s_Volume, H3_sgn_hS) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_sgn_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_sgn_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_sgn_g_Volume * H_sgn_NG);
  void H3_sgn_Init( void ) {
  
  }
};
