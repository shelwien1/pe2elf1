
static const byte H5_mbt_a_cb[8+1]={ 0,1,2,3,4,4,5,6,6 };
static const byte H5_mbt_a_en[14+1]={ 0,0,1,1,2,2,3,3,3,3,4,4,4,4,5 };

static const byte H5_mbt_b_cc[8+1]={ 0,1,2,3,3,4,5,6,6 };
static const byte H5_mbt_b_td[16+1]={ 0,0,1,1,1,1,1,1,1,2,3,3,3,3,3,3,4 };
static const byte H5_mbt_b_c1[9+1]={ 0,0,1,1,2,3,3,4,5,5 };

static const byte H5_mbt_c_ca[8+1]={ 0,1,1,1,1,2,3,4,4 };
static const byte H5_mbt_c_cc[8+1]={ 0,0,0,0,1,1,2,3,3 };

static const byte H5_mbt_d_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };

static const byte H5_mbt_e_ca[8+1]={ 0,1,2,3,3,4,5,6,6 };
static const byte H5_mbt_e_td[16+1]={ 0,0,0,0,0,1,1,1,1,1,2,2,3,3,3,4,4 };



static const byte H5_mbt_i_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };
static const byte H5_mbt_i_en[14+1]={ 0,1,2,2,3,3,3,3,4,4,4,4,5,5,5 };
static const byte H5_mbt_i_ca[8+1]={ 0,1,1,1,2,2,3,4,4 };

static const byte H5_mbt_j_cb[8+1]={ 0,1,1,1,1,2,3,4,4 };

static const byte H5_mbt_k_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };
static const byte H5_mbt_k_nsk[4+1]={ 0,1,2,3,3 };
static const byte H5_mbt_k_cc[8+1]={ 0,0,1,1,2,2,3,4,4 };

static const byte H5_mbt_l_ca[8+1]={ 0,1,2,3,4,5,6,7,7 };
static const byte H5_mbt_l_cb[8+1]={ 0,1,2,3,4,5,6,7,7 };





static const int H5_mbt_rA = (254+0) * (1);
static const int H5_mbt_rB = (231+0) * (1);
static const int H5_mbt_rC = (252+0) * (1);
static const int H5_mbt_rE = (68+0) * (1);
static const int H5_mbt_rH = (133+0) * (1);
static const int H5_mbt_rI = (252+0) * (1);
static const int H5_mbt_rJ = (95+0) * (1);
static const int H5_mbt_rK = (255+0) * (1);
static const int H5_mbt_rL = (255+0) * (1);
static const int H5_mbt_rD = (61+0) * (1);
static const int H5_mbt_lr = (14+0) * (1);
static const int H5_mbt_mwA = (1+0) * (1);
static const int H5_mbt_mwB = (13+0) * (1);
static const int H5_mbt_mwC = (125+0) * (1);
static const int H5_mbt_mwE = (96+0) * (1);
static const int H5_mbt_mwH = (64+0) * (1);
static const int H5_mbt_mwI = (13+0) * (1);
static const int H5_mbt_mwJ = (128+0) * (1);
static const int H5_mbt_mwK = (132+0) * (1);
static const int H5_mbt_mwL = (64+0) * (1);
static const int H5_mbt_mwD = (3+0) * (1);
static const int H5_mbt_mb = (31+0) * (1);
static const int H5_mbt_rT = (0+0) * (1);
static const int H5_mbt_mwT = (0+0) * (1);
static const int H5_mbt_mt = (0+0) * (1);
static const int H5_mbt_lrm = (0+0) * (1);
static const int H5_mbt_mbm = (0+0) * (1);
static const int H5_mbt_rG = (0+0) * (1);
static const int H5_mbt_mwG = (0+0) * (1);
static const int H5_mbt_hA = (24+0) * (1);
static const int H5_mbt_hB = (24+0) * (1);
static const int H5_mbt_hC = (24+0) * (1);
static const int H5_mbt_hE = (15+0) * (1);
static const int H5_mbt_hH = (24+0) * (1);
static const int H5_mbt_hI = (24+0) * (1);
static const int H5_mbt_hJ = (24+0) * (1);
static const int H5_mbt_hK = (24+0) * (1);
static const int H5_mbt_hL = (24+0) * (1);
static const int H5_mbt_hD = (24+0) * (1);
static const int H5_mbt_hM = (24+0) * (1);
static const int H5_mbt_hT = (24+0) * (1);
static const int H5_mbt_hN = (24+0) * (1);


static const long long H5_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 7), 6); // 168
static const long long H5_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 1), 5), 6); // 420
static const long long H5_mbt_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 4); // 20
static const long long H5_mbt_d_Volume = tc_vmul(1, 4); // 4
static const long long H5_mbt_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 5); // 35
static const long long H5_mbt_g_Volume = tc_vmul(1, 1); // 1
static const long long H5_mbt_h_Volume = tc_vmul(tc_vmul(1, 2), 1); // 2
static const long long H5_mbt_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 6), 5); // 120
static const long long H5_mbt_j_Volume = tc_vmul(tc_vmul(1, 1), 5); // 5
static const long long H5_mbt_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 5); // 80
static const long long H5_mbt_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 8); // 128
static const long long H5_mbt_m_Volume = tc_vmul(1, 2); // 2
static const long long H5_mbt_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_mbt_t_Volume = tc_vmul(1, 1); // 1

#define H5_mbt_USES(X) X(st) X(ca) X(cb) X(en) X(cc) X(td) X(c1) X(nsk)

struct H5_mbt_T {
  cm_cnt2 H5_mbt_A[tc_rows(H5_mbt_a_Volume, H5_mbt_hA) * H_mbt_ND];
  cm_cnt2 H5_mbt_B[tc_rows(H5_mbt_b_Volume, H5_mbt_hB) * H_mbt_ND];
  cm_cnt2 H5_mbt_C[tc_rows(H5_mbt_c_Volume, H5_mbt_hC) * H_mbt_ND];
  cm_cnt2 H5_mbt_D[tc_rows(H5_mbt_d_Volume, H5_mbt_hD) * H_mbt_ND];
  cm_cnt2 H5_mbt_E[tc_rows(H5_mbt_e_Volume, H5_mbt_hE) * H_mbt_ND];
  cm_cnt2 H5_mbt_H[tc_rows(H5_mbt_h_Volume, H5_mbt_hH) * H_mbt_ND];
  cm_cnt2 H5_mbt_I[tc_rows(H5_mbt_i_Volume, H5_mbt_hI) * H_mbt_ND];
  cm_cnt2 H5_mbt_J[tc_rows(H5_mbt_j_Volume, H5_mbt_hJ) * H_mbt_ND];
  cm_cnt2 H5_mbt_K[tc_rows(H5_mbt_k_Volume, H5_mbt_hK) * H_mbt_ND];
  cm_cnt2 H5_mbt_L[tc_rows(H5_mbt_l_Volume, H5_mbt_hL) * H_mbt_ND];
  cm_cnt H5_mbt_MT[tc_rows(H5_mbt_t_Volume, H5_mbt_hT) * H_mbt_MND];
  i32 H5_mbt_W[tc_rows(H5_mbt_m_Volume, H5_mbt_hM) * H_mbt_ND * 24];
  u8 H5_mbt_WC[tc_rows(H5_mbt_m_Volume, H5_mbt_hM) * H_mbt_ND];
  i32 H5_mbt_WM[tc_rows(H5_mbt_n_Volume, H5_mbt_hN) * H_mbt_MND * 3];
  u8 H5_mbt_WMC[tc_rows(H5_mbt_n_Volume, H5_mbt_hN) * H_mbt_MND];
  cm_cnt H5_mbt_G[H5_mbt_g_Volume * H_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_a_Volume, H5_mbt_hA) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_b_Volume, H5_mbt_hB) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_c_Volume, H5_mbt_hC) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_d_Volume, H5_mbt_hD) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_e_Volume, H5_mbt_hE) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_h_Volume, H5_mbt_hH) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_i_Volume, H5_mbt_hI) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_j_Volume, H5_mbt_hJ) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_k_Volume, H5_mbt_hK) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mbt_l_Volume, H5_mbt_hL) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_mbt_t_Volume, H5_mbt_hT) * H_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_mbt_m_Volume, H5_mbt_hM) * H_mbt_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_mbt_m_Volume, H5_mbt_hM) * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_mbt_n_Volume, H5_mbt_hN) * H_mbt_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_mbt_n_Volume, H5_mbt_hN) * H_mbt_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_mbt_g_Volume * H_mbt_NG);
  void H5_mbt_Init( void ) {}
  void H5_mbt_Quit( void ) {}
};
