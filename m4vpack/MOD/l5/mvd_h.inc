
static const byte M5_mvd_a_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_a_ns[16+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };

static const byte M5_mvd_b_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_b_oth[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M5_mvd_c_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_c_rs[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M5_mvd_d_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_d_dz[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M5_mvd_e_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_e_sp[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };


static const byte M5_mvd_h_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_h_lq[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };
static const byte M5_mvd_h_aq[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M5_mvd_i_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_i_fc[6+1]={ 0,1,2,3,4,5,6 };

static const byte M5_mvd_j_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_j_rbi[5+1]={ 0,1,2,3,4,5 };

static const byte M5_mvd_k_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M5_mvd_k_ns[16+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };
static const byte M5_mvd_k_oth[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M5_mvd_l_kind[5+1]={ 0,1,2,3,4,5 };

static const byte M5_mvd_m_kind[5+1]={ 0,1,2,3,4,5 };

static const byte M5_mvd_n_kind[5+1]={ 0,1,2,3,4,5 };

static const byte M5_mvd_t_kind[5+1]={ 0,1,2,3,4,5 };


static const int M5_mvd_rA = (255+0) * (1);
static const int M5_mvd_mwA = (9+0) * (1);
static const int M5_mvd_rB = (0+0) * (1);
static const int M5_mvd_mwB = (128+0) * (1);
static const int M5_mvd_rC = (252+0) * (1);
static const int M5_mvd_mwC = (5+0) * (1);
static const int M5_mvd_rD = (249+0) * (1);
static const int M5_mvd_mwD = (37+0) * (1);
static const int M5_mvd_rE = (255+0) * (1);
static const int M5_mvd_mwE = (3+0) * (1);
static const int M5_mvd_rH = (255+0) * (1);
static const int M5_mvd_mwH = (14+0) * (1);
static const int M5_mvd_rI = (218+0) * (1);
static const int M5_mvd_mwI = (6+0) * (1);
static const int M5_mvd_rJ = (177+0) * (1);
static const int M5_mvd_mwJ = (64+0) * (1);
static const int M5_mvd_rK = (229+0) * (1);
static const int M5_mvd_mwK = (57+0) * (1);
static const int M5_mvd_rL = (98+0) * (1);
static const int M5_mvd_mwL = (14+0) * (1);
static const int M5_mvd_lr = (15+0) * (1);
static const int M5_mvd_mb = (45+0) * (1);
static const int M5_mvd_rT = (28+0) * (1);
static const int M5_mvd_mwT = (9+0) * (1);
static const int M5_mvd_mt = (6+0) * (1);
static const int M5_mvd_lrm = (31+0) * (1);
static const int M5_mvd_mbm = (34+0) * (1);
static const int M5_mvd_rG = (0+0) * (1);
static const int M5_mvd_mwG = (0+0) * (1);
static const int M5_mvd_hA = (22+0) * (1);
static const int M5_mvd_hB = (22+0) * (1);
static const int M5_mvd_hC = (22+0) * (1);
static const int M5_mvd_hD = (22+0) * (1);
static const int M5_mvd_hE = (22+0) * (1);
static const int M5_mvd_hH = (22+0) * (1);
static const int M5_mvd_hI = (22+0) * (1);
static const int M5_mvd_hJ = (22+0) * (1);
static const int M5_mvd_hK = (22+0) * (1);
static const int M5_mvd_hL = (22+0) * (1);
static const int M5_mvd_hM = (22+0) * (1);
static const int M5_mvd_hT = (22+0) * (1);
static const int M5_mvd_hN = (22+0) * (1);


static const long long M5_mvd_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 6); // 72
static const long long M5_mvd_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7); // 84
static const long long M5_mvd_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7); // 84
static const long long M5_mvd_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7), 3); // 252
static const long long M5_mvd_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7); // 84
static const long long M5_mvd_g_Volume = 1; // 1
static const long long M5_mvd_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7), 7); // 588
static const long long M5_mvd_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 7); // 84
static const long long M5_mvd_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 6), 3); // 216
static const long long M5_mvd_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 6), 7); // 504
static const long long M5_mvd_l_Volume = tc_vmul(tc_vmul(1, 6), 2); // 12
static const long long M5_mvd_m_Volume = tc_vmul(tc_vmul(1, 6), 2); // 12
static const long long M5_mvd_n_Volume = tc_vmul(1, 6); // 6
static const long long M5_mvd_t_Volume = tc_vmul(tc_vmul(1, 6), 2); // 12

#define M5_mvd_USES(X) X(kind) X(comp) X(ns) X(oth) X(rs) X(dz) X(ds) X(sp) X(lq) X(aq) X(fc) X(rbi) X(rsame)

struct M5_mvd_T {
  cm_cnt2 M5_mvd_A[tc_rows(M5_mvd_a_Volume, M5_mvd_hA) * M_mvd_ND];
  cm_cnt2 M5_mvd_B[tc_rows(M5_mvd_b_Volume, M5_mvd_hB) * M_mvd_ND];
  cm_cnt2 M5_mvd_C[tc_rows(M5_mvd_c_Volume, M5_mvd_hC) * M_mvd_ND];
  cm_cnt2 M5_mvd_D[tc_rows(M5_mvd_d_Volume, M5_mvd_hD) * M_mvd_ND];
  cm_cnt2 M5_mvd_E[tc_rows(M5_mvd_e_Volume, M5_mvd_hE) * M_mvd_ND];
  cm_cnt2 M5_mvd_H[tc_rows(M5_mvd_h_Volume, M5_mvd_hH) * M_mvd_ND];
  cm_cnt2 M5_mvd_I[tc_rows(M5_mvd_i_Volume, M5_mvd_hI) * M_mvd_ND];
  cm_cnt2 M5_mvd_J[tc_rows(M5_mvd_j_Volume, M5_mvd_hJ) * M_mvd_ND];
  cm_cnt2 M5_mvd_K[tc_rows(M5_mvd_k_Volume, M5_mvd_hK) * M_mvd_ND];
  cm_cnt2 M5_mvd_L[tc_rows(M5_mvd_l_Volume, M5_mvd_hL) * M_mvd_ND];
  cm_cnt M5_mvd_MT[tc_rows(M5_mvd_t_Volume, M5_mvd_hT) * M_mvd_MND];
  i32 M5_mvd_W[tc_rows(M5_mvd_m_Volume, M5_mvd_hM) * M_mvd_ND * tc_nin(10)];
  u8 M5_mvd_WC[tc_rows(M5_mvd_m_Volume, M5_mvd_hM) * M_mvd_ND];
  i32 M5_mvd_WM[tc_rows(M5_mvd_n_Volume, M5_mvd_hN) * M_mvd_MND * TC_MIN];
  u8 M5_mvd_WMC[tc_rows(M5_mvd_n_Volume, M5_mvd_hN) * M_mvd_MND];
  cm_cnt M5_mvd_G[M5_mvd_g_Volume * M_mvd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_mvd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_a_Volume, M5_mvd_hA) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_b_Volume, M5_mvd_hB) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_c_Volume, M5_mvd_hC) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_d_Volume, M5_mvd_hD) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_e_Volume, M5_mvd_hE) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_h_Volume, M5_mvd_hH) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_i_Volume, M5_mvd_hI) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_j_Volume, M5_mvd_hJ) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_k_Volume, M5_mvd_hK) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_mvd_l_Volume, M5_mvd_hL) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_mvd_t_Volume, M5_mvd_hT) * M_mvd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mvd_m_Volume, M5_mvd_hM) * M_mvd_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mvd_m_Volume, M5_mvd_hM) * M_mvd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_mvd_n_Volume, M5_mvd_hN) * M_mvd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_mvd_n_Volume, M5_mvd_hN) * M_mvd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_mvd_g_Volume * M_mvd_NG);
  void M5_mvd_Init( void ) {}
  void M5_mvd_Quit( void ) {}
};
