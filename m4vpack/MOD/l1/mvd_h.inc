
static const byte M1_mvd_a_kind[5+1]={ 0,1,2,3,3,4 };
static const byte M1_mvd_a_dz[17+1]={ 0,0,1,2,3,4,4,4,5,5,5,5,5,5,5,6,6,6 };

static const byte M1_mvd_b_kind[5+1]={ 0,1,2,3,3,3 };
static const byte M1_mvd_b_sp[17+1]={ 0,1,2,3,4,4,5,6,6,6,7,7,7,7,8,8,9,9 };

static const byte M1_mvd_c_rs[17+1]={ 0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M1_mvd_d_lq[17+1]={ 0,1,2,3,4,4,4,4,5,6,6,6,6,6,6,6,7,7 };
static const byte M1_mvd_d_aq[17+1]={ 0,1,2,3,3,3,3,3,3,3,4,4,4,4,4,4,5,5 };




static const byte M1_mvd_t_kind[5+1]={ 0,1,2,3,4,5 };


static const int M1_mvd_rA = (255+0) * (1);
static const int M1_mvd_mwA = (4+0) * (1);
static const int M1_mvd_rB = (255+0) * (1);
static const int M1_mvd_mwB = (16+0) * (1);
static const int M1_mvd_rC = (255+0) * (1);
static const int M1_mvd_mwC = (47+0) * (1);
static const int M1_mvd_rD = (255+0) * (1);
static const int M1_mvd_mwD = (20+0) * (1);
static const int M1_mvd_lr = (15+0) * (1);
static const int M1_mvd_mb = (36+0) * (1);
static const int M1_mvd_rT = (3+0) * (1);
static const int M1_mvd_mwT = (64+0) * (1);
static const int M1_mvd_mt = (6+0) * (1);
static const int M1_mvd_lrm = (31+0) * (1);
static const int M1_mvd_mbm = (32+0) * (1);
static const int M1_mvd_rG = (0+0) * (1);
static const int M1_mvd_mwG = (0+0) * (1);
static const int M1_mvd_hA = (22+0) * (1);
static const int M1_mvd_hB = (22+0) * (1);
static const int M1_mvd_hC = (22+0) * (1);
static const int M1_mvd_hD = (22+0) * (1);
static const int M1_mvd_hM = (22+0) * (1);
static const int M1_mvd_hT = (22+0) * (1);
static const int M1_mvd_hN = (22+0) * (1);


static const long long M1_mvd_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 2), 7), 3); // 210
static const long long M1_mvd_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 10); // 40
static const long long M1_mvd_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 5); // 15
static const long long M1_mvd_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 8), 6); // 288
static const long long M1_mvd_g_Volume = 1; // 1
static const long long M1_mvd_m_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long M1_mvd_n_Volume = tc_vmul(1, 1); // 1
static const long long M1_mvd_t_Volume = tc_vmul(tc_vmul(1, 6), 1); // 6

#define M1_mvd_USES(X) X(kind) X(comp) X(dz) X(ds) X(sp) X(rs) X(lq) X(aq)

struct M1_mvd_T {
  cm_cnt2s M1_mvd_A[tc_rows(M1_mvd_a_Volume, M1_mvd_hA) * M_mvd_ND];
  cm_cnt2s M1_mvd_B[tc_rows(M1_mvd_b_Volume, M1_mvd_hB) * M_mvd_ND];
  cm_cnt2s M1_mvd_C[tc_rows(M1_mvd_c_Volume, M1_mvd_hC) * M_mvd_ND];
  cm_cnt2s M1_mvd_D[tc_rows(M1_mvd_d_Volume, M1_mvd_hD) * M_mvd_ND];
  cm_cnt M1_mvd_MT[tc_rows(M1_mvd_t_Volume, M1_mvd_hT) * M_mvd_MND];
  i32 M1_mvd_W[tc_rows(M1_mvd_m_Volume, M1_mvd_hM) * M_mvd_ND * tc_nin(4)];
  u8 M1_mvd_WC[tc_rows(M1_mvd_m_Volume, M1_mvd_hM) * M_mvd_ND];
  i32 M1_mvd_WM[tc_rows(M1_mvd_n_Volume, M1_mvd_hN) * M_mvd_MND * TC_MIN];
  u8 M1_mvd_WMC[tc_rows(M1_mvd_n_Volume, M1_mvd_hN) * M_mvd_MND];
  cm_cnt M1_mvd_G[M1_mvd_g_Volume * M_mvd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_mvd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvd_a_Volume, M1_mvd_hA) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvd_b_Volume, M1_mvd_hB) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvd_c_Volume, M1_mvd_hC) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvd_d_Volume, M1_mvd_hD) * M_mvd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_mvd_t_Volume, M1_mvd_hT) * M_mvd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mvd_m_Volume, M1_mvd_hM) * M_mvd_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mvd_m_Volume, M1_mvd_hM) * M_mvd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mvd_n_Volume, M1_mvd_hN) * M_mvd_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mvd_n_Volume, M1_mvd_hN) * M_mvd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_mvd_g_Volume * M_mvd_NG);
  void M1_mvd_Init( void ) {}
  void M1_mvd_Quit( void ) {}
};
