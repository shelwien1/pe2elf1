
static const byte H5_sgn_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H5_sgn_b_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H5_sgn_c_mg[7+1]={ 0,0,1,2,3,3,3,3 };

static const byte H5_sgn_d_mg[4+1]={ 0,1,2,3,4 };

static const byte H5_sgn_e_kq[19+1]={ 0,0,0,1,2,3,4,4,5,6,7,7,7,7,7,7,8,8,9,10 };


static const byte H5_sgn_h_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H5_sgn_h_kq[6+1]={ 0,1,1,2,3,3,3 };


static const byte H5_sgn_j_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const byte H5_sgn_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const int H5_sgn_rA = (48+0) * (1);
static const int H5_sgn_rB = (17+0) * (1);
static const int H5_sgn_rC = (253+0) * (1);
static const int H5_sgn_rD = (145+0) * (1);
static const int H5_sgn_rE = (255+0) * (1);
static const int H5_sgn_rH = (128+0) * (1);
static const int H5_sgn_rI = (40+0) * (1);
static const int H5_sgn_rJ = (255+0) * (1);
static const int H5_sgn_rK = (66+0) * (1);
static const int H5_sgn_rL = (131+0) * (1);
static const int H5_sgn_lr = (25+0) * (1);
static const int H5_sgn_mwA = (103+0) * (1);
static const int H5_sgn_mwB = (4+0) * (1);
static const int H5_sgn_mwC = (210+0) * (1);
static const int H5_sgn_mwD = (28+0) * (1);
static const int H5_sgn_mwE = (191+0) * (1);
static const int H5_sgn_mwH = (234+0) * (1);
static const int H5_sgn_mwI = (1+0) * (1);
static const int H5_sgn_mwJ = (245+0) * (1);
static const int H5_sgn_mwK = (4+0) * (1);
static const int H5_sgn_mwL = (10+0) * (1);
static const int H5_sgn_mb = (32+0) * (1);
static const int H5_sgn_rT = (0+0) * (1);
static const int H5_sgn_mwT = (0+0) * (1);
static const int H5_sgn_mt = (0+0) * (1);
static const int H5_sgn_rG = (0+0) * (1);
static const int H5_sgn_mwG = (0+0) * (1);
static const int H5_sgn_hA = (24+0) * (1);
static const int H5_sgn_hB = (24+0) * (1);
static const int H5_sgn_hC = (24+0) * (1);
static const int H5_sgn_hD = (24+0) * (1);
static const int H5_sgn_hE = (24+0) * (1);
static const int H5_sgn_hH = (24+0) * (1);
static const int H5_sgn_hI = (24+0) * (1);
static const int H5_sgn_hJ = (24+0) * (1);
static const int H5_sgn_hK = (24+0) * (1);
static const int H5_sgn_hL = (24+0) * (1);
static const int H5_sgn_hM = (24+0) * (1);
static const int H5_sgn_hT = (24+0) * (1);


static const long long H5_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 2), 2); // 48
static const long long H5_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 12); // 36
static const long long H5_sgn_c_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long H5_sgn_d_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long H5_sgn_e_Volume = tc_vmul(1, 11); // 11
static const long long H5_sgn_g_Volume = 1; // 1
static const long long H5_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 3), 4); // 144
static const long long H5_sgn_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 1); // 3
static const long long H5_sgn_j_Volume = tc_vmul(tc_vmul(1, 1), 12); // 12
static const long long H5_sgn_k_Volume = tc_vmul(tc_vmul(1, 3), 3); // 9
static const long long H5_sgn_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3); // 18
static const long long H5_sgn_m_Volume = tc_vmul(1, 12); // 12
static const long long H5_sgn_t_Volume = 1; // 1

#define H5_sgn_USES(X) X(cb) X(oc) X(ms) X(s2) X(pf) X(mg) X(kq) X(s1)

struct H5_sgn_T {
  cm_cnt2 H5_sgn_A[tc_rows(H5_sgn_a_Volume, H5_sgn_hA) * H_sgn_ND];
  cm_cnt2 H5_sgn_B[tc_rows(H5_sgn_b_Volume, H5_sgn_hB) * H_sgn_ND];
  cm_cnt2 H5_sgn_C[tc_rows(H5_sgn_c_Volume, H5_sgn_hC) * H_sgn_ND];
  cm_cnt2 H5_sgn_D[tc_rows(H5_sgn_d_Volume, H5_sgn_hD) * H_sgn_ND];
  cm_cnt2 H5_sgn_E[tc_rows(H5_sgn_e_Volume, H5_sgn_hE) * H_sgn_ND];
  cm_cnt2 H5_sgn_H[tc_rows(H5_sgn_h_Volume, H5_sgn_hH) * H_sgn_ND];
  cm_cnt2 H5_sgn_I[tc_rows(H5_sgn_i_Volume, H5_sgn_hI) * H_sgn_ND];
  cm_cnt2 H5_sgn_J[tc_rows(H5_sgn_j_Volume, H5_sgn_hJ) * H_sgn_ND];
  cm_cnt2 H5_sgn_K[tc_rows(H5_sgn_k_Volume, H5_sgn_hK) * H_sgn_ND];
  cm_cnt2 H5_sgn_L[tc_rows(H5_sgn_l_Volume, H5_sgn_hL) * H_sgn_ND];
  cm_cnt H5_sgn_MT[tc_rows(H5_sgn_t_Volume, H5_sgn_hT) * H_sgn_MND];
  i32 H5_sgn_W[tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND * tc_nin(10)];
  u8 H5_sgn_WC[tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND];
  cm_cnt H5_sgn_G[H5_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_a_Volume, H5_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_b_Volume, H5_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_c_Volume, H5_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_d_Volume, H5_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_e_Volume, H5_sgn_hE) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_h_Volume, H5_sgn_hH) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_i_Volume, H5_sgn_hI) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_j_Volume, H5_sgn_hJ) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_k_Volume, H5_sgn_hK) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_sgn_l_Volume, H5_sgn_hL) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_sgn_t_Volume, H5_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_sgn_g_Volume * H_sgn_NG);
  void H5_sgn_Init( void ) {}
  void H5_sgn_Quit( void ) {}
};
