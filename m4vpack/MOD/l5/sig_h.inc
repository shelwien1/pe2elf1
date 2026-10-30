
static const byte M5_sig_a_nc[17+1]={ 0,1,1,2,3,4,5,6,6,6,6,6,6,6,6,6,6,6 };
static const byte M5_sig_a_qs[32+1]={ 0,0,0,1,2,2,3,4,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };


static const byte M5_sig_c_nz[8+1]={ 0,1,2,3,4,4,5,6,7 };

static const byte M5_sig_d_past[8+1]={ 0,1,2,3,4,5,6,6,7 };
static const byte M5_sig_d_qs[32+1]={ 0,0,0,0,0,0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte M5_sig_e_run[16+1]={ 0,1,2,3,3,4,4,4,5,6,7,8,9,10,11,12,12 };



static const byte M5_sig_i_mvc[30+1]={ 0,1,2,2,3,4,4,4,4,5,5,5,6,7,7,7,7,7,8,9,10,11,12,13,13,13,13,13,13,13,13 };

static const byte M5_sig_j_rnz[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,5,5,5,5,6,6,6 };
static const byte M5_sig_j_xm[9+1]={ 0,1,2,2,2,2,3,3,4,4 };







static const int M5_sig_rA = (255+0) * (1);
static const int M5_sig_mwA = (115+0) * (1);
static const int M5_sig_rB = (255+0) * (1);
static const int M5_sig_mwB = (128+0) * (1);
static const int M5_sig_rC = (255+0) * (1);
static const int M5_sig_mwC = (63+0) * (1);
static const int M5_sig_rD = (214+0) * (1);
static const int M5_sig_mwD = (14+0) * (1);
static const int M5_sig_rE = (254+0) * (1);
static const int M5_sig_mwE = (1+0) * (1);
static const int M5_sig_rH = (255+0) * (1);
static const int M5_sig_mwH = (39+0) * (1);
static const int M5_sig_rI = (255+0) * (1);
static const int M5_sig_mwI = (95+0) * (1);
static const int M5_sig_rJ = (255+0) * (1);
static const int M5_sig_mwJ = (1+0) * (1);
static const int M5_sig_rK = (254+0) * (1);
static const int M5_sig_mwK = (41+0) * (1);
static const int M5_sig_rL = (253+0) * (1);
static const int M5_sig_mwL = (10+0) * (1);
static const int M5_sig_lr = (12+0) * (1);
static const int M5_sig_mb = (64+0) * (1);
static const int M5_sig_rT = (0+0) * (1);
static const int M5_sig_mwT = (0+0) * (1);
static const int M5_sig_mt = (0+0) * (1);
static const int M5_sig_lrm = (0+0) * (1);
static const int M5_sig_mbm = (0+0) * (1);
static const int M5_sig_rG = (0+0) * (1);
static const int M5_sig_mwG = (0+0) * (1);
static const int M5_sig_hA = (22+0) * (1);
static const int M5_sig_hB = (22+0) * (1);
static const int M5_sig_hC = (22+0) * (1);
static const int M5_sig_hD = (22+0) * (1);
static const int M5_sig_hE = (22+0) * (1);
static const int M5_sig_hH = (22+0) * (1);
static const int M5_sig_hI = (22+0) * (1);
static const int M5_sig_hJ = (22+0) * (1);
static const int M5_sig_hK = (22+0) * (1);
static const int M5_sig_hL = (20+0) * (1);
static const int M5_sig_hM = (22+0) * (1);
static const int M5_sig_hT = (22+0) * (1);
static const int M5_sig_hN = (22+0) * (1);


static const long long M5_sig_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 7), 7); // 196
static const long long M5_sig_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 2), (1<<6)); // 1024
static const long long M5_sig_c_Volume = tc_vmul(tc_vmul(1, (1<<1)), 8); // 16
static const long long M5_sig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 8), 4), (1<<6)); // 16384
static const long long M5_sig_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 13), (1<<6)); // 6656
static const long long M5_sig_g_Volume = 1; // 1
static const long long M5_sig_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), (1<<5)), (1<<6)); // 4096
static const long long M5_sig_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 14), (1<<6)); // 7168
static const long long M5_sig_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 7), 5); // 140
static const long long M5_sig_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<8)), (1<<6)); // 131072
static const long long M5_sig_l_Volume = tc_vmul(tc_vmul(1, (1<<6)), (1<<14)); // 1048576
static const long long M5_sig_m_Volume = tc_vmul(tc_vmul(1, (1<<2)), (1<<2)); // 16
static const long long M5_sig_n_Volume = 1; // 1
static const long long M5_sig_t_Volume = 1; // 1

#define M5_sig_USES(X) X(cls) X(nc) X(qs) X(kb) X(pos) X(nz) X(past) X(run) X(fp) X(mvc) X(rnz) X(xm) X(h8) X(h16)

struct M5_sig_T {
  cm_cnt2 M5_sig_A[tc_rows(M5_sig_a_Volume, M5_sig_hA) * M_sig_ND];
  cm_cnt2 M5_sig_B[tc_rows(M5_sig_b_Volume, M5_sig_hB) * M_sig_ND];
  cm_cnt2 M5_sig_C[tc_rows(M5_sig_c_Volume, M5_sig_hC) * M_sig_ND];
  cm_cnt2 M5_sig_D[tc_rows(M5_sig_d_Volume, M5_sig_hD) * M_sig_ND];
  cm_cnt2 M5_sig_E[tc_rows(M5_sig_e_Volume, M5_sig_hE) * M_sig_ND];
  cm_cnt2 M5_sig_H[tc_rows(M5_sig_h_Volume, M5_sig_hH) * M_sig_ND];
  cm_cnt2 M5_sig_I[tc_rows(M5_sig_i_Volume, M5_sig_hI) * M_sig_ND];
  cm_cnt2 M5_sig_J[tc_rows(M5_sig_j_Volume, M5_sig_hJ) * M_sig_ND];
  cm_cnt2 M5_sig_K[tc_rows(M5_sig_k_Volume, M5_sig_hK) * M_sig_ND];
  cm_cnt2 M5_sig_L[tc_rows(M5_sig_l_Volume, M5_sig_hL) * M_sig_ND];
  cm_cnt M5_sig_MT[tc_rows(M5_sig_t_Volume, M5_sig_hT) * M_sig_MND];
  i32 M5_sig_W[tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND * tc_nin(10)];
  u8 M5_sig_WC[tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND];
  i32 M5_sig_WM[tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND * TC_MIN];
  u8 M5_sig_WMC[tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND];
  cm_cnt M5_sig_G[M5_sig_g_Volume * M_sig_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_sig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_a_Volume, M5_sig_hA) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_b_Volume, M5_sig_hB) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_c_Volume, M5_sig_hC) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_d_Volume, M5_sig_hD) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_e_Volume, M5_sig_hE) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_h_Volume, M5_sig_hH) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_i_Volume, M5_sig_hI) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_j_Volume, M5_sig_hJ) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_k_Volume, M5_sig_hK) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_sig_l_Volume, M5_sig_hL) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_sig_t_Volume, M5_sig_hT) * M_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sig_m_Volume, M5_sig_hM) * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_sig_n_Volume, M5_sig_hN) * M_sig_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_sig_g_Volume * M_sig_NG);
  void M5_sig_Init( void ) {}
  void M5_sig_Quit( void ) {}
};
