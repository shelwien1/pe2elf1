
static const byte M3_mvd_a_kind[5+1]={ 0,1,2,3,3,4 };
static const byte M3_mvd_a_dz[17+1]={ 0,0,1,2,3,3,4,4,5,5,5,5,5,5,6,6,6,6 };

static const byte M3_mvd_b_sp[17+1]={ 0,1,2,3,4,5,6,6,7,7,8,9,9,9,9,10,11,11 };

static const byte M3_mvd_c_lq[17+1]={ 0,1,2,3,3,3,4,5,5,5,5,5,5,5,5,5,6,6 };
static const byte M3_mvd_c_aq[17+1]={ 0,1,2,3,3,4,5,6,6,6,6,6,6,6,6,7,8,8 };

static const byte M3_mvd_d_rs[17+1]={ 0,1,2,3,3,3,3,3,4,4,4,5,5,6,6,6,6,6 };

static const byte M3_mvd_e_kind[5+1]={ 0,0,1,2,3,3 };
static const byte M3_mvd_e_rbi[5+1]={ 0,1,2,3,4,5 };



static const byte M3_mvd_m_kind[5+1]={ 0,0,1,1,2,3 };




static const int M3_mvd_rA = (225+0) * (1);
static const int M3_mvd_mwA = (47+0) * (1);
static const int M3_mvd_rB = (255+0) * (1);
static const int M3_mvd_mwB = (40+0) * (1);
static const int M3_mvd_rC = (254+0) * (1);
static const int M3_mvd_mwC = (0+0) * (1);
static const int M3_mvd_rD = (127+0) * (1);
static const int M3_mvd_mwD = (127+0) * (1);
static const int M3_mvd_rE = (0+0) * (1);
static const int M3_mvd_mwE = (102+0) * (1);
static const int M3_mvd_rH = (253+0) * (1);
static const int M3_mvd_mwH = (111+0) * (1);
static const int M3_mvd_lr = (20+0) * (1);
static const int M3_mvd_mb = (11+0) * (1);
static const int M3_mvd_rT = (15+0) * (1);
static const int M3_mvd_mwT = (128+0) * (1);
static const int M3_mvd_mt = (6+0) * (1);
static const int M3_mvd_lrm = (31+0) * (1);
static const int M3_mvd_mbm = (0+0) * (1);
static const int M3_mvd_rG = (0+0) * (1);
static const int M3_mvd_mwG = (0+0) * (1);
static const int M3_mvd_hA = (22+0) * (1);
static const int M3_mvd_hB = (22+0) * (1);
static const int M3_mvd_hC = (22+0) * (1);
static const int M3_mvd_hD = (22+0) * (1);
static const int M3_mvd_hE = (22+0) * (1);
static const int M3_mvd_hH = (22+0) * (1);
static const int M3_mvd_hM = (22+0) * (1);
static const int M3_mvd_hT = (22+0) * (1);
static const int M3_mvd_hN = (22+0) * (1);


static const long long M3_mvd_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 2), 7), 3); // 210
static const long long M3_mvd_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 12); // 12
static const long long M3_mvd_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 7), 9); // 378
static const long long M3_mvd_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 7); // 42
static const long long M3_mvd_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 6), 3); // 72
static const long long M3_mvd_g_Volume = 1; // 1
static const long long M3_mvd_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long M3_mvd_m_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long M3_mvd_n_Volume = tc_vmul(1, 1); // 1
static const long long M3_mvd_t_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3

#define M3_mvd_USES(X) X(kind) X(comp) X(dz) X(ds) X(sp) X(lq) X(aq) X(rs) X(rbi) X(rsame) X(fc)

struct M3_mvd_T {
  cm_cnt2 M3_mvd_A[tc_rows(M3_mvd_a_Volume, M3_mvd_hA) * M_mvd_ND];
  cm_cnt2 M3_mvd_B[tc_rows(M3_mvd_b_Volume, M3_mvd_hB) * M_mvd_ND];
  cm_cnt2 M3_mvd_C[tc_rows(M3_mvd_c_Volume, M3_mvd_hC) * M_mvd_ND];
  cm_cnt2 M3_mvd_D[tc_rows(M3_mvd_d_Volume, M3_mvd_hD) * M_mvd_ND];
  cm_cnt2 M3_mvd_E[tc_rows(M3_mvd_e_Volume, M3_mvd_hE) * M_mvd_ND];
  cm_cnt2 M3_mvd_H[tc_rows(M3_mvd_h_Volume, M3_mvd_hH) * M_mvd_ND];
  cm_cnt M3_mvd_MT[tc_rows(M3_mvd_t_Volume, M3_mvd_hT) * M_mvd_MND];
  i32 M3_mvd_W[tc_rows(M3_mvd_m_Volume, M3_mvd_hM) * M_mvd_ND * tc_nin(6)];
  u8 M3_mvd_WC[tc_rows(M3_mvd_m_Volume, M3_mvd_hM) * M_mvd_ND];
  i32 M3_mvd_WM[tc_rows(M3_mvd_n_Volume, M3_mvd_hN) * M_mvd_MND * TC_MIN];
  u8 M3_mvd_WMC[tc_rows(M3_mvd_n_Volume, M3_mvd_hN) * M_mvd_MND];
  cm_cnt M3_mvd_G[M3_mvd_g_Volume * M_mvd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_mvd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_a_Volume, M3_mvd_hA) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_b_Volume, M3_mvd_hB) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_c_Volume, M3_mvd_hC) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_d_Volume, M3_mvd_hD) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_e_Volume, M3_mvd_hE) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_mvd_h_Volume, M3_mvd_hH) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_mvd_t_Volume, M3_mvd_hT) * M_mvd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mvd_m_Volume, M3_mvd_hM) * M_mvd_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mvd_m_Volume, M3_mvd_hM) * M_mvd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_mvd_n_Volume, M3_mvd_hN) * M_mvd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_mvd_n_Volume, M3_mvd_hN) * M_mvd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_mvd_g_Volume * M_mvd_NG);
  void M3_mvd_Init( void ) {}
  void M3_mvd_Quit( void ) {}
};
