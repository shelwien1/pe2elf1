
static const byte H5_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H5_cbp_a_cls[8+1]={ 0,0,0,1,2,3,4,4,4 };

static const byte H5_cbp_b_col[12+1]={ 0,0,1,2,3,3,3,3,4,5,5,5,5 };

static const byte H5_cbp_c_col[12+1]={ 0,0,1,2,3,3,3,3,4,5,5,5,5 };
static const byte H5_cbp_c_pg[16+1]={ 0,0,0,0,0,1,1,1,1,2,2,2,3,3,3,4,5 };

static const byte H5_cbp_d_inc[3+1]={ 0,1,2,3 };
static const byte H5_cbp_d_em[16+1]={ 0,1,2,3,3,4,4,5,5,6,6,6,7,7,8,8,9 };

static const byte H5_cbp_e_pg[16+1]={ 0,1,1,1,2,2,3,3,3,3,3,4,4,4,5,5,5 };
static const byte H5_cbp_e_tp[16+1]={ 0,1,1,1,1,2,2,2,2,3,3,4,5,5,5,5,5 };


static const byte H5_cbp_h_td[16+1]={ 0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4 };
static const byte H5_cbp_h_en[14+1]={ 0,1,1,1,1,2,2,2,3,3,3,3,4,4,4 };
static const byte H5_cbp_h_q6[8+1]={ 0,0,1,2,3,4,5,5,5 };


static const byte H5_cbp_j_tp[16+1]={ 0,1,2,2,2,2,2,3,3,3,4,4,4,5,5,5,5 };
static const byte H5_cbp_j_em[16+1]={ 0,1,2,2,3,3,4,4,5,5,6,7,7,8,8,8,8 };
static const byte H5_cbp_j_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };

static const byte H5_cbp_k_inc[3+1]={ 0,1,2,3 };
static const byte H5_cbp_k_pg[16+1]={ 0,0,0,0,0,0,0,1,1,1,2,2,2,3,4,4,4 };
static const byte H5_cbp_k_col[12+1]={ 0,0,1,2,3,4,4,5,5,5,5,5,5 };
static const byte H5_cbp_k_cls[8+1]={ 0,0,0,1,2,2,3,3,3 };

static const byte H5_cbp_l_en[14+1]={ 0,1,2,3,4,4,4,4,4,5,5,6,6,6,7 };





static const int H5_cbp_rA = (254+0) * (1);
static const int H5_cbp_rB = (219+0) * (1);
static const int H5_cbp_rC = (249+0) * (1);
static const int H5_cbp_rE = (255+0) * (1);
static const int H5_cbp_rH = (236+0) * (1);
static const int H5_cbp_rI = (127+0) * (1);
static const int H5_cbp_rJ = (220+0) * (1);
static const int H5_cbp_rK = (254+0) * (1);
static const int H5_cbp_rL = (0+0) * (1);
static const int H5_cbp_rD = (251+0) * (1);
static const int H5_cbp_lr = (10+0) * (1);
static const int H5_cbp_mwA = (16+0) * (1);
static const int H5_cbp_mwB = (13+0) * (1);
static const int H5_cbp_mwC = (0+0) * (1);
static const int H5_cbp_mwE = (13+0) * (1);
static const int H5_cbp_mwH = (16+0) * (1);
static const int H5_cbp_mwI = (32+0) * (1);
static const int H5_cbp_mwJ = (0+0) * (1);
static const int H5_cbp_mwK = (4+0) * (1);
static const int H5_cbp_mwL = (129+0) * (1);
static const int H5_cbp_mwD = (73+0) * (1);
static const int H5_cbp_mb = (15+0) * (1);
static const int H5_cbp_rT = (0+0) * (1);
static const int H5_cbp_mwT = (0+0) * (1);
static const int H5_cbp_mt = (0+0) * (1);
static const int H5_cbp_lrm = (0+0) * (1);
static const int H5_cbp_mbm = (0+0) * (1);
static const int H5_cbp_rG = (0+0) * (1);
static const int H5_cbp_mwG = (0+0) * (1);
static const int H5_cbp_hA = (24+0) * (1);
static const int H5_cbp_hB = (24+0) * (1);
static const int H5_cbp_hC = (24+0) * (1);
static const int H5_cbp_hE = (24+0) * (1);
static const int H5_cbp_hH = (24+0) * (1);
static const int H5_cbp_hI = (24+0) * (1);
static const int H5_cbp_hJ = (24+0) * (1);
static const int H5_cbp_hK = (24+0) * (1);
static const int H5_cbp_hL = (24+0) * (1);
static const int H5_cbp_hD = (24+0) * (1);
static const int H5_cbp_hM = (24+0) * (1);
static const int H5_cbp_hT = (24+0) * (1);
static const int H5_cbp_hN = (24+0) * (1);


static const long long H5_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 5), 3); // 180
static const long long H5_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 3); // 18
static const long long H5_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 6), 3), 6); // 324
static const long long H5_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 10), 3); // 120
static const long long H5_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 2), 6); // 72
static const long long H5_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H5_cbp_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 5), 6); // 150
static const long long H5_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long H5_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 9), 4); // 216
static const long long H5_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 6), 4); // 480
static const long long H5_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 8), 3); // 144
static const long long H5_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H5_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_cbp_t_Volume = tc_vmul(1, 1); // 1

#define H5_cbp_USES(X) X(inc) X(pc) X(cls) X(st) X(col) X(q6) X(pg) X(em) X(tp) X(td) X(en)

struct H5_cbp_T {
  cm_cnt2 H5_cbp_A[tc_rows(H5_cbp_a_Volume, H5_cbp_hA) * H_cbp_ND];
  cm_cnt2 H5_cbp_B[tc_rows(H5_cbp_b_Volume, H5_cbp_hB) * H_cbp_ND];
  cm_cnt2 H5_cbp_C[tc_rows(H5_cbp_c_Volume, H5_cbp_hC) * H_cbp_ND];
  cm_cnt2 H5_cbp_D[tc_rows(H5_cbp_d_Volume, H5_cbp_hD) * H_cbp_ND];
  cm_cnt2 H5_cbp_E[tc_rows(H5_cbp_e_Volume, H5_cbp_hE) * H_cbp_ND];
  cm_cnt2 H5_cbp_H[tc_rows(H5_cbp_h_Volume, H5_cbp_hH) * H_cbp_ND];
  cm_cnt2 H5_cbp_I[tc_rows(H5_cbp_i_Volume, H5_cbp_hI) * H_cbp_ND];
  cm_cnt2 H5_cbp_J[tc_rows(H5_cbp_j_Volume, H5_cbp_hJ) * H_cbp_ND];
  cm_cnt2 H5_cbp_K[tc_rows(H5_cbp_k_Volume, H5_cbp_hK) * H_cbp_ND];
  cm_cnt2 H5_cbp_L[tc_rows(H5_cbp_l_Volume, H5_cbp_hL) * H_cbp_ND];
  cm_cnt H5_cbp_MT[tc_rows(H5_cbp_t_Volume, H5_cbp_hT) * H_cbp_MND];
  i32 H5_cbp_W[tc_rows(H5_cbp_m_Volume, H5_cbp_hM) * H_cbp_ND * 24];
  u8 H5_cbp_WC[tc_rows(H5_cbp_m_Volume, H5_cbp_hM) * H_cbp_ND];
  i32 H5_cbp_WM[tc_rows(H5_cbp_n_Volume, H5_cbp_hN) * H_cbp_MND * 3];
  u8 H5_cbp_WMC[tc_rows(H5_cbp_n_Volume, H5_cbp_hN) * H_cbp_MND];
  cm_cnt H5_cbp_G[H5_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_a_Volume, H5_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_b_Volume, H5_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_c_Volume, H5_cbp_hC) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_d_Volume, H5_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_e_Volume, H5_cbp_hE) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_h_Volume, H5_cbp_hH) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_i_Volume, H5_cbp_hI) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_j_Volume, H5_cbp_hJ) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_k_Volume, H5_cbp_hK) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_cbp_l_Volume, H5_cbp_hL) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_cbp_t_Volume, H5_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_cbp_m_Volume, H5_cbp_hM) * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_cbp_m_Volume, H5_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_cbp_n_Volume, H5_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_cbp_n_Volume, H5_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_cbp_g_Volume * H_cbp_NG);
  void H5_cbp_Init( void ) {}
  void H5_cbp_Quit( void ) {}
};
