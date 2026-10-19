
static const byte H3_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H3_cbp_a_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H3_cbp_a_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H3_cbp_b_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H3_cbp_b_q6[8+1]={ 0,1,1,2,2,2,2,3,3 };

static const byte H3_cbp_c_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H3_cbp_c_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H3_cbp_d_inc[3+1]={ 0,1,2,3 };
static const byte H3_cbp_d_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_cbp_e_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const byte H3_cbp_h_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_cbp_i_inc[3+1]={ 0,1,2,3 };
static const byte H3_cbp_i_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H3_cbp_i_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_cbp_j_inc[3+1]={ 0,1,2,3 };
static const byte H3_cbp_j_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H3_cbp_k_inc[3+1]={ 0,1,2,3 };
static const byte H3_cbp_k_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H3_cbp_k_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H3_cbp_l_inc[3+1]={ 0,1,2,3 };
static const byte H3_cbp_l_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };



static const byte H3_cbp_s_inc[3+1]={ 0,1,2,3 };


static const byte H3_cbp_w_inc[3+1]={ 0,1,2,3 };


static const int H3_cbp_rA = (235+0) * (1);
static const int H3_cbp_rB = (245+0) * (1);
static const int H3_cbp_rC = (64+0) * (1);
static const int H3_cbp_rE = (64+0) * (1);
static const int H3_cbp_rH = (64+0) * (1);
static const int H3_cbp_rI = (100+0) * (1);
static const int H3_cbp_rJ = (64+0) * (1);
static const int H3_cbp_rK = (224+0) * (1);
static const int H3_cbp_rL = (64+0) * (1);
static const int H3_cbp_rD = (255+0) * (1);
static const int H3_cbp_rT = (55+0) * (1);
static const int H3_cbp_rG = (90+0) * (1);
static const int H3_cbp_lr = (15+0) * (1);
static const int H3_cbp_lrm = (12+0) * (1);
static const int H3_cbp_mbm = (0+0) * (1);
static const int H3_cbp_mwA = (0+0) * (1);
static const int H3_cbp_mwB = (0+0) * (1);
static const int H3_cbp_mwC = (0+0) * (1);
static const int H3_cbp_mwE = (4+0) * (1);
static const int H3_cbp_mwH = (0+0) * (1);
static const int H3_cbp_mwI = (0+0) * (1);
static const int H3_cbp_mwJ = (0+0) * (1);
static const int H3_cbp_mwK = (0+0) * (1);
static const int H3_cbp_mwL = (44+0) * (1);
static const int H3_cbp_mwD = (18+0) * (1);
static const int H3_cbp_mwT = (0+0) * (1);
static const int H3_cbp_mwG = (0+0) * (1);
static const int H3_cbp_mt = (6+0) * (1);
static const int H3_cbp_mb = (5+0) * (1);
static const int H3_cbp_hA = (24+0) * (1);
static const int H3_cbp_hB = (24+0) * (1);
static const int H3_cbp_hC = (24+0) * (1);
static const int H3_cbp_hE = (24+0) * (1);
static const int H3_cbp_hH = (24+0) * (1);
static const int H3_cbp_hI = (24+0) * (1);
static const int H3_cbp_hJ = (24+0) * (1);
static const int H3_cbp_hK = (24+0) * (1);
static const int H3_cbp_hL = (24+0) * (1);
static const int H3_cbp_hD = (24+0) * (1);
static const int H3_cbp_hS = (24+0) * (1);
static const int H3_cbp_hM = (24+0) * (1);
static const int H3_cbp_hT = (24+0) * (1);
static const int H3_cbp_hN = (24+0) * (1);


static const long long H3_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 9), 10); // 1080
static const long long H3_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 13), 4); // 52
static const long long H3_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 13), 3); // 351
static const long long H3_cbp_d_Volume = tc_vmul(tc_vmul(1, 4), 9); // 36
static const long long H3_cbp_e_Volume = tc_vmul(tc_vmul(1, 3), 9); // 27
static const long long H3_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_cbp_h_Volume = tc_vmul(tc_vmul(1, 3), 9); // 27
static const long long H3_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 9), 9); // 324
static const long long H3_cbp_j_Volume = tc_vmul(tc_vmul(1, 4), 10); // 40
static const long long H3_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 13), 9); // 468
static const long long H3_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 9); // 108
static const long long H3_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H3_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_cbp_s_Volume = tc_vmul(1, 4); // 4
static const long long H3_cbp_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_cbp_w_Volume = tc_vmul(tc_vmul(1, 4), 3); // 12

#define H3_cbp_USES(X) X(inc) X(pc) X(cls) X(st) X(col) X(q6)

struct H3_cbp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_cbp_A[tc_rows(H3_cbp_a_Volume, H3_cbp_hA) * H_cbp_ND];
  cm_cnt2 H3_cbp_B[tc_rows(H3_cbp_b_Volume, H3_cbp_hB) * H_cbp_ND];
  cm_cnt2 H3_cbp_C[tc_rows(H3_cbp_c_Volume, H3_cbp_hC) * H_cbp_ND];
  cm_cnt2 H3_cbp_D[tc_rows(H3_cbp_d_Volume, H3_cbp_hD) * H_cbp_ND];
  cm_cnt2 H3_cbp_E[tc_rows(H3_cbp_e_Volume, H3_cbp_hE) * H_cbp_ND];
  cm_cnt2 H3_cbp_H[tc_rows(H3_cbp_h_Volume, H3_cbp_hH) * H_cbp_ND];
  cm_cnt2 H3_cbp_I[tc_rows(H3_cbp_i_Volume, H3_cbp_hI) * H_cbp_ND];
  cm_cnt2 H3_cbp_J[tc_rows(H3_cbp_j_Volume, H3_cbp_hJ) * H_cbp_ND];
  cm_cnt2 H3_cbp_K[tc_rows(H3_cbp_k_Volume, H3_cbp_hK) * H_cbp_ND];
  cm_cnt2 H3_cbp_L[tc_rows(H3_cbp_l_Volume, H3_cbp_hL) * H_cbp_ND];
  cm_cnt H3_cbp_MT[tc_rows(H3_cbp_t_Volume, H3_cbp_hT) * H_cbp_MND];
  i32 H3_cbp_W[tc_rows(H3_cbp_m_Volume, H3_cbp_hM) * H_cbp_ND * 24];
  u8 H3_cbp_WC[tc_rows(H3_cbp_m_Volume, H3_cbp_hM) * H_cbp_ND];
  i32 H3_cbp_WM[tc_rows(H3_cbp_n_Volume, H3_cbp_hN) * H_cbp_MND * 3];
  u8 H3_cbp_WMC[tc_rows(H3_cbp_n_Volume, H3_cbp_hN) * H_cbp_MND];
  i32 H3_cbp_W2[H3_cbp_w_Volume * H_cbp_ND * 24];
  u8 H3_cbp_W2C[H3_cbp_w_Volume * H_cbp_ND];
  i32 H3_cbp_W3[tc_rows(H3_cbp_s_Volume, H3_cbp_hS) * H_cbp_ND * 24];
  u8 H3_cbp_W3C[tc_rows(H3_cbp_s_Volume, H3_cbp_hS) * H_cbp_ND];
  i32 H3_cbp_WF[H_cbp_ND * 5 * 4];
  u8 H3_cbp_WFC[H_cbp_ND * 5];
  cm_cnt H3_cbp_G[H3_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_a_Volume, H3_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_b_Volume, H3_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_c_Volume, H3_cbp_hC) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_d_Volume, H3_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_e_Volume, H3_cbp_hE) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_h_Volume, H3_cbp_hH) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_i_Volume, H3_cbp_hI) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_j_Volume, H3_cbp_hJ) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_k_Volume, H3_cbp_hK) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_cbp_l_Volume, H3_cbp_hL) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_cbp_t_Volume, H3_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_cbp_m_Volume, H3_cbp_hM) * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_cbp_m_Volume, H3_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_cbp_n_Volume, H3_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_cbp_n_Volume, H3_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_cbp_w_Volume * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_cbp_w_Volume * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_cbp_s_Volume, H3_cbp_hS) * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_cbp_s_Volume, H3_cbp_hS) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_cbp_g_Volume * H_cbp_NG);
  void H3_cbp_Init( void ) {
  
  }
};
