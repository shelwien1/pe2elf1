
static const byte H5_sgn_a_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H5_sgn_a_pos[3+1]={ 0,1,2,3 };
static const byte H5_sgn_a_sa[3+1]={ 0,1,2,3 };
static const byte H5_sgn_a_sb[3+1]={ 0,1,2,3 };
static const byte H5_sgn_a_gh[8+1]={ 0,1,2,2,3,4,5,5,6 };

static const byte H5_sgn_b_cg[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H5_sgn_b_scb[3+1]={ 0,1,2,3 };

static const byte H5_sgn_c_cg[13+1]={ 0,1,1,2,3,3,3,3,3,3,3,3,3,3 };
static const byte H5_sgn_c_md[19+1]={ 0,1,2,3,4,4,5,6,6,6,6,6,7,7,8,8,9,10,10,11 };

static const byte H5_sgn_d_cg[13+1]={ 0,1,2,3,3,4,4,4,4,4,4,4,4,4 };

static const byte H5_sgn_e_cg[13+1]={ 0,1,2,3,3,4,4,4,4,4,4,4,4,4 };


static const byte H5_sgn_h_cg[13+1]={ 0,0,0,1,2,3,3,3,3,3,3,3,3,3 };
static const byte H5_sgn_h_ts[6+1]={ 0,1,2,3,4,5,6 };
static const byte H5_sgn_h_dm[6+1]={ 0,0,1,2,3,4,5 };

static const byte H5_sgn_i_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };

static const byte H5_sgn_j_sa[3+1]={ 0,1,2,3 };
static const byte H5_sgn_j_sb[3+1]={ 0,1,2,3 };
static const byte H5_sgn_j_pos[3+1]={ 0,1,2,3 };


static const byte H5_sgn_l_cg[13+1]={ 0,0,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H5_sgn_l_ts[6+1]={ 0,1,2,3,4,5,6 };
static const byte H5_sgn_l_gh[8+1]={ 0,1,2,2,3,4,4,5,6 };

static const byte H5_sgn_m_cg[13+1]={ 0,1,2,3,3,4,4,4,4,4,4,4,4,4 };




static const int H5_sgn_rA = (253+0) * (1);
static const int H5_sgn_rB = (255+0) * (1);
static const int H5_sgn_rC = (39+0) * (1);
static const int H5_sgn_rE = (0+0) * (1);
static const int H5_sgn_rH = (217+0) * (1);
static const int H5_sgn_rI = (64+0) * (1);
static const int H5_sgn_rJ = (255+0) * (1);
static const int H5_sgn_rK = (64+0) * (1);
static const int H5_sgn_rL = (253+0) * (1);
static const int H5_sgn_rD = (254+0) * (1);
static const int H5_sgn_rT = (0+0) * (1);
static const int H5_sgn_rG = (0+0) * (1);
static const int H5_sgn_lr = (15+0) * (1);
static const int H5_sgn_lrm = (0+0) * (1);
static const int H5_sgn_mbm = (0+0) * (1);
static const int H5_sgn_mwA = (16+0) * (1);
static const int H5_sgn_mwB = (4+0) * (1);
static const int H5_sgn_mwC = (64+0) * (1);
static const int H5_sgn_mwE = (245+0) * (1);
static const int H5_sgn_mwH = (8+0) * (1);
static const int H5_sgn_mwI = (0+0) * (1);
static const int H5_sgn_mwJ = (9+0) * (1);
static const int H5_sgn_mwK = (193+0) * (1);
static const int H5_sgn_mwL = (4+0) * (1);
static const int H5_sgn_mwD = (254+0) * (1);
static const int H5_sgn_mwT = (0+0) * (1);
static const int H5_sgn_mwG = (0+0) * (1);
static const int H5_sgn_mt = (0+0) * (1);
static const int H5_sgn_mb = (128+0) * (1);
static const int H5_sgn_hA = (24+0) * (1);
static const int H5_sgn_hB = (24+0) * (1);
static const int H5_sgn_hC = (24+0) * (1);
static const int H5_sgn_hE = (24+0) * (1);
static const int H5_sgn_hH = (24+0) * (1);
static const int H5_sgn_hI = (24+0) * (1);
static const int H5_sgn_hJ = (24+0) * (1);
static const int H5_sgn_hK = (24+0) * (1);
static const int H5_sgn_hL = (24+0) * (1);
static const int H5_sgn_hD = (24+0) * (1);
static const int H5_sgn_hM = (24+0) * (1);
static const int H5_sgn_hT = (24+0) * (1);
static const int H5_sgn_hN = (24+0) * (1);


static const long long H5_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 4), 4), 4), 7); // 2688
static const long long H5_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 4), 2); // 40
static const long long H5_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 12); // 144
static const long long H5_sgn_d_Volume = tc_vmul(1, 5); // 5
static const long long H5_sgn_e_Volume = tc_vmul(tc_vmul(1, 5), 1); // 5
static const long long H5_sgn_g_Volume = tc_vmul(1, 1); // 1
static const long long H5_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 7), 6); // 336
static const long long H5_sgn_i_Volume = tc_vmul(tc_vmul(1, 6), 1); // 6
static const long long H5_sgn_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 4), 1), 4); // 192
static const long long H5_sgn_k_Volume = tc_vmul(1, 1); // 1
static const long long H5_sgn_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 2), 7), 7); // 490
static const long long H5_sgn_m_Volume = tc_vmul(1, 5); // 5
static const long long H5_sgn_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_sgn_t_Volume = tc_vmul(1, 1); // 1

#define H5_sgn_USES(X) X(cg) X(pos) X(sa) X(sb) X(gh) X(scb) X(mag) X(md) X(ts) X(dm)

struct H5_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


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
  i32 H5_sgn_W[tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND * 24];
  u8 H5_sgn_WC[tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND];
  i32 H5_sgn_WM[tc_rows(H5_sgn_n_Volume, H5_sgn_hN) * H_sgn_MND * 3];
  u8 H5_sgn_WMC[tc_rows(H5_sgn_n_Volume, H5_sgn_hN) * H_sgn_MND];
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
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_sgn_m_Volume, H5_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_sgn_n_Volume, H5_sgn_hN) * H_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_sgn_n_Volume, H5_sgn_hN) * H_sgn_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_sgn_g_Volume * H_sgn_NG);
  void H5_sgn_Init( void ) {
  
  }
  void H5_sgn_Quit( void ) {}
};
