
static const byte H5_mvs_a_ma[7+1]={ 0,0,1,2,3,3,3,3 };
static const byte H5_mvs_a_mb[7+1]={ 0,1,2,3,3,3,3,3 };
static const byte H5_mvs_a_oth[5+1]={ 0,1,2,2,3,4 };
static const byte H5_mvs_a_sp[8+1]={ 0,0,1,1,2,2,2,2,3 };
static const byte H5_mvs_a_rt[14+1]={ 0,0,1,1,1,1,1,1,1,2,2,2,3,3,4 };

static const byte H5_mvs_b_ps[3+1]={ 0,1,2,3 };
static const byte H5_mvs_b_rf[3+1]={ 0,1,2,3 };
static const byte H5_mvs_b_zd[12+1]={ 0,1,1,2,3,4,5,6,7,7,7,7,8 };

static const byte H5_mvs_c_pd[8+1]={ 0,1,2,3,3,4,5,5,6 };
static const byte H5_mvs_c_sp[8+1]={ 0,1,1,1,2,2,3,3,4 };
static const byte H5_mvs_c_zd[12+1]={ 0,1,1,2,3,3,3,4,4,5,6,6,6 };

static const byte H5_mvs_d_pd[8+1]={ 0,1,2,3,4,4,5,5,6 };
static const byte H5_mvs_d_nr[3+1]={ 0,1,2,3 };

static const byte H5_mvs_e_rt[14+1]={ 0,1,2,2,2,3,4,5,5,5,6,7,7,8,9 };


static const byte H5_mvs_h_oth[5+1]={ 0,1,2,3,3,4 };


static const byte H5_mvs_j_kind[3+1]={ 0,1,2,3 };
static const byte H5_mvs_j_ma[7+1]={ 0,1,2,3,4,5,5,6 };
static const byte H5_mvs_j_mb[7+1]={ 0,1,2,3,4,5,5,5 };
static const byte H5_mvs_j_cq[9+1]={ 0,1,2,3,3,3,4,4,4,4 };

static const byte H5_mvs_k_pd[8+1]={ 0,1,2,3,4,5,5,6,6 };
static const byte H5_mvs_k_sp[8+1]={ 0,0,1,2,2,3,3,4,4 };
static const byte H5_mvs_k_ps[3+1]={ 0,1,2,3 };

static const byte H5_mvs_l_kind[3+1]={ 0,1,2,3 };

static const byte H5_mvs_m_kind[3+1]={ 0,1,2,3 };




static const int H5_mvs_rA = (255+0) * (1);
static const int H5_mvs_rB = (0+0) * (1);
static const int H5_mvs_rC = (248+0) * (1);
static const int H5_mvs_rE = (255+0) * (1);
static const int H5_mvs_rH = (48+0) * (1);
static const int H5_mvs_rI = (191+0) * (1);
static const int H5_mvs_rJ = (249+0) * (1);
static const int H5_mvs_rK = (246+0) * (1);
static const int H5_mvs_rL = (128+0) * (1);
static const int H5_mvs_rD = (255+0) * (1);
static const int H5_mvs_rT = (28+0) * (1);
static const int H5_mvs_rG = (0+0) * (1);
static const int H5_mvs_lr = (12+0) * (1);
static const int H5_mvs_lrm = (0+0) * (1);
static const int H5_mvs_mbm = (0+0) * (1);
static const int H5_mvs_mwA = (2+0) * (1);
static const int H5_mvs_mwB = (63+0) * (1);
static const int H5_mvs_mwC = (32+0) * (1);
static const int H5_mvs_mwE = (16+0) * (1);
static const int H5_mvs_mwH = (65+0) * (1);
static const int H5_mvs_mwI = (7+0) * (1);
static const int H5_mvs_mwJ = (95+0) * (1);
static const int H5_mvs_mwK = (63+0) * (1);
static const int H5_mvs_mwL = (6+0) * (1);
static const int H5_mvs_mwD = (16+0) * (1);
static const int H5_mvs_mwT = (59+0) * (1);
static const int H5_mvs_mwG = (0+0) * (1);
static const int H5_mvs_mt = (6+0) * (1);
static const int H5_mvs_mb = (23+0) * (1);
static const int H5_mvs_hA = (16+0) * (1);
static const int H5_mvs_hB = (24+0) * (1);
static const int H5_mvs_hC = (24+0) * (1);
static const int H5_mvs_hE = (24+0) * (1);
static const int H5_mvs_hH = (16+0) * (1);
static const int H5_mvs_hI = (24+0) * (1);
static const int H5_mvs_hJ = (24+0) * (1);
static const int H5_mvs_hK = (24+0) * (1);
static const int H5_mvs_hL = (24+0) * (1);
static const int H5_mvs_hD = (24+0) * (1);
static const int H5_mvs_hM = (24+0) * (1);
static const int H5_mvs_hT = (24+0) * (1);
static const int H5_mvs_hN = (24+0) * (1);


static const long long H5_mvs_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 4), 4), 5), 4), 5); // 9600
static const long long H5_mvs_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2), 4), 4), 9); // 1152
static const long long H5_mvs_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 7), 5), 7); // 735
static const long long H5_mvs_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 7), 1), 4); // 112
static const long long H5_mvs_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 2), 10); // 40
static const long long H5_mvs_g_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1); // 1
static const long long H5_mvs_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 5); // 15
static const long long H5_mvs_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3); // 18
static const long long H5_mvs_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 7), 6), 5); // 1680
static const long long H5_mvs_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 7), 5), 4); // 420
static const long long H5_mvs_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 1); // 4
static const long long H5_mvs_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1); // 4
static const long long H5_mvs_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_mvs_t_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4

#define H5_mvs_USES(X) X(kind) X(comp) X(ma) X(mb) X(oth) X(sp) X(rt) X(lst) X(ps) X(rf) X(zd) X(pd) X(nr) X(cq)

struct H5_mvs_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H5_mvs_A[tc_rows(H5_mvs_a_Volume, H5_mvs_hA) * H_mvs_ND];
  cm_cnt2 H5_mvs_B[tc_rows(H5_mvs_b_Volume, H5_mvs_hB) * H_mvs_ND];
  cm_cnt2 H5_mvs_C[tc_rows(H5_mvs_c_Volume, H5_mvs_hC) * H_mvs_ND];
  cm_cnt2 H5_mvs_D[tc_rows(H5_mvs_d_Volume, H5_mvs_hD) * H_mvs_ND];
  cm_cnt2 H5_mvs_E[tc_rows(H5_mvs_e_Volume, H5_mvs_hE) * H_mvs_ND];
  cm_cnt2 H5_mvs_H[tc_rows(H5_mvs_h_Volume, H5_mvs_hH) * H_mvs_ND];
  cm_cnt2 H5_mvs_I[tc_rows(H5_mvs_i_Volume, H5_mvs_hI) * H_mvs_ND];
  cm_cnt2 H5_mvs_J[tc_rows(H5_mvs_j_Volume, H5_mvs_hJ) * H_mvs_ND];
  cm_cnt2 H5_mvs_K[tc_rows(H5_mvs_k_Volume, H5_mvs_hK) * H_mvs_ND];
  cm_cnt2 H5_mvs_L[tc_rows(H5_mvs_l_Volume, H5_mvs_hL) * H_mvs_ND];
  cm_cnt H5_mvs_MT[tc_rows(H5_mvs_t_Volume, H5_mvs_hT) * H_mvs_MND];
  i32 H5_mvs_W[tc_rows(H5_mvs_m_Volume, H5_mvs_hM) * H_mvs_ND * 24];
  u8 H5_mvs_WC[tc_rows(H5_mvs_m_Volume, H5_mvs_hM) * H_mvs_ND];
  i32 H5_mvs_WM[tc_rows(H5_mvs_n_Volume, H5_mvs_hN) * H_mvs_MND * 3];
  u8 H5_mvs_WMC[tc_rows(H5_mvs_n_Volume, H5_mvs_hN) * H_mvs_MND];
  cm_cnt H5_mvs_G[H5_mvs_g_Volume * H_mvs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_mvs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_a_Volume, H5_mvs_hA) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_b_Volume, H5_mvs_hB) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_c_Volume, H5_mvs_hC) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_d_Volume, H5_mvs_hD) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_e_Volume, H5_mvs_hE) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_h_Volume, H5_mvs_hH) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_i_Volume, H5_mvs_hI) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_j_Volume, H5_mvs_hJ) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_k_Volume, H5_mvs_hK) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_mvs_l_Volume, H5_mvs_hL) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_mvs_t_Volume, H5_mvs_hT) * H_mvs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_mvs_m_Volume, H5_mvs_hM) * H_mvs_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_mvs_m_Volume, H5_mvs_hM) * H_mvs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_mvs_n_Volume, H5_mvs_hN) * H_mvs_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_mvs_n_Volume, H5_mvs_hN) * H_mvs_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_mvs_g_Volume * H_mvs_NG);
  void H5_mvs_Init( void ) {
  
  }
};
