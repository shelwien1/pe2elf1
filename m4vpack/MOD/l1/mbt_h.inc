

static const byte M1_mbt_b_rc[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };

static const byte M1_mbt_c_rc[10+1]={ 0,1,1,2,3,3,3,3,3,3,3 };

static const byte M1_mbt_d_c4[4+1]={ 0,1,2,3,4 };






static const int M1_mbt_rA = (71+0) * (1);
static const int M1_mbt_mwA = (38+0) * (1);
static const int M1_mbt_rB = (253+0) * (1);
static const int M1_mbt_mwB = (4+0) * (1);
static const int M1_mbt_rC = (0+0) * (1);
static const int M1_mbt_mwC = (130+0) * (1);
static const int M1_mbt_rD = (255+0) * (1);
static const int M1_mbt_mwD = (106+0) * (1);
static const int M1_mbt_lr = (13+0) * (1);
static const int M1_mbt_mb = (44+0) * (1);
static const int M1_mbt_rT = (0+0) * (1);
static const int M1_mbt_mwT = (0+0) * (1);
static const int M1_mbt_mt = (0+0) * (1);
static const int M1_mbt_lrm = (0+0) * (1);
static const int M1_mbt_mbm = (0+0) * (1);
static const int M1_mbt_rG = (0+0) * (1);
static const int M1_mbt_mwG = (0+0) * (1);
static const int M1_mbt_hA = (22+0) * (1);
static const int M1_mbt_hB = (22+0) * (1);
static const int M1_mbt_hC = (22+0) * (1);
static const int M1_mbt_hD = (22+0) * (1);
static const int M1_mbt_hM = (22+0) * (1);
static const int M1_mbt_hT = (22+0) * (1);
static const int M1_mbt_hN = (22+0) * (1);


static const long long M1_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3); // 18
static const long long M1_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 3); // 15
static const long long M1_mbt_c_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long M1_mbt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 5); // 15
static const long long M1_mbt_g_Volume = 1; // 1
static const long long M1_mbt_m_Volume = tc_vmul(1, 1); // 1
static const long long M1_mbt_n_Volume = 1; // 1
static const long long M1_mbt_t_Volume = 1; // 1

#define M1_mbt_USES(X) X(st) X(rt) X(c4) X(rc) X(cl)

struct M1_mbt_T {
  cm_cnt2s M1_mbt_A[tc_rows(M1_mbt_a_Volume, M1_mbt_hA) * M_mbt_ND];
  cm_cnt2s M1_mbt_B[tc_rows(M1_mbt_b_Volume, M1_mbt_hB) * M_mbt_ND];
  cm_cnt2s M1_mbt_C[tc_rows(M1_mbt_c_Volume, M1_mbt_hC) * M_mbt_ND];
  cm_cnt2s M1_mbt_D[tc_rows(M1_mbt_d_Volume, M1_mbt_hD) * M_mbt_ND];
  cm_cnt M1_mbt_MT[tc_rows(M1_mbt_t_Volume, M1_mbt_hT) * M_mbt_MND];
  i32 M1_mbt_W[tc_rows(M1_mbt_m_Volume, M1_mbt_hM) * M_mbt_ND * tc_nin(4)];
  u8 M1_mbt_WC[tc_rows(M1_mbt_m_Volume, M1_mbt_hM) * M_mbt_ND];
  i32 M1_mbt_WM[tc_rows(M1_mbt_n_Volume, M1_mbt_hN) * M_mbt_MND * TC_MIN];
  u8 M1_mbt_WMC[tc_rows(M1_mbt_n_Volume, M1_mbt_hN) * M_mbt_MND];
  cm_cnt M1_mbt_G[M1_mbt_g_Volume * M_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mbt_a_Volume, M1_mbt_hA) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mbt_b_Volume, M1_mbt_hB) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mbt_c_Volume, M1_mbt_hC) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_mbt_d_Volume, M1_mbt_hD) * M_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_mbt_t_Volume, M1_mbt_hT) * M_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mbt_m_Volume, M1_mbt_hM) * M_mbt_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mbt_m_Volume, M1_mbt_hM) * M_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_mbt_n_Volume, M1_mbt_hN) * M_mbt_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_mbt_n_Volume, M1_mbt_hN) * M_mbt_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_mbt_g_Volume * M_mbt_NG);
  void M1_mbt_Init( void ) {}
  void M1_mbt_Quit( void ) {}
};
