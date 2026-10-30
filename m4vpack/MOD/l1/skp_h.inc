
static const byte M1_skp_a_st[3+1]={ 0,1,2,3 };

static const byte M1_skp_b_st[3+1]={ 0,1,2,3 };
static const byte M1_skp_b_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte M1_skp_c_st[3+1]={ 0,1,2,3 };
static const byte M1_skp_c_rc[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte M1_skp_c_run[16+1]={ 0,1,2,3,3,3,4,4,4,4,4,4,5,5,5,5,5 };

static const byte M1_skp_d_st[3+1]={ 0,1,2,3 };
static const byte M1_skp_d_c4[4+1]={ 0,1,2,3,4 };


static const byte M1_skp_m_st[3+1]={ 0,1,2,3 };




static const int M1_skp_rA = (254+0) * (1);
static const int M1_skp_mwA = (8+0) * (1);
static const int M1_skp_rB = (243+0) * (1);
static const int M1_skp_mwB = (4+0) * (1);
static const int M1_skp_rC = (250+0) * (1);
static const int M1_skp_mwC = (74+0) * (1);
static const int M1_skp_rD = (126+0) * (1);
static const int M1_skp_mwD = (7+0) * (1);
static const int M1_skp_lr = (15+0) * (1);
static const int M1_skp_mb = (32+0) * (1);
static const int M1_skp_rT = (0+0) * (1);
static const int M1_skp_mwT = (0+0) * (1);
static const int M1_skp_mt = (0+0) * (1);
static const int M1_skp_lrm = (0+0) * (1);
static const int M1_skp_mbm = (0+0) * (1);
static const int M1_skp_rG = (0+0) * (1);
static const int M1_skp_mwG = (0+0) * (1);
static const int M1_skp_hA = (22+0) * (1);
static const int M1_skp_hB = (22+0) * (1);
static const int M1_skp_hC = (22+0) * (1);
static const int M1_skp_hD = (22+0) * (1);
static const int M1_skp_hM = (22+0) * (1);
static const int M1_skp_hT = (22+0) * (1);
static const int M1_skp_hN = (22+0) * (1);


static const long long M1_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3); // 24
static const long long M1_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 11), 3); // 132
static const long long M1_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 11), 6); // 264
static const long long M1_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 5), 3); // 180
static const long long M1_skp_g_Volume = 1; // 1
static const long long M1_skp_m_Volume = tc_vmul(tc_vmul(1, 4), 3); // 12
static const long long M1_skp_n_Volume = 1; // 1
static const long long M1_skp_t_Volume = 1; // 1

#define M1_skp_USES(X) X(st) X(pz) X(rz) X(rc) X(cl) X(run) X(c4) X(rt)

struct M1_skp_T {
  cm_cnt2s M1_skp_A[tc_rows(M1_skp_a_Volume, M1_skp_hA) * M_skp_ND];
  cm_cnt2s M1_skp_B[tc_rows(M1_skp_b_Volume, M1_skp_hB) * M_skp_ND];
  cm_cnt2s M1_skp_C[tc_rows(M1_skp_c_Volume, M1_skp_hC) * M_skp_ND];
  cm_cnt2s M1_skp_D[tc_rows(M1_skp_d_Volume, M1_skp_hD) * M_skp_ND];
  cm_cnt M1_skp_MT[tc_rows(M1_skp_t_Volume, M1_skp_hT) * M_skp_MND];
  i32 M1_skp_W[tc_rows(M1_skp_m_Volume, M1_skp_hM) * M_skp_ND * tc_nin(4)];
  u8 M1_skp_WC[tc_rows(M1_skp_m_Volume, M1_skp_hM) * M_skp_ND];
  i32 M1_skp_WM[tc_rows(M1_skp_n_Volume, M1_skp_hN) * M_skp_MND * TC_MIN];
  u8 M1_skp_WMC[tc_rows(M1_skp_n_Volume, M1_skp_hN) * M_skp_MND];
  cm_cnt M1_skp_G[M1_skp_g_Volume * M_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_skp_a_Volume, M1_skp_hA) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_skp_b_Volume, M1_skp_hB) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_skp_c_Volume, M1_skp_hC) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_skp_d_Volume, M1_skp_hD) * M_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_skp_t_Volume, M1_skp_hT) * M_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_skp_m_Volume, M1_skp_hM) * M_skp_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_skp_m_Volume, M1_skp_hM) * M_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_skp_n_Volume, M1_skp_hN) * M_skp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_skp_n_Volume, M1_skp_hN) * M_skp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_skp_g_Volume * M_skp_NG);
  void M1_skp_Init( void ) {}
  void M1_skp_Quit( void ) {}
};
