


static const byte H7_unt_c_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H7_unt_d_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H7_unt_e_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };


static const byte H7_unt_h_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H7_unt_i_ctx[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H7_unt_i_p1[6+1]={ 0,1,2,3,4,5,6 };


static const byte H7_unt_k_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };
static const byte H7_unt_k_ctx[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_unt_l_p1[6+1]={ 0,1,2,3,4,5,6 };

static const byte H7_unt_m_fld[15+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H7_unt_s_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H7_unt_t_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H7_unt_w_fld[7+1]={ 0,1,2,2,2,3,3,3 };


static const int H7_unt_rA = (0+0) * (1);
static const int H7_unt_rB = (192+0) * (1);
static const int H7_unt_rC = (255+0) * (1);
static const int H7_unt_rD = (0+0) * (1);
static const int H7_unt_rE = (0+0) * (1);
static const int H7_unt_rH = (0+0) * (1);
static const int H7_unt_rI = (137+0) * (1);
static const int H7_unt_rJ = (0+0) * (1);
static const int H7_unt_rK = (0+0) * (1);
static const int H7_unt_rL = (253+0) * (1);
static const int H7_unt_rT = (128+0) * (1);
static const int H7_unt_rG = (64+0) * (1);
static const int H7_unt_lr = (23+0) * (1);
static const int H7_unt_mwA = (0+0) * (1);
static const int H7_unt_mwB = (0+0) * (1);
static const int H7_unt_mwC = (0+0) * (1);
static const int H7_unt_mwD = (0+0) * (1);
static const int H7_unt_mwE = (0+0) * (1);
static const int H7_unt_mwH = (0+0) * (1);
static const int H7_unt_mwI = (255+0) * (1);
static const int H7_unt_mwJ = (0+0) * (1);
static const int H7_unt_mwK = (0+0) * (1);
static const int H7_unt_mwL = (212+0) * (1);
static const int H7_unt_mwT = (255+0) * (1);
static const int H7_unt_mwG = (0+0) * (1);
static const int H7_unt_mt = (2+0) * (1);
static const int H7_unt_mb = (255+0) * (1);
static const int H7_unt_hA = (24+0) * (1);
static const int H7_unt_hB = (24+0) * (1);
static const int H7_unt_hC = (24+0) * (1);
static const int H7_unt_hD = (24+0) * (1);
static const int H7_unt_hE = (24+0) * (1);
static const int H7_unt_hH = (24+0) * (1);
static const int H7_unt_hI = (24+0) * (1);
static const int H7_unt_hJ = (24+0) * (1);
static const int H7_unt_hK = (24+0) * (1);
static const int H7_unt_hL = (24+0) * (1);
static const int H7_unt_hS = (24+0) * (1);
static const int H7_unt_hM = (24+0) * (1);
static const int H7_unt_hT = (24+0) * (1);


static const long long H7_unt_a_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H7_unt_b_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H7_unt_c_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H7_unt_d_Volume = tc_vmul(1, 16); // 16
static const long long H7_unt_e_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H7_unt_g_Volume = 1; // 1
static const long long H7_unt_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 16), 1), 1); // 16
static const long long H7_unt_i_Volume = tc_vmul(tc_vmul(1, 16), 7); // 112
static const long long H7_unt_j_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H7_unt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 8), 3); // 96
static const long long H7_unt_l_Volume = tc_vmul(tc_vmul(1, 7), 2); // 14
static const long long H7_unt_m_Volume = tc_vmul(1, 4); // 4
static const long long H7_unt_s_Volume = tc_vmul(1, 4); // 4
static const long long H7_unt_t_Volume = tc_vmul(1, 16); // 16
static const long long H7_unt_w_Volume = tc_vmul(1, 4); // 4

#define H7_unt_USES(X) X(fld) X(ctx) X(p1) X(p2)

struct H7_unt_T {
  cm_cnt2 H7_unt_A[tc_rows(H7_unt_a_Volume, H7_unt_hA) * H_unt_ND];
  cm_cnt2 H7_unt_B[tc_rows(H7_unt_b_Volume, H7_unt_hB) * H_unt_ND];
  cm_cnt2 H7_unt_C[tc_rows(H7_unt_c_Volume, H7_unt_hC) * H_unt_ND];
  cm_cnt2 H7_unt_D[tc_rows(H7_unt_d_Volume, H7_unt_hD) * H_unt_ND];
  cm_cnt2 H7_unt_E[tc_rows(H7_unt_e_Volume, H7_unt_hE) * H_unt_ND];
  cm_cnt2 H7_unt_H[tc_rows(H7_unt_h_Volume, H7_unt_hH) * H_unt_ND];
  cm_cnt2 H7_unt_I[tc_rows(H7_unt_i_Volume, H7_unt_hI) * H_unt_ND];
  cm_cnt2 H7_unt_J[tc_rows(H7_unt_j_Volume, H7_unt_hJ) * H_unt_ND];
  cm_cnt2 H7_unt_K[tc_rows(H7_unt_k_Volume, H7_unt_hK) * H_unt_ND];
  cm_cnt2 H7_unt_L[tc_rows(H7_unt_l_Volume, H7_unt_hL) * H_unt_ND];
  cm_cnt H7_unt_MT[tc_rows(H7_unt_t_Volume, H7_unt_hT) * H_unt_MND];
  i32 H7_unt_W[tc_rows(H7_unt_m_Volume, H7_unt_hM) * H_unt_ND * tc_nin(10)];
  u8 H7_unt_WC[tc_rows(H7_unt_m_Volume, H7_unt_hM) * H_unt_ND];
  i32 H7_unt_W2[H7_unt_w_Volume * H_unt_ND * tc_nin(10)];
  u8 H7_unt_W2C[H7_unt_w_Volume * H_unt_ND];
  i32 H7_unt_W3[tc_rows(H7_unt_s_Volume, H7_unt_hS) * H_unt_ND * tc_nin(10)];
  u8 H7_unt_W3C[tc_rows(H7_unt_s_Volume, H7_unt_hS) * H_unt_ND];
  i32 H7_unt_WF[H_unt_ND * 4];
  u8 H7_unt_WFC[H_unt_ND];
  cm_cnt H7_unt_G[H7_unt_g_Volume * H_unt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_unt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_a_Volume, H7_unt_hA) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_b_Volume, H7_unt_hB) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_c_Volume, H7_unt_hC) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_d_Volume, H7_unt_hD) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_e_Volume, H7_unt_hE) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_h_Volume, H7_unt_hH) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_i_Volume, H7_unt_hI) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_j_Volume, H7_unt_hJ) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_k_Volume, H7_unt_hK) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_unt_l_Volume, H7_unt_hL) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_unt_t_Volume, H7_unt_hT) * H_unt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_unt_m_Volume, H7_unt_hM) * H_unt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_unt_m_Volume, H7_unt_hM) * H_unt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_unt_w_Volume * H_unt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_unt_w_Volume * H_unt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_unt_s_Volume, H7_unt_hS) * H_unt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_unt_s_Volume, H7_unt_hS) * H_unt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_unt_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_unt_g_Volume * H_unt_NG);
  void H7_unt_Init( void ) {}
  void H7_unt_Quit( void ) {}
};
