
static const byte H5_unt_a_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };


static const byte H5_unt_c_fld[15+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H5_unt_d_fld[15+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H5_unt_e_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };



static const byte H5_unt_i_ctx[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H5_unt_i_p1[6+1]={ 0,1,2,3,4,5,6 };


static const byte H5_unt_k_ctx[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H5_unt_m_fld[15+1]={ 0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H5_unt_t_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int H5_unt_rA = (2+0) * (1);
static const int H5_unt_rB = (0+0) * (1);
static const int H5_unt_rC = (160+0) * (1);
static const int H5_unt_rD = (192+0) * (1);
static const int H5_unt_rE = (208+0) * (1);
static const int H5_unt_rH = (0+0) * (1);
static const int H5_unt_rI = (192+0) * (1);
static const int H5_unt_rJ = (12+0) * (1);
static const int H5_unt_rK = (0+0) * (1);
static const int H5_unt_rL = (108+0) * (1);
static const int H5_unt_rT = (128+0) * (1);
static const int H5_unt_rG = (64+0) * (1);
static const int H5_unt_lr = (31+0) * (1);
static const int H5_unt_mwA = (32+0) * (1);
static const int H5_unt_mwB = (15+0) * (1);
static const int H5_unt_mwC = (5+0) * (1);
static const int H5_unt_mwD = (18+0) * (1);
static const int H5_unt_mwE = (0+0) * (1);
static const int H5_unt_mwH = (10+0) * (1);
static const int H5_unt_mwI = (1+0) * (1);
static const int H5_unt_mwJ = (22+0) * (1);
static const int H5_unt_mwK = (32+0) * (1);
static const int H5_unt_mwL = (3+0) * (1);
static const int H5_unt_mwT = (255+0) * (1);
static const int H5_unt_mwG = (0+0) * (1);
static const int H5_unt_mt = (2+0) * (1);
static const int H5_unt_mb = (255+0) * (1);
static const int H5_unt_hA = (24+0) * (1);
static const int H5_unt_hB = (24+0) * (1);
static const int H5_unt_hC = (24+0) * (1);
static const int H5_unt_hD = (24+0) * (1);
static const int H5_unt_hE = (24+0) * (1);
static const int H5_unt_hH = (24+0) * (1);
static const int H5_unt_hI = (24+0) * (1);
static const int H5_unt_hJ = (24+0) * (1);
static const int H5_unt_hK = (24+0) * (1);
static const int H5_unt_hL = (24+0) * (1);
static const int H5_unt_hM = (24+0) * (1);
static const int H5_unt_hT = (24+0) * (1);


static const long long H5_unt_a_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H5_unt_b_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H5_unt_c_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H5_unt_d_Volume = tc_vmul(1, 4); // 4
static const long long H5_unt_e_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H5_unt_g_Volume = 1; // 1
static const long long H5_unt_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 1); // 3
static const long long H5_unt_i_Volume = tc_vmul(tc_vmul(1, 16), 7); // 112
static const long long H5_unt_j_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H5_unt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 3); // 72
static const long long H5_unt_l_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long H5_unt_m_Volume = tc_vmul(1, 4); // 4
static const long long H5_unt_t_Volume = tc_vmul(1, 16); // 16

#define H5_unt_USES(X) X(fld) X(ctx) X(p1)

struct H5_unt_T {
  cm_cnt2 H5_unt_A[tc_rows(H5_unt_a_Volume, H5_unt_hA) * H_unt_ND];
  cm_cnt2 H5_unt_B[tc_rows(H5_unt_b_Volume, H5_unt_hB) * H_unt_ND];
  cm_cnt2 H5_unt_C[tc_rows(H5_unt_c_Volume, H5_unt_hC) * H_unt_ND];
  cm_cnt2 H5_unt_D[tc_rows(H5_unt_d_Volume, H5_unt_hD) * H_unt_ND];
  cm_cnt2 H5_unt_E[tc_rows(H5_unt_e_Volume, H5_unt_hE) * H_unt_ND];
  cm_cnt2 H5_unt_H[tc_rows(H5_unt_h_Volume, H5_unt_hH) * H_unt_ND];
  cm_cnt2 H5_unt_I[tc_rows(H5_unt_i_Volume, H5_unt_hI) * H_unt_ND];
  cm_cnt2 H5_unt_J[tc_rows(H5_unt_j_Volume, H5_unt_hJ) * H_unt_ND];
  cm_cnt2 H5_unt_K[tc_rows(H5_unt_k_Volume, H5_unt_hK) * H_unt_ND];
  cm_cnt2 H5_unt_L[tc_rows(H5_unt_l_Volume, H5_unt_hL) * H_unt_ND];
  cm_cnt H5_unt_MT[tc_rows(H5_unt_t_Volume, H5_unt_hT) * H_unt_MND];
  i32 H5_unt_W[tc_rows(H5_unt_m_Volume, H5_unt_hM) * H_unt_ND * tc_nin(10)];
  u8 H5_unt_WC[tc_rows(H5_unt_m_Volume, H5_unt_hM) * H_unt_ND];
  cm_cnt H5_unt_G[H5_unt_g_Volume * H_unt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_unt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_a_Volume, H5_unt_hA) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_b_Volume, H5_unt_hB) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_c_Volume, H5_unt_hC) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_d_Volume, H5_unt_hD) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_e_Volume, H5_unt_hE) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_h_Volume, H5_unt_hH) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_i_Volume, H5_unt_hI) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_j_Volume, H5_unt_hJ) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_k_Volume, H5_unt_hK) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_unt_l_Volume, H5_unt_hL) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_unt_t_Volume, H5_unt_hT) * H_unt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_unt_m_Volume, H5_unt_hM) * H_unt_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_unt_m_Volume, H5_unt_hM) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_unt_g_Volume * H_unt_NG);
  void H5_unt_Init( void ) {}
  void H5_unt_Quit( void ) {}
};
