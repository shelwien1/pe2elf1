
static const byte H5_ipm_a_ma[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H5_ipm_a_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H5_ipm_a_mc[10+1]={ 0,1,2,3,4,4,5,6,7,8,9 };

static const byte H5_ipm_b_mp[9+1]={ 0,1,1,1,2,2,3,4,5,5 };
static const byte H5_ipm_b_pv[9+1]={ 0,1,2,3,4,5,6,7,8,8 };
static const byte H5_ipm_b_mh[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H5_ipm_c_mp[9+1]={ 0,1,2,3,4,4,5,6,7,7 };
static const byte H5_ipm_c_col[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H5_ipm_d_mp[9+1]={ 0,1,2,3,4,4,5,6,7,7 };
static const byte H5_ipm_d_dm[9+1]={ 0,1,1,2,3,4,5,6,7,7 };
static const byte H5_ipm_d_ds[9+1]={ 0,0,1,2,2,2,2,2,3,3 };



static const byte H5_ipm_h_dr[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H5_ipm_h_ds[9+1]={ 0,0,0,0,1,1,2,2,2,3 };

static const byte H5_ipm_i_dm[9+1]={ 0,1,2,2,2,2,2,2,2,3 };
static const byte H5_ipm_i_ma[10+1]={ 0,0,0,1,2,3,4,5,6,7,8 };

static const byte H5_ipm_j_mh[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H5_ipm_j_mb[10+1]={ 0,1,2,3,4,5,6,7,8,8,9 };

static const byte H5_ipm_k_pv[9+1]={ 0,0,1,2,3,3,3,4,4,5 };

static const byte H5_ipm_l_mc[10+1]={ 0,1,2,3,4,4,5,6,7,8,9 };
static const byte H5_ipm_l_ma[10+1]={ 0,1,2,3,4,4,5,6,7,8,8 };
static const byte H5_ipm_l_dm[9+1]={ 0,1,1,2,3,3,4,5,6,7 };





static const int H5_ipm_rA = (255+0) * (1);
static const int H5_ipm_rB = (237+0) * (1);
static const int H5_ipm_rC = (254+0) * (1);
static const int H5_ipm_rE = (254+0) * (1);
static const int H5_ipm_rH = (252+0) * (1);
static const int H5_ipm_rI = (255+0) * (1);
static const int H5_ipm_rJ = (254+0) * (1);
static const int H5_ipm_rK = (255+0) * (1);
static const int H5_ipm_rL = (254+0) * (1);
static const int H5_ipm_rD = (254+0) * (1);
static const int H5_ipm_rT = (0+0) * (1);
static const int H5_ipm_rG = (0+0) * (1);
static const int H5_ipm_lr = (15+0) * (1);
static const int H5_ipm_lrm = (0+0) * (1);
static const int H5_ipm_mbm = (0+0) * (1);
static const int H5_ipm_mwA = (32+0) * (1);
static const int H5_ipm_mwB = (128+0) * (1);
static const int H5_ipm_mwC = (2+0) * (1);
static const int H5_ipm_mwE = (8+0) * (1);
static const int H5_ipm_mwH = (15+0) * (1);
static const int H5_ipm_mwI = (68+0) * (1);
static const int H5_ipm_mwJ = (95+0) * (1);
static const int H5_ipm_mwK = (94+0) * (1);
static const int H5_ipm_mwL = (8+0) * (1);
static const int H5_ipm_mwD = (32+0) * (1);
static const int H5_ipm_mwT = (0+0) * (1);
static const int H5_ipm_mwG = (0+0) * (1);
static const int H5_ipm_mt = (0+0) * (1);
static const int H5_ipm_mb = (22+0) * (1);
static const int H5_ipm_hA = (24+0) * (1);
static const int H5_ipm_hB = (24+0) * (1);
static const int H5_ipm_hC = (24+0) * (1);
static const int H5_ipm_hE = (24+0) * (1);
static const int H5_ipm_hH = (24+0) * (1);
static const int H5_ipm_hI = (24+0) * (1);
static const int H5_ipm_hJ = (24+0) * (1);
static const int H5_ipm_hK = (24+0) * (1);
static const int H5_ipm_hL = (24+0) * (1);
static const int H5_ipm_hD = (24+0) * (1);
static const int H5_ipm_hM = (24+0) * (1);
static const int H5_ipm_hT = (24+0) * (1);
static const int H5_ipm_hN = (24+0) * (1);


static const long long H5_ipm_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 11), 10); // 2420
static const long long H5_ipm_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 9), 17); // 1836
static const long long H5_ipm_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 10); // 80
static const long long H5_ipm_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 8), 4); // 512
static const long long H5_ipm_e_Volume = tc_vmul(tc_vmul(1, 1), 2); // 2
static const long long H5_ipm_g_Volume = tc_vmul(1, 1); // 1
static const long long H5_ipm_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 9), 4); // 36
static const long long H5_ipm_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 9); // 36
static const long long H5_ipm_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 17), 10), 1); // 340
static const long long H5_ipm_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 6); // 12
static const long long H5_ipm_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 10), 9), 8); // 720
static const long long H5_ipm_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long H5_ipm_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_ipm_t_Volume = tc_vmul(1, 1); // 1

#define H5_ipm_USES(X) X(k8) X(ma) X(mb) X(mc) X(mp) X(pv) X(mh) X(col) X(dm) X(ds) X(dr)

struct H5_ipm_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H5_ipm_A[tc_rows(H5_ipm_a_Volume, H5_ipm_hA) * H_ipm_ND];
  cm_cnt2 H5_ipm_B[tc_rows(H5_ipm_b_Volume, H5_ipm_hB) * H_ipm_ND];
  cm_cnt2 H5_ipm_C[tc_rows(H5_ipm_c_Volume, H5_ipm_hC) * H_ipm_ND];
  cm_cnt2 H5_ipm_D[tc_rows(H5_ipm_d_Volume, H5_ipm_hD) * H_ipm_ND];
  cm_cnt2 H5_ipm_E[tc_rows(H5_ipm_e_Volume, H5_ipm_hE) * H_ipm_ND];
  cm_cnt2 H5_ipm_H[tc_rows(H5_ipm_h_Volume, H5_ipm_hH) * H_ipm_ND];
  cm_cnt2 H5_ipm_I[tc_rows(H5_ipm_i_Volume, H5_ipm_hI) * H_ipm_ND];
  cm_cnt2 H5_ipm_J[tc_rows(H5_ipm_j_Volume, H5_ipm_hJ) * H_ipm_ND];
  cm_cnt2 H5_ipm_K[tc_rows(H5_ipm_k_Volume, H5_ipm_hK) * H_ipm_ND];
  cm_cnt2 H5_ipm_L[tc_rows(H5_ipm_l_Volume, H5_ipm_hL) * H_ipm_ND];
  cm_cnt H5_ipm_MT[tc_rows(H5_ipm_t_Volume, H5_ipm_hT) * H_ipm_MND];
  i32 H5_ipm_W[tc_rows(H5_ipm_m_Volume, H5_ipm_hM) * H_ipm_ND * 24];
  u8 H5_ipm_WC[tc_rows(H5_ipm_m_Volume, H5_ipm_hM) * H_ipm_ND];
  i32 H5_ipm_WM[tc_rows(H5_ipm_n_Volume, H5_ipm_hN) * H_ipm_MND * 3];
  u8 H5_ipm_WMC[tc_rows(H5_ipm_n_Volume, H5_ipm_hN) * H_ipm_MND];
  cm_cnt H5_ipm_G[H5_ipm_g_Volume * H_ipm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_ipm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_a_Volume, H5_ipm_hA) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_b_Volume, H5_ipm_hB) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_c_Volume, H5_ipm_hC) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_d_Volume, H5_ipm_hD) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_e_Volume, H5_ipm_hE) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_h_Volume, H5_ipm_hH) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_i_Volume, H5_ipm_hI) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_j_Volume, H5_ipm_hJ) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_k_Volume, H5_ipm_hK) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_ipm_l_Volume, H5_ipm_hL) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_ipm_t_Volume, H5_ipm_hT) * H_ipm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_ipm_m_Volume, H5_ipm_hM) * H_ipm_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_ipm_m_Volume, H5_ipm_hM) * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_ipm_n_Volume, H5_ipm_hN) * H_ipm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_ipm_n_Volume, H5_ipm_hN) * H_ipm_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_ipm_g_Volume * H_ipm_NG);
  void H5_ipm_Init( void ) {
  
  }
};
