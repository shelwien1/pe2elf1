
static const byte H5_qpg_a_mag[7+1]={ 0,0,1,1,2,3,4,4 };
static const byte H5_qpg_a_ss[9+1]={ 0,0,0,1,2,3,4,4,5,5 };
static const byte H5_qpg_a_xe[18+1]={ 0,0,0,0,0,0,0,1,1,1,1,1,1,1,2,2,3,3,4 };
static const byte H5_qpg_a_lq[9+1]={ 0,0,0,1,1,2,3,3,3,3 };


static const byte H5_qpg_c_cs[9+1]={ 0,0,0,1,1,2,3,4,4,5 };

static const byte H5_qpg_d_bs[9+1]={ 0,0,0,1,1,2,3,4,5,6 };

static const byte H5_qpg_e_cs[9+1]={ 0,0,0,1,1,1,2,2,3,3 };
static const byte H5_qpg_e_ss[9+1]={ 0,0,0,1,1,2,3,4,5,5 };


static const byte H5_qpg_h_as[9+1]={ 0,0,0,1,1,2,2,2,2,3 };
static const byte H5_qpg_h_rs[3+1]={ 0,1,2,3 };

static const byte H5_qpg_i_as[9+1]={ 0,0,0,0,1,2,3,3,4,5 };
static const byte H5_qpg_i_cs[9+1]={ 0,0,0,1,2,3,3,3,4,4 };
static const byte H5_qpg_i_bs[9+1]={ 0,0,0,1,2,2,3,4,5,6 };

static const byte H5_qpg_j_cls[8+1]={ 0,0,0,0,1,1,2,3,3 };
static const byte H5_qpg_j_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };

static const byte H5_qpg_k_mag[7+1]={ 0,0,1,2,3,3,3,3 };
static const byte H5_qpg_k_as[9+1]={ 0,0,0,1,2,3,4,5,6,7 };

static const byte H5_qpg_l_cs[9+1]={ 0,0,0,1,2,3,4,5,6,6 };
static const byte H5_qpg_l_bs[9+1]={ 0,0,0,1,2,3,4,5,5,5 };





static const int H5_qpg_rA = (232+0) * (1);
static const int H5_qpg_rB = (122+0) * (1);
static const int H5_qpg_rC = (241+0) * (1);
static const int H5_qpg_rE = (65+0) * (1);
static const int H5_qpg_rH = (255+0) * (1);
static const int H5_qpg_rI = (254+0) * (1);
static const int H5_qpg_rJ = (129+0) * (1);
static const int H5_qpg_rK = (252+0) * (1);
static const int H5_qpg_rL = (212+0) * (1);
static const int H5_qpg_rD = (254+0) * (1);
static const int H5_qpg_rT = (0+0) * (1);
static const int H5_qpg_rG = (0+0) * (1);
static const int H5_qpg_lr = (15+0) * (1);
static const int H5_qpg_lrm = (0+0) * (1);
static const int H5_qpg_mbm = (0+0) * (1);
static const int H5_qpg_mwA = (0+0) * (1);
static const int H5_qpg_mwB = (23+0) * (1);
static const int H5_qpg_mwC = (167+0) * (1);
static const int H5_qpg_mwE = (8+0) * (1);
static const int H5_qpg_mwH = (46+0) * (1);
static const int H5_qpg_mwI = (126+0) * (1);
static const int H5_qpg_mwJ = (128+0) * (1);
static const int H5_qpg_mwK = (6+0) * (1);
static const int H5_qpg_mwL = (124+0) * (1);
static const int H5_qpg_mwD = (1+0) * (1);
static const int H5_qpg_mwT = (0+0) * (1);
static const int H5_qpg_mwG = (0+0) * (1);
static const int H5_qpg_mt = (0+0) * (1);
static const int H5_qpg_mb = (130+0) * (1);
static const int H5_qpg_hA = (24+0) * (1);
static const int H5_qpg_hB = (24+0) * (1);
static const int H5_qpg_hC = (24+0) * (1);
static const int H5_qpg_hE = (24+0) * (1);
static const int H5_qpg_hH = (24+0) * (1);
static const int H5_qpg_hI = (24+0) * (1);
static const int H5_qpg_hJ = (24+0) * (1);
static const int H5_qpg_hK = (24+0) * (1);
static const int H5_qpg_hL = (24+0) * (1);
static const int H5_qpg_hD = (24+0) * (1);
static const int H5_qpg_hM = (24+0) * (1);
static const int H5_qpg_hT = (24+0) * (1);
static const int H5_qpg_hN = (24+0) * (1);


static const long long H5_qpg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 6), 5), 4); // 600
static const long long H5_qpg_b_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H5_qpg_c_Volume = tc_vmul(tc_vmul(1, 3), 6); // 18
static const long long H5_qpg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 3), 2); // 42
static const long long H5_qpg_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 6); // 48
static const long long H5_qpg_g_Volume = tc_vmul(1, 1); // 1
static const long long H5_qpg_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 2); // 32
static const long long H5_qpg_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 5), 7); // 210
static const long long H5_qpg_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 3); // 48
static const long long H5_qpg_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 8), 3); // 96
static const long long H5_qpg_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 6), 1); // 42
static const long long H5_qpg_m_Volume = tc_vmul(1, 3); // 3
static const long long H5_qpg_n_Volume = tc_vmul(1, 1); // 1
static const long long H5_qpg_t_Volume = tc_vmul(1, 1); // 1

#define H5_qpg_USES(X) X(mag) X(ss) X(xe) X(lq) X(as) X(cs) X(bs) X(bk) X(rs) X(ls) X(cls) X(st)

struct H5_qpg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H5_qpg_A[tc_rows(H5_qpg_a_Volume, H5_qpg_hA) * H_qpg_ND];
  cm_cnt2 H5_qpg_B[tc_rows(H5_qpg_b_Volume, H5_qpg_hB) * H_qpg_ND];
  cm_cnt2 H5_qpg_C[tc_rows(H5_qpg_c_Volume, H5_qpg_hC) * H_qpg_ND];
  cm_cnt2 H5_qpg_D[tc_rows(H5_qpg_d_Volume, H5_qpg_hD) * H_qpg_ND];
  cm_cnt2 H5_qpg_E[tc_rows(H5_qpg_e_Volume, H5_qpg_hE) * H_qpg_ND];
  cm_cnt2 H5_qpg_H[tc_rows(H5_qpg_h_Volume, H5_qpg_hH) * H_qpg_ND];
  cm_cnt2 H5_qpg_I[tc_rows(H5_qpg_i_Volume, H5_qpg_hI) * H_qpg_ND];
  cm_cnt2 H5_qpg_J[tc_rows(H5_qpg_j_Volume, H5_qpg_hJ) * H_qpg_ND];
  cm_cnt2 H5_qpg_K[tc_rows(H5_qpg_k_Volume, H5_qpg_hK) * H_qpg_ND];
  cm_cnt2 H5_qpg_L[tc_rows(H5_qpg_l_Volume, H5_qpg_hL) * H_qpg_ND];
  cm_cnt H5_qpg_MT[tc_rows(H5_qpg_t_Volume, H5_qpg_hT) * H_qpg_MND];
  i32 H5_qpg_W[tc_rows(H5_qpg_m_Volume, H5_qpg_hM) * H_qpg_ND * 24];
  u8 H5_qpg_WC[tc_rows(H5_qpg_m_Volume, H5_qpg_hM) * H_qpg_ND];
  i32 H5_qpg_WM[tc_rows(H5_qpg_n_Volume, H5_qpg_hN) * H_qpg_MND * 3];
  u8 H5_qpg_WMC[tc_rows(H5_qpg_n_Volume, H5_qpg_hN) * H_qpg_MND];
  cm_cnt H5_qpg_G[H5_qpg_g_Volume * H_qpg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_qpg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_a_Volume, H5_qpg_hA) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_b_Volume, H5_qpg_hB) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_c_Volume, H5_qpg_hC) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_d_Volume, H5_qpg_hD) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_e_Volume, H5_qpg_hE) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_h_Volume, H5_qpg_hH) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_i_Volume, H5_qpg_hI) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_j_Volume, H5_qpg_hJ) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_k_Volume, H5_qpg_hK) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_qpg_l_Volume, H5_qpg_hL) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_qpg_t_Volume, H5_qpg_hT) * H_qpg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_qpg_m_Volume, H5_qpg_hM) * H_qpg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_qpg_m_Volume, H5_qpg_hM) * H_qpg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_qpg_n_Volume, H5_qpg_hN) * H_qpg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_qpg_n_Volume, H5_qpg_hN) * H_qpg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_qpg_g_Volume * H_qpg_NG);
  void H5_qpg_Init( void ) {
  
  }
};
