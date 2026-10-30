
static const byte M1_mvg_a_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M1_mvg_a_mag[16+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };

static const byte M1_mvg_b_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M1_mvg_b_dz[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };

static const byte M1_mvg_c_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M1_mvg_c_hm[3+1]={ 0,1,2,3 };

static const byte M1_mvg_d_kind[5+1]={ 0,1,2,3,4,5 };


static const byte M1_mvg_m_kind[5+1]={ 0,1,2,3,4,5 };




static const int M1_mvg_rA = (129+0) * (1);
static const int M1_mvg_mwA = (11+0) * (1);
static const int M1_mvg_rB = (0+0) * (1);
static const int M1_mvg_mwB = (69+0) * (1);
static const int M1_mvg_rC = (255+0) * (1);
static const int M1_mvg_mwC = (70+0) * (1);
static const int M1_mvg_rD = (251+0) * (1);
static const int M1_mvg_mwD = (7+0) * (1);
static const int M1_mvg_lr = (12+0) * (1);
static const int M1_mvg_mb = (129+0) * (1);
static const int M1_mvg_rT = (0+0) * (1);
static const int M1_mvg_mwT = (0+0) * (1);
static const int M1_mvg_mt = (0+0) * (1);
static const int M1_mvg_lrm = (0+0) * (1);
static const int M1_mvg_mbm = (0+0) * (1);
static const int M1_mvg_rG = (0+0) * (1);
static const int M1_mvg_mwG = (0+0) * (1);
static const int M1_mvg_hA = (22+0) * (1);
static const int M1_mvg_hB = (22+0) * (1);
static const int M1_mvg_hC = (22+0) * (1);
static const int M1_mvg_hD = (22+0) * (1);
static const int M1_mvg_hM = (22+0) * (1);
static const int M1_mvg_hT = (22+0) * (1);
static const int M1_mvg_hN = (22+0) * (1);


static const long long M1_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 3), 6); // 216
static const long long M1_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 3), 7); // 252
static const long long M1_mvg_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 3), 4); // 144
static const long long M1_mvg_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 3), 3), 3); // 162
static const long long M1_mvg_g_Volume = 1; // 1
static const long long M1_mvg_m_Volume = tc_vmul(tc_vmul(1, 6), 2); // 12
static const long long M1_mvg_n_Volume = 1; // 1
static const long long M1_mvg_t_Volume = 1; // 1

#define M1_mvg_USES(X) X(kind) X(comp) X(ds) X(mag) X(dz) X(hm) X(ls) X(as)

struct M1_mvg_T {
  cm_cnt2s M1_mvg_A[tc_rows(M1_mvg_a_Volume, M1_mvg_hA) * M_mvg_ND];
  cm_cnt2s M1_mvg_B[tc_rows(M1_mvg_b_Volume, M1_mvg_hB) * M_mvg_ND];
  cm_cnt2s M1_mvg_C[tc_rows(M1_mvg_c_Volume, M1_mvg_hC) * M_mvg_ND];
  cm_cnt2s M1_mvg_D[tc_rows(M1_mvg_d_Volume, M1_mvg_hD) * M_mvg_ND];
  cm_cnt M1_mvg_MT[tc_rows(M1_mvg_t_Volume, M1_mvg_hT) * M_mvg_MND];
  i32 M1_mvg_W[tc_rows(M1_mvg_m_Volume, M1_mvg_hM) * M_mvg_ND * tc_nin(4)];
  u8 M1_mvg_WC[tc_rows(M1_mvg_m_Volume, M1_mvg_hM) * M_mvg_ND];
  i32 M1_mvg_WM[tc_rows(M1_mvg_n_Volume, M1_mvg_hN) * M_mvg_MND * TC_MIN];
  u8 M1_mvg_WMC[tc_rows(M1_mvg_n_Volume, M1_mvg_hN) * M_mvg_MND];
  cm_cnt M1_mvg_G[M1_mvg_g_Volume * M_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvg_a_Volume, M1_mvg_hA) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvg_b_Volume, M1_mvg_hB) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvg_c_Volume, M1_mvg_hC) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mvg_d_Volume, M1_mvg_hD) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_mvg_t_Volume, M1_mvg_hT) * M_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mvg_m_Volume, M1_mvg_hM) * M_mvg_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mvg_m_Volume, M1_mvg_hM) * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mvg_n_Volume, M1_mvg_hN) * M_mvg_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mvg_n_Volume, M1_mvg_hN) * M_mvg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_mvg_g_Volume * M_mvg_NG);
  void M1_mvg_Init( void ) {}
  void M1_mvg_Quit( void ) {}
};
